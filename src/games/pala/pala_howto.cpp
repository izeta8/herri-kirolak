#include "games/pala/pala_howto.h"
#include <wiiuse/wpad.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/remote3d.h"
#include "game/text.h"

namespace {

const int NUM_STEPS = 5;
const float PERIOD[NUM_STEPS] = {3.9f, 4.0f, 2.2f, 4.5f, 4.2f};
const Color GREEN_WALL = rgba(0x5A, 0xAE, 0x7C), GREEN_DARK = rgba(0x3E, 0x86, 0x5C), FLOOR = rgba(0xC8, 0xD6, 0xCE);
const Color BALL = rgba(0xF6, 0xF4, 0xEC), INK = rgba(0x20, 0x30, 0x40);
const float TARGETS[4][2] = {{-0.55f, 0.15f}, {0.5f, 0.45f}, {-0.2f, 0.85f}, {0.6f, -0.1f}};  // paso 1 (−1..1)

inline bool crossed(float prev, float now, float th) { return prev < th && now >= th; }

void ball2D(float x, float y, float r) {
    gfx::circle(x + 2, y + 3, r, rgba(0, 0, 0, 60), 16);
    gfx::circle(x, y, r, BALL, 16);
    gfx::circle(x - r * 0.3f, y - r * 0.3f, r * 0.35f, UI_WHITE, 10);
}

// Pelotari visto desde arriba: cabeza y pala
void pelotari2D(float x, float y, float r) {
    gfx::circle(x + 2, y + 3, r, rgba(0, 0, 0, 50), 20);
    gfx::circle(x, y, r, UI_TEAL, 20);
    gfx::circle(x, y, r * 0.55f, rgba(0x26, 0x26, 0x24), 16);  // txapela
    gfx::roundRect(x + r * 0.7f, y - r * 0.2f, r * 1.3f, r * 0.45f, r * 0.2f, rgba(0xE4, 0xB8, 0x78));
}

}  // namespace

void PalaHowTo::start() {
    total_ = 0;
    touched_ = false;
    setStep(0);
}

void PalaHowTo::setStep(int s) {
    step_ = clampv(s, 0, NUM_STEPS - 1);
    t_ = 0;
    loops_ = 0;
    prevU_ = 0;
    px_ = 0;
    py_ = 0.6f;
    target_ = 0;
    tx_ = TARGETS[0][0];
    ty_ = TARGETS[0][1];
    trailN_ = 0;
}

bool PalaHowTo::update(float dt, unsigned down) {
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
    if (!touched_ && loops_ >= 2 && step_ < NUM_STEPS - 1) {
        setStep(step_ + 1);
        audio::play(SFX_MOVE, 0.4f);
        return false;
    }
    float u = fmodf(t_, per) / per, up = u < prevU_ ? 0 : prevU_;
    // Sonidos sincronizados con la animación
    if (step_ == 2 && crossed(up, u, 0.42f)) audio::play(SFX_PALA, 0.7f, 1.05f);
    if (step_ == 2 && crossed(up, u, 0.34f)) audio::play(SFX_WHOOSH, 0.45f, 1.2f);
    if (step_ == 3)
        for (int k = 0; k < 3; k++) {
            if (crossed(up, u, k / 3.0f + 0.12f)) audio::play(SFX_PALA, 0.5f);
            if (crossed(up, u, k / 3.0f + 0.17f)) audio::play(SFX_FRONTIS, 0.45f);
        }
    if (step_ == 4)
        for (int k = 0; k < 3; k++)
            if (crossed(up, u, k / 3.0f + 0.2f + (k - 1) * 0.04f)) audio::play(k == 1 ? SFX_PALA : SFX_WHOOSH, 0.55f);
    prevU_ = u;

    // Paso 1: el pelotari va hacia cada pelota y se coloca a su lado
    if (step_ == 0) {
        int k = (int)(t_ / 1.3f) % 4;
        if (k != target_) {
            target_ = k;
            tx_ = TARGETS[k][0];
            ty_ = TARGETS[k][1];
        }
        float gx = tx_ - 0.22f, gy = ty_;  // de derecha: la pelota a su derecha
        float dx = gx - px_, dy = gy - py_, l = sqrtf(dx * dx + dy * dy);
        float v = fminf(l, 1.6f * dt);
        if (l > 0.001f) {
            px_ += dx / l * v;
            py_ += dy / l * v;
        }
    }
    return false;
}

