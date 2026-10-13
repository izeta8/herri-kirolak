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

## Gesto del hachazo (src/engine/input.cpp, valores en src/engine/tuning.h)
Ajustado con gestos reales grabados por telemetría (`tuning-data/`): 28 gestos, todos detectados con el lado bien.
1. **Subir**: el mando gira sobre su eje X de "apuntando a la tele" (0°) a "por encima del hombro" (240-270°,
   punta hacia abajo por detrás). Ángulo = atan2(-gy, gz). Se arma si pasa de 120° (lectura directa del
   acelerómetro cuando |a| está entre 0.5 y 1.6 g, o gravedad filtrada) durante 50 ms acumulados.
   En el mando real "punta arriba" = gravedad en -Y (`RAISE_SIGN = -1`).
2. **Bajar**: desde armado, pico |a| ≥ `CHOP_MIN_G` (los hachazos reales dan 7-8.7 g; subir no pasa de 2.4 g).
3. **Lado de la V**: integral de la aceleración X durante la bajada (de |a| > 2 g a |a| < 1.4 g).
   Derecha → -0.28..-0.93 g·s; izquierda → -0.21..+0.44. Frontera `LAT_MID_GS = -0.24`.
- Ajustes del jugador en save.dat (v3): invertir izquierda/derecha, invertir arriba/abajo, sensibilidad.

### Telemetría y herramientas
- El juego abre el puerto TCP 4300 y envía cada lectura: `mando,ms,ax,ay,az,gx,gy,gz,egoera,kolpea,botoiak`.
- Recibir: `python tools/telemetry.py <IP de la Wii> build/telemetria.csv` (en segundo plano).
- Etiquetar gestos en Kolpe proba: pulsar ← (izquierda) o → (derecha) al empezar y al terminar cada gesto
  (o + al empezar y − al terminar).
- `python tools/replay_detector.py <csv...>`: réplica del detector de input.cpp; dice cuántos gestos
  etiquetados se detectan y con qué lado. **Mantenerla igual que input.cpp** al cambiar el detector.
- `python tools/compare_sides.py <csv>`: compara rasgos de gestos izquierda/derecha.
- Dolphin: K inclina la punta hacia arriba (arma), M = swing hacia abajo. `scripts/keys.ps1 "K>",700,"M+250","K<"`.

## Pendiente de probar en Wii
- Gestos de otras personas (zurdos, otra forma de coger el mando): grabar con telemetría y pasar replay_detector.
- Rendimiento a 60 fps con 28 espectadores + astillas (si va lento: menos público).
- Guardado de jugadores y récords en el USB (`usb:/apps/herri-kirolak/save.dat`).
- Que los textos no queden cortados por los bordes de la tele (overscan).

## Convenciones técnicas
- Caras en orden antihorario (como OpenGL) → `GX_SetCullMode(GX_CULL_FRONT)`. `MeshBuilder::tri()` orienta solo.
- 2D en coordenadas virtuales: 854×480 en 16:9, 640×480 en 4:3 (`gfx::screenW()`).
- Personaje: mira hacia +Z con yaw 0. El tronco está tumbado sobre el eje X; la V de delante es +Z.
- Las mallas de personajes se cachean por aspecto (`characterCache`, 32 huecos); el público usa 10 aspectos.
- Dolphin: config en `D:\Tools\Dolphin\Dolphin-x64\User\Config\` (estadísticas desactivadas; A también con X, B con Z).
