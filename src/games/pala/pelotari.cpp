#include "games/pala/pelotari.h"
#include "engine/tuning.h"

static Mesh s_pala;

static const float BLADE_LEN = 0.36f;     // pala de madera: hoja de 36 cm…
static const float BLADE_W = 0.095f;      // …por 9,5 de ancho
static const float BLADE_Y = 0.09f;       // donde empieza la hoja (desde la mano)
static const float FOLLOW_TIME = 0.32f;   // del impacto al final del golpe
static const float THETA_BACK = -1.45f;   // pala atrás (preparada)
static const float THETA_CONTACT = 0.1f;  // a la altura de la pelota, un poco por delante
static const float THETA_END = 2.1f;      // final del golpe, cruzada por delante del cuerpo
static const float THETA_REST = 0.75f;    // en reposo, delante
static const float ARM = 0.42f;           // del hombro a la mano
static const float DROP_TIME = 0.35f;
// Piernas del cuerpo de Resort (m): muslo y espinilla, para saber cuánto baja la cadera al doblarlas
static const float THIGH = 2.13f * tuning::MII_BODY_UNIT, SHIN = 1.75f * tuning::MII_BODY_UNIT;

static void buildPala() {
    if (s_pala.valid()) return;
    MeshBuilder b;
    const Color wood = rgba(0xE4, 0xB8, 0x78), edge = rgba(0xB0, 0x7C, 0x44), grip = rgba(0xF4, 0xF2, 0xEC);
    const Color tape = rgba(0xD8, 0x30, 0x28);
    // Mango con cinta a lo largo de +Y desde la mano; hoja plana en el plano XY que golpea con la cara +Z
    b.cylinder(Vec3(0, -0.07f, 0), 0.018f, 0.018f, 0.16f, grip, grip, 10, true);
    b.cylinder(Vec3(0, -0.075f, 0), 0.021f, 0.021f, 0.02f, tape, tape, 10, true);
    b.box(Vec3(0, BLADE_Y + BLADE_LEN / 2, 0), Vec3(BLADE_W / 2, BLADE_LEN / 2, 0.012f), wood);
    // canto más oscuro y punta redondeada
    b.box(Vec3(BLADE_W / 2, BLADE_Y + BLADE_LEN / 2, 0), Vec3(0.004f, BLADE_LEN / 2, 0.0125f), edge);
    b.box(Vec3(-BLADE_W / 2, BLADE_Y + BLADE_LEN / 2, 0), Vec3(0.004f, BLADE_LEN / 2, 0.0125f), edge);
    b.xf = Xf().translate(Vec3(0, BLADE_Y + BLADE_LEN, 0)).rotateX(PI / 2);
    b.cylinder(Vec3(0, -0.012f, 0), BLADE_W / 2, BLADE_W / 2, 0.024f, edge, wood, 16, true);
    b.xf = Xf();
    b.box(Vec3(0, BLADE_Y + 0.015f, 0), Vec3(BLADE_W / 2 - 0.018f, 0.02f, 0.014f), edge);
    b.build(s_pala);
}

void Pelotari::setup(const Appearance& look) {
    buildPala();
    look_ = look;
}

void Pelotari::prepare(int side, float height, float amount) {
    if (swingT_ < 0) side_ = side;
    if (swingT_ < 0) height_ = height;
    prepTarget_ = amount;
}

void Pelotari::swing(int side, float height, float contactIn, float strength) {
    if (beforeContact()) {  // ya va hacia la pelota: solo se ajusta cuándo llega
        contactAt_ = swingT_ + clampv(contactIn, 0.02f, 0.3f);
        height_ = height;
        strength_ = fmaxf(strength_, strength);
        return;
    }
    if (swingT_ >= 0 && swingT_ < contactAt_ + FOLLOW_TIME * 0.5f) return;
    side_ = side;
    height_ = height;
    strength_ = strength;
    swingT_ = 0;
    contactAt_ = clampv(contactIn, 0.03f, 0.3f);
    thetaFrom_ = fminf(theta_, THETA_BACK * 0.6f);
}

void Pelotari::dropBall() { dropT_ = 0; }

