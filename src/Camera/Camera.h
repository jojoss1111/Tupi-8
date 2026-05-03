// Camera.h — TupiEngine (SDL2)

#ifndef CAMERA_H
#define CAMERA_H

#include <math.h>
#include <stdio.h>

typedef struct {
    float alvo_x, alvo_y;     // posição do alvo no mundo
    float ancora_x, ancora_y; // ponto na tela onde o alvo aparece; -1 = centro
    float zoom;                // 1.0 = normal, >1 = zoom in, <1 = zoom out
    float rotacao;             // ângulo em radianos

    float _cam_x, _cam_y;     // posição calculada (não editar)
    int   _ativo;              // 1 = câmera válida
} TupiCamera;

// Validação implementada em Rust
int tupi_camera_validar(float x, float y, float zoom, float rotacao);

// Cria câmera; ancora -1 = centraliza na tela
TupiCamera tupi_camera_criar(float alvo_x, float alvo_y,
                              float ancora_x, float ancora_y);
void tupi_camera_destruir(TupiCamera* cam);

// Define/retorna câmera ativa
void        tupi_camera_ativar(TupiCamera* cam);
TupiCamera* tupi_camera_ativa(void);

// Chamado pelo Renderer a cada frame
void tupi_camera_frame(TupiCamera* cam, int largura, int altura);

// Movimentação
void tupi_camera_pos(TupiCamera* cam, float x, float y);
void tupi_camera_mover(TupiCamera* cam, float dx, float dy);
void tupi_camera_zoom(TupiCamera* cam, float z);
void tupi_camera_rotacao(TupiCamera* cam, float angulo);
void tupi_camera_ancora(TupiCamera* cam, float ax, float ay);

// Segue alvo com lerp exponencial (independente de frame rate)
void tupi_camera_seguir(TupiCamera* cam,
                         float alvo_x, float alvo_y,
                         float lerp_fator, float dt);

// Getters
float tupi_camera_get_x(const TupiCamera* cam);
float tupi_camera_get_y(const TupiCamera* cam);
float tupi_camera_get_zoom(const TupiCamera* cam);
float tupi_camera_get_rotacao(const TupiCamera* cam);

// Conversão de coordenadas (Y cresce para baixo em ambos os espaços)
void tupi_camera_tela_para_mundo(const TupiCamera* cam, int largura, int altura,
                                  float sx, float sy, float* wx, float* wy);
void tupi_camera_mundo_para_tela(const TupiCamera* cam, int largura, int altura,
                                  float wx, float wy, float* sx, float* sy);

// Wrappers Lua usando a câmera ativa global
void tupi_camera_tela_mundo_lua(float sx, float sy, float* wx, float* wy);
void tupi_camera_mundo_tela_lua(float wx, float wy, float* sx, float* sy);

#endif // CAMERA_H