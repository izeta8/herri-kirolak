// Pala: el partido en el frontón (prototipo, frontón provisional).
//  - Entrenamendua: un pelotari pelotea contra el frontis; cuenta los golpes buenos seguidos.
//  - Lehiaketa: 1v1 (con 1 jugador, contra la CPU). Gana quien llega antes a 10/15/22 tantos.
// Movimiento con el stick del Nunchuk (obligatorio); golpe con el latigazo del Wii Remote. El latigazo decide el
// momento (pronto = hacia el ancho, tarde = a la pared) y la fuerza (fuerte = rápida y larga, suave = dejada).
// Derecha o revés según por dónde llega la pelota. Saque: A para botarla y latigazo tras el bote.
#include <stdio.h>
#include <string.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/tuning.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "game/world.h"
#include "games/pala/ball.h"
#include "games/pala/fronton.h"
#include "games/pala/pelotari.h"
#include "scenes/scenes.h"

using namespace tuning;

namespace {

enum State { ST_INTRO, ST_SERVE, ST_RALLY, ST_POINT, ST_FINISH, ST_RESULTS };

const int MAXP = 2;
const float HIST_DT = 0.01f;
const int HIST_N = 32;  // 0.32 s de pelota hacia atrás (golpes tardíos)

struct FloatLabel {
    char text[32];
    Vec3 pos;
    float t;
    Color color;
};

struct Player {
    int pad = 0;
    bool bot = false;
    const Profile* prof = nullptr;
    Pelotari pel;
    int score = 0;
    // golpe que llega pronto: se resuelve cuando la pelota llega a su lado
    bool pending = false;
    float pendingT = 0, pendingTiming = 0, pendingStrength = 0;
    float lastTiming = 0, lastStrength = 0, lastShow = 0;  // para el indicador del último golpe
    // CPU
    bool botPlanned = false;
    Vec3 botContact;
    float botContactT = 0, botNoise = 0, botStrength = 0.7f, botWait = 0;
};

class PalaScene : public Scene {
public:
    explicit PalaScene(const MatchConfig& cfg) : cfg_(cfg) {
        fronton_.build();
        n_ = cfg_.training ? 1 : 2;
        target_ = palaPoints(cfg_.rounds);
        for (int i = 0; i < n_; i++) {
            Player& p = pl_[i];
            p.pad = i;
            // Lehiaketa con un jugador: el segundo es la CPU
            p.bot = cfg_.bot[i] || (!cfg_.training && cfg_.numPlayers < 2 && i == 1);
            p.prof = &save::data().profiles[cfg_.profile[i] % MAX_PROFILES];
            p.pel.setup(Appearance::from(*p.prof));
        }
        server_ = 0;
        resetPositions();
        camEye_ = Vec3(8, 9, COURT_LEN + 6);
        camTarget_ = Vec3(0, 3, 10);
    }

    void update(float dt) override;
    void draw() override;

private:
    int padFor(const Player& p) const { return input::pad(p.pad).connected ? p.pad : 0; }
    int other(int i) const { return n_ == 1 ? i : 1 - i; }
    bool needsNunchuk() const;
    void resetPositions();
    void startDrop(int i);
    void updatePlayer(int i, float dt);
    void updateBot(int i, float dt);
    void tryHit(int i, float strength);
    void doHit(int i, float timing, float strength);
    void onBallEvent(const BallEvent& e);
    void endPoint(int loser, const char* why, const Vec3& at);
    void addLabel(const char* t, const Vec3& pos, Color c);
    void enterResults();
    void updateCamera(float dt);
    void drawHud();
    void drawResults();
    void drawPause();
    // Momento en que la pelota pasa más cerca del pelotari i (s; < 0 = ya ha pasado) y a qué distancia
    bool closest(int i, float& tau, float& dist, Vec3& at) const;

