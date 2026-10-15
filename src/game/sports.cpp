#include "game/sports.h"
#include "engine/math.h"
#include "game/text.h"
#include "scenes/scenes.h"

static const int PALA_POINTS[3] = {10, 15, 22};

// Orden = orden en el menú. Los que tienen makeMatch = nullptr salen como "Laster!" (próximamente).
static const Sport SPORTS[] = {
    {txt::AIZKOLARITZA, txt::AIZKO_DESC, txt::HOW_MANY_LOGS, 3, makeAizkoScene, makeSwingTestScene},
    {txt::PALA, txt::PALA_DESC, txt::HOW_MANY_POINTS, 3, makePalaScene, nullptr, 2, PALA_POINTS, txt::CHOOSE_PELOTARI},
    {txt::HARRI, txt::SOON, nullptr, 3, nullptr, nullptr},
    {txt::SOKATIRA, txt::SOON, nullptr, 3, nullptr, nullptr},
    {txt::TRONTZA, txt::SOON, nullptr, 3, nullptr, nullptr},
};

int sportCount() { return (int)(sizeof(SPORTS) / sizeof(SPORTS[0])); }
int palaPoints(int rounds) { return PALA_POINTS[clampv(rounds, 1, 3) - 1]; }
const Sport& sport(int index) { return SPORTS[index < 0 || index >= sportCount() ? 0 : index]; }
