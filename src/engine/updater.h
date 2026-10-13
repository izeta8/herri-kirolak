#pragma once
// Actualización en caliente por WiFi: el juego escucha en el puerto TCP 4301; si recibe un DOL
// nuevo (scripts/deploy.ps1), apaga todo y salta a él sin pasar por el Homebrew Channel.
// Protocolo: "HKUP" + tamaño (u32 big-endian) + DOL. La Wii contesta "OK\n" al recibirlo entero.

namespace updater {

constexpr int PORT = 4301;

void startServer();  // lo llama el hilo de red cuando la red está lista
bool ready();        // hay un DOL nuevo recibido
void launch();       // no vuelve

}  // namespace updater
