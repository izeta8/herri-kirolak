#pragma once
// El pelotari: personaje con la pala de madera en la mano derecha, mirando siempre al frontis (-Z).
// La escena le dice dónde está y cómo se mueve; él anima la carrera, la preparación (derecha o revés),
// el golpe y el bote del saque.
#include "game/character.h"

class Pelotari {
public:
    void setup(const Appearance& look);
    void update(float dt);
    void draw();

    Vec3 pos;           // pies
    Vec3 vel;           // para animar la carrera
    // Preparar el golpe: lado (+1 derecha, -1 revés), altura de la pelota y cuánto (0 = reposo, 1 = pala atrás)
    void prepare(int side, float height, float amount);
    void swing(int side, float height);  // golpe (o golpe al aire)
    void dropBall();                     // saque: suelta la pelota con la mano izquierda
    void celebrate(bool on) { celebrate_ = on; }
    bool swinging() const { return swingT_ >= 0; }
    // Dónde se coloca la pelota al soltarla para sacar (a la derecha, por delante)
    Vec3 dropPoint() const;

private:
    void computePose(CharacterPose& pose, Xf& palaXf);
    Appearance look_;
    float time_ = 0, run_ = 0, runPhase_ = 0;
    int side_ = 1;
    float height_ = 1.0f, prep_ = 0, prepTarget_ = 0;
    float swingT_ = -1;  // < 0 = no está golpeando
    float theta_ = 0.3f;
    float dropT_ = -1;
    bool celebrate_ = false;
    float hop_ = 0, blinkT_ = 2;
};
