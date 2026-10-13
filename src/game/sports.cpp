#include "game/sports.h"
#include "game/text.h"
#include "scenes/scenes.h"

// Orden = orden en el menú. Los que tienen makeMatch = nullptr salen como "Laster!" (próximamente).
static const Sport SPORTS[] = {
    {txt::AIZKOLARITZA, txt::AIZKO_DESC, txt::HOW_MANY_LOGS, 3, makeAizkoScene, makeSwingTestScene},
    {txt::HARRI, txt::SOON, nullptr, 3, nullptr, nullptr},
    {txt::SOKATIRA, txt::SOON, nullptr, 3, nullptr, nullptr},
    {txt::TRONTZA, txt::SOON, nullptr, 3, nullptr, nullptr},
};

int sportCount() { return (int)(sizeof(SPORTS) / sizeof(SPORTS[0])); }
const Sport& sport(int index) { return SPORTS[index < 0 || index >= sportCount() ? 0 : index]; }
