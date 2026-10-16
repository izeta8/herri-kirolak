#pragma once
// Registro de deportes. Para añadir uno nuevo: crear src/games/<nombre>/ con su escena de partida y
// añadir una entrada en sports.cpp. Los menús (preparar partida) se construyen a partir de esta lista.
// Guía completa: docs/NUEVO-JUEGO.md
#include "engine/scene.h"

constexpr int MAX_PLAYERS = 4;

// Configuración de una partida, común a todos los deportes
struct MatchConfig {
    int sport = 0;           // índice en el registro
    bool training = true;    // entrenamiento (1 jugador) o lehiaketa
    int numPlayers = 1;      // lehiaketa: 1-4 a la vez, cada uno con su Wii Remote
    int rounds = 1;          // rondas: 1..maxRounds (en aizkolaritza: troncos; en pala, 10/15/22 tantos)
    int profile[MAX_PLAYERS] = {0, 1, 2, 3};
    bool bot[MAX_PLAYERS] = {false, false, false, false};
    int level = 1;           // nivel de la CPU: 0 erraza, 1 normala, 2 zaila (deportes con botLevels)
    bool demo = false;       // de fondo en el título: sin HUD, sin intro ni final, sin pausa
};

struct Sport {
    const char* name;            // nombre en euskera (menú)
    const char* desc;            // subtítulo en el menú de deportes
    const char* roundsQuestion;  // pregunta de rondas en la lehiaketa ("Zenbat enbor?")
    int maxRounds;               // 1..3
    Scene* (*makeMatch)(const MatchConfig&);  // nullptr = todavía no está ("Laster!")
    Scene* (*makeGestureTest)();              // pantalla de prueba del gesto (opcional)
    int maxPlayers = 4;                       // lehiaketa: jugadores humanos como mucho
    const int* roundValues = nullptr;         // lo que se enseña en cada opción de rondas (nullptr = 1, 2, 3)
    const char* chooseChar = nullptr;         // "Aukeratu aizkolaria" (nullptr = el de aizkolaritza)
    bool botLevels = false;                   // lehiaketa: preguntar el nivel de la CPU
    bool nunchuk = false;                     // se juega con Nunchuk (se avisa al elegir personaje si falta)
};

int sportCount();
const Sport& sport(int index);
int palaPoints(int rounds);  // tantos del partido de pala según MatchConfig::rounds