    MatchConfig cfg_;
    Fronton fronton_;
    int n_ = 1, target_ = 15;
    Player pl_[MAXP];
    Ball ball_;
    BallPath path_;
    Vec3 hist_[HIST_N];
    bool histOk_[HIST_N];
    int histHead_ = 0;
    float histAcc_ = 0;
    State state_ = ST_INTRO;
    float stateT_ = 0, time_ = 0;
    bool rally_ = false;     // la jugada está viva (se aplican las reglas)
    bool serving_ = false;   // la pelota en vuelo es un saque (tiene que botar entre falta y pasa)
    int server_ = 0, hitter_ = 0, mustHit_ = 0, lastWinner_ = -1;
    int streak_ = 0, best_ = 0;
    FloatLabel labels_[8];
    int numLabels_ = 0;
    float shake_ = 0, hitstop_ = 0;
    Vec3 camEye_, camTarget_;
    bool paused_ = false;
    ui::ListMenu pauseMenu_, resultsMenu_;
    float resultsT_ = 0;
};

bool PalaScene::needsNunchuk() const {
    for (int i = 0; i < n_; i++)
        if (!pl_[i].bot && !input::pad(padFor(pl_[i])).nunchuk) return true;
    return false;
}

void PalaScene::resetPositions() {
    for (int i = 0; i < n_; i++) {
        Player& p = pl_[i];
        bool srv = i == server_;
        p.pel.pos = srv ? Vec3(-0.5f, 0, SERVE_Z) : Vec3(1.2f, 0, RECEIVE_Z);
        p.pel.vel = Vec3();
        p.pel.celebrate(false);
        p.pending = false;
        p.botPlanned = false;
        p.botWait = frand(0.8f, 1.4f);
    }
    ball_.live = false;
    rally_ = false;
    serving_ = false;
    mustHit_ = hitter_ = server_;
}

void PalaScene::addLabel(const char* t, const Vec3& pos, Color c) {
    if (numLabels_ == 8) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * 7);
        numLabels_--;
    }
    FloatLabel& l = labels_[numLabels_++];
    strncpy(l.text, t, sizeof(l.text) - 1);
    l.text[sizeof(l.text) - 1] = 0;
    l.pos = pos;
    l.t = 0;
    l.color = c;
}

// Saque: el pelotari suelta la pelota a su derecha; tiene que darle tras el primer bote
void PalaScene::startDrop(int i) {
    Player& p = pl_[i];
    p.pel.dropBall();
    ball_ = Ball();
    ball_.live = true;
    ball_.drop = true;
    ball_.pos = p.pel.dropPoint();
    ball_.vel = Vec3(0, DROP_VY, 0);
    rally_ = true;
    serving_ = false;
    hitter_ = mustHit_ = i;
    state_ = ST_RALLY;
    stateT_ = 0;
    for (int k = 0; k < n_; k++) pl_[k].botPlanned = false;
}

bool PalaScene::closest(int i, float& tau, float& dist, Vec3& at) const {
    const Vec3& me = pl_[i].pel.pos;
    float best = 1e9f;
    bool found = false;
    auto consider = [&](const Vec3& b, float t) {
        if (b.y < REACH_Y_MIN || b.y > REACH_Y_MAX) return;
        float d = length(Vec3(b.x - me.x, 0, b.z - me.z));
        // A igualdad de distancia, mejor lo más cercano a ahora
        float score = d + fabsf(t) * 0.5f;
        if (score < best) {
            best = score;
            tau = t;
            dist = d;
            at = b;
            found = true;
        }
    };
    // Lo que ya ha pasado (golpe tarde)…
    for (int k = 1; k < HIST_N; k++) {
        int idx = (histHead_ - k + HIST_N) % HIST_N;
        if (histOk_[idx]) consider(hist_[idx], -k * HIST_DT);
        if (k * HIST_DT >= HIT_LATE) break;
    }
    // …lo de ahora y lo que viene (golpe pronto)
    if (ball_.hittable()) consider(ball_.pos, 0);
    for (int k = 0; k < path_.n && (k + 1) * BallPath::DT <= HIT_EARLY; k++)
        if (path_.ok[k]) consider(path_.p[k], (k + 1) * BallPath::DT);
    return found;
}

void PalaScene::tryHit(int i, float strength) {
    Player& p = pl_[i];
    int side = ball_.live && ball_.pos.x < p.pel.pos.x ? -1 : 1;
    float h = ball_.live ? ball_.pos.y : 1.0f;
    if (!rally_ || mustHit_ != i || p.pending) {
        p.pel.swing(side, h);  // golpe al aire
        return;
    }
    float tau = 0, dist = 0;
    Vec3 at;
    if (closest(i, tau, dist, at) && dist <= REACH) {
        side = at.x < p.pel.pos.x ? -1 : 1;
        p.pel.swing(side, at.y);
        float timing = clampv(tau / TIMING_WINDOW, -TIMING_MAX, TIMING_MAX);
        if (tau > 0.02f) {
            p.pending = true;
            p.pendingT = tau;
            p.pendingTiming = timing;
            p.pendingStrength = strength;
        } else {
            doHit(i, timing, strength);
        }
        return;
    }
    p.pel.swing(side, h);
    audio::play(SFX_WHOOSH, 0.5f, frand(1.0f, 1.2f));
    if (ball_.live && length(ball_.pos - p.pel.pos) < 4.0f) addLabel(txt::MISS, p.pel.pos + Vec3(0, 2.0f, 0), rgba(0xD0, 0xD0, 0xD0));
}

