#include "engine/ui.h"
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "game/palette.h"
#include <stdio.h>
#include <wiiuse/wpad.h>

namespace ui {

// Nube esponjosa de dibujo (centro de la base, escala)
static void cloud2D(float cx, float cy, float s, Color c) {
    gfx::circle(cx, cy - 14 * s, 30 * s, c, 24);
    gfx::circle(cx + 34 * s, cy - 4 * s, 24 * s, c, 24);
    gfx::circle(cx - 34 * s, cy - 2 * s, 22 * s, c, 24);
    gfx::circle(cx + 12 * s, cy - 30 * s, 22 * s, c, 24);
    gfx::roundRect(cx - 56 * s, cy - 6 * s, 112 * s, 24 * s, 12 * s, c);
}

// Palmera de dibujo: tronco curvado de base (bx, by) a la copa (tx, ty)
static void palm2D(float bx, float by, float tx, float ty, float s) {
    const int n = 10;
    float px = bx, py = by;
    for (int i = 1; i <= n; i++) {
        float t = i / (float)n;
        float x = bx + (tx - bx) * t * t, y = by + (ty - by) * t;
        float w0 = 11 * s * (1 - 0.4f * (t - 1.0f / n)), w1 = 11 * s * (1 - 0.4f * t);
        Color c = (i & 1) ? rgba(0x9E, 0x7A, 0x50) : rgba(0x88, 0x66, 0x42);
        gfx::triangle(px - w0, py, px + w0, py, x + w1, y, c);
        gfx::triangle(px - w0, py, x + w1, y, x - w1, y, c);
        px = x;
        py = y;
    }
    for (int k = 0; k < 7; k++) {
        float a = -PI * 0.05f - k * PI * 0.15f;  // de la derecha a la izquierda, hacia arriba
        float len = (k & 1) ? 95 * s : 120 * s;
        float ex = tx + cosf(a) * len, ey = ty + sinf(a) * len * 0.45f + len * 0.42f;
        float mx = tx + cosf(a) * len * 0.5f, my = ty + sinf(a) * len * 0.5f - 6 * s;
        float nx = -sinf(a) * 16 * s, ny = cosf(a) * 16 * s;
        Color g = (k & 1) ? rgba(0x2E, 0x96, 0x3A) : rgba(0x3A, 0xAA, 0x40);
        gfx::triangle(tx, ty, mx + nx, my + ny, ex, ey, g);
        gfx::triangle(tx, ty, mx - nx, my - ny, ex, ey, shade(g, 0.85f));
    }
}

void background(float t) {
    // Playa de Wii Sports Resort: cielo intenso, nubes, mar turquesa, arena y palmeras
    float w = gfx::screenW();
    gfx::rectGrad(0, 0, w, 190, rgba(0x14, 0x78, 0xE0), rgba(0x5C, 0xB4, 0xF2));
    gfx::rectGrad(0, 190, w, 100, rgba(0x5C, 0xB4, 0xF2), rgba(0xC4, 0xE8, 0xFA));
    for (int i = 0; i < 6; i++) {
        float s = 0.7f + (i % 3) * 0.3f;
        float cx = fmodf(i * 211.0f + t * (5 + (i % 3) * 3), w + 320) - 160;
        float cy = 70 + (i * 67) % 160;
        cloud2D(cx, cy, s, rgba(255, 255, 255, 235));
    }
    // Islas en el horizonte: colina verde y el volcán
    gfx::circle(w * 0.16f, 318, 46, rgba(0x58, 0xA8, 0x4C), 32);
    gfx::circle(w * 0.24f, 314, 30, rgba(0x4C, 0x9A, 0x44), 28);
    gfx::triangle(w * 0.62f, 292, w * 0.7f, 236, w * 0.79f, 292, rgba(0xA8, 0x86, 0x60));
    gfx::triangle(w * 0.7f, 236, w * 0.79f, 292, w * 0.72f, 292, rgba(0x8A, 0x6C, 0x4C));
    gfx::rect(w * 0.6f, 284, w * 0.21f, 8, rgba(0x58, 0xA8, 0x44));
    // Mar
    gfx::rectGrad(0, 290, w, 36, rgba(0x18, 0x84, 0xD8), rgba(0x2A, 0xAE, 0xE2));
    gfx::rectGrad(0, 326, w, 34, rgba(0x2A, 0xAE, 0xE2), rgba(0x70, 0xDC, 0xE6));
    for (int i = 0; i < 8; i++) {
        float lx = fmodf(i * 137.0f + t * 10, w + 80) - 40, ly = 300 + (i * 23) % 52;
        gfx::rect(lx, ly, 26 + (i % 3) * 10, 2, rgba(255, 255, 255, 110));
    }
    // Orilla con espuma y arena
    float foam = sinf(t * 1.2f) * 3;
    gfx::rect(0, 356 + foam, w, 8, rgba(0xF4, 0xFC, 0xFC, 230));
    gfx::rectGrad(0, 362 + foam, w, 118 - foam, rgba(0xF6, 0xEA, 0xC6), rgba(0xEC, 0xD8, 0xA8));
    palm2D(w - 70, 470, w - 120, 236, 1.0f);
    palm2D(w - 10, 470, w - 30, 300, 0.75f);
}

// Panel de cristal de Resort: borde blanco, relleno blanco-azulado translúcido
static void glass(float x, float y, float w, float h, float r, float glow) {
    gfx::roundRect(x + 2, y + 5, w, h, r, rgba(0x08, 0x40, 0x70, 40));
    if (glow > 0.01f) gfx::roundRect(x - 7, y - 7, w + 14, h + 14, r + 7, withAlpha(UI_TEAL_GLOW, 0.5f * glow));
    gfx::roundRect(x, y, w, h, r, mixColor(rgba(255, 255, 255, 235), UI_TEAL_GLOW, glow));
    float bt = 3 + 2 * glow;
    gfx::roundRectGrad(x + bt, y + bt, w - 2 * bt, h - 2 * bt, fmaxf(r - bt, 2.0f), UI_GLASS_TOP, UI_GLASS_BOTTOM);
}

void header(const char* title) {
    float w = font::width(title, 32) + 56;
    glass(-24, 16, w + 24, 56, 16, 0);
    font::draw(title, 26, 24, 32, UI_TEAL, UI_WHITE);
}

void panel(float x, float y, float w, float h, Color c) {
    (void)c;
    glass(x, y, w, h, 18, 0);
}

void button(float cx, float cy, float w, float h, const char* label, float anim, bool enabled, const char* sub,
            Color accent) {
    (void)accent;
    float s = 1.0f + 0.06f * easeOutBack(anim);
    float bw = w * s, bh = h * s;
    float x = cx - bw / 2, y = cy - bh / 2;
    glass(x, y, bw, bh, 14 * s, anim);
    Color tc = enabled ? UI_TEAL : UI_TEXT_LIGHT;
    float size = fminf(32.0f, bh * 0.44f) * s;
    if (sub) {
        font::draw(label, cx, cy - size * 0.98f, size, tc, UI_WHITE, ALIGN_CENTER);
        font::plain(sub, cx, cy + size * 0.12f, size * 0.62f, UI_TEXT_LIGHT, ALIGN_CENTER);
    } else {
        font::draw(label, cx, cy - size * 0.64f, size, tc, UI_WHITE, ALIGN_CENTER);
    }
}

void buttonIcon(float cx, float cy, const char* letter, float r, Color c) {
    gfx::circle(cx, cy + 2, r, rgba(0, 0, 0, 50), 24);
    gfx::circle(cx, cy, r, UI_WHITE, 24);
    gfx::circle(cx, cy, r - 2.5f, c, 24);
    float size = r * 1.35f;
    font::plain(letter, cx, cy - size * 0.62f, size, UI_WHITE, ALIGN_CENTER);
}

// Pastilla de cristal de la barra inferior (como "Back" / "Settings" en Resort); devuelve su ancho
static float hintPill(float right, const char* letter, const char* label, Color iconCol) {
    float tw = font::width(label, 22);
    float pw = tw + 64, x = right - pw;
    glass(x, 430, pw, 40, 20, 0);
    buttonIcon(x + 24, 450, letter, 14, iconCol);
    font::plain(label, x + 44, 438, 22, UI_TEXT);
    return pw;
}

void hints(const char* aLabel, const char* bLabel) {
    float x = gfx::screenW() - 20;
    if (bLabel) x -= hintPill(x, "B", bLabel, rgba(0x80, 0x88, 0x94)) + 12;
    if (aLabel) hintPill(x, "A", aLabel, UI_BLUE);
}

void fade(float a, Color c) {
    if (a <= 0.001f) return;
    gfx::rect(0, 0, gfx::screenW(), 480, withAlpha(c, clampv(a, 0.0f, 1.0f)));
}

void colorDot(float cx, float cy, float r, Color c) {
    gfx::circle(cx, cy + 1.5f, r + 2, rgba(0, 0, 0, 50), 20);
    gfx::circle(cx, cy, r + 2, UI_WHITE, 20);
    gfx::circle(cx, cy, r, c, 20);
}

void formatTime(unsigned ms, char* out, int size) {
    unsigned cs = ms / 10;
    unsigned s = cs / 100, c = cs % 100;
    if (s >= 60) snprintf(out, size, "%u:%02u.%02u", s / 60, s % 60, c);
    else snprintf(out, size, "%u.%02u", s, c);
}

int ListMenu::update(float dt, unsigned down) {
    for (int i = 0; i < count && i < 16; i++) anim[i] = approach(anim[i], i == sel ? 1.0f : 0.0f, 14.0f, dt);
    unsigned prev = horizontal ? WPAD_BUTTON_LEFT : WPAD_BUTTON_UP;
    unsigned next = horizontal ? WPAD_BUTTON_RIGHT : WPAD_BUTTON_DOWN;
    // El Wii Remote se suele coger en horizontal en los menús: la cruceta queda girada
    unsigned prevAlt = horizontal ? WPAD_BUTTON_UP : WPAD_BUTTON_LEFT;
    unsigned nextAlt = horizontal ? WPAD_BUTTON_DOWN : WPAD_BUTTON_RIGHT;
    int dir = 0;
    if (down & (prev | prevAlt)) dir = -1;
    if (down & (next | nextAlt)) dir = 1;
    if (dir && count > 0) {
        int s = sel;
        for (int k = 0; k < count; k++) {
            s = (s + dir + count) % count;
            if (enabled[s]) break;
        }
        if (s != sel) {
            sel = s;
            audio::play(SFX_MOVE, 0.6f);
        }
    }
    if (down & WPAD_BUTTON_A) {
        if (enabled[sel]) {
            audio::play(SFX_OK, 0.7f);
            return sel;
        }
        audio::play(SFX_BAD, 0.4f);
    }
    if (down & WPAD_BUTTON_B) {
        audio::play(SFX_BACK, 0.7f);
        return -2;
    }
    return -1;
}

}  // namespace ui
