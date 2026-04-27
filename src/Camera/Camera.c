// Camera/Camera.c
// TupiEngine — Sistema de Câmera 2D (objeto)

#include "Camera.h"
#include "../Renderizador/RendererGL.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

extern GLuint _tupi_shader_get(void);
extern GLint  _tupi_loc_projecao_get(void);
extern void   tupi_sprite_set_projecao(const float* mat4);

// ──────────────────────────────────────────────
//  Câmera ativa (usada pelo renderer e pelos wrappers Lua)
// ──────────────────────────────────────────────
static TupiCamera* _cam_ativa = NULL;
static int _win_w = 160;
static int _win_h = 144;


// ──────────────────────────────────────────────
//  Helpers internos
// ──────────────────────────────────────────────

/// Recalcula _cam_x/_cam_y a partir do alvo e da âncora.
/// A âncora define onde o alvo deve aparecer na tela; ao subtrair o offset
/// da âncora em relação ao centro obtemos a posição real da câmera.
/// floorf() garante posição em pixel inteiro, evitando texture bleeding nos tiles.
static void _recalcular(TupiCamera* cam, int largura, int altura) {
    float ax = (cam->ancora_x < 0) ? largura  * 0.5f : cam->ancora_x;
    float ay = (cam->ancora_y < 0) ? altura   * 0.5f : cam->ancora_y;

    float off_x = ax / cam->zoom;
    float off_y = ay / cam->zoom;

    // ✅ FIX: arredonda para pixel inteiro — elimina sub-pixel jitter
    // que causava partículas/pixels brancos nos tiles ao mover a câmera.
    // roundf (não floorf) evita o viés direcional que cortava tiles na
    // borda direita/inferior quando o alvo estava no limite do clamp.
    cam->_cam_x = roundf(cam->alvo_x - off_x);
    cam->_cam_y = roundf(cam->alvo_y - off_y);
}

void tupi_camera_ativar(TupiCamera* cam) {
    if (!cam || !cam->_ativo) return;
    _cam_ativa = cam;
    _win_w = tupi_janela_largura();
    _win_h = tupi_janela_altura();
    _recalcular(cam, _win_w, _win_h);
}
TupiCamera* tupi_camera_ativa(void)      { return _cam_ativa; }


/// View matrix 4×4 column-major: translate → rotate → scale
///
/// Pixel-snapping na translação: arredonda _cam_x/_cam_y para pixel inteiro
/// ANTES de calcular tx/ty. Isso garante que a translação resultante na MVP
/// cai exatamente num pixel lógico, eliminando o bleeding de sub-pixel nos
/// tiles do tilemap mesmo quando o alvo tem posição fracionária.
/// Sem isso, multiplicações de ponto flutuante na MVP introduzem offsets
/// fracionários que fazem o sampler GL_NEAREST amostrar o tile vizinho.
static TupiMatriz _view_matrix(const TupiCamera* cam) {
    float co = cosf(-cam->rotacao) * cam->zoom;
    float si = sinf(-cam->rotacao) * cam->zoom;

    /* Snap para pixel inteiro antes de montar a translação.
       _cam_x/_cam_y já passaram por roundf() em _recalcular(),
       mas aqui garantimos mesmo com zoom != 1 ou rotação != 0. */
    float cx = roundf(cam->_cam_x);
    float cy = roundf(cam->_cam_y);

    float tx = -(cx * co + cy * si);
    float ty = (cx * si - cy * co);

    TupiMatriz v = {{0}};
    v.m[0]  =  co;   v.m[4]  =  si;   v.m[8]  = 0.0f;  v.m[12] = tx;
    v.m[1]  = -si;   v.m[5]  =  co;   v.m[9]  = 0.0f;  v.m[13] = ty;
    v.m[2]  = 0.0f;  v.m[6]  = 0.0f;  v.m[10] = 1.0f;  v.m[14] = 0.0f;
    v.m[3]  = 0.0f;  v.m[7]  = 0.0f;  v.m[11] = 0.0f;  v.m[15] = 1.0f;
    return v;
}
// ──────────────────────────────────────────────
//  Ciclo de vida
// ──────────────────────────────────────────────

