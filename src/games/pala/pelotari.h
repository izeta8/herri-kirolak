#pragma once
// El pelotari: Mii con la pala de madera en la mano derecha, mirando al frontis (-Z).
// La escena le dice dónde está y cómo se mueve; él anima (estilo Wii Sports Resort):
//  - espera flexionado y botando un poco, carrera con piernas y rodillas, cuerpo que se gira hacia donde corre;
//  - plantado y agachado mientras apunta (Z);
//  - golpe: preparación (pala atrás por el lado de la pelota), golpe que llega a la pelota justo en el impacto, giro
//    del cuerpo, paso adelante y final; con un golpe fuerte, un saltito;
//  - bote del saque, celebración (salto con la pala arriba) y lamento (cabeza abajo).
#include "game/character.h"

class Pelotari {
public:
    void setup(const Appearance& look);
    void update(float dt);
    void draw();

    Vec3 pos;  // pies
    Vec3 vel;  // para animar la carrera

    // Preparar el golpe: lado (+1 derecha, -1 revés), altura de la pelota y cuánto (0 = reposo, 1 = pala atrás)
    void prepare(int side, float height, float amount);
    // Golpe: la pala llega a la pelota dentro de `contactIn` s (si ya estaba golpeando, solo se reajusta el momento)
    void swing(int side, float height, float contactIn, float strength);
    void dropBall();  // saque: suelta la pelota con la mano izquierda
    void splitStep() { splitT_ = 0; }  // saltito de preparación cuando golpea el rival
    void setPlanted(bool on) { planted_ = on; }  // apuntando: quieto, agachado
    enum Mood { MOOD_NONE, MOOD_HAPPY, MOOD_SAD };
    void setMood(Mood m) { mood_ = m; moodT_ = 0; }
    bool swinging() const { return swingT_ >= 0; }
    bool beforeContact() const { return swingT_ >= 0 && swingT_ < contactAt_; }
    // Dónde se coloca la pelota al soltarla para sacar (a la derecha, por delante)
    Vec3 dropPoint() const;
    Vec3 palaTip() const { return tip_; }    // punta de la pala en el mundo (para la estela)
    Vec3 palaCenter() const { return mid_; }  // centro de la hoja

private:
    void computePose(CharacterPose& pose, Xf& palaXf);
    Appearance look_;
    float time_ = 0, run_ = 0, runPhase_ = 0, yaw_ = PI;
    int side_ = 1;
    float height_ = 1.0f, prep_ = 0, prepTarget_ = 0, plant_ = 0;
    bool planted_ = false;
    float swingT_ = -1, contactAt_ = 0.08f, strength_ = 0.5f;  // swingT_ < 0 = no está golpeando
    float theta_ = 0.9f, thetaFrom_ = 0.9f;
    float dropT_ = -1;
    Mood mood_ = MOOD_NONE;
    float moodT_ = 0;
    float hop_ = 0, blinkT_ = 2, splitT_ = -1;
    Vec3 tip_, mid_;
};
