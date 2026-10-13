#include "games/aizkolaritza/aizkolari.h"
#include "engine/audio.h"
#include "games/aizkolaritza/log.h"
#include "game/palette.h"

static Mesh s_axe;

static const float PHI_RAISED = 3.55f;   // hacha por encima de la cabeza, un poco hacia atrás
static const float PHI_REST = 0.55f;     // apoyada delante
static const float PHI_LOW = 1.25f;      // sujeta delante, lista para subirla
static const float HEAD_DIST = 0.66f;    // de la empuñadura a la cabeza del hacha
static const float SWING_TIME = 0.13f;
static const float STUCK_TIME = 0.09f;
static const float RECOVER_TIME = 0.34f;
static const float TURN_TIME = 0.75f;

static void buildAxe() {
    if (s_axe.valid()) return;
    MeshBuilder b;
    const Color handle = rgba(0xD0, 0xA0, 0x64), metal = rgba(0x8C, 0x94, 0x9E), edge = rgba(0xE6, 0xEA, 0xF0);
    b.cylinder(Vec3(0, -0.06f, 0), 0.017f, 0.015f, 0.8f, handle, handle, 8, true);
    b.box(Vec3(0, HEAD_DIST, 0.06f), Vec3(0.013f, 0.045f, 0.075f), metal);
    b.box(Vec3(0, HEAD_DIST, 0.14f), Vec3(0.008f, 0.065f, 0.012f), edge);
    b.box(Vec3(0, HEAD_DIST, -0.03f), Vec3(0.022f, 0.035f, 0.03f), rgba(0x5A, 0x60, 0x6A));
    b.build(s_axe);
}

void Aizkolari::setup(const Appearance& look, const Log* log) {
    buildAxe();
    model_ = characterCache(look);
    log_ = log;
    reset();
}

void Aizkolari::reset() {
    phase_ = IDLE;
    t_ = 0;
    side_ = 0;
    yaw_ = 0;
    phi_ = PHI_REST;
    tilt_ = 0;
    hop_ = 0;
    queued_ = false;
    impact_ = false;
    feet_ = Vec3(0, log_ ? log_->topY() - 0.02f : 0, 0);
}

void Aizkolari::setReady() {
    if (phase_ == IDLE || phase_ == CELEBRATE) {
        phase_ = RECOVER;
        phiFrom_ = phi_;
        t_ = 0;
    }
}

void Aizkolari::setIdle() {
    phase_ = IDLE;
    t_ = 0;
}

bool Aizkolari::chop(float strength, float tilt) {
    bool canNow = phase_ == READY || (phase_ == RECOVER && t_ > RECOVER_TIME * 0.45f);
    if (!canNow) {
        if (phase_ == SWING || phase_ == STUCK || phase_ == RECOVER) {
            queued_ = true;
            qStrength_ = strength;
            qTilt_ = tilt;
        }
        return false;
    }
    phase_ = SWING;
    t_ = 0;
    phiFrom_ = phi_;
    strength_ = strength;
    chopTilt_ = tilt;
    audio::play(SFX_WHOOSH, 0.35f + 0.3f * strength, frand(0.9f, 1.1f));
    return true;
}

void Aizkolari::turnAround() {
    if (phase_ == TURN) return;
    phase_ = TURN;
    t_ = 0;
    yawFrom_ = yaw_;
    queued_ = false;
    audio::play(SFX_WHOOSH, 0.5f, 0.7f);
}

void Aizkolari::celebrate() {
    phase_ = CELEBRATE;
    t_ = 0;
    queued_ = false;
}

float Aizkolari::readyPhi() const { return lerpf(PHI_LOW, PHI_RAISED, clampv(raise_, 0.0f, 1.0f)); }

float Aizkolari::impactAngle() {
    // Ángulo con el que la cabeza del hacha llega al punto de impacto
    if (!log_) return 0.15f;
    Vec3 target = log_->impactPoint(side_, chopTilt_);
    Vec3 local = Xf().rotateY(-yaw_).apply(target - feet_);
    Vec3 shoulder(0, 0.42f + 0.40f * cosf(0.45f), 0.40f * sinf(0.45f) + 0.04f);
    Vec3 v = local - shoulder;
    return atan2f(v.z, -v.y);
}

