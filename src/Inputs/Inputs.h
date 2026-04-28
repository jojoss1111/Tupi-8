// Inputs.h — TupiEngine (SDL2)

#ifndef INPUTS_H
#define INPUTS_H

#include <SDL2/SDL.h>

// --- Inicialização ---

void tupi_input_iniciar(SDL_Window* janela);
void tupi_input_processar_evento(const SDL_Event* ev);

// Registra ponteiros vivos para as dimensões do renderer.
// Necessário para tupi_mouse_mundo_* funcionar após resize.
void tupi_input_set_dimensoes(int* fis_w, int* fis_h, int* log_w, int* log_h);

// Chamar uma vez por frame (antes do poll de eventos)
void tupi_input_salvar_estado(void);
void tupi_input_atualizar_mouse(void);

// --- Teclado ---

int tupi_tecla_pressionou(int tecla);  // só dispara no frame em que pressionou
int tupi_tecla_segurando(int tecla);   // dispara todo frame enquanto segurado
int tupi_tecla_soltou(int tecla);      // só dispara no frame em que soltou

// --- Mouse: posição física (px) ---

double tupi_mouse_x(void);
double tupi_mouse_y(void);
double tupi_mouse_dx(void);
double tupi_mouse_dy(void);

// Mouse: coordenadas lógicas do mundo (correto após resize)
double tupi_mouse_mundo_x(void);
double tupi_mouse_mundo_y(void);
double tupi_mouse_mundo_dx(void);
double tupi_mouse_mundo_dy(void);

// --- Mouse: botões ---

int tupi_mouse_clicou(int botao);
int tupi_mouse_segurando(int botao);
int tupi_mouse_soltou(int botao);

// --- Mouse: scroll (acumulado no frame) ---

double tupi_scroll_x(void);
double tupi_scroll_y(void);

// --- Constantes de tecla ---

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

#define TUPI_TECLA_CIMA         265
#define TUPI_TECLA_BAIXO        264
#define TUPI_TECLA_ESQUERDA     263
#define TUPI_TECLA_DIREITA      262

#define TUPI_TECLA_A  65
#define TUPI_TECLA_B  66
#define TUPI_TECLA_C  67
#define TUPI_TECLA_D  68
#define TUPI_TECLA_E  69
#define TUPI_TECLA_F  70
#define TUPI_TECLA_G  71
#define TUPI_TECLA_H  72
#define TUPI_TECLA_I  73
#define TUPI_TECLA_J  74
#define TUPI_TECLA_K  75
#define TUPI_TECLA_L  76
#define TUPI_TECLA_M  77
#define TUPI_TECLA_N  78
#define TUPI_TECLA_O  79
#define TUPI_TECLA_P  80
#define TUPI_TECLA_Q  81
#define TUPI_TECLA_R  82
#define TUPI_TECLA_S  83
#define TUPI_TECLA_T  84
#define TUPI_TECLA_U  85
#define TUPI_TECLA_V  86
#define TUPI_TECLA_W  87
#define TUPI_TECLA_X  88
#define TUPI_TECLA_Y  89
#define TUPI_TECLA_Z  90

#define TUPI_TECLA_0  48
#define TUPI_TECLA_1  49
#define TUPI_TECLA_2  50
#define TUPI_TECLA_3  51
#define TUPI_TECLA_4  52
#define TUPI_TECLA_5  53
#define TUPI_TECLA_6  54
#define TUPI_TECLA_7  55
#define TUPI_TECLA_8  56
#define TUPI_TECLA_9  57

#define TUPI_TECLA_NUM0  320
#define TUPI_TECLA_NUM1  321
#define TUPI_TECLA_NUM2  322
#define TUPI_TECLA_NUM3  323
#define TUPI_TECLA_NUM4  324
#define TUPI_TECLA_NUM5  325
#define TUPI_TECLA_NUM6  326
#define TUPI_TECLA_NUM7  327
#define TUPI_TECLA_NUM8  328
#define TUPI_TECLA_NUM9  329

#define TUPI_TECLA_F1   290
#define TUPI_TECLA_F2   291
#define TUPI_TECLA_F3   292
#define TUPI_TECLA_F4   293
#define TUPI_TECLA_F5   294
#define TUPI_TECLA_F6   295
#define TUPI_TECLA_F7   296
#define TUPI_TECLA_F8   297
#define TUPI_TECLA_F9   298
#define TUPI_TECLA_F10  299
#define TUPI_TECLA_F11  300
#define TUPI_TECLA_F12  301

#define TUPI_MOUSE_ESQ   0
#define TUPI_MOUSE_DIR   1
#define TUPI_MOUSE_MEIO  2

#endif // INPUTS_H