// Nunchuk de perfil (cuerpo, stick y botones C/Z). (sx, sy) = hacia dónde está empujado el stick (−1..1)
void PalaHowTo::drawNunchuk(float cx, float cy, float sx, float sy, bool zLit) {
    const Color body = rgba(0xF6, 0xF6, 0xF4), shadow = rgba(0xD0, 0xD4, 0xDA);
    gfx::roundRect(cx - 34 + 4, cy - 50 + 6, 68, 120, 34, rgba(0, 0, 0, 40));
    gfx::roundRect(cx - 34, cy - 50, 68, 120, 34, body);
    gfx::roundRect(cx - 30, cy + 30, 60, 36, 18, shadow);
    // stick
    gfx::circle(cx, cy - 18, 22, shadow, 24);
    gfx::circle(cx + sx * 9, cy - 18 - sy * 9, 15, rgba(0x6A, 0x6E, 0x76), 20);
    gfx::circle(cx + sx * 9 - 4, cy - 22 - sy * 9, 5, rgba(0x9A, 0x9E, 0xA6), 12);
    // C y Z en la punta (Z grande, abajo)
    gfx::roundRect(cx - 14, cy - 66, 28, 12, 6, shadow);
    float pulse = 0.5f + 0.5f * sinf(total_ * 8);
    if (zLit) gfx::roundRect(cx - 22, cy - 86, 44, 22, 11, rgba(0x3C, 0xCC, 0xF4, (int)(150 + 100 * pulse)));
    gfx::roundRect(cx - 18, cy - 82, 36, 14, 7, zLit ? UI_WHITE : shadow);
    font::plain("Z", cx, cy - 86, 16, zLit ? UI_TEAL : UI_TEXT_LIGHT, ALIGN_CENTER);
    // el arco del stick, para que se vea que se empuja
    if (fabsf(sx) + fabsf(sy) > 0.1f) {
        float a = atan2f(-sy, sx);
        gfx::arc(cx, cy - 18, 26, 31, a - 0.5f, a + 0.5f, rgba(0x3C, 0xCC, 0xF4, 220), 10);
    }
}

// Paso 1: cancha desde arriba (frontis arriba, pared izquierda a la izquierda) y el Nunchuk que la mueve
void PalaHowTo::drawMove(float cx, float cy) {
    const float w = 170, h = 210, x0 = cx + 10, y0 = cy - h / 2 - 8;
    gfx::roundRect(x0 - 4, y0 - 4, w + 8, h + 8, 10, rgba(255, 255, 255, 160));
    gfx::rect(x0, y0, w, h, FLOOR);
    gfx::rect(x0, y0, w, 10, GREEN_WALL);        // frontis
    gfx::rect(x0, y0, 10, h, GREEN_DARK);        // pared izquierda
    gfx::rect(x0 + w - 2, y0, 2, h, UI_WHITE);   // lado abierto
    gfx::rect(x0, y0 + h * 0.42f, w, 2, UI_RED); // falta
    gfx::rect(x0, y0 + h * 0.72f, w, 2, UI_RED); // pasa
    auto mapX = [&](float x) { return x0 + 14 + (x + 1) * 0.5f * (w - 22); };
    auto mapY = [&](float y) { return y0 + 18 + (y + 1) * 0.5f * (h - 26); };
    float bx = mapX(tx_), by = mapY(ty_);
    // marca de bote y pelota
    gfx::ring(bx, by, 9, 12, rgba(0xFF, 0xFF, 0xFF, 220), 20);
    ball2D(bx, by, 6);
    float qx = mapX(px_), qy = mapY(py_);
    pelotari2D(qx, qy, 11);
    float dx = (tx_ - 0.22f) - px_, dy = ty_ - py_;
    float l = sqrtf(dx * dx + dy * dy);
    if (l < 0.04f) font::draw(txt::GOOD, bx, by - 34, 20, UI_GREEN, UI_WHITE, ALIGN_CENTER);
    float sx = l > 0.02f ? dx / l : 0, sy = l > 0.02f ? -dy / l : 0;
    drawNunchuk(cx - 120, cy + 10, sx, sy, false);
}

