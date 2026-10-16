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

// --- Palas: latigazo y nunchuk (sin datos reales todavía: calibrar con telemetría) ---
constexpr float WHIP_START_G = 2.2f;    // |a| que inicia el latigazo
constexpr float WHIP_MIN_G = 2.6f;      // pico mínimo para que sea golpe
constexpr float WHIP_FULL_G = 7.5f;     // pico que da fuerza máxima
constexpr float WHIP_COOLDOWN = 0.25f;  // s entre latigazos
constexpr float STICK_DEADZONE = 0.15f;

// --- Palas: frontón (m). El frontis está en z = 0 y la cancha va hacia +z; pared izquierda en x = -COURT_W/2 ---
constexpr float COURT_LEN = 30.0f;      // largo de la cancha (frontón corto)
constexpr float COURT_W = 10.0f;        // ancho (de la pared izquierda a la línea del lado abierto)
constexpr float CUADRO = 3.5f;          // separación de las rayas de la pared izquierda
constexpr float WALL_H = 10.0f;         // altura del frontis y de la pared izquierda
constexpr float CHAPA_H = 0.9f;         // chapa: la pelota tiene que dar por encima
constexpr float TOP_LINE = 9.5f;        // línea de arriba del frontis
constexpr float FALTA_Z = 14.0f;        // saque: tiene que botar entre falta (cuadro 4)…
constexpr float PASA_Z = 24.5f;         // …y pasa (cuadro 7)
constexpr float SERVE_Z = 17.5f;        // desde donde se saca (cuadro 5)
constexpr float RECEIVE_Z = 23.0f;      // donde espera el que resta

// --- Palas: pelota ---
constexpr float BALL_R = 0.035f;        // radio real
constexpr float BALL_DRAW_R = 0.11f;    // radio con el que se dibuja (si no, no se ve desde la cámara)
constexpr float BALL_G = 9.8f;
constexpr float FLOOR_E = 0.62f;        // rebote vertical en el suelo
constexpr float FLOOR_F = 0.86f;        // y lo que conserva en horizontal
constexpr float FRONTIS_E = 0.62f;      // rebote en el frontis (velocidad normal)
constexpr float FRONTIS_F = 0.9f;       // y lo que conserva en tangencial
constexpr float WALL_E = 0.7f;          // pared izquierda
constexpr float DROP_VY = -4.0f;        // saque: velocidad con la que el pelotari bota la pelota (m/s)
constexpr float DROP_Y = 1.0f;          // altura desde la que la suelta

