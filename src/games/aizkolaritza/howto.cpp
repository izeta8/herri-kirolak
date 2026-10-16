#include "games/aizkolaritza/howto.h"
#include <wiiuse/wpad.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/remote3d.h"
#include "game/text.h"

namespace {

using remote3d::RL;
using remote3d::RD;
const int NUM_STEPS = 5;
const float PERIOD[NUM_STEPS] = {2.6f, 2.8f, 2.4f, 3.4f, 3.2f};

HowTo::Pose lerpPose(const HowTo::Pose& a, const HowTo::Pose& b, float t) {
    return {lerpf(a.x, b.x, t), lerpf(a.y, b.y, t), lerpf(a.roll, b.roll, t), lerpf(a.pitch, b.pitch, t), lerpf(a.yaw, b.yaw, t)};
}
const HowTo::Pose REST = {0, -0.07f, 0, 1.1f, 0};
HowTo::Pose top(int s) { return {s * 0.06f, 0.0f, -s * 0.35f, -0.25f, 0}; }
HowTo::Pose hit(int s) { return {-s * 0.03f, -0.045f, s * 0.2f, 0.85f, 0}; }
inline float easeIn(float t) { t = clampv(t, 0.0f, 1.0f); return t * t; }
inline bool crossed(float prev, float now, float th) { return prev < th && now >= th; }

// Línea gruesa de (x0,y0) a (x1,y1) con alfa distinto en cada punta (para la estela)
void strip(float x0, float y0, float x1, float y1, float w0, float w1, Color c0, Color c1) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    if (l < 0.5f) return;
    float nx = -dy / l, ny = dx / l;
    gfx::setBlend(true);
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
    GX_Position3f32(x0 + nx * w0, y0 + ny * w0, 0); GX_Color1u32(c0);
    GX_Position3f32(x1 + nx * w1, y1 + ny * w1, 0); GX_Color1u32(c1);
    GX_Position3f32(x1 - nx * w1, y1 - ny * w1, 0); GX_Color1u32(c1);
    GX_Position3f32(x0 - nx * w0, y0 - ny * w0, 0); GX_Color1u32(c0);
    GX_End();
}

}  // namespace

void HowTo::start() {
    total_ = 0;
    touched_ = false;
    setStep(0);
}

void HowTo::setStep(int s) {
    step_ = clampv(s, 0, NUM_STEPS - 1);
    t_ = 0;
    loops_ = 0;
    prevU_ = 0;
    depthL_ = depthR_ = step_ == 4 ? 1.0f : 0.0f;
    shake_ = jo_ = hitText_ = gauge_ = flip_ = 0;
    lastSide_ = -1;
    trailN_ = 0;
    swinging_ = false;
    for (Chip& c : chips_) c.life = 0;
}

bool HowTo::update(float dt, unsigned down) {
    t_ += dt;
    total_ += dt;
    if (t_ > 0.25f && (down & WPAD_BUTTON_PLUS)) {  // saltar el tutorial
        audio::play(SFX_OK, 0.7f);
        return true;
    }
    if (t_ > 0.25f && (down & WPAD_BUTTON_A)) {
        touched_ = true;
        if (step_ == NUM_STEPS - 1) {
            audio::play(SFX_OK, 0.7f);
            return true;
        }
        audio::play(SFX_MOVE, 0.7f);
        setStep(step_ + 1);
        return false;
    }
    if ((down & WPAD_BUTTON_B) && step_ > 0) {
        touched_ = true;
        audio::play(SFX_BACK, 0.6f);
        setStep(step_ - 1);
        return false;
    }
    float per = PERIOD[step_];
    if (t_ >= per * (loops_ + 1)) loops_++;
    // Si nadie toca nada, pasa solo al siguiente paso después de dos vueltas
    if (!touched_ && loops_ >= 2 && step_ < NUM_STEPS - 1) {
        setStep(step_ + 1);
        audio::play(SFX_MOVE, 0.4f);
        return false;
    }
    float u = fmodf(t_, per) / per;
    stepPose(pose_, u, prevU_);
    prevU_ = u;

    shake_ = fmaxf(0.0f, shake_ - dt * 4);
    hitText_ = fmaxf(0.0f, hitText_ - dt * 1.4f);
    for (Chip& c : chips_) {
        if (c.life <= 0) continue;
        c.vy += 900 * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
        c.life -= dt;
    }
    return false;
}