// Paso 2: el frontis de frente y la pared izquierda en perspectiva; la mira va de un lado al otro y llega a la pared
void PalaHowTo::drawAim(float cx, float cy) {
    const float fx = cx - 20, fy = cy - 95, fw = 190, fh = 150;
    // pared izquierda (trapecio hacia el espectador)
    gfx::triangle(fx, fy, fx - 70, fy - 40, fx - 70, fy + fh + 50, GREEN_DARK);
    gfx::triangle(fx, fy, fx - 70, fy + fh + 50, fx, fy + fh, GREEN_DARK);
    gfx::rect(fx, fy, fw, fh, GREEN_WALL);
    gfx::rect(fx, fy + fh - 18, fw, 18, rgba(0xC8, 0xD0, 0xD8));  // chapa
    gfx::rect(fx, fy + fh - 20, fw, 3, UI_WHITE);
    gfx::rect(fx, fy + fh, fw, 50, FLOOR);
    gfx::triangle(fx, fy + fh, fx - 70, fy + fh + 50, fx, fy + fh + 50, FLOOR);
    float u = fmodf(t_, PERIOD[1]) / PERIOD[1];
    // u: 1 (lado abierto) → −2 (pared) y vuelta
    float a = 1.0f - 3.0f * (0.5f - 0.5f * cosf(2 * PI * u));
    float rx, ry = fy + fh * 0.45f;
    bool wall = a < -1.0f;
    if (!wall) rx = fx + 14 + (a + 1) * 0.5f * (fw - 28);
    else {
        float k = clampv(-1.0f - a, 0.0f, 1.0f);  // por la pared, hacia el espectador
        rx = fx - k * 60;
        ry = fy + fh * 0.45f + k * 12;
    }
    float pulse = 1.0f + 0.12f * sinf(total_ * 10);
    gfx::ring(rx, ry, 14 * pulse, 18 * pulse, INK, 24);
    gfx::ring(rx, ry, 12 * pulse, 16 * pulse, UI_TEAL_GLOW, 24);
    gfx::rect(rx - 1, ry - 24, 2, 9, UI_TEAL_GLOW);
    gfx::rect(rx - 1, ry + 15, 2, 9, UI_TEAL_GLOW);
    if (wall) {
        // la pelota da en la pared, luego en el frontis y sale hacia el ancho
        float k = clampv(-1.0f - a, 0.0f, 1.0f);
        float hx = fx + 40 + k * 60, hy = fy + fh * 0.5f;
        gfx::triangle(rx, ry - 2, hx, hy - 2, hx, hy + 2, rgba(255, 255, 255, 180));
        gfx::triangle(hx, hy - 2, fx + fw + 30, fy + fh + 40, fx + fw + 26, fy + fh + 44, rgba(255, 255, 255, 140));
        font::draw(txt::BIPARETA, fx + fw * 0.6f, fy - 34, 26, UI_GOLD, rgba(0x80, 0x48, 0x00), ALIGN_CENTER);
    }
    float stick = a >= 0 ? a : a / 2.0f;
    drawNunchuk(cx - (gfx::widescreen() ? 150 : 138), cy + 20, stick, 0, true);
}

