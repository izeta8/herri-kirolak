// Aizkolaritza: la partida.
//  - Entrenamendua: un jugador, cámara cercana que gira con él, consejos, indicador grande.
//  - Lehiaketa: 1-4 jugadores A LA VEZ, cada uno con su Wii Remote, su aizkolari y su tronco en fila
//    en la plaza (como en las competiciones de verdad). Gana quien corte antes sus troncos.
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
#include "games/aizkolaritza/aizkolari.h"
#include "games/aizkolaritza/log.h"
#include "scenes/scenes.h"

using namespace tuning;

namespace {

enum State { ST_INTRO, ST_COUNTDOWN, ST_PLAY, ST_FINISH, ST_RESULTS };
enum PState { P_PLAY, P_SPLIT, P_NEXTLOG, P_DONE };
enum SideHit { HIT_LEFT, HIT_RIGHT, HIT_STRAIGHT, HIT_NONE };

const int MAXP = 4;
const float SPACING = 2.1f;  // distancia entre troncos en la competición

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
    Log log;
    Chips chips;
    Aizkolari aizk;
    PState st = P_PLAY;
    float stT = 0;
    int logIndex = 0;
    float timer = 0;
    bool running = false;
    unsigned timeMs = 0;
    int rank = 0;
    bool record = false;
    int lastSide[2] = {HIT_NONE, HIT_NONE};
    float botT = 1.0f, botRaise = 0;
    float needle = 0, needleTarget = 0;
    float sideFlash = 0;
    int sideFlashHit = HIT_NONE;
    float idleT = 0;
    float fatigue = 0;
    float hitstop = 0;      // pausa breve tras un buen golpe (sensación de impacto)
    float strength = 0;     // fuerza del último golpe (0..1) para el medidor
    float strengthShow = 0; // se va apagando
};

class AizkoScene : public Scene {
public:
    explicit AizkoScene(const AizkoConfig& cfg) : cfg_(cfg) {
        n_ = cfg_.training ? 1 : clampv(cfg_.numPlayers, 1, MAXP);
        for (int i = 0; i < n_; i++) {
            Player& p = pl_[i];
            p.pad = i;
            p.bot = cfg_.bot[i];
            p.prof = &save::data().profiles[cfg_.profile[i] % MAX_PROFILES];
            p.chips.init();
            float x = (i - (n_ - 1) * 0.5f) * SPACING;
            p.log.pos = Vec3(x, 0, 0);
            p.aizk.setup(Appearance::from(*p.prof), &p.log);
            resetLog(p);
        }
        showHowTo_ = cfg_.training && !cfg_.bot[0];
        camEye_ = Vec3(6, 3.5f, 9);
        camTarget_ = Vec3(0, 0.9f, 0);
    }

    void update(float dt) override;
    void draw() override;

private:
    bool multi() const { return n_ > 1; }
    int padFor(const Player& p) const { return input::pad(p.pad).connected ? p.pad : 0; }
    void resetLog(Player& p);
    void updatePlayer(Player& p, float dt, u32 downAll);
    void handleImpact(Player& p);
    void updateBot(Player& p, float dt);
    void addLabel(const char* t, const Vec3& pos, Color c);
    void enterResults();
    void updateCamera(float dt);
    void drawTrainingHud();
    void drawMultiHud();
    void drawCommon();
    void drawResults();
    void drawPause();
    void drawHowTo();

    AizkoConfig cfg_;
    int n_ = 1;
    Player pl_[MAXP];
    State state_ = ST_INTRO;
    float stateT_ = 0;
    int finished_ = 0;
    FloatLabel labels_[12];
    int numLabels_ = 0;
    float shake_ = 0;
    Vec3 camEye_, camTarget_;
    float time_ = 0;
    bool paused_ = false;
    ui::ListMenu pauseMenu_, resultsMenu_;
    int order_[MAXP] = {0, 1, 2, 3};
    float resultsT_ = 0;
    bool showHowTo_ = false;
    float howToT_ = 0;
};

void AizkoScene::resetLog(Player& p) {
    p.log.reset();
    p.chips.clear();
    p.aizk.reset();
    p.lastSide[0] = p.lastSide[1] = HIT_NONE;
}

void AizkoScene::addLabel(const char* t, const Vec3& pos, Color c) {
    if (numLabels_ == 12) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * 11);
        numLabels_--;
    }
    FloatLabel& l = labels_[numLabels_++];
    strncpy(l.text, t, sizeof(l.text) - 1);
    l.text[sizeof(l.text) - 1] = 0;
    l.pos = pos;
    l.t = 0;
    l.color = c;
}

