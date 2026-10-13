// Aizkolaritza: la partida (entrenamiento o lehiaketa por turnos).
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

enum State { ST_INTRO, ST_COUNTDOWN, ST_PLAY, ST_SPLIT, ST_NEXTLOG, ST_FINISH, ST_RESULTS };
enum SideHit { HIT_LEFT, HIT_RIGHT, HIT_STRAIGHT, HIT_NONE };

struct FloatLabel {
    char text[32];
    Vec3 pos;
    float t;
    Color color;
};

struct PlayerRes {
    unsigned timeMs = 0;
    bool done = false;
    bool record = false;
};

// Nombre + sufijo de genitivo: "Ane" → "Aneren", "Mikel" → "Mikelen"
static void turnText(const char* name, char* out, int size) {
    int n = strlen(name);
    char last = n ? name[n - 1] : 'a';
    bool vowel = strchr("aeiouAEIOU", last) != nullptr;
    snprintf(out, size, txt::TURN_OF, name, vowel ? "ren" : "en");
}

class AizkoScene : public Scene {
public:
    explicit AizkoScene(const AizkoConfig& cfg) : cfg_(cfg) {
        chips_.init();
        log_.pos = Vec3(0, 0, 0);
        startPlayer(0);
        camEye_ = Vec3(6, 3.5f, 7);
        camTarget_ = log_.center();
    }

    void update(float dt) override;
    void draw() override;

private:
    void startPlayer(int idx);
    void resetLog();
    void handleImpact();
    void finishPlayer();
    void enterResults();
    void updateBot(float dt);
    void addLabel(const char* t, const Vec3& p, Color c);
    void drawHud();
    void drawResults();
    void drawPause();
    int padFor(int player) const;
    const Profile& profile(int player) const { return save::data().profiles[cfg_.profile[player] % MAX_PROFILES]; }

    AizkoConfig cfg_;
    State state_ = ST_INTRO;
    float stateT_ = 0;
    int cur_ = 0;
    int logIndex_ = 0;
    Log log_;
    Chips chips_;
    Aizkolari aizk_;
    float timer_ = 0;
    bool timerRunning_ = false;
    float fatigue_ = 0;
    int lastSide_[2] = {HIT_NONE, HIT_NONE};
    PlayerRes res_[4];
    FloatLabel labels_[8];
    int numLabels_ = 0;
    float shake_ = 0;
    Vec3 camEye_, camTarget_;
    float botT_ = 1.0f;
    float tipT_ = 0;
    int tip_ = 0;
    float time_ = 0;
    // pausa y resultados
    bool paused_ = false;
    ui::ListMenu pauseMenu_;
    ui::ListMenu resultsMenu_;
    int order_[4] = {0, 1, 2, 3};
    float resultsT_ = 0;
};

int AizkoScene::padFor(int player) const {
    if (input::pad(player).connected) return player;
    return 0;
}

void AizkoScene::resetLog() {
    log_.reset();
    chips_.clear();
    aizk_.reset();
    lastSide_[0] = lastSide_[1] = HIT_NONE;
}

void AizkoScene::startPlayer(int idx) {
    cur_ = idx;
    logIndex_ = 0;
    timer_ = 0;
    timerRunning_ = false;
    fatigue_ = 0;
    aizk_.setup(Appearance::from(profile(idx)), &log_);
    resetLog();
    state_ = ST_INTRO;
    stateT_ = 0;
    botT_ = 1.0f;
}

void AizkoScene::addLabel(const char* t, const Vec3& p, Color c) {
    if (numLabels_ == 8) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * 7);
        numLabels_--;
    }
    FloatLabel& l = labels_[numLabels_++];
    strncpy(l.text, t, sizeof(l.text) - 1);
    l.text[sizeof(l.text) - 1] = 0;
    l.pos = p;
    l.t = 0;
    l.color = c;
}