// Paso 3: el Wii Remote en 3D hace el latigazo; se llena la fuerza y sale la pelota
void PalaHowTo::drawWhip(float cx, float cy) {
    float u = fmodf(t_, PERIOD[2]) / PERIOD[2];
    // pitch: atrás (−) → delante (+), con un giro rápido entre 0.34 y 0.42
    float pitch, yaw = 0.25f;
    if (u < 0.2f) pitch = lerpf(0.2f, -0.7f, smooth01(u / 0.2f));
    else if (u < 0.34f) pitch = -0.7f;
    else if (u < 0.42f) { float k = (u - 0.34f) / 0.08f; pitch = lerpf(-0.7f, 0.9f, k * k); yaw = lerpf(0.25f, -0.25f, k); }
    else if (u < 0.6f) { pitch = lerpf(0.9f, 1.25f, smooth01((u - 0.42f) / 0.18f)); yaw = -0.25f; }
    else { pitch = lerpf(1.25f, 0.2f, smooth01((u - 0.6f) / 0.4f)); yaw = lerpf(-0.25f, 0.25f, smooth01((u - 0.6f) / 0.4f)); }
    bool fast = u >= 0.34f && u < 0.46f;
    // fuerza: se llena en el golpe
    float g = u < 0.42f ? 0 : u < 0.8f ? 0.85f : 0.85f * (1 - smooth01((u - 0.8f) / 0.2f));
    float gx = cx + 120, gy = cy - 100, gh = 190;
    gfx::roundRect(gx - 9, gy - 3, 18, gh + 6, 9, rgba(255, 255, 255, 200));
    if (g > 0.02f) gfx::roundRect(gx - 6, gy + gh * (1 - g), 12, gh * g, 6, mixColor(UI_GOLD, UI_RED, g));
    font::plain(txt::STRENGTH, gx, gy + gh + 8, 16, UI_TEXT_LIGHT, ALIGN_CENTER);
    // pelota que sale despedida
    if (u >= 0.42f && u < 0.75f) {
        float k = (u - 0.42f) / 0.33f;
        ball2D(cx + 10 + k * 160, cy - 60 - k * 90 + k * k * 40, 7 * (1 - 0.4f * k));
        if (u < 0.6f) font::draw(txt::GREAT, cx - 60, cy - 130 - k * 20, 30, UI_GOLD, rgba(0x80, 0x48, 0x00), ALIGN_CENTER);
    }
    // estela de la punta
    for (int i = 1; i < trailN_; i++) {
        float a0 = (float)(i - 1) / trailN_, a1 = (float)i / trailN_;
        gfx::triangle(trailX_[i - 1], trailY_[i - 1] - 6 * a0, trailX_[i], trailY_[i] - 8 * a1, trailX_[i], trailY_[i] + 8 * a1,
                      rgba(0xFF, 0xFF, 0xFF, (int)(170 * a1)));
    }
    // mando 3D
    const float W = gfx::screenW();
    gfx::clearDepth();
    Camera cam;
    const float dist = 0.62f, fov = 34;
    float halfH = dist * tanf(fov * 0.5f * DEG), halfW = halfH * (W / 480.0f);
    float sx = cx - 40, sy = cy + 10;
    float ox = (0.5f - sx / W) * 2 * halfW, oy = (sy / 480.0f - 0.5f) * 2 * halfH;
    cam.eye = Vec3(ox, oy, dist);
    cam.target = Vec3(ox, oy, 0);
    cam.fov = fov;
    cam.znear = 0.05f;
    gfx::begin3D(cam);
    // agarre por abajo: pivota en la mano (abajo del mando)
    Xf xf = Xf().translate(Vec3(0, -0.06f, 0)).rotateY(yaw).rotateX(-pitch).translate(Vec3(0, remote3d::RL * 0.42f, 0));
    gfx::drawMesh(remote3d::mesh(), xf);
    float tipX, tipY;
    gfx::project(xf.apply(Vec3(0, remote3d::RL / 2, 0)), tipX, tipY);
    gfx::begin2D();
    if (fast) {
        if (trailN_ == 12) {
            for (int i = 1; i < 12; i++) { trailX_[i - 1] = trailX_[i]; trailY_[i - 1] = trailY_[i]; }
            trailN_--;
        }
        trailX_[trailN_] = tipX;
        trailY_[trailN_] = tipY;
        trailN_++;
    } else if (trailN_ > 0) {
        for (int i = 1; i < trailN_; i++) { trailX_[i - 1] = trailX_[i]; trailY_[i - 1] = trailY_[i]; }
        trailN_--;
    }
}