Vec3 Pelotari::dropPoint() const { return pos + Vec3(0.55f, tuning::DROP_Y, -0.2f); }

void Pelotari::update(float dt) {
    time_ += dt;
    moodT_ += dt;
    Vec3 hv(vel.x, 0, vel.z);
    float speed = length(hv);
    run_ = approach(run_, clampv(speed / tuning::PLAYER_SPEED, 0.0f, 1.0f), 10, dt);
    runPhase_ += dt * (5.0f + 9.0f * run_);
    prep_ = approach(prep_, prepTarget_, 10, dt);
    plant_ = approach(plant_, planted_ ? 1.0f : 0.0f, 12, dt);
    // Mira al frontis, pero el cuerpo se gira un poco hacia donde corre de lado
    float wantYaw = PI - clampv(vel.x / tuning::PLAYER_SPEED, -1.0f, 1.0f) * 0.5f * (1 - plant_);
    if (swingT_ >= 0) wantYaw = PI;
    yaw_ = approach(yaw_, wantYaw, 8, dt);

    if (swingT_ >= 0) {
        swingT_ += dt;
        if (swingT_ < contactAt_) {
            float k = swingT_ / contactAt_;
            theta_ = lerpf(thetaFrom_, THETA_CONTACT, k * k);  // acelera hacia la pelota
        } else {
            float k = (swingT_ - contactAt_) / FOLLOW_TIME;
            theta_ = lerpf(THETA_CONTACT, THETA_END, easeOutCubic(k));
            if (k >= 1.0f) {
                swingT_ = -1;
                prep_ = 0;
            }
        }
    } else {
        theta_ = approach(theta_, lerpf(THETA_REST, THETA_BACK, prep_), 12, dt);
    }
    if (dropT_ >= 0) {
        dropT_ += dt;
        if (dropT_ > DROP_TIME) dropT_ = -1;
    }
    // Saltitos: celebrar, golpe fuerte (justo después del impacto) y el botecito de la espera
    float hopSwing = swingT_ > contactAt_ && strength_ > 0.7f ? sinf(PI * clampv((swingT_ - contactAt_) / 0.3f, 0.0f, 1.0f)) * 0.06f : 0;
    if (mood_ == MOOD_HAPPY) hop_ = fabsf(sinf(moodT_ * 7.0f)) * 0.28f;
    else hop_ = hopSwing + fabsf(sinf(runPhase_)) * 0.04f * run_;
    blinkT_ -= dt;
    if (blinkT_ < -0.12f) blinkT_ = frand(1.5f, 4.0f);
}

