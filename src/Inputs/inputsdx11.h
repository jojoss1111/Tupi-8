// Inputs_DX11.h
// TupiEngine - Sistema de Inputs [backend: Direct3D 11 / Win32]

#ifndef INPUTS_DX11_H
#define INPUTS_DX11_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ============================================================
// INICIALIZAÇÃO (chamada internamente por RendererDX11.c)
// ============================================================

// Registra o HWND para leitura de mouse — o teclado/scroll
// é capturado diretamente no _wnd_proc via tupi_input_wndproc.
void tupi_input_iniciar(HWND hwnd);

// Deve ser chamada DENTRO do _wnd_proc do RendererDX11.c
// para capturar WM_KEYDOWN, WM_KEYUP, WM_MOUSEMOVE, etc.
// Retorna 1 se a mensagem foi consumida, 0 se deve seguir para DefWindowProc.
int  tupi_input_wndproc(UINT msg, WPARAM wp, LPARAM lp);

// Atualiza estado dos inputs (chamar uma vez por frame)
void tupi_input_salvar_estado(void);
void tupi_input_atualizar_mouse(void);

// ============================================================
// TECLADO — API idêntica ao Inputs.h
// ============================================================

int tupi_tecla_pressionou(int tecla);
int tupi_tecla_segurando(int tecla);
int tupi_tecla_soltou(int tecla);

// ============================================================
// MOUSE - POSIÇÃO — API idêntica ao Inputs.h
// ============================================================

double tupi_mouse_x(void);
double tupi_mouse_y(void);
double tupi_mouse_dx(void);
double tupi_mouse_dy(void);

// ============================================================
// MOUSE - BOTÕES — API idêntica ao Inputs.h
// ============================================================

int tupi_mouse_clicou(int botao);
int tupi_mouse_segurando(int botao);
int tupi_mouse_soltou(int botao);

// ============================================================
// MOUSE - SCROLL — API idêntica ao Inputs.h
// ============================================================

double tupi_scroll_x(void);
double tupi_scroll_y(void);

// ============================================================
// CONSTANTES DE TECLA
// Mapeadas para Virtual Key codes do Win32.
// Os VALORES são idênticos ao Inputs.h — código de jogo não muda.
// ============================================================

// Teclas especiais
#define TUPI_TECLA_ESPACO       32    // VK_SPACE
#define TUPI_TECLA_ENTER        257   // VK_RETURN  (remapeado internamente)
#define TUPI_TECLA_TAB          258   // VK_TAB
#define TUPI_TECLA_BACKSPACE    259   // VK_BACK
#define TUPI_TECLA_ESC          256   // VK_ESCAPE
#define TUPI_TECLA_SHIFT_ESQ    340   // VK_LSHIFT
#define TUPI_TECLA_SHIFT_DIR    344   // VK_RSHIFT
#define TUPI_TECLA_CTRL_ESQ     341   // VK_LCONTROL
#define TUPI_TECLA_CTRL_DIR     345   // VK_RCONTROL
#define TUPI_TECLA_ALT_ESQ      342   // VK_LMENU
#define TUPI_TECLA_ALT_DIR      346   // VK_RMENU

// Setas
#define TUPI_TECLA_CIMA         265   // VK_UP
#define TUPI_TECLA_BAIXO        264   // VK_DOWN
#define TUPI_TECLA_ESQUERDA     263   // VK_LEFT
#define TUPI_TECLA_DIREITA      262   // VK_RIGHT

// Letras A-Z
#define TUPI_TECLA_A            65
#define TUPI_TECLA_B            66
#define TUPI_TECLA_C            67
#define TUPI_TECLA_D            68
#define TUPI_TECLA_E            69
#define TUPI_TECLA_F            70
#define TUPI_TECLA_G            71
#define TUPI_TECLA_H            72
#define TUPI_TECLA_I            73
#define TUPI_TECLA_J            74
#define TUPI_TECLA_K            75
#define TUPI_TECLA_L            76
#define TUPI_TECLA_M            77
#define TUPI_TECLA_N            78
#define TUPI_TECLA_O            79
#define TUPI_TECLA_P            80
#define TUPI_TECLA_Q            81
#define TUPI_TECLA_R            82
#define TUPI_TECLA_S            83
#define TUPI_TECLA_T            84
#define TUPI_TECLA_U            85
#define TUPI_TECLA_V            86
#define TUPI_TECLA_W            87
#define TUPI_TECLA_X            88
#define TUPI_TECLA_Y            89
#define TUPI_TECLA_Z            90

// Números linha de cima
#define TUPI_TECLA_0            48
#define TUPI_TECLA_1            49
#define TUPI_TECLA_2            50
#define TUPI_TECLA_3            51
#define TUPI_TECLA_4            52
#define TUPI_TECLA_5            53
#define TUPI_TECLA_6            54
#define TUPI_TECLA_7            55
#define TUPI_TECLA_8            56
#define TUPI_TECLA_9            57

// Numpad
#define TUPI_TECLA_NUM0         320
#define TUPI_TECLA_NUM1         321
#define TUPI_TECLA_NUM2         322
#define TUPI_TECLA_NUM3         323
#define TUPI_TECLA_NUM4         324
#define TUPI_TECLA_NUM5         325
#define TUPI_TECLA_NUM6         326
#define TUPI_TECLA_NUM7         327
#define TUPI_TECLA_NUM8         328
#define TUPI_TECLA_NUM9         329

// F-Keys
#define TUPI_TECLA_F1           290
#define TUPI_TECLA_F2           291
#define TUPI_TECLA_F3           292
#define TUPI_TECLA_F4           293
#define TUPI_TECLA_F5           294
#define TUPI_TECLA_F6           295
#define TUPI_TECLA_F7           296
#define TUPI_TECLA_F8           297
#define TUPI_TECLA_F9           298
#define TUPI_TECLA_F10          299
#define TUPI_TECLA_F11          300
#define TUPI_TECLA_F12          301

// Botões do mouse — idênticos ao Inputs.h
#define TUPI_MOUSE_ESQ          0
#define TUPI_MOUSE_DIR          1
#define TUPI_MOUSE_MEIO         2

#endif // INPUTS_DX11_H