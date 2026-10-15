#pragma once
// Escenario de aizkolaritza: el probaleku de Usurbil (empedrado, gradas con toldo, iglesia de San Salvador,
// frontón cubierto y casas de Etxebeste e Irazu) con su público.
#include "engine/mesh.h"
#include "game/character.h"

class World {
public:
    void build(unsigned seed = 7);
    void update(float dt);
    void drawSky();                  // 2D: llamar antes de begin3D
    void draw();                     // 3D: escenario + público
    // Con la cámara lejos (competición), ocultar el público entre la cámara y la plaza
    void setCameraHide(const Vec3& camEye, bool on) { hideCam_ = camEye; hideOn_ = on; }
    void cheer(float seconds);       // el público salta y aplaude
    void clapLight(float seconds);   // aplauso suave (solo manos)
    void drawShadow(const Vec3& pos, float rx, float rz, float alpha = 1.0f);

    static constexpr Color HORIZON = rgba(0xC8, 0xE2, 0xF2);
    static constexpr int MAX_SPECTATORS = 32;

private:
    struct Spectator {
        Vec3 pos;
        float yaw, phase, jump, jumpV;
        Appearance look;
        CharacterModel* model;
    };
    Mesh scenery_, clouds_, shadow_;
    Spectator spectators_[MAX_SPECTATORS];
    int numSpectators_ = 0;
    float time_ = 0, cheerT_ = 0, clapT_ = 0;
    Vec3 hideCam_;
    bool hideOn_ = false;
    bool hidden(const Spectator& s) const;
};
