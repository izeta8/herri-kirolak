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
| `input` | 4 Wii Remotes: botones, acelerómetro a ~100 lecturas/s, **detector de hachazo** | `input::pad(i)` → `Pad` (`down`, `held`, `accel`, `grav`, `raise`, `state`, `chop`, `swingStart`, `swingTilt`…); `allDown()`, `rumble(i, s)` |
| `audio` | Efectos con ASND (PCM 16 bits BE 32 kHz) | `audio::play(SFX_x, volumen, tono, pan)` |
| `save` | `save.dat` junto al `boot.dol` (USB o SD), versión 3 con migración | `save::data()` (`profiles`, `records`, `settings`), `save::write()`, `save::directory()` |
| `ui` | Elementos estilo Wii manejados con cruceta | `background`, `header`, `panel`, `button`, `buttonIcon`, `hints`, `fade`, `colorDot`, `formatTime`, `ListMenu` |
| `scene` | Escenas con fundido y diálogo de HOME global | `class Scene { update(dt); draw(); }`, `scenes::go(new X)`, `scenes::openExitDialog()` |
| `tuning.h` | **Todas** las constantes ajustables (gesto, juego) | — |
| `telemetry` | Hilo de red único: telemetría TCP 4300 + servidor de actualización | `telemetry::push`, `note`, `status`, `ip`, `netListen`, `readable` |
| `updater` | Actualización en caliente (TCP 4301) y salto al cargador `tools/booter` | `updater::open/poll/ready/launch` |

## Común a los deportes (`src/game/`)
- **`sports.h/.cpp`** — registro de deportes y `MatchConfig` (deporte, entrenamiento/lehiaketa, jugadores, rondas,
  perfiles, CPU). El menú "preparar partida" se construye con esta lista. Ver `docs/NUEVO-JUEGO.md`.
- **`character`** — Mii calcado a los de la Wii: cabeza de huevo con barbilla en punta, cara pintada con trazos sobre la
  superficie (`decalStroke`/`decalEllipse`: ojos, cejas, nariz de gancho, boca de raya), pelo de casco con flequillo en
  picos (4 peinados, `Profile::hairStyle`), cuerpo estrecho con brazos de manga larga y manos de bola, txapela.
  `Appearance::casual` = público (pantalón gris, menos polígonos). `characterCache(Appearance)` (32 huecos),
  `drawCharacter(model, CharacterPose)`. Mira hacia +Z con yaw 0.
- **`world`** — escenario de aizkolaritza: **el probaleku de Usurbil** (empedrado, muros de cantos con albardilla, gradas
  con toldo, porterías, iglesia de San Salvador con la torre, frontón cubierto, casas de Etxebeste e Irazu, montes;
  29 espectadores con 10 aspectos). −Z = sur (iglesia), +X = oeste (Etxebeste); la plataforma a y = 0, calles a −1.
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

## Convenciones
- Caras en orden antihorario → `GX_CULL_FRONT`; `MeshBuilder::tri()` orienta las caras solo con un vector "hacia fuera".
- Unidades: metros. El suelo es y = 0.
- Las mallas no tienen destructor: se liberan con `release()` o viven toda la partida.
- Los textos con `font::draw` llevan contorno; `font::plain` sin contorno.
- Recursos nuevos: generarlos con un script en `tools/` (nunca binarios hechos a mano) y apuntarlos en `ASSETS.md`.