void AizkoScene::handleImpact() {
    const float R = LOG_RADIUS;
    float strength = aizk_.impactStrength();
    float tilt = aizk_.impactTilt();
    int hit = tilt < -TILT_STRAIGHT ? HIT_LEFT : tilt > TILT_STRAIGHT ? HIT_RIGHT : HIT_STRAIGHT;
    int notch = aizk_.side();
    Vec3 at = log_.impactPoint(notch, tilt);
    Vec3 outward(0, 0, notch == 0 ? 1.0f : -1.0f);
    int pad = padFor(cur_);
    bool human = !cfg_.bot[cur_];

    float factor = strength;
    const char* label = nullptr;
    Color lc = UI_WHITE;
    bool full = notch == 0 && log_.depth[0] >= SIDE_MAX * R - 0.001f;
    if (full) {
        factor = 0;
        label = txt::TURN_AROUND;
        lc = UI_GOLD;
    } else {
        if (hit == HIT_STRAIGHT) {
            factor *= STRAIGHT_FACTOR;
            label = txt::TOO_STRAIGHT;
            lc = rgba(0xFF, 0xB0, 0x60);
        } else if (hit == lastSide_[notch]) {
            factor *= REPEAT_SIDE_FACTOR;
            label = txt::SAME_SIDE;
            lc = rgba(0xFF, 0x90, 0x60);
        }
        if (fatigue_ > FATIGUE_HEAVY) {
            factor *= 0.6f;
            if (!label) { label = txt::TIRED; lc = rgba(0xC0, 0xC8, 0xFF); }
        }
        if (!label) {
            if (factor >= 0.75f) { label = txt::GREAT; lc = UI_GOLD; }
            else if (factor >= 0.45f) { label = txt::GOOD; lc = UI_WHITE; }
            else { label = txt::WEAK; lc = rgba(0xD0, 0xD0, 0xD0); }
        }
        float gain = CUT_PER_CHOP * R * factor;
        if (notch == 0) gain = fminf(gain, SIDE_MAX * R - log_.depth[0]);
        else gain = fminf(gain, 2 * R - log_.total());
        log_.addDepth(notch, fmaxf(0.0f, gain));
        chips_.spawn(at, outward, 2 + (int)(factor * 9), 0.5f + factor * 0.6f);
    }
    fatigue_ = fminf(1.0f, fatigue_ + FATIGUE_PER_CHOP);
    if (hit != HIT_STRAIGHT) lastSide_[notch] = hit;

    float pan = clampv(at.x * 0.5f, -1.0f, 1.0f);
    if (factor >= 0.45f) audio::play(SFX_CHOP, 0.7f + 0.3f * strength, frand(0.92f, 1.08f), pan);
    else audio::play(SFX_CHOP_WEAK, 0.8f, frand(0.9f, 1.1f), pan);
    if (human) input::rumble(pad, 0.05f + 0.08f * strength);
    shake_ = fmaxf(shake_, 0.02f + 0.035f * strength);
    addLabel(label, at + Vec3(0, 0.35f, 0), lc);
    if (factor >= 0.75f && frand() < 0.3f) sharedWorld().clapLight(1.0f);

    if (log_.cut()) {
        state_ = ST_SPLIT;
        stateT_ = 0;
        timerRunning_ = false;
        log_.startSplit();
        aizk_.celebrate();
        audio::play(SFX_SPLIT, 1.0f);
        audio::play(SFX_CHEER, 0.7f);
        sharedWorld().cheer(2.0f);
        chips_.spawn(log_.center() + Vec3(0, 0.1f, 0), Vec3(0, 1, 0), 18, 1.2f);
        if (human) input::rumble(pad, 0.35f);
    }
}

