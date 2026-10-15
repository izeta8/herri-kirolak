#pragma once
// Frontón provisional del prototipo de pala: frontis con chapa, pared izquierda con las rayas de los cuadros,
// cancha con las líneas de falta y pasa, contracancha y una grada sencilla en el lado abierto.
// (Más adelante, el frontón de Zarautz.) Medidas en engine/tuning.h.
#include "engine/mesh.h"

class Fronton {
public:
    void build();
    void draw();

private:
    Mesh mesh_;
};