// Lanza la pelota hacia el frontis según el momento (timing) y la fuerza del golpe
void PalaScene::doHit(int i, float timing, float strength) {
    Player& p = pl_[i];
    bool serve = ball_.drop;
    Vec3 P = ball_.pos;
    float speed = lerpf(SHOT_SPEED_MIN, SHOT_SPEED_MAX, strength);
    float depth = serve ? lerpf(SERVE_DEPTH_MIN, SERVE_DEPTH_MAX, strength)
                        : lerpf(SHOT_DEPTH_MIN, SHOT_DEPTH_MAX, strength) + frand(-1, 1) * SHOT_DEPTH_RAND;
    float lx = SHOT_X_MID + timing * SHOT_X_SPAN + frand(-1, 1) * SHOT_X_RAND;
    if (serve) lx = SHOT_X_MID + timing * SHOT_X_SPAN * 0.6f;

    // Altura en el frontis que hace botar la pelota a esa distancia al volver
    float tf = fmaxf(P.z, 0.6f) / speed, best = 1e9f, vy0 = 0, tr = 1;
    for (float ty = CHAPA_H + 0.25f; ty <= 7.5f; ty += 0.05f) {
        float v0 = (ty - P.y) / tf + 0.5f * BALL_G * tf;
        float vy2 = (v0 - BALL_G * tf) * FRONTIS_F;
        float t = (vy2 + sqrtf(fmaxf(0.0f, vy2 * vy2 + 2 * BALL_G * ty))) / BALL_G;
        float err = fabsf(speed * FRONTIS_E * t - depth);
        if (err < best) {
            best = err;
            vy0 = v0;
            tr = t;
        }
    }
    float vx = (lx - P.x) / (tf + FRONTIS_F * tr);
    ball_.vel = Vec3(vx, vy0, -speed);
    ball_.needFrontis = true;
    ball_.bounces = 0;
    ball_.drop = false;
    serving_ = serve;
    hitter_ = i;
    mustHit_ = other(i);
    for (int k = 0; k < n_; k++) pl_[k].botPlanned = false;

    p.lastTiming = timing;
    p.lastStrength = strength;
    p.lastShow = 1.0f;
    float pan = clampv(P.x / 6.0f, -1.0f, 1.0f);
    audio::play(SFX_PALA, 0.6f + 0.4f * strength, frand(0.95f, 1.05f) + 0.1f * strength, pan);
    if (!p.bot) input::rumble(padFor(p), 0.04f + 0.08f * strength);
    if (strength > 0.7f) {
        hitstop_ = 0.03f + 0.03f * strength;
        shake_ = fmaxf(shake_, 0.02f * strength);
    }
}

void PalaScene::endPoint(int loser, const char* why, const Vec3& at) {
    if (!rally_) return;
    rally_ = false;
    addLabel(why, at + Vec3(0, 0.6f, 0), UI_GOLD);
    audio::play(SFX_BAD, 0.5f);
    state_ = ST_POINT;
    stateT_ = 0;
    if (cfg_.training) {
        streak_ = 0;
        return;
    }
    int winner = other(loser);
    pl_[winner].score++;
    lastWinner_ = winner;
    server_ = winner;  // saca el que gana el tanto
    pl_[winner].pel.celebrate(true);
    audio::play(SFX_CLAP, 0.5f);
}

void PalaScene::onBallEvent(const BallEvent& e) {
    const float XR = COURT_W / 2;
    switch (e.type) {
        case BE_FRONTIS:
            audio::play(SFX_FRONTIS, 0.7f, frand(0.95f, 1.05f), clampv(e.at.x / 6.0f, -1.0f, 1.0f));
            if (!rally_ || !e.first) break;
            {
                if (e.at.y < CHAPA_H) endPoint(hitter_, txt::CHAPA, e.at);
                else if (e.at.y > TOP_LINE) endPoint(hitter_, txt::TOO_HIGH, e.at);
                else if (e.at.x > XR) endPoint(hitter_, txt::OUT, e.at);
                else if (cfg_.training) {
                    streak_++;
                    best_ = streak_ > best_ ? streak_ : best_;
                }
            }
            break;
        case BE_FLOOR:
            audio::play(SFX_BOTE, 0.35f, frand(0.95f, 1.1f), clampv(e.at.x / 6.0f, -1.0f, 1.0f));
            if (!rally_) break;
            if (ball_.drop) {
                if (e.bounces >= 2) endPoint(server_, txt::FALTA, e.at);
                break;
            }
            if (e.before) {
                endPoint(hitter_, txt::SHORT, e.at);
                break;
            }
            if (e.bounces == 1) {
                if (e.at.x > XR) endPoint(hitter_, txt::OUT, e.at);
                else if (e.at.z > COURT_LEN) endPoint(hitter_, txt::LONG, e.at);
                else if (serving_ && e.at.z < FALTA_Z) endPoint(hitter_, txt::FALTA, e.at);
                else if (serving_ && e.at.z > PASA_Z) endPoint(hitter_, txt::PASA, e.at);
            } else if (e.bounces >= 2) {
                endPoint(mustHit_, txt::TWO_BOUNCES, e.at);
            }
            break;
        case BE_BACK:
            // Pared del fondo sin botar antes: larga
            if (rally_ && !e.before && e.bounces == 0 && !ball_.drop) endPoint(hitter_, txt::LONG, e.at);
            break;
        case BE_WALL:
            break;
    }
}

