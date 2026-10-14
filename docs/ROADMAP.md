# Roadmap

Tareas medianas, en orden. Marcar `[x]` al terminar (con commit + push).

## Fase 0 — Entorno
- [x] devkitPro (wii-dev) instalado en `C:\devkitPro`, variables `DEVKITPRO`/`DEVKITPPC` configuradas
- [x] Dolphin portable en `D:\Tools\Dolphin\Dolphin-x64`
- [x] Repo público `izeta8/herri-kirolak` creado y con push
- [x] Scripts: build, run (Dolphin), screenshot, keys (teclas en Dolphin), install-usb, deploy (wiiload)

## Fase 1 — Hola mundo
- [x] Makefile libogc que genera `boot.dol`; arranca en Dolphin y muestra un cubo 3D con GX
- [x] Probado en Wii real vía Homebrew Channel desde USB
- [x] Botón HOME vuelve al Homebrew Channel

## Fase 2 — Motor base
- [x] Bucle principal, gestor de escenas con fundidos, diálogo de HOME, reset/power
- [x] Render: cámara, 2 luces + ambiente, niebla, mallas en display lists, colores planos, 16:9/4:3
- [x] Geometría procedural (esferas, cilindros, cápsulas, cajas) — de momento no hace falta Blender
- [ ] Pipeline de modelos Blender → OBJ → formato binario (cuando haga falta algo más complejo)
- [x] Fuente redondeada Nunito (OFL) con contorno, caracteres euskera (ñ, tildes)
- [x] Entrada: WPAD 4 mandos, gravedad filtrada, detector de hachazo (fuerza + inclinación de muñeca)
- [x] Audio: ASND con efectos sintetizados (tools/gen_sfx.py)
- [x] UI: botones estilo Wii navegables con cruceta, barra de ayuda, paneles
- [x] Guardado en USB/SD (libfat, carpeta de argv[0])

## Fase 3 — Personajes
- [x] Personaje "tipo Mii" (cabeza con cara, cuerpo, manos flotantes, txapela, gerriko)
- [x] Animaciones procedurales: reposo, levantar hacha, golpe, darse la vuelta, celebración
- [x] Editor de jugadores (nombre con teclado en pantalla, color, piel, pelo, txapela)

## Fase 4 — Aizkolaritza (MVP)
- [x] Escenario: plaza, tronco, público que aplaude y salta, baserris, ikurriña
- [x] Tronco deformable con corte en V (profundidad por lado), astillas como partículas
- [x] Mecánica de golpe (fuerza, lado ezker/eskuin, calidad, cansancio)
- [x] "Buelta eman!" y segunda V, el tronco se parte
- [x] Modo Entrenamendua (con consejos)
- [x] Modo Lehiaketa 1–4 jugadores A LA VEZ (cada uno con su mando y su tronco) + CPU + resultados + récords
- [x] Ajuste fino del gesto con mandos reales (telemetría + grabación guiada + replay_detector)
- [x] Gesto definido: pulgar en 1-2, subir por un lado ("JO!"), bajar en diagonal sin girar la muñeca
- [x] Golpe resuelto en el pico del gesto; hacha que entra en la madera a la vez
- [x] Sensación de impacto: pausa breve, temblor según la fuerza, astillas según la calidad
- [x] La fuerza cuenta (medidor de fuerza)
- [x] Actualización en caliente por WiFi (scripts/deploy.ps1)
- [ ] Probar la competición con 2-4 mandos reales
- [x] Icono del Homebrew Channel (`hbc/herri-kirolak/icon.png`, 128×48)

## Mejoras posibles (Aizkolaritza)
- [x] Pantalla "Kolpe proba" para ver fuerza e inclinación en vivo (Jokatu → Aizkolaritza → Kolpe proba)
- [x] Varios jugadores a la vez (en fila, cámara compartida)
- [ ] Troncos de distinto grosor
- [ ] Rival fantasma (mejor tiempo)

## Harness y estructura
- [x] Documentación para IA: CLAUDE.md índice + docs/ARQUITECTURA, NUEVO-JUEGO, GESTOS, DECISIONES, WII-NOTAS, HARNESS
- [x] Skills del proyecto (.claude/skills): probar-en-dolphin, mandar-a-wii, calibrar-gesto, nuevo-juego
- [x] Registro de deportes (`src/game/sports.cpp`) y `MatchConfig` común
- [ ] Récords por deporte en save.dat (ahora son de aizkolaritza)

## Estilo Wii (rama `estilo-wii-sports-resort`, pendiente de que el usuario lo vea en la Wii)
- [x] UI y menús estilo Wii Sports Resort; fondo de menús = escenario 3D
- [x] Aizkolaritza en el probaleku de Usurbil (iglesia, frontón, gradas, porterías, casas, empedrado, muros de cantos)
- [x] Miis calcados a los de la Wii + peinado en el editor (Orrazkera); escena 9 = galería de Miis — **descartado**:
  el usuario quiere las piezas oficiales, no imitaciones
- [x] Módulo `mii/`: piezas oficiales leídas de la NAND, montaje de la cabeza y la cara (compila; **sin probar**)
- [ ] Encender la Wii: `deploy.ps1` y `pull-miis.ps1`; probar y corregir los Miis en Dolphin y en la Wii
- [x] Cada jugador elige uno de sus Miis del Canal Mii (Jokalariak: Mii + txapela); al arrancar se enlazan solos los
  perfiles sin Mii; público con Miis de la consola (compila; **sin probar**)
- [ ] Cuerpo de los Miis igual que en Wii Sports Resort (proporciones según altura y complexión del Mii)
- [ ] Revisar con el usuario en la Wii: parecido de Miis y plaza, y fluidez (hay muchos más polígonos)
- [ ] Marcador de la partida al estilo Resort
- [ ] Más caras a elegir (ojos, cejas, bocas, narices) como en el editor de Miis
- [ ] Juntar con `main` cuando lo apruebe

## Próximo
- [ ] **Puntero IR en los menús** (además de la cruceta)
- [ ] Probar la lehiaketa con 2-4 mandos reales
- [ ] Siguiente deporte (skill `nuevo-juego`)

## Más adelante
- [ ] Cansancio bien diseñado (desactivado ahora)
- [ ] Música
- [ ] Final de tronco más espectacular, público que reacciona a cada golpe
- [ ] Miis reales
- [ ] Música
- [ ] Siguientes juegos (ver SPEC §6)
