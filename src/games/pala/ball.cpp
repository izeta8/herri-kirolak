#include "games/pala/ball.h"
#include "engine/tuning.h"

using namespace tuning;

static const float SUBSTEP = 0.004f;

int Ball::step(float dt, BallEvent* ev, int maxEv) {
    int n = 0;
    auto add = [&](BallEventType t, bool first) {
        if (n < maxEv) ev[n++] = {t, pos, first, bounces, needFrontis};
    };
    const float XL = -COURT_W / 2, BACK = COURT_LEN + 4.0f;
    while (dt > 1e-6f) {
        float h = fminf(dt, SUBSTEP);
        dt -= h;
        vel.y -= BALL_G * h;
        pos += vel * h;
        if (pos.y < BALL_R && vel.y < 0) {
            pos.y = BALL_R;
            vel.y = -vel.y * FLOOR_E;
            if (vel.y < 0.4f) vel.y = 0;  // ya rueda
            vel.x *= FLOOR_F;
            vel.z *= FLOOR_F;
            if (vel.y > 0) {
                if (!needFrontis) bounces++;
                add(BE_FLOOR, false);
            }
        }
        if (pos.z < BALL_R && vel.z < 0) {
            pos.z = BALL_R;
            vel.z = -vel.z * FRONTIS_E;
            vel.x *= FRONTIS_F;
            vel.y *= FRONTIS_F;
            add(BE_FRONTIS, needFrontis);
            if (needFrontis) {
                needFrontis = false;
                bounces = 0;
            }
        }
        if (pos.x < XL + BALL_R && vel.x < 0) {
            pos.x = XL + BALL_R;
            vel.x = -vel.x * WALL_E;
            add(BE_WALL, false);
        }
        if (pos.z > BACK - BALL_R && vel.z > 0) {
            pos.z = BACK - BALL_R;
            vel.z = -vel.z * WALL_E;
            add(BE_BACK, false);
        }
    }
    return n;
}

void BallPath::predict(const Ball& b, float seconds, float sinceBounce) {
    Ball c = b;
    n = 0;
    landing = -1;
    int steps = clampv((int)(seconds / DT), 1, MAX);
    BallEvent ev[4];
    float since = sinceBounce;
    for (int i = 0; i < steps; i++) {
        int k = c.step(DT, ev, 4);
        since += DT;
        for (int e = 0; e < k; e++) {
            if (ev[e].type == BE_FLOOR || ev[e].type == BE_FRONTIS) since = 0;
            if (ev[e].type == BE_FLOOR && landing < 0 && !c.needFrontis && !c.drop && c.bounces == 1) landing = i;
        }
        p[n] = c.pos;
        ok[n] = c.hittable();
        bnc[n] = (unsigned char)clampv(c.bounces, 0, 9);
        sinceB[n] = since;
        n++;
    }
}
