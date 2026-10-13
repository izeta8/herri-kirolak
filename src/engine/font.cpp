#include "engine/font.h"
#include "engine/gfx.h"
#include <gccore.h>
#include <malloc.h>
#include <string.h>
#include "font_bin.h"

namespace font {

struct Glyph {
    u16 code, x, y, w, h;
    s16 xoff, yoff, adv;
};

static GXTexObj s_tex;
static int s_texW, s_texH, s_size, s_ascent, s_lineH;
static Glyph s_glyphs[256];  // indexado por punto de código (solo Latin-1)
static bool s_has[256];

static u16 be16(const u8* p) { return (u16)((p[0] << 8) | p[1]); }

void init() {
    const u8* p = font_bin;
    if (memcmp(p, "FNT1", 4) != 0) return;
    s_texW = be16(p + 4);
    s_texH = be16(p + 6);
    s_size = be16(p + 8);
    s_ascent = (s16)be16(p + 10);
    s_lineH = (s16)be16(p + 14);
    int n = be16(p + 16);
    const u8* g = p + 20;
    for (int i = 0; i < n; i++, g += 16) {
        Glyph gl;
        gl.code = be16(g);
        gl.x = be16(g + 2);
        gl.y = be16(g + 4);
        gl.w = be16(g + 6);
        gl.h = be16(g + 8);
        gl.xoff = (s16)be16(g + 10);
        gl.yoff = (s16)be16(g + 12);
        gl.adv = (s16)be16(g + 14);
        if (gl.code < 256) {
            s_glyphs[gl.code] = gl;
            s_has[gl.code] = true;
        }
    }
    unsigned off = (unsigned)(g - p);
    off = (off + 31) & ~31u;
    unsigned texBytes = s_texW * s_texH * 2;
    void* tex = memalign(32, texBytes);
    memcpy(tex, p + off, texBytes);
    DCFlushRange(tex, texBytes);
    GX_InitTexObj(&s_tex, tex, s_texW, s_texH, GX_TF_IA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjLOD(&s_tex, GX_LINEAR, GX_LINEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
}

// Decodifica el siguiente carácter UTF-8 (solo hasta U+00FF; el resto se convierte en '?')
static unsigned nextChar(const char*& s) {
    unsigned char c = (unsigned char)*s++;
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0 && *s) {
        unsigned cp = ((c & 0x1F) << 6) | ((unsigned char)*s++ & 0x3F);
        return cp < 256 ? cp : '?';
    }
    while ((*s & 0xC0) == 0x80) s++;
    return '?';
}

float width(const char* text, float size) {
    float k = size / s_size, w = 0;
    while (*text) {
        unsigned cp = nextChar(text);
        if (cp < 256 && s_has[cp]) w += s_glyphs[cp].adv * k;
    }
    return w;
}

float lineHeight(float size) { return s_lineH * size / s_size; }

static void drawImpl(const char* text, float x, float y, float size, Color fill, Color outline, Align align,
                     float alpha, bool outlined) {
    if (!s_size || alpha <= 0.0f) return;
    float k = size / s_size;
    if (align == ALIGN_CENTER) x -= width(text, size) * 0.5f;
    else if (align == ALIGN_RIGHT) x -= width(text, size);
    float baseline = y + s_ascent * k;
    Color c = withAlpha(fill, alpha);
    gfx::beginTextured(&s_tex, outline, outlined);
    while (*text) {
        unsigned cp = nextChar(text);
        if (cp >= 256 || !s_has[cp]) continue;
        const Glyph& g = s_glyphs[cp];
        if (g.w && g.h) {
            float u0 = (float)g.x / s_texW, v0 = (float)g.y / s_texH;
            float u1 = (float)(g.x + g.w) / s_texW, v1 = (float)(g.y + g.h) / s_texH;
            gfx::texQuad(x + g.xoff * k, baseline + g.yoff * k, g.w * k, g.h * k, u0, v0, u1, v1, c);
        }
        x += g.adv * k;
    }
    gfx::endTextured();
}

void draw(const char* text, float x, float y, float size, Color fill, Color outline, Align align, float alpha) {
    drawImpl(text, x, y, size, fill, outline, align, alpha, true);
}

void plain(const char* text, float x, float y, float size, Color c, Align align, float alpha) {
    drawImpl(text, x, y, size, c, c, align, alpha, false);
}

}  // namespace font
