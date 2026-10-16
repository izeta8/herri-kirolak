# Arquitectura

C++17 sobre **libogc** (devkitPPC), sin excepciones ni RTTI. Todo se compila en un único `boot.dol`; los recursos
(`data/*.bin`) se embeben con `bin2o` (símbolos `<nombre>_bin` y `<nombre>_bin_size`, cabecera `<nombre>_bin.h`).

```
src/
  main.cpp                 init de módulos, bucle principal, START_SCENE (pruebas), reset/power, salto de actualización
  engine/                  motor genérico (no sabe nada de deportes)
  game/                    común a todos los deportes: personajes, plaza, paleta, textos, registro de deportes
  scenes/                  menús: título, menú principal, preparar partida, jugadores, récords, Kolpe proba
  games/<deporte>/         cada deporte: sus objetos, animaciones y la escena de partida
tools/                     generadores (fuente, sonidos), telemetría y análisis de gestos, cargador (booter)
data/                      binarios embebidos (generados por tools/)
tuning-data/               grabaciones reales del mando (CSV) para calibrar gestos
```

## Bucle principal (`main.cpp`)
`gfx::init → input::init → audio::init → font::init → save::init(argv[0]) → save::applySettings → telemetry::init`,
luego `scenes::start(firstScene())` y en cada frame: `input::update(dt)` → si `updater::ready()` salta a la versión
nueva → `scenes::frame(dt)` (actualiza y dibuja la escena, fundidos y diálogo de HOME). `dt` medido y limitado a
[1/120, 1/20] s. 60 fps (50 en PAL 50 Hz).

## Motor (`src/engine/`)

| Módulo | Qué hace | API principal |
|---|---|---|
| `gfx` | Vídeo y GX. 3D con 2 luces + ambiente + niebla; 2D en coordenadas virtuales **854×480 (16:9) / 640×480 (4:3)** | `begin3D(Camera)`, `drawMesh(mesh, Xf)`, `setLighting/Depth/Blend/Cull/Fog`, `project(p, sx, sy)`; `begin2D()`, `rect`, `rectGrad`, `roundRect(Grad)`, `circle`, `ring`, `arc`, `triangle`; `screenW()`, `widescreen()` |
| `Xf` (en gfx.h) | Transformaciones encadenables estilo OpenGL | `Xf().translate(p).rotateY(a).scale(s)`, `fromBasis`, `apply`, `applyDir` |
| `mesh` | Mallas con color por vértice → display lists de GX | `MeshBuilder` (`box`, `sphere`, `cylinder`, `capsule`, `disc`, `tri`, `quad`, campo `xf`) → `build(Mesh&)`; `Mesh::release()` |
| `font` | Texto con contorno (Nunito OFL, atlas IA8, Latin-1: ñ, tildes) | `font::draw(txt, x, y, size, relleno, contorno, align, alpha)`, `font::plain(...)`, `font::width` |
| `input` | 4 Wii Remotes: botones, acelerómetro a ~100 lecturas/s, **detector de hachazo**, **latigazo** (pala) y **stick del Nunchuk** | `input::pad(i)` → `Pad` (`down`, `held`, `accel`, `grav`, `raise`, `state`, `chop`, `swingStart`, `swingTilt`, `whip`, `whipStart`, `nunchuk`, `stickX/Y`…); `allDown()`, `rumble(i, s)` |
| `audio` | Efectos con ASND (PCM 16 bits BE 32 kHz) | `audio::play(SFX_x, volumen, tono, pan)` |
| `save` | `save.dat` junto al `boot.dol` (USB o SD), versión 4 con migración (v1–v3 se leen) | `save::data()` (`profiles`, `records` de aizkolaritza, `settings`, `pala`), `save::write()`, `save::directory()` |
| `ui` | Elementos estilo Wii manejados con cruceta | `background`, `header`, `panel`, `button`, `buttonIcon`, `hints`, `fade`, `colorDot`, `formatTime`, `ListMenu` |
| `scene` | Escenas con fundido y diálogo de HOME global | `class Scene { update(dt); draw(); }`, `scenes::go(new X)`, `scenes::openExitDialog()` |
| `tuning.h` | **Todas** las constantes ajustables (gesto, juego) | — |
| `telemetry` | Hilo de red único: telemetría TCP 4300 + servidor de actualización | `telemetry::push`, `note`, `status`, `ip`, `netListen`, `readable` |
| `updater` | Actualización en caliente (TCP 4301) y salto al cargador `tools/booter` | `updater::open/poll/ready/launch` |

