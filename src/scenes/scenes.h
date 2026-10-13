#pragma once
#include "engine/scene.h"

class World;

struct AizkoConfig {
    bool training = true;
    int numPlayers = 1;
    int numLogs = 1;
    int profile[4] = {0, 1, 2, 3};
    bool bot[4] = {false, false, false, false};
};

Scene* makeTitleScene();
Scene* makeMainMenu(int sel = 0);
Scene* makeSetupScene();
Scene* makeProfilesScene();
Scene* makeRecordsScene();
Scene* makeSwingTestScene();
Scene* makeAizkoScene(const AizkoConfig& cfg);

// Escenario compartido (se construye una vez)
World& sharedWorld();