void PalaScene::updateBot(int i, float dt) {
    Player& p = pl_[i];
    Pelotari& pel = p.pel;
    Vec3 goal(1.0f, 0, RECEIVE_Z - 1.0f);  // esperar en el centro, atrás
    float speed = PLAYER_SPEED * BOT_SPEED;

    if (state_ == ST_SERVE && server_ == i) {
        goal = pel.pos;
        p.botWait -= dt;
        if (p.botWait <= 0) startDrop(i);
    } else if (rally_ && mustHit_ == i) {
        if (!p.botPlanned) {
            // Buscar dónde darle: tras el bote, a media altura; si no, de volea
            int pick = -1;
            for (int k = 0; k < path_.n; k++) {
                if (!path_.ok[k] || path_.p[k].z > COURT_LEN - 0.5f) continue;
                bool afterBounce = ball_.drop || (path_.landing >= 0 && k > path_.landing);
                if (afterBounce && path_.p[k].y > 0.55f && path_.p[k].y < 1.5f) { pick = k; break; }
            }
            if (pick < 0)
                for (int k = 0; k < path_.n; k++)
                    if (path_.ok[k] && path_.p[k].y > 0.4f && path_.p[k].y < 1.9f) { pick = k; break; }
            if (pick >= 0) {
                p.botPlanned = true;
                p.botContact = path_.p[pick];
                p.botContactT = (pick + 1) * BallPath::DT;
                p.botNoise = frand(-1, 1) * (frand() < BOT_MISTAKE ? BOT_MISTAKE_NOISE : BOT_TIMING_NOISE);
                p.botStrength = frand() < BOT_DROP_SHOT ? frand(0.05f, 0.2f) : frand(0.5f, 0.95f);
            }
        }
        if (p.botPlanned) {
            p.botContactT -= dt;
            // De derecha: la pelota le queda a su derecha (+X)
            goal = p.botContact + Vec3(-0.55f, 0, 0);
            goal.y = 0;
            if (ball_.drop) goal = pel.pos;
            if (p.botContactT <= p.botNoise && !p.pending && !pel.swinging()) {
                tryHit(i, p.botStrength);
                p.botPlanned = false;
                p.botContactT = 99;
            }
        }
    } else if (rally_ && n_ == 2) {
        // Dejar sitio al otro: un poco apartado de por donde va la pelota
        goal = Vec3(ball_.pos.x > 0 ? -2.0f : 2.5f, 0, RECEIVE_Z);
    }
    Vec3 d = goal - pel.pos;
    d.y = 0;
    float len = length(d);
    Vec3 want = len > 0.05f ? d * (fminf(len * 4.0f, speed) / len) : Vec3();
    pel.vel = approachv(pel.vel, want, PLAYER_ACCEL, dt);
}

