# Herri Kirolak Wii — instrucciones para Claude

Juego homebrew para la **Wii real** del usuario, estilo **Wii Sports**, con deportes rurales vascos (herri kirolak).
C++17 + libogc (devkitPro). Se prueba en Dolphin y en la Wii por WiFi. Repo público: github.com/izeta8/herri-kirolak.
Estado: **Aizkolaritza terminado** (entrenamiento + lehiaketa 1–4 a la vez). Siguiente: ver `docs/ROADMAP.md`.

## Qué leer y cuándo
| Vas a… | Lee |
|---|---|
| empezar cualquier tarea | este fichero + `docs/ROADMAP.md` (por dónde vamos) |
| tocar algo que el usuario ya decidió (controles, UI, modos) | `docs/DECISIONES.md` — **no deshacer decisiones sin preguntar** |
| entender el código / el motor | `docs/ARQUITECTURA.md` |
| crear un deporte nuevo | `docs/NUEVO-JUEGO.md` (+ skill `nuevo-juego`) |
| diseñar o ajustar un gesto del mando | `docs/GESTOS.md` (+ skill `calibrar-gesto`) |
| compilar, probar en Dolphin, mandar a la Wii, telemetría | `docs/HARNESS.md` (+ skills `probar-en-dolphin`, `mandar-a-wii`) |
| algo raro con la Wii, la red, Dolphin o Windows | `docs/WII-NOTAS.md` |
| qué es el juego (specs) | `docs/SPEC.md` |

## Reglas de trabajo
- El usuario habla **castellano**; los textos del juego van en **euskera** y todos en `src/game/text.h`.
- Tareas medianas: implementar → compilar → probar en Dolphin (con captura revisada) → si hace falta, Wii real con
  `scripts/deploy.ps1` → commit + push. Comprobar siempre también la CPU (escenas 6 y 7).
- Todo lo que afecta al tacto (gestos, tiempos) se decide **con datos reales** (telemetría + réplica), no a ojo.
- Constantes ajustables en `src/engine/tuning.h`; nada de números mágicos sueltos.
- El repo es **público**: solo assets propios o CC0/OFL (apuntarlos en `ASSETS.md`). Nada de Nintendo ni ISOs.
- No pedir al usuario usar la SD (no tiene lector). Instalar con USB o mandar por WiFi.
- Al pedir algo al usuario en la Wii: instrucciones cortas y numeradas (lo lee en el móvil con la tele delante).

## Mantener el harness al día (obligatorio al terminar cada tarea)
1. `docs/ROADMAP.md`: marcar lo hecho y añadir lo nuevo pendiente.
2. `docs/DECISIONES.md`: cualquier decisión nueva del usuario (con fecha y motivo).
3. `docs/ARQUITECTURA.md` / `docs/NUEVO-JUEGO.md`: si cambia la estructura, un módulo o la forma de añadir cosas.
4. `docs/GESTOS.md`: si cambia un detector o sus constantes (con los aciertos medidos) y nuevos `tuning-data/`.
5. `docs/WII-NOTAS.md`: cada trampa nueva descubierta.
6. `docs/HARNESS.md` y las skills: si cambia un script, una escena de prueba (`START_SCENE`) o el flujo.
7. Si los docs y el código no coinciden, manda el código: corregir los docs.

## Entorno (Windows, torre del usuario)
- Proyecto en `D:\PROYECTOS\herri-kirolak`; devkitPro en `C:\devkitPro` (C: casi lleno: todo lo demás en D:).
- Dolphin portable en `D:\Tools\Dolphin\Dolphin-x64`.
- Wii en la WiFi de casa (MAC 78-a2-a0-91-11-36; `deploy.ps1` la encuentra). Torre por cable: 192.168.1.129.
- Blender con addon MCP disponible (no se usa: la geometría es procedural).

## Comandos rápidos
```
scripts/build.ps1                     # build/boot.dol
scripts/build.ps1 -Test -Scene 6      # build-test/boot.dol, escena directa + golpes con botones
scripts/run.ps1 [-Dol ruta]           # Dolphin
scripts/screenshot.ps1 -Name x        # build/screenshots/x.png
scripts/keys.ps1 X,DOWN,X             # teclas en Dolphin
scripts/deploy.ps1 [-Dol ruta]        # a la Wii (en caliente o wiiload)
scripts/install-usb.ps1 -Drive F      # instalar en el USB de la Wii
python tools/replay_detector.py tuning-data/*.csv
```
