#pragma once
// Frontón provisional de la pala (más adelante, el de Zarautz). Medidas en engine/tuning.h (sección Palas).
//  - frontis (z = 0) de bloques de piedra con la chapa y la línea de arriba; pared izquierda (x = -COURT_W/2) con las
//    rayas de los cuadros (falta y pasa en rojo); suelo con sombra suave junto a las paredes; rebote al fondo;
//  - grada en el lado abierto con público Mii que aplaude y salta; fondo de casas y montes.
#include "engine/mesh.h"
#include "game/character.h"

class Fronton {
public:
    void build();
    void update(float dt);
    void draw();
    // 2D, después de la escena: números de los cuadros en la pared izquierda
    void drawLabels();
    void cheer(float seconds) { cheerT_ = fmaxf(cheerT_, seconds); }
    void clap(float seconds) { clapT_ = fmaxf(clapT_, seconds); }

    static constexpr int MAX_CROWD = 18;

private:
    struct Fan {
        Vec3 pos;
        float phase, jump, jumpV;
        Appearance look;
    };
    Mesh mesh_, far_;
    Fan fans_[MAX_CROWD];
    int numFans_ = 0;
    float time_ = 0, cheerT_ = 0, clapT_ = 0;
};
