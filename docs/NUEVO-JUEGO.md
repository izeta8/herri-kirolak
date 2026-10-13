# Cómo crear un deporte nuevo

Objetivo: que cada deporte sea una carpeta y una línea en el registro, reutilizando motor, personajes, plaza y UI.
Referencia completa: Aizkolaritza (`src/games/aizkolaritza/`).

## 0. Antes de programar (con el usuario)
1. **El deporte real**: cómo se hace, cómo se gana, qué hace el cuerpo. Escribirlo en `docs/SPEC.md` (nueva sección).
2. **El gesto con el Wii Remote** (lo más importante). Diseñarlo con `docs/GESTOS.md`:
   - qué mide el acelerómetro y qué no (no sabe la posición de la mano; solo orientación cuando está quieto y
     aceleración cuando se mueve fuerte),
   - definir **agarre + fases** del gesto por escrito, igual que el hachazo, y apuntarlo en `docs/DECISIONES.md`.
3. **Modos**: Entrenamendua (1 jugador, con tarjeta "Nola jo") y Lehiaketa (1–4 **a la vez**, cada uno con su
   mando, puestos en fila o enfrentados según el deporte). Qué son las "rondas" del deporte.

## 1. Código
```
src/games/<nombre>/
  <objeto>.h/.cpp          lo que se manipula (piedra, cuerda, sierra…) con su malla y física
  <personaje>.h/.cpp       rig de animación encima de game/character (como Aizkolari)
  <nombre>_scene.cpp       la partida: struct Player por jugador, estados INTRO→COUNTDOWN→PLAY→FINISH→RESULTS
```
1. Añadir la carpeta a `SOURCES` en el `Makefile`.
2. Declarar la fábrica en `src/scenes/scenes.h`: `Scene* make<Nombre>Scene(const MatchConfig& cfg);`
3. Registrar el deporte en `src/game/sports.cpp` (sustituye su entrada "Laster!"):
   `{txt::NOMBRE, txt::NOMBRE_DESC, txt::NOMBRE_ROUNDS_Q, 3, make<Nombre>Scene, make<Gesto>TestScene}`.
4. Textos en euskera en `src/game/text.h`.
5. Constantes ajustables en `src/engine/tuning.h` (sección del deporte).
6. **Gesto**: si es distinto del hachazo, añadir un detector propio en `src/engine/input.cpp` (otra máquina de estados,
   con sus eventos en `Pad`) y su réplica en `tools/replay_detector.py` (o un `replay_<gesto>.py`).
7. Récords: `SaveData.records` es todavía de aizkolaritza; ampliar a récords por deporte con una versión nueva de
   `save.dat` (subir `VERSION` y migrar la anterior en `save.cpp`).

## 2. Plantilla de la escena (resumen de aizko_scene.cpp)
- `Player { pad, bot, prof, <objetos>, estado propio, timer, rank, … }` y `Player pl_[MAX_PLAYERS]`.
- Entrenamiento: cámara cercana; Lehiaketa: cámara que encuadra a todos + `sharedWorld().setCameraHide(eye, true)`.
- HUD: entrenamiento con indicadores grandes; lehiaketa con un panel por jugador abajo y avisos sobre cada cabeza
  (`gfx::project`). Nada de textos que roten (ver DECISIONES).
- Pausa con +, resultados con "Berriro jokatu / Menura itzuli", récord con `save::write()`.
- CPU: un `updateBot()` que haga el gesto "de mentira" con tiempos humanos.
- Sensación: pausa de impacto, temblor según la fuerza, partículas según la calidad, vibración del mando.

## 3. Pruebas (ver `docs/HARNESS.md`)
1. Añadir valores a `START_SCENE` en `main.cpp` para arrancar directo en la partida (con CPU) y en la prueba del gesto.
2. Dolphin: compilar con `scripts/build.ps1 -Test -Scene N`, `run.ps1`, `screenshot.ps1` y revisar la captura.
3. Wii real: `scripts/deploy.ps1` (en caliente) y grabación guiada en la pantalla de prueba del gesto → calibrar con
   datos (`docs/GESTOS.md`) → comprobar con la réplica → mandar.

## Checklist de "deporte terminado"
- [ ] Gesto definido por escrito y calibrado con grabaciones reales (aciertos medidos con la réplica)
- [ ] Entrenamendua con tarjeta "Nola jo"
- [ ] Lehiaketa 1–4 a la vez + CPU + resultados + récord
- [ ] Probado en Wii real (con varios mandos en lehiaketa)
- [ ] SPEC, ROADMAP, DECISIONES y GESTOS actualizados; capturas en README