## Común a los deportes (`src/game/`)
- **`sports.h/.cpp`** — registro de deportes y `MatchConfig` (deporte, entrenamiento/lehiaketa, jugadores, rondas,
  perfiles, CPU). Cada `Sport` puede limitar los jugadores (`maxPlayers`), enseñar otros valores en las rondas
  (`roundValues`, p. ej. 10/15/22 tantos) y cambiar el título de elegir personaje (`chooseChar`). El menú "preparar partida" se construye con esta lista. Ver `docs/NUEVO-JUEGO.md`.
- **`character`** — Mii calcado a los de la Wii: cabeza de huevo con barbilla en punta, cara pintada con trazos sobre la
  superficie (`decalStroke`/`decalEllipse`: ojos, cejas, nariz de gancho, boca de raya), pelo de casco con flequillo en
  picos (4 peinados, `Profile::hairStyle`), cuerpo estrecho con brazos de manga larga y manos de bola, txapela.
  `Appearance::casual` = público (pantalón gris, menos polígonos). Con el cuerpo de Resort, `CharacterPose` también
  mueve las piernas (`legL/legR` muslo hacia delante, `kneeL/kneeR` rodilla; huesos `leg_l1/leg_l2/...`). `characterCache(Appearance)` (32 huecos),
  `drawCharacter(model, CharacterPose)`. Mira hacia +Z con yaw 0.
- **`world`** — escenario de aizkolaritza: **el probaleku de Usurbil** (empedrado, muros de cantos con albardilla, gradas
  con toldo, porterías, iglesia de San Salvador con la torre, frontón cubierto, casas de Etxebeste e Irazu, montes;
  29 espectadores con 10 aspectos). −Z = sur (iglesia), +X = oeste (Etxebeste); la plataforma a y = 0, calles a −1.
- **`mii/`** — **Miis de verdad**: piezas (modelos y texturas) de `RFL_Res.dat`: primero la NAND
  (`/shared2/FaceLib/`, así está en Dolphin) y si no, `RFL_Res.dat` en la carpeta del juego del USB/SD (`save::directory()`;
  en la Wii real no está en la NAND). Miis del Canal Mii de `/shared2/menu/FaceLib/RFL_DB.dat` con `engine/nand`. `MiiHead` monta la cabeza como
  la RVLFaceLib (referencia: decompilación pública SMGCommunity/Petari): contorno, máscara, pelo, gorra, frente, nariz,
  raya de la nariz, barba y gafas con display lists indexadas (formato de vértices 2, s16), y la cara (ojos, cejas,
  boca, bigote, lunar) pintada en una textura RGB5A3 al empezar el frame (`gfx::offscreen*`, cola `mii::processPending`).
  Si no hay piezas (Dolphin sin copiarlas), `CharacterModel` usa nuestra cabeza de siempre. **Nada de esto va al repo.**
