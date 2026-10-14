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
solo cada 4 s). **Al crear algo nuevo, añadir su número.**

## 2. Probar en Dolphin
- `scripts/run.ps1 [-Dol ruta]` cierra Dolphin si estaba abierto y arranca el .dol.
- `scripts/screenshot.ps1 -Name x` → `build/screenshots/x.png` (leerla con la herramienta Read para verla). Usa
  PrintWindow: captura aunque Dolphin esté tapado y **no le quita el foco** al usuario (`-Focus` = método antiguo).
- `scripts/keys.ps1 X,DOWN,X,"K>",700,"M+250","K<"` pulsa teclas: X=A, Z=B, flechas, 1, 2, Q=−, E=+, ENTER=HOME,
  I/K/J/L inclinar, M/N/O swing; `"K+800"` mantener; `"K>"`/`"K<"` pulsar/soltar; un número = pausa en ms.
- Para varias capturas seguidas, unir en una cuadrícula con PIL y mirar una sola imagen.
- Probar siempre la CPU (escenas 6 y 7) tras tocar animaciones o la escena de partida.
- Dolphin no sirve para el tacto del gesto (no gira más de 90°): eso se valida en la Wii con telemetría.

## 2b. Miis de verdad en Dolphin — `scripts/pull-miis.ps1`
Con el juego abierto en la Wii (versión con "HKPL"), copia `RFL_Res.dat` y `RFL_DB.dat` de la consola a la NAND de
Dolphin (`D:\Tools\Dolphin\Dolphin-x64\User\Wii\shared2\...`). Solo hace falta una vez. Son ficheros de Nintendo de la
consola del usuario: **nunca al repositorio**.

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
