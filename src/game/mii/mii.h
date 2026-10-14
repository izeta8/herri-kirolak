#pragma once
// Miis de verdad: las piezas (formas de cara, pelos, narices, texturas de ojos, cejas, bocas…) se leen de la propia
// Wii (/shared2/FaceLib/RFL_Res.dat) y los Miis del Canal Mii de /shared2/menu/FaceLib/RFL_DB.dat. Nada de eso va en
// el repositorio. La cabeza se monta igual que la librería de Miis de la Wii (RVLFaceLib); el cuerpo es del juego.
#include <vector>
#include "engine/gfx.h"

// Los mismos parámetros que guarda la Wii para cada Mii (sin empaquetar)
struct MiiInfo {
    u8 faceType = 0, faceColor = 0, faceTex = 0;
    u8 hairType = 33, hairColor = 1, hairFlip = 0;
    u8 eyeType = 2, eyeColor = 0, eyeScale = 4, eyeRotate = 4, eyeX = 2, eyeY = 12;
    u8 browType = 6, browColor = 1, browScale = 4, browRotate = 6, browX = 2, browY = 10;
    u8 noseType = 1, noseScale = 4, noseY = 9;
    u8 mouthType = 23, mouthColor = 0, mouthScale = 4, mouthY = 13;
    u8 mustache = 0, beardType = 0, beardColor = 0, beardScale = 4, beardY = 10;
    u8 glassType = 0, glassColor = 0, glassScale = 4, glassY = 10;
    u8 moleType = 0, moleScale = 4, moleX = 2, moleY = 20;
    u8 height = 64, build = 64;
    u8 favColor = 0;
    bool female = false;
    char name[11] = {0};  // en Latin-1
};

namespace mii {

bool init();       // carga las piezas y la base de datos de la NAND; false si esta Wii/Dolphin no las tiene
bool available();  // hay piezas: se pueden dibujar Miis de verdad
int userCount();   // Miis del Canal Mii
const MiiInfo& user(int i);
MiiInfo guest(int i);  // los 6 invitados de la Wii: 0-2 chicos, 3-5 chicas
MiiInfo random(u32 seed, bool female);
Color favoriteColor(int i);
Color skinColor(int faceColor);
void processPending();  // al empezar el frame: pinta las caras que falten (usa el EFB)

}  // namespace mii

// Cabeza de un Mii lista para dibujar. Origen en la barbilla, mirando a +Z, en metros (cara de ~0.43 m de ancho).
class MiiHead {
public:
    bool build(const MiiInfo& info, u16 maskSize);
    void release();
    bool ready() const { return built_ && masksDone_; }
    void draw(const Xf& model, bool blink) const;
    float top() const { return top_; }  // altura del punto más alto (pelo) sobre la barbilla, en metros

    // Interno (lo usa la cola de caras pendientes)
    void bakeMasks();

private:
    struct Part {
        s16* pos = nullptr;
        s16* nrm = nullptr;
        s16* txc = nullptr;
        u8* dl = nullptr;
        u32 dlSize = 0;
        u16 nPos = 0;
        bool hasTxc = false;
    };
    bool loadShape(Part& p, int arc, int file, bool transform, bool flipX, float scale, const float* trans, float* extra);
    bool loadTex(GXTexObj& obj, u8*& buf, int arc, int file);
    void releasePart(Part& p);

    MiiInfo info_;
    Part face_, mask_, hair_, cap_, forehead_, nose_, noseline_, beard_, glass_;
    GXTexObj faceTex_, capTex_, nlineTex_, glassTex_;
    u8 *faceTexBuf_ = nullptr, *capTexBuf_ = nullptr, *nlineTexBuf_ = nullptr, *glassTexBuf_ = nullptr;
    GXTexObj maskObj_[2];
    u8* maskBuf_[2] = {nullptr, nullptr};
    u16 maskSize_ = 128;
    float scale_ = 0.01f, top_ = 0.5f;
    float center_[3] = {0, 0, 0};
    bool built_ = false, masksDone_ = false;
};
