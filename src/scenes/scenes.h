#pragma once
#include "engine/scene.h"
#include "game/sports.h"

class World;

Scene* makeTitleScene();
Scene* makeMainMenu(int sel = 0);
Scene* makeSetupScene();
Scene* makeProfilesScene();
Scene* makeRecordsScene();
Scene* makeSwingTestScene();
Scene* makeMiiGalleryScene();  // prueba: Miis de cerca
Scene* makeAizkoScene(const MatchConfig& cfg);

// Escenario compartido (se construye una vez)
World& sharedWorld();
