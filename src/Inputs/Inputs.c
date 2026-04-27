// Inputs.c
// TupiEngine - Sistema de Inputs
// Gerencia teclado, mouse (posição, botões, scroll) com estados por frame

#include "Inputs.h"
#include <string.h>

// ============================================================
// ESTADO INTERNO
// ============================================================

#define TUPI_MAX_TECLAS  512
#define TUPI_MAX_BOTOES  8

// Estado do teclado
// _agora  = estado atual reportado pelo GLFW
// _antes  = estado do frame anterior
// Assim conseguimos detectar "pressionou" e "soltou" num único frame

static int _tecla_agora[TUPI_MAX_TECLAS];
static int _tecla_antes[TUPI_MAX_TECLAS];

// Estado dos botões do mouse
static int _botao_agora[TUPI_MAX_BOTOES];
static int _botao_antes[TUPI_MAX_BOTOES];

// Posição do mouse
static double _mouse_x      = 0.0;
static double _mouse_y      = 0.0;
static double _mouse_x_ant  = 0.0;
static double _mouse_y_ant  = 0.0;

// Scroll acumulado no frame atual
static double _scroll_x = 0.0;
static double _scroll_y = 0.0;

// Scroll acumulado pendente (do callback, antes do atualizar)
static double _scroll_x_pend = 0.0;
static double _scroll_y_pend = 0.0;

// Referência da janela
static GLFWwindow* _janela_ref = NULL;

/* Ponteiros para as dimensões físicas e lógicas do renderer.
   Preenchidos por tupi_input_set_dimensoes() chamado do RendererGL.c.
   Permitem converter screen-coords do GLFW → coordenadas lógicas do mundo. */
static int* _ref_fis_w = NULL;
static int* _ref_fis_h = NULL;
static int* _ref_log_w = NULL;
static int* _ref_log_h = NULL;

// ============================================================
// CALLBACKS GLFW
// ============================================================

static void _cb_teclado(GLFWwindow* janela, int tecla, int scancode, int acao, int mods) {
    (void)janela; (void)scancode; (void)mods;
    if (tecla < 0 || tecla >= TUPI_MAX_TECLAS) return;

    if (acao == GLFW_PRESS) {
        _tecla_agora[tecla] = 1;
    } else if (acao == GLFW_RELEASE) {
        _tecla_agora[tecla] = 0;
    }
    // GLFW_REPEAT ignoramos — usamos _segurando pra isso
}

static void _cb_mouse_botao(GLFWwindow* janela, int botao, int acao, int mods) {
    (void)janela; (void)mods;
    if (botao < 0 || botao >= TUPI_MAX_BOTOES) return;

    if (acao == GLFW_PRESS) {
        _botao_agora[botao] = 1;
    } else if (acao == GLFW_RELEASE) {
        _botao_agora[botao] = 0;
    }
}

static void _cb_scroll(GLFWwindow* janela, double dx, double dy) {
    (void)janela;
    _scroll_x_pend += dx;
    _scroll_y_pend += dy;
}

// ============================================================
// INICIALIZAÇÃO
// ============================================================

void tupi_input_iniciar(GLFWwindow* janela) {
    _janela_ref = janela;

    memset(_tecla_agora,  0, sizeof(_tecla_agora));
    memset(_tecla_antes,  0, sizeof(_tecla_antes));
    memset(_botao_agora,  0, sizeof(_botao_agora));
    memset(_botao_antes,  0, sizeof(_botao_antes));

    glfwSetKeyCallback(janela,         _cb_teclado);
    glfwSetMouseButtonCallback(janela, _cb_mouse_botao);
    glfwSetScrollCallback(janela,      _cb_scroll);
}

/* Registra ponteiros vivos para as dimensões do renderer.
   Deve ser chamado uma vez após tupi_input_iniciar().
   A partir daí, qualquer resize atualiza automaticamente as conversões. */
void tupi_input_set_dimensoes(int* fis_w, int* fis_h, int* log_w, int* log_h) {
    _ref_fis_w = fis_w;
    _ref_fis_h = fis_h;
    _ref_log_w = log_w;
    _ref_log_h = log_h;
}

// ============================================================
// ATUALIZAÇÃO POR FRAME
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
    if (_janela_ref) {
        glfwGetCursorPos(_janela_ref, &_mouse_x, &_mouse_y);
    }
}

// ============================================================
// TECLADO
// ============================================================

// Pressionou: estava 0, agora é 1
int tupi_tecla_pressionou(int tecla) {
    if (tecla < 0 || tecla >= TUPI_MAX_TECLAS) return 0;
    return (_tecla_agora[tecla] == 1 && _tecla_antes[tecla] == 0);
}

// Segurando: está 1 agora (independente do frame anterior)
int tupi_tecla_segurando(int tecla) {
    if (tecla < 0 || tecla >= TUPI_MAX_TECLAS) return 0;
    return _tecla_agora[tecla] == 1;
}

// Soltou: estava 1, agora é 0
int tupi_tecla_soltou(int tecla) {
    if (tecla < 0 || tecla >= TUPI_MAX_TECLAS) return 0;
    return (_tecla_agora[tecla] == 0 && _tecla_antes[tecla] == 1);
}

// ============================================================
// MOUSE - POSIÇÃO
// ============================================================

double tupi_mouse_x(void)  { return _mouse_x; }
double tupi_mouse_y(void)  { return _mouse_y; }
double tupi_mouse_dx(void) { return _mouse_x - _mouse_x_ant; }
double tupi_mouse_dy(void) { return _mouse_y - _mouse_y_ant; }

// ============================================================
// MOUSE - COORDENADAS LÓGICAS (correto após resize da janela)
// ============================================================

/* Converte screen-coord X para coordenada lógica do mundo.
   Quando o usuário redimensiona a janela, o espaço físico muda mas o espaço
   lógico (definido em tupi_janela_criar) permanece fixo. Esta função faz a
   proporção correta: mouse_fisico * (logico / fisico_atual). */
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

// ============================================================
// MOUSE - BOTÕES
// ============================================================

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

// ============================================================
// SCROLL
// ============================================================

double tupi_scroll_x(void) { return _scroll_x; }
double tupi_scroll_y(void) { return _scroll_y; }