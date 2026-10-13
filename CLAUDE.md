# Herri Kirolak Wii — instrucciones para Claude

Juego homebrew de Wii estilo Wii Sports con deportes rurales vascos. Lee **docs/SPEC.md** (qué se construye) y **docs/ROADMAP.md** (por dónde vamos) antes de empezar cualquier tarea.

## Forma de trabajar
- El usuario habla castellano; los textos del juego van en **euskera** (todos en `src/game/text.h`).
- Tareas medianas: coge el siguiente punto sin hacer de ROADMAP.md, impleméntalo, compílalo, pruébalo en Dolphin (con captura de pantalla) y luego haz commit + push y marca el punto en el roadmap.
- Lo que solo se puede comprobar en la Wii real (cómo se sienten los gestos, el rendimiento) se apunta en la sección "Pendiente de probar en Wii" del final, para que lo pruebe el usuario.
- El repo es **público**: solo assets propios o con licencia CC0/OFL (apúntalos en `ASSETS.md`). Nada de assets de Nintendo ni ISOs.

## Entorno (Windows)
- devkitPro: `C:\devkitPro` (instalador por defecto; entorno de devkitPro: `DEVKITPRO=/opt/devkitpro`, `DEVKITPPC=/opt/devkitpro/devkitPPC`)
- Dolphin: `D:\Tools\Dolphin\Dolphin-x64\Dolphin.exe` (portable; config en `User/` junto al exe)
- Blender: addon MCP (herramientas `mcp__blender__*`), de momento no se usa: la geometría es procedural
- Disco C: casi lleno: proyecto, Dolphin y descargas en D:

## Comandos
- Compilar versión para jugar: `scripts/build.ps1` → `build/boot.dol`
- Compilar versión de prueba: `scripts/build.ps1 -Test -Scene N` → `build-test/boot.dol`
  - Activa `DEBUG_INPUT` (botones 1 = golpe ezkerra, 2 = golpe eskuina, − = golpe recto)
  - `START_SCENE`: 0 título, 1 menú, 2 preparar partida, 3 jugadores, 4 récords, 5 entrenamiento,
    6 lehiaketa CPU contra CPU (resultados), 7 entrenamiento con la CPU, 8 kolpe proba
- Ejecutar en Dolphin: `scripts/run.ps1 [-Dol ruta]`
- Captura de Dolphin: `scripts/screenshot.ps1 -Name x` → `build/screenshots/x.png`
- Pulsar teclas en Dolphin: `scripts/keys.ps1 X,DOWN,X,1,2` (X = A, Z = B, flechas = cruceta, 1/2, Q = −, E = +, ENTER = HOME; un número = pausa en ms)
- Copiar al USB de la Wii (`ESD-USB`, normalmente F:; el `WiiGames` tiene mal el conector): `scripts/install-usb.ps1 -Drive F`
- **Mandar a la Wii por WiFi (lo normal)**: `scripts/deploy.ps1` (wiiload a 192.168.1.155, MAC 78-a2-a0-91-11-36; con el Homebrew Channel abierto). Si la IP cambia, buscarla por la MAC con un barrido de ping + `Get-NetNeighbor`. Torre: 192.168.1.129
- Regenerar recursos: `python tools/bake_font.py assets/fonts/Nunito-Variable.ttf data/font.bin`, `python tools/gen_sfx.py data`

## Estructura
- `src/engine/` motor (gfx, mesh, font, input, audio, save, ui, scene, tuning.h)
- `src/game/` cosas comunes a todos los juegos (personaje, escenario, paleta, textos)
- `src/scenes/` menús (título, menú, preparar partida, jugadores, récords)
- `src/games/aizkolaritza/` el juego: tronco, aizkolari (animaciones) y la escena de partida
- `data/` binarios embebidos en el .dol (fuente, sonidos), generados por `tools/`

## Gesto del hachazo (src/engine/input.cpp)
1. **Subir**: la punta del Wii Remote hacia arriba (gravedad en +Y, `pitchUp > RAISE_ON`) durante `RAISE_HOLD` → ARMADO.
   El hacha del personaje sigue la altura del mando (`Pad::raise`).
2. **Bajar**: desde armado, pico de |a| ≥ `CHOP_MIN_G[sensibilidad]` → HACHAZO. Sin armar, no hay golpe.
3. **Lado de la V**: `ROLL_WEIGHT` × inclinación del mando arriba (dónde empieza: arriba a la derecha/izquierda)
   + resto × empuje lateral integrado durante la bajada.
- Se procesan todas las lecturas del mando (~100/s) con `WPAD_ReadPending`, no una por frame.
- Ajustes del jugador en save.dat (v2): invertir izquierda/derecha, invertir arriba/abajo, sensibilidad.
- Dolphin: I/K inclinan adelante/atrás (I = punta arriba → arma), J/L izquierda/derecha, M = swing hacia abajo.
  Ejemplo: `scripts/keys.ps1 "I>",700,"M+250","I<"`.
- **Kolpe proba** graba lecturas en `usb:/apps/herri-kirolak/golpeak.csv` (botón 1; etiqueta con ←/→).
  Columnas: saioa, ms, etiketa (L/Z/R), ax, ay, az, gx, gy, gz (gravedad filtrada), egoera (0 idle, 1 subiendo,
  2 armado, 3 bajando, 4 enfriando), kolpea (1 = golpe detectado), ajustes. Analizarlo para afinar tuning.h.

## Pendiente de probar en Wii
- **Grabar golpes en Kolpe proba** (unos 10 de cada: ezkerra, zuzen, eskuina) y analizar golpeak.csv para fijar
  `RAISE_SIGN`, `RAISE_ON`, `CHOP_MIN_G`, `LAT_FULL_GS`, `LAT_SIGN`, `ROLL_WEIGHT`, `TILT_SIGN`.
- Rendimiento a 60 fps con 28 espectadores + astillas (si va lento: menos público).
- Guardado de jugadores y récords en el USB (`usb:/apps/herri-kirolak/save.dat`).
- Que los textos no queden cortados por los bordes de la tele (overscan).

## Convenciones técnicas
- Caras en orden antihorario (como OpenGL) → `GX_SetCullMode(GX_CULL_FRONT)`. `MeshBuilder::tri()` orienta solo.
- 2D en coordenadas virtuales: 854×480 en 16:9, 640×480 en 4:3 (`gfx::screenW()`).
- Personaje: mira hacia +Z con yaw 0. El tronco está tumbado sobre el eje X; la V de delante es +Z.
- Las mallas de personajes se cachean por aspecto (`characterCache`, 32 huecos); el público usa 10 aspectos.
- Dolphin: config en `D:\Tools\Dolphin\Dolphin-x64\User\Config\` (estadísticas desactivadas; A también con X, B con Z).
