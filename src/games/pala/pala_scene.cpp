// Pala: el partido en el frontón (frontón provisional). Diseño de habilidad: docs/SPEC.md §6.
//  - Entrenamendua: un pelotari pelotea contra el frontis; cuenta los golpes buenos seguidos.
//  - Lehiaketa: 1v1 (con 1 jugador, contra la CPU de nivel Erraza/Normala/Zaila). Gana quien llega antes a 10/15/22.
// Controles (los mismos para el mando y la CPU, en Ctl):
//  - stick del Nunchuk (obligatorio): moverse. Hay que colocarse para que la pelota pase a la distancia de la pala.
//  - Z + stick: el pelotari se planta y mueve la mira por el frontis y la pared izquierda (biparetas).
//  - latigazo del Wii Remote: golpe en el pico. La fuerza = velocidad; el momento y el sitio = calidad (punto dulce).
//  - botón al golpear: B = dejada, nada = a media cancha, A = largo. Saque: A para botarla y latigazo tras el bote.
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
#include "games/pala/pala_howto.h"
#include "games/pala/pelotari.h"
#include "scenes/scenes.h"

using namespace tuning;

namespace {

enum State { ST_INTRO, ST_SERVE, ST_RALLY, ST_POINT, ST_FINISH, ST_RESULTS };
enum ShotType { SHOT_DROP = 0, SHOT_MID = 1, SHOT_LONG = 2 };

const int MAXP = 2;
const float HIST_DT = 0.01f;
const int HIST_N = 32;  // 0.32 s de pelota hacia atrás (golpes tardíos)
const int TRAIL_N = 8;  // estela de la pelota (frames)
const int SWOOSH_N = 9; // estela de la pala (frames)
const float XL = -COURT_W / 2, XR = COURT_W / 2;
const float HOWTO_W = 256;  // ancho de la tarjeta "Nola jo"

// Lo que "aprieta" cada jugador este frame: lo rellena el mando o la CPU
struct Ctl {
    float mx = 0, my = 0;  // stick
    bool z = false, a = false, b = false, aDown = false;
    bool whipStart = false, whip = false;
    float strength = 0;
};

// Instante en que la pelota pasa más cerca del pelotari (pasado o futuro)
struct Contact {
    float tau = 0;  // s respecto a ahora (< 0 = ya ha pasado)
    float dist = 0;
    Vec3 at;
    int bnc = 0;
    float sinceB = 1;
};

struct HitInfo {
    float q = 1, tau = 0, dist = 0, strength = 0.5f, aim = 0;
    int type = SHOT_MID;
    bool volley = false, bp = false;
};

struct FloatLabel {
    char text[32];
    Vec3 pos;
    float t, size;
    Color color;
};

struct Fx {
    Vec3 pos;
    float t = 99, size = 1;
    int kind = 0;  // 0 anillo en el frontis, 1 chispa del golpe
    Color color = 0xFFFFFFFF;
};

struct Player {
    int pad = 0;
    bool bot = false;
    const Profile* prof = nullptr;
    Pelotari pel;
    int score = 0;
    Ctl ctl;
    float aim = 0, aimTarget = 0;  // mira (u, ver tuning AIM_WALL_LEN)
    float aimGlow = 0;
    // golpe que llegó pronto: se resuelve cuando la pelota llega a su lado
    bool pending = false;
    float pendingT = 0;
    HitInfo pendingHit;
    // último golpe (HUD) y estadísticas del partido
    HitInfo last;
    int greats = 0, biparetas = 0;
    float lastShow = 0;
    // estela de la pala
    Vec3 swTip[SWOOSH_N], swMid[SWOOSH_N];
    int swN = 0;
    // CPU
    bool botPlanned = false, botStarted = false;
    Vec3 botContact, botStand;
    float botContactT = 0, botNoise = 0, botStrength = 0.7f, botWait = 0, botAim = 0;
    int botType = SHOT_MID;
};

// Punto de la mira en la pared: u en [-1, 1] frontis, u < -1 pared izquierda
Vec3 aimPoint(float u, float y) {
    if (u >= -1.0f) return Vec3(lerpf(XL + 0.25f, XR - 0.35f, (u + 1) * 0.5f), y, 0.03f);
    return Vec3(XL + 0.03f, y, clampv(-1.0f - u, 0.0f, 1.0f) * AIM_WALL_LEN);
}
float aimFromStick(float sx) { return sx >= 0 ? sx : sx * 2.0f; }
float stickFromAim(float u) { return u >= 0 ? u : u * 0.5f; }
// Altura de la mira dibujada según el tipo de golpe (orientativa)
float aimHeight(int type) { return type == SHOT_DROP ? 1.5f : type == SHOT_LONG ? 4.4f : 2.7f; }
int shotTypeOf(const Ctl& c) { return c.b ? SHOT_DROP : c.a ? SHOT_LONG : SHOT_MID; }

class PalaScene : public Scene {
public:
    explicit PalaScene(const MatchConfig& cfg) : cfg_(cfg) {
        fronton_.build();
        n_ = cfg_.training ? 1 : 2;
        target_ = palaPoints(cfg_.rounds);
#if defined(START_SCENE) && START_SCENE == 15
        target_ = 2;  // prueba: partido corto para ver el final y los resultados
#endif
        level_ = BOT_LEVELS[clampv(cfg_.level, 0, 2)];
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
        showHowTo_ = cfg_.training && !pl_[0].bot;
        if (showHowTo_) howTo_.start();
        camTarget_ = Vec3(0, 1, 16);
        camDist_ = 34;
    }

    void update(float dt) override;
    void draw() override;

private:
    int padFor(const Player& p) const { return input::pad(p.pad).connected ? p.pad : 0; }
    int other(int i) const { return n_ == 1 ? i : 1 - i; }
    bool needsNunchuk() const;
    void resetPositions();
    void startDrop(int i);
    void readHuman(int i);
    void updatePlayer(int i, float dt);
    void updateBot(int i, float dt);
    void tryHit(int i);
    void doHit(int i, const HitInfo& h);
    Vec3 solveShot(const Vec3& P, const Vec3& A, float vh, float depth) const;
    void onBallEvent(const BallEvent& e);
    void endPoint(int loser, const char* why, const Vec3& at);
    void checkRecords();
    void addLabel(const char* t, const Vec3& pos, Color c, float size = 1);
    void addFx(int kind, const Vec3& pos, Color c, float size);
    void enterResults();
    void updateCamera(float dt);
    bool inView(const Vec3& eye, const Vec3& q) const;
    void drawEffects2D();
    void drawHud();
    void drawHowTo();
    void drawResults();
    void drawPause();
    bool closest(int i, Contact& c) const;

    MatchConfig cfg_;
    BotLevel level_;
    Fronton fronton_;
    int n_ = 1, target_ = 15;
    Player pl_[MAXP];
    Ball ball_;
    BallPath path_;
    Vec3 hist_[HIST_N];
    bool histOk_[HIST_N];
    unsigned char histBnc_[HIST_N];
    float histSince_[HIST_N];
    int histHead_ = 0;
    float histAcc_ = 0, sinceBounce_ = 1;
    Vec3 trail_[TRAIL_N];
    int trailN_ = 0;
    State state_ = ST_INTRO;
    float stateT_ = 0, time_ = 0;
    bool rally_ = false;    // la jugada está viva (se aplican las reglas)
    bool serving_ = false;  // la pelota en vuelo es un saque (tiene que botar entre falta y pasa)
    int server_ = 0, hitter_ = 0, mustHit_ = 0, lastWinner_ = -1;
    int rallyHits_ = 0, streak_ = 0, best_ = 0, longestRally_ = 0;
    FloatLabel labels_[8];
    int numLabels_ = 0;
    Fx fx_[12];
    float shake_ = 0, hitstop_ = 0, newRecord_ = 0;
    Vec3 camTarget_, camEyeDraw_, camTargetDraw_;
    float camDist_ = 12;
    mutable float camDistTry_ = 12, camFovTry_ = CAM_FOV;
    float camFov_ = CAM_FOV;
    bool paused_ = false;
    ui::ListMenu pauseMenu_, resultsMenu_;
    float resultsT_ = 0;
    bool showHowTo_ = false;
    PalaHowTo howTo_;
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
        p.pel.pos = srv ? Vec3(-0.6f, 0, SERVE_Z) : Vec3(1.4f, 0, RECEIVE_Z);
        p.pel.vel = Vec3();
        p.pel.setMood(Pelotari::MOOD_NONE);
        p.pending = false;
        p.botPlanned = p.botStarted = false;
        p.botWait = frand(0.9f, 1.5f);
    }
    ball_.live = false;
    rally_ = false;
    serving_ = false;
    rallyHits_ = 0;
    trailN_ = 0;
    mustHit_ = hitter_ = server_;
}

