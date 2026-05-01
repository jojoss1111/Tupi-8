// Camera.c — TupiEngine (SDL2)

#include "Camera.h"
#include "../Renderizador/Renderer.h"
#include "../Sprites/Sprites.h"

// Câmera ativa e dimensões da janela
static TupiCamera* _cam_ativa = NULL;
static int _win_w = 160;
static int _win_h = 144;

// Recalcula _cam_x/_cam_y com base no alvo e na âncora
static void _recalcular(TupiCamera* cam, int largura, int altura) {
    float ax = (cam->ancora_x < 0) ? largura * 0.5f : cam->ancora_x;
    float ay = (cam->ancora_y < 0) ? altura  * 0.5f : cam->ancora_y;

    // roundf() faz pixel-snapping (evita bleeding nos tiles)
    cam->_cam_x = roundf(cam->alvo_x - ax / cam->zoom);
    cam->_cam_y = roundf(cam->alvo_y - ay / cam->zoom);
}

// Monta a view matrix 4x4 (column-major) com zoom e rotação
static TupiMatriz _view_matrix(const TupiCamera* cam) {
    float co = cosf(-cam->rotacao) * cam->zoom;
    float si = sinf(-cam->rotacao) * cam->zoom;

    float cx = roundf(cam->_cam_x);
    float cy = roundf(cam->_cam_y);

    float tx = -(cx * co - cy * si);
    float ty = -(cx * si + cy * co);

    TupiMatriz v = {{0}};
    v.m[0]  =  co;   v.m[4]  = -si;   v.m[8]  = 0.0f;  v.m[12] = tx;
    v.m[1]  =  si;   v.m[5]  =  co;   v.m[9]  = 0.0f;  v.m[13] = ty;
    v.m[2]  = 0.0f;  v.m[6]  = 0.0f;  v.m[10] = 1.0f;  v.m[14] = 0.0f;
    v.m[3]  = 0.0f;  v.m[7]  = 0.0f;  v.m[11] = 0.0f;  v.m[15] = 1.0f;
    return v;
}

// --- Ciclo de vida ---

TupiCamera tupi_camera_criar(float alvo_x, float alvo_y,
                              float ancora_x, float ancora_y) {
    TupiCamera cam = {0};
    cam.alvo_x   = alvo_x;
    cam.alvo_y   = alvo_y;
    cam.ancora_x = ancora_x;
    cam.ancora_y = ancora_y;
    cam.zoom     = 1.0f;
    cam.rotacao  = 0.0f;
    cam._ativo   = 1;
    cam._cam_x   = roundf(alvo_x);
    cam._cam_y   = roundf(alvo_y);
    return cam;
}

void tupi_camera_destruir(TupiCamera* cam) {
    if (!cam) return;
    if (_cam_ativa == cam) _cam_ativa = NULL;
    *cam = (TupiCamera){0};
}

// --- Ativação ---

void tupi_camera_ativar(TupiCamera* cam) {
    if (!cam || !cam->_ativo) return;
    _cam_ativa = cam;
    _win_w = tupi_janela_largura();
    _win_h = tupi_janela_altura();
    _recalcular(cam, _win_w, _win_h);
}

TupiCamera* tupi_camera_ativa(void) { return _cam_ativa; }

// --- Frame ---

void tupi_camera_frame(TupiCamera* cam, int largura, int altura) {
    if (!cam || !cam->_ativo) return;
    _win_w = largura;
    _win_h = altura;

    _recalcular(cam, largura, altura);

    // Calcula MVP e envia para o sprite renderer
    TupiMatriz proj = tupi_projecao_ortografica(largura, altura);
    TupiMatriz view = _view_matrix(cam);
    TupiMatriz mvp  = tupi_mat4_multiplicar(&proj, &view);

    tupi_sprite_set_viewport(largura, altura);
    tupi_sprite_set_projecao(mvp.m);
}

// --- Movimentação ---

void tupi_camera_pos(TupiCamera* cam, float x, float y) {
    if (!cam || !cam->_ativo) return;
    if (!tupi_camera_validar(x, y, cam->zoom, cam->rotacao)) {
        fprintf(stderr, "[Camera] pos invalida (%.2f, %.2f)\n", x, y);
        return;
    }
    cam->alvo_x = x;
    cam->alvo_y = y;
}