void HowTo::stepPose(Pose& p, float u, float up) {
    bool wrapped = u < up;
    if (wrapped) up = 0;
    swinging_ = false;
    auto impact = [this](int side) {
        if (side < 0) depthL_ = fminf(1.0f, depthL_ + (step_ == 2 ? 0.5f : 0.34f));
        else depthR_ = fminf(1.0f, depthR_ + 0.34f);
        shake_ = 1;
        hitText_ = 1;
        lastSide_ = side;
        audio::play(SFX_CHOP, 0.6f, 1.0f, side < 0 ? -0.4f : 0.4f);
        for (int i = 0; i < 10; i++) {
            Chip& c = chips_[(i + (int)(total_ * 60)) % 24];
            c.x = side * 14.0f;
            c.y = 0;
            c.vx = side * frand(60, 220);
            c.vy = -frand(180, 380);
            c.life = frand(0.5f, 0.9f);
        }
    };
    switch (step_) {
        case 0:  // sujetarlo: tranquilo, girando un poco para que se vea en 3D
            p = {0, -0.035f + 0.006f * sinf(total_ * 2.4f), 0.05f * sinf(total_ * 1.3f), 0.08f * sinf(total_ * 1.7f),
                 0.35f * sinf(total_ * 0.9f)};
            break;
        case 1:  // subir por un lado hasta que sale JO!
            if (u < 0.15f) {
                p = REST;
                gauge_ = 0;
                jo_ = 0;
            } else if (u < 0.6f) {
                float k = smooth01((u - 0.15f) / 0.45f);
                p = lerpPose(REST, top(-1), k);
                gauge_ = k;
                swinging_ = true;
            } else if (u < 0.88f) {
                p = top(-1);
                gauge_ = 1;
                jo_ = clampv((u - 0.6f) / 0.08f, 0.0f, 1.0f);
                if (crossed(up, u, 0.6f)) audio::play(SFX_BEEP, 0.5f, 1.3f);
            } else {
                float k = smooth01((u - 0.88f) / 0.12f);
                p = lerpPose(top(-1), REST, k);
                gauge_ = 1 - k;
                jo_ = 0;
            }
            break;
        case 2:  // golpe en diagonal
            if (wrapped && depthL_ >= 1.0f) depthL_ = 0;
            if (u < 0.3f) {
                p = top(-1);
                jo_ = 1;
            } else if (u < 0.42f) {
                p = lerpPose(top(-1), hit(-1), easeIn((u - 0.3f) / 0.12f));
                jo_ = 0;
                swinging_ = true;
                if (crossed(up, u, 0.3f)) audio::play(SFX_WHOOSH, 0.5f);
            } else if (u < 0.75f) {
                p = hit(-1);
                if (crossed(up, u, 0.42f)) impact(-1);
            } else {
                p = lerpPose(hit(-1), top(-1), smooth01((u - 0.75f) / 0.25f));
            }
            break;
        case 3: {  // ezkerra, eskuina: la V
            if (wrapped && depthL_ >= 1.0f && depthR_ >= 1.0f) depthL_ = depthR_ = 0;
            int half = u < 0.5f ? 0 : 1, side = half == 0 ? -1 : 1;
            float v = fmodf(u * 2, 1.0f), vp = (int)(up * 2) == half ? fmodf(up * 2, 1.0f) : 0;
            if (v < 0.25f) {
                p = lerpPose(hit(-side), top(side), smooth01(v / 0.25f));
                jo_ = 0;
            } else if (v < 0.45f) {
                p = top(side);
                jo_ = 1;
            } else if (v < 0.57f) {
                p = lerpPose(top(side), hit(side), easeIn((v - 0.45f) / 0.12f));
                jo_ = 0;
                swinging_ = true;
                if (crossed(vp, v, 0.45f)) audio::play(SFX_WHOOSH, 0.5f);
            } else {
                p = hit(side);
                if (crossed(vp, v, 0.57f)) impact(side);
            }
            break;
        }
        case 4:  // darse la vuelta con B
            if (u < 0.25f) p = {0, -0.03f, 0, 0.1f, PI * smooth01(u / 0.25f)};
            else if (u < 0.75f) p = {0, -0.03f, 0, 0.1f, PI};
            else p = {0, -0.03f, 0, 0.1f, PI * (1 - smooth01((u - 0.75f) / 0.25f))};
            flip_ = clampv((u - 0.35f) / 0.3f, 0.0f, 1.0f);
            if (u > 0.9f) flip_ = 0;
            if (crossed(up, u, 0.33f)) audio::play(SFX_OK, 0.5f, 1.2f);
            if (crossed(up, u, 0.4f)) audio::play(SFX_WHOOSH, 0.4f, 0.8f);
            break;
    }
}