// Paso 4: de lado (frontis a la izquierda): dejada, media y larga, una tras otra, con su botón
void PalaHowTo::drawShots(float cx, float cy) {
    const float fx = cx - 160, floorY = cy + 70, len = 300;
    gfx::rect(fx - 14, cy - 110, 14, floorY - cy + 110, GREEN_WALL);
    gfx::rect(fx - 14, floorY - 18, 14, 18, rgba(0xC8, 0xD0, 0xD8));
    gfx::rect(fx - 14, floorY, len + 30, 6, FLOOR);
    float u = fmodf(t_, PERIOD[3]) / PERIOD[3];
    int k = clampv((int)(u * 3), 0, 2);
    float v = u * 3 - k;  // 0..1 dentro de cada golpe
    const float land[3] = {0.18f, 0.5f, 0.88f}, hit[3] = {0.12f, 0.35f, 0.6f};
    float startX = fx + len * 0.78f, startY = floorY - 40;
    float wallY = floorY - 18 - hit[k] * 130;
    float landX = fx + len * land[k];
    // la trayectoria entera, tenue
    for (int i = 0; i < 24; i++) {
        float s = i / 23.0f, x, y;
        if (s < 0.3f) { float q = s / 0.3f; x = lerpf(startX, fx, q); y = lerpf(startY, wallY, q) - sinf(PI * q) * 12; }
        else { float q = (s - 0.3f) / 0.7f; x = lerpf(fx, landX, q); y = lerpf(wallY, floorY - 4, q * q) - sinf(PI * q) * (20 + 30 * k); }
        gfx::circle(x, y, 2, rgba(255, 255, 255, 150), 6);
    }
    // la pelota
    float x, y;
    if (v < 0.12f) { x = startX; y = startY; }
    else if (v < 0.3f) { float q = (v - 0.12f) / 0.18f; x = lerpf(startX, fx, q); y = lerpf(startY, wallY, q) - sinf(PI * q) * 12; }
    else if (v < 0.85f) { float q = (v - 0.3f) / 0.55f; x = lerpf(fx, landX, q); y = lerpf(wallY, floorY - 4, q * q) - sinf(PI * q) * (20 + 30 * k); }
    else { x = landX; y = floorY - 4; gfx::ring(landX, floorY + 2, 8, 11, UI_WHITE, 16); }
    ball2D(x, y, 6);
    // el pelotari a la derecha
    gfx::circle(startX + 16, floorY - 50, 12, UI_TEAL, 18);
    gfx::roundRect(startX + 8, floorY - 40, 16, 40, 6, rgba(0x26, 0x32, 0x58));
    // las tres opciones, con la de ahora resaltada
    const char* keys[3] = {"B", "", "A"};
    for (int t = 0; t < 3; t++) {
        float bx = cx - 110 + t * 110, by = cy - 132;
        bool on = t == k;
        gfx::roundRect(bx - 50, by - 17, 100, 34, 17, on ? UI_TEAL : rgba(0x9C, 0xB4, 0xC4));
        if (keys[t][0]) ui::buttonIcon(bx - 34, by, keys[t], 12, on ? UI_TEAL_GLOW : rgba(0x80, 0x88, 0x94));
        font::plain(txt::SHOT_NAMES[t], bx + (keys[t][0] ? 10 : 0), by - 12, 20, UI_WHITE, ALIGN_CENTER);
    }
}

// Paso 5: el momento del golpe: pronto, justo (Bikain!) y tarde, con la barra del juego
void PalaHowTo::drawSweet(float cx, float cy) {
    float u = fmodf(t_, PERIOD[4]) / PERIOD[4];
    int k = clampv((int)(u * 3), 0, 2);
    float v = u * 3 - k;
    const float hitAt[3] = {0.48f, 0.6f, 0.72f};  // pronto, justo, tarde
    const float pX = cx + 40, lineY = cy - 20;
    // el pelotari de lado y la zona buena a su lado
    gfx::roundRect(pX - 40, lineY - 40, 80, 80, 40, rgba(0x3C, 0xB8, 0x48, 70));
    gfx::circle(pX + 30, lineY - 28, 13, UI_TEAL, 18);
    gfx::roundRect(pX + 22, lineY - 16, 16, 44, 6, rgba(0x26, 0x32, 0x58));
    // la pelota viene de la izquierda
    float bx = lerpf(cx - 170, pX + 60, clampv(v / 0.85f, 0.0f, 1.0f)), by = lineY + sinf(PI * v) * -20;
    bool hitNow = v >= hitAt[k];
    if (hitNow) {
        float q = clampv((v - hitAt[k]) / 0.25f, 0.0f, 1.0f);
        float atX = lerpf(cx - 170, pX + 60, hitAt[k] / 0.85f);
        bx = lerpf(atX, cx - 170, q);
        by = lineY - 10 - q * 60;
        if (k == 1) gfx::circle(atX, lineY, 16 * (1 - q) + 1, rgba(255, 255, 255, (int)(200 * (1 - q))), 20);
    }
    ball2D(bx, by, 7);
    // la pala: arco rápido en el momento del golpe
    float sw = clampv((v - hitAt[k] + 0.05f) / 0.12f, 0.0f, 1.0f);
    float ang = lerpf(1.3f, -1.6f, sw);
    float hx = pX + 18, hy = lineY - 6;
    gfx::triangle(hx, hy, hx + cosf(ang) * 46 - sinf(ang) * 6, hy + sinf(ang) * 46 + cosf(ang) * 6,
                  hx + cosf(ang) * 46 + sinf(ang) * 6, hy + sinf(ang) * 46 - cosf(ang) * 6, rgba(0xE4, 0xB8, 0x78));
    // la barra de tiempo (como en el juego)
    float w = 220, by2 = cy + 92;
    gfx::roundRect(cx - w / 2 - 70, by2 - 22, w + 140, 44, 16, rgba(0x10, 0x30, 0x48, 150));
    gfx::roundRect(cx - w / 2, by2 - 4, w, 8, 4, rgba(255, 255, 255, 130));
    gfx::roundRect(cx - w * 0.11f, by2 - 4, w * 0.22f, 8, 4, UI_GREEN);
    font::plain(txt::LATE, cx - w / 2 - 8, by2 - 9, 15, UI_WHITE, ALIGN_RIGHT);
    font::plain(txt::EARLY, cx + w / 2 + 8, by2 - 9, 15, UI_WHITE, ALIGN_LEFT);
    if (hitNow) {
        const float mark[3] = {0.75f, 0.0f, -0.75f};
        gfx::circle(cx + mark[k] * w / 2, by2, 8, UI_GOLD, 14);
        const char* lab = k == 0 ? txt::EARLY : k == 1 ? txt::GREAT : txt::LATE;
        font::draw(lab, pX, lineY - 100, 30, k == 1 ? UI_GOLD : UI_WHITE, rgba(0x60, 0x40, 0x10), ALIGN_CENTER);
    }
}