void AizkoScene::updateBot(float dt) {
    if (aizk_.phase() != Aizkolari::READY) return;
    botT_ -= dt;
    if (botT_ > 0) return;
    const float R = LOG_RADIUS;
    if (aizk_.side() == 0 && log_.depth[0] >= TURN_PROMPT * R) {
        aizk_.turnAround();
        botT_ = 0.5f;
        return;
    }
    if (fatigue_ > 0.62f) {
        botT_ = 0.25f;
        return;
    }
    int last = lastSide_[aizk_.side()];
    float tilt = last == HIT_LEFT ? 1.0f : -1.0f;
    if (frand() < 0.07f) tilt = -tilt;
    aizk_.chop(frand(0.7f, 1.0f), tilt);
    botT_ = frand(0.12f, 0.35f);
}

void AizkoScene::finishPlayer() {
    res_[cur_].timeMs = (unsigned)(timer_ * 1000.0f);
    res_[cur_].done = true;
    state_ = ST_FINISH;
    stateT_ = 0;
    audio::play(SFX_BELL, 0.8f);
    audio::play(SFX_CLAP, 0.7f);
    sharedWorld().clapLight(2.5f);
}

void AizkoScene::enterResults() {
    state_ = ST_RESULTS;
    stateT_ = 0;
    resultsT_ = 0;
    for (int i = 0; i < 4; i++) order_[i] = i;
    for (int i = 0; i < cfg_.numPlayers; i++)
        for (int j = i + 1; j < cfg_.numPlayers; j++)
            if (res_[order_[j]].timeMs < res_[order_[i]].timeMs) std::swap(order_[i], order_[j]);
    // Récords
    int slot = cfg_.training ? REC_TRAINING : (REC_COMP_1 + clampv(cfg_.numLogs, 1, 3) - 1);
    Record& rec = save::data().records[slot];
    int best = order_[0];
    bool anyRecord = false;
    if (!cfg_.bot[best] && res_[best].done && (rec.timeMs == 0 || res_[best].timeMs < rec.timeMs)) {
        rec.timeMs = res_[best].timeMs;
        strncpy(rec.name, profile(best).name, NAME_LEN);
        rec.name[NAME_LEN] = 0;
        res_[best].record = true;
        anyRecord = true;
    }
    if (anyRecord) save::write();
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
    if (state_ != ST_RESULTS && (downAll & WPAD_BUTTON_PLUS)) {
        paused_ = true;
        pauseMenu_ = ui::ListMenu();
        pauseMenu_.count = 3;
        input::stopRumble();
        audio::play(SFX_OK, 0.6f);
        return;
    }

    stateT_ += dt;
    int pad = padFor(cur_);
    const input::Pad& p = input::pad(pad);
    bool human = !cfg_.bot[cur_];
    aizk_.setLiveTilt(human ? p.tiltLive : 0.0f);

    switch (state_) {
        case ST_INTRO:
            if (stateT_ > 2.4f || (human && stateT_ > 0.6f && (p.down & WPAD_BUTTON_A))) {
                state_ = ST_COUNTDOWN;
                stateT_ = 0;
                aizk_.setReady();
                audio::play(SFX_BEEP, 0.6f);
            }
            break;
        case ST_COUNTDOWN:
            if (stateT_ >= 1.3f) {
                state_ = ST_PLAY;
                stateT_ = 0;
                timerRunning_ = true;
                audio::play(SFX_GO, 0.7f);
                audio::play(SFX_BELL, 0.5f);
            }
            break;
        case ST_PLAY: {
            const float R = LOG_RADIUS;
            if (human) {
                if (p.chop.happened) aizk_.chop(p.chop.strength, p.chop.tilt);
                if ((p.down & WPAD_BUTTON_B) && aizk_.side() == 0 && log_.depth[0] >= TURN_ALLOWED * R && !aizk_.busy())
                    aizk_.turnAround();
            } else {
                updateBot(dt);
            }
            break;
        }
        case ST_SPLIT:
            if (stateT_ > 2.3f) {
                logIndex_++;
                if (logIndex_ < cfg_.numLogs) {
                    state_ = ST_NEXTLOG;
                    stateT_ = 0;
                } else {
                    finishPlayer();
                }
            }
            break;
        case ST_NEXTLOG:
            if (stateT_ >= 0.4f && stateT_ - dt < 0.4f) {
                resetLog();
                aizk_.setReady();
            }
            if (stateT_ > 0.9f) {
                state_ = ST_PLAY;
                stateT_ = 0;
                timerRunning_ = true;
                audio::play(SFX_GO, 0.6f);
            }
            break;
        case ST_FINISH:
            if (stateT_ > 3.2f || (human && stateT_ > 1.0f && (p.down & WPAD_BUTTON_A))) {
                if (cur_ + 1 < cfg_.numPlayers) startPlayer(cur_ + 1);
                else enterResults();
            }
            break;
        case ST_RESULTS: {
            resultsT_ += dt;
            if (resultsT_ > 0.8f) {
                int r = resultsMenu_.update(dt, downAll);
                if (r == 0) scenes::go(makeAizkoScene(cfg_));
                else if (r == 1 || r == -2) scenes::go(makeMainMenu());
            }
            break;
        }
    }

    if (timerRunning_) timer_ += dt;
    fatigue_ = fmaxf(0.0f, fatigue_ - FATIGUE_RECOVER * dt);
    aizk_.fatigueLook = clampv((fatigue_ - 0.4f) / 0.6f, 0.0f, 1.0f);
    aizk_.update(dt);
    if (aizk_.impact() && state_ == ST_PLAY) handleImpact();
    log_.update(dt);
    chips_.update(dt);
    world.update(dt);
    for (int i = 0; i < numLabels_; i++) labels_[i].t += dt;
    while (numLabels_ > 0 && labels_[0].t > 1.1f) {
        memmove(labels_, labels_ + 1, sizeof(FloatLabel) * (numLabels_ - 1));
        numLabels_--;
    }
    shake_ = approach(shake_, 0, 10, dt);
    tipT_ += dt;
    if (tipT_ > 5.0f) {
        tipT_ = 0;
        tip_ = (tip_ + 1) % 4;
    }

    // Cámara: tres cuartos por delante del aizkolari, gira con él al darse la vuelta
    float camYaw = aizk_.yaw() + 1.0f;
    float dist = 4.0f, height = 1.2f;
    Vec3 target = log_.center() + Vec3(0, 0.9f, 0);
    if (state_ == ST_INTRO) {
        float k = smooth01(stateT_ / 2.2f);
        camYaw += (1 - k) * 1.3f;
        dist += (1 - k) * 3.5f;
        height += (1 - k) * 1.6f;
    } else if (state_ == ST_SPLIT) {
        float k = smooth01(stateT_ / 1.0f);
        dist -= 0.6f * k;
        camYaw += 0.3f * k;
        target = target - Vec3(0, 0.25f * k, 0);
    } else if (state_ == ST_FINISH || state_ == ST_RESULTS) {
        camYaw += time_ * 0.15f;
        dist += 0.8f;
    }
    Vec3 eye = target + Vec3(sinf(camYaw) * dist, height, cosf(camYaw) * dist);
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
    world.draw();
    world.drawShadow(log_.center(), tuning::LOG_LENGTH * 0.55f, LOG_RADIUS * 1.6f);
    log_.draw();
    aizk_.draw();
    chips_.draw();

    gfx::begin2D();
    drawHud();
    if (state_ == ST_RESULTS) drawResults();
    if (paused_) drawPause();
}