void AizkoScene::handleImpact(Player& p) {
    const float R = LOG_RADIUS;
    Aizkolari& a = p.aizk;
    Log& log = p.log;
    float strength = a.impactStrength();
    float tilt = a.impactTilt();
    int hit = tilt < -TILT_STRAIGHT ? HIT_LEFT : tilt > TILT_STRAIGHT ? HIT_RIGHT : HIT_STRAIGHT;
    int notch = a.side();
    Vec3 at = log.impactPoint(notch, tilt);
    Vec3 outward(0, 0, notch == 0 ? 1.0f : -1.0f);
    int pad = padFor(p);

    // La fuerza cuenta de verdad: un golpe flojo corta mucho menos que uno fuerte
    float power = 0.3f + 0.7f * strength * strength;
    float factor = power;
    const char* label = nullptr;
    Color lc = UI_WHITE;
    bool full = notch == 0 && log.depth[0] >= SIDE_MAX * R - 0.001f;
    if (full) {
        factor = 0;
        label = txt::TURN_AROUND;
        lc = UI_GOLD;
    } else {
        if (hit == HIT_STRAIGHT) {
            factor *= STRAIGHT_FACTOR;
            label = txt::TOO_STRAIGHT;
            lc = rgba(0xFF, 0xB0, 0x60);
        } else if (hit == p.lastSide[notch]) {
            factor *= REPEAT_SIDE_FACTOR;
            label = txt::SAME_SIDE;
            lc = rgba(0xFF, 0x90, 0x60);
        }
        if (FATIGUE_ON && p.fatigue > FATIGUE_HEAVY) {
            factor *= 0.6f;
            if (!label) { label = txt::TIRED; lc = rgba(0xC0, 0xC8, 0xFF); }
        }
        if (!label) {
            if (factor >= 0.75f) { label = txt::GREAT; lc = UI_GOLD; }
            else if (factor >= 0.45f) { label = txt::GOOD; lc = UI_WHITE; }
            else { label = txt::WEAK; lc = rgba(0xD0, 0xD0, 0xD0); }
        }
        float gain = CUT_PER_CHOP * R * factor;
        if (notch == 0) gain = fminf(gain, SIDE_MAX * R - log.depth[0]);
        else gain = fminf(gain, 2 * R - log.total());
        log.addDepth(notch, fmaxf(0.0f, gain));
        // Astillas: más y más grandes cuanto mejor el golpe
        p.chips.spawn(at, outward, 2 + (int)(factor * 12), 0.45f + factor * 0.95f);
    }
    if (FATIGUE_ON) p.fatigue = fminf(1.0f, p.fatigue + FATIGUE_PER_CHOP);
    if (hit != HIT_STRAIGHT) p.lastSide[notch] = hit;
    p.sideFlash = 1.0f;
    p.sideFlashHit = hit;
    p.strength = strength;
    p.strengthShow = 1.0f;

    float pan = clampv(at.x * 0.25f, -1.0f, 1.0f);
    float vol = multi() ? 0.6f : 1.0f;
    if (factor >= 0.45f) audio::play(SFX_CHOP, vol * (0.6f + 0.4f * strength), frand(0.92f, 1.08f) - 0.1f * factor, pan);
    else audio::play(SFX_CHOP_WEAK, vol * 0.8f, frand(0.9f, 1.1f), pan);
    if (!p.bot) input::rumble(pad, 0.04f + 0.12f * factor);
    // Golpe contundente: pausa corta, temblor de cámara según la fuerza
    if (factor >= 0.6f) p.hitstop = 0.03f + 0.05f * factor;
    shake_ = fmaxf(shake_, (multi() ? 0.01f : 0.015f) + 0.05f * factor * strength);
    addLabel(label, at + Vec3(0, 0.35f, 0), lc);
    if (factor >= 0.75f && frand() < 0.3f) sharedWorld().clapLight(1.0f);

    if (log.cut()) {
        p.st = P_SPLIT;
        p.stT = 0;
        p.running = false;
        log.startSplit();
        a.celebrate();
        audio::play(SFX_SPLIT, vol);
        audio::play(SFX_CHEER, 0.6f);
        sharedWorld().cheer(2.0f);
        p.chips.spawn(log.center() + Vec3(0, 0.1f, 0), Vec3(0, 1, 0), 18, 1.2f);
        if (!p.bot) input::rumble(pad, 0.35f);
    }
}

void AizkoScene::updateBot(Player& p, float dt) {
    Aizkolari& a = p.aizk;
    if (a.phase() != Aizkolari::READY) {
        p.botRaise = approach(p.botRaise, 0.0f, 10, dt);
        return;
    }
    p.botT -= dt;
    p.botRaise = approach(p.botRaise, p.botT < 0.25f ? 1.0f : 0.15f, 9, dt);
    if (p.botT > 0 || p.botRaise < 0.9f) return;
    const float R = LOG_RADIUS;
    if (a.side() == 0 && p.log.depth[0] >= TURN_PROMPT * R) {
        a.turnAround();
        p.botT = 0.5f;
        return;
    }
    int last = p.lastSide[a.side()];
    float tilt = last == HIT_LEFT ? 1.0f : -1.0f;
    if (frand() < 0.07f) tilt = -tilt;
    a.chop(frand(0.6f, 1.0f), tilt);
    p.botT = frand(0.3f, 0.5f);
}

