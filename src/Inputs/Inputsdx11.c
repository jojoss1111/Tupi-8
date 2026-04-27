// Inputs_DX11.c
// TupiEngine - Sistema de Inputs [backend: Direct3D 11 / Win32]

#include "Inputsdx11.h"
#include <string.h>

// ============================================================
// ESTADO INTERNO — idêntico ao Inputs.c
// ============================================================

#define TUPI_MAX_TECLAS  512
#define TUPI_MAX_BOTOES  8

static int _tecla_agora[TUPI_MAX_TECLAS];
static int _tecla_antes[TUPI_MAX_TECLAS];

static int _botao_agora[TUPI_MAX_BOTOES];
static int _botao_antes[TUPI_MAX_BOTOES];

static double _mouse_x     = 0.0;
static double _mouse_y     = 0.0;
static double _mouse_x_ant = 0.0;
static double _mouse_y_ant = 0.0;

static double _scroll_x      = 0.0;
static double _scroll_y      = 0.0;
static double _scroll_x_pend = 0.0;
static double _scroll_y_pend = 0.0;

static HWND _hwnd_ref = NULL;

// ============================================================
// TABELA DE MAPEAMENTO — Virtual Key → TUPI_TECLA_*
// ============================================================

// Tamanho da tabela interna de VK (Win32 usa até 0xFF = 255)
#define VK_MAX 256

// vk_para_tupi[vk] = código TUPI equivalente (0 = não mapeado)
static int _vk_para_tupi[VK_MAX];

static void _montar_tabela_vk(void) {
    memset(_vk_para_tupi, 0, sizeof(_vk_para_tupi));

    // Letras A-Z: VK_A=65 ... VK_Z=90 — mesmos valores que TUPI/GLFW
    for (int i = 65; i <= 90; i++) _vk_para_tupi[i] = i;

    // Números 0-9: VK_0=48 ... VK_9=57 — mesmos valores
    for (int i = 48; i <= 57; i++) _vk_para_tupi[i] = i;

    // Espaço
    _vk_para_tupi[VK_SPACE]   = TUPI_TECLA_ESPACO;

    // Especiais — VK → TUPI (que espelha o GLFW)
    _vk_para_tupi[VK_RETURN]  = TUPI_TECLA_ENTER;
    _vk_para_tupi[VK_TAB]     = TUPI_TECLA_TAB;
    _vk_para_tupi[VK_BACK]    = TUPI_TECLA_BACKSPACE;
    _vk_para_tupi[VK_ESCAPE]  = TUPI_TECLA_ESC;

    // Shift / Ctrl / Alt
    _vk_para_tupi[VK_LSHIFT]   = TUPI_TECLA_SHIFT_ESQ;
    _vk_para_tupi[VK_RSHIFT]   = TUPI_TECLA_SHIFT_DIR;
    _vk_para_tupi[VK_LCONTROL] = TUPI_TECLA_CTRL_ESQ;
    _vk_para_tupi[VK_RCONTROL] = TUPI_TECLA_CTRL_DIR;
    _vk_para_tupi[VK_LMENU]    = TUPI_TECLA_ALT_ESQ;
    _vk_para_tupi[VK_RMENU]    = TUPI_TECLA_ALT_DIR;

    // Setas
    _vk_para_tupi[VK_UP]    = TUPI_TECLA_CIMA;
    _vk_para_tupi[VK_DOWN]  = TUPI_TECLA_BAIXO;
    _vk_para_tupi[VK_LEFT]  = TUPI_TECLA_ESQUERDA;
    _vk_para_tupi[VK_RIGHT] = TUPI_TECLA_DIREITA;

    // Numpad 0-9: VK_NUMPAD0=96 ... VK_NUMPAD9=105
    _vk_para_tupi[VK_NUMPAD0] = TUPI_TECLA_NUM0;
    _vk_para_tupi[VK_NUMPAD1] = TUPI_TECLA_NUM1;
    _vk_para_tupi[VK_NUMPAD2] = TUPI_TECLA_NUM2;
    _vk_para_tupi[VK_NUMPAD3] = TUPI_TECLA_NUM3;
    _vk_para_tupi[VK_NUMPAD4] = TUPI_TECLA_NUM4;
    _vk_para_tupi[VK_NUMPAD5] = TUPI_TECLA_NUM5;
    _vk_para_tupi[VK_NUMPAD6] = TUPI_TECLA_NUM6;
    _vk_para_tupi[VK_NUMPAD7] = TUPI_TECLA_NUM7;
    _vk_para_tupi[VK_NUMPAD8] = TUPI_TECLA_NUM8;
    _vk_para_tupi[VK_NUMPAD9] = TUPI_TECLA_NUM9;

    // F-Keys: VK_F1=112 ... VK_F12=123
    _vk_para_tupi[VK_F1]  = TUPI_TECLA_F1;
    _vk_para_tupi[VK_F2]  = TUPI_TECLA_F2;
    _vk_para_tupi[VK_F3]  = TUPI_TECLA_F3;
    _vk_para_tupi[VK_F4]  = TUPI_TECLA_F4;
    _vk_para_tupi[VK_F5]  = TUPI_TECLA_F5;
    _vk_para_tupi[VK_F6]  = TUPI_TECLA_F6;
    _vk_para_tupi[VK_F7]  = TUPI_TECLA_F7;
    _vk_para_tupi[VK_F8]  = TUPI_TECLA_F8;
    _vk_para_tupi[VK_F9]  = TUPI_TECLA_F9;
    _vk_para_tupi[VK_F10] = TUPI_TECLA_F10;
    _vk_para_tupi[VK_F11] = TUPI_TECLA_F11;
    _vk_para_tupi[VK_F12] = TUPI_TECLA_F12;
}

