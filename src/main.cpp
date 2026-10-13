// Herri Kirolak — punto de entrada
#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <fat.h>
#include <stdlib.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/scene.h"
#include "scenes/scenes.h"

// START_SCENE (solo para pruebas): 0 título, 1 menú, 2 preparar partida, 3 jugadores, 4 récords,
// 5 entrenamiento, 6 lehiaketa CPU contra CPU, 7 entrenamiento con la CPU, 8 kolpe proba
#ifndef START_SCENE
#define START_SCENE 0
#endif

static volatile bool s_reset = false, s_power = false;
static void onReset(u32, void*) { s_reset = true; }
static void onPower() { s_power = true; }
static void onPadPower(s32) { s_power = true; }

static Scene* firstScene() {
    AizkoConfig cfg;
    switch (START_SCENE) {
        case 1: return makeMainMenu();
        case 2: return makeSetupScene();
        case 3: return makeProfilesScene();
        case 4: return makeRecordsScene();
        case 5: return makeAizkoScene(cfg);
        case 6:
            cfg.training = false;
            cfg.numPlayers = 2;
            cfg.numLogs = 1;
            cfg.bot[0] = cfg.bot[1] = true;
            return makeAizkoScene(cfg);
        case 8: return makeSwingTestScene();
        case 7:
            cfg.bot[0] = true;
            return makeAizkoScene(cfg);
        default: return makeTitleScene();
    }
}

int main(int argc, char** argv) {
    gfx::init();
    input::init();
    audio::init();
    font::init();
    save::init(argc > 0 ? argv[0] : nullptr);
    SYS_SetResetCallback(onReset);
    SYS_SetPowerCallback(onPower);
    WPAD_SetPowerButtonCallback(onPadPower);
    srand(gettime());

    sharedWorld();
    scenes::start(firstScene());

    u64 last = gettime();
    while (!s_reset && !s_power) {
        u64 now = gettime();
        float dt = clampv(diff_usec(last, now) / 1000000.0f, 1.0f / 120.0f, 1.0f / 20.0f);
        last = now;
        input::update(dt);
        if (!scenes::frame(dt)) break;
    }

    input::stopRumble();
    audio::stopAll();
    fatUnmount("sd:/");
    fatUnmount("usb:/");
    if (s_power) SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    return 0;  // vuelve al Homebrew Channel
}