void AizkoScene::updatePlayer(Player& p, float dt, u32 downAll) {
    (void)downAll;
    const float R = LOG_RADIUS;
    const input::Pad& in = input::pad(padFor(p));
    Aizkolari& a = p.aizk;
    // El hacha sigue al mando; durante la bajada se inclina hacia la diagonal
    a.setLiveTilt(!p.bot && in.state == input::SW_SWING ? in.swingTilt : 0.0f);
    a.setRaise(p.bot ? p.botRaise : in.raise);

    p.stT += dt;
    bool playing = state_ == ST_PLAY && p.st == P_PLAY;
    if (playing) {
        if (!p.bot) {
            if (in.swingStart) a.startSwing();
            if (in.chop.happened) a.chop(in.chop.strength, in.chop.tilt);
            else if (in.swingAbort) a.cancelSwing();
            if ((in.down & WPAD_BUTTON_B) && a.side() == 0 && p.log.depth[0] >= TURN_ALLOWED * R && !a.busy())
                a.turnAround();
            p.idleT = (in.raise < 0.45f && a.phase() == Aizkolari::READY) ? p.idleT + dt : 0;
        } else {
            updateBot(p, dt);
        }
    }
    switch (p.st) {
        case P_PLAY:
            break;
        case P_SPLIT:
            if (p.stT > 2.3f) {
                p.logIndex++;
                if (p.logIndex < cfg_.numLogs) {
                    p.st = P_NEXTLOG;
                    p.stT = 0;
                } else {
                    p.st = P_DONE;
                    p.timeMs = (unsigned)(p.timer * 1000.0f);
                    p.rank = ++finished_;
                    audio::play(SFX_BELL, 0.7f);
                    audio::play(SFX_CLAP, 0.6f);
                    sharedWorld().clapLight(2.5f);
                }
            }
            break;
        case P_NEXTLOG:
            if (p.stT >= 0.4f && p.stT - dt < 0.4f) {
                resetLog(p);
                p.aizk.setReady();
            }
            if (p.stT > 0.9f) {
                p.st = P_PLAY;
                p.stT = 0;
                p.running = true;
                audio::play(SFX_GO, 0.5f);
            }
            break;
        case P_DONE:
            break;
    }

    if (p.running) p.timer += dt;
    p.fatigue = fmaxf(0.0f, p.fatigue - FATIGUE_RECOVER * dt);
    a.fatigueLook = clampv((p.fatigue - 0.4f) / 0.6f, 0.0f, 1.0f);
    // Pausa de impacto: el aizkolari y las astillas se congelan un instante
    if (p.hitstop > 0) {
        p.hitstop -= dt;
    } else {
        a.update(dt);
        if (a.impact() && state_ == ST_PLAY && p.st == P_PLAY) handleImpact(p);
        p.chips.update(dt);
    }
    p.log.update(dt);
    p.sideFlash = fmaxf(0.0f, p.sideFlash - dt * 1.5f);
    p.strengthShow = fmaxf(0.0f, p.strengthShow - dt * 0.5f);
    if (in.state == input::SW_SWING) p.needleTarget = in.swingTilt;
    if (in.chop.happened) p.needleTarget = in.chop.tilt;
    p.needle = approach(p.needle, p.needleTarget, 18, dt);
}

void AizkoScene::enterResults() {
    state_ = ST_RESULTS;
    stateT_ = 0;
    resultsT_ = 0;
    for (int i = 0; i < MAXP; i++) order_[i] = i;
    for (int i = 0; i < n_; i++)
        for (int j = i + 1; j < n_; j++)
            if (pl_[order_[j]].timeMs < pl_[order_[i]].timeMs) std::swap(order_[i], order_[j]);
    int slot = cfg_.training ? REC_TRAINING : (REC_COMP_1 + clampv(cfg_.numLogs, 1, 3) - 1);
    Record& rec = save::data().records[slot];
    Player& best = pl_[order_[0]];
    if (!best.bot && (rec.timeMs == 0 || best.timeMs < rec.timeMs)) {
        rec.timeMs = best.timeMs;
        memcpy(rec.name, best.prof->name, NAME_LEN + 1);
        best.record = true;
        save::write();
    }
    resultsMenu_ = ui::ListMenu();
    resultsMenu_.count = 2;
    resultsMenu_.horizontal = true;
    audio::play(SFX_CHEER, 0.6f);
    sharedWorld().cheer(2.5f);
}

void AizkoScene::update(float dt) {
    time_ += dt;
    World& world = sharedWorld();
    u32 downAll = input::allDown();

    if (paused_) {
        int r = pauseMenu_.update(dt, downAll);
        if (r == 0 || r == -2 || (downAll & WPAD_BUTTON_PLUS)) paused_ = false;
        else if (r == 1) scenes::go(makeAizkoScene(cfg_));
        else if (r == 2) scenes::go(makeMainMenu());
        return;
    }
    if (!showHowTo_ && state_ != ST_RESULTS && (downAll & WPAD_BUTTON_PLUS)) {
        paused_ = true;
        pauseMenu_ = ui::ListMenu();
        pauseMenu_.count = 3;
        input::stopRumble();
        audio::play(SFX_OK, 0.6f);
        return;
    }

    stateT_ += dt;
    switch (state_) {
        case ST_INTRO:
            if (showHowTo_) {
                howToT_ += dt;
                stateT_ = 0;
                if (howToT_ > 0.6f && (downAll & WPAD_BUTTON_A)) {
                    showHowTo_ = false;
                    audio::play(SFX_OK, 0.7f);
                }
                break;
            }
            if (stateT_ > 2.6f || (stateT_ > 0.6f && (downAll & WPAD_BUTTON_A))) {
                state_ = ST_COUNTDOWN;
                stateT_ = 0;
                for (int i = 0; i < n_; i++) pl_[i].aizk.setReady();
                audio::play(SFX_BEEP, 0.6f);
            }
            break;
        case ST_COUNTDOWN:
            if (stateT_ >= 1.3f) {
                state_ = ST_PLAY;
                stateT_ = 0;
                for (int i = 0; i < n_; i++) pl_[i].running = true;
                audio::play(SFX_GO, 0.7f);
                audio::play(SFX_BELL, 0.5f);
            }
            break;
        case ST_PLAY:
            if (finished_ >= n_) {
                state_ = ST_FINISH;
                stateT_ = 0;
            }
            break;
        case ST_FINISH:
            if (stateT_ > 3.0f || (stateT_ > 1.0f && (downAll & WPAD_BUTTON_A))) enterResults();
            break;
        case ST_RESULTS:
            resultsT_ += dt;
            if (resultsT_ > 0.8f) {
                int r = resultsMenu_.update(dt, downAll);
                if (r == 0) scenes::go(makeAizkoScene(cfg_));
                else if (r == 1 || r == -2) scenes::go(makeMainMenu());
            }
            break;
    }

    for (int i = 0; i < n_; i++) updatePlayer(pl_[i], dt, downAll);
    world.update(dt);
    for (int i = 0; i < numLabels_; i++) labels_[i].t += dt;
    while (numLabels_ > 0 && labels_[0].t > 1.1f) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * (numLabels_ - 1));
        numLabels_--;
    }
    shake_ = approach(shake_, 0, 10, dt);
    updateCamera(dt);
}

