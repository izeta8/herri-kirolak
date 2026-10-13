---
name: mandar-a-wii
description: Manda la versión compilada de Herri Kirolak a la Wii real del usuario por WiFi (actualización en caliente o wiiload) o la instala en su USB. Usar cuando el usuario vaya a probar en la Wii.
---

# Mandar a la Wii

1. Compilar la versión adecuada:
   - para jugar: `scripts/build.ps1` → `build/boot.dol`
   - para arrancar en una escena sin golpes de prueba (p. ej. Kolpe proba), en el bash de devkitPro:
     `make -j8 OUTDIR=build-rec DEFS='-DSTART_SCENE=8'` → `build-rec/boot.dol`
2. Mandar:
   ```
   & D:\PROYECTOS\herri-kirolak\scripts\deploy.ps1 [-Dol ruta]
   ```
   - "actualizando en caliente … OK": el juego estaba abierto y ya arranca la versión nueva.
   - "Homebrew Channel … wiiload … done": fue por el HBC.
   - "No encuentro la Wii": pedir al usuario (instrucciones cortas, lo lee en el móvil) que abra el **Homebrew
     Channel con la WiFi conectada** y avise; reintentar.
   - "transfer failed" a medias: la WiFi de la Wii se atascó; reintentar una vez.
3. Si la red del juego se colgó (no responde 4301 pero sí ping), la única salida es HOME → Bai → Homebrew Channel
   y mandar por wiiload. Investigar la causa con las notas `#` de la telemetría.
4. Telemetría: si se va a probar el tacto, lanzar el receptor en segundo plano
   (`python -u tools/telemetry.py <IP> build/telemetria.csv >> build/telemetria.log`) y comprobar que llegan datos.
5. Instalar para jugar sin PC (USB `ESD-USB` conectado a la torre, normalmente F:):
   `scripts/install-usb.ps1 -Drive F` y verificar con hash que el boot.dol copiado es el mismo.