// --- Palas: golpe (diseño v2 de habilidad, SPEC §6) ---
// Tipo de golpe según el botón pulsado al hacer el latigazo: 0 = B dejada, 1 = nada (media), 2 = A largo.
// Velocidad horizontal hacia la mira (m/s) con la fuerza mínima y máxima, y dónde bota al volver (z)
constexpr float SHOT_SPEED[3][2] = {{8.5f, 12.0f}, {13.0f, 19.0f}, {15.0f, 23.0f}};
constexpr float SHOT_DEPTH[3][2] = {{4.5f, 7.0f}, {13.0f, 17.5f}, {24.0f, 28.5f}};
constexpr float SERVE_DEPTH[3] = {15.5f, 19.0f, 23.0f};  // saque: siempre entre falta y pasa (más o menos al límite)
constexpr float SHOT_MIN_SPEED_Q = 0.72f;  // con calidad 0 la pelota sale a este × la velocidad
// Mira (Z + stick): recorrido continuo. u en [-1, 1] = frontis (de la esquina con la pared al lado abierto);
// u en [-2, -1) = pared izquierda, hasta AIM_WALL_LEN m desde el frontis (biparetas)
constexpr float AIM_WALL_LEN = 7.0f;
constexpr float AIM_RATE = 14.0f;          // suavizado de la mira
constexpr float AIM_ERR = 0.9f;            // error de la mira (en u) con calidad 0
constexpr float AIM_ERR_TIMING = 0.5f;     // y cuánto lo desvía pegar pronto (hacia el ancho) o tarde (hacia la pared)
constexpr float HEIGHT_ERR = 3.5f;         // error vertical (m/s) con calidad 0: puede acabar en la chapa o alta
constexpr float PLANT_SPEED = 0.15f;       // apuntando con Z, el pelotari casi no se mueve (× velocidad)
// Punto dulce: tiempo (pico del latigazo frente al momento en que la pelota pasa a su lado) y sitio (distancia)
constexpr float HIT_WINDOW = 0.16f;        // s: fuera de esto, ni la toca
constexpr float SWEET_TIME = 0.035f;       // s: dentro de esto, tiempo perfecto
constexpr float SWEET_DIST = 0.78f;        // m: distancia ideal del cuerpo a la pelota (brazo + pala)
constexpr float SWEET_DIST_TOL = 0.2f;     // m: dentro de esto, sitio perfecto
constexpr float SWEET_DIST_FALL = 0.5f;    // m más allá de la tolerancia hasta calidad mínima
constexpr float SWEET_DIST_MIN_Q = 0.15f;  // calidad de sitio al límite del alcance
constexpr float REACH = 1.4f;              // m: más lejos, no llega
constexpr float REACH_Y_MIN = 0.1f, REACH_Y_MAX = 2.4f;
constexpr float SWEET_Y_LO = 0.35f, SWEET_Y_HI = 1.55f;  // alturas cómodas; fuera, peor calidad
constexpr float WINDOW_STRONG = 0.7f;      // con fuerza máxima las ventanas se estrechan a esto…
constexpr float WINDOW_SOFT = 1.25f;       // …y con la mínima se ensanchan a esto
constexpr float WINDOW_VOLLEY = 0.75f;     // volea y bote pronto: ventana más corta…
constexpr float SPEED_VOLLEY = 1.12f;      // …pero la pelota sale más rápida
constexpr float BOTE_PRONTO_T = 0.09f;     // s tras el bote que cuentan como bote pronto
constexpr float Q_GREAT = 0.85f, Q_GOOD = 0.5f;  // Bikain! / Ondo / Txarto
// Peloteo que se acelera
constexpr float RALLY_SPEEDUP = 0.03f;     // por golpe bueno
constexpr float RALLY_SPEEDUP_MAX = 0.3f;
constexpr float PLAYER_SPEED = 6.5f;       // m/s corriendo con el stick
constexpr float PLAYER_ACCEL = 14.0f;      // suavizado de la carrera
constexpr float PLAYER_GAP = 0.8f;         // m: los dos pelotaris no se acercan más que esto
constexpr bool SHOW_LANDING = true;        // marca dónde va a botar la pelota (ayuda)
// CPU por niveles (0 erraza, 1 normala, 2 zaila)
struct BotLevel {
    float timingNoise;  // s de error en el momento del golpe
    float distNoise;    // m de error al colocarse
    float speed;        // × PLAYER_SPEED
    float mistake;      // probabilidad de un golpe muy a destiempo
    float risk;         // 0..1: cuánto busca la pared, el ancho y las dejadas
};
constexpr BotLevel BOT_LEVELS[3] = {
    {0.07f, 0.35f, 0.75f, 0.14f, 0.15f},
    {0.045f, 0.2f, 0.88f, 0.07f, 0.45f},
    {0.025f, 0.1f, 1.0f, 0.03f, 0.8f},
};
constexpr float BOT_MISTAKE_NOISE = 0.13f;
// Cámara isométrica (desde atrás a la derecha, en alto) con zoom según la distancia de la pelota al frontis
constexpr float CAM_DIR_X = 0.3f, CAM_DIR_Y = 0.38f, CAM_DIR_Z = 0.8f;  // del objetivo a la cámara (se normaliza)
constexpr float CAM_FOV = 30.0f;
constexpr float CAM_FOV_MAX = 44.0f;      // para que quepa todo se abre el ángulo antes de alejarse (más perspectiva)
constexpr float CAM_LOOK_UP = 1.6f;       // m por encima del objetivo a los que mira (para ver el frontis)
constexpr float CAM_DIST_NEAR = 7.0f;      // pelota junto al frontis
constexpr float CAM_DIST_FAR = 12.0f;      // pelota al fondo
constexpr float CAM_DIST_MAX = 19.0f;      // tope al abrirse para no perder al pelotari
constexpr float CAM_BALL_WEIGHT = 0.6f;    // el objetivo es la pelota con este peso; el resto, el pelotari que golpea
constexpr float CAM_RATE = 2.6f;           // suavizado
constexpr float CAM_MARGIN = 0.85f;        // fracción de la pantalla en la que tiene que caber el pelotari

