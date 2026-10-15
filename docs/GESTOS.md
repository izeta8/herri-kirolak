# Gestos con el Wii Remote: método y datos

## Lo que el Wii Remote puede y no puede medir (sin MotionPlus ni puntero)
- **Acelerómetro de 3 ejes** a ~100 lecturas/s (se leen todas con `WPAD_ReadPending`, no una por frame).
- **Quieto**: solo mide la gravedad → sabe hacia dónde apunta (no el giro alrededor de la vertical).
- **Moviéndose**: gravedad + aceleración del movimiento mezcladas. Los movimientos fuertes se ven muy bien; los lentos
  casi no.
- **No sabe dónde está la mano.** Arriba a la derecha y arriba a la izquierda con la misma inclinación dan lecturas
  idénticas (comprobado con datos: diferencia nula). Cualquier "lado" se decide por el movimiento, no por la posición.
- **Satura en ~±5 g por eje** (|a| máx. ≈ 8,7 g): los golpes fuertes llegan al tope.
- Ejes del mando real: X a lo ancho (cruceta), Y a lo largo, Z hacia los botones. **Punta hacia arriba = gravedad en
  −Y** en el mando real; en Dolphin es al revés (para "subir" en Dolphin se usa la tecla K).

## Método para diseñar y calibrar un gesto (lo que funcionó con el hachazo)
1. **Definir el gesto por escrito con el usuario**: agarre + fases (p. ej. subir / bajar). Un gesto concreto y siempre
   igual es mucho más fácil de detectar que uno libre. Apuntarlo en `docs/DECISIONES.md`.
2. **Grabar datos reales etiquetados** con la telemetría (`docs/HARNESS.md`): en la pantalla de prueba del gesto, botón 1
   = **grabación guiada** (la pantalla pide el gesto a hacer y etiqueta cada uno). Mínimo ~15 por clase; mejor 30.
   Guardar el CSV en `tuning-data/AAAA-MM-DD-<qué>.csv`.
3. **Mirar los datos** antes de programar: `tools/analyze_gestures.py`, `tools/compare_sides.py`, o un script de rasgos
   (ver más abajo). Preguntas clave: ¿qué separa las clases?, ¿cuánto margen hay?, ¿cuándo pasa cada fase (ms)?
4. **Detector simple con rasgos físicos** (máquina de estados + pocos rasgos), constantes en `tuning.h`.
   Para separar clases, un discriminante lineal (LDA) con 2–4 rasgos y **validación dejando uno fuera**.
5. **Réplica en Python** (`tools/replay_detector.py`) idéntica a `input.cpp`: medir aciertos sobre todos los CSV
   antes de mandar nada a la Wii. Mantenerlas sincronizadas.
6. Probar en la Wii (actualización en caliente), recoger la impresión del usuario y volver a grabar si hace falta.

## El hachazo (aizkolaritza) — estado actual
Gesto oficial: pulgar en 1-2 · subir por un lado hasta "JO!" · bajar en diagonal al centro **sin girar la muñeca**.

| Fase | Cómo se detecta | Constantes (`tuning.h`) |
|---|---|---|
| Subir | ángulo = atan2(−ay, az) con la lectura directa (si 0,5 < \|a\| < 3 g) o la gravedad filtrada; arriba del todo son 240–270° | `RAISE_ON_DEG` 120°, `RAISE_HOLD` 50 ms acumulados, `RAISE_RAW_*` |
| Armado | estado ARMED (la barra *Altuera* se pone verde con "JO!") | — |
| Bajar | \|a\| > 3 g desde ARMED **o desde RAISING** (golpeando seguido no se para arriba) | `SWING_START_G` |
| Resolver | en el **pico**: pico ≥ `CHOP_MIN_G` y \|a\| < 0,85 × pico (≈30 ms tras el pico); si no, al bajar de 1,4 g | `PEAK_DROP`, `CHOP_MIN_G[3]` (sensibilidad) |
| Fuerza | (pico − mín) / (8,5 − mín) → 0,25..1; en el juego corta 0,3 + 0,7·fuerza² | `CHOP_FULL_G` |
| Lado | ∫ax e ∫ax de la 1.ª mitad, desde el inicio de la bajada hasta el pico: puntos = −1,72·∫ax − 10,74·∫ax₁ − 1,32 (> 0 = derecha) | `SIDE_W_*`, `SIDE_CUT` |

