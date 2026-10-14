#pragma once
// "Nola jokatu": tutorial animado de aizkolaritza al estilo de las pantallas de instrucciones de Wii Sports Resort.
// Un Wii Remote en 3D hace cada movimiento (con estela, "JO!", golpe y astillas) mientras un tronco dibujado se va
// abriendo en V. A = siguiente paso (en el último, empezar), B = paso anterior. Sin tocar nada, avanza solo.
#include "engine/mesh.h"

class HowTo {
public:
    void start();
    bool update(float dt, unsigned down);  // true cuando el jugador termina el tutorial
    void draw();                           // encima de la escena 3D ya dibujada

    struct Pose {
        float x, y, roll, pitch, yaw;
    };

private:
    void setStep(int s);
    void stepPose(Pose& pose, float u, float uPrev);
    void drawLog2D(float cx, float cy);

    int step_ = 0;
    float t_ = 0, total_ = 0, prevU_ = 0;
    int loops_ = 0;
    bool touched_ = false;
    // Estado de la animación del paso actual
    Pose pose_ = {0, 0, 0, 0, 0};
    float depthL_ = 0, depthR_ = 0;  // profundidad de la V (0..1) en el tronco dibujado
    float shake_ = 0, jo_ = 0, hitText_ = 0, gauge_ = 0, flip_ = 0;
    int lastSide_ = -1;
    struct Chip { float x, y, vx, vy, life; };
    Chip chips_[24];
    float trailX_[16], trailY_[16];
    int trailN_ = 0;
    bool swinging_ = false;
    float b12X_ = -1, b12Y_ = -1;  // dónde quedaron los botones 1 y 2 en pantalla (para dibujar la mano)
};
