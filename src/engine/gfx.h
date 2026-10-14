#pragma once
// Capa gráfica sobre GX: vídeo, cámara 3D con luces, y primitivas 2D para la UI.
#include <gccore.h>
#include "engine/math.h"

class Mesh;

// Transformación afín (envoltorio de Mtx de libogc). Se encadena como en OpenGL:
// Xf().translate(p).rotateY(a) → primero rota, luego traslada.
struct Xf {
    Mtx m;
    Xf();
    Xf& translate(const Vec3& t);
    Xf& rotateX(float rad);
    Xf& rotateY(float rad);
    Xf& rotateZ(float rad);
    Xf& rotateAxis(const Vec3& axis, float rad);
    Xf& scale(const Vec3& s);
    Xf& scale(float s) { return scale(Vec3(s, s, s)); }
    Xf operator*(const Xf& o) const;
    // Matriz a partir de una base (columnas X, Y, Z) y un origen
    static Xf fromBasis(const Vec3& origin, const Vec3& x, const Vec3& y, const Vec3& z);
    Vec3 apply(const Vec3& p) const;
    Vec3 applyDir(const Vec3& d) const;
};

struct Camera {
    Vec3 eye{0, 2, 6};
    Vec3 target{0, 0, 0};
    Vec3 up{0, 1, 0};
    float fov = 45.0f;
    float znear = 0.1f;
    float zfar = 400.0f;
};

namespace gfx {

constexpr float SCREEN_H = 480.0f;

void init();
bool widescreen();
float screenW();  // ancho virtual 2D: 854 en 16:9, 640 en 4:3
bool isPAL50();

void beginFrame();
void endFrame();

// ---------- 3D ----------
void begin3D(const Camera& cam);
void setLighting(bool lit);
void setDepth(bool test, bool write);
void setBlend(bool on);
void setCull(bool on);
void setFog(bool on, float start = 0, float end = 0, Color c = 0);
void drawMesh(const Mesh& mesh, const Xf& model);
// Para módulos con su propio formato de vértices (los Miis): carga la matriz del modelo (posición y normales) en
// PNMTX0, y después restore3D() deja otra vez el estado que esperan drawMesh y el resto del motor.
void loadModelMtx(const Xf& model, float normalScale = 1.0f);  // normalScale deshace una escala uniforme en las normales
void restore3D();

// Dibujo fuera de pantalla para generar texturas (p. ej. la cara de un Mii). Solo entre frames: se hace al empezar
// el frame, mediante el callback de inicio de frame. Usa la esquina (0,0)-(size,size) del EFB con alfa.
void setFrameStartCallback(void (*cb)());
void offscreenBegin(u16 size);                       // EFB con alfa, borrado a (0,0,0,0), proyección 1 unidad = 1 píxel
void offscreenCopy(void* dst, u16 size, u8 texFmt);  // copia la esquina a una textura y la borra
void offscreenEnd();                                 // vuelve el formato del EFB, el color de borrado y el viewport
// Proyecta un punto del mundo a coordenadas 2D virtuales. Devuelve false si queda detrás de la cámara.
bool project(const Vec3& p, float& sx, float& sy);
void use3DFormat();  // deja el formato de vértice 3D activo (para construir display lists)

// ---------- 2D ----------
void begin2D();
// Borra el Z-buffer (para dibujar 3D encima de una escena 3D ya pintada, p. ej. el fondo de los menús)
void clearDepth();
void rect(float x, float y, float w, float h, Color c);
void rectGrad(float x, float y, float w, float h, Color top, Color bottom);
void roundRect(float x, float y, float w, float h, float r, Color c);
void roundRectGrad(float x, float y, float w, float h, float r, Color top, Color bottom);
void circle(float cx, float cy, float r, Color c, int segs = 32);
void ring(float cx, float cy, float r0, float r1, Color c, int segs = 32);
// Arco de anillo entre los ángulos a0 y a1 (radianes, 0 = derecha, sentido horario en pantalla)
void arc(float cx, float cy, float r0, float r1, float a0, float a1, Color c, int segs = 16);
void triangle(float x0, float y0, float x1, float y1, float x2, float y2, Color c);

// Quads texturizados para el texto (uso interno de Font)
// outlined=false: la letra sin contorno (alfa = relleno)
void beginTextured(GXTexObj* tex, Color outline, bool outlined = true);
void texQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, Color c);
void endTextured();

}  // namespace gfx
