#pragma once
// Escenas con transición por fundido, y diálogo global de HOME.

struct MatchConfig;  // game/sports.h

class Scene {
public:
    virtual ~Scene() {}
    virtual void update(float dt) = 0;
    virtual void draw() = 0;
};

namespace scenes {
void start(Scene* first);
void go(Scene* next);         // cambia de escena con fundido (la anterior se borra)
// Igual, pero la escena se construye cuando la pantalla ya está en negro (para las partidas, que tardan en construirse:
// así el parón no se ve al pulsar A)
void goMatch(Scene* (*make)(const MatchConfig&), const MatchConfig& cfg);
bool frame(float dt);         // false = salir del juego
bool transitioning();
void openExitDialog();         // "¿Volver al Homebrew Channel?"
}  // namespace scenes
