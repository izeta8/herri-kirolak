# Notas y trampas: Wii, libogc, Dolphin y Windows

## Wii del usuario
- Sistema **4.3E** (System Menu v514), Priiloader 0.3 (sin hacks.ini), Homebrew Channel, BootMii como IOS.
- WiFi: IP por DHCP (ha sido .155 y .135), **MAC 78-a2-a0-91-11-36**. `scripts/deploy.ps1` la busca sola.
- La WiFi de la Wii es floja: a veces no conecta (error 52032: reiniciar router/Wii) y a veces se atasca unos segundos.
- Tele 16:9. No sabe si 50 o 60 Hz: el juego vale para los dos.
- USB de la Wii: `ESD-USB` (FAT32, en el PC suele ser F:). El Homebrew Channel lee `apps/` del USB.

## libogc / red de IOS (aprendido a golpes)
- `net_init()` devuelve −EAGAIN varias veces al principio: reintentar (hasta ~20 s).
- **Un solo hilo de red.** Varios hilos con llamadas bloqueantes se bloquean entre sí.
- **Sockets de escucha no bloqueantes** (`net_fcntl(s, F_SETFL, flags | 4)`; el bit de IOS es 0x04, no `O_NONBLOCK`).
  Un `accept` bloqueante se queda colgado si alguien conecta y se va.
- **Conexiones aceptadas bloqueantes** (heredan el no bloqueante: quitarlo). Con envíos no bloqueantes la Wii
  **reenviaba datos viejos**.
- Antes de leer o enviar, preguntar con `net_poll` (timeout 0). Bits de IOS: POLLRDNORM 0x01, POLLRDBAND 0x02,
  POLLWRNORM 0x08, POLLERR 0x20, POLLHUP 0x40 (`POLLIN` = 0x03).
- `net_listen(s, 4)`: con cola 1, las comprobaciones de puerto del PC bloqueaban la conexión real.
- Enviar solo si `poll` dice que se puede: con la WiFi lenta un `send` bloqueante colgaba el hilo.
- Para saltar a otro DOL: cerrar sockets, `net_deinit`, copiar DOL a 0x92000000 y el cargador a 0x93000000,
  `SYS_ResetSystem(SYS_SHUTDOWN)`, desactivar interrupciones y saltar (ver `updater.cpp` y `tools/booter/`).

## libogc / gráficos y sistema
- `Vtx` ya existe en libogc: nuestra estructura de vértice se llama `Vert`.
- GX considera frontales las caras en sentido horario: usamos antihorario + `GX_CULL_FRONT`.
- Los datos embebidos con `bin2o` quedan alineados a 32; aun así, copiar texturas a memoria con `memalign(32)` y `DCFlushRange`.
- `CONF_GetAspectRatio()` para 16:9; con 16:9 se usa `viWidth = 678` y 2D virtual de 854 px de ancho.
- Salir con `return` de `main` vuelve al Homebrew Channel; HOME abre un diálogo propio ("Homebrew Channel-era itzuli?").

## Dolphin (D:\Tools\Dolphin\Dolphin-x64, portable)
- El mando emulado: X = A, Z = B, flechas = cruceta, 1/2, Q = −, E = +, Enter = HOME, I/K/J/L = inclinar, M/N/O = swing.
- **Al revés que el mando real**: "punta arriba" con K. No llega a 240°: la barra *Altuera* no se pone verde en Dolphin.
- `-b -e boot.dol` arranca directamente. Las teclas se inyectan con `scripts/keys.ps1` (necesita el foco).
- **El overlay de Discord** ("Usa Discord desde el overlay…") se queda con el foco y las teclas no llegan: desactivar
  el overlay de Discord para Dolphin o cerrarlo antes de probar.
- La primera vez que el juego abre puertos de red, Windows pregunta por el cortafuegos para Dolphin: no hace falta.
- La SD virtual sincronizada con carpeta no se rellena al arrancar un .dol suelto: el guardado en Dolphin no funciona.

## Windows / herramientas
- C: casi lleno: proyecto, Dolphin y descargas en D:. devkitPro en C:\devkitPro.
- Editar ficheros con heredocs de bash + Python rompe los `\n` de C (`"\n"` acaba como salto real): para cambios con
  `\n` usar la herramienta Edit o un script Python escrito con Write.
- `Set-Content` de PowerShell 5 escribe con BOM; gcc lo acepta, pero mejor `[IO.File]::WriteAllText`.
- El receptor de telemetría corre en segundo plano y se reconecta solo; si se cae, relanzarlo.
