#pragma once
// Utilidades compartidas por las escenas de menú
#include "game/character.h"

// Dibuja un personaje de pie en 3D, girando, para las pantallas de menú.
// offsetX desplaza el personaje en pantalla (unidades de mundo, + = derecha).
void drawCharacterPreview(const Appearance& look, float offsetX, float yaw, float time, float wave = 0);

// Perfiles enlazados a Miis de la consola: setProfileMii pone el Mii n.º `index` (nombre, color y piel para los
// menús); linkProfilesToMiis enlaza al arrancar los perfiles que aún no tienen Mii (y guarda).
void setProfileMii(Profile& p, int index);
void linkProfilesToMiis();

// Fondo de los menús: el escenario en 3D (se registra con ui::setBackground)
void drawMenuBackdrop(float t);

// En partida: el número (1..4) del Wii Remote que le falta a algún humano, o 0 si están todos. El jugador i juega con
// el mando i; con un solo humano vale cualquier mando conectado (como en el resto del juego).
int missingRemote(const bool* human, int n);
// Aviso de mando que falta, encima de la partida parada (con un Wii Remote dibujado y el mensaje)
void drawConnectOverlay(const char* title, const char* sub);