void AizkoScene::updateCamera(float dt) {
    Vec3 eye, target;
    if (!multi()) {
        // Entrenamiento: tres cuartos por delante del aizkolari, gira con él al darse la vuelta
        Player& p = pl_[0];
        float camYaw = p.aizk.yaw() + 1.0f;
        float dist = 4.0f, height = 1.2f;
        target = p.log.center() + Vec3(0, 0.9f, 0);
        if (state_ == ST_INTRO) {
            float k = smooth01(stateT_ / 2.2f);
            camYaw += (1 - k) * 1.3f;
            dist += (1 - k) * 3.5f;
            height += (1 - k) * 1.6f;
        } else if (p.st == P_SPLIT) {
            float k = smooth01(p.stT / 1.0f);
            dist -= 0.6f * k;
            camYaw += 0.3f * k;
            target = target - Vec3(0, 0.25f * k, 0);
        } else if (state_ == ST_FINISH || state_ == ST_RESULTS) {
            camYaw += time_ * 0.15f;
            dist += 0.8f;
        }
        eye = target + Vec3(sinf(camYaw) * dist, height, cosf(camYaw) * dist);
    } else {
        // Competición: cámara fija por delante que encuadra todos los troncos
        float halfW = (n_ - 1) * SPACING * 0.5f + 1.3f;
        float dist = fmaxf(4.2f, halfW / 0.72f);
        target = Vec3(0, 0.95f, 0);
        eye = Vec3(0, 1.6f + dist * 0.18f, dist);
        if (state_ == ST_INTRO) {
            float k = smooth01(stateT_ / 2.4f);
            eye = eye + Vec3((1 - k) * 4.0f, (1 - k) * 2.5f, (1 - k) * 3.0f);
        } else if (state_ == ST_FINISH || state_ == ST_RESULTS) {
            eye = eye + Vec3(sinf(time_ * 0.3f) * 1.5f, 0.3f, 0);
        }
    }
    camEye_ = approachv(camEye_, eye, 5.0f, dt);
    camTarget_ = approachv(camTarget_, target, 6.0f, dt);
}

void AizkoScene::draw() {
    World& world = sharedWorld();
    gfx::begin2D();
    world.drawSky();
    Camera cam;
    Vec3 jitter(frand(-1, 1) * shake_, frand(-1, 1) * shake_, frand(-1, 1) * shake_);
    cam.eye = camEye_ + jitter;
    cam.target = camTarget_ + jitter * 0.5f;
    gfx::begin3D(cam);
    world.setCameraHide(cam.eye, multi());
    world.draw();
    for (int i = 0; i < n_; i++) world.drawShadow(pl_[i].log.center(), LOG_LENGTH * 0.55f, LOG_RADIUS * 1.6f);
    for (int i = 0; i < n_; i++) {
        pl_[i].log.draw();
        pl_[i].aizk.draw();
        pl_[i].chips.draw();
    }

    gfx::begin2D();
    if (showHowTo_) {
        drawHowTo();
        return;
    }
    drawCommon();
    if (state_ != ST_RESULTS) {
        if (multi()) drawMultiHud();
        else drawTrainingHud();
    }
    if (state_ == ST_RESULTS) drawResults();
    if (paused_) drawPause();
}

// Etiquetas flotantes, mensajes de estado y tiempo
void AizkoScene::drawCommon() {
    const float W = gfx::screenW();
    char buf[64];
    for (int i = 0; i < numLabels_; i++) {
        const FloatLabel& l = labels_[i];
        float sx, sy;
        if (!gfx::project(l.pos, sx, sy)) continue;
        float pop = easeOutBack(clampv(l.t / 0.2f, 0.0f, 1.0f));
        float a = 1.0f - smooth01((l.t - 0.7f) / 0.4f);
        font::draw(l.text, sx, sy - 30 - l.t * 40, (multi() ? 24 : 30) * pop + 2, l.color, rgba(0x30, 0x30, 0x38), ALIGN_CENTER, a);
    }
    if (state_ == ST_RESULTS) return;

    // Nombres sobre las cabezas en la competición
    if (multi() && (state_ == ST_INTRO || state_ == ST_COUNTDOWN || state_ == ST_FINISH)) {
        for (int i = 0; i < n_; i++) {
            float sx, sy;
            if (!gfx::project(pl_[i].aizk.feet() + Vec3(0, 1.75f, 0), sx, sy)) continue;
            ui::colorDot(sx - font::width(pl_[i].prof->name, 24) / 2 - 16, sy + 14, 8, FAV_COLORS[pl_[i].prof->color % NUM_COLORS]);
            font::draw(pl_[i].prof->name, sx + 6, sy, 24, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER);
        }
    }

    if (state_ == ST_INTRO) {
        float a = smooth01(stateT_ / 0.4f) * (1 - smooth01((stateT_ - 2.2f) / 0.4f));
        float y = 120 + (1 - easeOutCubic(stateT_ / 0.5f)) * -40;
        font::draw(cfg_.training ? txt::TRAINING : txt::COMPETITION, W / 2, y, 56, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, a);
        if (cfg_.numLogs == 1) snprintf(buf, sizeof(buf), "%s", txt::LOG_1);
        else snprintf(buf, sizeof(buf), txt::LOGS_N, cfg_.numLogs);
        font::draw(buf, W / 2, y + 66, 30, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, a);
    } else if (state_ == ST_COUNTDOWN) {
        float s = easeOutBack(clampv(stateT_ / 0.3f, 0.0f, 1.0f));
        font::draw(txt::READY, W / 2, 140, 64 * s + 1, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER);
    } else if (state_ == ST_PLAY && stateT_ < 0.8f) {
        float s = easeOutBack(clampv(stateT_ / 0.25f, 0.0f, 1.0f));
        font::draw(txt::GO, W / 2, 130, 80 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER, 1 - smooth01((stateT_ - 0.5f) / 0.3f));
    } else if (state_ == ST_FINISH) {
        float s = easeOutBack(clampv(stateT_ / 0.3f, 0.0f, 1.0f));
        font::draw(txt::FINISHED, W / 2, 110, 64 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER);
        if (!multi()) {
            ui::formatTime(pl_[0].timeMs, buf, sizeof(buf));
            font::draw(buf, W / 2, 190, 72, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER, smooth01((stateT_ - 0.3f) / 0.3f));
        }
    }

    // Tiempo (el del primero que sigue cortando, o el del único jugador)
    float tshow = 0;
    for (int i = 0; i < n_; i++) tshow = fmaxf(tshow, pl_[i].timer);
    ui::formatTime((unsigned)(tshow * 1000), buf, sizeof(buf));
    font::plain(txt::TIME, W - 40, 24, 20, rgba(0x30, 0x40, 0x50), ALIGN_RIGHT);
    font::draw(buf, W - 40, 44, 46, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_RIGHT);
}