- **`mii/body`** — cuerpo de Wii Sports Resort leído de `hk_body.bin` v2 (`mii::readAsset`; lo genera
  `tools/mii_body.py`): los de los bolos, A6 (hombre) y B6 (mujer) según `MiiInfo::female`, C1 (público) y la mano
  `hand1`. La geometría es común (una display list por hueso, formato de vértices 4: posición, normal, alfa de oclusión
  y UV) y `Body` solo guarda colores (camiseta = C0, piel = C1) y si la camiseta es clara (ropa `_awy`). Se dibuja con
  el **TEV de Resort** (`setMaterial`: ropa = textura + color favorito, piel por máscara; luego la luz). La pose se
  calcula en espacio del modelo: inclinación del pecho y cabeza. Sin brazos: las manos son la bola de Resort en
  `handL/handR` (la derecha en espejo) con la ropa del cuerpo; el público mueve sus manos (`handL/handR` del modelo).
  `draw()` devuelve dónde van la cabeza (`head_set`) y el gerriko. Unidades de Resort × `MII_BODY_UNIT` (tuning.h).
  Sin escala por altura ni complexión (Resort no la usa). `CharacterModel::body` lo usa si existe; si no, el cuerpo
  propio de antes.
  Ropa de los jugadores: la pinta `tools/mii_outfit.py` (UV propias: ángulo alrededor del cuerpo × altura; mitad de
  textura para el cuerpo y mitad para la chaqueta del hombre), con el mismo material: alfa = color del jugador.
- **Caché de personajes** (`characterCache`): desaloja el menos usado; **pedir el modelo en cada frame** que se
  dibuje, nunca guardar el puntero entre frames.
- **`mii/light`** — la luz de los Miis de Resort: dos lightmaps de 32×32 (lm0 difusa, lm1 brillos y borde) indexadas
  por la normal en cámara (`GX_TEXCOORD1`, `GX_TEXMTX0` + post-matriz "env camera"), que se repintan en la CPU cuando
  gira la cámara con el algoritmo de `EGG::LightTexture` y las luces de la bolera (`tuning::MII_LIGHTS`). Cuerpo,
  manos y cabeza (`MiiHead::draw`) hacen `albedo × (lm0·oclusión + ambiente) + lm1·oclusión`. Usar
  `miilight::loadModelMtx` en vez de `gfx::loadModelMtx` para todo lo que lleve lightmaps.
  Jugadores = Miis de la consola: `Profile::mii` (índice + 1) → `Appearance::user`; `linkProfilesToMiis()` (al arrancar)
  y `setProfileMii()` en `scenes/common`. El público usa también esos Miis (`World::build`).
- **Fondo de menús**: `ui::setBackground(drawMenuBackdrop)` (en `main.cpp`) pinta el escenario 3D detrás de los paneles
  de cristal (estilo Wii Sports Resort); `gfx::clearDepth()` borra el Z antes del personaje de vista previa.
  `sharedWorld()` (una sola instancia), `drawSky()` (2D, antes de 3D), `draw()`, `cheer(s)`, `clapLight(s)`,
  `drawShadow(pos, rx, rz)`, `setCameraHide(eye, on)` (oculta el público entre una cámara lejana y la plaza).
- **`palette.h`** — colores favoritos, piel, pelo, colores de UI y madera.
- **`text.h`** — **todos** los textos en euskera (nada de cadenas sueltas en el código).

## Menús (`src/scenes/`)
`title` (demo de la CPU cortando) → `menu` (Jokatu / Jokalariak / Errekorrak / Irten) → `setup` (deporte → modo →
jugadores → rondas → personajes, con − para CPU) → escena del deporte. `profiles` (editor con teclado en pantalla),
`records`, `swingtest` (Kolpe proba: gráficas del mando, grabación guiada), `common` (`drawCharacterPreview`).

## Aizkolaritza (`src/games/aizkolaritza/`)
- `log` — `Log`: tronco tumbado sobre X con dos V (delante +Z, detrás −Z), malla regenerada al cambiar la profundidad,
  partido en dos al final. `Chips`: astillas con física simple.
- `aizkolari` — `Aizkolari`: personaje + hacha encima del tronco; fases IDLE/READY/SWING/STUCK/RECOVER/TURN/CELEBRATE.
  La bajada empieza con `startSwing()` (gesto) y se resuelve con `chop(fuerza, lado)`; `impact()` es el evento de corte.
- `aizko_scene` — la partida. `Player` guarda el estado de cada jugador (tronco, aizkolari, tiempo, puesto…).
  Entrenamiento: 1 jugador, cámara que gira con él. Lehiaketa: hasta 4 a la vez en fila (`SPACING` 2.1 m), cámara fija,
  un panel por jugador.

