#pragma once
// Todos los textos del juego (euskara). Si algún día hay más idiomas, se traducen aquí.
namespace txt {

constexpr const char* GAME_TITLE = "HERRI KIROLAK";
constexpr const char* PRESS = "Sakatu";
constexpr const char* PRESS_A_TAIL = "botoia";

constexpr const char* PLAY = "Jokatu";
constexpr const char* PLAYERS = "Jokalariak";
constexpr const char* RECORDS = "Errekorrak";
constexpr const char* EXIT = "Irten";

constexpr const char* SELECT = "Aukeratu";
constexpr const char* BACK = "Atzera";
constexpr const char* OK = "Ados";
constexpr const char* YES = "Bai";
constexpr const char* NO = "Ez";
constexpr const char* SOON = "Laster!";

constexpr const char* CHOOSE_SPORT = "Aukeratu kirola";
constexpr const char* AIZKOLARITZA = "Aizkolaritza";
constexpr const char* AIZKO_DESC = "Moztu enborra aizkoraz, V forman!";
constexpr const char* HARRI = "Harri-jasotzea";
constexpr const char* SOKATIRA = "Sokatira";
constexpr const char* TRONTZA = "Trontza";
constexpr const char* TXINGAK = "Txingak";

constexpr const char* CHOOSE_MODE = "Aukeratu modua";
constexpr const char* TRAINING = "Entrenamendua";
constexpr const char* TRAINING_DESC = "Enbor bat, lasai eta aholkuekin.";
constexpr const char* COMPETITION = "Lehiaketa";
constexpr const char* COMPETITION_DESC = "1-4 jokalari aldi berean. Azkarrena irabazle!";
constexpr const char* SWING_TEST = "Kolpe proba";
constexpr const char* SWING_TEST_DESC = "Ikusi nola neurtzen duen Wii Remote-ak zure kolpea.";
constexpr const char* RAISE_REMOTE = "Altxatu Wii Remote-a";
constexpr const char* ARMED = "Prest! Jo behera!";
constexpr const char* HIT = "Kolpea!";
constexpr const char* RAISE_LABEL = "Altuera";
constexpr const char* HIT_NOW = "JO!";
constexpr const char* TILT_LABEL = "Okertzea";
constexpr const char* HITS = "Kolpeak";
constexpr const char* STRENGTH = "Indarra";
constexpr const char* STRAIGHT = "Zuzen";
constexpr const char* SENSITIVITY = "Sentikortasuna";
constexpr const char* SENS_SOFT = "Leuna";
constexpr const char* SENS_NORMAL = "Normala";
constexpr const char* SENS_HARD = "Gogorra";
constexpr const char* INVERT_LR = "Ezker/eskuin alderantziz";
constexpr const char* INVERT_UD = "Gora/behera alderantziz (gurutzea)";
constexpr const char* RECORD = "Grabatu kolpeak";
constexpr const char* RECORDING = "Grabatzen...";
constexpr const char* LABEL = "Etiketa";
constexpr const char* REC_SAVED = "golpeak.csv gordeta: %d kolpe, %d lagin";
constexpr const char* RAISE_AXE = "Altxatu aizkora!";
constexpr const char* LIVE = "Zuzenean";
constexpr const char* GESTURE = "KEINUA";
constexpr const char* ANGLE = "Angelua";
constexpr const char* GESTURES = "Keinuak";
constexpr const char* LAST_STROKE = "Azken kolpea";
constexpr const char* PEAK = "Pikoa";
constexpr const char* LATERAL = "Alboko bultza";
constexpr const char* START = "Hasieran (x y z):";
constexpr const char* END = "Amaieran (x y z):";
constexpr const char* DO_A_STROKE = "Egin kolpe bat...";
constexpr const char* LR_SHORT = "ezker/eskuin";
constexpr const char* UD_SHORT = "gora/behera";
constexpr const char* HOW_MANY_PLAYERS = "Zenbat jokalari?";
constexpr const char* HOW_MANY_LOGS = "Zenbat enbor?";
constexpr const char* LOG_1 = "Enbor 1";
constexpr const char* LOGS_N = "%d enbor";
constexpr const char* CHOOSE_CHAR = "Aukeratu aizkolaria";
constexpr const char* PLAYER_N = "%d. jokalaria";
constexpr const char* CPU = "CPU";

// Partida
constexpr const char* TURN_OF = "%s%s txanda";  // nombre + "(r)en"
constexpr const char* READY = "Prest?";
constexpr const char* GO = "Hasi!";
constexpr const char* TIME = "Denbora";
constexpr const char* LOG_N_OF = "Enborra %d/%d";
constexpr const char* FATIGUE = "Nekea";
constexpr const char* GREAT = "Bikain!";
constexpr const char* GOOD = "Ondo!";
constexpr const char* WEAK = "Ahul...";
constexpr const char* SAME_SIDE = "Aldatu aldea!";
constexpr const char* TOO_STRAIGHT = "Okertu aizkora!";
constexpr const char* TIRED = "Nekatuta!";
constexpr const char* TURN_AROUND = "Buelta eman!";
constexpr const char* PRESS_B = "Sakatu B";
constexpr const char* LOG_CUT = "Enborra moztuta!";
constexpr const char* FINISHED = "Amaitu!";
constexpr const char* NEXT_SIDE = "Hurrengoa:";
constexpr const char* LEFT = "Ezkerra";
constexpr const char* RIGHT = "Eskuina";
constexpr const char* CONNECT_REMOTE = "Konektatu %d. Wii Remote-a";

constexpr const char* TIP_1 = "Heldu behetik, erpurua 1 eta 2 botoien gainean.";
constexpr const char* TIP_2 = "Jaitsi diagonalean erdira, eskumuturra biratu gabe. V forma!";
constexpr const char* TIP_3 = "Ez jo azkarregi: nekatuz gero, kolpeak ahulagoak dira.";
constexpr const char* TIP_4 = "Erdira iristean, eman buelta B botoiarekin.";
// Tutorial animado "Nola jokatu" (estilo Wii Sports Resort): título y dos líneas por paso
constexpr const char* HOWTO = "Nola jokatu";
constexpr const char* HOWTO_NEXT = "Hurrengoa";
constexpr const char* HOWTO_STEPS[5][3] = {
    {"Heldu horrela", "Wii Remote-a zutik,", "erpurua 1 eta 2 botoien gainean."},
    {"Altxatu alde batera", "Igo aizkora ezkerrera edo", "eskuinera, JO! agertu arte."},
    {"Jo diagonalean", "Jaitsi bizkor erdira,", "eskumuturra biratu gabe."},
    {"Egin V forma", "Txandakatu: ezkerretik,", "eskuinetik, ezkerretik..."},
    {"Buelta eman", "Aurrekoa erdiraino iristean,", "sakatu B atzekoa jotzeko."},
};
// Tarjeta "Nola jo" (cómo golpear)
constexpr const char* HOW_TO = "Nola jo";
constexpr const char* HOW_1 = "Heldu behetik";
constexpr const char* HOW_1B = "erpurua 1-2 gainean";
constexpr const char* HOW_2 = "Altxatu alde batetik";
constexpr const char* HOW_2B = "JO! agertu arte";
constexpr const char* HOW_3 = "Jaitsi erdira";
constexpr const char* HOW_3B = "eskumuturra biratu gabe";
constexpr const char* NEXT = "Orain:";

constexpr const char* RESULTS = "Emaitzak";
constexpr const char* WINNER = "Irabazlea";
constexpr const char* NEW_RECORD = "Errekor berria!";
constexpr const char* PLAY_AGAIN = "Berriro jokatu";
constexpr const char* TO_MENU = "Menura itzuli";
constexpr const char* YOUR_TIME = "Zure denbora";

constexpr const char* PAUSE = "Pausa";
constexpr const char* CONTINUE = "Jarraitu";
constexpr const char* RESTART = "Berriz hasi";

constexpr const char* EXIT_QUESTION = "Homebrew Channel-era itzuli?";

// Jokalariak
constexpr const char* EDIT = "Aldatu";
constexpr const char* NEW_PLAYER = "Jokalari berria";
constexpr const char* NAME = "Izena";
constexpr const char* COLOR = "Kolorea";
constexpr const char* SKIN = "Azala";
constexpr const char* HAIR = "Ilea";
constexpr const char* HAIR_STYLE = "Orrazkera";
constexpr const char* MII = "Mii";
constexpr const char* HAIR_STYLES[] = {"Motza", "Zutik", "Luzea", "Ertaina"};  // peinados (NUM_HAIR_STYLES)
constexpr const char* TXAPELA = "Txapela";
constexpr const char* SAVE = "Gorde";
constexpr const char* DELETE = "Ezabatu";
constexpr const char* NO_SAVE = "Ezin da gorde: ez dago USB edo SD txartelik";
constexpr const char* SAVED = "Gordeta!";

constexpr const char* NO_RECORD = "Errekorrik ez";
constexpr const char* REC_TRAINING = "Entrenamendua";
constexpr const char* REC_COMP = "Lehiaketa - %d enbor";
constexpr const char* REC_PALA_STREAK = "Entrenamendua: jarraian";
constexpr const char* REC_PALA_RALLY = "Lehiaketa: peloteo luzeena";
constexpr const char* REC_HITS = "%d kolpe";

// Pala
constexpr const char* PALA = "Pala";
constexpr const char* PALA_DESC = "Jo pilota frontoira egurrezko palarekin!";
constexpr const char* HOW_MANY_POINTS = "Zenbat tantotara?";
constexpr const char* CHOOSE_PELOTARI = "Aukeratu pelotaria";
constexpr const char* POINTS_N = "%d tantotara";
constexpr const char* PALA_TRAINING_SUB = "Peloteoa frontoiaren kontra";
constexpr const char* CHAPA = "Txapa!";
constexpr const char* TOO_HIGH = "Gainetik!";
constexpr const char* OUT = "Kanpora!";
constexpr const char* LONG = "Luze!";
constexpr const char* TWO_BOUNCES = "Bi bote!";
constexpr const char* FALTA = "Falta!";
constexpr const char* PASA = "Pasa!";
constexpr const char* SHORT = "Motz!";
constexpr const char* MISS = "Huts!";
constexpr const char* POINT_FOR = "Tantoa: %s";
constexpr const char* SERVE = "Sakea";
constexpr const char* SERVE_HOW = "Sakatu A eta jo";
constexpr const char* CONNECT_NUNCHUK = "Konektatu Nunchuk-a";
constexpr const char* NUNCHUK_NEEDED = "Palan Nunchuk-arekin mugitzen zara";
constexpr const char* STREAK = "Jarraian";
constexpr const char* BEST = "Onena";
constexpr const char* EARLY = "Goiz";
constexpr const char* LATE = "Berandu";
constexpr const char* WHIP_TEST = "Pala: kolpe proba";
constexpr const char* CPU_LEVEL = "CPU-aren maila";
constexpr const char* HIT_BAD = "Txarto";
constexpr const char* VOLLEY = "Bolea!";
constexpr const char* BOTE_PRONTO = "Bote pronto!";
constexpr const char* BIPARETA = "Bipareta!";
constexpr const char* SHOT_NAMES[3] = {"Dejada", "Erdira", "Luzea"};
constexpr const char* PLACE = "Tokia";
constexpr const char* AIMING = "Zuzentzen";
// Tarjeta "Nola jo" de la pala: tecla y qué hace
constexpr const char* PALA_HOW[6][2] = {
    {"Nunchuk", "mugitu"}, {"Z + makila", "zuzendu"}, {"Wii Remote", "jo (indarra)"},
    {"B + jo", "dejada"}, {"jo", "erdira"}, {"A + jo", "luzea"},
};
constexpr const char* LEVELS[3] = {"Erraza", "Normala", "Zaila"};
constexpr const char* WHIP_GUIDE[3] = {"LEUN eskuinetik", "GOGOR eskuinetik", "EZKERRETIK"};
constexpr const char* WHIP_GUIDE_SHORT[3] = {"Leun", "Gogor", "Ezker"};

}  // namespace txt