void PalaScene::updatePlayer(int i, float dt) {
    Player& p = pl_[i];
    Pelotari& pel = p.pel;
    const input::Pad& in = input::pad(padFor(p));
    bool canPlay = state_ == ST_SERVE || state_ == ST_RALLY || state_ == ST_POINT;

    if (canPlay) {
        if (p.bot) {
            updateBot(i, dt);
        } else {
            // Stick: arriba = hacia el frontis (-Z), derecha = +X
            Vec3 want(in.stickX * PLAYER_SPEED, 0, -in.stickY * PLAYER_SPEED);
            if (state_ == ST_SERVE && server_ == i) want = Vec3(want.x, 0, 0);  // al sacar, solo de lado
            pel.vel = approachv(pel.vel, want, PLAYER_ACCEL, dt);
            if (state_ == ST_SERVE && server_ == i && (in.down & WPAD_BUTTON_A)) startDrop(i);
            if (in.whip.happened) tryHit(i, in.whip.strength);
        }
    } else {
        pel.vel = approachv(pel.vel, Vec3(), PLAYER_ACCEL, dt);
    }

    // Golpe que llegó pronto: la pelota llega ahora a su lado
    if (p.pending) {
        p.pendingT -= dt;
        if (p.pendingT <= 0) {
            p.pending = false;
            if (rally_ && mustHit_ == i && ball_.hittable()) doHit(i, p.pendingTiming, p.pendingStrength);
        }
    }

    pel.pos += pel.vel * dt;
    pel.pos.x = clampv(pel.pos.x, -COURT_W / 2 + 0.4f, COURT_W / 2 + 4.0f);
    pel.pos.z = clampv(pel.pos.z, 2.0f, COURT_LEN + 3.0f);

    // Preparar la pala cuando le toca y la pelota viene
    if (rally_ && mustHit_ == i && (ball_.hittable() || (!ball_.needFrontis && !ball_.drop) || ball_.drop)) {
        int side = ball_.pos.x < pel.pos.x ? -1 : 1;
        float near = 1.0f - smooth01((length(ball_.pos - pel.pos) - 3.0f) / 10.0f);
        pel.prepare(side, ball_.pos.y, ball_.vel.z > 0 || ball_.drop ? near : 0);
    } else {
        pel.prepare(1, 1.0f, 0);
    }
    pel.update(dt);
    p.lastShow = fmaxf(0.0f, p.lastShow - dt * 0.6f);
}

void PalaScene::enterResults() {
    state_ = ST_RESULTS;
    stateT_ = 0;
    resultsT_ = 0;
    resultsMenu_ = ui::ListMenu();
    resultsMenu_.count = 2;
    resultsMenu_.horizontal = true;
    audio::play(SFX_CHEER, 0.6f);
}

void PalaScene::update(float dt) {
    time_ += dt;
    u32 downAll = input::allDown();

    if (paused_) {
        int r = pauseMenu_.update(dt, downAll);
        if (r == 0 || r == -2 || (downAll & WPAD_BUTTON_PLUS)) paused_ = false;
        else if (r == 1) scenes::go(makePalaScene(cfg_));
        else if (r == 2) scenes::go(makeMainMenu());
        return;
    }
    if (state_ != ST_RESULTS && (downAll & WPAD_BUTTON_PLUS)) {
        paused_ = true;
        pauseMenu_ = ui::ListMenu();
        pauseMenu_.count = 3;
        input::stopRumble();
        audio::play(SFX_OK, 0.6f);
        return;
    }
    // Sin Nunchuk no se juega: todo quieto hasta que se conecte
    if (state_ != ST_RESULTS && needsNunchuk()) return;

    if (hitstop_ > 0) {
        hitstop_ -= dt;
        return;
    }

    stateT_ += dt;
    switch (state_) {
        case ST_INTRO:
            if (stateT_ > 2.4f || (stateT_ > 0.6f && (downAll & WPAD_BUTTON_A))) {
                state_ = ST_SERVE;
                stateT_ = 0;
                audio::play(SFX_BELL, 0.5f);
            }
            break;
        case ST_SERVE:
        case ST_RALLY:
            break;
        case ST_POINT:
            if (stateT_ > 2.0f) {
                bool over = false;
                for (int i = 0; i < n_; i++) over |= !cfg_.training && pl_[i].score >= target_;
                if (over) {
                    state_ = ST_FINISH;
                    stateT_ = 0;
                    audio::play(SFX_BELL, 0.7f);
                    audio::play(SFX_CHEER, 0.6f);
                } else {
                    resetPositions();
                    state_ = ST_SERVE;
                    stateT_ = 0;
                }
            }
            break;
        case ST_FINISH:
            if (stateT_ > 3.0f || (stateT_ > 1.0f && (downAll & WPAD_BUTTON_A))) enterResults();
            break;
        case ST_RESULTS:
            resultsT_ += dt;
            if (resultsT_ > 0.8f) {
                int r = resultsMenu_.update(dt, downAll);
                if (r == 0) scenes::go(makePalaScene(cfg_));
                else if (r == 1 || r == -2) scenes::go(makeMainMenu());
            }
            break;
    }

    // Pelota: física, reglas e historial (para los golpes tardíos)
    if (ball_.live) {
        BallEvent ev[8];
        int k = ball_.step(dt, ev, 8);
        for (int e = 0; e < k; e++) onBallEvent(ev[e]);
        histAcc_ += dt;
        while (histAcc_ >= HIST_DT) {
            histAcc_ -= HIST_DT;
            histHead_ = (histHead_ + 1) % HIST_N;
            hist_[histHead_] = ball_.pos;
            histOk_[histHead_] = rally_ && ball_.hittable();
        }
        path_.predict(ball_, 2.5f);
        if (!rally_ && state_ == ST_POINT && stateT_ > 1.5f) ball_.live = false;
    } else {
        path_.n = 0;
        path_.landing = -1;
        for (bool& ok : histOk_) ok = false;
    }

    for (int i = 0; i < n_; i++) updatePlayer(i, dt);
    if (n_ == 2) {
        // No se atraviesan: cada uno se aparta la mitad
        Vec3 d = pl_[1].pel.pos - pl_[0].pel.pos;
        d.y = 0;
        float len = length(d);
        if (len < PLAYER_GAP) {
            Vec3 dir = len > 0.01f ? d * (1.0f / len) : Vec3(1, 0, 0);
            Vec3 push = dir * ((PLAYER_GAP - len) * 0.5f);
            pl_[0].pel.pos -= push;
            pl_[1].pel.pos += push;
        }
    }
    for (int i = 0; i < numLabels_; i++) labels_[i].t += dt;
    while (numLabels_ > 0 && labels_[0].t > 1.4f) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * (numLabels_ - 1));
        numLabels_--;
    }
    shake_ = approach(shake_, 0, 10, dt);
    updateCamera(dt);
}

