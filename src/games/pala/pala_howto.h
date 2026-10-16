#pragma once
// "Nola jokatu" de la pala: tutorial animado al estilo de Wii Sports Resort (como el de aizkolaritza). Cinco pasos:
// moverse con el Nunchuk, apuntar con Z (biparetas), el latigazo del Wii Remote, los tipos de golpe (B / nada / A) y
// el punto dulce. A = siguiente (en el último, empezar), B = anterior. Sin tocar nada, avanza solo.

class PalaHowTo {
public:
    void start();
    bool update(float dt, unsigned down);  // true cuando el jugador termina el tutorial
    void draw();                           // encima de la escena 3D ya dibujada

private:
    void setStep(int s);
    void drawNunchuk(float cx, float cy, float sx, float sy, bool zLit);
    void drawMove(float cx, float cy);
    void drawAim(float cx, float cy);
    void drawWhip(float cx, float cy);
    void drawShots(float cx, float cy);
    void drawSweet(float cx, float cy);

    int step_ = 0;
    float t_ = 0, total_ = 0, prevU_ = 0;
    int loops_ = 0;
    bool touched_ = false;
    // paso 1: el pelotari y la pelota en la cancha vista desde arriba
    float px_ = 0, py_ = 0, tx_ = 0, ty_ = 0;
    int target_ = 0;
    float trailX_[12], trailY_[12];
    int trailN_ = 0;
};
