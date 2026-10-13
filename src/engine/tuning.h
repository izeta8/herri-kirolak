#pragma once
// Constantes de ajuste fino. Casi todo esto hay que afinarlo probando con un Wiimote real.
namespace tuning {

// --- Detector de hachazo (valores en g) ---
constexpr float SWING_START_G = 1.9f;   // |a| que inicia un golpe
constexpr float SWING_END_G = 1.35f;    // |a| por debajo del cual el golpe ha terminado
constexpr float SWING_MAX_TIME = 0.45f; // s
constexpr float CHOP_MIN_G = 2.3f;      // pico mínimo para contar como hachazo
constexpr float CHOP_FULL_G = 4.6f;     // pico que da fuerza máxima
constexpr float CHOP_COOLDOWN = 0.22f;  // s entre golpes
// Inclinación de muñeca: g lateral (eje X del mando) que cuenta como inclinación completa
constexpr float TILT_FULL_G = 0.5f;
constexpr float TILT_SIGN = 1.0f;       // poner a -1 si izquierda/derecha salen al revés
constexpr float TILT_STRAIGHT = 0.25f;  // |tilt| menor = golpe recto (poco eficaz)

// --- Aizkolaritza ---
constexpr float LOG_RADIUS = 0.24f;     // m
constexpr float LOG_LENGTH = 1.5f;      // m
constexpr float CUT_PER_CHOP = 0.105f;  // fracción del radio por golpe perfecto
constexpr float REPEAT_SIDE_FACTOR = 0.35f;  // golpe por el mismo lado de la V que el anterior
constexpr float STRAIGHT_FACTOR = 0.5f;      // golpe sin inclinar
constexpr float FATIGUE_PER_CHOP = 0.11f;
constexpr float FATIGUE_RECOVER = 0.28f;     // por segundo
constexpr float FATIGUE_HEAVY = 0.7f;        // a partir de aquí los golpes pierden fuerza
constexpr float TURN_ALLOWED = 0.6f;         // profundidad (× radio) para poder darse la vuelta
constexpr float TURN_PROMPT = 0.9f;          // profundidad para mostrar "Buelta eman!"
constexpr float SIDE_MAX = 1.3f;             // profundidad máxima por lado (× radio)

}  // namespace tuning