void PalaScene::updateCamera(float dt) {
    // Detrás de los pelotaris, algo alta, mirando al frontis
    Vec3 mid;
    float back = 0;
    for (int i = 0; i < n_; i++) {
        mid += pl_[i].pel.pos * (1.0f / n_);
        back = fmaxf(back, pl_[i].pel.pos.z);
    }
    float z = fminf(back + 7.5f, COURT_LEN + 9.0f);  // puede quedar por encima del rebote
    Vec3 eye(mid.x * 0.45f + 0.5f, 5.2f, z);
    Vec3 target(mid.x * 0.3f, 1.6f, mid.z - 11.0f);
    if (state_ == ST_INTRO) {
        float k = smooth01(stateT_ / 2.2f);
        eye = eye + Vec3((1 - k) * 7.0f, (1 - k) * 5.0f, 0);
    } else if (state_ == ST_FINISH || state_ == ST_RESULTS) {
        eye = eye + Vec3(sinf(time_ * 0.3f) * 2.0f, 0.5f, 0);
    }
    camEye_ = approachv(camEye_, eye, 3.0f, dt);
    camTarget_ = approachv(camTarget_, target, 3.5f, dt);
}

void PalaScene::draw() {
    World& world = sharedWorld();
    gfx::begin2D();
    world.drawSky();
    Camera cam;
    Vec3 jitter(frand(-1, 1) * shake_, frand(-1, 1) * shake_, frand(-1, 1) * shake_);
    cam.eye = camEye_ + jitter;
    cam.target = camTarget_ + jitter * 0.5f;
    cam.fov = 50;
    gfx::begin3D(cam);
    gfx::setFog(true, 60.0f, 200.0f, World::HORIZON);
    fronton_.draw();
    gfx::setFog(false);

    for (int i = 0; i < n_; i++) world.drawShadow(pl_[i].pel.pos, 0.35f, 0.35f);
    if (ball_.live) {
        world.drawShadow(Vec3(ball_.pos.x, 0, ball_.pos.z), 0.09f, 0.09f);
        // Ayuda del prototipo: dónde va a botar
        if (SHOW_LANDING && rally_ && path_.landing >= 0) {
            const Vec3& l = path_.p[path_.landing];
            world.drawShadow(Vec3(l.x, 0, l.z), 0.28f, 0.28f);
        }
    }
    for (int i = 0; i < n_; i++) pl_[i].pel.draw();
    if (ball_.live) {
        static Mesh ballMesh;
        if (!ballMesh.valid()) {
            MeshBuilder b;
            b.sphere(Vec3(), Vec3(1, 1, 1), rgba(0xF4, 0xF2, 0xEA), 12, 8);
            b.build(ballMesh);
        }
        gfx::drawMesh(ballMesh, Xf().translate(ball_.pos).scale(BALL_DRAW_R));
    }

    gfx::begin2D();
    drawHud();
    if (state_ == ST_RESULTS) drawResults();
    if (paused_) drawPause();
}

