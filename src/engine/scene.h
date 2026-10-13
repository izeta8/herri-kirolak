#pragma once
// Escenas con transición por fundido, y diálogo global de HOME.

class Scene {
public:
    virtual ~Scene() {}
    virtual void update(float dt) = 0;
    virtual void draw() = 0;
};

namespace scenes {
void start(Scene* first);
void go(Scene* next);         // cambia de escena con fundido (la anterior se borra)
bool frame(float dt);         // false = salir del juego
bool transitioning();
void openExitDialog();         // "¿Volver al Homebrew Channel?"
}  // namespace scenes