void PalaScene::addLabel(const char* t, const Vec3& pos, Color c, float size) {
    if (numLabels_ == 8) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * 7);
        numLabels_--;
    }
    FloatLabel& l = labels_[numLabels_++];
    strncpy(l.text, t, sizeof(l.text) - 1);
    l.text[sizeof(l.text) - 1] = 0;
    l.pos = pos;
    l.t = 0;
    l.size = size;
    l.color = c;
}

void PalaScene::addFx(int kind, const Vec3& pos, Color c, float size) {
    int k = 0;
    for (int i = 1; i < 12; i++)
        if (fx_[i].t > fx_[k].t) k = i;
    fx_[k] = Fx{pos, 0, size, kind, c};
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
    sinceBounce_ = 1;
    for (int k = 0; k < n_; k++) pl_[k].botPlanned = false;
}

bool PalaScene::closest(int i, Contact& c) const {
    const Vec3& me = pl_[i].pel.pos;
    float best = 1e9f;
    bool found = false;
    auto consider = [&](const Vec3& b, float t, int bnc, float since) {
        if (b.y < REACH_Y_MIN || b.y > REACH_Y_MAX) return;
        float d = length(Vec3(b.x - me.x, 0, b.z - me.z));
        if (d < best) {
            best = d;
            c.tau = t;
            c.dist = d;
            c.at = b;
            c.bnc = bnc;
            c.sinceB = since;
            found = true;
        }
    };
    const float span = HIT_WINDOW * WINDOW_SOFT;
    // Lo que ya ha pasado (golpe tarde)…
    for (int k = 1; k < HIST_N && k * HIST_DT <= span; k++) {
        int idx = (histHead_ - k + HIST_N) % HIST_N;
        if (histOk_[idx]) consider(hist_[idx], -k * HIST_DT, histBnc_[idx], histSince_[idx]);
    }
    // …lo de ahora y lo que viene (golpe pronto)
    if (ball_.hittable()) consider(ball_.pos, 0, ball_.bounces, sinceBounce_);
    for (int k = 0; k < path_.n && (k + 1) * BallPath::DT <= span; k++)
        if (path_.ok[k]) consider(path_.p[k], (k + 1) * BallPath::DT, path_.bnc[k], path_.sinceB[k]);
    return found;
}

// Latigazo: ¿le da? ¿con qué calidad? (punto dulce de tiempo, sitio y altura)
void PalaScene::tryHit(int i) {
    Player& p = pl_[i];
    const Ctl& c = p.ctl;
    int side = ball_.live && ball_.pos.x < p.pel.pos.x ? -1 : 1;
    float h = ball_.live ? ball_.pos.y : 1.0f;
    if (!rally_ || mustHit_ != i || p.pending) {
        p.pel.swing(side, h, 0.05f, c.strength);  // golpe al aire
        return;
    }
    Contact ct;
    bool found = closest(i, ct);
    bool volley = found && !ball_.drop && ct.bnc == 0;
    bool bp = found && !ball_.drop && ct.bnc == 1 && ct.sinceB < BOTE_PRONTO_T;
    float win = lerpf(WINDOW_SOFT, WINDOW_STRONG, c.strength) * (volley || bp ? WINDOW_VOLLEY : 1.0f);
    float hitWin = HIT_WINDOW * win, sweetT = SWEET_TIME * win;
    if (!found || fabsf(ct.tau) > hitWin || ct.dist > REACH) {
        p.pel.swing(side, h, 0.05f, c.strength);
        audio::play(SFX_WHOOSH, 0.5f, frand(1.0f, 1.2f));
        if (ball_.live && length(ball_.pos - p.pel.pos) < 4.0f) addLabel(txt::MISS, p.pel.pos + Vec3(0, 2.0f, 0), rgba(0xD0, 0xD0, 0xD0));
        return;
    }
    float at = fabsf(ct.tau);
    float qt = at <= sweetT ? 1.0f : clampv(1.0f - (at - sweetT) / (hitWin - sweetT), 0.0f, 1.0f);
    float dd = fabsf(ct.dist - SWEET_DIST), tol = SWEET_DIST_TOL * win;
    float qd = dd <= tol ? 1.0f : lerpf(1.0f, SWEET_DIST_MIN_Q, clampv((dd - tol) / SWEET_DIST_FALL, 0.0f, 1.0f));
    float y = ct.at.y;
    float qh = (y >= SWEET_Y_LO || bp) && y <= SWEET_Y_HI ? 1.0f
             : y < SWEET_Y_LO ? lerpf(0.45f, 1.0f, clampv((y - REACH_Y_MIN) / (SWEET_Y_LO - REACH_Y_MIN), 0.0f, 1.0f))
                              : lerpf(1.0f, 0.5f, clampv((y - SWEET_Y_HI) / (REACH_Y_MAX - SWEET_Y_HI), 0.0f, 1.0f));
    HitInfo hit;
    hit.q = qt * qd * qh;
    hit.tau = ct.tau;
    hit.dist = ct.dist;
    hit.strength = c.strength;
    hit.aim = p.aim;
    hit.type = shotTypeOf(c);
    hit.volley = volley;
    hit.bp = bp;
    side = ct.at.x < p.pel.pos.x ? -1 : 1;
    p.pel.swing(side, ct.at.y, fmaxf(ct.tau, 0.02f), c.strength);
    if (ct.tau > 0.02f) {
        p.pending = true;
        p.pendingT = ct.tau;
        p.pendingHit = hit;
    } else {
        doHit(i, hit);
    }
}

// Velocidad para que la pelota vaya de P hacia la mira A (en el frontis o en la pared) con velocidad horizontal vh y
// bote, de vuelta, lo más cerca posible de `depth`: prueba alturas en la pared simulando la pelota de verdad
Vec3 PalaScene::solveShot(const Vec3& P, const Vec3& A, float vh, float depth) const {
    Vec3 d(A.x - P.x, 0, A.z - P.z);
    float D = fmaxf(length(d), 0.5f);
    d = d * (1.0f / D);
    float t1 = D / vh;
    Vec3 best(d.x * vh, 5, d.z * vh);
    float bestErr = 1e9f;
    BallEvent ev[6];
    for (float ty = CHAPA_H + 0.15f; ty <= 8.0f; ty += 0.25f) {
        float vy = (ty - P.y) / t1 + 0.5f * BALL_G * t1;
        Ball b;
        b.live = true;
        b.needFrontis = true;
        b.pos = P;
        b.vel = Vec3(d.x * vh, vy, d.z * vh);
        bool valid = true, done = false;
        float land = 99;
        for (int s = 0; s < 175 && !done; s++) {
            int k = b.step(0.02f, ev, 6);
            for (int e = 0; e < k && !done; e++) {
                const BallEvent& E = ev[e];
                if (E.type == BE_FRONTIS && E.first)
                    valid &= E.at.y > CHAPA_H + 0.08f && E.at.y < TOP_LINE - 0.3f && E.at.x < XR - 0.1f;
                if (E.type == BE_FLOOR) {
                    if (E.before) valid = false;
                    else if (E.at.x > XR) valid = false;
                    land = E.at.z;
                    done = true;
                }
            }
        }
        float err = fabsf(land - depth) + (valid ? 0 : 100);
        if (err < bestErr) {
            bestErr = err;
            best = Vec3(d.x * vh, vy, d.z * vh);
        }
    }
    return best;
}

