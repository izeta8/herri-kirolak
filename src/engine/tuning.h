#pragma once
// Constantes de ajuste fino. Casi todo esto hay que afinarlo probando con un Wiimote real.
namespace tuning {

// --- Detector de hachazo. Valores sacados de gestos reales grabados por telemetría (tuning-data/) ---
// Subir: el mando gira sobre su eje X desde "apuntando a la tele" (0°) hasta quedar por encima
// del hombro (unos 240-270°: la punta hacia abajo por detrás). Se mide con la lectura directa del
// acelerómetro, porque la gravedad filtrada no llega a verlo si el gesto es rápido.
constexpr float RAISE_SIGN = -1.0f;     // en el mando real, punta arriba = gravedad en -Y
                                        // (en Dolphin es al revés: para "subir" usar la tecla K, no I)
constexpr float RAISE_ON_DEG = 120.0f;  // ángulo para considerar el hacha arriba
constexpr float RAISE_OFF_DEG = 70.0f;  // con la gravedad filtrada por debajo: deja de estar armado
constexpr float RAISE_HOLD = 0.05f;     // s acumulados por encima de RAISE_ON_DEG para armar
constexpr float RAISE_RAW_MIN_G = 0.5f; // la lectura directa solo cuenta si |a| está entre estos valores
constexpr float RAISE_RAW_MAX_G = 3.0f;  // golpeando seguido, arriba del todo el mando ya va a 2-2.5 g
// Bajar: los hachazos reales dan picos de 7-8.7 g (el mando satura en ~5 g por eje);
// subir el mando no pasa de ~2.4 g
constexpr float SWING_START_G = 3.0f;   // |a| que inicia la bajada (arriba del todo puede haber 2.5 g)
constexpr float SWING_END_G = 1.4f;     // |a| por debajo del cual la bajada ha terminado
constexpr float SWING_MAX_TIME = 0.45f; // s
constexpr float CHOP_MIN_G[3] = {2.6f, 3.6f, 4.6f};  // pico mínimo según sensibilidad (suave/normal/fuerte)
constexpr float CHOP_FULL_G = 8.5f;     // pico que da fuerza máxima (los hachazos reales dan 5-8.8 g)
constexpr float CHOP_COOLDOWN = 0.2f;   // s
// El golpe se resuelve en el pico de fuerza (cuando |a| baja de PEAK_DROP × pico), que es cuando el
// jugador "golpea" (≈170 ms tras empezar a bajar); esperar al final de la bajada metía ≈180 ms de retraso.
constexpr float PEAK_DROP = 0.85f;
// Lado de la V con el gesto oficial (pulgar en 1-2, bajar en diagonal sin girar la muñeca), con la
// aceleración X desde que empieza la bajada (|a| > 3 g) hasta el pico. Desde la derecha la mano acelera
// primero hacia un lado; desde la izquierda, hacia el otro. Ajustado con tuning-data/2026-10-13-gidatua3.csv
// (15 de 16 en validación cruzada):
//   puntos = W_IX·∫ax + W_IX1·∫ax(1ª mitad) + W_MINX·min(ax) + W_MAXX·max(ax) - CUT   (> 0 = derecha)
constexpr float SIDE_W_IX = -1.72f;
constexpr float SIDE_W_IX1 = -10.74f;
constexpr float SIDE_W_MINX = 0.0f;
constexpr float SIDE_W_MAXX = 0.0f;
constexpr float SIDE_CUT = 1.32f;
constexpr float SIDE_SCALE = 1.0f;      // puntos que cuentan como lado claro
// (antes: solo el empuje lateral, integral de la aceleración X durante la bajada, en g·s)
// Datos reales (golpeando seguido): desde la derecha -0.33..-0.94; desde la izquierda -0.24..+0.02
constexpr float LAT_MID_GS = -0.30f;    // frontera entre izquierda y derecha (37 de 38 bien en tuning-data)
constexpr float LAT_SCALE_GS = 0.15f;   // distancia a la frontera que cuenta como lado claro
constexpr float TILT_SIGN = 1.0f;       // signo global izquierda/derecha (también hay ajuste en Kolpe proba)
constexpr float TILT_FULL_G = 0.5f;     // (indicador en pantalla) g lateral de la gravedad = inclinación completa
constexpr float TILT_STRAIGHT = 0.03f;  // |tilt| menor = golpe recto (poco eficaz); casi nunca

// --- Aizkolaritza ---
constexpr float LOG_RADIUS = 0.24f;     // m
constexpr float LOG_LENGTH = 1.5f;      // m
constexpr float CUT_PER_CHOP = 0.105f;  // fracción del radio por golpe perfecto
constexpr float REPEAT_SIDE_FACTOR = 0.35f;  // golpe por el mismo lado de la V que el anterior
constexpr float STRAIGHT_FACTOR = 0.5f;      // golpe sin inclinar
constexpr bool FATIGUE_ON = false;          // de momento sin cansancio (se recuperaba demasiado rápido)
constexpr float FATIGUE_PER_CHOP = 0.1f;
constexpr float FATIGUE_RECOVER = 0.14f;     // por segundo
constexpr float FATIGUE_HEAVY = 0.7f;        // a partir de aquí los golpes pierden fuerza
constexpr float TURN_ALLOWED = 0.6f;         // profundidad (× radio) para poder darse la vuelta
constexpr float TURN_PROMPT = 0.9f;          // profundidad para mostrar "Buelta eman!"
constexpr float SIDE_MAX = 1.3f;             // profundidad máxima por lado (× radio)

// --- Cuerpo de Mii (Wii Sports Resort, game/mii/body) ---
// Unidades del modelo de Resort: hombros a 9.25 sobre los pies, cuello a 9.7
constexpr float MII_BODY_UNIT = 0.0886f;     // m por unidad: hombros a 0.82 m (como el cuerpo anterior)
constexpr float MII_HEAD_UNITS = 3.9f;       // ancho de la cara en unidades del cuerpo (bolos de Resort: como el torso)
constexpr float MII_HEAD_DY = 0.0f;          // barbilla respecto a head_set (unidades)
constexpr float MII_HEAD_DZ = 0.0f;
// Proporciones según el Mii (0..127): escala vertical por la altura y horizontal por la complexión
constexpr float MII_BODY_SY0 = 0.8f, MII_BODY_SY_RANGE = 0.4f;
constexpr float MII_BODY_SXZ0 = 0.8f, MII_BODY_SXZ_RANGE = 0.4f;
// Manos sueltas (bola de Resort, 1.6 unidades de diámetro, centro en x = 0.6 de su modelo)
constexpr float MII_HAND_SCALE = 1.3f;       // en los bolos de Resort la mano mide ~0.55 del ancho de la cara
constexpr float MII_HAND_CX = 0.6f;
constexpr float MII_SASH_Y = 5.65f;          // altura del gerriko: donde acaba la camiseta (unidades)
constexpr float MII_SASH_UNITS = 1.78f;      // su radio: lo más ancho del torso es 1.69 (unidades)
constexpr float MII_SASH_DEPTH = 1.0f;       // y su fondo respecto al ancho

}  // namespace tuning
