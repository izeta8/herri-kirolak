# Herri Kirolak Wii — Especificación

Juego homebrew para **Nintendo Wii real** (vía Homebrew Channel), estilo **Wii Sports**, con deportes rurales vascos (herri kirolak). Proyecto personal para jugar con amigos.

## 1. Plataforma y restricciones

| Tema | Decisión |
|---|---|
| Hardware objetivo | Wii real (PAL/NTSC), lanzado desde Homebrew Channel. El usuario **no puede usar SD** en el PC: se instala en **USB** (`usb:/apps/herri-kirolak/boot.dol`, FAT32) o se envía por red con wiiload. Soportar SD y USB igualmente |
| Toolchain | devkitPro → devkitPPC + libogc (GX, WPAD, ASND, libfat). Sin SDK oficial de Nintendo |
| Lenguaje | C++17 (sin excepciones ni RTTI), algo de C donde convenga |
| Emulador / visor | Dolphin (`D:\Tools\Dolphin\Dolphin-x64`, modo portable) |
| Despliegue rápido | `wiiload` por red al Homebrew Channel |
| Vídeo | 640×480 (480i/480p según config de la Wii), 4:3 y 16:9 (leer `CONF_GetAspectRatio`) |
| Memoria | ~88 MB total (24 MB MEM1 + 64 MB MEM2). Modelos low-poly, texturas ≤256×256 en formatos GX (CMPR/RGB5A3/I8) |
| Rendimiento | 60 fps objetivo (50 Hz PAL aceptable) |
| Idioma | **Euskera** (todos los textos). Textos centralizados en una tabla para poder añadir más idiomas después |
| Audio | MVP: solo efectos (golpes de hacha, astillas, público, campana). **Sin música** por ahora |
| Repo | Público en GitHub: `izeta8/herri-kirolak`. **Solo assets propios o con licencia libre (CC0/OFL)**. Nunca ISOs ni assets extraídos de juegos de Nintendo |

## 2. Controles

- **Mando**: Wiimote solo (acelerómetro). Nunchuk opcional en futuros juegos.
- **Menús con cruceta y puntero IR**: ahora funciona la **cruceta + A (aceptar) + B (atrás)**; el **puntero** se añadirá (las dos formas a la vez, como en Wii Sports). Selección resaltada estilo Wii (botón que crece/brilla).
- **Sin MotionPlus**.
- Hasta **4 jugadores**, cada uno con su Wiimote.
- **Modo debug de entrada** (para probar en Dolphin sin mando real): con un botón/tecla se simula un golpe completo con fuerza y ángulo configurables. Activable por flag de compilación y por menú oculto.

## 3. Estilo visual (tipo Wii Sports)

- **Personajes "tipo Mii"** propios: cabeza grande (esfera con cara simple: ojos, cejas, boca), cuerpo cápsula, **manos flotantes sin brazos**, piernas cortas. Colores planos, sombreado suave (iluminación difusa + ambiente), sin texturas realistas.
- Personalización MVP: color favorito (camiseta/gerriko), txapela sí/no, nombre.
- **Futuro**: cargar Miis reales de la Wii (leer base de datos de Miis de la NAND y renderizar caras).
- Escenario: plaza de pueblo vasco/frontón, hierba, cielo azul con nubes, público low-poly animado.
- UI: fondos blancos/azul claro, botones redondeados con borde, tipografía redondeada (fuente OFL, p.ej. Nunito / M PLUS Rounded), transiciones suaves con fundidos.
- Cámara: tercera persona cercana, estilo Wii Sports.

## 4. Estructura del juego

```
Pantalla título (Herri Kirolak) → "Sakatu A" 
  └ Menú principal
      ├ Jokatu (jugar) → elegir juego (MVP: solo Aizkolaritza)
      │    └ Jokalari kopurua (1–4) → elegir personajes → partida → resultados
      ├ Jokalariak (crear/editar personajes)
      ├ Errekorrak (récords guardados en SD)
      └ Irten (volver al Homebrew Channel)
```

Guardado: `save.dat` en la misma carpeta desde la que se lanzó el juego (`argv[0]`: `usb:/apps/herri-kirolak/` o `sd:/apps/herri-kirolak/`), montando con `fatInitDefault()`. Si se lanzó por wiiload (sin ruta), probar `usb:` y luego `sd:`. Si no hay almacenamiento, jugar sin guardar.

## 5. Juego 1 — Aizkolaritza (MVP)

### Cómo es en la vida real
El aizkolari se sube **encima de un tronco tumbado** y corta **entre sus pies**. Hace un **corte en V**: golpes alternos en diagonal, uno desde un lado y otro desde el otro, sacando astillas. Cuando llega más o menos a la mitad del tronco, **se da la vuelta** y hace otra V por el lado contrario hasta que el tronco se parte en dos. En las competiciones se cortan varios troncos y gana el que tarde menos.

