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
- [x] Récords por deporte en save.dat: v4 añade los de la pala al final (los de aizkolaritza siguen en `records`)

## Estilo Wii (rama `estilo-wii-sports-resort`, pendiente de que el usuario lo vea en la Wii)
- [x] UI y menús estilo Wii Sports Resort; fondo de menús = escenario 3D
- [x] Aizkolaritza en el probaleku de Usurbil (iglesia, frontón, gradas, porterías, casas, empedrado, muros de cantos)
- [x] Miis calcados a los de la Wii + peinado en el editor (Orrazkera); escena 9 = galería de Miis — **descartado**:
  el usuario quiere las piezas oficiales, no imitaciones
- [x] Módulo `mii/`: piezas oficiales y montaje de la cabeza y la cara; **probado en Dolphin** con los Miis del usuario
  (galería, escena 9) comparando con Wii Sports Resort: se parecen. Gafas en espejo y cara de 256 px para jugadores
- [x] `RFL_Res.dat` no está en la NAND de la Wii: sacado del disco de Wii Sports Resort del usuario y copiado a
  `F:pps\herri-kirolak\` (USB) y a la NAND de Dolphin; el juego lo lee del USB si no está en la NAND
- [ ] Ver los Miis en la Wii real (USB ya instalado) y comparar con Wii Sports Resort
- [ ] Revisar caras: algún Mii se ve con los ojos pixelados de cerca (escala grande del ojo)
- [x] Cada jugador elige uno de sus Miis del Canal Mii (Jokalariak: Mii + txapela); al arrancar se enlazan solos los
  perfiles sin Mii; público con Miis de la consola (compila; **sin probar**)
- [x] Cuerpo de Wii Sports Resort **sin brazos** (como en Resort): torso y piernas de `WS2_avaA1` sin los huesos del
  brazo, manos sueltas `WS2_ava_hand1` (bola) y público `WS2_avaC1` (bala, base oscura, manos del color de la
  camiseta); gerriko en la cintura. Comparado con los bolos de Resort en Dolphin y probado en las escenas 7 y 9.
  (Primer intento con brazos y dedos —el cuerpo del paracaidismo— descartado: "parecen Pinocho")
- [x] **Material de Resort** sacado del disco y de las decompilaciones (ogws, Skyward Sword): cuerpo, manos, público y
  cabeza con el TEV de Resort y las dos lightmaps de EGG (difusa y brillos) calculadas en la CPU con las luces de la
  bolera (`game/mii/light`); ropa con sus texturas (camisa hawaiana de los bolos). Cuerpos de los bolos: **A6 (hombre)
  y B6 (mujer)**, que ya vienen sin brazos. `hk_body.bin` pasa a la versión 2 (`HKB2`). Probado en Dolphin (escenas 6,
  7 y 9) y comparado con los bolos de Resort
- [x] Cuerpos que se veían transparentes (se pintaba su interior): ahora con la cara frontal de GX, como Resort
- [x] Altura/complexión: Resort **no las usa** (ninguna lectura en su `main.dol`; `GetBodyModelIndex` las ignora):
  quitada nuestra escala aproximada, todos los Miis con el mismo cuerpo
- [ ] Ver los cuerpos nuevos en la Wii real: **copiar el `hk_body.bin` v2 (con la ropa vasca)** (`D:\Tools\wsr-tmp\hk_body.bin`) al USB
  (`F:pps\herri-kirolak\`); con el v1 el juego vuelve a los cuerpos propios
- [ ] Público: Resort usa sus propios modelos (`Audience/G3D/WS2_common_avaC1.brres`, en otro archivo del disco); ahora
  va el C1 de Kokeshi con la base casi negra medida en una captura
- [ ] Luces por deporte: ahora siempre las de la bolera (`tuning::MII_LIGHTS`); cada escena de Resort trae las suyas
  (`Common/<Escena>/Env/*.plight`, `*.plmap`)
- [x] **Ropa vasca** en los jugadores (`tools/mii_outfit.py`, UV propias): hombre con chaqueta azul marino abierta,
  camisa blanca, pañuelo de cuadros del color del jugador, gerriko rojo, pantalón azul a rayas, calcetines con las
  cintas de las abarkas; mujer con chal oscuro (dibujo del color del jugador) y flecos, delantal blanco, blusa,
  medias negras y alpargatas blancas. Público sin cambios (colores lisos). Probado en Dolphin (escenas 6 y 9)
- [x] Caché de personajes: el espectador 33 pisaba al primer aizkolari (salía con cuerpo de público); ahora se
  desaloja el menos usado y los modelos se piden en cada frame
- [ ] Revisar el traje con el usuario (colores, pañuelo, chal) y en la Wii real
- [ ] Revisar con el usuario en la Wii: parecido de Miis y plaza, y fluidez (hay muchos más polígonos)
- [x] Tutorial "Nola jokatu" animado estilo Resort: Wii Remote en 3D, estela, JO!, tronco en V, 5 pasos
  (`games/aizkolaritza/howto.cpp`); grabado en Dolphin, sin probar aún en la Wii
- [ ] Marcador de la partida al estilo Resort
- [ ] Más caras a elegir (ojos, cejas, bocas, narices) como en el editor de Miis
- [x] Juntado en `develop` (2026-10-15, aprobado el estilo: "me gusta el estilo conseguido")
- [ ] Juntar `develop` con `main` cuando lo apruebe (tras probarlo en la Wii real)

## Próximo
- [ ] **Puntero IR en los menús** (además de la cruceta)
- [ ] Probar la lehiaketa con 2-4 mandos reales
- [x] Siguiente deporte elegido: **palas** (reglas y controles en SPEC §6 y DECISIONES)

## Fase 5 — Pala
Diseño en `docs/SPEC.md` §6 (v2, habilidad). Código en `src/games/pala/`; constantes en `tuning.h` (sección Palas).
**Cada paso es una sesión corta**: hacerlo, compilar, probar con la CPU en Dolphin (escenas 11 y 13), marcarlo aquí,
commit + push en la rama `palas`. No hace falta la Wii hasta el paso 5.9.

Hecho (prototipo v1):
- [x] Frontón provisional, pelota con física y predicción, pelotari con pala, reglas, marcador 10/15/22, CPU básica
- [x] Nunchuk obligatorio, choque entre pelotaris, sonidos, escenas 10–14, pantalla de prueba del latigazo

Diseño v2 (en orden; trabajo autónomo del 16/10, ver SPEC §6):
- [x] **5.1 Cámara isométrica cerca que sigue a la pelota** (`updateCamera`): dirección fija atrás-derecha, distancia
  y posición a lo largo de la cancha según la z de la pelota, tirando hacia el humano.
- [x] **5.2 Controles**: `Ctl` por jugador (stick, Z, A, B, latigazo) que rellena el mando o la CPU; Z + stick mueve
  la mira (pared izquierda → txoko → frontis) y planta al pelotari; A/B/nada = largo/dejada/media. Mira 3D en la pared.
- [x] **5.3 Golpe nuevo** (`doHit`): dirección hacia la mira (bipareta si es la pared), velocidad por la fuerza, altura
  buscada simulando con `Ball` para que bote a la profundidad del tipo de golpe; punto dulce (tiempo × sitio) que da
  error y pierde velocidad; ventanas más estrechas con la fuerza; volea / bote pronto; peloteo que se acelera.
- [x] **5.4 CPU por niveles** (Erraza/Normala/Zaila) usando los mismos `Ctl` (se coloca, planta, apunta, A/B, golpea);
  paso de nivel en `setup`.
- [x] **5.5 HUD y efectos**: valoración del golpe (Bikain!/Ondo/Txarto, Bolea!, Bote pronto!, Bipareta!), estela de la
  pelota, destello en el impacto, indicador de tipo de golpe y mira, "Nola jo" de la pala.
- [x] **5.6 Estilo Resort**: frontón con más detalle (bloques, números de cuadro, grada con público Mii que reacciona,
  fondo de monte), animaciones (preparado con saltito, carrera, giro de cuerpo en el golpe, celebración y bajón).
- [x] Escena 15 (CPU Zaila, 2 tantos) para probar el final; repaso con capturas de 10/11/13/15 y aizkolaritza (6)
- [x] Tutorial animado "Nola jokatu" de la pala (5 pasos) al empezar el entrenamiento
- [x] Ikurriña, paneles de colores, intro de cámara desde lo alto del frontis, saltito de preparación
- [ ] **5.7 Repaso**: escenas 11 y 13 con capturas, aizkolaritza (6), docs.
- [ ] **5.9 Wii real**: calibrar el latigazo con la grabación guiada (escena 14) y probar el tacto con el usuario.
- [x] Récords de la pala en save.dat v4 (mejor racha del entrenamiento, peloteo más largo) y página "Pala" en Errekorrak (←/→)
- [ ] 5.11 Frontón de Zarautz (fotos del usuario) · 5.12 2v2

## Más adelante
- [ ] Cansancio bien diseñado (desactivado ahora)
- [ ] Música
- [ ] Final de tronco más espectacular, público que reacciona a cada golpe
- [ ] Miis reales
- [ ] Música
- [ ] Siguientes juegos (ver SPEC §6)