Resultados con la réplica: grabación guiada con el agarre oficial (`2026-10-13-gidatua3.csv`) **15/16**; el golpe se
resuelve 11–58 ms después del pico.

### Lo aprendido con el hachazo
- Primer intento ("punta arriba" = armado) fallaba: al subir un hacha el mando sigue girando hasta ~250°.
- La gravedad filtrada no ve la subida si el gesto es rápido → usar la lectura directa cuando \|a\| es razonable.
- Subiendo con energía, arriba del todo ya hay 2–3 g → la bajada empieza a 3 g, no a 2.
- El lado: la inclinación arriba no sirve; el empuje lateral sí, pero depende del agarre (con el agarre nuevo cambió de
  signo) → por eso el agarre está fijado y el clasificador se ajusta con datos del agarre oficial.
- Tiempos medidos: inicio de la bajada → pico ≈ 170 ms; → fin ≈ 350 ms. Resolver al final metía ≈ 180 ms de retraso
  (se notaba: "el hacha toca la madera antes que yo"). Ahora la animación espera justo encima y entra en el pico.
- Sin pausa en el impacto ni temblor, el golpe "no pesa".

## El latigazo (pala) — sin calibrar
Golpe de pala: un latigazo con el Wii Remote, **sin subirlo antes** (independiente del hachazo, en `stepWhip`).
Decide el **momento** (el pico es el instante del golpe) y la **fuerza**; el lado (derecha/revés) lo pone el juego.

| Fase | Cómo se detecta | Constantes (`tuning.h`) |
|---|---|---|
| Empieza | \|a\| > `WHIP_START_G` (2,2 g) → evento `whipStart` (para animar) | `WHIP_START_G` |
| Resolver | en el pico (\|a\| < 0,85 × pico) si el pico ≥ `WHIP_MIN_G` (2,6 g) → evento `whip` | `WHIP_MIN_G`, `PEAK_DROP` |
| Fuerza | (pico − 2,6) / (7,5 − 2,6) → 0..1 | `WHIP_FULL_G` |

Pantalla de prueba: Jokatu → Pala → Kolpe proba (`games/pala/whiptest.cpp`, escena 14): gráfica de los 3 ejes y de
\|a\| con las líneas de inicio y de golpe mínimo, fuerza y pico del último latigazo. **Botón 1 = grabación guiada**:
pide por turnos LEUN eskuinetik / GOGOR eskuinetik / EZKERRETIK (revés) y etiqueta las lecturas con `S` / `F` / `B`.

Valores puestos a ojo a partir del hachazo: **falta grabar latigazos reales** (suaves, normales, fuertes; de derecha
y de revés) y ajustar con datos. También: comprobar el retraso entre el golpe real y el evento.

### Datos (`tuning-data/`)
| Fichero | Contenido |
|---|---|
| `2026-10-13-eskuina.csv` | 6 gestos derecha (+/−), agarre antiguo |
| `2026-10-13-ezker-eskuin.csv` | 18 gestos etiquetados con ←/→, agarre antiguo, gestos sueltos |
| `2026-10-13-gidatua2.csv` | grabación guiada, golpes seguidos, agarre antiguo |
| `2026-10-13-gidatua3.csv` | grabación guiada con el **agarre oficial** (la que manda) |

Formato de cada línea: `mando,ms,ax,ay,az,gx,gy,gz,egoera,kolpea,botoiak,etiketa` (g = gravedad filtrada;
egoera 0 idle, 1 subiendo, 2 armado, 3 bajando, 4 enfriando; botoiak en hex; etiqueta R/L/-).
Las líneas `# conexion …` separan sesiones; puede haber líneas rotas (los scripts las descartan).
