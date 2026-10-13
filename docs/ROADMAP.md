# Roadmap

Tareas medianas, en orden. Marcar `[x]` al terminar (con commit + push).

## Fase 0 — Entorno
- [ ] devkitPro (wii-dev) instalado en `D:\devkitPro`, variables `DEVKITPRO`/`DEVKITPPC` configuradas
- [x] Dolphin portable en `D:\Tools\Dolphin\Dolphin-x64`
- [ ] Repo público `izeta8/herri-kirolak` creado y con push
- [ ] Scripts: `scripts/build.ps1`, `scripts/run.ps1` (Dolphin), `scripts/screenshot.ps1`, `scripts/deploy.ps1` (wiiload)

## Fase 1 — Hola mundo
- [ ] Makefile libogc que genera `boot.dol`; arranca en Dolphin y muestra un cubo 3D con GX
- [ ] Probado en Wii real vía Homebrew Channel (`hbc/herri-kirolak/` con `meta.xml` + `icon.png`)
- [ ] Botón HOME vuelve al Homebrew Channel

## Fase 2 — Motor base
- [ ] Bucle principal, timing fijo, gestor de escenas (título, menú, juego, resultados)
- [ ] Render: cámara, luz difusa + ambiente, meshes, colores planos, 16:9/4:3
- [ ] Pipeline de modelos: Blender → exportador Python → formato binario propio → embebido en el `.dol`
- [ ] Texturas → TPL/GX (CMPR/RGB5A3)
- [ ] Fuente bitmap redondeada (OFL) con caracteres euskera; renderizado de texto 2D
- [ ] Entrada: WPAD para 4 mandos, aceleración filtrada, detector de gestos (levantar/golpear, giro de muñeca)
- [ ] Audio: ASND, cargar WAV/PCM para efectos
- [ ] UI: botones estilo Wii navegables con cruceta, fundidos
- [ ] Guardado en SD (libfat)

## Fase 3 — Personajes
- [ ] Modelo de personaje "tipo Mii" (cabeza, cuerpo, manos flotantes, txapela, gerriko)
- [ ] Animaciones procedurales: idle, levantar hacha, golpe, giro, celebración
- [ ] Editor sencillo de personaje (color, txapela, nombre)

## Fase 4 — Aizkolaritza (MVP)
- [ ] Escenario: plaza, tronco, público
- [ ] Tronco deformable con corte en V (profundidad por lado), astillas como partículas
- [ ] Mecánica de golpe (fuerza, ángulo ezker/eskuin, calidad, cansancio)
- [ ] "Buelta eman!" y segunda V, el tronco se parte
- [ ] Modo Entrenamendua
- [ ] Modo Lehiaketa 1–4 jugadores por turnos + resultados + récords
- [ ] Ajuste fino en Wii real con mandos de verdad

## Más adelante
- [ ] Miis reales
- [ ] Música
- [ ] Siguientes juegos (ver SPEC §6)
