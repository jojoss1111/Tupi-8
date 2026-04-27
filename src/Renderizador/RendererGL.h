// RendererGL.h — TupiEngine (OpenGL 3.3 Core Profile)

#ifndef RENDERERGL_H
#define RENDERERGL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// --- Tipos básicos ---

typedef struct { float r, g, b, a; } Cor;
typedef struct { float x, y;       } Vec2;

// --- Rust / libtupi_seguro.a ---

typedef struct {
    unsigned char* ptr;
    size_t         tamanho;
} TupiAsset;

typedef enum {
    TUPI_RET = 0,
    TUPI_CIR = 1,
    TUPI_LIN = 2,
    TUPI_TRI = 3,
} TupiPrimitiva;

typedef struct {
    TupiPrimitiva primitiva;
    float         verts[8];
    int           n_verts;
    float         cor[4];
} TupiDrawCall;

typedef struct {
    float m[16];
} TupiMatriz;

TupiAsset  tupi_asset_carregar(const char* caminho);
void       tupi_asset_liberar(TupiAsset asset);

typedef void (*TupiFlushFn)(const TupiDrawCall* calls, int n);
void       tupi_batcher_registrar_flush(TupiFlushFn cb);
void       tupi_batcher_adicionar(TupiDrawCall dc);
void       tupi_batcher_flush(void);
int        tupi_batcher_tamanho(void);

TupiMatriz tupi_projecao_ortografica(int largura, int altura);
TupiMatriz tupi_mat4_multiplicar(const TupiMatriz* a, const TupiMatriz* b);

// --- Janela ---

int  tupi_janela_criar(int largura, int altura, const char* titulo, float escala, int sem_borda, const char* icone);

void  tupi_janela_set_titulo(const char* titulo);
void  tupi_janela_set_decoracao(int ativo);
void  tupi_janela_tela_cheia(int ativo);

void  tupi_janela_tela_cheia_letterbox(int ativo);

/* Retorna 1 se o modo letterbox está ativo, 0 caso contrário.               */
int   tupi_janela_letterbox_ativo(void);

int    tupi_janela_aberta(void);
void   tupi_janela_limpar(void);
void   tupi_janela_atualizar(void);
void   tupi_janela_fechar(void);
double tupi_tempo(void);
double tupi_delta_tempo(void);

int    tupi_janela_largura(void);
int    tupi_janela_altura(void);
int    tupi_janela_largura_px(void);
int    tupi_janela_altura_px(void);
float  tupi_janela_escala(void);

// --- Cor ---

void tupi_cor_fundo(float r, float g, float b);
void tupi_cor(float r, float g, float b, float a);

// --- Formas 2D ---

void tupi_retangulo(float x, float y, float largura, float altura);
void tupi_retangulo_borda(float x, float y, float largura, float altura, float espessura);
void tupi_triangulo(float x1, float y1, float x2, float y2, float x3, float y3);
void tupi_circulo(float x, float y, float raio, int segmentos);
void tupi_circulo_borda(float x, float y, float raio, int segmentos, float espessura);
void tupi_linha(float x1, float y1, float x2, float y2, float espessura);

#endif // RENDERERGL_H