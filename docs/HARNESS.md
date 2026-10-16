# Harness de desarrollo

Todo lo que permite programar, probar y ajustar el juego sin depender de que el usuario esté delante de la tele.

## El ciclo
```
editar código ─► compilar ─► probar en Dolphin (capturas + teclas) ─► mandar a la Wii (en caliente)
      ▲                                                                      │
      └──── calibrar con la réplica ◄── datos reales del mando (telemetría) ◄┘
```

## 1. Compilar — `scripts/build.ps1`
| Comando | Resultado |
|---|---|
| `scripts/build.ps1` | `build/boot.dol`: versión para jugar (arranca en el título) |
| `scripts/build.ps1 -Test -Scene N` | `build-test/boot.dol`: arranca en la escena N y activa `DEBUG_INPUT` (1/2/− = golpes simulados) |
| make directo (bash de devkitPro) | `make OUTDIR=build-rec DEFS='-DSTART_SCENE=8'`: escena N **sin** DEBUG_INPUT (para mandar a la Wii) |

`START_SCENE` (en `main.cpp`): 0 título · 1 menú · 2 preparar partida · 3 jugadores · 4 récords · 5 entrenamiento ·
6 lehiaketa 4 CPU a la vez · 7 entrenamiento con la CPU · 8 Kolpe proba · 9 galería de Miis (de cerca; el zoom cambia
solo cada 4 s) · **pala**: 10 entrenamiento · 11 CPU contra CPU · 12 jugador contra la CPU · 13 entrenamiento de la CPU ·
14 kolpe proba de la pala (latigazo, grabación guiada S/F/B) · 15 CPU Zaila contra CPU Zaila a 2 tantos (para ver
el final, los resultados y las biparetas) · 16 animaciones del pelotari de cerca · 17 Aukerak (ajustes). La escena 10 empieza con el tutorial
animado (sin tocar nada avanza solo).
Con `-Test`, en la pala 1 = latigazo suave y 2 = fuerte; el saque con A (X en Dolphin) y el stick del Nunchuk con WASD. **Al crear algo nuevo, añadir su número.**

## 2. Probar en Dolphin
- `scripts/run.ps1 [-Dol ruta]` cierra Dolphin si estaba abierto y arranca el .dol.
- `scripts/screenshot.ps1 -Name x` → `build/screenshots/x.png` (leerla con la herramienta Read para verla). Usa
  PrintWindow: captura aunque Dolphin esté tapado y **no le quita el foco** al usuario (`-Focus` = método antiguo).
- `scripts/keys.ps1 X,DOWN,X,"K>",700,"M+250","K<"` pulsa teclas: X=A, Z=B, flechas, 1, 2, Q=−, E=+, ENTER=HOME,
  I/K/J/L inclinar, M/N/O swing, W/A/S/D stick del Nunchuk, LSHIFT/LCONTROL = Z/C; `"K+800"` mantener; `"K>"`/`"K<"` pulsar/soltar; un número = pausa en ms.
- Para varias capturas seguidas, unir en una cuadrícula con PIL y mirar una sola imagen.
- Probar siempre la CPU (escenas 6 y 7; en la pala 11 y 13) tras tocar animaciones o la escena de partida.
- Dolphin no sirve para el tacto del gesto (no gira más de 90°): eso se valida en la Wii con telemetría.

- **Vídeo para el usuario** (cuando no está en casa): `scripts/record.ps1 -Dol build-test/boot.dol -Seconds 75 -Name x`
  vuelca cada fotograma de Dolphin (vale con la pantalla apagada, donde PrintWindow sale en blanco) y deja
  `build/screenshots/x.gif`; mandarlo con SendUserFile. Restaura la configuración de Dolphin al terminar.
- En este PC Dolphin tarda **~40 s** en abrir la ventana: esperar con un bucle, no darlo por colgado a los 15 s.

