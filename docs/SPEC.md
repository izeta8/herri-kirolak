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

## 6. Juego 2 — Pala (diseño v2 de habilidad, 16/10/2026)

### Cómo es en la vida real
Pelota vasca con **pala de madera** en frontón (frontis + pared izquierda; el lado derecho abierto, con la grada).
La pelota tiene que dar en el **frontis** entre la **chapa** de abajo y la línea de arriba (puede tocar antes la pared
izquierda) y **solo puede botar una vez** antes de devolverla (o se le da de volea). Saque: el pelotari bota la pelota y
la golpea; tiene que volver del frontis y caer entre la línea de **falta** y la de **pasa**. En la vida real se juega
sobre todo por parejas (delantero y zaguero) y a 35–40 tantos.

### Diseño v2: juego de HABILIDAD (16/10/2026)
El prototipo era demasiado permisivo (casi cualquier latigazo devolvía la pelota). Objetivo: que ganar dependa de
**colocarse bien, golpear en el momento justo y elegir cuándo arriesgar**, como de verdad.

**Base que se mantiene**: 1v1 primero (luego 2v2) · moverse con el **Nunchuk (obligatorio)** · golpe = **latigazo**
del Wii Remote · derecha/revés automático según el lado de la pelota · saque con **A (bota) + latigazo** · partidos a
**10/15/22** · frontón provisional ahora, **Zarautz** después.

**1. Punto dulce (tiempo + sitio).** Cada golpe tiene una *calidad* 0..1 = tiempo × sitio:
- *Tiempo*: diferencia entre el pico del latigazo y el instante ideal (pelota a la altura de la pala). Perfecto
  ±`SWEET_TIME` (≈40 ms); fuera de eso cae hasta 0 en `HIT_EARLY/LATE`.
- *Sitio*: distancia horizontal pelota–pala respecto a la ideal (`SWEET_DIST`, el largo del brazo + pala). Muy cerca
  (encima) o muy lejos (estirándose) = peor.
- Calidad alta → la pelota va a donde se apunta, rápida y baja. Calidad baja → sale floja, alta y desviada; muy baja →
  chapa o fuera. Se ve en pantalla: *Bikain! / Ondo / Txarto* + un destello en la pala.

**2. Apuntar con Z + stick (16/10, idea del usuario).** El stick **mueve** al pelotari por la cancha (tiene que estar
físicamente bien colocado para llegar a la pelota). **Manteniendo Z**, el pelotari se planta y el stick mueve una
**mira sobre las paredes**: un recorrido continuo que va por la **pared izquierda** (desde el cuadro 4 hasta el txoko) y
sigue por el **frontis** hasta la raya del lado abierto. Moverse y apuntar no se pueden hacer a la vez.
- Si la mira está en la pared, la pelota da primero en ella y luego en el frontis: **bipareta**, sale muy desviada
  hacia el ancho. Cuanto más cerca del txoko, más desvío.
- Sin Z la mira se queda donde estaba (al principio, en el centro del frontis).
- La mira solo marca **adónde** (en horizontal); la altura en la pared sale del tipo de golpe y de la fuerza.

**3. Tipo de golpe con los botones del Wii Remote (16/10, del usuario)**, mantenidos al hacer el latigazo:
- **B: dejada** (bota cerca del frontis) · **sin botón: a media cancha** · **A: tiro largo** (al fondo del frontón).
- En el saque: B = justo pasada la falta, sin botón = al medio, A = cerca de la pasa (arriesgado).
- La **fuerza** (pico de aceleración del latigazo) da la **velocidad**. Una dejada fuerte va baja y rápida: riesgo de
  chapa. Un tiro largo flojo se queda corto. **Riesgo con la fuerza**: cuanto más fuerte, más pequeño el punto dulce.

**4. Volea y bote pronto.** Darle **antes del bote** (volea) o **justo al botar** (bote pronto, <~80 ms tras el bote)
tiene una ventana más corta, pero la pelota sale más rápida y el rival tiene menos tiempo. Se anuncia (*Bolea!*,
*Bote pronto!*).

**5. Peloteo que se acelera.** Cada golpe bueno del peloteo sube la velocidad base un poco (`RALLY_SPEEDUP`, con tope);
en el tanto siguiente vuelve a la base.

**6. CPU por niveles.** Erraza / Normala / Zaila (se elige al preparar el partido). El nivel cambia su error de tiempo,
su velocidad, lo que arriesga (chapa, pared) y lo bien que se coloca. La difícil busca la pared y la chapa.

**7. Cámara isométrica, mucho más cerca (16/10).** Fija en diagonal desde **atrás y a la derecha** (lado de la grada),
en alto, con FOV estrecho; **no gira**. **Sigue a la pelota a lo largo de la cancha**: cuanto más cerca está la pelota
del frontis, más cerca del frontis está la cámara (y más cerrada); cuanto más se aleja, más se aleja la cámara. Tira
un poco hacia el pelotari humano para que no se pierda de vista.

Se mantiene la **marca de bote** (ayuda) de momento.

### Reglas (sin cambios)
- Tanto perdido si la pelota: da en la chapa o por debajo, por encima de la línea de arriba, fuera de la cancha o del
  lado abierto antes de botar, bota dos veces, o no llega al frontis.
- Saca el que ganó el tanto anterior. Saque a un bote entre falta y pasa.

### Modos
1. **Entrenamendua**: peloteo contra el frontis con tarjeta "Nola jo"; cuenta la racha.
2. **Lehiaketa**: 1v1 humano contra humano o contra la CPU (con nivel). Resultados y récord.

## 7. Otros juegos (después del MVP, uno por uno)
Harri-jasotzea, Sokatira, Trontza, Txingak, Lasto altxatzea, Estropadak, Segalariak.

## 8. Fuera de alcance por ahora
MotionPlus, online, otros idiomas. (El puntero IR, la música y los Miis reales están planificados en el ROADMAP.)
