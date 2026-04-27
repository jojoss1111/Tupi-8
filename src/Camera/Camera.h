// Camera/Camera.h
// TupiEngine — Sistema de Câmera 2D (objeto)

#ifndef CAMERA_H
#define CAMERA_H

#include <math.h>
#include <stdio.h>

// ──────────────────────────────────────────────
//  Estrutura pública da câmera
// ──────────────────────────────────────────────

typedef struct {
    // Posição do alvo no mundo (ex.: x/y do player)
    float alvo_x;
    float alvo_y;

    // Ponto de ancoragem na tela em pixels
    // (onde o alvo aparece; padrão = centro da janela)
    float ancora_x;
    float ancora_y;

    float zoom;     // 1.0 = normal, >1 = aproximado, <1 = afastado
    float rotacao;  // ângulo em radianos

    // Estado interno — não editar diretamente
    float _cam_x;   // posição real da câmera calculada a partir do alvo+âncora
    float _cam_y;
    int   _ativo;   // 1 = câmera válida e inicializada
} TupiCamera;

// ──────────────────────────────────────────────
//  Validação (implementada em camera.rs)
// ──────────────────────────────────────────────
int tupi_camera_validar(float x, float y, float zoom, float rotacao);

// ──────────────────────────────────────────────
//  Ciclo de vida
// ──────────────────────────────────────────────

/// Cria e inicializa uma câmera.
/// alvo_x/alvo_y  — posição inicial do alvo no mundo (ex.: player)
/// ancora_x/ancora_y — ponto na tela onde o alvo ficará (pixels)
///                     Passe -1 para usar o centro da janela.
TupiCamera tupi_camera_criar(float alvo_x, float alvo_y,
                              float ancora_x, float ancora_y);

/// Destrói / invalida a câmera (zera campos, _ativo = 0).
void tupi_camera_destruir(TupiCamera* cam);

// ──────────────────────────────────────────────
//  Chamado pelo RendererGL a cada frame
// ──────────────────────────────────────────────
void tupi_camera_frame(TupiCamera* cam, int largura, int altura);

// ──────────────────────────────────────────────
//  Movimentação e ajustes
// ──────────────────────────────────────────────

/// Move o alvo para (x, y) — câmera reposiciona instantaneamente.
void tupi_camera_pos(TupiCamera* cam, float x, float y);

/// Desloca o alvo por (dx, dy).
void tupi_camera_mover(TupiCamera* cam, float dx, float dy);

/// Define o zoom da câmera.
void tupi_camera_zoom(TupiCamera* cam, float z);

/// Define a rotação da câmera (radianos).
void tupi_camera_rotacao(TupiCamera* cam, float angulo);

/// Define o ponto de ancoragem na tela (pixels).
/// -1 nos dois eixos reaplica o centro da janela atual.
void tupi_camera_ancora(TupiCamera* cam, float ax, float ay);

/// Segue suavemente o alvo (lerp exponencial, frame-rate independente).
void tupi_camera_seguir(TupiCamera* cam,
                         float alvo_x, float alvo_y,
                         float lerp_fator, float dt);

// ──────────────────────────────────────────────
//  Getters
// ──────────────────────────────────────────────
float tupi_camera_get_x(const TupiCamera* cam);
float tupi_camera_get_y(const TupiCamera* cam);
float tupi_camera_get_zoom(const TupiCamera* cam);
float tupi_camera_get_rotacao(const TupiCamera* cam);

// ──────────────────────────────────────────────
//  Conversão de coordenadas
// ──────────────────────────────────────────────
void tupi_camera_tela_para_mundo(const TupiCamera* cam, int largura, int altura,
                                  float sx, float sy, float* wx, float* wy);

void tupi_camera_mundo_para_tela(const TupiCamera* cam, int largura, int altura,
                                  float wx, float wy, float* sx, float* sy);

// ──────────────────────────────────────────────
//  Wrappers FFI Lua (câmera "ativa" global)
//  A câmera ativa é definida por tupi_camera_ativar().
// ──────────────────────────────────────────────

/// Registra uma câmera como a câmera ativa do renderer.
void tupi_camera_ativar(TupiCamera* cam);

/// Retorna o ponteiro da câmera ativa (pode ser NULL).
TupiCamera* tupi_camera_ativa(void);

// Wrappers de conversão usando a câmera ativa
void tupi_camera_tela_mundo_lua(float sx, float sy, float* wx, float* wy);
void tupi_camera_mundo_tela_lua(float wx, float wy, float* sx, float* sy);

#endif // CAMERA_H