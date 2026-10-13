#pragma once
// Utilidades compartidas por las escenas de menú
#include "game/character.h"

// Dibuja un personaje de pie en 3D, girando, para las pantallas de menú.
// offsetX desplaza el personaje en pantalla (unidades de mundo, + = derecha).
void drawCharacterPreview(const Appearance& look, float offsetX, float yaw, float time, float wave = 0);