## Pala (`src/games/pala/`) — diseño v2 de habilidad (SPEC §6), frontón provisional
- `fronton` — malla del frontón: frontis (z = 0) y pared izquierda (x = −COURT_W/2) de bloques de piedra con color
  algo distinto, chapa, línea de arriba, rayas de los cuadros (falta y pasa en rojo), suelo con sombra suave junto a las
  paredes, rebote, grada con barandilla; lejos, casas y montes. Público Mii sentado en la grada (`cheer`, `clap`) y
  números de los cuadros en 2D (`drawLabels`). Medidas en `tuning.h` (sección Palas).
- `ball` — `Ball`: física con subpasos (suelo, frontis, pared izquierda, rebote) y estado de la jugada (`needFrontis`,
  `bounces`, `drop` = saque botado). `step()` devuelve eventos con el estado de ese instante; las reglas las aplica la
  escena. `BallPath::predict()` simula hacia delante (con botes y tiempo desde el bote: volea / bote pronto).
- `pelotari` — `Pelotari`: Mii mirando al frontis (−Z) con la pala en la mano derecha. Piernas con los huesos del cuerpo
  de Resort (`CharacterPose::legL/legR/kneeL/kneeR`): espera flexionado, carrera, plantado al apuntar, paso al golpear.
  `swing(lado, altura, contactoEn, fuerza)` hace que la pala llegue a la pelota justo en el impacto (se reajusta si ya
  iba). Celebración y lamento (`setMood`). `palaTip()/palaCenter()` para la estela.
- `pala_scene` — la partida. Cada jugador rellena un `Ctl` (stick, Z, A, B, latigazo) desde el mando (`readHuman`) o
  la CPU (`updateBot`), y el resto del código es el mismo para los dos.
  - **Golpe** (`tryHit`): `closest()` busca cuándo pasa la pelota más cerca (historial hacia atrás + `BallPath`); calidad
    = tiempo × sitio × altura con ventanas que se estrechan con la fuerza y en volea/bote pronto. Si llega pronto,
    queda pendiente hasta que la pelota llega.
  - **Tiro** (`doHit` + `solveShot`): la mira `u` (frontis −1..1, pared izquierda < −1) da el punto; el botón
    (B / nada / A) la profundidad; la fuerza la velocidad; `solveShot` prueba alturas simulando `Ball` y elige la que
    bota más cerca de la profundidad pedida (las biparetas salen solas de la física). La calidad mete error y quita
    velocidad. El peloteo se acelera (`rallyHits_`).
  - **CPU** por niveles (`tuning::BOT_LEVELS`, `MatchConfig::level`): elige dónde golpear en la trayectoria, se coloca a
    `SWEET_DIST`, se planta y apunta con su Z, elige tipo de golpe según el rival y golpea con su error de tiempo.
  - **Cámara** (`updateCamera`): dirección fija desde atrás-derecha; objetivo entre el pelotari que golpea y la pelota;
    distancia según la z de la pelota (cerca del frontis = cerca); si no caben el que golpea, la pelota y los humanos,
    primero abre el FOV (hasta `CAM_FOV_MAX`) y luego se aleja (hasta `CAM_DIST_MAX`).
  - Efectos 2D: estela de la pala, anillo en el frontis, chispa del golpe, mira; HUD con marcador, tiempo/sitio del
    último golpe, tipo de golpe según los botones y tarjeta "Nola jo" (entrenamiento).
- `whiptest` — "Pala: kolpe proba": el latigazo en directo y grabación guiada para calibrarlo.

## Convenciones
- Caras en orden antihorario → `GX_CULL_FRONT`; `MeshBuilder::tri()` orienta las caras solo con un vector "hacia fuera".
- Unidades: metros. El suelo es y = 0.
- Las mallas no tienen destructor: se liberan con `release()` o viven toda la partida.
- Los textos con `font::draw` llevan contorno; `font::plain` sin contorno.
- Recursos nuevos: generarlos con un script en `tools/` (nunca binarios hechos a mano) y apuntarlos en `ASSETS.md`.
