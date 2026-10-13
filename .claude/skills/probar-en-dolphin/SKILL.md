---
name: probar-en-dolphin
description: Compila Herri Kirolak y lo prueba en Dolphin con capturas y teclas. Usar tras cualquier cambio de código, antes de mandarlo a la Wii o de hacer commit.
---

# Probar en Dolphin

1. Elegir la escena a probar (`START_SCENE` en `src/main.cpp`; lista en `docs/HARNESS.md`). Si lo nuevo no tiene
   escena directa, añadirla a `firstScene()` y documentarla en `docs/HARNESS.md`.
2. Compilar (PowerShell):
   ```
   & D:\PROYECTOS\herri-kirolak\scripts\build.ps1 -Test -Scene N
   ```
   Si falla, la salida de make está en el error; corregir y repetir.
3. Arrancar y capturar:
   ```
   & D:\PROYECTOS\herri-kirolak\scripts\run.ps1 -Dol D:\PROYECTOS\herri-kirolak\build-test\boot.dol
   Start-Sleep 6
   & D:\PROYECTOS\herri-kirolak\scripts\screenshot.ps1 -Name prueba
   ```
   Leer `D:\PROYECTOS\herri-kirolak\build\screenshots\prueba.png` con Read y revisarla de verdad (textos cortados,
   solapes, cosas fuera de pantalla, colores).
4. Si hace falta interactuar: `scripts/keys.ps1 X,DOWN,X` (X = A, Z = B, flechas, 1/2 = golpes de prueba,
   E = +, ENTER = HOME, K = punta arriba, M = swing). Varias capturas → unirlas en una cuadrícula con PIL.
5. Comprobar también la CPU: escena 6 (lehiaketa 4 CPU) y 7 (entrenamiento CPU), dejar 20–40 s y capturar.
6. Cerrar: `Get-Process Dolphin | Stop-Process -Force`.

Trampas: si las teclas no llegan, el overlay de Discord tiene el foco (ver `docs/WII-NOTAS.md`). Dolphin no puede
validar el tacto de los gestos (no gira más de 90°): eso se comprueba en la Wii con la skill `calibrar-gesto`.