// Lanza la pelota: hacia la mira, con la velocidad de la fuerza, a la profundidad del tipo de golpe; la calidad mete
// error (de dirección, de altura) y quita velocidad
void PalaScene::doHit(int i, const HitInfo& h) {
    Player& p = pl_[i];
    bool serve = ball_.drop;
    Vec3 P = ball_.pos;
    float err = 1.0f - h.q;
    float rally = 1.0f + fminf(rallyHits_ * RALLY_SPEEDUP, RALLY_SPEEDUP_MAX);
    float vh = lerpf(SHOT_SPEED[h.type][0], SHOT_SPEED[h.type][1], h.strength) * rally *
               (h.volley || h.bp ? SPEED_VOLLEY : 1.0f) * lerpf(SHOT_MIN_SPEED_Q, 1.0f, h.q);
    float depth = serve ? SERVE_DEPTH[h.type] + (h.strength - 0.5f) * 1.5f
                        : lerpf(SHOT_DEPTH[h.type][0], SHOT_DEPTH[h.type][1], h.strength);
    // Pronto = hacia el ancho; tarde = hacia la pared (además del error al azar)
    float timingSign = h.tau > 0 ? 1.0f : -1.0f;
    float u = h.aim + frand(-1, 1) * AIM_ERR * err + timingSign * AIM_ERR_TIMING * err;
    if (serve) u = clampv(u, -1.0f, 1.0f);  // el saque, sin biparetas
    u = clampv(u, -2.0f, 1.15f);
    Vec3 A = aimPoint(u, 0);
    Vec3 v = solveShot(P, A, vh, depth);
    v.y += frand(-1, 1) * HEIGHT_ERR * err * err;
    ball_.vel = v;
    ball_.needFrontis = true;
    ball_.bounces = 0;
    ball_.drop = false;
    ball_.rolling = false;
    serving_ = serve;
    hitter_ = i;
    mustHit_ = other(i);
    if (mustHit_ != i) pl_[mustHit_].pel.splitStep();
    rallyHits_++;
    trailN_ = 0;
    for (int k = 0; k < n_; k++) pl_[k].botPlanned = false;

    p.last = h;
    p.lastShow = 1.0f;
    // Valoración y efectos
    Vec3 lp = P + Vec3(0, 0.7f, 0);
    if (h.q >= Q_GREAT) p.greats++;
    if (u < -1.05f && !serve) p.biparetas++;
    if (h.q >= Q_GREAT) addLabel(txt::GREAT, lp, UI_GOLD, 1.1f);
    else if (h.q >= Q_GOOD) addLabel(txt::GOOD, lp, UI_WHITE);
    else addLabel(txt::HIT_BAD, lp, rgba(0xC8, 0xC8, 0xD0), 0.9f);
    if (h.volley) addLabel(txt::VOLLEY, lp + Vec3(0, 0.9f, 0), UI_TEAL_GLOW, 0.8f);
    else if (h.bp) addLabel(txt::BOTE_PRONTO, lp + Vec3(0, 0.9f, 0), UI_TEAL_GLOW, 0.8f);
    if (u < -1.05f && !serve) addLabel(txt::BIPARETA, lp + Vec3(0, h.volley || h.bp ? 1.8f : 0.9f, 0), UI_GOLD, 0.8f);
    addFx(1, P, h.q >= Q_GREAT ? UI_GOLD : UI_WHITE, 0.6f + 0.8f * h.strength * h.q);
    float pan = clampv(P.x / 6.0f, -1.0f, 1.0f);
    audio::play(SFX_PALA, 0.5f + 0.5f * h.strength, frand(0.95f, 1.05f) + 0.12f * h.strength + (h.q < Q_GOOD ? -0.15f : 0), pan);
    if (!p.bot) input::rumble(padFor(p), 0.03f + 0.09f * h.strength * (0.5f + 0.5f * h.q));
    if (h.q >= Q_GREAT && rallyHits_ >= 5) fronton_.clap(0.9f);  // el público aprecia el buen peloteo
    if (h.q >= Q_GREAT && h.strength > 0.55f) {
        hitstop_ = 0.035f + 0.04f * h.strength;
        shake_ = fmaxf(shake_, 0.03f * h.strength);
    }
}

void PalaScene::checkRecords() {
    PalaRecords& r = save::data().pala;
    const Player* human = nullptr;
    for (int i = 0; i < n_ && !human; i++)
        if (!pl_[i].bot) human = &pl_[i];
    if (!human) return;
    bool better = false;
    if (cfg_.training && streak_ > r.bestStreak) {
        r.bestStreak = (u16)streak_;
        memcpy(r.streakName, human->prof->name, NAME_LEN + 1);
        better = true;
    }
    if (!cfg_.training && rallyHits_ > r.bestRally) {
        r.bestRally = (u16)rallyHits_;
        memcpy(r.rallyName, human->prof->name, NAME_LEN + 1);
        better = true;
    }
    if (better && (cfg_.training ? streak_ : rallyHits_) >= 3) {
        newRecord_ = 2.5f;
        save::write();
        audio::play(SFX_BELL, 0.5f);
    }
}

void PalaScene::endPoint(int loser, const char* why, const Vec3& at) {
    if (!rally_) return;
    rally_ = false;
    addLabel(why, at + Vec3(0, 0.6f, 0), UI_GOLD, 1.2f);
    audio::play(SFX_BAD, 0.5f);
    state_ = ST_POINT;
    stateT_ = 0;
    longestRally_ = rallyHits_ > longestRally_ ? rallyHits_ : longestRally_;
    checkRecords();
    if (cfg_.training) {
        streak_ = 0;
        pl_[0].pel.setMood(Pelotari::MOOD_SAD);
        return;
    }
    int winner = other(loser);
    pl_[winner].score++;
    lastWinner_ = winner;
    server_ = winner;  // saca el que gana el tanto
    pl_[winner].pel.setMood(Pelotari::MOOD_HAPPY);
    pl_[loser].pel.setMood(Pelotari::MOOD_SAD);
    audio::play(SFX_CLAP, 0.5f);
    if (rallyHits_ >= 6) {
        fronton_.cheer(2.0f);
        audio::play(SFX_CHEER, 0.45f);
    } else {
        fronton_.clap(1.6f);
    }
}

void PalaScene::onBallEvent(const BallEvent& e) {
    switch (e.type) {
        case BE_FRONTIS:
            audio::play(SFX_FRONTIS, 0.7f, frand(0.95f, 1.05f), clampv(e.at.x / 6.0f, -1.0f, 1.0f));
            sinceBounce_ = 0;
            if (!rally_ || !e.first) break;
            addFx(0, e.at, e.at.y < CHAPA_H ? UI_RED : UI_WHITE, 1.0f);
            if (e.at.y < CHAPA_H) {
                audio::play(SFX_BAD, 0.6f, 1.3f);
                endPoint(hitter_, txt::CHAPA, e.at);
            } else if (e.at.y > TOP_LINE) {
                endPoint(hitter_, txt::TOO_HIGH, e.at);
            } else if (e.at.x > XR) {
                endPoint(hitter_, txt::OUT, e.at);
            } else if (cfg_.training) {
                streak_++;
                best_ = streak_ > best_ ? streak_ : best_;
            }
            break;
        case BE_FLOOR:
            audio::play(SFX_BOTE, 0.35f, frand(0.95f, 1.1f), clampv(e.at.x / 6.0f, -1.0f, 1.0f));
            sinceBounce_ = 0;
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
            audio::play(SFX_FRONTIS, 0.45f, frand(1.05f, 1.15f), -1.0f);
            break;
    }
}

void PalaScene::readHuman(int i) {
    Player& p = pl_[i];
    const input::Pad& in = input::pad(padFor(p));
    Ctl& c = p.ctl;
    c.mx = in.stickX;
    c.my = in.stickY;
    c.z = (in.held & WPAD_NUNCHUK_BUTTON_Z) != 0;
    c.a = (in.held & WPAD_BUTTON_A) != 0;
    c.b = (in.held & WPAD_BUTTON_B) != 0;
    c.aDown = (in.down & WPAD_BUTTON_A) != 0;
    c.whipStart = in.whipStart;
    c.whip = in.whip.happened;
    c.strength = in.whip.strength;
}

