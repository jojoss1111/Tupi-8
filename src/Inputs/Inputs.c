// Inputs.c — TupiEngine (SDL2)

#include "Inputs.h"
#include <string.h>

#define TUPI_MAX_TECLAS  512
#define TUPI_MAX_BOTOES  8

static int _tecla_agora[TUPI_MAX_TECLAS];
static int _tecla_antes[TUPI_MAX_TECLAS];
static int _botao_agora[TUPI_MAX_BOTOES];
static int _botao_antes[TUPI_MAX_BOTOES];

static double _mouse_x = 0.0, _mouse_y = 0.0;
static double _mouse_x_ant = 0.0, _mouse_y_ant = 0.0;

static double _scroll_x = 0.0, _scroll_y = 0.0;
static double _scroll_x_pend = 0.0, _scroll_y_pend = 0.0;

static SDL_Window* _janela_ref = NULL;
static int *_ref_fis_w, *_ref_fis_h, *_ref_log_w, *_ref_log_h;

// SDL_Scancode → constante TUPI_TECLA_*
static int _mapear_tecla(SDL_Scancode sc, SDL_Keycode key) {
    switch (sc) {
        case SDL_SCANCODE_SPACE:          return TUPI_TECLA_ESPACO;
        case SDL_SCANCODE_RETURN:         return TUPI_TECLA_ENTER;
        case SDL_SCANCODE_TAB:            return TUPI_TECLA_TAB;
        case SDL_SCANCODE_BACKSPACE:      return TUPI_TECLA_BACKSPACE;
        case SDL_SCANCODE_ESCAPE:         return TUPI_TECLA_ESC;
        case SDL_SCANCODE_LSHIFT:         return TUPI_TECLA_SHIFT_ESQ;
        case SDL_SCANCODE_RSHIFT:         return TUPI_TECLA_SHIFT_DIR;
        case SDL_SCANCODE_LCTRL:          return TUPI_TECLA_CTRL_ESQ;
        case SDL_SCANCODE_RCTRL:          return TUPI_TECLA_CTRL_DIR;
        case SDL_SCANCODE_LALT:           return TUPI_TECLA_ALT_ESQ;
        case SDL_SCANCODE_RALT:           return TUPI_TECLA_ALT_DIR;
        case SDL_SCANCODE_UP:             return TUPI_TECLA_CIMA;
        case SDL_SCANCODE_DOWN:           return TUPI_TECLA_BAIXO;
        case SDL_SCANCODE_LEFT:           return TUPI_TECLA_ESQUERDA;
        case SDL_SCANCODE_RIGHT:          return TUPI_TECLA_DIREITA;
        case SDL_SCANCODE_HOME:           return 268;
        case SDL_SCANCODE_END:            return 269;
        case SDL_SCANCODE_PAGEUP:         return 266;
        case SDL_SCANCODE_PAGEDOWN:       return 267;
        case SDL_SCANCODE_DELETE:         return 261;
        case SDL_SCANCODE_INSERT:         return 260;
        case SDL_SCANCODE_F1:             return 290;
        case SDL_SCANCODE_F2:             return 291;
        case SDL_SCANCODE_F3:             return 292;
        case SDL_SCANCODE_F4:             return 293;
        case SDL_SCANCODE_F5:             return 294;
        case SDL_SCANCODE_F6:             return 295;
        case SDL_SCANCODE_F7:             return 296;
        case SDL_SCANCODE_F8:             return 297;
        case SDL_SCANCODE_F9:             return 298;
        case SDL_SCANCODE_F10:            return 299;
        case SDL_SCANCODE_F11:            return 300;
        case SDL_SCANCODE_F12:            return 301;
        case SDL_SCANCODE_KP_0:           return 320;
        case SDL_SCANCODE_KP_1:           return 321;
        case SDL_SCANCODE_KP_2:           return 322;
        case SDL_SCANCODE_KP_3:           return 323;
        case SDL_SCANCODE_KP_4:           return 324;
        case SDL_SCANCODE_KP_5:           return 325;
        case SDL_SCANCODE_KP_6:           return 326;
        case SDL_SCANCODE_KP_7:           return 327;
        case SDL_SCANCODE_KP_8:           return 328;
        case SDL_SCANCODE_KP_9:           return 329;
        case SDL_SCANCODE_MINUS:          return 45;
        case SDL_SCANCODE_EQUALS:         return 61;
        case SDL_SCANCODE_LEFTBRACKET:    return 91;
        case SDL_SCANCODE_RIGHTBRACKET:   return 93;
        case SDL_SCANCODE_BACKSLASH:      return 92;
        case SDL_SCANCODE_SEMICOLON:      return 59;
        case SDL_SCANCODE_APOSTROPHE:     return 39;
        case SDL_SCANCODE_COMMA:          return 44;
        case SDL_SCANCODE_PERIOD:         return 46;
        case SDL_SCANCODE_SLASH:          return 47;
        case SDL_SCANCODE_GRAVE:          return 96;
        case SDL_SCANCODE_NONUSBACKSLASH: return 226;
        default: break;
    }
    if (key >= SDLK_a && key <= SDLK_z) return 65 + (key - SDLK_a);
    if (key >= SDLK_0 && key <= SDLK_9) return 48 + (key - SDLK_0);
    return -1;
}

