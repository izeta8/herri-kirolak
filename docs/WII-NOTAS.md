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
- Si el usuario está usando el PC, capturar copiando la pantalla saca **su** ventana: `screenshot.ps1` usa PrintWindow
  (funciona con Dolphin tapado). Las teclas de `keys.ps1` no llegan si Dolphin no tiene el foco: no robárselo.
- La primera vez que el juego abre puertos de red, Windows pregunta por el cortafuegos para Dolphin: no hace falta.
- La SD virtual sincronizada con carpeta no se rellena al arrancar un .dol suelto: el guardado en Dolphin no funciona.

## Miis
- La Wii **no** guarda las piezas de los Miis (`RFL_Res.dat`) en la NAND: solo `RFL_DB.dat` en
  `/shared2/menu/FaceLib/`. Cada juego trae su copia (Wii Sports Resort: `Common/Kokeshi/common.carc`).
- `ISFS_ReadDir` con lista nula para contar entradas colgó el juego: usar siempre un búfer fijo.
- Las gafas son media textura: `GX_MIRROR` en S. La cara a 128 px se ve borrosa de cerca (jugadores: 256).
- Wii Sports Resort en Dolphin pide quitar el Nunchuk (`WiimoteNew.ini`, `Extension = None`) y se maneja apuntando
  con el ratón (D-pad no mueve el menú). Sirve de referencia para comparar Miis. Devolver luego `Extension = Nunchuk`.
- Cuerpos de Resort (`Common/Kokeshi/common.carc` → `G3D/WS2_ava*.brres`, MDL0 v11): A* y B* con brazos (28 huesos;
  A1/A5/A8/A11/B8/B11 en pose de T), C* sin brazos y con manos sueltas; `WS2_ava_hand*` manos aparte. `clothes.brres`
  solo trae calcomanías (logo de Nintendo, flores), no el color: la ropa se colorea por zonas. Unidades: hombros a
  9.25, cuello (`head_set`) a 9.7. Casi todo rígido; triángulos entre huesos solo en dedos y cuello. Las mallas en
  unidades grandes queman la luz: corregir la escala de las normales (`loadModelMtx(xf, escala)`).
- **Los Miis de Resort no tienen brazos**: manos sueltas (bola) junto al torso en pera; el público, torso de bala sin
  piernas con la base oscura y manos del color de la camiseta. A1 (con brazos y dedos) es el del paracaidismo.
  Proporciones medidas en los bolos: cara ≈ ancho del torso (3.9 u), camiseta hasta 4.5 u, piernas 4.3 u.
- Navegar Resort en Dolphin: `IR/Auto-Hide = True` en `WiimoteNew.ini` (sin Nunchuk); el puntero solo sigue al ratón
  si se mueve poco a poco (`SetCursorPos` a saltos lo deja pegado al borde); en la elección de Mii, la **cruceta**
  (`keys.ps1 DOWN`, con la marca de tecla extendida) mueve la mano. Los bolos dan la mejor vista de cuerpo entero.
- **Material de los Miis de Resort** (sacado de los `.brres` y de las decompilaciones públicas ogws —Wii Sports— y
  zeldaret/ss —EGG—; nada de su código va al repo): TEV del cuerpo = `lerp(texA.rgb, C0 favorito, texA.a)` →
  `lerp(·, C1 piel, texB.i)` → C2; luego `C2 × (lm0·RASA + RASC) + lm1·RASA`, con RASC = ambiente (100 en el lightset
  de los Miis) y RASA = alfa del vértice (oclusión horneada). lm0/lm1 son `EGG::LightTexture`: matcaps de 32×32
  pintadas cada frame; cada luz suma `lerp(negro, color·intensidad, degradado[32 + 31·n·L])` (10 degradados de 64
  texels, ver `game/mii/light.cpp`). Luces y lightmaps de cada escena en `Common/<Escena>/Env/*.plight` y `*.plmap`
  (formatos LGHT/LOBJ y LMAP/LTEX de EGG); el lightset de los Miis en el SCN0 de la escena ("Abatar_Lighset").
- **Los modelos de Nintendo van en orden horario** (el frontal de GX; sus materiales dicen "cull back") y el motor
  dibuja lo suyo al revés (`GX_CULL_FRONT`): los `.brres` hay que dibujarlos con `GX_CULL_BACK` (y la mano en espejo con
  FRONT). Con el del motor se pintaba el interior y los cuerpos parecían transparentes en la cintura.
- **GX escala las coordenadas de textura por coordenada, no por textura**: dos texturas de tamaño distinto no pueden
  compartir `GX_TEXCOORDn` (la ropa salía revuelta). Resort usa coordenadas separadas para texA (64×64) y texB (16×16).
- En A6/B6 el pantalón, la cadera y las piernas tienen **una sola UV por hueso** (color plano de una tira de muestras
  de la textura); solo la camisa y los zapatos tienen UV de verdad. Para pintar ropa propia hay que dar UV nuevas
  (lo hace `tools/mii_outfit.py`).
- Ropa: `clothes.brres` trae, por cuerpo, `_a` (dibujos claros), `_aw`/`_ay`/`_awy` (dibujos oscuros, para el favorito
  blanco 10, amarillo 2 o verde lima 3) y `_b` (máscara de piel). Bolos: A6 (camisa hawaiana abierta, pantalón corto
  gris) y B6. La mano tiene una sola UV, que cae en la piel de la máscara. El público de los partidos es otro modelo
  (`Audience/G3D/WS2_common_avaC1.brres`) con la base casi negra.
- **`main.dol` de Resort** (7,3 MB, todo el juego): sale con `DolphinTool extract -i RZTP01.iso -p DATA -o carpeta`
  (queda en `DATA/sys/`; con `-s sys/main.dol` no sale) y se desensambla con el objdump de devkitPPC
  (`powerpc-eabi-objdump -D -b binary -m powerpc:750 -M 750cl -EB --adjust-vma=…` por sección). r2 = 0x806FE720.
  Hallazgos: gestor de cuerpos en 0x80245EA0–0x80246CC8 (`GetBodyFileName` 0x8024604C con `G3D/WS2_ava%s.brres`,
  `GetBodyModelIndex` 0x80246328 —solo tipo y sexo—, `GetClothesName` 0x80245FF4, variante por color 0x80245FCC),
  escala de la cabeza RFL 0.08 (constructor 0x80246BE0). **Altura y complexión no se leen en ningún sitio**.
  Desensamblado y archivos sueltos en `D:\Tools\wsr-tmp\re`, `full` (partición entera, 0,8 GB), `bwl` y `arc`.
- Copia del disco en `D:\Tools\wsr-tmp` (ver su `LEEME.txt`): ISO para comparar en Dolphin y volver a sacar piezas.
  Se puede borrar cuando los Miis (cabeza y cuerpo) estén aprobados en la Wii; `cleanrip-usb\` es redundante ya.

## Windows / herramientas
- C: casi lleno: proyecto, Dolphin y descargas en D:. devkitPro en C:\devkitPro.
- Editar ficheros con heredocs de bash + Python rompe los `\n` de C (`"\n"` acaba como salto real): para cambios con
  `\n` usar la herramienta Edit o un script Python escrito con Write.
- `Set-Content` de PowerShell 5 escribe con BOM; gcc lo acepta, pero mejor `[IO.File]::WriteAllText`.
- El receptor de telemetría corre en segundo plano y se reconecta solo; si se cae, relanzarlo.
