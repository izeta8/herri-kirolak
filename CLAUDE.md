# Herri Kirolak Wii — instrucciones para Claude

Juego homebrew de Wii estilo Wii Sports con deportes rurales vascos. Lee **docs/SPEC.md** (qué se construye) y **docs/ROADMAP.md** (por dónde vamos) antes de empezar cualquier tarea.

## Forma de trabajar
- El usuario habla castellano; los textos del juego van en **euskera**.
- Tareas medianas: coge el siguiente punto sin hacer de ROADMAP.md, impleméntalo, compílalo, pruébalo en Dolphin (con captura de pantalla) y luego haz commit + push y marca el punto en el roadmap.
- Lo que solo se puede comprobar en la Wii real (cómo se sienten los gestos, el rendimiento) se apunta en la sección "Pendiente de probar en Wii" del final, para que lo pruebe el usuario.
- El repo es **público**: solo assets propios o con licencia CC0/OFL (apúntalos en `ASSETS.md`). Nada de assets de Nintendo ni ISOs.

## Entorno (Windows)
- devkitPro: `D:\devkitPro` (entorno de devkitPro: `DEVKITPRO=/opt/devkitpro`, `DEVKITPPC=/opt/devkitpro/devkitPPC`)
- Dolphin: `D:\Tools\Dolphin\Dolphin-x64\Dolphin.exe` (portable; config en `User/` junto al exe)
- Blender: abierto con el addon MCP (herramientas `mcp__blender__*`) para crear modelos
- Disco C: casi lleno: todo va en D:

## Comandos
- Compilar: `scripts/build.ps1` → `build/boot.dol`
- Ejecutar en Dolphin: `scripts/run.ps1`
- Captura de Dolphin: `scripts/screenshot.ps1`
- Mandar a la Wii: `scripts/deploy.ps1` (wiiload; IP de la Wii en `$env:WIILOAD = "tcp:192.168.x.x"`)

## Pendiente de probar en Wii
(nada todavía)