static int _mapear_botao(Uint8 botao) {
    switch (botao) {
        case SDL_BUTTON_LEFT:   return TUPI_MOUSE_ESQ;
        case SDL_BUTTON_RIGHT:  return TUPI_MOUSE_DIR;
        case SDL_BUTTON_MIDDLE: return TUPI_MOUSE_MEIO;
        default:                return -1;
    }
}

// --- API ---

void tupi_input_iniciar(SDL_Window* janela) {
    _janela_ref = janela;
    memset(_tecla_agora, 0, sizeof(_tecla_agora));
    memset(_tecla_antes, 0, sizeof(_tecla_antes));
    memset(_botao_agora, 0, sizeof(_botao_agora));
    memset(_botao_antes, 0, sizeof(_botao_antes));
}

void tupi_input_set_dimensoes(int* fis_w, int* fis_h, int* log_w, int* log_h) {
    _ref_fis_w = fis_w; _ref_fis_h = fis_h;
    _ref_log_w = log_w; _ref_log_h = log_h;
}

void tupi_input_processar_evento(const SDL_Event* ev) {
    if (!ev) return;

    if (ev->type == SDL_KEYDOWN || ev->type == SDL_KEYUP) {
        int tecla = _mapear_tecla(ev->key.keysym.scancode, ev->key.keysym.sym);
        if (tecla >= 0 && tecla < TUPI_MAX_TECLAS)
            _tecla_agora[tecla] = (ev->type == SDL_KEYDOWN) ? 1 : 0;
        return;
    }

    if (ev->type == SDL_MOUSEBUTTONDOWN || ev->type == SDL_MOUSEBUTTONUP) {
        int botao = _mapear_botao(ev->button.button);
        if (botao >= 0 && botao < TUPI_MAX_BOTOES)
            _botao_agora[botao] = (ev->type == SDL_MOUSEBUTTONDOWN) ? 1 : 0;
        return;
    }

    if (ev->type == SDL_MOUSEWHEEL) {
        _scroll_x_pend += ev->wheel.preciseX;
        _scroll_y_pend += ev->wheel.preciseY;
    }
}

// Copia estado atual → anterior (chamar antes de poll, uma vez por frame)
void tupi_input_salvar_estado(void) {
    memcpy(_tecla_antes, _tecla_agora, sizeof(_tecla_agora));
    memcpy(_botao_antes, _botao_agora, sizeof(_botao_agora));
    _mouse_x_ant  = _mouse_x;
    _mouse_y_ant  = _mouse_y;
    _scroll_x     = _scroll_x_pend;
    _scroll_y     = _scroll_y_pend;
    _scroll_x_pend = 0.0;
    _scroll_y_pend = 0.0;
}

void tupi_input_atualizar_mouse(void) {
    int x, y;
    SDL_GetMouseState(&x, &y);
    _mouse_x = x;
    _mouse_y = y;
}

// --- Teclado ---

int tupi_tecla_pressionou(int tecla) {
    if (tecla < 0 || tecla >= TUPI_MAX_TECLAS) return 0;
    return (_tecla_agora[tecla] == 1 && _tecla_antes[tecla] == 0);
}

int tupi_tecla_segurando(int tecla) {
    if (tecla < 0 || tecla >= TUPI_MAX_TECLAS) return 0;
    return _tecla_agora[tecla] == 1;
}

int tupi_tecla_soltou(int tecla) {
    if (tecla < 0 || tecla >= TUPI_MAX_TECLAS) return 0;
    return (_tecla_agora[tecla] == 0 && _tecla_antes[tecla] == 1);
}

// --- Mouse ---

double tupi_mouse_x(void)  { return _mouse_x; }
double tupi_mouse_y(void)  { return _mouse_y; }
double tupi_mouse_dx(void) { return _mouse_x - _mouse_x_ant; }
double tupi_mouse_dy(void) { return _mouse_y - _mouse_y_ant; }

// Converte px físico → coordenada lógica do mundo
static double _to_logico_x(double v) {
    if (!_ref_fis_w || !_ref_log_w || *_ref_fis_w <= 0) return v;
    return v * ((double)*_ref_log_w / (double)*_ref_fis_w);
}
static double _to_logico_y(double v) {
    if (!_ref_fis_h || !_ref_log_h || *_ref_fis_h <= 0) return v;
    return v * ((double)*_ref_log_h / (double)*_ref_fis_h);
}

double tupi_mouse_mundo_x(void)  { return _to_logico_x(_mouse_x); }
double tupi_mouse_mundo_y(void)  { return _to_logico_y(_mouse_y); }
double tupi_mouse_mundo_dx(void) { return _to_logico_x(_mouse_x - _mouse_x_ant); }
double tupi_mouse_mundo_dy(void) { return _to_logico_y(_mouse_y - _mouse_y_ant); }

int tupi_mouse_clicou(int botao) {
    if (botao < 0 || botao >= TUPI_MAX_BOTOES) return 0;
    return (_botao_agora[botao] == 1 && _botao_antes[botao] == 0);
}

int tupi_mouse_segurando(int botao) {
    if (botao < 0 || botao >= TUPI_MAX_BOTOES) return 0;
    return _botao_agora[botao] == 1;
}

int tupi_mouse_soltou(int botao) {
    if (botao < 0 || botao >= TUPI_MAX_BOTOES) return 0;
    return (_botao_agora[botao] == 0 && _botao_antes[botao] == 1);
}

// --- Scroll ---

double tupi_scroll_x(void) { return _scroll_x; }
double tupi_scroll_y(void) { return _scroll_y; }