TupiCamera tupi_camera_criar(float alvo_x, float alvo_y,
                              float ancora_x, float ancora_y) {
    TupiCamera cam = {0};
    cam.alvo_x   = alvo_x;
    cam.alvo_y   = alvo_y;
    cam.ancora_x = ancora_x; // -1 = usar centro da janela
    cam.ancora_y = ancora_y;
    cam.zoom     = 1.0f;
    cam.rotacao  = 0.0f;
    cam._ativo   = 1;
    // _cam_x/_cam_y são calculados no primeiro frame (tamanho da janela desconhecido aqui)
    cam._cam_x   = floorf(alvo_x);
    cam._cam_y   = floorf(alvo_y);
    return cam;
}

void tupi_camera_destruir(TupiCamera* cam) {
    if (!cam) return;
    if (_cam_ativa == cam) _cam_ativa = NULL;
    *cam = (TupiCamera){0};
}

// ──────────────────────────────────────────────
//  Frame — chamado pelo RendererGL
// ──────────────────────────────────────────────

void tupi_camera_frame(TupiCamera* cam, int largura, int altura) {
    if (!cam || !cam->_ativo) return;
    _win_w = largura;
    _win_h = altura;

    _recalcular(cam, largura, altura);

    TupiMatriz proj = tupi_projecao_ortografica(largura, altura);
    TupiMatriz view = _view_matrix(cam);
    TupiMatriz mvp  = tupi_mat4_multiplicar(&proj, &view);

    glUseProgram(_tupi_shader_get());
    glUniformMatrix4fv(_tupi_loc_projecao_get(), 1, GL_FALSE, mvp.m);
    tupi_sprite_set_projecao(mvp.m);
}

// ──────────────────────────────────────────────
//  Movimentação
// ──────────────────────────────────────────────

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

// ──────────────────────────────────────────────
//  Getters
// ──────────────────────────────────────────────

float tupi_camera_get_x(const TupiCamera* cam)       { return cam ? cam->_cam_x   : 0.0f; }
float tupi_camera_get_y(const TupiCamera* cam)       { return cam ? cam->_cam_y   : 0.0f; }
float tupi_camera_get_zoom(const TupiCamera* cam)    { return cam ? cam->zoom     : 1.0f; }
float tupi_camera_get_rotacao(const TupiCamera* cam) { return cam ? cam->rotacao  : 0.0f; }

// ──────────────────────────────────────────────
//  Conversão de coordenadas
// ──────────────────────────────────────────────

void tupi_camera_tela_para_mundo(const TupiCamera* cam, int largura, int altura,
                                  float sx, float sy, float* wx, float* wy) {
    float cx = sx - largura  * 0.5f;
    float cy = (sy - altura  * 0.5f) * -1.0f;
    float inv_zoom = 1.0f / cam->zoom;
    float co = cosf(cam->rotacao);
    float si = sinf(cam->rotacao);
    *wx = cam->_cam_x + (cx * co - cy * si) * inv_zoom;
    *wy = cam->_cam_y + (cx * si + cy * co) * inv_zoom;
}

void tupi_camera_mundo_para_tela(const TupiCamera* cam, int largura, int altura,
                                  float wx, float wy, float* sx, float* sy) {
    float dx = wx - cam->_cam_x;
    float dy = wy - cam->_cam_y;
    float co = cosf(-cam->rotacao) * cam->zoom;
    float si = sinf(-cam->rotacao) * cam->zoom;
    *sx =  (dx * co + dy * si) + largura  * 0.5f;
    *sy = -(dx * si - dy * co) + altura   * 0.5f;
}

// ──────────────────────────────────────────────
//  Wrappers FFI Lua (usam a câmera ativa)
// ──────────────────────────────────────────────

void tupi_camera_tela_mundo_lua(float sx, float sy, float* wx, float* wy) {
    if (!_cam_ativa) { *wx = sx; *wy = sy; return; }
    tupi_camera_tela_para_mundo(_cam_ativa, _win_w, _win_h, sx, sy, wx, wy);
}

void tupi_camera_mundo_tela_lua(float wx, float wy, float* sx, float* sy) {
    if (!_cam_ativa) { *sx = wx; *sy = wy; return; }
    tupi_camera_mundo_para_tela(_cam_ativa, _win_w, _win_h, wx, wy, sx, sy);
}