// Tronco visto de frente con la V; `flip_` lo gira para enseñar el otro lado
void HowTo::drawLog2D(float cx, float cy) {
    float sx = step_ == 4 ? fabsf(cosf(PI * smooth01(flip_))) : 1.0f;
    float dL = depthL_, dR = depthR_;
    if (step_ == 4 && flip_ > 0.5f) dL = dR = 0;  // por detrás todavía no hay corte
    const float w = 230 * sx, h = 52, top = cy - h / 2;
    gfx::roundRect(cx - w / 2 + 4, top + 8, w, h, 24, rgba(0, 0, 0, 50));
    gfx::roundRectGrad(cx - w / 2, top, w, h, 24, rgba(0xA4, 0x6C, 0x3A), rgba(0x6E, 0x44, 0x22));
    for (int i = 0; i < 4; i++) gfx::rect(cx - w / 2 + 20 * sx + i * 52 * sx, top + 12 + (i & 1) * 18, 30 * sx, 3, rgba(0x5A, 0x36, 0x1A, 160));
    if (sx > 0.2f) {
        // V: las dos caras del corte, cada una tan honda como sus golpes
        const float maxD = 40, half = 34 * sx;
        Color in = rgba(0xF0, 0xD2, 0x9C), inDark = rgba(0xD6, 0xAE, 0x72);
        if (dL > 0) gfx::triangle(cx - half * dL - 2, top, cx, top + maxD * dL, cx, top, in);
        if (dR > 0) gfx::triangle(cx, top, cx, top + maxD * dR, cx + half * dR + 2, top, inDark);
    }
    if (step_ == 4 && flip_ > 0 && flip_ < 1) {
        font::draw(txt::TURN_AROUND, cx + 150, top - 130, 30, UI_TEAL, UI_WHITE, ALIGN_CENTER, sinf(PI * flip_));
    }
}

