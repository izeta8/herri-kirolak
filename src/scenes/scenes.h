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
Scene* makePalaScene(const MatchConfig& cfg);
Scene* makeWhipTestScene();  // pala: prueba del latigazo
Scene* makePelotariShowcase();  // prueba: animaciones del pelotari de cerca

// Escenario compartido (se construye una vez)
World& sharedWorld();