## 2b. Miis de verdad en Dolphin — `scripts/pull-miis.ps1`
Con el juego abierto en la Wii (versión con "HKPL"), copia `RFL_Res.dat` y `RFL_DB.dat` de la consola a la NAND de
Dolphin (`D:\Tools\Dolphin\Dolphin-x64\User\Wii\shared2\...`). Solo hace falta una vez. Son ficheros de Nintendo de la
consola del usuario: **nunca al repositorio**.
- **`RFL_Res.dat` no está en la NAND de la Wii** (va dentro de cada juego): `pull-miis.ps1` solo trae `RFL_DB.dat`. Las
  piezas se sacaron del disco de Wii Sports Resort (CleanRip por wiiload → USB; `DolphinTool extract` de
  `Common/Kokeshi/common.carc`, Yaz0 + U8). Van a `User\Wii\shared2\FaceLib\` (Dolphin) y a `apps\herri-kirolak\` (USB).
- `scripts/wii-ls.ps1 /ruta/ …`: lista carpetas de la NAND con el juego abierto ("HKPL" con la ruta acabada en `/`).
  Solo rutas de Miis (`allowedPull` en `updater.cpp`); la nota `# nand: … (err N)` de la telemetría da el error de ISFS
  (-106 = no existe). `/title/.../content` no se puede listar desde el Homebrew Channel.
- **Cuerpos de Resort**: `python tools/mii_body.py D:\Tools\wsr-tmp\u8\Kokeshi\G3D D:\Tools\wsr-tmp\hk_body.bin` y copiar
  `hk_body.bin` a `User\Wii\shared2\FaceLib\` (Dolphin) y a `F:\apps\herri-kirolak\` (USB). Un tercer argumento elige la ropa de los jugadores: `vasca` (por defecto,
  `tools/mii_outfit.py`) o `resort` (la de los bolos). Es la versión 2 (`HKB2`:
  geometría, UV, oclusión y texturas de ropa); con un `hk_body.bin` viejo el juego usa los cuerpos propios.
  `tools/brres_mdl0.py x.brres x.obj` lista huesos y objetos de un modelo y lo exporta en reposo (para mirarlo);
  `tools/brres_tex0.py x.brres carpeta` vuelca sus texturas en PNG.

## 3. Mandar a la Wii — `scripts/deploy.ps1 [-Dol ruta]`
1. Si el juego está abierto: **actualización en caliente** (TCP 4301). La Wii muestra "Eguneratzen…" y salta a la
   versión nueva sin pasar por el Homebrew Channel.
2. Si no: **wiiload** al Homebrew Channel (TCP 4299; el HBC tiene que estar abierto).
3. Busca la Wii por su MAC si la IP guardada (`build/wii-ip.txt`) no responde.
- Instalarla para jugar sin PC: `scripts/install-usb.ps1 -Drive F` (USB `ESD-USB`; conserva `save.dat`).

## 4. Telemetría (datos reales del mando)
- El juego envía cada lectura del Wii Remote por TCP 4300 (y notas `# …` de depuración, p. ej. del actualizador).
- Recibir en segundo plano: `python -u tools/telemetry.py <IP> build/telemetria.csv >> build/telemetria.log`
  (se reconecta solo; `tail build/telemetria.log` da un resumen cada medio segundo).
- **Grabación guiada**: en Kolpe proba (o la prueba del gesto del deporte), botón 1. La pantalla pide el gesto
  (ESKUINA / EZKERRA…) y la etiqueta viaja en la telemetría. También: flechas ←/→ o +/− para marcar gestos a mano.
- Copiar la sesión útil a `tuning-data/AAAA-MM-DD-<qué>.csv` (desde la línea `# conexion` correspondiente).

## 5. Analizar y calibrar (`tools/`)
| Script | Para qué |
|---|---|
| `replay_detector.py <csv…>` | Réplica del detector de `input.cpp`: aciertos por gesto etiquetado. **Mantener idéntica a input.cpp** |
| `analyze_gestures.py <csv> [paso]` | Ver gesto a gesto las lecturas (marcados con +/−) |
| `compare_sides.py <csv>` | Rasgos de los gestos ← frente a → |
| `telemetry.py` | Receptor |
Patrón de análisis que funcionó: extraer rasgos por gesto → separación (Fisher) por rasgo → LDA con 2–4 rasgos y
validación dejando uno fuera → poner pesos en `tuning.h` → comprobar con `replay_detector.py` → mandar.

## 6. Recursos
- Fuente: `python tools/bake_font.py assets/fonts/Nunito-Variable.ttf data/font.bin`
- Sonidos (sintetizados): `python tools/gen_sfx.py data`
- Cargador de la actualización en caliente: `sh tools/booter/build.sh` (en el bash de devkitPro) → `data/booter.bin`

## Skills de Claude Code (`.claude/skills/`)
`probar-en-dolphin`, `mandar-a-wii`, `calibrar-gesto`, `nuevo-juego`: procedimientos paso a paso de lo anterior.
