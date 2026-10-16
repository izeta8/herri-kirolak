// Jokalariak: cada jugador es uno de los Miis de la consola (y si lleva txapela). Sin Miis en la consola, editor
// propio de reserva (nombre, color, piel, pelo, peinado, txapela).
#include <stdio.h>
#include <string.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/ui.h"
#include "game/mii/mii.h"
#include "game/palette.h"
#include "game/text.h"
#include "scenes/common.h"
#include "scenes/scenes.h"

namespace {

enum Mode { MODE_LIST, MODE_EDIT, MODE_KEYBOARD, MODE_CONFIRM };
enum Confirm { CONF_DELETE, CONF_UNSAVED };
enum Field { F_MII, F_NAME, F_COLOR, F_SKIN, F_HAIR, F_STYLE, F_TXAPELA, F_SAVE, F_DELETE, F_COUNT };

// Teclado en pantalla: 4 filas de 10 teclas; la última fila son teclas especiales
static const char* KEYS[3][10] = {
    {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"},
    {"K", "L", "M", "N", "Ñ", "O", "P", "Q", "R", "S"},
    {"T", "U", "V", "W", "X", "Y", "Z", "-", ".", " "},
};
enum SpecialKey { K_SHIFT, K_DEL, K_OK, K_SPECIAL_COUNT };

// Rectángulo de una tecla (fila 3 = especiales), el mismo para dibujar y para el puntero
const float KEY_W = 62, KEY_H = 54, KEY_GAP = 8, KEY_Y0 = 190;
void keyRect(int r, int c, float& x, float& y, float& w, float& h) {
    float totalW = 10 * KEY_W + 9 * KEY_GAP, x0 = gfx::screenW() / 2 - totalW / 2;
    y = KEY_Y0 + r * (KEY_H + KEY_GAP);
    h = KEY_H;
    if (r < 3) {
        x = x0 + c * (KEY_W + KEY_GAP);
        w = KEY_W;
    } else {
        w = (totalW - 2 * KEY_GAP) / 3;
        x = x0 + c * (w + KEY_GAP);
    }
}

class ProfilesScene : public Scene {
public:
    ProfilesScene() {
        list_.count = MAX_PROFILES;
        rebuildList();
    }

    void update(float dt) override {
        t_ += dt;
        u32 down = input::allDown();
        switch (mode_) {
            case MODE_LIST: updateList(dt, down); break;
            case MODE_EDIT: updateEdit(dt, down); break;
            case MODE_KEYBOARD: updateKeyboard(down); break;
            case MODE_CONFIRM: updateConfirm(dt, down); break;
        }
        msgT_ = fmaxf(0.0f, msgT_ - dt);
    }

    void draw() override;

private:
    void rebuildList() {
        // Visibles: los usados + un hueco libre para crear
        count_ = 0;
        bool freeShown = false;
        for (int i = 0; i < MAX_PROFILES; i++) {
            if (save::data().profiles[i].used) slots_[count_++] = i;
            else if (!freeShown) {
                slots_[count_++] = i;
                freeShown = true;
            }
        }
        list_.count = count_;
        if (list_.sel >= count_) list_.sel = count_ - 1;
    }

    int usedCount() const {
        int n = 0;
        for (int i = 0; i < MAX_PROFILES; i++) n += save::data().profiles[i].used;
        return n;
    }

    void updateList(float dt, u32 down) {
        int r = list_.update(dt, down);
        if (r == -2) scenes::go(makeMainMenu(1));
        else if (r >= 0) {
            editing_ = slots_[r];
            work_ = save::data().profiles[editing_];
            if (!work_.used) {
                memset(&work_, 0, sizeof(work_));
                strcpy(work_.name, "Berria");
                work_.color = (u8)(editing_ * 5 % NUM_COLORS);
                work_.skin = 0;
                work_.hair = 1;
                if (miiMode()) setProfileMii(work_, firstFreeMii());
            }
            mode_ = MODE_EDIT;
            edit_ = ui::ListMenu();
            nFields_ = 0;
            if (miiMode()) {
                for (int f : {F_MII, F_TXAPELA, F_SAVE, F_DELETE}) fields_[nFields_++] = f;
            } else {
                for (int f : {F_NAME, F_COLOR, F_SKIN, F_HAIR, F_STYLE, F_TXAPELA, F_SAVE, F_DELETE}) fields_[nFields_++] = f;
            }
            edit_.count = nFields_;
            edit_.enabled[nFields_ - 1] = save::data().profiles[editing_].used && usedCount() > 1;  // F_DELETE
        }
    }

    static bool miiMode() { return mii::userCount() > 0; }

    // El primer Mii de la consola que no sea ya de otro jugador
    int firstFreeMii() const {
        int count = mii::userCount();
        for (int k = 0; k < count; k++) {
            bool used = false;
            for (int i = 0; i < MAX_PROFILES; i++)
                if (i != editing_ && save::data().profiles[i].used && save::data().profiles[i].mii == k + 1) used = true;
            if (!used) return k;
        }
        return 0;
    }

    void updateEdit(float dt, u32 down) {
        int field = fields_[edit_.sel];
        int dir = (down & WPAD_BUTTON_RIGHT) ? 1 : (down & WPAD_BUTTON_LEFT) ? -1 : 0;
        // Con el puntero: A sobre la flecha izquierda o derecha de la fila elegida
        if ((down & WPAD_BUTTON_A) && arrowY_ > 0 && edit_.hovered() == edit_.sel) {
            if (ui::pointerOver(arrowLX_ - 18, arrowY_ - 16, 36, 32)) dir = -1;
            else if (ui::pointerOver(arrowRX_ - 18, arrowY_ - 16, 36, 32)) dir = 1;
            if (dir) down &= ~WPAD_BUTTON_A;
        }
        // En esta pantalla izquierda/derecha cambian el valor, no la selección
        u32 navDown = down & ~(WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT);
        if (dir) {
            switch (field) {
                case F_COLOR: work_.color = (u8)((work_.color + dir + NUM_COLORS) % NUM_COLORS); break;
                case F_SKIN: work_.skin = (u8)((work_.skin + dir + NUM_SKINS) % NUM_SKINS); break;
                case F_HAIR: work_.hair = (u8)((work_.hair + dir + NUM_HAIRS) % NUM_HAIRS); break;
                case F_STYLE: work_.hairStyle = (u8)((work_.hairStyle + dir + NUM_HAIR_STYLES) % NUM_HAIR_STYLES); break;
                case F_TXAPELA: work_.txapela = !work_.txapela; break;
                case F_MII: {
                    int count = mii::userCount();
                    int cur = work_.mii ? work_.mii - 1 : 0;
                    setProfileMii(work_, (cur + dir + count) % count);
                    break;
                }
                default: break;
            }
            if (field == F_MII || (field >= F_COLOR && field <= F_TXAPELA)) audio::play(SFX_MOVE, 0.6f);
        }
        int r = edit_.update(dt, navDown);
        if (r >= 0) r = fields_[r];
        if (r == -2) {
            if (changed()) askConfirm(CONF_UNSAVED);
            else mode_ = MODE_LIST;
        } else if (r == F_NAME) {
            mode_ = MODE_KEYBOARD;
            keyRow_ = 0;
            keyCol_ = 0;
            shift_ = strlen(work_.name) == 0;
        } else if (r == F_TXAPELA) {
            work_.txapela = !work_.txapela;
        } else if (r == F_SAVE) {
            saveAndExit();
        } else if (r == F_DELETE) {
            askConfirm(CONF_DELETE);
        }
    }

    // ¿Hay algo distinto de lo guardado? (un jugador nuevo siempre cuenta como cambio)
    bool changed() const {
        const Profile& o = save::data().profiles[editing_];
        if (!o.used) return true;
        return strcmp(o.name, work_.name) != 0 || o.color != work_.color || o.skin != work_.skin || o.hair != work_.hair ||
               o.txapela != work_.txapela || o.hairStyle != work_.hairStyle || o.mii != work_.mii;
    }

    void saveAndExit() {
        work_.used = 1;
        if (!work_.name[0]) strcpy(work_.name, "?");
        save::data().profiles[editing_] = work_;
        bool ok = save::write();
        msg_ = ok ? txt::SAVED : txt::NO_SAVE;
        msgT_ = 2.0f;
        mode_ = MODE_LIST;
        rebuildList();
    }

    void askConfirm(Confirm c) {
        confirm_ = c;
        confirmMenu_ = ui::ListMenu();
        confirmMenu_.count = 2;
        confirmMenu_.horizontal = true;
        confirmMenu_.sel = c == CONF_DELETE ? 1 : 0;  // borrar: por defecto "Ez"; sin guardar: por defecto "Gorde"
        mode_ = MODE_CONFIRM;
    }

    void updateConfirm(float dt, u32 down) {
        int r = confirmMenu_.update(dt, down);
        if (r == -2) {
            mode_ = MODE_EDIT;  // B: seguir editando
        } else if (confirm_ == CONF_DELETE) {
            if (r == 0) {
                save::data().profiles[editing_].used = 0;
                save::write();
                msg_ = txt::DELETED;
                msgT_ = 2.0f;
                mode_ = MODE_LIST;
                rebuildList();
            } else if (r == 1) {
                mode_ = MODE_EDIT;
            }
        } else {
            if (r == 0) saveAndExit();
            else if (r == 1) mode_ = MODE_LIST;  // salir sin guardar
        }
    }

    void typeKey(const char* k) {
        char add[4];
        strcpy(add, k);
        if (!shift_) {
            if (add[0] >= 'A' && add[0] <= 'Z') add[0] = add[0] - 'A' + 'a';
            else if (!strcmp(add, "Ñ")) strcpy(add, "ñ");
        }
        if (strlen(work_.name) + strlen(add) <= NAME_LEN) {
            strcat(work_.name, add);
            audio::play(SFX_MOVE, 0.7f, 1.2f);
        } else {
            audio::play(SFX_BAD, 0.4f);
        }
        shift_ = false;
    }

    void deleteChar() {
        int n = strlen(work_.name);
        if (!n) return;
        n--;
        while (n > 0 && ((unsigned char)work_.name[n] & 0xC0) == 0x80) n--;
        work_.name[n] = 0;
        audio::play(SFX_BACK, 0.5f);
        if (n == 0) shift_ = true;
    }

    void updateKeyboard(u32 down) {
        // Puntero: la tecla que tiene debajo queda elegida
        ui::usePointer();
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < (r == 3 ? K_SPECIAL_COUNT : 10); c++) {
                float x, y, w, h;
                keyRect(r, c, x, y, w, h);
                if (ui::pointerOver(x, y, w, h) && (r != keyRow_ || c != keyCol_)) {
                    keyRow_ = r;
                    keyCol_ = c;
                    ui::hoverFeedback();
                }
            }
        int rows = 4;
        int cols = keyRow_ == 3 ? K_SPECIAL_COUNT : 10;
        if (down & WPAD_BUTTON_UP) keyRow_ = (keyRow_ + rows - 1) % rows;
        if (down & WPAD_BUTTON_DOWN) keyRow_ = (keyRow_ + 1) % rows;
        if (down & (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN)) {
            audio::play(SFX_MOVE, 0.5f);
            int newCols = keyRow_ == 3 ? K_SPECIAL_COUNT : 10;
            if (newCols != cols) keyCol_ = newCols == 3 ? clampv(keyCol_ * 3 / 10, 0, 2) : clampv(keyCol_ * 10 / 3 + 1, 0, 9);
            cols = newCols;
        }
        if (down & WPAD_BUTTON_LEFT) { keyCol_ = (keyCol_ + cols - 1) % cols; audio::play(SFX_MOVE, 0.5f); }
        if (down & WPAD_BUTTON_RIGHT) { keyCol_ = (keyCol_ + 1) % cols; audio::play(SFX_MOVE, 0.5f); }
        if (down & WPAD_BUTTON_B) deleteChar();
        if (down & WPAD_BUTTON_PLUS) { mode_ = MODE_EDIT; audio::play(SFX_OK, 0.7f); }
        if (down & WPAD_BUTTON_A) {
            if (keyRow_ < 3) typeKey(KEYS[keyRow_][keyCol_]);
            else if (keyCol_ == K_SHIFT) { shift_ = !shift_; audio::play(SFX_MOVE, 0.6f); }
            else if (keyCol_ == K_DEL) deleteChar();
            else { mode_ = MODE_EDIT; audio::play(SFX_OK, 0.7f); }
        }
    }

    Mode mode_ = MODE_LIST;
    Confirm confirm_ = CONF_DELETE;
    ui::ListMenu list_, edit_, confirmMenu_;
    int slots_[MAX_PROFILES];
    int count_ = 0;
    int editing_ = 0;
    int fields_[F_COUNT];  // campos visibles en el editor (con Miis: Mii, txapela, guardar, borrar)
    int nFields_ = 0;
    Profile work_;
    float arrowLX_ = 0, arrowRX_ = 0, arrowY_ = -1;  // flechas de la fila elegida (para el puntero)
    int keyRow_ = 0, keyCol_ = 0;
    bool shift_ = true;
    float t_ = 0, msgT_ = 0;
    const char* msg_ = "";
};

void ProfilesScene::draw() {
    const float W = gfx::screenW();
    char buf[48];
    gfx::begin2D();
    ui::background(t_);

    const Profile* shown = nullptr;
    if (mode_ == MODE_LIST) {
        const Profile& p = save::data().profiles[slots_[list_.sel]];
        if (p.used) shown = &p;
    } else {
        shown = &work_;
    }
    if (shown && mode_ != MODE_KEYBOARD) {
        drawCharacterPreview(Appearance::from(*shown), 1.0f * W / 854.0f, sinf(t_ * 0.7f) * 0.6f, t_, 0);
        gfx::begin2D();
    }
    ui::header(txt::PLAYERS);

    if (mode_ == MODE_LIST) {
        for (int i = 0; i < count_; i++) {
            const Profile& p = save::data().profiles[slots_[i]];
            float y = 112 + i * 40;
            float a = list_.anim[i];
            float x = 40 + a * 10;
            list_.place(i, 40, y, 340, 36);
            ui::row(x, y, 330, 36, a);
            if (p.used) {
                ui::colorDot(x + 22, y + 18, 9, FAV_COLORS[p.color % NUM_COLORS]);
                font::plain(p.name, x + 42, y + 5, 25, UI_TEXT);
            } else {
                font::plain("+", x + 22, y + 2, 28, UI_TEAL, ALIGN_CENTER);
                font::plain(txt::NEW_PLAYER, x + 42, y + 6, 23, UI_TEAL);
            }
        }
        ui::hints(txt::EDIT, txt::BACK);
    } else if (mode_ == MODE_EDIT || mode_ == MODE_CONFIRM) {
        float px = 40, py = 100;
        arrowY_ = -1;
        ui::panel(px, py, 380, 22 + nFields_ * 38);
        const char* labels[F_COUNT] = {txt::MII,         txt::NAME,    txt::COLOR, txt::SKIN,  txt::HAIR,
                                       txt::HAIR_STYLE, txt::TXAPELA, txt::SAVE,  txt::DELETE};
        for (int k = 0; k < nFields_; k++) {
            int i = fields_[k];
            float y = py + 12 + k * 38;
            float a = edit_.anim[k];
            edit_.place(k, px + 10, y - 2, 360, 38);
            if (a > 0.05f) gfx::roundRect(px + 10, y - 2, 360, 38, 19, withAlpha(rgba(0xC8, 0xE6, 0xFF), a));
            Color tc = edit_.enabled[k] ? UI_TEXT : UI_TEXT_LIGHT;
            if (i >= F_SAVE) {
                font::plain(labels[i], px + 190, y + 4, 26, i == F_SAVE ? UI_BLUE_DARK : (edit_.enabled[k] ? UI_RED : UI_TEXT_LIGHT), ALIGN_CENTER);
                continue;
            }
            font::plain(labels[i], px + 24, y + 6, 23, tc);
            float vx = i == F_MII ? px + 240 : px + 270;
            if (i == F_MII && a > 0.5f) {
                gfx::triangle(vx - 92, y + 18, vx - 82, y + 10, vx - 82, y + 26, UI_BLUE);
                gfx::triangle(vx + 92, y + 18, vx + 82, y + 10, vx + 82, y + 26, UI_BLUE);
            }
            if (k == edit_.sel) {
                bool arrows = i == F_MII || (i >= F_COLOR && i <= F_TXAPELA);
                float off = i == F_MII ? 87.0f : 27.0f;
                arrowLX_ = vx - off;
                arrowRX_ = vx + off;
                arrowY_ = arrows ? y + 18 : -1;
            }
            switch (i) {
                case F_MII: font::plain(work_.name, vx, y + 4, 25, UI_TEXT, ALIGN_CENTER); break;
                case F_NAME: font::plain(work_.name, px + 360, y + 4, 25, UI_TEXT, ALIGN_RIGHT); break;
                case F_COLOR: ui::colorDot(vx, y + 18, 12, FAV_COLORS[work_.color]); break;
                case F_SKIN: ui::colorDot(vx, y + 18, 12, SKIN_COLORS[work_.skin]); break;
                case F_HAIR: ui::colorDot(vx, y + 18, 12, HAIR_COLORS[work_.hair]); break;
                case F_STYLE:
                    font::plain(txt::HAIR_STYLES[work_.hairStyle % NUM_HAIR_STYLES], vx, y + 6, 22, UI_TEXT, ALIGN_CENTER);
                    break;
                case F_TXAPELA: font::plain(work_.txapela ? txt::YES : txt::NO, vx, y + 4, 25, UI_TEXT, ALIGN_CENTER); break;
            }
            if (i >= F_COLOR && i <= F_TXAPELA && a > 0.5f) {
                gfx::triangle(vx - 32, y + 18, vx - 22, y + 10, vx - 22, y + 26, UI_BLUE);
                gfx::triangle(vx + 32, y + 18, vx + 22, y + 10, vx + 22, y + 26, UI_BLUE);
            }
        }
        // Cómo se cambian los valores (cruceta o, con el puntero, las flechas)
        {
            float hy = py + 22 + nFields_ * 38 + 10, hw = font::width(txt::CHANGE_HINT, 17) + 30;
            gfx::roundRect(px, hy, hw, 28, 14, rgba(255, 255, 255, 190));
            font::plain(txt::CHANGE_HINT, px + 15, hy + 4, 17, UI_TEXT_LIGHT);
        }
        if (mode_ == MODE_CONFIRM) {
            // Diálogo de confirmación encima del editor
            ui::fade(0.4f);
            float dw = 470, dh = 210, dx = W / 2 - dw / 2, dy = 130;
            ui::panel(dx, dy, dw, dh);
            if (confirm_ == CONF_DELETE) snprintf(buf, sizeof(buf), txt::DELETE_Q, work_.name);
            else snprintf(buf, sizeof(buf), "%s", txt::UNSAVED_Q);
            font::plain(buf, W / 2, dy + 34, 32, UI_TEXT, ALIGN_CENTER);
            const char* a = confirm_ == CONF_DELETE ? txt::DELETE : txt::SAVE;
            const char* b = confirm_ == CONF_DELETE ? txt::NO : txt::DONT_SAVE;
            ui::menuButton(confirmMenu_, 0, W / 2 - 110, dy + 140, 190, 64, a);
            ui::menuButton(confirmMenu_, 1, W / 2 + 110, dy + 140, 190, 64, b);
        }
        ui::hints(txt::SELECT, txt::BACK);
    } else {
        // Teclado
        float kw = KEY_W, kh = KEY_H;
        gfx::roundRect(W / 2 - 230, 104, 460, 64, 32, UI_WHITE);
        snprintf(buf, sizeof(buf), "%s%s", work_.name, fmodf(t_, 1.0f) < 0.5f ? "|" : " ");
        font::plain(buf, W / 2, 114, 38, UI_TEXT, ALIGN_CENTER);
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 10; c++) {
                bool sel = keyRow_ == r && keyCol_ == c;
                float x, y, w, h;
                keyRect(r, c, x, y, w, h);
                gfx::roundRect(x, y + 3, kw, kh, 12, rgba(0, 0, 0, 40));
                gfx::roundRect(x, y, kw, kh, 12, sel ? UI_BLUE : UI_WHITE);
                const char* k = KEYS[r][c];
                char lab[4];
                strcpy(lab, k);
                if (!shift_) {
                    if (lab[0] >= 'A' && lab[0] <= 'Z') lab[0] = lab[0] - 'A' + 'a';
                    else if (!strcmp(lab, "Ñ")) strcpy(lab, "ñ");
                }
                if (!strcmp(lab, " ")) strcpy(lab, "_");
                font::plain(lab, x + kw / 2, y + 9, 30, sel ? UI_WHITE : UI_TEXT, ALIGN_CENTER);
            }
        const char* special[K_SPECIAL_COUNT] = {"Aa", "<", txt::OK};
        for (int c = 0; c < K_SPECIAL_COUNT; c++) {
            bool sel = keyRow_ == 3 && keyCol_ == c;
            float x, y, sw, h;
            keyRect(3, c, x, y, sw, h);
            gfx::roundRect(x, y + 3, sw, kh, 14, rgba(0, 0, 0, 40));
            Color bg = sel ? UI_BLUE : (c == K_OK ? rgba(0xD8, 0xF0, 0xD8) : UI_WHITE);
            if (c == K_SHIFT && shift_ && !sel) bg = rgba(0xFF, 0xF0, 0xB8);
            gfx::roundRect(x, y, sw, kh, 14, bg);
            font::plain(special[c], x + sw / 2, y + 9, 30, sel ? UI_WHITE : UI_TEXT, ALIGN_CENTER);
        }
        ui::hints(txt::SELECT, "<", txt::OK);
    }
    if (msgT_ > 0) font::draw(msg_, W / 2, 392, 26, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER, clampv(msgT_ * 2, 0.0f, 1.0f));
}

}  // namespace

Scene* makeProfilesScene() { return new ProfilesScene(); }
