#include "game/remote3d.h"
#include "game/palette.h"

namespace remote3d {

static Mesh s_remote;

const Mesh& mesh() {
    if (s_remote.valid()) return s_remote;
    MeshBuilder b;
    const Color body = rgba(0xF6, 0xF6, 0xF4), grey = rgba(0xDC, 0xDE, 0xE2), dark = rgba(0x5A, 0x5E, 0x66);
    const float r = 0.008f, hx = RW / 2, hz = RD / 2;
    // Cuerpo con las aristas largas redondeadas
    b.box(Vec3(0, 0, 0), Vec3(hx - r, RL / 2, hz), body);
    b.box(Vec3(0, 0, 0), Vec3(hx, RL / 2, hz - r), body);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            b.cylinder(Vec3(sx * (hx - r), -RL / 2, sz * (hz - r)), r, r, RL, body, body, 10, true);
    // Sensor negro en la punta
    b.box(Vec3(0, RL / 2 + 0.0008f, 0), Vec3(hx - 0.002f, 0.0012f, hz - 0.002f), rgba(0x1C, 0x1C, 0x22));
    Xf saved = b.xf;
    b.xf = Xf().rotateX(PI / 2);  // discos y cilindros (eje +Y) pasan a mirar a +Z, la cara de los botones
    auto button = [&](float y, float rad, float h, Color c) {
        // rotateX(90°) lleva (x, y, z) a (x, −z, y): para que quede en (0, y, hz) hay que construir en (0, hz, −y)
        b.cylinder(Vec3(0, hz - 0.0005f, -y), rad, rad * 0.92f, h, c, shade(c, 1.04f), 16, true);
    };
    button(RL / 2 - 0.012f, 0.003f, 0.0012f, rgba(0xB8, 0x30, 0x30));  // encendido
    button(RL / 2 - 0.062f, 0.0078f, 0.0025f, grey);                    // A
    button(-0.012f, 0.0028f, 0.0012f, grey);                            // HOME
    button(-0.046f, 0.0056f, 0.002f, grey);                             // 1
    button(-0.062f, 0.0056f, 0.002f, grey);                             // 2
    b.xf = saved;
    // Cruceta
    b.box(Vec3(0, RL / 2 - 0.032f, hz + 0.001f), Vec3(0.011f, 0.0035f, 0.0015f), dark);
    b.box(Vec3(0, RL / 2 - 0.032f, hz + 0.001f), Vec3(0.0035f, 0.011f, 0.0015f), dark);
    // − y +, y las luces de jugador abajo
    b.box(Vec3(-0.011f, -0.012f, hz + 0.0006f), Vec3(0.0028f, 0.0028f, 0.001f), grey);
    b.box(Vec3(0.011f, -0.012f, hz + 0.0006f), Vec3(0.0028f, 0.0028f, 0.001f), grey);
    for (int i = 0; i < 4; i++)
        b.box(Vec3(-0.0105f + i * 0.007f, -RL / 2 + 0.011f, hz + 0.0004f), Vec3(0.0018f, 0.0012f, 0.0006f),
              i == 0 ? rgba(0x40, 0x9C, 0xF0) : rgba(0xB0, 0xB4, 0xBC));
    // Gatillo B por detrás
    b.box(Vec3(0, RL / 2 - 0.05f, -hz - 0.003f), Vec3(0.0085f, 0.012f, 0.0035f), grey);
    b.build(s_remote);
    return s_remote;
}

}  // namespace remote3d