void Aizkolari::update(float dt) {
    time_ += dt;
    impact_ = false;
    t_ += dt;
    switch (phase_) {
        case IDLE:
            phi_ = approach(phi_, PHI_REST, 6, dt);
            tilt_ = approach(tilt_, 0, 6, dt);
            break;
        case READY:
            phi_ = approach(phi_, readyPhi() + sinf(time_ * 2.2f) * 0.04f, 16, dt);
            tilt_ = approach(tilt_, liveTilt_, 12, dt);
            if (queued_) {
                queued_ = false;
                chop(qStrength_, qTilt_);
            }
            break;
        case SWING: {
            float k = clampv(t_ / SWING_TIME, 0.0f, 1.0f);
            phi_ = lerpf(phiFrom_, impactAngle(), k * k);
            tilt_ = approach(tilt_, chopTilt_, 25, dt);
            if (k >= 1.0f) {
                phase_ = STUCK;
                t_ = 0;
                impact_ = true;
                impStrength_ = strength_;
                impTilt_ = chopTilt_;
            }
            break;
        }
        case STUCK:
            if (t_ >= STUCK_TIME) {
                phase_ = RECOVER;
                t_ = 0;
                phiFrom_ = phi_;
            }
            break;
        case RECOVER: {
            float k = easeOutCubic(t_ / RECOVER_TIME);
            phi_ = lerpf(phiFrom_, readyPhi(), k);
            tilt_ = approach(tilt_, liveTilt_, 8, dt);
            if (queued_ && t_ > RECOVER_TIME * 0.45f) {
                queued_ = false;
                chop(qStrength_, qTilt_);
            } else if (t_ >= RECOVER_TIME) {
                phase_ = READY;
                t_ = 0;
            }
            break;
        }
        case TURN: {
            float k = clampv(t_ / TURN_TIME, 0.0f, 1.0f);
            yaw_ = yawFrom_ + PI * smooth01(k);
            hop_ = sinf(PI * k) * 0.28f;
            phi_ = approach(phi_, 2.2f, 10, dt);
            tilt_ = approach(tilt_, 0, 10, dt);
            if (k >= 1.0f) {
                side_ = 1 - side_;
                hop_ = 0;
                phase_ = READY;
                t_ = 0;
            }
            break;
        }
        case CELEBRATE:
            hop_ = fabsf(sinf(t_ * 6.0f)) * 0.22f;
            phi_ = approach(phi_, PI, 8, dt);
            tilt_ = approach(tilt_, sinf(t_ * 3) * 0.6f, 6, dt);
            break;
    }
    blinkT_ -= dt;
    if (blinkT_ < -0.12f) blinkT_ = frand(1.5f, 4.0f);
}

void Aizkolari::computePose(CharacterPose& pose, Xf& axeXf) {
    // Inclinación del cuerpo según la altura del hacha
    float raise = clampv((phi_ - 0.2f) / (PHI_RAISED - 0.2f), 0.0f, 1.0f);
    float lean = lerpf(0.45f, -0.1f, smooth01(raise));
    if (phase_ == CELEBRATE) lean = -0.15f;
    lean += fatigueLook * 0.08f * (0.5f + 0.5f * sinf(time_ * 3.0f));

    pose.pos = feet_ + Vec3(0, hop_, 0);
    pose.yaw = yaw_;
    pose.lean = lean;
    pose.side = -tilt_ * 0.12f;
    pose.headPitch = phase_ == CELEBRATE ? -0.25f : lerpf(0.35f, -0.1f, raise);
    pose.squash = phase_ == TURN ? -sinf(PI * clampv(t_ / TURN_TIME, 0.0f, 1.0f)) * 0.12f : 0;
    pose.blink = blinkT_ < 0;

    Xf root = Xf().translate(pose.pos).rotateY(yaw_);
    Vec3 shoulder(0, 0.42f + 0.40f * cosf(lean), 0.40f * sinf(lean) + 0.04f);

    float tau = tilt_ * 0.38f;
    Vec3 d(cosf(phi_) * sinf(tau) * -1.0f, -cosf(phi_) * cosf(tau), sinf(phi_));
    d = normalize(d);
    Vec3 tangent = normalize(Vec3(-sinf(phi_) * sinf(tau) * -1.0f, sinf(phi_) * cosf(tau), cosf(phi_)));
    Vec3 side = normalize(cross(d, tangent));

    float arm = 0.34f;
    if (log_ && (phase_ == SWING || phase_ == STUCK)) {
        Vec3 target = Xf().rotateY(-yaw_).apply(log_->impactPoint(side_, chopTilt_) - pose.pos);
        float need = length(target - shoulder) - HEAD_DIST;
        float k = phase_ == STUCK ? 1.0f : clampv(t_ / SWING_TIME, 0.0f, 1.0f);
        arm = lerpf(arm, clampv(need, 0.18f, 0.5f), k);
    }
    Vec3 grip = shoulder + d * arm;
    Vec3 grip2 = grip + d * lerpf(0.05f, 0.22f, raise);
    pose.handR = root.apply(grip);
    pose.handL = root.apply(grip2);

    axeXf = root * Xf::fromBasis(grip, side, d, tangent);
    axeHead_ = axeXf.apply(Vec3(0, HEAD_DIST, 0.1f));
}

void Aizkolari::draw() {
    if (!model_) return;
    CharacterPose pose;
    Xf axeXf;
    computePose(pose, axeXf);
    drawCharacter(*model_, pose);
    gfx::drawMesh(s_axe, axeXf);
}