void tupi_camera_mover(TupiCamera* cam, float dx, float dy) {
    if (!cam || !cam->_ativo) return;
    tupi_camera_pos(cam, cam->alvo_x + dx, cam->alvo_y + dy);
}

void tupi_camera_zoom(TupiCamera* cam, float z) {
    if (!cam || !cam->_ativo) return;
    if (!tupi_camera_validar(cam->alvo_x, cam->alvo_y, z, cam->rotacao)) {
        fprintf(stderr, "[Camera] zoom invalido (%.4f)\n", z);
        return;
    }
    cam->zoom = z;
}

void tupi_camera_rotacao(TupiCamera* cam, float angulo) {
    if (!cam || !cam->_ativo) return;
    if (!tupi_camera_validar(cam->alvo_x, cam->alvo_y, cam->zoom, angulo)) {
        fprintf(stderr, "[Camera] rotacao invalida (%.4f)\n", angulo);
        return;
    }
    cam->rotacao = angulo;
}

void tupi_camera_ancora(TupiCamera* cam, float ax, float ay) {
    if (!cam || !cam->_ativo) return;
    cam->ancora_x = ax;
    cam->ancora_y = ay;
}

// Lerp exponencial — suaviza o seguimento independente do frame rate
void tupi_camera_seguir(TupiCamera* cam,
                         float alvo_x, float alvo_y,
                         float lerp_fator, float dt) {
    if (!cam || !cam->_ativo) return;
    float fator = 1.0f - powf(1.0f - lerp_fator, dt * 60.0f);
    float nx = cam->alvo_x + (alvo_x - cam->alvo_x) * fator;
    float ny = cam->alvo_y + (alvo_y - cam->alvo_y) * fator;
    if (tupi_camera_validar(nx, ny, cam->zoom, cam->rotacao)) {
        cam->alvo_x = nx;
        cam->alvo_y = ny;
    }
}

// --- Getters ---

float tupi_camera_get_x(const TupiCamera* cam)       { return cam ? cam->_cam_x  : 0.0f; }
float tupi_camera_get_y(const TupiCamera* cam)       { return cam ? cam->_cam_y  : 0.0f; }
float tupi_camera_get_zoom(const TupiCamera* cam)    { return cam ? cam->zoom    : 1.0f; }
float tupi_camera_get_rotacao(const TupiCamera* cam) { return cam ? cam->rotacao : 0.0f; }

// --- Conversão de coordenadas ---

// Tela → mundo
void tupi_camera_tela_para_mundo(const TupiCamera* cam, int largura, int altura,
                                  float sx, float sy, float* wx, float* wy) {
    float dx = (sx - (cam->ancora_x < 0 ? largura * 0.5f : cam->ancora_x)) / cam->zoom;
    float dy = (sy - (cam->ancora_y < 0 ? altura  * 0.5f : cam->ancora_y)) / cam->zoom;
    float co = cosf(cam->rotacao);
    float si = sinf(cam->rotacao);
    *wx = cam->_cam_x + (dx * co - dy * si);
    *wy = cam->_cam_y + (dx * si + dy * co);
}

// Mundo → tela
void tupi_camera_mundo_para_tela(const TupiCamera* cam, int largura, int altura,
                                  float wx, float wy, float* sx, float* sy) {
    float dx = wx - cam->_cam_x;
    float dy = wy - cam->_cam_y;
    float co = cosf(-cam->rotacao) * cam->zoom;
    float si = sinf(-cam->rotacao) * cam->zoom;
    float ax = (cam->ancora_x < 0) ? largura * 0.5f : cam->ancora_x;
    float ay = (cam->ancora_y < 0) ? altura  * 0.5f : cam->ancora_y;
    *sx = (dx * co - dy * si) + ax;
    *sy = (dx * si + dy * co) + ay;
}

// --- Wrappers Lua ---

void tupi_camera_tela_mundo_lua(float sx, float sy, float* wx, float* wy) {
    if (!_cam_ativa) { *wx = sx; *wy = sy; return; }
    tupi_camera_tela_para_mundo(_cam_ativa, _win_w, _win_h, sx, sy, wx, wy);
}

void tupi_camera_mundo_tela_lua(float wx, float wy, float* sx, float* sy) {
    if (!_cam_ativa) { *sx = wx; *sy = wy; return; }
    tupi_camera_mundo_para_tela(_cam_ativa, _win_w, _win_h, wx, wy, sx, sy);
}