// Medidor vertical (altura del hacha o fuerza del golpe)
static void meter(float x, float y, float w, float h, float fill, Color c) {
    gfx::roundRect(x - 3, y - 3, w + 6, h + 6, w / 2 + 3, rgba(0, 0, 0, 90));
    gfx::roundRect(x, y, w, h, w / 2, rgba(255, 255, 255, 150));
    fill = clampv(fill, 0.0f, 1.0f);
    if (fill > 0.04f) gfx::roundRect(x, y + h * (1 - fill), w, h * fill, w / 2, c);
}

// Esquema del tronco visto desde arriba con las dos V
static void notchDiagram(float cx, float top, float w, float h, const Log& log, int side) {
    const float R = LOG_RADIUS;
    gfx::roundRect(cx - w / 2 - 5, top - 5, w + 10, h + 10, 10, rgba(255, 255, 255, 190));
    gfx::rect(cx - w / 2, top, w, h, BARK);
    gfx::rect(cx - w / 2, top + 3, w, h - 6, WOOD_MID);
    Color hole = rgba(0xEA, 0xF4, 0xFA);
    float df = log.depth[1] / (2 * R) * h, db = log.depth[0] / (2 * R) * h;
    if (df > 0.5f) gfx::triangle(cx - df, top, cx + df, top, cx, top + df, hole);
    if (db > 0.5f) gfx::triangle(cx - db, top + h, cx + db, top + h, cx, top + h - db, hole);
    float ay = side == 0 ? top + h + 10 : top - 10;
    float dir = side == 0 ? 1.0f : -1.0f;
    gfx::triangle(cx - 7, ay, cx + 7, ay, cx, ay - dir * 7, UI_RED);
}

