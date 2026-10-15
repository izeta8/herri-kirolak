#include "games/pala/fronton.h"
#include "engine/tuning.h"

using namespace tuning;

static const Color WALL = rgba(0x4C, 0x9A, 0x6C);       // frontis y pared izquierda, verdes
static const Color WALL_TOP = rgba(0x3E, 0x84, 0x5A);
static const Color FLOOR = rgba(0x8E, 0xA4, 0x9A);      // cancha de cemento
static const Color FLOOR_OUT = rgba(0xB0, 0xB4, 0xB0);  // contracancha
static const Color LINE = rgba(0xF4, 0xF6, 0xF4);
static const Color LINE_RED = rgba(0xD8, 0x3A, 0x30);
static const Color CHAPA = rgba(0xB8, 0xC0, 0xC8);
static const Color STONE = rgba(0xC8, 0xBE, 0xAA);
static const Color SEAT = rgba(0x2C, 0x6C, 0xB8);

void Fronton::build() {
    if (mesh_.valid()) return;
    MeshBuilder b;
    const float XL = -COURT_W / 2, XR = COURT_W / 2;
    const float back = COURT_LEN + 4.0f;  // la cancha sigue un poco por detrás de la última raya
    const float out = XR + 5.0f;          // ancho de la contracancha

    // Suelo: cancha y contracancha
    b.box(Vec3(0, -0.05f, back / 2), Vec3(COURT_W / 2, 0.05f, back / 2), FLOOR);
    b.box(Vec3((XR + out) / 2, -0.06f, back / 2), Vec3((out - XR) / 2, 0.05f, back / 2), FLOOR_OUT);
    // Líneas del suelo: lado abierto, falta, pasa y las rayas de los cuadros
    const float lw = 0.06f, ly = 0.004f;
    b.box(Vec3(XR, ly, back / 2), Vec3(lw, ly, back / 2), LINE);
    for (float z = CUADRO; z < back; z += CUADRO) b.box(Vec3(XR - 0.25f, ly, z), Vec3(0.25f, ly, lw), LINE);
    b.box(Vec3(0, ly, FALTA_Z), Vec3(COURT_W / 2, ly, lw), LINE_RED);
    b.box(Vec3(0, ly, PASA_Z), Vec3(COURT_W / 2, ly, lw), LINE_RED);

    // Frontis: muro con la chapa abajo y la línea de arriba
    b.box(Vec3((XL + out) / 2 - 0.4f, WALL_H / 2, -0.4f), Vec3((out - XL) / 2 + 0.4f, WALL_H / 2, 0.4f), WALL);
    b.box(Vec3((XL + out) / 2 - 0.4f, WALL_H + 0.2f, -0.4f), Vec3((out - XL) / 2 + 0.5f, 0.2f, 0.5f), WALL_TOP);
    b.box(Vec3(0, CHAPA_H / 2, 0.02f), Vec3(COURT_W / 2, CHAPA_H / 2, 0.02f), CHAPA);
    b.box(Vec3(0, CHAPA_H, 0.045f), Vec3(COURT_W / 2, 0.035f, 0.01f), LINE);
    b.box(Vec3(0, TOP_LINE, 0.01f), Vec3(COURT_W / 2, 0.06f, 0.01f), LINE);
    b.box(Vec3(XR, WALL_H / 2, 0.01f), Vec3(lw, WALL_H / 2, 0.01f), LINE);  // raya del lado abierto en el frontis

    // Pared izquierda con las rayas de los cuadros (falta y pasa en rojo)
    b.box(Vec3(XL - 0.4f, WALL_H / 2, back / 2), Vec3(0.4f, WALL_H / 2, back / 2), WALL);
    b.box(Vec3(XL - 0.4f, WALL_H + 0.2f, back / 2), Vec3(0.5f, 0.2f, back / 2), WALL_TOP);
    for (float z = CUADRO; z < back; z += CUADRO)
        b.box(Vec3(XL + 0.005f, WALL_H / 2, z), Vec3(0.005f, WALL_H / 2, lw * 0.7f), LINE);
    b.box(Vec3(XL + 0.008f, WALL_H / 2, FALTA_Z), Vec3(0.006f, WALL_H / 2, lw), LINE_RED);
    b.box(Vec3(XL + 0.008f, WALL_H / 2, PASA_Z), Vec3(0.006f, WALL_H / 2, lw), LINE_RED);
    b.box(Vec3(XL + 0.005f, TOP_LINE, back / 2), Vec3(0.005f, 0.06f, back / 2), LINE);

    // Rebote (pared del fondo, baja)
    b.box(Vec3((XL + out) / 2, 1.5f, back + 0.3f), Vec3((out - XL) / 2 + 0.4f, 1.5f, 0.3f), STONE);

    // Grada del lado abierto: escalones de piedra con asientos
    for (int i = 0; i < 4; i++) {
        float x = out + 0.6f + i * 1.0f, h = 0.45f * (i + 1);
        b.box(Vec3(x, h / 2, back / 2 + 1), Vec3(0.5f, h / 2, back / 2 + 1), STONE);
        b.box(Vec3(x - 0.15f, h + 0.04f, back / 2 + 1), Vec3(0.3f, 0.04f, back / 2 - 1), SEAT);
    }
    b.box(Vec3(out + 4.6f, 2.5f, back / 2 + 1), Vec3(0.3f, 2.5f, back / 2 + 1), STONE);
    b.build(mesh_);
}

void Fronton::draw() { gfx::drawMesh(mesh_, Xf()); }
