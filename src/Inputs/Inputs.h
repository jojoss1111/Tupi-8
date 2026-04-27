// Inputs.h
// TupiEngine - Sistema de Inputs
// Teclado: digitou, pressionando, soltou
// Mouse: posição, botões, scroll

#ifndef INPUTS_H
#define INPUTS_H

#include <GLFW/glfw3.h>

// ============================================================
// INICIALIZAÇÃO (chamada internamente pela janela)
// ============================================================

// Registra os callbacks de input na janela GLFW
void tupi_input_iniciar(GLFWwindow* janela);

// Registra ponteiros vivos para as dimensões físicas e lógicas do renderer.
// Deve ser chamado uma vez após tupi_input_iniciar() no RendererGL.c.
// Permite que tupi_mouse_mundo_* converta corretamente mesmo após resize.
void tupi_input_set_dimensoes(int* fis_w, int* fis_h, int* log_w, int* log_h);

// Atualiza estado dos inputs (chamar uma vez por frame, antes de poll)
void tupi_input_salvar_estado(void);
void tupi_input_atualizar_mouse(void);

// ============================================================
// TECLADO
// ============================================================

// Retorna 1 se a tecla foi PRESSIONADA nesse frame (só dispara uma vez)
int tupi_tecla_pressionou(int tecla);

// Retorna 1 se a tecla está SENDO SEGURADA (dispara todo frame)
int tupi_tecla_segurando(int tecla);

// Retorna 1 se a tecla foi SOLTA nesse frame (só dispara uma vez)
int tupi_tecla_soltou(int tecla);

// ============================================================
// MOUSE - POSIÇÃO
// ============================================================

// Posição X do cursor na tela (em pixels)
double tupi_mouse_x(void);

// Posição Y do cursor na tela (em pixels)
double tupi_mouse_y(void);

// Movimento do mouse desde o último frame (delta)
double tupi_mouse_dx(void);
double tupi_mouse_dy(void);

// Posição do mouse convertida para coordenadas LÓGICAS do mundo.
// Funciona corretamente mesmo após o usuário redimensionar a janela.
// Use sempre estas para colisões, UI, pontoRet, pontoCir, etc.
double tupi_mouse_mundo_x(void);
double tupi_mouse_mundo_y(void);
double tupi_mouse_mundo_dx(void);
double tupi_mouse_mundo_dy(void);

// ============================================================
// MOUSE - BOTÕES
// ============================================================

// Retorna 1 se o botão foi CLICADO nesse frame
int tupi_mouse_clicou(int botao);

// Retorna 1 se o botão está SENDO SEGURADO
int tupi_mouse_segurando(int botao);

// Retorna 1 se o botão foi SOLTO nesse frame
int tupi_mouse_soltou(int botao);

// ============================================================
// MOUSE - SCROLL
// ============================================================

// Scroll acumulado nesse frame (positivo = pra cima, negativo = pra baixo)
double tupi_scroll_x(void);
double tupi_scroll_y(void);

// ============================================================
// CONSTANTES DE TECLA (espelham o GLFW para o Lua)
// Uso: tupi_tecla_pressionou(TUPI_TECLA_ESPACO)
// ============================================================

// Teclas especiais
#define TUPI_TECLA_ESPACO       32
#define TUPI_TECLA_ENTER        257
#define TUPI_TECLA_TAB          258
#define TUPI_TECLA_BACKSPACE    259
#define TUPI_TECLA_ESC          256
#define TUPI_TECLA_SHIFT_ESQ    340
#define TUPI_TECLA_SHIFT_DIR    344
#define TUPI_TECLA_CTRL_ESQ     341
#define TUPI_TECLA_CTRL_DIR     345
#define TUPI_TECLA_ALT_ESQ      342
#define TUPI_TECLA_ALT_DIR      346

// Setas direcionais
#define TUPI_TECLA_CIMA         265
#define TUPI_TECLA_BAIXO        264
#define TUPI_TECLA_ESQUERDA     263
#define TUPI_TECLA_DIREITA      262

// Letras (A-Z)
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

// Números da linha de cima
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

// Botões do mouse
#define TUPI_MOUSE_ESQ          0
#define TUPI_MOUSE_DIR          1
#define TUPI_MOUSE_MEIO         2

#endif // INPUTS_H