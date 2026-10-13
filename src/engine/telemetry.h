#pragma once
// Telemetría por WiFi: la Wii abre el puerto TCP 4300 y envía todas las lecturas del mando
// a quien se conecte (tools/telemetry.py en el PC). Sirve para afinar el gesto del hachazo.
#include "engine/input.h"

namespace telemetry {

constexpr int PORT = 4300;

void init();  // arranca un hilo: inicia la red y espera conexiones (no bloquea el juego)
void push(int chan, const input::Sample& s);
// 0 = sin red, 1 = red lista (esperando PC), 2 = PC conectado
int status();
const char* ip();  // IP de la Wii ("" si no hay red)
s32 netListen(u16 port);     // socket de escucha con reintentos (-1 si falla)
void setNonBlocking(s32 sock);
void stop();                 // cierra los sockets (antes de saltar a otra versión)

}  // namespace telemetry