void PalaHowTo::draw() {
    const float W = gfx::screenW();
    gfx::begin2D();
    ui::fade(0.45f, rgba(0x0A, 0x2C, 0x4C));
    ui::header(txt::HOWTO);
    const float px = W * 0.05f, pw = W * 0.9f, py = 90, ph = 320;
    ui::panel(px, py, pw, ph);
    // En 4:3 el panel es más estrecho: la ilustración un poco más a la derecha y el texto más estrecho (se encoge solo)
    const bool wide = gfx::widescreen();
    const float sx = px + pw * (wide ? 0.27f : 0.31f), sy = py + ph * 0.56f;
    gfx::circle(sx, sy - 10, 150, rgba(255, 255, 255, 90), 48);
    gfx::circle(sx, sy - 10, 110, rgba(255, 255, 255, 70), 48);

    // Texto del paso
    const float tx = px + pw * (wide ? 0.54f : 0.63f);
    gfx::circle(tx + 20, py + 52, 20, UI_TEAL, 24);
    char num[2] = {(char)('1' + step_), 0};
    font::plain(num, tx + 20, py + 37, 28, UI_WHITE, ALIGN_CENTER);
    const float textW = px + pw - 24 - tx;
    float ts = fminf(34.0f, 34.0f * (textW - 52) / fmaxf(1.0f, font::width(txt::PALA_HOWTO_STEPS[step_][0], 34)));
    float ds = 22.0f;
    for (int k = 1; k <= 2; k++) ds = fminf(ds, 22.0f * textW / fmaxf(1.0f, font::width(txt::PALA_HOWTO_STEPS[step_][k], 22)));
    font::draw(txt::PALA_HOWTO_STEPS[step_][0], tx + 52, py + 52 - ts * 0.62f, ts, UI_TEAL, UI_WHITE);
    font::plain(txt::PALA_HOWTO_STEPS[step_][1], tx, py + 104, ds, UI_TEXT);
    font::plain(txt::PALA_HOWTO_STEPS[step_][2], tx, py + 104 + ds * 1.4f, ds, UI_TEXT);
    for (int i = 0; i < NUM_STEPS; i++)
        gfx::circle(tx + 14 + i * 30, py + ph - 42, i == step_ ? 9 : 6, i == step_ ? UI_TEAL : rgba(0xB4, 0xC6, 0xD2), 20);

    switch (step_) {
        case 0: drawMove(sx, sy); break;
        case 1: drawAim(sx, sy); break;
        case 2: drawWhip(sx, sy); break;
        case 3: drawShots(sx, sy); break;
        case 4: drawSweet(sx, sy); break;
    }
    ui::hints(step_ == NUM_STEPS - 1 ? txt::GO : txt::HOWTO_NEXT, step_ > 0 ? txt::BACK : nullptr,
              step_ < NUM_STEPS - 1 ? txt::SKIP : nullptr);
}
