#include "games/pala/pelotari.h"
#include "engine/tuning.h"

static Mesh s_pala;

static const float BLADE_LEN = 0.36f;     // pala de madera: hoja de 36 cm…
static const float BLADE_W = 0.095f;      // …por 9,5 de ancho
static const float SWING_TIME = 0.42f;    // golpe completo
static const float CONTACT_AT = 0.07f;    // el latigazo llega en el pico: la pala está ya casi en la pelota
static const float THETA_BACK = -1.35f;   // pala atrás (preparada)
static const float THETA_CONTACT = 0.1f;  // a la altura de la pelota, un poco por delante
static const float THETA_END = 1.9f;      // final del golpe, por delante del cuerpo
static const float THETA_REST = 0.9f;     // en reposo, delante
static const float ARM = 0.42f;           // del hombro a la mano
static const float DROP_TIME = 0.35f;

static void buildPala() {
    if (s_pala.valid()) return;
    MeshBuilder b;
    const Color wood = rgba(0xDC, 0xB0, 0x70), dark = rgba(0x9A, 0x6A, 0x3C), grip = rgba(0xF0, 0xF0, 0xE8);
    // Mango (con cinta blanca) a lo largo de +Y desde la mano; hoja plana en el plano XY, golpea con la cara +Z
    b.cylinder(Vec3(0, -0.07f, 0), 0.017f, 0.017f, 0.16f, grip, grip, 8, true);
    b.box(Vec3(0, 0.09f + BLADE_LEN / 2, 0), Vec3(BLADE_W / 2, BLADE_LEN / 2, 0.012f), wood);
    b.box(Vec3(0, 0.09f + BLADE_LEN, 0), Vec3(BLADE_W / 2 - 0.012f, 0.012f, 0.0115f), wood);  // punta redondeada
    b.box(Vec3(0, 0.1f, 0), Vec3(BLADE_W / 2 - 0.02f, 0.02f, 0.014f), dark);
    b.build(s_pala);
}

void Pelotari::setup(const Appearance& look) {
    buildPala();
    look_ = look;
}

void Pelotari::prepare(int side, float height, float amount) {
    if (swingT_ < 0) side_ = side;
    height_ = height;
    prepTarget_ = amount;
}

void Pelotari::swing(int side, float height) {
    if (swingT_ >= 0 && swingT_ < SWING_TIME * 0.6f) return;
    side_ = side;
    height_ = height;
    swingT_ = 0;
}

void Pelotari::dropBall() { dropT_ = 0; }

Vec3 Pelotari::dropPoint() const { return pos + Vec3(0.5f, tuning::DROP_Y, -0.15f); }

void Pelotari::update(float dt) {
    time_ += dt;
    float speed = length(Vec3(vel.x, 0, vel.z));
    run_ = approach(run_, clampv(speed / tuning::PLAYER_SPEED, 0.0f, 1.0f), 10, dt);
    runPhase_ += dt * (4.0f + 9.0f * run_);
    prep_ = approach(prep_, prepTarget_, 9, dt);
    if (swingT_ >= 0) {
        swingT_ += dt;
        if (swingT_ < CONTACT_AT) theta_ = lerpf(theta_, THETA_CONTACT, clampv(dt / (CONTACT_AT - swingT_ + dt), 0.0f, 1.0f));
        else theta_ = lerpf(THETA_CONTACT, THETA_END, easeOutCubic((swingT_ - CONTACT_AT) / (SWING_TIME - CONTACT_AT)));
        if (swingT_ >= SWING_TIME) {
            swingT_ = -1;
            prep_ = 0;
        }
    } else {
        theta_ = approach(theta_, lerpf(THETA_REST, THETA_BACK, prep_), 12, dt);
    }
    if (dropT_ >= 0) {
        dropT_ += dt;
        if (dropT_ > DROP_TIME) dropT_ = -1;
    }
    hop_ = celebrate_ ? fabsf(sinf(time_ * 6.0f)) * 0.2f : fabsf(sinf(runPhase_)) * 0.05f * run_;
    blinkT_ -= dt;
    if (blinkT_ < -0.12f) blinkT_ = frand(1.5f, 4.0f);
}

void Pelotari::computePose(CharacterPose& pose, Xf& palaXf) {
    const float yaw = PI;  // mirando al frontis
    float twist = (swingT_ >= 0 ? clampv(theta_, -1.0f, 1.2f) : -prep_) * 0.22f * side_;
    pose.pos = pos + Vec3(0, hop_, 0);
    pose.yaw = yaw + twist;
    pose.lean = 0.12f + 0.15f * run_ + (height_ < 0.6f ? 0.15f : 0);
    pose.side = -side_ * 0.06f * prep_;
    pose.headPitch = celebrate_ ? -0.25f : -0.05f;
    pose.blink = blinkT_ < 0;

    Xf root = Xf().translate(pose.pos).rotateY(yaw);
    // En el espacio del personaje: delante = +Z, su derecha = -X
    Vec3 fwd(0, 0, 1), sideDir(side_ > 0 ? -1.0f : 1.0f, 0, 0);
    float hy = clampv(height_, 0.35f, 2.0f);
    float handY = celebrate_ ? 1.5f + 0.1f * sinf(time_ * 6) : lerpf(0.75f, hy, swingT_ >= 0 ? 1.0f : prep_);
    Vec3 shoulder = sideDir * 0.16f + Vec3(0, tuning::MII_BODY_UNIT * 9.25f, 0);
    Vec3 radial = sideDir * cosf(theta_) + fwd * sinf(theta_);
    Vec3 tangent = normalize(sideDir * -sinf(theta_) + fwd * cosf(theta_));
    Vec3 hand = shoulder + radial * ARM;
    hand.y = handY;
    if (celebrate_) hand = Vec3(-0.25f, handY, 0.1f);
    // La hoja sale hacia fuera y hacia arriba; la cara mira hacia donde va el golpe
    Vec3 axis = normalize(radial * 0.8f + Vec3(0, celebrate_ ? 2.0f : 0.55f, 0));
    Vec3 normal = normalize(tangent - axis * dot(tangent, axis));
    palaXf = root * Xf::fromBasis(hand, cross(axis, normal), axis, normal);
    pose.handR = root.apply(hand);

    // Mano izquierda: suelta la pelota en el saque; si no, por delante del cuerpo
    Vec3 handL(0.22f, 0.8f, 0.18f);
    if (dropT_ >= 0) handL = Xf().rotateY(-yaw).apply(dropPoint() - pos) + Vec3(0, 0.05f * (1 - dropT_ / DROP_TIME), 0);
    if (celebrate_) handL = Vec3(0.25f, 1.5f + 0.1f * sinf(time_ * 6 + 1), 0.1f);
    pose.handL = root.apply(handL);
}

void Pelotari::draw() {
    CharacterModel* model = characterCache(look_);  // en cada frame (ver characterCache)
    if (!model) return;
    CharacterPose pose;
    Xf palaXf;
    computePose(pose, palaXf);
    drawCharacter(*model, pose);
    gfx::drawMesh(s_pala, palaXf);
}