// ============================================================
// INICIALIZAÇÃO
// ============================================================

void tupi_input_iniciar(HWND hwnd) {
    _hwnd_ref = hwnd;

    memset(_tecla_agora, 0, sizeof(_tecla_agora));
    memset(_tecla_antes, 0, sizeof(_tecla_antes));
    memset(_botao_agora, 0, sizeof(_botao_agora));
    memset(_botao_antes, 0, sizeof(_botao_antes));
    _montar_tabela_vk();

}

// ============================================================
// WNDPROC — chamado pelo _wnd_proc do RendererDX11.c
//
// RendererDX11.c deve chamar tupi_input_wndproc(msg, wp, lp)
// logo no início do seu _wnd_proc, por exemplo:
//
//   if (tupi_input_wndproc(msg, wp, lp)) return 0;
//
// ============================================================

int tupi_input_wndproc(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

        // ── Teclado ──────────────────────────────────────────
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            int vk = (int)wp;
            if (vk >= 0 && vk < VK_MAX) {
                int tupi = _vk_para_tupi[vk];
                if (tupi > 0 && tupi < TUPI_MAX_TECLAS) {
                    _tecla_agora[tupi] = 1;
                }
            }
            return 1;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP: {
            int vk = (int)wp;
            if (vk >= 0 && vk < VK_MAX) {
                int tupi = _vk_para_tupi[vk];
                if (tupi > 0 && tupi < TUPI_MAX_TECLAS) {
                    _tecla_agora[tupi] = 0;
                }
            }
            return 1;
        }

        // ── Mouse — Posição ───────────────────────────────────
        case WM_MOUSEMOVE: {
            _mouse_x = (double)(short)LOWORD(lp);
            _mouse_y = (double)(short)HIWORD(lp);
            return 1;
        }

        // ── Mouse — Botões ────────────────────────────────────
        case WM_LBUTTONDOWN: _botao_agora[TUPI_MOUSE_ESQ]  = 1; return 1;
        case WM_LBUTTONUP:   _botao_agora[TUPI_MOUSE_ESQ]  = 0; return 1;
        case WM_RBUTTONDOWN: _botao_agora[TUPI_MOUSE_DIR]  = 1; return 1;
        case WM_RBUTTONUP:   _botao_agora[TUPI_MOUSE_DIR]  = 0; return 1;
        case WM_MBUTTONDOWN: _botao_agora[TUPI_MOUSE_MEIO] = 1; return 1;
        case WM_MBUTTONUP:   _botao_agora[TUPI_MOUSE_MEIO] = 0; return 1;

        // ── Scroll ────────────────────────────────────────────
        case WM_MOUSEWHEEL: {
            // GET_WHEEL_DELTA_WPARAM retorna múltiplos de WHEEL_DELTA (120)
            // Normalizamos para que 1 clique = 1.0, igual ao GLFW
            short delta = GET_WHEEL_DELTA_WPARAM(wp);
            _scroll_y_pend += (double)delta / WHEEL_DELTA;
            return 1;
        }

        case WM_MOUSEHWHEEL: {
            short delta = GET_WHEEL_DELTA_WPARAM(wp);
            _scroll_x_pend += (double)delta / WHEEL_DELTA;
            return 1;
        }

        default:
            return 0; // mensagem não consumida — segue para DefWindowProc
    }
}

// ============================================================
// ATUALIZAÇÃO POR FRAME — idêntica ao Inputs.c
// ============================================================

void tupi_input_salvar_estado(void) {
    memcpy(_tecla_antes, _tecla_agora, sizeof(_tecla_agora));
    memcpy(_botao_antes, _botao_agora, sizeof(_botao_agora));
    _mouse_x_ant = _mouse_x;
    _mouse_y_ant = _mouse_y;
    _scroll_x = _scroll_x_pend;
    _scroll_y = _scroll_y_pend;
    _scroll_x_pend = 0.0;
    _scroll_y_pend = 0.0;
}

void tupi_input_atualizar_mouse(void) {
    if (_hwnd_ref) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(_hwnd_ref, &pt);
        _mouse_x = (double)pt.x;
        _mouse_y = (double)pt.y;
    }
}

// ============================================================
// TECLADO — idêntico ao Inputs.c
// ============================================================

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

// ============================================================
// MOUSE — idêntico ao Inputs.c
// ============================================================

double tupi_mouse_x(void)  { return _mouse_x; }
double tupi_mouse_y(void)  { return _mouse_y; }
double tupi_mouse_dx(void) { return _mouse_x - _mouse_x_ant; }
double tupi_mouse_dy(void) { return _mouse_y - _mouse_y_ant; }

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

double tupi_scroll_x(void) { return _scroll_x; }
double tupi_scroll_y(void) { return _scroll_y; }