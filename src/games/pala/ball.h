#pragma once
// La pelota de pala: física con botes en el suelo, el frontis, la pared izquierda y el rebote, y el estado de la
// jugada que hace falta para saber si se le puede dar (ya ha tocado el frontis, cuántos botes lleva).
// Las reglas (quién pierde el tanto) las decide la escena con los eventos que devuelve step().
#include "engine/math.h"

enum BallEventType { BE_FLOOR, BE_FRONTIS, BE_WALL, BE_BACK };

struct BallEvent {
    BallEventType type;
    Vec3 at;
    bool first;   // frontis: es el que esperaba el último golpe
    int bounces;  // suelo: botes que lleva con este (desde el frontis o desde que se botó para sacar)
    bool before;  // suelo: ha botado antes de llegar al frontis
};

struct Ball {
    Vec3 pos, vel;
    bool live = false;         // en juego (si no, no se dibuja)
    bool needFrontis = false;  // el último golpe todavía no ha llegado al frontis
    int bounces = 0;           // botes desde el frontis (o desde que se bota para sacar)
    bool drop = false;         // saque: el pelotari la ha botado y tiene que darle tras el bote
    bool rolling = false;      // ya no bota: rueda por el suelo (empezar a rodar cuenta como un bote)

    // Avanza dt segundos; apunta los eventos (como mucho maxEv) y devuelve cuántos hubo
    int step(float dt, BallEvent* ev, int maxEv);
    // ¿Se le puede dar ahora? (ha tocado el frontis y no lleva dos botes; en el saque, tras el primer bote)
    bool hittable() const { return live && !needFrontis && bounces <= 1 && (!drop || bounces == 1); }
};

// Trayectoria prevista (para el golpe, la CPU y la marca de bote)
struct BallPath {
    static constexpr int MAX = 300;
    static constexpr float DT = 0.01f;
    Vec3 p[MAX];
    bool ok[MAX];  // en ese instante se le puede dar
    unsigned char bnc[MAX];  // botes desde el frontis en ese instante
    float sinceB[MAX];       // s desde el último bote (o desde el frontis si aún no ha botado)
    int n = 0;
    int landing = -1;  // primer bote tras el frontis (índice), -1 si no lo hay
    void predict(const Ball& b, float seconds, float sinceBounce = 1.0f);
};