### El gesto (definido el 13/10/2026 con el usuario)
1. **Agarre**: el Wii Remote cogido por la parte de abajo, con el **pulgar encima de los botones 1 y 2**, como el mango de un hacha.
2. **Subir**: por encima de la cabeza, por el lado del corte (arriba a la derecha o arriba a la izquierda). El mando acaba por encima del hombro (ángulo de subida > 120°). En pantalla, la barra *Altuera* se pone verde con **"JO!"**.
3. **Bajar**: en diagonal hacia el centro (la V) **sin girar la muñeca**: el mando mantiene su orientación y es la mano la que hace la diagonal. Así el movimiento lateral cae siempre en el eje X del mando.
   - Desde arriba a la derecha → **eskuina**; desde arriba a la izquierda → **ezkerra**.
   - La **fuerza** sale del pico de aceleración.
- Limitación: con solo acelerómetro (sin MotionPlus) no se puede saber la posición de la mano antes de bajar; el lado se decide **durante la bajada**. Por eso la pantalla indica qué lado toca y confirma el detectado al golpear.

### Mecánica
- **La V**: alternar izquierda/derecha → la V avanza bien y salen astillas grandes. Repetir el mismo lado → avanza poco. Flecha grande en pantalla con el lado que toca y valoración del golpe (*Bikain! / Ondo! / Ahul...*).
- **Cansancio**: desactivado de momento (se recuperaba demasiado rápido para notarse).
- **Darse la vuelta**: cuando la V frontal llega a ~50 % de profundidad aparece **"Buelta eman!"**. Se pulsa B (o un gesto) y la animación gira al aizkolari para atacar el otro lado.
- **Fin del tronco**: cuando las dos V se juntan el tronco se parte (animación + astillas + público). Se pasa al siguiente tronco o se termina.
- Visual: la V se ve de verdad en la malla del tronco (profundidad por lado actualizada en tiempo real) y vuelan astillas como partículas.

### Modos (MVP)
1. **Entrenamendua** (práctica): 1 tronco, sin límite, con consejos en pantalla.
2. **Lehiaketa** (competición): 1–4 jugadores **a la vez**, cada uno con su Wii Remote, su aizkolari y su tronco en fila en la plaza (como en las competiciones reales). Cada uno corta N troncos (1–3); gana quien acabe antes. Cámara fija que encuadra a todos y un panel por jugador abajo. Puede haber jugadores CPU. Récord por número de troncos.

### Futuro (no MVP)
- 2 jugadores a la vez a pantalla partida.
- Rival de la CPU (fantasma).
- Distintos troncos (grosor y tipo de madera).

## 6. Juego 2 — Palas (en diseño, 15/10/2026)

### Cómo es en la vida real
Pelota vasca con **pala de madera** en frontón (frontis + pared izquierda; el lado derecho abierto, con la grada).
La pelota tiene que dar en el **frontis** entre la **chapa** de abajo y la línea de arriba (puede tocar antes la pared
izquierda) y **solo puede botar una vez** antes de devolverla (o se le da de volea). Saque: el pelotari bota la pelota y
la golpea; tiene que volver del frontis y caer entre la línea de **falta** y la de **pasa**. En la vida real se juega
sobre todo por parejas (delantero y zaguero) y a 35–40 tantos.

### Lo que hemos decidido
- **1v1 primero** (mano a mano); después se amplía a **2v2**.
- **Moverse con el nunchuk** (stick); el Wii Remote, en la otra mano, es la pala. **Nunchuk obligatorio**: sin él no
  se juega a palas y se avisa en pantalla. La CPU no lo necesita.
- **Golpe**: el **latigazo** del Wii Remote. Derecha o revés sale solo según por dónde viene la pelota. El gesto decide:
  - **el momento**: pronto = pelota cruzada hacia el ancho (lado abierto); tarde = pegada a la pared izquierda;
  - **la fuerza**: latigazo fuerte = pelota rápida y larga; suave = dejada.
- **Saque**: pulsar **A** → el pelotari bota la pelota → **latigazo** cuando sube. Mal hecho (corto/largo) = falta.
- **Partido a 10, 15 o 22 tantos** (se elige al empezar, como las rondas de aizkolaritza).
- **Cámara detrás, viendo el frontis**, algo alta (como desde la grada del fondo). Los dos miran al mismo lado: una
  sola pantalla, sin partirla.
- Sitio: **frontón de Zarautz** (más adelante; primero un frontón provisional para el prototipo).

### Reglas del prototipo (propuesta, ajustable)
- Tanto perdido si la pelota: da en la chapa o por debajo, por encima de la línea de arriba, fuera de la cancha o del
  lado abierto antes de botar, bota dos veces, o no llega al frontis.
- Saca el que ganó el tanto anterior (como en pelota). Saque a un bote entre falta y pasa.

### Modos
1. **Entrenamendua**: peloteo contra el frontis (sin rival) con tarjeta "Nola jo".
2. **Lehiaketa**: 1v1 humano contra humano o contra la CPU. Resultados y récord (tantos de diferencia / racha).

## 7. Otros juegos (después del MVP, uno por uno)
Harri-jasotzea, Sokatira, Trontza, Txingak, Lasto altxatzea, Estropadak, Segalariak.

## 8. Fuera de alcance por ahora
MotionPlus, online, otros idiomas. (El puntero IR, la música y los Miis reales están planificados en el ROADMAP.)