void PalaScene::drawHud() {
    const float W = gfx::screenW();
    char buf[64];
    // Etiquetas flotantes
    for (int i = 0; i < numLabels_; i++) {
        const FloatLabel& l = labels_[i];
        float sx, sy;
        if (!gfx::project(l.pos, sx, sy)) continue;
        float pop = easeOutBack(clampv(l.t / 0.2f, 0.0f, 1.0f));
        float a = 1.0f - smooth01((l.t - 1.0f) / 0.4f);
        font::draw(l.text, sx, sy - 30 - l.t * 30, 40 * pop + 2, l.color, rgba(0x30, 0x30, 0x38), ALIGN_CENTER, a);
    }
    if (state_ == ST_RESULTS) return;

    // Marcador
    if (cfg_.training) {
        ui::panel(34, 26, 230, 74);
        font::plain(txt::STREAK, 54, 32, 22, UI_TEXT_LIGHT);
        snprintf(buf, sizeof(buf), "%d", streak_);
        font::plain(buf, 54, 54, 40, UI_TEXT);
        font::plain(txt::BEST, 244, 32, 22, UI_TEXT_LIGHT, ALIGN_RIGHT);
        snprintf(buf, sizeof(buf), "%d", best_);
        font::plain(buf, 244, 54, 40, UI_TEXT, ALIGN_RIGHT);
    } else {
        float pw = 440, px = W / 2 - pw / 2, py = 20;
        ui::panel(px, py, pw, 70);
        for (int i = 0; i < 2; i++) {
            const Player& p = pl_[i];
            bool left = i == 0;
            float nx = left ? px + 44 : px + pw - 44;
            ui::colorDot(left ? px + 24 : px + pw - 24, py + 26, 9, FAV_COLORS[p.prof->color % NUM_COLORS]);
            font::plain(p.prof->name, nx, py + 12, 26, UI_TEXT, left ? ALIGN_LEFT : ALIGN_RIGHT);
            if (p.bot) font::plain(txt::CPU, nx, py + 42, 16, UI_TEXT_LIGHT, left ? ALIGN_LEFT : ALIGN_RIGHT);
            if (server_ == i) gfx::circle(left ? px + 24 : px + pw - 24, py + 52, 6, rgba(0xF4, 0xF2, 0xEA), 12);
            snprintf(buf, sizeof(buf), "%d", p.score);
            font::draw(buf, W / 2 + (left ? -36 : 36), py + 10, 46, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER);
        }
        font::plain("-", W / 2, py + 14, 40, UI_TEXT, ALIGN_CENTER);
        snprintf(buf, sizeof(buf), txt::POINTS_N, target_);
        font::plain(buf, W / 2, py + 74, 18, UI_WHITE, ALIGN_CENTER);
    }

    // Flecha sobre quien tiene que dar
    if (rally_ && n_ > 1) {
        const Player& p = pl_[mustHit_];
        float sx, sy;
        if (gfx::project(p.pel.pos + Vec3(0, 2.1f, 0), sx, sy)) {
            float b = 4 * sinf(time_ * 8);
            Color c = FAV_COLORS[p.prof->color % NUM_COLORS];
            gfx::triangle(sx - 12, sy - 18 + b, sx + 12, sy - 18 + b, sx, sy + b, rgba(0x20, 0x30, 0x40));
            gfx::triangle(sx - 8, sy - 16 + b, sx + 8, sy - 16 + b, sx, sy - 4 + b, c);
        }
    }

    // Último golpe del jugador: momento (goiz / berandu) y fuerza
    for (int i = 0; i < n_; i++) {
        const Player& p = pl_[i];
        if (p.bot || p.lastShow <= 0) continue;
        float cx = n_ == 1 ? W / 2 : (i == 0 ? W * 0.27f : W * 0.73f), cy = 440, w = 220;
        float a = smooth01(p.lastShow * 3);
        gfx::roundRect(cx - w / 2 - 6, cy - 8, w + 12, 16, 8, withAlpha(rgba(0, 0, 0, 90), a));
        gfx::roundRect(cx - w / 2, cy - 4, w, 8, 4, withAlpha(rgba(255, 255, 255, 170), a));
        gfx::rect(cx - 1, cy - 9, 2, 18, withAlpha(UI_GREEN, a));
        float mx = cx + clampv(p.lastTiming, -1.0f, 1.0f) * w / 2;  // pronto a la derecha (hacia el ancho)
        gfx::circle(mx, cy, 9, withAlpha(mixColor(UI_GOLD, UI_RED, p.lastStrength), a), 16);
        font::plain(txt::LATE, cx - w / 2 - 12, cy - 12, 18, withAlpha(UI_WHITE, a), ALIGN_RIGHT);
        font::plain(txt::EARLY, cx + w / 2 + 12, cy - 12, 18, withAlpha(UI_WHITE, a), ALIGN_LEFT);
    }

    // Saque
    if (state_ == ST_SERVE && !pl_[server_].bot) {
        float s = 1.0f + 0.04f * sinf(time_ * 6);
        font::draw(txt::SERVE, W / 2, 130, 48 * s, UI_GOLD, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
        float tw = font::width(txt::SERVE_HOW, 28);
        font::draw(txt::SERVE_HOW, W / 2 + 14, 186, 28, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER);
        ui::buttonIcon(W / 2 - tw / 2 - 8, 202, "A", 15);
    }
    if (state_ == ST_POINT && !cfg_.training && lastWinner_ >= 0 && stateT_ > 0.5f) {
        snprintf(buf, sizeof(buf), txt::POINT_FOR, pl_[lastWinner_].prof->name);
        font::draw(buf, W / 2, 140, 44, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, smooth01((stateT_ - 0.5f) / 0.3f));
    }
    if (state_ == ST_INTRO) {
        float a = smooth01(stateT_ / 0.4f) * (1 - smooth01((stateT_ - 2.0f) / 0.4f));
        font::draw(cfg_.training ? txt::TRAINING : txt::COMPETITION, W / 2, 130, 56, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, a);
        if (cfg_.training) snprintf(buf, sizeof(buf), "%s", txt::PALA_TRAINING_SUB);
        else snprintf(buf, sizeof(buf), txt::POINTS_N, target_);
        font::draw(buf, W / 2, 196, 30, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, a);
    } else if (state_ == ST_FINISH) {
        float s = easeOutBack(clampv(stateT_ / 0.3f, 0.0f, 1.0f));
        int w = pl_[0].score >= pl_[1].score ? 0 : 1;
        snprintf(buf, sizeof(buf), "%s: %s!", txt::WINNER, pl_[w].prof->name);
        font::draw(buf, W / 2, 130, 52 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER);
    }

    if (needsNunchuk()) {
        ui::fade(0.45f);
        ui::panel(W / 2 - 260, 170, 520, 130);
        font::plain(txt::CONNECT_NUNCHUK, W / 2, 190, 36, UI_TEXT, ALIGN_CENTER);
        font::plain(txt::NUNCHUK_NEEDED, W / 2, 246, 22, UI_TEXT_LIGHT, ALIGN_CENTER);
    }
}

void PalaScene::drawResults() {
    const float W = gfx::screenW();
    char buf[64];
    float a = smooth01(resultsT_ / 0.4f);
    ui::fade(0.35f * a);
    float pw = 560, ph = 70 + 2 * 56 + 52;
    float px = W / 2 - pw / 2, py = 60 + (1 - easeOutCubic(resultsT_ / 0.5f)) * 60;
    ui::panel(px, py, pw, ph);
    font::plain(txt::RESULTS, W / 2, py + 14, 36, UI_TEXT, ALIGN_CENTER);
    int first = pl_[0].score >= pl_[1].score ? 0 : 1;
    for (int r = 0; r < 2; r++) {
        const Player& p = pl_[r == 0 ? first : 1 - first];
        float y = py + 66 + r * 56;
        if (r == 0) gfx::roundRect(px + 16, y - 4, pw - 32, 50, 16, rgba(0xFF, 0xF0, 0xB8));
        snprintf(buf, sizeof(buf), "%d.", r + 1);
        font::plain(buf, px + 34, y + 4, 30, r == 0 ? rgba(0xD8, 0x98, 0x10) : UI_TEXT_LIGHT);
        ui::colorDot(px + 92, y + 21, 10, FAV_COLORS[p.prof->color % NUM_COLORS]);
        font::plain(p.prof->name, px + 114, y + 4, 30, UI_TEXT);
        snprintf(buf, sizeof(buf), "%d", p.score);
        font::plain(buf, px + pw - 36, y + 4, 30, UI_TEXT, ALIGN_RIGHT);
    }
    snprintf(buf, sizeof(buf), "%s: %s!", txt::WINNER, pl_[first].prof->name);
    font::draw(buf, W / 2, py + ph - 50, 34, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER, a);
    float by = fminf(430.0f, py + ph + 44);
    ui::button(W / 2 - 150, by, 260, 60, txt::PLAY_AGAIN, resultsMenu_.anim[0]);
    ui::button(W / 2 + 150, by, 260, 60, txt::TO_MENU, resultsMenu_.anim[1]);
}

void PalaScene::drawPause() {
    const float W = gfx::screenW();
    ui::fade(0.5f);
    float pw = 420, ph = 330;
    ui::panel(W / 2 - pw / 2, 240 - ph / 2, pw, ph);
    font::plain(txt::PAUSE, W / 2, 240 - ph / 2 + 18, 38, UI_TEXT, ALIGN_CENTER);
    const char* items[3] = {txt::CONTINUE, txt::RESTART, txt::TO_MENU};
    for (int i = 0; i < 3; i++) ui::button(W / 2, 175 + i * 76, 320, 62, items[i], pauseMenu_.anim[i]);
}

}  // namespace

Scene* makePalaScene(const MatchConfig& cfg) { return new PalaScene(cfg); }
