#pragma once
// Luz de los Miis como en Wii Sports Resort. EGG::LightTexture pinta cada fotograma dos lightmaps (difusa lm0 y brillos
// lm1) que los modelos leen con la normal en cámara (como una matcap); aquí se calculan en la CPU con las luces de la
// bolera (tuning::MII_LIGHTS). El material de cuerpo y cabeza hace: albedo × (lm0·oclusión + ambiente) + lm1·oclusión.
#include <gccore.h>
#include "engine/gfx.h"

namespace miilight {

void init();

// Deja lm0 en GX_TEXMAP1, lm1 en GX_TEXMAP2, la coordenada GX_TEXCOORD1 (normal → lightmap) y el canal COLOR0A0 con el
// ambiente en el color y, en el alfa, el del vértice (oclusión) o 1 si el modelo no lo trae. Recalcula las lightmaps si
// la cámara giró.
void bind(bool vertexAlpha);

// Carga la matriz del modelo (como gfx::loadModelMtx) y la de su normal para la coordenada de la lightmap
void loadModelMtx(const Xf& model, float normalScale = 1.0f);

}  // namespace miilight