// --- Cuerpo de Mii (Wii Sports Resort, game/mii/body) ---
// Unidades del modelo de Resort: hombros a 9.25 sobre los pies, cuello a 9.7
constexpr float MII_BODY_UNIT = 0.0886f;     // m por unidad: hombros a 0.82 m (como el cuerpo anterior)
constexpr float MII_HEAD_UNITS = 3.9f;       // ancho de la cara en unidades del cuerpo (bolos de Resort: como el torso)
constexpr float MII_HEAD_DY = 0.0f;          // barbilla respecto a head_set (unidades)
constexpr float MII_HEAD_DZ = 0.0f;
// Sin proporciones por altura ni complexión: Resort no las lee nunca (main.dol: ninguna instrucción extrae esos campos de
// RFLAdditionalInfo y su GetBodyModelIndex solo mira el tipo de cuerpo y el sexo). Todos los Miis, el mismo cuerpo.
// Manos sueltas (bola de Resort, 1.6 unidades de diámetro, centro en x = 0.6 de su modelo)
constexpr float MII_HAND_SCALE = 1.3f;       // en los bolos de Resort la mano mide ~0.55 del ancho de la cara
constexpr float MII_HAND_CX = 0.6f;
constexpr float MII_SASH_Y = 5.65f;          // altura del gerriko: donde acaba la camiseta (unidades)
constexpr float MII_SASH_UNITS = 1.78f;      // su radio: lo más ancho del torso es 1.69 (unidades)
constexpr float MII_SASH_DEPTH = 1.0f;       // y su fondo respecto al ancho
// Ropa: Resort usa la textura "_aw"/"_ay"/"_awy" (dibujos oscuros) con el color favorito blanco (10), amarillo (2) o
// verde lima (3). Con camisetas que no son el color favorito del Mii (CPU de colores), a partir de esta luminancia
constexpr float MII_SHIRT_LIGHT_LUMA = 0.7f;
// Público (C1): su textura reparte color favorito y "segundo color" (la base); en los bolos de Resort la base es casi
// negra (medida en una captura: 0..20 con la luz)
constexpr int MII_CROWD_BASE_R = 16, MII_CROWD_BASE_G = 16, MII_CROWD_BASE_B = 16;

// --- Luz de los Miis como en Wii Sports Resort (game/mii/light) ---
// La de la bolera: Common/BwlScene (WS2_bwl_Light.plight, WS2_bwl_Lmap.plmap) y el lightset "Abatar_Lighset" de
// Stage/BwlScene/Normal (bwg_field_rsca.brres). Dos lightmaps indexadas por la normal en cámara, que EGG pinta cada
// fotograma: lm0 (difusa) y lm1 (brillos y borde). Color = albedo × (lm0·oclusión + ambiente) + lm1·oclusión
constexpr int MII_LM_SIZE = 32;              // lado de las lightmaps (EGG::LightTexture, 32 por defecto)
constexpr int MII_AMBIENT = 100;             // ambiente del lightset de los Miis (0..255)
struct MiiLight {
    int map;           // 0 = lm0, 1 = lm1
    int type;          // 0 dirección de mundo, 1 brillo (medio ángulo con la cámara), 2 fija en la cámara, 4 contraluz
    float x, y, z;     // posición de la luz; alumbra hacia el origen
    int gradient;      // degradado de EGG (0..9) en función de n·L
    float intensity;
    int r, g, b;
};
constexpr MiiLight MII_LIGHTS[] = {
    {0, 0, 0, 100000, 0, 1, 1.0f, 255, 255, 255},            // cenital, en media lambert (de arriba blanco a abajo negro)
    {1, 4, 0, 0, 0, 7, 0.4f, 255, 255, 200},                 // contraluz cálido: el borde de la silueta
    {1, 2, 0, 5000, 8000, 4, 0.05f, 255, 255, 255},          // desde la cámara, algo por arriba
    {1, 1, -50000, 100000, -30000, 6, 0.3f, 255, 255, 255},  // brillo de arriba
    {1, 1, 15000, -15000, 20000, 5, 0.1f, 255, 255, 255},    // brillo de abajo
};

}  // namespace tuning
