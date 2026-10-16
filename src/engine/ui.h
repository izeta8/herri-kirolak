#pragma once
// Elementos de interfaz con estilo Wii, manejados con la cruceta + A/B o con el puntero IR del Wii Remote.
#include "engine/math.h"

namespace ui {

void background(float t);
// El juego puede poner su propio fondo de menús (p. ej. el escenario en 3D); ui no sabe nada del juego.
void setBackground(void (*draw)(float t));
void header(const char* title);
void panel(float x, float y, float w, float h, Color c = 0xFFFFFFF0);
// Fila de lista de cristal (como un botón, pero el contenido lo dibuja quien la usa); anim 0..1 = elegida
void row(float x, float y, float w, float h, float anim);
// Botón grande y redondeado. `anim` 0..1 = cuánto está seleccionado (para animarlo).
void button(float cx, float cy, float w, float h, const char* label, float anim, bool enabled = true,
            const char* sub = nullptr, Color accent = 0x2C9CE8FF);
// Icono redondo de botón del Wii Remote (A, B, +, 1...)
void buttonIcon(float cx, float cy, const char* letter, float r = 15, Color c = 0x2C9CE8FF);
// Barra inferior de ayuda: "(A) aukeratu   (B) atzera"
void hints(const char* aLabel, const char* bLabel = nullptr, const char* plusLabel = nullptr);
void fade(float a, Color c = 0x000000FF);
void colorDot(float cx, float cy, float r, Color c);
// Pila del mando: 4 rayas (verde, amarillo con 1, rojo con 0)
void battery(float x, float y, int bars);
// Formatea un tiempo en ms como "12.34" o "1:02.34"
void formatTime(unsigned ms, char* out, int size);

// ---------- Puntero IR ----------
// El de cualquier Wii Remote que apunte a la pantalla; manda el último que se movió o pulsó algo.
struct Pointer {
    bool valid = false;
    float x = 0, y = 0;
    int pad = 0;
};
void updatePointer();                 // una vez por frame, antes de la escena (scenes::frame)
const Pointer& pointer();
bool pointerOver(float x, float y, float w, float h);  // rectángulo con la esquina arriba a la izquierda
void usePointer();                    // la escena usa el puntero este frame: se dibujan las manos
void drawPointers();                  // al final del frame (scenes::frame), si alguien lo pidió
void hoverFeedback();                 // clic y vibración corta al pasar el puntero a otro botón

// Lista de opciones navegable con la cruceta o con el puntero
struct ListMenu {
    int sel = 0;
    int count = 0;
    float anim[16] = {};
    bool horizontal = false;
    bool enabled[16];
    // Rectángulos de las opciones (los pone el dibujo con place() o menuButton()) para el puntero
    float rx[16], ry[16], rw[16], rh[16];
    bool placed[16] = {};
    ListMenu() { for (bool& e : enabled) e = true; }
    void place(int i, float x, float y, float w, float h);
    int hovered() const;  // opción bajo el puntero, −1 si ninguna
    // Devuelve el índice elegido con A, -2 si se pulsa B, -1 en otro caso
    int update(float dt, unsigned down);
};

// Diálogo "¿Seguro?" (Bai / Ez) encima de la pantalla; empieza con "Ez" elegido
struct Confirm {
    bool open = false;
    int action = 0;  // lo que se hará si dice que sí (lo decide quien lo usa)
    ListMenu menu;
    void ask(int what);
    int update(float dt, unsigned down);  // 1 = sí, 0 = no (o B), −1 = sigue abierto
    void draw(const char* question);
};

// Botón de una ListMenu: lo dibuja y apunta su rectángulo para el puntero
void menuButton(ListMenu& m, int i, float cx, float cy, float w, float h, const char* label, const char* sub = nullptr);

}  // namespace ui