void AizkoScene::drawHud() {
    const float W = gfx::screenW();
    const float R = LOG_RADIUS;
    char buf[64];

    // Etiquetas flotantes ("Bikain!")
    for (int i = 0; i < numLabels_; i++) {
        const FloatLabel& l = labels_[i];
        float sx, sy;
        if (!gfx::project(l.pos, sx, sy)) continue;
        float pop = easeOutBack(clampv(l.t / 0.2f, 0.0f, 1.0f));
        float a = 1.0f - smooth01((l.t - 0.7f) / 0.4f);
        font::draw(l.text, sx, sy - 30 - l.t * 40, 30 * pop + 2, l.color, rgba(0x30, 0x30, 0x38), ALIGN_CENTER, a);
    }

    if (state_ == ST_RESULTS) return;
    const Profile& pr = profile(cur_);

    // Jugador y número de tronco
    ui::panel(34, 26, 250, 74);
    ui::colorDot(62, 52, 11, FAV_COLORS[pr.color % NUM_COLORS]);
    font::plain(pr.name, 82, 34, 28, UI_TEXT);
    if (cfg_.bot[cur_]) font::plain(txt::CPU, 268, 38, 20, UI_TEXT_LIGHT, ALIGN_RIGHT);
    snprintf(buf, sizeof(buf), txt::LOG_N_OF, clampv(logIndex_ + 1, 1, cfg_.numLogs), cfg_.numLogs);
    font::plain(buf, 50, 68, 22, UI_TEXT_LIGHT);

    // Tiempo
    ui::formatTime((unsigned)(timer_ * 1000), buf, sizeof(buf));
    font::plain(txt::TIME, W - 40, 24, 20, rgba(0x30, 0x40, 0x50), ALIGN_RIGHT);
    font::draw(buf, W - 40, 44, 46, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_RIGHT);

    // Esquema del tronco visto desde arriba con las dos V
    {
        float w = 150, h = 48, cx = W - 40 - w / 2, top = 112;
        gfx::roundRect(cx - w / 2 - 6, top - 6, w + 12, h + 12, 12, rgba(255, 255, 255, 190));
        gfx::rect(cx - w / 2, top, w, h, BARK);
        gfx::rect(cx - w / 2, top + 4, w, h - 8, WOOD_MID);
        Color hole = rgba(0xEA, 0xF4, 0xFA);
        float df = log_.depth[1] / (2 * R) * h, db = log_.depth[0] / (2 * R) * h;  // arriba = detrás
        if (df > 0.5f) gfx::triangle(cx - df, top, cx + df, top, cx, top + df, hole);
        if (db > 0.5f) gfx::triangle(cx - db, top + h, cx + db, top + h, cx, top + h - db, hole);
        // Flecha del lado que se corta ahora
        float ay = aizk_.side() == 0 ? top + h + 12 : top - 12;
        float dir = aizk_.side() == 0 ? 1.0f : -1.0f;
        gfx::triangle(cx - 9, ay, cx + 9, ay, cx, ay - dir * 9, UI_RED);
    }

    // Cansancio
    {
        float x = 44, y = 396, w = 170, h = 18;
        font::plain(txt::FATIGUE, x, y - 28, 22, UI_WHITE);
        gfx::roundRect(x - 3, y - 3, w + 6, h + 6, 11, rgba(0, 0, 0, 90));
        gfx::roundRect(x, y, w, h, 9, rgba(255, 255, 255, 140));
        float f = clampv(fatigue_, 0.0f, 1.0f);
        Color fc = f > FATIGUE_HEAVY ? UI_RED : mixColor(UI_GREEN, UI_GOLD, f / FATIGUE_HEAVY);
        if (f > 0.02f) gfx::roundRect(x, y, w * f, h, 9, fc);
    }

    // Indicador de inclinación del hacha y siguiente lado
    if (!cfg_.bot[cur_] && (state_ == ST_PLAY || state_ == ST_COUNTDOWN)) {
        float cx = W / 2, cy = 430, r = 62;
        int last = lastSide_[aizk_.side()];
        int want = last == HIT_LEFT ? HIT_RIGHT : last == HIT_RIGHT ? HIT_LEFT : HIT_NONE;
        float pulse = 0.6f + 0.4f * sinf(time_ * 8);
        float a0 = PI, a1 = 2 * PI;
        float st = TILT_STRAIGHT;
        auto ang = [&](float t) { return 1.5f * PI + t * 0.5f * PI; };
        gfx::arc(cx, cy, r - 18, r + 4, a0, a1, rgba(0, 0, 0, 80), 24);
        Color cl = want == HIT_LEFT ? mixColor(UI_BLUE, UI_WHITE, 1 - pulse) : rgba(0x9C, 0xC8, 0xEC);
        Color cr = want == HIT_RIGHT ? mixColor(UI_BLUE, UI_WHITE, 1 - pulse) : rgba(0x9C, 0xC8, 0xEC);
        gfx::arc(cx, cy, r - 14, r, ang(-1), ang(-st), cl, 12);
        gfx::arc(cx, cy, r - 14, r, ang(-st), ang(st), rgba(0xD8, 0xD8, 0xD8), 6);
        gfx::arc(cx, cy, r - 14, r, ang(st), ang(1), cr, 12);
        float na = ang(clampv(input::pad(padFor(cur_)).tiltLive, -1.0f, 1.0f));
        gfx::triangle(cx + cosf(na) * (r + 6), cy + sinf(na) * (r + 6), cx + cosf(na + 1.57f) * 5, cy + sinf(na + 1.57f) * 5,
                      cx + cosf(na - 1.57f) * 5, cy + sinf(na - 1.57f) * 5, rgba(0x30, 0x30, 0x38));
        gfx::circle(cx, cy, 7, rgba(0x30, 0x30, 0x38), 12);
        font::draw(txt::LEFT, cx - r - 12, cy - 30, 22, want == HIT_LEFT ? UI_WHITE : rgba(0xE0, 0xE0, 0xE0), rgba(0x20, 0x30, 0x40), ALIGN_RIGHT);
        font::draw(txt::RIGHT, cx + r + 12, cy - 30, 22, want == HIT_RIGHT ? UI_WHITE : rgba(0xE0, 0xE0, 0xE0), rgba(0x20, 0x30, 0x40), ALIGN_LEFT);
    }

    // Consejos del entrenamiento
    if (cfg_.training && state_ == ST_PLAY) {
        const char* tips[4] = {txt::TIP_1, txt::TIP_2, txt::TIP_3, txt::TIP_4};
        float a = smooth01(tipT_ / 0.4f) * (1 - smooth01((tipT_ - 4.6f) / 0.4f));
        font::draw(tips[tip_], W / 2, 330, 22, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER, a);
    }

    // Aviso de darse la vuelta
    if (state_ == ST_PLAY && aizk_.side() == 0 && !aizk_.turning() && log_.depth[0] >= TURN_PROMPT * R) {
        float s = 1.0f + 0.06f * sinf(time_ * 9);
        font::draw(txt::TURN_AROUND, W / 2, 120, 50 * s, UI_GOLD, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
        if (!cfg_.bot[cur_]) {
            float tw = font::width(txt::PRESS_B, 26);
            font::draw(txt::PRESS_B, W / 2 - 14, 180, 26, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER);
            ui::buttonIcon(W / 2 + tw / 2 + 4, 196, "B", 15, rgba(0x80, 0x88, 0x94));
        }
    }

    // Mensajes grandes de estado
    if (state_ == ST_INTRO) {
        float a = smooth01(stateT_ / 0.4f) * (1 - smooth01((stateT_ - 2.0f) / 0.4f));
        float y = 150 + (1 - easeOutCubic(stateT_ / 0.5f)) * -40;
        if (cfg_.training) {
            font::draw(txt::TRAINING, W / 2, y, 56, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, a);
        } else {
            turnText(pr.name, buf, sizeof(buf));
            font::draw(buf, W / 2, y, 56, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, a);
        }
        if (cfg_.numLogs == 1) snprintf(buf, sizeof(buf), "%s", txt::LOG_1);
        else snprintf(buf, sizeof(buf), txt::LOGS_N, cfg_.numLogs);
        font::draw(buf, W / 2, y + 66, 30, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, a);
    } else if (state_ == ST_COUNTDOWN) {
        float s = easeOutBack(clampv(stateT_ / 0.3f, 0.0f, 1.0f));
        font::draw(txt::READY, W / 2, 160, 64 * s + 1, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER);
    } else if (state_ == ST_PLAY && stateT_ < 0.8f) {
        float s = easeOutBack(clampv(stateT_ / 0.25f, 0.0f, 1.0f));
        font::draw(txt::GO, W / 2, 150, 80 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER, 1 - smooth01((stateT_ - 0.5f) / 0.3f));
    } else if (state_ == ST_SPLIT) {
        float s = easeOutBack(clampv(stateT_ / 0.3f, 0.0f, 1.0f));
        font::draw(txt::LOG_CUT, W / 2, 130, 56 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER);
    } else if (state_ == ST_NEXTLOG) {
        float a = 1 - fabsf(stateT_ - 0.4f) / 0.4f;
        ui::fade(clampv(a, 0.0f, 1.0f), UI_WHITE);
    } else if (state_ == ST_FINISH) {
        float s = easeOutBack(clampv(stateT_ / 0.3f, 0.0f, 1.0f));
        font::draw(txt::FINISHED, W / 2, 110, 64 * s + 1, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER);
        ui::formatTime(res_[cur_].timeMs, buf, sizeof(buf));
        font::draw(buf, W / 2, 190, 72, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER, smooth01((stateT_ - 0.3f) / 0.3f));
    }
}

void AizkoScene::drawResults() {
    const float W = gfx::screenW();
    char buf[64];
    float a = smooth01(resultsT_ / 0.4f);
    ui::fade(0.35f * a);
    float pw = 560, ph = 70 + cfg_.numPlayers * 56 + 40;
    float px = W / 2 - pw / 2, py = 70 + (1 - easeOutCubic(resultsT_ / 0.5f)) * 60;
    ui::panel(px, py, pw, ph);
    font::plain(txt::RESULTS, W / 2, py + 14, 36, UI_TEXT, ALIGN_CENTER);
    for (int i = 0; i < cfg_.numPlayers; i++) {
        int pi = order_[i];
        const Profile& pr = profile(pi);
        float y = py + 70 + i * 56;
        if (i == 0) gfx::roundRect(px + 16, y - 4, pw - 32, 50, 16, rgba(0xFF, 0xF0, 0xB8));
        snprintf(buf, sizeof(buf), "%d.", i + 1);
        font::plain(buf, px + 34, y + 4, 30, i == 0 ? rgba(0xD8, 0x98, 0x10) : UI_TEXT_LIGHT);
        ui::colorDot(px + 92, y + 21, 10, FAV_COLORS[pr.color % NUM_COLORS]);
        font::plain(pr.name, px + 114, y + 4, 30, UI_TEXT);
        ui::formatTime(res_[pi].timeMs, buf, sizeof(buf));
        font::plain(buf, px + pw - 36, y + 4, 30, UI_TEXT, ALIGN_RIGHT);
        if (res_[pi].record) {
            float s = 1 + 0.05f * sinf(time_ * 8);
            font::draw(txt::NEW_RECORD, px + pw - 150, y + 8, 20 * s, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_RIGHT);
        }
    }
    if (cfg_.numPlayers > 1) {
        snprintf(buf, sizeof(buf), "%s: %s!", txt::WINNER, profile(order_[0]).name);
        font::draw(buf, W / 2, py + ph + 10, 40, UI_GOLD, rgba(0x80, 0x30, 0x10), ALIGN_CENTER, a);
    }
    float by = 400;
    ui::button(W / 2 - 150, by, 260, 64, txt::PLAY_AGAIN, resultsMenu_.anim[0]);
    ui::button(W / 2 + 150, by, 260, 64, txt::TO_MENU, resultsMenu_.anim[1]);
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
