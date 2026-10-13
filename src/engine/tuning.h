#pragma once
// Constantes de ajuste fino. Casi todo esto hay que afinarlo probando con un Wiimote real.
namespace tuning {

// --- Detector de hachazo (valores en g). Hay que afinarlos con datos de un mando real ---
// Subir: la punta del mando hacia arriba (componente Y de la gravedad)
constexpr float RAISE_SIGN = 1.0f;      // poner a -1 si "subir" se detecta al revés
constexpr float RAISE_ON = 0.55f;       // pitchUp para considerar el hacha arriba (~35°)
constexpr float RAISE_OFF = 0.25f;      // por debajo, deja de estar armado
constexpr float RAISE_HOLD = 0.10f;     // s que hay que mantenerlo arriba para armar
// Bajar: pico de aceleración
constexpr float SWING_START_G = 2.0f;   // |a| que inicia la bajada
constexpr float SWING_END_G = 1.4f;     // |a| por debajo del cual la bajada ha terminado
constexpr float SWING_MAX_TIME = 0.45f; // s
constexpr float CHOP_MIN_G[3] = {2.3f, 2.8f, 3.3f};  // pico mínimo según sensibilidad (suave/normal/fuerte)
constexpr float CHOP_FULL_G = 5.2f;     // pico que da fuerza máxima
constexpr float CHOP_COOLDOWN = 0.2f;   // s
// Lado de la V: inclinación del mando arriba (roll) + empuje lateral durante la bajada
constexpr float TILT_FULL_G = 0.5f;     // g lateral de la gravedad = inclinación completa
constexpr float TILT_SIGN = 1.0f;       // signo global izquierda/derecha (también hay ajuste en Kolpe proba)
constexpr float LAT_FULL_GS = 0.12f;    // empuje lateral (g·s) que cuenta como diagonal completa
constexpr float LAT_SIGN = 1.0f;        // signo del empuje lateral respecto a la inclinación
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
