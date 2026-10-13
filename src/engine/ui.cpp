#include "engine/ui.h"
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "game/palette.h"
#include <stdio.h>
#include <wiiuse/wpad.h>

namespace ui {

void background(float t) {
    float w = gfx::screenW();
    gfx::rectGrad(0, 0, w, 480, UI_BG_TOP, UI_BG_BOTTOM);
    // Franjas diagonales suaves que se desplazan
    float off = fmodf(t * 18.0f, 120.0f);
    for (float x = -480 - 120 + off; x < w + 120; x += 120) {
        gfx::setBlend(true);
        GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
        Color c = rgba(255, 255, 255, 70);
        GX_Position3f32(x, 480, 0); GX_Color1u32(c);
        GX_Position3f32(x + 50, 480, 0); GX_Color1u32(c);
        GX_Position3f32(x + 530, 0, 0); GX_Color1u32(c);
        GX_Position3f32(x + 480, 0, 0); GX_Color1u32(c);
        GX_End();
    }
    // Burbujas
    for (int i = 0; i < 6; i++) {
        float bx = fmodf(i * 173.0f + t * (8 + i * 3), w + 200) - 100;
        float by = 420 - fmodf(i * 97.0f + t * (6 + i), 520);
        gfx::circle(bx, by, 30 + i * 9, rgba(255, 255, 255, 50), 28);
    }
}

void header(const char* title) {
    float w = font::width(title, 34) + 80;
    gfx::roundRect(-30, 18, w + 30, 58, 29, rgba(0, 0, 0, 40));
    gfx::roundRectGrad(-30, 14, w + 30, 58, 29, rgba(0x4C, 0xB4, 0xF4), UI_BLUE_DARK);
    font::draw(title, 34, 22, 34, UI_WHITE, UI_BLUE_DARK);
}

void panel(float x, float y, float w, float h, Color c) {
    gfx::roundRect(x + 3, y + 5, w, h, 22, rgba(0, 0, 0, 35));
    gfx::roundRect(x, y, w, h, 22, rgba(0xD8, 0xE4, 0xEE));
    gfx::roundRect(x + 3, y + 3, w - 6, h - 6, 19, c);
}

void button(float cx, float cy, float w, float h, const char* label, float anim, bool enabled, const char* sub,
            Color accent) {
    float s = 1.0f + 0.07f * easeOutBack(anim);
    float bw = w * s, bh = h * s;
    float x = cx - bw / 2, y = cy - bh / 2;
    float r = bh * 0.45f;
    gfx::roundRect(x + 2, y + 6, bw, bh, r, rgba(0, 0, 0, 45));
    if (anim > 0.01f) {
        gfx::roundRect(x - 6, y - 6, bw + 12, bh + 12, r + 6, withAlpha(accent, 0.35f * anim));
    }
    Color border = mixColor(rgba(0xC4, 0xCE, 0xD8), accent, anim);
    gfx::roundRect(x, y, bw, bh, r, border);
    float bt = 3 + 2 * anim;
    gfx::roundRectGrad(x + bt, y + bt, bw - 2 * bt, bh - 2 * bt, r - bt, UI_WHITE,
                       enabled ? rgba(0xE6, 0xF0, 0xF8) : rgba(0xD8, 0xDA, 0xDE));
    Color tc = enabled ? UI_TEXT : UI_TEXT_LIGHT;
    float size = fminf(32.0f, bh * 0.42f) * s;
    if (sub) {
        font::plain(label, cx, cy - size * 0.95f, size, tc, ALIGN_CENTER);
        font::plain(sub, cx, cy + size * 0.12f, size * 0.62f, UI_TEXT_LIGHT, ALIGN_CENTER);
    } else {
        font::plain(label, cx, cy - size * 0.62f, size, tc, ALIGN_CENTER);
    }
}

void buttonIcon(float cx, float cy, const char* letter, float r, Color c) {
    gfx::circle(cx, cy + 2, r, rgba(0, 0, 0, 50), 24);
    gfx::circle(cx, cy, r, UI_WHITE, 24);
    gfx::circle(cx, cy, r - 2.5f, c, 24);
    float size = r * 1.35f;
    font::plain(letter, cx, cy - size * 0.62f, size, UI_WHITE, ALIGN_CENTER);
}

void hints(const char* aLabel, const char* bLabel) {
    float w = gfx::screenW();
    gfx::rect(0, 438, w, 42, rgba(255, 255, 255, 200));
    gfx::rect(0, 436, w, 2, rgba(0xB8, 0xD4, 0xEA));
    float x = w - 30;
    if (bLabel) {
        float tw = font::width(bLabel, 22);
        font::plain(bLabel, x - tw, 446, 22, UI_TEXT);
        x -= tw + 26;
        buttonIcon(x, 459, "B", 14, rgba(0x80, 0x88, 0x94));
        x -= 34;
    }
    if (aLabel) {
        float tw = font::width(aLabel, 22);
        font::plain(aLabel, x - tw, 446, 22, UI_TEXT);
        x -= tw + 26;
        buttonIcon(x, 459, "A", 14);
    }
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