void HowTo::draw() {
    const float W = gfx::screenW();
    gfx::begin2D();
    ui::fade(0.45f, rgba(0x0A, 0x2C, 0x4C));
    ui::header(txt::HOWTO);
    const float px = W * 0.05f, pw = W * 0.9f, py = 90, ph = 320;
    ui::panel(px, py, pw, ph);
    const float sx = px + pw * 0.27f, sy = py + ph * 0.56f;
    // Foco claro detrás del mando
    gfx::circle(sx, sy - 10, 150, rgba(255, 255, 255, 90), 48);
    gfx::circle(sx, sy - 10, 110, rgba(255, 255, 255, 70), 48);

    // Texto del paso
    const float tx = px + pw * 0.54f;
    gfx::circle(tx + 20, py + 52, 20, UI_TEAL, 24);
    char num[2] = {(char)('1' + step_), 0};
    font::plain(num, tx + 20, py + 37, 28, UI_WHITE, ALIGN_CENTER);
    // Que el texto quepa en el panel también en 4:3
    const float textW = px + pw - 24 - tx;
    float ts = fminf(34.0f, 34.0f * (textW - 52) / fmaxf(1.0f, font::width(txt::HOWTO_STEPS[step_][0], 34)));
    float ds = 23.0f;
    for (int k = 1; k <= 2; k++) ds = fminf(ds, 23.0f * textW / fmaxf(1.0f, font::width(txt::HOWTO_STEPS[step_][k], 23)));
    font::draw(txt::HOWTO_STEPS[step_][0], tx + 52, py + 52 - ts * 0.62f, ts, UI_TEAL, UI_WHITE);
    font::plain(txt::HOWTO_STEPS[step_][1], tx, py + 104, ds, UI_TEXT);
    font::plain(txt::HOWTO_STEPS[step_][2], tx, py + 104 + ds * 1.4f, ds, UI_TEXT);
    for (int i = 0; i < NUM_STEPS; i++)
        gfx::circle(tx + 14 + i * 30, py + ph - 42, i == step_ ? 9 : 6, i == step_ ? UI_TEAL : rgba(0xB4, 0xC6, 0xD2), 20);
    // Barra de altura (como la del juego) en los pasos de subir y golpear
    if (step_ == 1 || step_ == 2 || step_ == 3) {
        float gx = sx + 150, gy = sy - 110, gh = 200, g = step_ == 1 ? gauge_ : (jo_ > 0.5f || swinging_ ? 1.0f : 0.3f);
        gfx::roundRect(gx - 9, gy - 3, 18, gh + 6, 9, rgba(255, 255, 255, 200));
        gfx::roundRect(gx - 6, gy + gh * (1 - g), 12, gh * g, 6, g >= 1 ? UI_GREEN : UI_TEAL);
        font::plain(txt::RAISE_LABEL, gx, gy + gh + 8, 16, UI_TEXT_LIGHT, ALIGN_CENTER);
    }
    // Tronco y astillas (solo en los pasos de golpear y de darse la vuelta)
    float shakeX = shake_ * 6 * sinf(total_ * 90), logY = py + ph - 48;
    if (step_ >= 2) drawLog2D(sx + shakeX, logY);
    // Palma de la mano por detrás del mando (posición del cuadro anterior)
    if (step_ == 0 && b12X_ > 0) {
        const Color skin = rgba(0xF2, 0xBC, 0x8C), skinD = rgba(0xDE, 0xA2, 0x74);
        gfx::roundRect(b12X_ - 46, b12Y_ + 4, 92, 82, 34, skinD);
        gfx::roundRect(b12X_ - 42, b12Y_ + 8, 84, 74, 30, skin);
    }
    for (const Chip& c : chips_)
        if (c.life > 0) gfx::rect(sx + c.x - 3, logY - 26 + c.y - 2, 7, 4, rgba(0xF0, 0xD2, 0x9C, (int)(255 * fminf(1.0f, c.life * 2))));
    if (hitText_ > 0) {
        const char* lab = step_ == 3 ? (lastSide_ < 0 ? txt::LEFT : txt::RIGHT) : txt::GREAT;
        float lx = sx + (step_ == 3 ? lastSide_ * 120.0f : -120.0f);
        font::draw(lab, lx, logY - 70 - (1 - hitText_) * 20, 28, UI_GOLD, rgba(0x80, 0x48, 0x00), ALIGN_CENTER, fminf(1.0f, hitText_ * 2));
    }
    // Estela del golpe (de los cuadros anteriores)
    for (int i = 1; i < trailN_; i++) {
        float a0 = (float)(i - 1) / trailN_, a1 = (float)i / trailN_;
        strip(trailX_[i - 1], trailY_[i - 1], trailX_[i], trailY_[i], 3 + 10 * a0, 3 + 10 * a1, rgba(0xB8, 0xF0, 0xFF, (int)(160 * a0)),
              rgba(0xFF, 0xFF, 0xFF, (int)(200 * a1)));
    }

    // Mando en 3D encima del panel
    gfx::clearDepth();
    Camera cam;
    const float dist = 0.62f, fov = 34;
    float halfH = dist * tanf(fov * 0.5f * DEG), halfW = halfH * (W / 480.0f);
    float cx = (0.5f - sx / W) * 2 * halfW, cy = (sy / 480.0f - 0.5f) * 2 * halfH;
    cam.eye = Vec3(cx, cy, dist);
    cam.target = Vec3(cx, cy, 0);
    cam.fov = fov;
    cam.znear = 0.05f;
    gfx::begin3D(cam);
    Xf xf = Xf().translate(Vec3(pose_.x, pose_.y, 0)).rotateY(pose_.yaw).rotateZ(pose_.roll).rotateX(pose_.pitch).translate(Vec3(0, 0.045f, 0));
    gfx::drawMesh(remote3d::mesh(), xf);
    float tipX, tipY, b12X, b12Y, bX, bY;
    gfx::project(xf.apply(Vec3(0, RL / 2, 0)), tipX, tipY);
    gfx::project(xf.apply(Vec3(0, -0.054f, RD / 2)), b12X, b12Y);
    gfx::project(xf.apply(Vec3(0, RL / 2 - 0.05f, -RD / 2 - 0.006f)), bX, bY);
    // Guardar la punta para la estela
    if (swinging_) {
        if (trailN_ == 16) {
            for (int i = 1; i < 16; i++) { trailX_[i - 1] = trailX_[i]; trailY_[i - 1] = trailY_[i]; }
            trailN_--;
        }
        trailX_[trailN_] = tipX;
        trailY_[trailN_] = tipY;
        trailN_++;
    } else if (trailN_ > 0) {
        for (int i = 1; i < trailN_; i++) { trailX_[i - 1] = trailX_[i]; trailY_[i - 1] = trailY_[i]; }
        trailN_--;
    }

    gfx::begin2D();
    float pulse = 0.5f + 0.5f * sinf(total_ * 6);
    b12X_ = b12X;
    b12Y_ = b12Y;
    if (step_ == 0) {
        // Dedos que asoman por los lados y el pulgar encima de 1 y 2
        const Color skin = rgba(0xF2, 0xBC, 0x8C), skinD = rgba(0xDE, 0xA2, 0x74);
        for (int k = 0; k < 3; k++) {
            gfx::roundRect(b12X - 50, b12Y + 16 + k * 20, 22, 17, 8, k & 1 ? skinD : skin);
            gfx::roundRect(b12X + 28, b12Y + 16 + k * 20, 22, 17, 8, k & 1 ? skinD : skin);
        }
        gfx::ring(b12X, b12Y, 22 + 6 * pulse, 26 + 6 * pulse, rgba(0x3C, 0xCC, 0xF4, 220), 32);
        gfx::roundRect(b12X - 13, b12Y - 6, 26, 40, 13, skinD);
        gfx::roundRect(b12X - 11, b12Y - 8, 22, 36, 11, skin);
        font::draw("1 2", b12X + 70, b12Y - 18, 26, UI_TEAL, UI_WHITE, ALIGN_CENTER);
    }
    if ((step_ >= 1 && step_ <= 3) && jo_ > 0) {
        float s = 1.0f + 0.35f * (1 - easeOutBack(jo_));
        // Encima de la barra de altura, como en el juego
        font::draw(txt::HIT_NOW, sx + 150, sy - 152 - 8 * s, 36 * s, UI_GREEN, UI_WHITE, ALIGN_CENTER, fminf(1.0f, jo_ * 2));
    }
    if (step_ == 4) {
        bool back = cosf(pose_.yaw) < 0;
        if (back) {
            gfx::ring(bX, bY, 16 + 6 * pulse, 20 + 6 * pulse, rgba(0x3C, 0xCC, 0xF4, 230), 32);
            ui::buttonIcon(bX + 56, bY - 6, "B", 20 + 3 * pulse, rgba(0x80, 0x88, 0x94));
        }
    }
    ui::hints(step_ == NUM_STEPS - 1 ? txt::GO : txt::HOWTO_NEXT, step_ > 0 ? txt::BACK : nullptr,
              step_ < NUM_STEPS - 1 ? txt::SKIP : nullptr);
}
