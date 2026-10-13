#pragma once
// Constantes de ajuste fino. Casi todo esto hay que afinarlo probando con un Wiimote real.
namespace tuning {

// --- Detector de hachazo. Valores sacados de gestos reales grabados por telemetría (13/10/2026) ---
// Subir: el mando gira sobre su eje X desde "apuntando a la tele" (Z = +1) hasta quedar por encima
// del hombro con los botones hacia abajo (Z ≈ -0.9). Se mide como ángulo: 0° delante, 180° atrás.
constexpr float RAISE_SIGN = -1.0f;     // en el mando real, punta arriba = gravedad en -Y
                                        // (en Dolphin es al revés: para "subir" usar la tecla K, no I)
constexpr float RAISE_ON_DEG = 120.0f;  // ángulo para considerar el hacha arriba
constexpr float RAISE_OFF_DEG = 70.0f;  // por debajo, deja de estar armado (lo ha bajado despacio)
constexpr float RAISE_HOLD = 0.08f;     // s que hay que mantenerlo arriba para armar
// Bajar: los hachazos reales dan picos de 7-8.6 g (el mando satura en ~5 g por eje);
// subir el mando no pasa de ~2.4 g
constexpr float SWING_START_G = 2.0f;   // |a| que inicia la bajada
constexpr float SWING_END_G = 1.4f;     // |a| por debajo del cual la bajada ha terminado
constexpr float SWING_MAX_TIME = 0.45f; // s
constexpr float CHOP_MIN_G[3] = {2.6f, 3.6f, 4.6f};  // pico mínimo según sensibilidad (suave/normal/fuerte)
constexpr float CHOP_FULL_G = 7.5f;     // pico que da fuerza máxima
constexpr float CHOP_COOLDOWN = 0.2f;   // s
// Lado de la V. Golpe desde arriba a la derecha: arriba el mando está inclinado a la derecha
// (gravedad X +0.3..+0.6) y al bajar empuja fuerte en X negativa (-2..-5 g)
constexpr float TILT_FULL_G = 0.5f;     // g lateral de la gravedad = inclinación completa
constexpr float TILT_SIGN = 1.0f;       // signo global izquierda/derecha (también hay ajuste en Kolpe proba)
constexpr float LAT_FULL_GS = 0.3f;     // empuje lateral (g·s) que cuenta como diagonal completa
constexpr float LAT_SIGN = -1.0f;       // el empuje va al contrario que la inclinación de arriba
constexpr float ROLL_WEIGHT = 0.6f;     // peso de la inclinación frente al empuje lateral
constexpr float TILT_STRAIGHT = 0.25f;  // |tilt| menor = golpe recto (poco eficaz)

// --- Aizkolaritza ---
constexpr float LOG_RADIUS = 0.24f;     // m
constexpr float LOG_LENGTH = 1.5f;      // m
constexpr float CUT_PER_CHOP = 0.105f;  // fracción del radio por golpe perfecto
constexpr float REPEAT_SIDE_FACTOR = 0.35f;  // golpe por el mismo lado de la V que el anterior
constexpr float STRAIGHT_FACTOR = 0.5f;      // golpe sin inclinar
constexpr float FATIGUE_PER_CHOP = 0.1f;
constexpr float FATIGUE_RECOVER = 0.14f;     // por segundo
constexpr float FATIGUE_HEAVY = 0.7f;        // a partir de aquí los golpes pierden fuerza
constexpr float TURN_ALLOWED = 0.6f;         // profundidad (× radio) para poder darse la vuelta
constexpr float TURN_PROMPT = 0.9f;          // profundidad para mostrar "Buelta eman!"
constexpr float SIDE_MAX = 1.3f;             // profundidad máxima por lado (× radio)

}  // namespace tuning