void AizkoScene::drawTrainingHud() {
    const float W = gfx::screenW();
    const float R = LOG_RADIUS;
    char buf[64];
    Player& p = pl_[0];
    const Profile& pr = *p.prof;

    ui::panel(34, 26, 250, 74);
    ui::colorDot(62, 52, 11, FAV_COLORS[pr.color % NUM_COLORS]);
    font::plain(pr.name, 82, 34, 28, UI_TEXT);
    if (p.bot) font::plain(txt::CPU, 268, 38, 20, UI_TEXT_LIGHT, ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), txt::LOG_N_OF, clampv(p.logIndex + 1, 1, cfg_.numLogs), cfg_.numLogs);
    font::plain(buf, 50, 68, 22, UI_TEXT_LIGHT);
    notchDiagram(W - 40 - 75, 112, 150, 48, p.log, p.aizk.side());

    if (!p.bot && (state_ == ST_PLAY || state_ == ST_COUNTDOWN)) {
        const input::Pad& in = input::pad(padFor(p));
        float cx = W / 2, cy = 430, r = 62;
        int last = p.lastSide[p.aizk.side()];
        int want = last == HIT_LEFT ? HIT_RIGHT : last == HIT_RIGHT ? HIT_LEFT : HIT_NONE;
        float pulse = 0.6f + 0.4f * sinf(time_ * 8);
        auto ang = [&](float t) { return 1.5f * PI + t * 0.5f * PI; };
        gfx::arc(cx, cy, r - 18, r + 4, PI, 2 * PI, rgba(0, 0, 0, 80), 24);
        Color cl = want == HIT_LEFT ? mixColor(UI_BLUE, UI_WHITE, 1 - pulse) : rgba(0x9C, 0xC8, 0xEC);
        Color cr = want == HIT_RIGHT ? mixColor(UI_BLUE, UI_WHITE, 1 - pulse) : rgba(0x9C, 0xC8, 0xEC);
        gfx::arc(cx, cy, r - 14, r, ang(-1), ang(0), cl, 12);
        gfx::arc(cx, cy, r - 14, r, ang(0), ang(1), cr, 12);
        if (state_ == ST_PLAY && !(p.aizk.side() == 0 && p.log.depth[0] >= SIDE_MAX * R - 0.001f)) {
            int next = want == HIT_NONE ? HIT_RIGHT : want;
            const char* label = next == HIT_LEFT ? "<<  EZKERRA" : "ESKUINA  >>";
            Color c = next == HIT_LEFT ? rgba(0xFF, 0xA0, 0x40) : rgba(0x6C, 0xC4, 0xFF);
            float s = 1.0f + 0.04f * sinf(time_ * 6);
            font::plain(txt::NEXT, W / 2, 62, 22, UI_WHITE, ALIGN_CENTER);
            font::draw(label, W / 2, 84, 44 * s, c, rgba(0x20, 0x30, 0x40), ALIGN_CENTER);
        }
        if (p.sideFlash > 0 && p.sideFlashHit != HIT_STRAIGHT) {
            bool left = p.sideFlashHit == HIT_LEFT;
            font::draw(left ? txt::LEFT : txt::RIGHT, left ? cx - r - 12 : cx + r + 12, cy - 100, 34 * (1 + 0.3f * p.sideFlash),
                       UI_GOLD, rgba(0x60, 0x30, 0x10), left ? ALIGN_RIGHT : ALIGN_LEFT, smooth01(p.sideFlash * 2));
        }
        float na = ang(clampv(p.needle, -1.0f, 1.0f) * 0.85f);
        gfx::triangle(cx + cosf(na) * (r + 6), cy + sinf(na) * (r + 6), cx + cosf(na + 1.57f) * 6, cy + sinf(na + 1.57f) * 6,
                      cx + cosf(na - 1.57f) * 6, cy + sinf(na - 1.57f) * 6, rgba(0x30, 0x30, 0x38));
        gfx::circle(cx, cy, 7, rgba(0x30, 0x30, 0x38), 12);
        font::draw(txt::LEFT, cx - r - 12, cy - 30, 22, want == HIT_LEFT ? UI_WHITE : rgba(0xE0, 0xE0, 0xE0), rgba(0x20, 0x30, 0x40), ALIGN_RIGHT);
        font::draw(txt::RIGHT, cx + r + 12, cy - 30, 22, want == HIT_RIGHT ? UI_WHITE : rgba(0xE0, 0xE0, 0xE0), rgba(0x20, 0x30, 0x40), ALIGN_LEFT);

        // Altura del hacha ("JO!" cuando ya se puede golpear) y fuerza del último golpe
        bool armed = in.state == input::SW_ARMED || in.state == input::SW_RAISING;
        float bx = cx + r + 110, bh = 86, by = 466 - bh;
        meter(bx, by, 22, bh, armed ? 1.0f : in.raise, armed ? UI_GREEN : mixColor(UI_BLUE, UI_GOLD, in.raise));
        if (armed) font::draw(txt::HIT_NOW, bx + 11, by - 40, 32 * (1 + 0.08f * sinf(time_ * 14)), UI_GREEN, UI_WHITE, ALIGN_CENTER);
        else font::plain(txt::RAISE_LABEL, bx + 11, by - 26, 18, UI_WHITE, ALIGN_CENTER);
        float sx = cx - r - 132;
        Color sc = mixColor(UI_GOLD, UI_RED, p.strength);
        meter(sx, by, 22, bh, p.strength, withAlpha(sc, 0.4f + 0.6f * p.strengthShow));
        font::plain(txt::STRENGTH, sx + 11, by - 26, 18, UI_WHITE, ALIGN_CENTER);
    }

    if (state_ == ST_PLAY && p.idleT > 2.5f && !p.bot)
        font::draw(txt::RAISE_AXE, W / 2, 200, 40, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, smooth01((p.idleT - 2.5f) / 0.4f));
    if (state_ == ST_PLAY && p.st == P_PLAY && p.aizk.side() == 0 && !p.aizk.turning() && p.log.depth[0] >= TURN_PROMPT * R) {
        float s = 1.0f + 0.06f * sinf(time_ * 9);
        font::draw(txt::TURN_AROUND, W / 2, 140, 50 * s, UI_GOLD, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
        if (!p.bot) {
            float tw = font::width(txt::PRESS_B, 26);
            font::draw(txt::PRESS_B, W / 2 - 14, 200, 26, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER);
            ui::buttonIcon(W / 2 + tw / 2 + 4, 216, "B", 15, rgba(0x80, 0x88, 0x94));
        }
    }
    if (p.st == P_SPLIT) {
        float s = easeOutBack(clampv(p.stT / 0.3f, 0.0f, 1.0f));
        font::draw(txt::LOG_CUT, W / 2, 130, 56 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER);
    } else if (p.st == P_NEXTLOG) {
        ui::fade(clampv(1 - fabsf(p.stT - 0.4f) / 0.4f, 0.0f, 1.0f), UI_WHITE);
    }
}

