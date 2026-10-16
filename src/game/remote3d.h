#pragma once
// Wii Remote en 3D para los tutoriales "Nola jokatu" (estilo Wii Sports Resort). En metros: largo (Y, la punta con el
// sensor arriba), ancho (X) y grueso (Z, botones en +Z); el gatillo B por detrás (-Z).
#include "engine/mesh.h"

namespace remote3d {
constexpr float RL = 0.148f, RW = 0.036f, RD = 0.03f;
const Mesh& mesh();  // se construye la primera vez
}  // namespace remote3d