// La CPU: se coloca a la distancia de la pala, se planta, apunta, elige el tipo de golpe y golpea con su error de nivel
void PalaScene::updateBot(int i, float dt) {
    Player& p = pl_[i];
    Pelotari& pel = p.pel;
    Ctl& c = p.ctl;
    c = Ctl();
    Vec3 goal(n_ == 2 ? 1.2f : 0.5f, 0, RECEIVE_Z - 1.0f);  // esperar en el centro, atrás

    if (state_ == ST_SERVE && server_ == i) {
        goal = pel.pos;
        p.botWait -= dt;
        if (p.botWait <= 0) c.aDown = true;
    } else if (rally_ && mustHit_ == i) {
        if (!p.botPlanned) {
            // Dónde darle: normalmente tras el bote a media altura; a veces bote pronto o volea (según el riesgo)
            int pick = -1;
            bool tryBp = !ball_.drop && frand() < level_.risk * 0.25f;
            bool tryVolley = !ball_.drop && !tryBp && frand() < level_.risk * 0.2f;
            for (int k = 0; k < path_.n && pick < 0; k++) {
                if (!path_.ok[k] || path_.p[k].z > COURT_LEN - 0.3f || path_.p[k].z < 3.0f) continue;
                float y = path_.p[k].y;
                if (tryVolley && path_.bnc[k] == 0 && y > 0.6f && y < 1.6f && path_.p[k].z > 8) pick = k;
                else if (tryBp && path_.bnc[k] == 1 && path_.sinceB[k] < 0.06f && y > 0.15f) pick = k;
            }
            for (int k = 0; k < path_.n && pick < 0; k++) {
                if (!path_.ok[k] || path_.p[k].z > COURT_LEN - 0.3f) continue;
                bool after = ball_.drop || path_.bnc[k] >= 1;
                float y = path_.p[k].y;
                if (after && y > 0.5f && y < 1.35f && (ball_.drop || path_.p[k].z > 3.0f)) pick = k;
            }
            for (int k = 0; k < path_.n && pick < 0; k++)
                if (path_.ok[k] && path_.p[k].y > 0.4f && path_.p[k].y < 1.9f) pick = k;
            // Bota muy bajo: a por lo más alto que llegue tras el bote (mejor intentarlo que dejarla)
            if (pick < 0) {
                float best = REACH_Y_MIN;
                for (int k = 0; k < path_.n; k++)
                    if (path_.ok[k] && path_.p[k].y > best && path_.p[k].z < COURT_LEN + 2) {
                        best = path_.p[k].y;
                        pick = k;
                    }
            }
            if (pick >= 0) {
                p.botPlanned = true;
                p.botStarted = false;
                p.botContact = path_.p[pick];
                p.botContactT = (pick + 1) * BallPath::DT;
                p.botNoise = frand(-1, 1) * (frand() < level_.mistake ? BOT_MISTAKE_NOISE : level_.timingNoise);
                // De derecha (la pelota a su derecha, +X) salvo pegado a la pared
                float off = SWEET_DIST + frand(-1, 1) * level_.distNoise;
                bool reves = p.botContact.x - off < XL + 0.45f;
                p.botStand = p.botContact + Vec3(reves ? off : -off, 0, 0);
                p.botStand.y = 0;
                // Tipo de golpe y mira según dónde está el rival
                const Pelotari& rival = pl_[other(i)].pel;
                float r = level_.risk;
                p.botType = SHOT_MID;
                if (n_ == 2 && rival.pos.z > 21 && frand() < 0.5f * r) p.botType = SHOT_DROP;
                else if (n_ == 2 && rival.pos.z < 16 && frand() < 0.7f) p.botType = SHOT_LONG;
                else if (frand() < 0.3f) p.botType = frand() < 0.5f ? SHOT_LONG : SHOT_DROP;
                if (ball_.drop) p.botType = frand() < 0.5f ? SHOT_MID : SHOT_LONG;
                float away = n_ == 2 ? (rival.pos.x > 0.5f ? -1.0f : 1.0f) : (frand() < 0.5f ? -1.0f : 1.0f);
                p.botAim = away < 0 ? -0.4f - 1.3f * r * frand() : 0.25f + 0.6f * r * frand();
                if (ball_.drop) p.botAim = frand(-0.5f, 0.5f);
                p.botStrength = p.botType == SHOT_DROP ? frand(0.15f, 0.45f) : frand(0.45f, 0.75f + 0.25f * r);
            }
        }
        if (p.botPlanned) {
            p.botContactT -= dt;
            goal = ball_.drop ? pel.pos : p.botStand;
            // Plantarse y apuntar en el último tramo; los botones del tipo de golpe
            if (p.botContactT < 0.45f) {
                c.z = true;
                c.mx = stickFromAim(p.botAim);
            }
            c.a = p.botType == SHOT_LONG;
            c.b = p.botType == SHOT_DROP;
            c.strength = p.botStrength;
            if (!p.botStarted && p.botContactT <= p.botNoise + 0.08f) {
                c.whipStart = true;
                p.botStarted = true;
            }
            if (p.botContactT <= p.botNoise && !p.pending) {
                c.whip = true;
                p.botPlanned = false;
                p.botContactT = 99;
            }
        }
    } else if (rally_ && n_ == 2) {
        // Dejar sitio al otro: un poco apartado de por donde va la pelota
        goal = Vec3(ball_.pos.x > 0 ? -2.0f : 2.5f, 0, RECEIVE_Z);
    }
    // El stick que llevaría al objetivo
    if (!c.z) {
        Vec3 d = goal - pel.pos;
        d.y = 0;
        float len = length(d);
        float k = len > 0.05f ? fminf(len * 1.5f, 1.0f) * level_.speed / len : 0;
        c.mx = d.x * k;
        c.my = -d.z * k;
    }
}

void PalaScene::updatePlayer(int i, float dt) {
    Player& p = pl_[i];
    Pelotari& pel = p.pel;
    bool canPlay = state_ == ST_SERVE || state_ == ST_RALLY || state_ == ST_POINT;
    if (canPlay) {
        if (p.bot) updateBot(i, dt);
        else readHuman(i);
    } else {
        p.ctl = Ctl();
    }
    const Ctl& c = p.ctl;

    // Moverse, o plantarse y apuntar con Z
    Vec3 want(c.mx * PLAYER_SPEED, 0, -c.my * PLAYER_SPEED);
    if (c.z) {
        want = want * PLANT_SPEED;
        p.aimTarget = aimFromStick(c.mx);
    }
    p.aim = approach(p.aim, p.aimTarget, AIM_RATE, dt);
    p.aimGlow = approach(p.aimGlow, c.z ? 1.0f : 0.0f, 12, dt);
    pel.setPlanted(c.z);
    if (state_ == ST_SERVE && server_ == i) want = Vec3(want.x, 0, 0);  // al sacar, solo de lado
    pel.vel = approachv(pel.vel, want, PLAYER_ACCEL, dt);
    if (canPlay) {
        // (la A con la que se salta la intro no saca: hace falta otra pulsación)
        if (state_ == ST_SERVE && server_ == i && c.aDown && stateT_ > 0.3f) startDrop(i);
        if (c.whipStart && rally_ && mustHit_ == i && ball_.live) {
            int side = ball_.pos.x < pel.pos.x ? -1 : 1;
            pel.swing(side, ball_.pos.y, 0.09f, fmaxf(c.strength, 0.5f));  // la pala ya va hacia la pelota
            audio::play(SFX_WHOOSH, 0.3f, frand(1.15f, 1.3f), clampv(pel.pos.x / 6.0f, -1.0f, 1.0f));
        }
        if (c.whip) tryHit(i);
    }

    // Golpe que llegó pronto: la pelota llega ahora a su lado
    if (p.pending) {
        p.pendingT -= dt;
        if (p.pendingT <= 0) {
            p.pending = false;
            if (rally_ && mustHit_ == i && ball_.hittable()) doHit(i, p.pendingHit);
        }
    }

    pel.pos += pel.vel * dt;
    pel.pos.x = clampv(pel.pos.x, XL + 0.35f, XR + 4.0f);
    pel.pos.z = clampv(pel.pos.z, 1.5f, COURT_LEN + 3.0f);

    // Preparar la pala cuando le toca y la pelota viene
    if (rally_ && mustHit_ == i && !ball_.needFrontis) {
        int side = ball_.pos.x < pel.pos.x ? -1 : 1;
        float near = 1.0f - smooth01((length(ball_.pos - pel.pos) - 2.5f) / 9.0f);
        pel.prepare(side, ball_.pos.y, ball_.vel.z > 0 || ball_.drop ? near : 0);
    } else {
        pel.prepare(1, 1.0f, 0);
    }
    pel.update(dt);
    p.lastShow = fmaxf(0.0f, p.lastShow - dt * 0.55f);

    // Estela de la pala mientras golpea (posiciones del frame anterior, en el mundo)
    if (pel.swinging()) {
        memmove(p.swTip + 1, p.swTip, sizeof(Vec3) * (SWOOSH_N - 1));
        memmove(p.swMid + 1, p.swMid, sizeof(Vec3) * (SWOOSH_N - 1));
        p.swTip[0] = pel.palaTip();
        p.swMid[0] = pel.palaCenter();
        p.swN = p.swN < SWOOSH_N ? p.swN + 1 : SWOOSH_N;
    } else if (p.swN > 0) {
        p.swN--;
    }
}

