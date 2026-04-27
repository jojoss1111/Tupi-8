// Camera/Cameradx11.c — TupiEngine [backend: Direct3D 11]

#include "Camera.h"
#include "../Renderizador/RendererDX11.h"

extern void tupi_sprite_set_projecao(const float* mat4);

// --- Estado interno ---

static TupiCamera _cam  = { 0.0f, 0.0f, 1.0f, 0.0f };
static int        _win_w = 800;
static int        _win_h = 600;

// --- View matrix ---

static TupiMatriz _view_matrix(const TupiCamera* c) {
    float co = cosf(-c->rotacao) * c->zoom;
    float si = sinf(-c->rotacao) * c->zoom;
    float tx = -(c->x * co + c->y * si);
    float ty =  (c->x * si - c->y * co);

    TupiMatriz v = {{0}};
    v.m[0]  =  co;   v.m[4]  =  si;   v.m[8]  = 0.0f;  v.m[12] = tx;
    v.m[1]  = -si;   v.m[5]  =  co;   v.m[9]  = 0.0f;  v.m[13] = ty;
    v.m[2]  = 0.0f;  v.m[6]  = 0.0f;  v.m[10] = 1.0f;  v.m[14] = 0.0f;
    v.m[3]  = 0.0f;  v.m[7]  = 0.0f;  v.m[11] = 0.0f;  v.m[15] = 1.0f;
    return v;
}

// --- Integração com RendererDX11 ---

void tupi_camera_frame(int largura, int altura) {
    _win_w = largura;
    _win_h = altura;

    TupiMatriz proj = tupi_projecao_ortografica(largura, altura);
    TupiMatriz view = _view_matrix(&_cam);
    TupiMatriz mvp  = tupi_mat4_multiplicar(&proj, &view);

    tupi_dx11_atualizar_projecao(&mvp);
    tupi_sprite_set_projecao(mvp.m);
}

// --- Conversões de coordenadas ---

void tupi_camera_tela_para_mundo(const TupiCamera* cam,
                                  int largura, int altura,
                                  float sx, float sy,
                                  float* wx, float* wy) {
    float cx =  (sx - largura * 0.5f);
    float cy = -(sy - altura  * 0.5f);
    float inv_zoom = 1.0f / cam->zoom;
    float co = cosf(cam->rotacao);
    float si = sinf(cam->rotacao);
    *wx = cam->x + (cx * co - cy * si) * inv_zoom;
    *wy = cam->y + (cx * si + cy * co) * inv_zoom;
}

void tupi_camera_mundo_para_tela(const TupiCamera* cam,
                                  int largura, int altura,
                                  float wx, float wy,
                                  float* sx, float* sy) {
    float dx = wx - cam->x;
    float dy = wy - cam->y;
    float co = cosf(-cam->rotacao) * cam->zoom;
    float si = sinf(-cam->rotacao) * cam->zoom;
    float cx =  dx * co + dy * si;
    float cy = -dx * si + dy * co;
    *sx =  cx + largura * 0.5f;
    *sy = -cy + altura  * 0.5f;
}

// --- API pública ---

void tupi_camera_reset(void) {
    _cam.x = 0.0f; _cam.y = 0.0f;
    _cam.zoom = 1.0f; _cam.rotacao = 0.0f;
}

void tupi_camera_pos(float x, float y) {
    if (!tupi_camera_validar(x, y, _cam.zoom, _cam.rotacao)) {
        fprintf(stderr, "[Camera] pos invalida (%.2f, %.2f)\n", x, y);
        return;
    }
    _cam.x = x; _cam.y = y;
}

void tupi_camera_mover(float dx, float dy) {
    tupi_camera_pos(_cam.x + dx, _cam.y + dy);
}

void tupi_camera_zoom(float z) {
    if (!tupi_camera_validar(_cam.x, _cam.y, z, _cam.rotacao)) {
        fprintf(stderr, "[Camera] zoom invalido (%.4f)\n", z);
        return;
    }
    _cam.zoom = z;
}

void tupi_camera_rotacao(float angulo) {
    if (!tupi_camera_validar(_cam.x, _cam.y, _cam.zoom, angulo)) {
        fprintf(stderr, "[Camera] rotacao invalida (%.4f)\n", angulo);
        return;
    }
    _cam.rotacao = angulo;
}

void tupi_camera_seguir(float alvo_x, float alvo_y, float lerp_fator, float dt) {
    float fator = 1.0f - powf(1.0f - lerp_fator, dt * 60.0f);
    float nx = _cam.x + (alvo_x - _cam.x) * fator;
    float ny = _cam.y + (alvo_y - _cam.y) * fator;
    if (tupi_camera_validar(nx, ny, _cam.zoom, _cam.rotacao)) {
        _cam.x = nx; _cam.y = ny;
    }
}

float tupi_camera_get_x(void)       { return _cam.x;       }
float tupi_camera_get_y(void)       { return _cam.y;       }
float tupi_camera_get_zoom(void)    { return _cam.zoom;    }
float tupi_camera_get_rotacao(void) { return _cam.rotacao; }

// --- Wrappers FFI Lua ---

void tupi_camera_tela_mundo_lua(float sx, float sy, float* wx, float* wy) {
    tupi_camera_tela_para_mundo(&_cam, _win_w, _win_h, sx, sy, wx, wy);
}

void tupi_camera_mundo_tela_lua(float wx, float wy, float* sx, float* sy) {
    tupi_camera_mundo_para_tela(&_cam, _win_w, _win_h, wx, wy, sx, sy);
}