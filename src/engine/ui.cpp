#include "engine/ui.h"
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
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

static void (*s_background)(float) = nullptr;

void setBackground(void (*draw)(float t)) { s_background = draw; }

void background(float t) {
    float w = gfx::screenW();
    if (s_background) {
        // Fondo del juego (el escenario en 3D) con un velo claro para que destaquen los paneles
        s_background(t);
        gfx::begin2D();
        gfx::clearDepth();
        gfx::rectGrad(0, 0, w, 480, rgba(255, 255, 255, 40), rgba(255, 255, 255, 90));
        return;
    }
    // Sin escenario: cielo con nubes
    gfx::rectGrad(0, 0, w, 480, rgba(0x2C, 0x84, 0xE0), rgba(0xC8, 0xE2, 0xF2));
    for (int i = 0; i < 6; i++) {
        float s = 0.7f + (i % 3) * 0.3f;
        float cx = fmodf(i * 211.0f + t * (5 + (i % 3) * 3), w + 320) - 160;
        float cy = 70 + (i * 67) % 300;
        cloud2D(cx, cy, s, rgba(255, 255, 255, 235));
    }
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

void row(float x, float y, float w, float h, float anim) { glass(x, y, w, h, h / 2, anim); }

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

// ------------------------------------------------------------------ puntero
static Pointer s_ptr;
static bool s_ptrWanted = false;
static float s_lastX[input::MAX_PADS], s_lastY[input::MAX_PADS];
static const Color PTR_COLORS[input::MAX_PADS] = {rgba(0x2C, 0x9C, 0xE8), rgba(0xE8, 0x3A, 0x3A), rgba(0x3C, 0xB8, 0x48),
                                                  rgba(0xF0, 0xB0, 0x20)};

void updatePointer() {
    s_ptrWanted = false;
    // Manda el último mando que se movió o pulsó algo con el puntero en la pantalla
    for (int i = 0; i < input::MAX_PADS; i++) {
        const input::Pad& p = input::pad(i);
        if (!p.connected || !p.irValid) continue;
        float d = fabsf(p.irX - s_lastX[i]) + fabsf(p.irY - s_lastY[i]);
        if (d > 3.0f || p.down) s_ptr.pad = i;
        s_lastX[i] = p.irX;
        s_lastY[i] = p.irY;
    }
    const input::Pad& a = input::pad(s_ptr.pad);
    s_ptr.valid = a.connected && a.irValid;
    if (!s_ptr.valid) {
        for (int i = 0; i < input::MAX_PADS && !s_ptr.valid; i++)
            if (input::pad(i).connected && input::pad(i).irValid) {
                s_ptr.pad = i;
                s_ptr.valid = true;
            }
    }
    if (s_ptr.valid) {
        s_ptr.x = input::pad(s_ptr.pad).irX;
        s_ptr.y = input::pad(s_ptr.pad).irY;
    }
}

const Pointer& pointer() { return s_ptr; }

bool pointerOver(float x, float y, float w, float h) {
    return s_ptr.valid && s_ptr.x >= x && s_ptr.x <= x + w && s_ptr.y >= y && s_ptr.y <= y + h;
}

void usePointer() { s_ptrWanted = true; }

void hoverFeedback() {
    audio::play(SFX_MOVE, 0.35f, 1.15f);
    input::rumble(s_ptr.pad, 0.025f);
}

// Mano de Wii: dedo índice apuntando (la punta es el punto exacto), palma del color del jugador con su número
static void hand(float x, float y, Color c, int num) {
    const float k = 1.4f;
    const Color white = UI_WHITE, line = rgba(0x30, 0x40, 0x50, 170), shadow = rgba(0, 0, 0, 55);
    auto shape = [&](float dx, float dy, float grow, Color col) {
        gfx::roundRect(x + dx - (4 + grow) * k, y + dy - grow * k, (11 + 2 * grow) * k, (26 + grow) * k, (5.5f + grow) * k, col);
        gfx::circle(x + dx + 9 * k, y + dy + 30 * k, (14.5f + grow) * k, col, 22);
        gfx::circle(x + dx - 4 * k, y + dy + 29 * k, (6 + grow) * k, col, 14);  // pulgar
    };
    shape(3, 4, 0, shadow);
    shape(0, 0, 1.4f, line);
    shape(0, 0, 0, white);
    gfx::circle(x + 9 * k, y + 31 * k, 9.5f * k, c, 18);
    char n[2] = {(char)('1' + num), 0};
    font::plain(n, x + 9 * k, y + 22 * k, 15 * k, white, ALIGN_CENTER);
}

void drawPointers() {
    if (!s_ptrWanted) return;
    gfx::begin2D();
    for (int i = input::MAX_PADS - 1; i >= 0; i--) {
        const input::Pad& p = input::pad(i);
        if (p.connected && p.irValid) hand(p.irX, p.irY, PTR_COLORS[i], i);
    }
}

void ListMenu::place(int i, float x, float y, float w, float h) {
    if (i < 0 || i >= 16) return;
    rx[i] = x;
    ry[i] = y;
    rw[i] = w;
    rh[i] = h;
    placed[i] = true;
}

int ListMenu::hovered() const {
    if (!s_ptr.valid) return -1;
    for (int i = 0; i < count && i < 16; i++)
        if (placed[i] && enabled[i] && pointerOver(rx[i], ry[i], rw[i], rh[i])) return i;
    return -1;
}

void menuButton(ListMenu& m, int i, float cx, float cy, float w, float h, const char* label, const char* sub) {
    m.place(i, cx - w / 2, cy - h / 2, w, h);
    button(cx, cy, w, h, label, m.anim[i], m.enabled[i], sub);
}

int ListMenu::update(float dt, unsigned down) {
    usePointer();
    // El puntero elige la opción que tiene debajo
    int hov = hovered();
    if (hov >= 0 && hov != sel) {
        sel = hov;
        hoverFeedback();
    }
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
