#pragma once
// Texto con contorno a partir del atlas generado por tools/bake_font.py
#include "engine/math.h"

enum Align { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

namespace font {

void init();
// Dibuja texto UTF-8. `size` es la altura de letra en píxeles virtuales (la base es 44).
// `y` es la parte superior de la línea.
void draw(const char* text, float x, float y, float size, Color fill, Color outline, Align align = ALIGN_LEFT,
          float alpha = 1.0f);
// Igual, pero sin contorno visible (contorno = relleno)
void plain(const char* text, float x, float y, float size, Color c, Align align = ALIGN_LEFT, float alpha = 1.0f);
float width(const char* text, float size);
float lineHeight(float size);

}  // namespace font