void PalaScene::enterResults() {
    state_ = ST_RESULTS;
    stateT_ = 0;
    resultsT_ = 0;
    resultsMenu_ = ui::ListMenu();
    resultsMenu_.count = 2;
    resultsMenu_.horizontal = true;
    audio::play(SFX_CHEER, 0.6f);
    fronton_.cheer(3.0f);
}

void PalaScene::update(float dt) {
    time_ += dt;
    u32 downAll = input::allDown();

    if (paused_) {
        int r = pauseMenu_.update(dt, downAll);
        if (r == 0 || r == -2 || (downAll & WPAD_BUTTON_PLUS)) paused_ = false;
        else if (r == 1) {
            paused_ = false;  // repasar los controles a mitad de partido
            showHowTo_ = true;
            howTo_.start();
        }
        else if (r == 2) scenes::go(makePalaScene(cfg_));
        else if (r == 3) scenes::go(makeMainMenu());
        return;
    }
    if (showHowTo_) {
        if (howTo_.update(dt, downAll)) showHowTo_ = false;
        fronton_.update(dt);
        return;
    }
    if (state_ != ST_RESULTS && (downAll & WPAD_BUTTON_PLUS)) {
        paused_ = true;
        pauseMenu_ = ui::ListMenu();
        pauseMenu_.count = 4;
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
            if (stateT_ > 2.2f) {
                bool over = false;
                for (int i = 0; i < n_; i++) over |= !cfg_.training && pl_[i].score >= target_;
                if (over) {
                    state_ = ST_FINISH;
                    stateT_ = 0;
                    audio::play(SFX_BELL, 0.7f);
                    audio::play(SFX_CHEER, 0.6f);
                    fronton_.cheer(3.0f);
                    int w = pl_[0].score >= pl_[1].score ? 0 : 1;
                    pl_[w].pel.setMood(Pelotari::MOOD_HAPPY);
                    pl_[1 - w].pel.setMood(Pelotari::MOOD_SAD);
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

    // Pelota: física, reglas, historial (para los golpes tardíos) y estela
    if (ball_.live) {
        BallEvent ev[8];
        int k = ball_.step(dt, ev, 8);
        sinceBounce_ += dt;
        for (int e = 0; e < k; e++) onBallEvent(ev[e]);
        histAcc_ += dt;
        while (histAcc_ >= HIST_DT) {
            histAcc_ -= HIST_DT;
            histHead_ = (histHead_ + 1) % HIST_N;
            hist_[histHead_] = ball_.pos;
            histOk_[histHead_] = rally_ && ball_.hittable();
            histBnc_[histHead_] = (unsigned char)clampv(ball_.bounces, 0, 9);
            histSince_[histHead_] = sinceBounce_;
        }
        path_.predict(ball_, 2.5f, sinceBounce_);
        memmove(trail_ + 1, trail_, sizeof(Vec3) * (TRAIL_N - 1));
        trail_[0] = ball_.pos;
        trailN_ = trailN_ < TRAIL_N ? trailN_ + 1 : TRAIL_N;
        if (!rally_ && state_ == ST_POINT && stateT_ > 1.6f) ball_.live = false;
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
    fronton_.update(dt);
    for (int i = 0; i < numLabels_; i++) labels_[i].t += dt;
    while (numLabels_ > 0 && labels_[0].t > 1.3f) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * (numLabels_ - 1));
        numLabels_--;
    }
    for (Fx& f : fx_) f.t += dt;
    newRecord_ = fmaxf(0.0f, newRecord_ - dt);
    shake_ = approach(shake_, 0, 10, dt);
    updateCamera(dt);
}

// ¿Cabe el punto q en la pantalla con la cámara en `eye` mirando a lo largo de -dir?
bool PalaScene::inView(const Vec3& eye, const Vec3& q) const {
    // La cámara mira a target + CAM_LOOK_UP: su dirección real depende de la distancia (aproximada con la del objetivo)
    Vec3 dir = normalize(Vec3(CAM_DIR_X, CAM_DIR_Y, CAM_DIR_Z));
    Vec3 f = normalize(-dir * camDistTry_ + Vec3(0, CAM_LOOK_UP, 0));
    Vec3 v = q - eye;
    float z = dot(v, f);
    if (z < 1.0f) return false;
    Vec3 right = normalize(cross(f, Vec3(0, 1, 0)));
    Vec3 up = cross(right, f);
    float t = tanf(camFovTry_ * 0.5f * DEG) * CAM_MARGIN;
    float aspect = gfx::widescreen() ? 16.0f / 9.0f : 4.0f / 3.0f;
    return fabsf(dot(v, up) / z) < t && fabsf(dot(v, right) / z) < t * aspect;
}

void PalaScene::updateCamera(float dt) {
    // Isométrica fija desde atrás a la derecha. Mira a la pelota y al pelotari que tiene que golpear; se acerca cuando
    // la pelota está junto al frontis y se aleja con ella, pero sin dejar nunca fuera a ese pelotari
    const Vec3 dir = normalize(Vec3(CAM_DIR_X, CAM_DIR_Y, CAM_DIR_Z));
    int focus = rally_ ? mustHit_ : state_ == ST_POINT && lastWinner_ >= 0 ? lastWinner_ : server_;
    const Vec3 me = pl_[focus].pel.pos + Vec3(0, 0.9f, 0);
    Vec3 ball = ball_.live ? ball_.pos : me + Vec3(0.4f, 0.2f, -1.5f);
    ball.y = clampv(ball.y, 0.0f, 4.0f);
    // Tras el tanto la pelota está muerta: la cámara se queda con el pelotari
    bool deadBall = state_ == ST_POINT || !ball_.live;
    Vec3 target = lerpv(me, ball, deadBall ? 0.15f : CAM_BALL_WEIGHT);
    float zk = clampv(ball.z / COURT_LEN, 0.0f, 1.0f);
    float dist = lerpf(CAM_DIST_NEAR, CAM_DIST_FAR, smooth01(zk));
    if (state_ == ST_SERVE || (state_ == ST_RALLY && ball_.drop)) dist = CAM_DIST_NEAR + 1.0f;
    // Que quepan el que golpea (pies y cabeza), la pelota y los humanos: primero se abre el ángulo, luego se aleja
    float fov = CAM_FOV;
    for (int k = 0; k < 60 && dist < CAM_DIST_MAX; k++) {
        Vec3 eye = target + dir * dist;
        camDistTry_ = dist;
        camFovTry_ = fov;
        bool ok = inView(eye, pl_[focus].pel.pos) && inView(eye, pl_[focus].pel.pos + Vec3(0, 1.7f, 0)) &&
                  (deadBall || inView(eye, ball));
        // Los humanos siempre; al sacar, los dos (que se vea al que resta)
        for (int i = 0; i < n_; i++)
            if (!pl_[i].bot || state_ == ST_SERVE) ok = ok && inView(eye, pl_[i].pel.pos) && inView(eye, pl_[i].pel.pos + Vec3(0, 1.7f, 0));
        if (ok) break;
        if (fov < CAM_FOV_MAX) fov += 2.0f;
        else dist += 0.5f;
    }
    if (state_ == ST_INTRO) {
        // la presentación la hace draw(); aquí la cámara ya espera en su sitio de juego
        camTarget_ = target;
        camDist_ = dist;
        camFov_ = fov;
        return;
    } else if (state_ == ST_FINISH || state_ == ST_RESULTS) {
        dist = CAM_DIST_NEAR + 2.0f;
        fov = CAM_FOV;
        target = pl_[pl_[0].score >= pl_[n_ - 1].score ? 0 : n_ - 1].pel.pos + Vec3(0, 0.9f, 0);
    }
    camTarget_ = approachv(camTarget_, target, CAM_RATE, dt);
    camDist_ = approach(camDist_, dist, CAM_RATE * 0.8f, dt);
    camFov_ = approach(camFov_, state_ == ST_INTRO || state_ == ST_FINISH || state_ == ST_RESULTS ? CAM_FOV : fov, CAM_RATE, dt);
}

void PalaScene::draw() {
    World& world = sharedWorld();
    gfx::begin2D();
    world.drawSky();
    Camera cam;
    Vec3 jitter(frand(-1, 1) * shake_, frand(-1, 1) * shake_, frand(-1, 1) * shake_);
    Vec3 dir = normalize(Vec3(CAM_DIR_X, CAM_DIR_Y, CAM_DIR_Z));
    cam.eye = camTarget_ + dir * camDist_ + jitter;
    cam.target = camTarget_ + Vec3(0, CAM_LOOK_UP, 0) + jitter * 0.5f;
    cam.fov = camFov_;
    if (state_ == ST_INTRO) {
        // Presentación (estilo Resort): desde lo alto de la esquina del frontis, con la ikurriña, hasta el juego
        float k = smooth01(stateT_ / 2.3f);
        Vec3 e0(XR + 7.0f, 13.0f, -3.0f), t0(-1.0f, 1.0f, 17.0f);
        cam.eye = lerpv(e0, cam.eye, k);
        cam.target = lerpv(t0, cam.target, k);
        cam.fov = lerpf(46.0f, camFov_, k);
    }
    cam.zfar = 400;
    gfx::begin3D(cam);
    camEyeDraw_ = cam.eye;
    camTargetDraw_ = cam.target;
    gfx::setFog(true, 70.0f, 260.0f, World::HORIZON);
    fronton_.draw();
    gfx::setFog(false);

    for (int i = 0; i < n_; i++) world.drawShadow(pl_[i].pel.pos, 0.36f, 0.36f);
    if (ball_.live) {
        world.drawShadow(Vec3(ball_.pos.x, 0, ball_.pos.z), 0.1f, 0.1f);
        // Ayuda: dónde va a botar
        if (SHOW_LANDING && rally_ && path_.landing >= 0) {
            static Mesh ring;
            if (!ring.valid()) {
                MeshBuilder b;
                const int N = 28;
                const Color c = rgba(0xFF, 0xFF, 0xFF, 170);
                for (int i = 0; i < N; i++) {
                    float a0 = i * 2 * PI / N, a1 = (i + 1) * 2 * PI / N;
                    Vec3 i0(cosf(a0) * 0.75f, 0, sinf(a0) * 0.75f), o0(cosf(a0), 0, sinf(a0));
                    Vec3 i1(cosf(a1) * 0.75f, 0, sinf(a1) * 0.75f), o1(cosf(a1), 0, sinf(a1));
                    b.tri(i0, o0, o1, c, Vec3(0, 1, 0));
                    b.tri(i0, o1, i1, c, Vec3(0, 1, 0));
                }
                b.build(ring);
            }
            const Vec3& l = path_.p[path_.landing];
            float r = 0.32f * (1.0f + 0.12f * sinf(time_ * 9));
            gfx::setLighting(false);
            gfx::setBlend(true);
            gfx::setDepth(true, false);
            gfx::drawMesh(ring, Xf().translate(Vec3(l.x, 0.02f, l.z)).scale(r));
            gfx::setDepth(true, true);
            gfx::setBlend(false);
            gfx::setLighting(true);
        }
    }
    for (int i = 0; i < n_; i++) pl_[i].pel.draw();
    static Mesh ballMesh, ghostMesh;
    if (!ballMesh.valid()) {
        MeshBuilder b;
        b.sphere(Vec3(), Vec3(1, 1, 1), rgba(0xF6, 0xF4, 0xEC), 14, 10);
        b.build(ballMesh);
        b.clear();
        b.sphere(Vec3(), Vec3(1, 1, 1), rgba(0xFF, 0xFF, 0xFF, 70), 10, 6);
        b.build(ghostMesh);
    }
    if (ball_.live) {
        // Estela: bolas cada vez más pequeñas y transparentes
        gfx::setBlend(true);
        gfx::setDepth(true, false);
        gfx::setLighting(false);
        for (int k = 2; k < trailN_; k += 1)
            gfx::drawMesh(ghostMesh, Xf().translate(trail_[k]).scale(BALL_DRAW_R * (1.0f - k / (float)TRAIL_N * 0.7f)));
        gfx::setLighting(true);
        gfx::setDepth(true, true);
        gfx::setBlend(false);
        gfx::drawMesh(ballMesh, Xf().translate(ball_.pos).scale(BALL_DRAW_R));
    }

    gfx::begin2D();
    if (showHowTo_) {
        howTo_.draw();
        return;
    }
    fronton_.drawLabels();
    drawEffects2D();
    drawHud();
    if (state_ == ST_RESULTS) drawResults();
    if (paused_) drawPause();
}

// Efectos en 2D sobre la escena: estela de la pala, anillos en el frontis, chispas del golpe y la mira
void PalaScene::drawEffects2D() {
    // Contorno de la pelota para que se lea siempre sobre el suelo claro y el frontis
    if (ball_.live) {
        Vec3 f = normalize(camTargetDraw_ - camEyeDraw_);
        Vec3 up = normalize(cross(normalize(cross(f, Vec3(0, 1, 0))), f));
        float sx, sy, ux, uy;
        if (gfx::project(ball_.pos, sx, sy) && gfx::project(ball_.pos + up * BALL_DRAW_R, ux, uy)) {
            float r = fmaxf(2.5f, sqrtf((ux - sx) * (ux - sx) + (uy - sy) * (uy - sy)));
            gfx::ring(sx, sy, r, r + 1.6f, rgba(0x20, 0x30, 0x40, 150), 16);
        }
    }
    for (int i = 0; i < n_; i++) {
        const Player& p = pl_[i];
        for (int k = 0; k + 1 < p.swN; k++) {
            float ax, ay, bx, by, cx, cy, dx, dy;
            if (!gfx::project(p.swTip[k], ax, ay) || !gfx::project(p.swMid[k], bx, by) ||
                !gfx::project(p.swTip[k + 1], cx, cy) || !gfx::project(p.swMid[k + 1], dx, dy))
                continue;
            float a = (1.0f - (float)k / SWOOSH_N) * 0.55f;
            Color c = withAlpha(UI_WHITE, a);
            gfx::triangle(ax, ay, bx, by, cx, cy, c);
            gfx::triangle(bx, by, dx, dy, cx, cy, c);
        }
    }
    for (const Fx& f : fx_) {
        if (f.t > 0.5f) continue;
        float sx, sy;
        if (!gfx::project(f.pos, sx, sy)) continue;
        float k = f.t / 0.5f;
        if (f.kind == 0) {
            float r = (14 + 46 * easeOutCubic(k)) * f.size;
            gfx::ring(sx, sy, r, r + 5, withAlpha(f.color, 1 - k), 28);
        } else {
            float r = (10 + 40 * easeOutCubic(k)) * f.size;
            for (int s = 0; s < 8; s++) {
                float an = s * PI / 4 + 0.3f;
                float x0 = sx + cosf(an) * r * 0.45f, y0 = sy + sinf(an) * r * 0.45f;
                float x1 = sx + cosf(an) * r, y1 = sy + sinf(an) * r;
                float px = -sinf(an) * 3, py = cosf(an) * 3;
                gfx::triangle(x0 + px, y0 + py, x0 - px, y0 - py, x1, y1, withAlpha(f.color, 1 - k));
            }
            gfx::circle(sx, sy, 10 * f.size * (1 - k), withAlpha(UI_WHITE, 1 - k), 16);
        }
    }
    // Mira de cada humano en la pared (más viva mientras se mantiene Z)
    for (int i = 0; i < n_; i++) {
        const Player& p = pl_[i];
        if (p.bot || !(state_ == ST_RALLY || state_ == ST_SERVE)) continue;
        float sx, sy;
        int type = shotTypeOf(p.ctl);
        if (!gfx::project(aimPoint(p.aim, aimHeight(type)), sx, sy)) continue;
        Color col = FAV_COLORS[p.prof->color % NUM_COLORS];
        float g = p.aimGlow, pulse = 1.0f + 0.12f * sinf(time_ * 10) * g;
        float r = (16 + 8 * g) * pulse;
        gfx::ring(sx, sy, r, r + 4 + 2 * g, withAlpha(rgba(0x20, 0x30, 0x40), 0.35f + 0.4f * g), 24);
        gfx::ring(sx, sy, r - 3, r + 2 + 2 * g, withAlpha(col, 0.45f + 0.55f * g), 24);
        gfx::rect(sx - 1, sy - r - 6, 2, 10, withAlpha(col, 0.5f + 0.5f * g));
        gfx::rect(sx - 1, sy + r - 4, 2, 10, withAlpha(col, 0.5f + 0.5f * g));
        gfx::rect(sx - r - 6, sy - 1, 10, 2, withAlpha(col, 0.5f + 0.5f * g));
        gfx::rect(sx + r - 4, sy - 1, 10, 2, withAlpha(col, 0.5f + 0.5f * g));
        if (g > 0.5f) font::draw(txt::SHOT_NAMES[type], sx, sy + r + 6, 20, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER, g);
    }
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
        float a = 1.0f - smooth01((l.t - 0.9f) / 0.4f);
        font::draw(l.text, sx, sy - 30 - l.t * 26, 38 * l.size * pop + 2, l.color, rgba(0x30, 0x30, 0x38), ALIGN_CENTER, a);
    }
    if (state_ == ST_RESULTS) return;

    // Marcador (estilo Resort: placa de cristal con los dos nombres y los tantos grandes)
    if (cfg_.training) {
        gfx::roundRectGrad(30, 22, 250, 78, 22, UI_GLASS_TOP, UI_GLASS_BOTTOM);
        font::plain(txt::STREAK, 52, 30, 22, UI_TEAL);
        snprintf(buf, sizeof(buf), "%d", streak_);
        font::draw(buf, 52, 50, 44, UI_WHITE, UI_TEAL);
        font::plain(txt::BEST, 258, 30, 22, UI_TEAL, ALIGN_RIGHT);
        snprintf(buf, sizeof(buf), "%d", best_);
        font::draw(buf, 258, 50, 44, UI_WHITE, UI_TEAL, ALIGN_RIGHT);
    } else {
        float pw = 470, px = W / 2 - pw / 2, py = 18;
        gfx::roundRect(px - 3, py - 3, pw + 6, 82, 26, rgba(0x10, 0x60, 0x90, 120));
        gfx::roundRectGrad(px, py, pw, 76, 24, UI_GLASS_TOP, UI_GLASS_BOTTOM);
        for (int i = 0; i < 2; i++) {
            const Player& p = pl_[i];
            bool left = i == 0;
            Color col = FAV_COLORS[p.prof->color % NUM_COLORS];
            float nx = left ? px + 46 : px + pw - 46;
            gfx::roundRect(left ? px + 14 : px + pw - 30, py + 16, 16, 44, 8, col);
            font::plain(p.prof->name, nx, py + 12, 26, UI_TEXT, left ? ALIGN_LEFT : ALIGN_RIGHT);
            if (p.bot) {
                snprintf(buf, sizeof(buf), "%s · %s", txt::CPU, txt::LEVELS[clampv(cfg_.level, 0, 2)]);
                font::plain(buf, nx, py + 44, 16, UI_TEXT_LIGHT, left ? ALIGN_LEFT : ALIGN_RIGHT);
            }
            if (server_ == i) gfx::circle(left ? px + 22 : px + pw - 22, py + 68, 6, rgba(0xF6, 0xF4, 0xEC), 12);
            snprintf(buf, sizeof(buf), "%d", p.score);
            float pop = lastWinner_ == i && state_ == ST_POINT ? 1.0f + 0.25f * (1 - smooth01(stateT_ / 0.5f)) : 1.0f;
            font::draw(buf, W / 2 + (left ? -40 : 40), py + 12 - 6 * (pop - 1), 50 * pop, UI_WHITE, UI_TEAL, ALIGN_CENTER);
        }
        font::plain("-", W / 2, py + 16, 40, UI_TEAL, ALIGN_CENTER);
        snprintf(buf, sizeof(buf), txt::POINTS_N, target_);
        font::draw(buf, W / 2, py + 80, 18, UI_WHITE, UI_TEAL, ALIGN_CENTER);
    }

    // Flecha sobre quien tiene que dar
    if (rally_ && n_ > 1) {
        const Player& p = pl_[mustHit_];
        float sx, sy;
        if (gfx::project(p.pel.pos + Vec3(0, 2.0f, 0), sx, sy)) {
            float b = 4 * sinf(time_ * 8);
            Color c = FAV_COLORS[p.prof->color % NUM_COLORS];
            gfx::triangle(sx - 13, sy - 20 + b, sx + 13, sy - 20 + b, sx, sy + b, UI_WHITE);
            gfx::triangle(sx - 9, sy - 17 + b, sx + 9, sy - 17 + b, sx, sy - 4 + b, c);
        }
    }

    // Último golpe de cada humano: momento (goiz / berandu, con la zona buena) y sitio (cerca / lejos)
    for (int i = 0; i < n_; i++) {
        const Player& p = pl_[i];
        if (p.bot || p.lastShow <= 0) continue;
        float a = smooth01(p.lastShow * 3);
        float cx = n_ == 1 ? W / 2 : (i == 0 ? W * 0.27f : W * 0.73f), cy = 432, w = 200;
        gfx::roundRect(cx - w / 2 - 70, cy - 30, w + 140, 60, 18, withAlpha(rgba(0x10, 0x30, 0x48, 150), a));
        auto bar = [&](float y, float v, float good, const char* lo, const char* hi, const char* name) {
            gfx::roundRect(cx - w / 2, y - 4, w, 8, 4, withAlpha(rgba(255, 255, 255, 120), a));
            gfx::roundRect(cx - good * w / 2, y - 4, good * w, 8, 4, withAlpha(UI_GREEN, a));
            float mx = cx + clampv(v, -1.0f, 1.0f) * w / 2;
            gfx::circle(mx, y, 7, withAlpha(UI_GOLD, a), 14);
            font::plain(lo, cx - w / 2 - 8, y - 9, 15, withAlpha(UI_WHITE, a), ALIGN_RIGHT);
            font::plain(hi, cx + w / 2 + 8, y - 9, 15, withAlpha(UI_WHITE, a), ALIGN_LEFT);
            (void)name;
        };
        float hw = HIT_WINDOW * lerpf(WINDOW_SOFT, WINDOW_STRONG, p.last.strength);
        bar(cy - 12, p.last.tau / hw, SWEET_TIME / HIT_WINDOW, txt::LATE, txt::EARLY, txt::TIME);
        bar(cy + 12, (p.last.dist - SWEET_DIST) / (REACH - SWEET_DIST), SWEET_DIST_TOL / (REACH - SWEET_DIST), "<", ">", txt::PLACE);
    }

    // Tipo de golpe que se va a hacer (según los botones) para cada humano en juego
    if (state_ == ST_RALLY || state_ == ST_SERVE) {
        for (int i = 0; i < n_; i++) {
            const Player& p = pl_[i];
            if (p.bot || p.lastShow > 0.3f) continue;
            int type = shotTypeOf(p.ctl);
            float cx = n_ == 1 ? W / 2 : (i == 0 ? W * 0.27f : W * 0.73f), cy = 440;
            const char* keys[3] = {"B", "", "A"};
            for (int t = 0; t < 3; t++) {
                float x = cx + (t - 1) * 104;
                bool on = t == type;
                gfx::roundRect(x - 48, cy - 16, 96, 32, 16, on ? UI_TEAL : rgba(0x10, 0x30, 0x48, 110));
                if (keys[t][0]) ui::buttonIcon(x - 33, cy, keys[t], 11, on ? UI_TEAL_GLOW : rgba(0x80, 0x88, 0x94));
                font::plain(txt::SHOT_NAMES[t], x + (keys[t][0] ? 10 : 0), cy - 11, 19, on ? UI_WHITE : rgba(0xD0, 0xE0, 0xE8), ALIGN_CENTER);
            }
        }
    }

    // Saque (en el entrenamiento, a la izquierda de la tarjeta "Nola jo")
    // También en el primer saque de la lehiaketa si juega algún humano
    bool anyHuman = false;
    for (int i = 0; i < n_; i++) anyHuman |= !pl_[i].bot;
    bool firstPoint = !cfg_.training && pl_[0].score + pl_[n_ - 1].score == 0;
    bool howTo = (cfg_.training || firstPoint) && (state_ == ST_SERVE || state_ == ST_INTRO) && anyHuman;
    if (state_ == ST_SERVE && !pl_[server_].bot) {
        // Abajo a la derecha, sobre la contracancha: arriba están el frontis y la mira
        float cx = W * 0.78f, cy = 286;
        float s = 1.0f + 0.04f * sinf(time_ * 6);
        font::draw(txt::SERVE, cx, cy, 46 * s, UI_GOLD, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
        float tw = font::width(txt::SERVE_HOW, 24);
        font::draw(txt::SERVE_HOW, cx + 13, cy + 52, 24, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER);
        ui::buttonIcon(cx - tw / 2 - 6, cy + 66, "A", 13);
    }
    if (howTo) drawHowTo();
    if (state_ == ST_POINT && !cfg_.training && lastWinner_ >= 0 && stateT_ > 0.5f) {
        snprintf(buf, sizeof(buf), txt::POINT_FOR, pl_[lastWinner_].prof->name);
        font::draw(buf, W / 2, 140, 44, UI_WHITE, UI_TEAL, ALIGN_CENTER, smooth01((stateT_ - 0.5f) / 0.3f));
    }
    if (state_ == ST_INTRO) {
        float a = smooth01(stateT_ / 0.4f) * (1 - smooth01((stateT_ - 2.0f) / 0.4f));
        font::draw(cfg_.training ? txt::TRAINING : txt::COMPETITION, W / 2, 130, 56, UI_WHITE, UI_TEAL, ALIGN_CENTER, a);
        if (cfg_.training) snprintf(buf, sizeof(buf), "%s", txt::PALA_TRAINING_SUB);
        else snprintf(buf, sizeof(buf), txt::POINTS_N, target_);
        font::draw(buf, W / 2, 196, 30, UI_WHITE, UI_TEAL, ALIGN_CENTER, a);
    } else if (state_ == ST_FINISH) {
        float s = easeOutBack(clampv(stateT_ / 0.3f, 0.0f, 1.0f));
        int w = pl_[0].score >= pl_[1].score ? 0 : 1;
        snprintf(buf, sizeof(buf), "%s: %s!", txt::WINNER, pl_[w].prof->name);
        font::draw(buf, W / 2, 130, 52 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER);
    }

    if (newRecord_ > 0) {
        float s = 1 + 0.05f * sinf(time_ * 8);
        font::draw(txt::NEW_RECORD, W / 2, 200, 40 * s, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER, smooth01(newRecord_ * 2));
    }
    if (needsNunchuk()) {
        ui::fade(0.45f);
        ui::panel(W / 2 - 260, 170, 520, 130);
        font::plain(txt::CONNECT_NUNCHUK, W / 2, 190, 36, UI_TEXT, ALIGN_CENTER);
        font::plain(txt::NUNCHUK_NEEDED, W / 2, 246, 22, UI_TEXT_LIGHT, ALIGN_CENTER);
    }
}

// Tarjeta "Nola jo" del entrenamiento: los controles de la pala
void PalaScene::drawHowTo() {
    float pw = HOWTO_W, ph = 36 + 6 * 32, px = 20, py = 112;
    gfx::roundRectGrad(px, py, pw, ph, 20, UI_GLASS_TOP, UI_GLASS_BOTTOM);
    font::plain(txt::HOW_TO, px + pw / 2, py + 6, 26, UI_TEAL, ALIGN_CENTER);
    for (int i = 0; i < 6; i++) {
        float y = py + 40 + i * 32;
        font::plain(txt::PALA_HOW[i][0], px + 14, y, 19, UI_TEXT);
        font::plain(txt::PALA_HOW[i][1], px + 128, y + 2, 16, UI_TEXT_LIGHT);
    }
}

void PalaScene::drawResults() {
    const float W = gfx::screenW();
    char buf[64];
    float a = smooth01(resultsT_ / 0.4f);
    ui::fade(0.35f * a);
    float pw = 560, ph = 70 + 2 * 66 + 84;
    float px = W / 2 - pw / 2, py = 60 + (1 - easeOutCubic(resultsT_ / 0.5f)) * 60;
    ui::panel(px, py, pw, ph);
    font::plain(txt::RESULTS, W / 2, py + 14, 36, UI_TEXT, ALIGN_CENTER);
    int first = n_ == 2 && pl_[1].score > pl_[0].score ? 1 : 0;
    for (int r = 0; r < n_; r++) {
        const Player& p = pl_[r == 0 ? first : 1 - first];
        float y = py + 66 + r * 66;
        if (r == 0) gfx::roundRect(px + 16, y - 4, pw - 32, 62, 16, rgba(0xFF, 0xF0, 0xB8));
        snprintf(buf, sizeof(buf), "%d.", r + 1);
        font::plain(buf, px + 34, y + 4, 30, r == 0 ? rgba(0xD8, 0x98, 0x10) : UI_TEXT_LIGHT);
        ui::colorDot(px + 92, y + 21, 10, FAV_COLORS[p.prof->color % NUM_COLORS]);
        font::plain(p.prof->name, px + 114, y + 4, 30, UI_TEXT);
        snprintf(buf, sizeof(buf), "%d", p.score);
        font::plain(buf, px + pw - 36, y + 4, 30, UI_TEXT, ALIGN_RIGHT);
        snprintf(buf, sizeof(buf), txt::STATS_LINE, p.greats, p.biparetas);
        font::plain(buf, px + 114, y + 36, 17, UI_TEXT_LIGHT);
    }
    snprintf(buf, sizeof(buf), txt::LONGEST_RALLY, longestRally_);
    font::plain(buf, W / 2, py + ph - 80, 20, UI_TEXT_LIGHT, ALIGN_CENTER);
    snprintf(buf, sizeof(buf), "%s: %s!", txt::WINNER, pl_[first].prof->name);
    font::draw(buf, W / 2, py + ph - 50, 34, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER, a);
    float by = fminf(430.0f, py + ph + 44);
    ui::button(W / 2 - 150, by, 260, 60, txt::PLAY_AGAIN, resultsMenu_.anim[0]);
    ui::button(W / 2 + 150, by, 260, 60, txt::TO_MENU, resultsMenu_.anim[1]);
}

void PalaScene::drawPause() {
    const float W = gfx::screenW();
    ui::fade(0.5f);
    float pw = 420, ph = 370;
    ui::panel(W / 2 - pw / 2, 240 - ph / 2, pw, ph);
    font::plain(txt::PAUSE, W / 2, 240 - ph / 2 + 16, 36, UI_TEXT, ALIGN_CENTER);
    const char* items[4] = {txt::CONTINUE, txt::HOWTO, txt::RESTART, txt::TO_MENU};
    for (int i = 0; i < 4; i++) ui::button(W / 2, 148 + i * 68, 320, 56, items[i], pauseMenu_.anim[i]);
}

}  // namespace

Scene* makePalaScene(const MatchConfig& cfg) { return new PalaScene(cfg); }