void Pelotari::computePose(CharacterPose& pose, Xf& palaXf) {
    bool happy = mood_ == MOOD_HAPPY, sad = mood_ == MOOD_SAD;
    float swingK = swingT_ >= 0 ? clampv(swingT_ / (contactAt_ + FOLLOW_TIME), 0.0f, 1.0f) : 0;
    // Giro del cuerpo: atrás al preparar, adelante en el golpe
    float twist = (swingT_ >= 0 ? clampv(theta_ * 0.8f, -1.1f, 1.2f) : -prep_ * 0.9f) * 0.35f * side_;

    // Piernas: carrera, espera flexionada, plantado (más agachado), paso adelante al golpear
    float idle = 1 - run_;
    float bounce = sinf(time_ * 5.0f) * 0.04f * idle * (1 - plant_);
    float crouch = 0.18f + 0.22f * plant_ + 0.12f * prep_ + bounce + (sad ? 0.1f : 0);
    float legL = crouch * 0.6f, legR = crouch * 0.6f, kneeL = crouch * 1.4f, kneeR = crouch * 1.4f;
    float s = sinf(runPhase_), c = cosf(runPhase_);
    legL += s * 0.75f * run_;
    legR -= s * 0.75f * run_;
    kneeL += fmaxf(0.0f, -c) * 1.1f * run_;
    kneeR += fmaxf(0.0f, c) * 1.1f * run_;
    if (swingT_ >= 0) {
        // Paso: la pierna del lado contrario a la pala va adelante en el golpe
        float step = sinf(PI * swingK) * 0.45f;
        if (side_ > 0) { legL += step; legR -= step * 0.4f; }
        else { legR += step; legL -= step * 0.4f; }
    }
    if (happy && hop_ > 0.05f) { kneeL += 0.9f; kneeR += 0.9f; legL += 0.5f; legR += 0.5f; }
    auto drop = [](float a, float k) { return THIGH * (1 - cosf(a)) + SHIN * (1 - cosf(k - a)); };
    float hipDrop = fminf(drop(legL, kneeL), drop(legR, kneeR));
    if (happy && hop_ > 0.05f) hipDrop = 0;  // en el aire, las piernas recogidas no bajan el cuerpo

    pose.pos = pos + Vec3(0, hop_ - hipDrop, 0);
    pose.yaw = yaw_ + twist;
    pose.lean = 0.1f + 0.22f * run_ + 0.1f * plant_ + (height_ < 0.6f ? 0.2f : 0) + (sad ? 0.25f : 0) - (happy ? 0.12f : 0);
    pose.side = -side_ * 0.07f * prep_;
    pose.headPitch = happy ? -0.3f : sad ? 0.45f : -0.08f;
    pose.blink = blinkT_ < 0 || (sad && moodT_ > 0.3f);
    pose.legL = legL;
    pose.legR = legR;
    pose.kneeL = kneeL;
    pose.kneeR = kneeR;

    Xf root = Xf().translate(pose.pos).rotateY(pose.yaw);
    // En el espacio del personaje: delante = +Z, su derecha = -X
    Vec3 fwd(0, 0, 1), sideDir(side_ > 0 ? -1.0f : 1.0f, 0, 0);
    float hy = clampv(height_ - pose.pos.y + pos.y, 0.3f, 2.0f);
    float handY = lerpf(0.72f, hy, swingT_ >= 0 ? 1.0f : prep_);
    Vec3 shoulder = sideDir * 0.16f + Vec3(0, tuning::MII_BODY_UNIT * 9.25f, 0);
    Vec3 radial = sideDir * cosf(theta_) + fwd * sinf(theta_);
    Vec3 tangent = normalize(sideDir * -sinf(theta_) + fwd * cosf(theta_));
    Vec3 hand = shoulder + radial * ARM;
    hand.y = handY;
    // La hoja sale hacia fuera y hacia arriba; la cara mira hacia donde va el golpe
    Vec3 axis = normalize(radial * 0.8f + Vec3(0, 0.55f, 0));
    if (happy) {
        hand = Vec3(-0.22f, 1.35f + 0.08f * sinf(moodT_ * 7), 0.12f);
        axis = normalize(Vec3(-0.2f, 1, 0.1f));
        tangent = Vec3(0, 0, 1);
    } else if (sad) {
        hand = Vec3(-0.25f, 0.45f, 0.1f);
        axis = normalize(Vec3(-0.1f, -1, 0.3f));
        tangent = Vec3(0, 0, 1);
    }
    Vec3 normal = normalize(tangent - axis * dot(tangent, axis));
    palaXf = root * Xf::fromBasis(hand, cross(axis, normal), axis, normal);
    pose.handR = root.apply(hand);
    tip_ = palaXf.apply(Vec3(0, BLADE_Y + BLADE_LEN, 0));
    mid_ = palaXf.apply(Vec3(0, BLADE_Y + BLADE_LEN * 0.55f, 0));

    // Mano izquierda: suelta la pelota en el saque; corriendo, braceo; si no, por delante del cuerpo
    Vec3 handL(0.24f, 0.78f + 0.06f * s * run_, 0.16f + 0.12f * s * run_);
    if (swingT_ >= 0) handL = Vec3(0.3f, 0.85f, 0.25f - 0.2f * swingK);  // equilibrio
    if (dropT_ >= 0) handL = Xf().rotateY(-pose.yaw).apply(dropPoint() - pose.pos) + Vec3(0, 0.05f * (1 - dropT_ / DROP_TIME), 0);
    if (happy) handL = Vec3(0.26f, 1.3f + 0.08f * sinf(moodT_ * 7 + 1), 0.1f);
    if (sad) handL = Vec3(0.22f, 0.5f, 0.1f);
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
