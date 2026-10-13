#pragma once
// Actualización en caliente por WiFi: el juego escucha en el puerto TCP 4301; si recibe un DOL
// nuevo (scripts/deploy.ps1), apaga todo y salta a él sin pasar por el Homebrew Channel.
// Protocolo: "HKUP" + tamaño (u32 big-endian) + DOL. La Wii contesta "OK\n" al recibirlo entero.
//
// En la Wii real varias llamadas de red bloqueantes en hilos distintos se bloquean entre sí, así que
// todo va con sockets no bloqueantes desde el único hilo de red (telemetry.cpp llama a poll()).

namespace updater {

constexpr int PORT = 4301;

void open();   // abre el puerto (desde el hilo de red, con la red ya iniciada)
void poll();   // atiende conexiones y datos sin bloquear
bool ready();  // hay un DOL nuevo recibido
void launch(); // no vuelve

}  // namespace updater
