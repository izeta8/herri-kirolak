# Herri Kirolak Wii — Especificación

Juego homebrew para **Nintendo Wii real** (vía Homebrew Channel), estilo **Wii Sports**, con deportes rurales vascos (herri kirolak). Proyecto personal para jugar con amigos.

## 1. Plataforma y restricciones

| Tema | Decisión |
|---|---|
| Hardware objetivo | Wii real (PAL/NTSC), lanzado desde Homebrew Channel (`SD:/apps/herri-kirolak/boot.dol`) |
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
- **Sin puntero IR**: toda la navegación de menús con **cruceta + A (aceptar) + B (atrás)**. Selección resaltada estilo Wii (botón que crece/brilla).
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

Guardado: `SD:/apps/herri-kirolak/save.dat` (personajes + récords). Si no hay SD, jugar sin guardar.

## 5. Juego 1 — Aizkolaritza (MVP)

### Cómo es en la vida real
El aizkolari se sube **encima de un tronco tumbado** y corta **entre sus pies**. Hace un **corte en V**: golpes alternos en diagonal, uno desde un lado y otro desde el otro, sacando astillas. Cuando llega más o menos a la mitad del tronco, **se da la vuelta** y hace otra V por el lado contrario hasta que el tronco se parte en dos. En las competiciones se cortan varios troncos y gana el que tarde menos.

### Mecánica
- **Golpe**: levantar el Wiimote por encima de la cabeza y bajarlo con fuerza. La **fuerza** sale del pico de aceleración y la **precisión** del momento del impacto.
- **Ángulo del golpe (la V)**: el giro de muñeca del Wiimote en el momento del golpe decide si el corte es **ezkerra** (diagonal izquierda) o **eskuina** (diagonal derecha).
  - Alternar izquierda/derecha → la V avanza bien y salen astillas grandes.
  - Repetir el mismo lado → avanza poco ("golpe malo").
  - Indicador en pantalla de qué lado toca y de la calidad del golpe (*Bikain! / Ondo / Txarto*).
- **Ritmo/cansancio**: golpear demasiado rápido sin levantar bien baja la precisión. Hay una barra de *nekea* (cansancio) que se recupera al bajar el ritmo.
- **Darse la vuelta**: cuando la V frontal llega a ~50 % de profundidad aparece **"Buelta eman!"**. Se pulsa B (o un gesto) y la animación gira al aizkolari para atacar el otro lado.
- **Fin del tronco**: cuando las dos V se juntan el tronco se parte (animación + astillas + público). Se pasa al siguiente tronco o se termina.
- Visual: la V se ve de verdad en la malla del tronco (profundidad por lado actualizada en tiempo real) y vuelan astillas como partículas.

### Modos (MVP)
1. **Entrenamendua** (práctica): 1 tronco, sin límite, con consejos en pantalla.
2. **Lehiaketa** (competición): 1–4 jugadores **por turnos** (como los bolos de Wii Sports). Cada uno corta N troncos (configurable 1–3) contrarreloj. Gana el mejor tiempo. Hay récord personal.

### Futuro (no MVP)
- 2 jugadores a la vez a pantalla partida.
- Rival de la CPU (fantasma).
- Distintos troncos (grosor y tipo de madera).

## 6. Otros juegos (después del MVP, uno por uno)
Harri-jasotzea, Sokatira, Trontza, Txingak, Lasto altxatzea, Estropadak, Segalariak.

## 7. Fuera de alcance por ahora
Puntero IR, MotionPlus, música, Miis reales, online, otros idiomas.
