// Camera.h — TupiEngine (SDL2)

#ifndef CAMERA_H
#define CAMERA_H

#include <math.h>
#include <stdio.h>

// --- Estrutura da câmera ---

typedef struct {
    float alvo_x, alvo_y;      // posição do alvo no mundo (ex.: player)
    float ancora_x, ancora_y;  // ponto na tela onde o alvo aparece (px); -1 = centro
    float zoom;                 // 1.0 = normal, >1 = zoom in, <1 = zoom out
    float rotacao;              // ângulo em radianos

    // Interno — não editar diretamente
    float _cam_x, _cam_y;      // posição real calculada (alvo - âncora/zoom)
    int   _ativo;               // 1 = câmera válida
} TupiCamera;

// --- Validação (implementada no Rust) ---
int tupi_camera_validar(float x, float y, float zoom, float rotacao);

// --- Ciclo de vida ---

// alvo = posição inicial no mundo; ancora = pixel na tela (-1 = centro)
TupiCamera tupi_camera_criar(float alvo_x, float alvo_y,
                              float ancora_x, float ancora_y);
void tupi_camera_destruir(TupiCamera* cam);

// --- Ativação ---

void        tupi_camera_ativar(TupiCamera* cam);  // define como câmera ativa
TupiCamera* tupi_camera_ativa(void);              // retorna câmera ativa (ou NULL)

// --- Frame (chamado pelo Renderer) ---
void tupi_camera_frame(TupiCamera* cam, int largura, int altura);

// --- Movimentação ---

void tupi_camera_pos(TupiCamera* cam, float x, float y);
void tupi_camera_mover(TupiCamera* cam, float dx, float dy);
void tupi_camera_zoom(TupiCamera* cam, float z);
void tupi_camera_rotacao(TupiCamera* cam, float angulo);
void tupi_camera_ancora(TupiCamera* cam, float ax, float ay);

// Segue o alvo suavemente (lerp exponencial, frame-rate independente)
void tupi_camera_seguir(TupiCamera* cam,
                         float alvo_x, float alvo_y,
                         float lerp_fator, float dt);

// --- Getters ---

float tupi_camera_get_x(const TupiCamera* cam);
float tupi_camera_get_y(const TupiCamera* cam);
float tupi_camera_get_zoom(const TupiCamera* cam);
float tupi_camera_get_rotacao(const TupiCamera* cam);

// --- Conversão de coordenadas (SDL2: Y↓ em ambos os espaços) ---

void tupi_camera_tela_para_mundo(const TupiCamera* cam, int largura, int altura,
                                  float sx, float sy, float* wx, float* wy);

void tupi_camera_mundo_para_tela(const TupiCamera* cam, int largura, int altura,
                                  float wx, float wy, float* sx, float* sy);

// --- Wrappers Lua (câmera ativa global) ---

void tupi_camera_tela_mundo_lua(float sx, float sy, float* wx, float* wy);
void tupi_camera_mundo_tela_lua(float wx, float wy, float* sx, float* sy);

#endif // CAMERA_H