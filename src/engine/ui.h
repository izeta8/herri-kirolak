#pragma once
// Elementos de interfaz con estilo Wii, manejados solo con cruceta + A/B.
#include "engine/math.h"

namespace ui {

void background(float t);
// El juego puede poner su propio fondo de menús (p. ej. el escenario en 3D); ui no sabe nada del juego.
void setBackground(void (*draw)(float t));
void header(const char* title);
void panel(float x, float y, float w, float h, Color c = 0xFFFFFFF0);
// Botón grande y redondeado. `anim` 0..1 = cuánto está seleccionado (para animarlo).
void button(float cx, float cy, float w, float h, const char* label, float anim, bool enabled = true,
            const char* sub = nullptr, Color accent = 0x2C9CE8FF);
// Icono redondo de botón del Wii Remote (A, B, +, 1...)
void buttonIcon(float cx, float cy, const char* letter, float r = 15, Color c = 0x2C9CE8FF);
// Barra inferior de ayuda: "(A) aukeratu   (B) atzera"
void hints(const char* aLabel, const char* bLabel = nullptr);
void fade(float a, Color c = 0x000000FF);
void colorDot(float cx, float cy, float r, Color c);
// Formatea un tiempo en ms como "12.34" o "1:02.34"
void formatTime(unsigned ms, char* out, int size);

// Lista de opciones navegable con la cruceta
struct ListMenu {
    int sel = 0;
    int count = 0;
    float anim[16] = {};
    bool horizontal = false;
    bool enabled[16];
    ListMenu() { for (bool& e : enabled) e = true; }
    // Devuelve el índice elegido con A, -2 si se pulsa B, -1 en otro caso
    int update(float dt, unsigned down);
};

}  // namespace ui