// Competición: un panel por jugador abajo, y avisos sobre cada aizkolari
void AizkoScene::drawMultiHud() {
    const float W = gfx::screenW();
    const float R = LOG_RADIUS;
    char buf[64];
    float slot = fminf(270.0f, (W - 40) / n_);
    float x0 = W / 2 - slot * n_ / 2;
    for (int i = 0; i < n_; i++) {
        Player& p = pl_[i];
        const input::Pad& in = input::pad(padFor(p));
        float px = x0 + i * slot + 6, py = 366, pw = slot - 12, ph = 98;
        Color col = FAV_COLORS[p.prof->color % NUM_COLORS];
        gfx::roundRect(px + 2, py + 4, pw, ph, 16, rgba(0, 0, 0, 50));
        gfx::roundRect(px, py, pw, ph, 16, rgba(255, 255, 255, 225));
        gfx::roundRect(px, py, pw, 6, 3, col);
        ui::colorDot(px + 18, py + 24, 8, col);
        font::plain(p.prof->name, px + 32, py + 11, 22, UI_TEXT);
        if (p.bot) font::plain(txt::CPU, px + pw - 10, py + 13, 16, UI_TEXT_LIGHT, ALIGN_RIGHT);
        if (p.st == P_DONE) {
            snprintf(buf, sizeof(buf), "%d.", p.rank);
            font::draw(buf, px + 30, py + 44, 36, p.rank == 1 ? UI_GOLD : UI_WHITE, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
            ui::formatTime(p.timeMs, buf, sizeof(buf));
            font::plain(buf, px + pw - 12, py + 50, 30, UI_TEXT, ALIGN_RIGHT);
            continue;
        }
        snprintf(buf, sizeof(buf), "%d/%d", clampv(p.logIndex + 1, 1, cfg_.numLogs), cfg_.numLogs);
        font::plain(buf, px + 12, py + 40, 18, UI_TEXT_LIGHT);
        notchDiagram(px + 30 + 12, py + 66, 56, 20, p.log, p.aizk.side());
        if (!p.bot && state_ == ST_PLAY && p.st == P_PLAY) {
            int last = p.lastSide[p.aizk.side()];
            int next = last == HIT_LEFT ? HIT_RIGHT : HIT_LEFT;
            if (last == HIT_NONE) next = HIT_RIGHT;
            Color c = next == HIT_LEFT ? rgba(0xE8, 0x80, 0x20) : rgba(0x2C, 0x9C, 0xE8);
            font::plain(next == HIT_LEFT ? "<<" : ">>", px + pw * 0.55f, py + 46, 36, c, ALIGN_CENTER);
            bool armed = in.state == input::SW_ARMED || in.state == input::SW_RAISING;
            meter(px + pw - 26, py + 36, 12, 52, armed ? 1.0f : in.raise, armed ? UI_GREEN : mixColor(UI_BLUE, UI_GOLD, in.raise));
            meter(px + pw - 46, py + 36, 12, 52, p.strength, withAlpha(mixColor(UI_GOLD, UI_RED, p.strength), 0.4f + 0.6f * p.strengthShow));
        }
        if (!input::pad(p.pad).connected && !p.bot) {
            snprintf(buf, sizeof(buf), txt::CONNECT_REMOTE, p.pad + 1);
            font::plain(buf, px + pw / 2, py + 74, 14, UI_RED, ALIGN_CENTER);
        }

        // Avisos sobre la cabeza de cada aizkolari
        float sx, sy;
        if (!gfx::project(p.aizk.feet() + Vec3(0, 1.9f, 0), sx, sy)) continue;
        if (p.st == P_PLAY && state_ == ST_PLAY && p.aizk.side() == 0 && !p.aizk.turning() && p.log.depth[0] >= TURN_PROMPT * R) {
            float s = 1.0f + 0.06f * sinf(time_ * 9);
            font::draw(txt::TURN_AROUND, sx, sy - 30, 24 * s, UI_GOLD, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
            if (!p.bot) ui::buttonIcon(sx, sy + 8, "B", 12, rgba(0x80, 0x88, 0x94));
        } else if (p.st == P_SPLIT) {
            font::draw(txt::LOG_CUT, sx, sy - 30, 22, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER);
        } else if (p.st == P_DONE) {
            snprintf(buf, sizeof(buf), "%d.", p.rank);
            font::draw(buf, sx, sy - 40, 44, p.rank == 1 ? UI_GOLD : UI_WHITE, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
        }
    }
}

void AizkoScene::drawResults() {
    const float W = gfx::screenW();
    char buf[64];
    float a = smooth01(resultsT_ / 0.4f);
    ui::fade(0.35f * a);
    const float rowH = n_ > 2 ? 46 : 56;
    float pw = 560, ph = 70 + n_ * rowH + (n_ > 1 ? 52 : 20);
    float px = W / 2 - pw / 2, py = 50 + (1 - easeOutCubic(resultsT_ / 0.5f)) * 60;
    ui::panel(px, py, pw, ph);
    font::plain(txt::RESULTS, W / 2, py + 14, 36, UI_TEXT, ALIGN_CENTER);
    for (int i = 0; i < n_; i++) {
        const Player& p = pl_[order_[i]];
        float y = py + 66 + i * rowH;
        if (i == 0) gfx::roundRect(px + 16, y - 4, pw - 32, rowH - 6, 16, rgba(0xFF, 0xF0, 0xB8));
        snprintf(buf, sizeof(buf), "%d.", i + 1);
        font::plain(buf, px + 34, y + 4, 30, i == 0 ? rgba(0xD8, 0x98, 0x10) : UI_TEXT_LIGHT);
        ui::colorDot(px + 92, y + 21, 10, FAV_COLORS[p.prof->color % NUM_COLORS]);
        font::plain(p.prof->name, px + 114, y + 4, 30, UI_TEXT);
        ui::formatTime(p.timeMs, buf, sizeof(buf));
        font::plain(buf, px + pw - 36, y + 4, 30, UI_TEXT, ALIGN_RIGHT);
        if (p.record) {
            float s = 1 + 0.05f * sinf(time_ * 8);
            font::draw(txt::NEW_RECORD, px + pw - 150, y + 8, 20 * s, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_RIGHT);
        }
    }
    if (n_ > 1) {
        snprintf(buf, sizeof(buf), "%s: %s!", txt::WINNER, pl_[order_[0]].prof->name);
        font::draw(buf, W / 2, py + ph - 50, 34, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER, a);
    }
    float by = fminf(430.0f, py + ph + 44);
    ui::button(W / 2 - 150, by, 260, 60, txt::PLAY_AGAIN, resultsMenu_.anim[0]);
    ui::button(W / 2 + 150, by, 260, 60, txt::TO_MENU, resultsMenu_.anim[1]);
}

// Línea gruesa (dos triángulos)
static void thickLine(float x0, float y0, float x1, float y1, float w, Color c) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    if (l < 0.01f) return;
    float nx = -dy / l * w / 2, ny = dx / l * w / 2;
    gfx::triangle(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, c);
    gfx::triangle(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, c);
}

static void arrow(float x0, float y0, float x1, float y1, Color c) {
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    float ux = dx / l, uy = dy / l;
    thickLine(x0, y0, x1 - ux * 14, y1 - uy * 14, 8, c);
    gfx::triangle(x1, y1, x1 - ux * 20 - uy * 12, y1 - uy * 20 + ux * 12, x1 - ux * 20 + uy * 12, y1 - uy * 20 - ux * 12, c);
}

// Wii Remote visto de frente, vertical, con la punta (sensor) arriba
static void remoteIcon(float cx, float cy, float s, bool thumb) {
    float w = 26 * s, h = 96 * s;
    gfx::roundRect(cx - w / 2 + 2, cy - h / 2 + 3, w, h, 9 * s, rgba(0, 0, 0, 50));
    gfx::roundRect(cx - w / 2, cy - h / 2, w, h, 9 * s, UI_WHITE);
    gfx::rect(cx - w / 2 + 3 * s, cy - h / 2 + 2 * s, w - 6 * s, 6 * s, rgba(0x30, 0x30, 0x38));
    gfx::rect(cx - 7 * s, cy - h / 2 + 20 * s, 14 * s, 4 * s, rgba(0x70, 0x74, 0x7C));
    gfx::rect(cx - 2 * s, cy - h / 2 + 15 * s, 4 * s, 14 * s, rgba(0x70, 0x74, 0x7C));
    gfx::circle(cx, cy - h / 2 + 38 * s, 6 * s, rgba(0xC8, 0xCC, 0xD4), 16);
    gfx::circle(cx, cy + h / 2 - 30 * s, 4 * s, rgba(0xC8, 0xCC, 0xD4), 12);
    gfx::circle(cx, cy + h / 2 - 18 * s, 4 * s, rgba(0xC8, 0xCC, 0xD4), 12);
    if (thumb) {
        gfx::circle(cx + 3 * s, cy + h / 2 - 24 * s, 11 * s, rgba(0xF2, 0xBC, 0x8C, 0xE0), 20);
        gfx::roundRect(cx - w / 2 - 6 * s, cy + h / 2 - 34 * s, w + 12 * s, 30 * s, 10 * s, rgba(0xF2, 0xBC, 0x8C, 0x90));
    }
}

void AizkoScene::drawHowTo() {
    const float W = gfx::screenW();
    float a = smooth01(howToT_ / 0.3f);
    ui::fade(0.45f * a);
    float pw = 760, ph = 330, px = W / 2 - pw / 2, py = 60 + (1 - easeOutCubic(howToT_ / 0.4f)) * 40;
    ui::panel(px, py, pw, ph);
    font::plain(txt::HOW_TO, W / 2, py + 12, 36, UI_TEXT, ALIGN_CENTER);
    const char* t1[3] = {txt::HOW_1, txt::HOW_2, txt::HOW_3};
    const char* t2[3] = {txt::HOW_1B, txt::HOW_2B, txt::HOW_3B};
    for (int i = 0; i < 3; i++) {
        float cx = px + pw * (i + 0.5f) / 3, cy = py + 160;
        gfx::circle(cx - 92, py + 74, 16, UI_BLUE, 20);
        char num[2] = {(char)('1' + i), 0};
        font::plain(num, cx - 92, py + 61, 24, UI_WHITE, ALIGN_CENTER);
        if (i == 0) {
            remoteIcon(cx, cy, 1.35f, true);
        } else if (i == 1) {
            gfx::circle(cx - 10, cy + 55, 26, rgba(0xF2, 0xBC, 0x8C), 24);
            gfx::rect(cx - 36, cy + 42, 52, 10, rgba(0x1C, 0x1E, 0x26));
            remoteIcon(cx + 56, cy - 22, 0.9f, false);
            arrow(cx + 20, cy + 70, cx + 46, cy + 22, UI_GREEN);
            font::draw(txt::HIT_NOW, cx + 56, cy - 88, 26, UI_GREEN, UI_WHITE, ALIGN_CENTER);
        } else {
            arrow(cx + 56, cy - 50, cx + 6, cy + 48, rgba(0x6C, 0xC4, 0xFF));
            arrow(cx - 56, cy - 50, cx - 6, cy + 48, rgba(0xFF, 0xA0, 0x40));
            remoteIcon(cx + 70, cy - 70, 0.6f, false);
            remoteIcon(cx - 70, cy - 70, 0.6f, false);
        }
        font::plain(t1[i], cx, py + 236, 24, UI_TEXT, ALIGN_CENTER);
        font::plain(t2[i], cx, py + 266, 19, UI_TEXT_LIGHT, ALIGN_CENTER);
    }
    if (howToT_ > 0.6f) ui::hints(txt::OK, nullptr);
}

void AizkoScene::drawPause() {
    const float W = gfx::screenW();
    ui::fade(0.5f);
    float pw = 420, ph = 330;
    ui::panel(W / 2 - pw / 2, 240 - ph / 2, pw, ph);
    font::plain(txt::PAUSE, W / 2, 240 - ph / 2 + 18, 38, UI_TEXT, ALIGN_CENTER);
    const char* items[3] = {txt::CONTINUE, txt::RESTART, txt::TO_MENU};
    for (int i = 0; i < 3; i++) ui::button(W / 2, 175 + i * 76, 320, 62, items[i], pauseMenu_.anim[i]);
}

}  // namespace

Scene* makeAizkoScene(const AizkoConfig& cfg) { return new AizkoScene(cfg); }
