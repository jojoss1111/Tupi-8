// Spritesdx11.h
// TupiEngine - Sistema de Sprites, Atlas e Batching [backend: Direct3D 11]

#ifndef SPRITES_DX11_H
#define SPRITES_DX11_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// --- Estrutura de sprite (imagem na GPU) ---

typedef struct {
    void* _srv;    // ID3D11ShaderResourceView* — gerenciado internamente
    int   largura;
    int   altura;
} TupiSprite;

// --- Estrutura de objeto (instância na tela) ---

typedef struct {
    float       x, y;
    float       largura, altura;
    int         coluna, linha;
    float       transparencia;
    float       escala;
    TupiSprite* imagem;
} TupiObjeto;

// --- Atlas (ponteiro opaco para Rust) ---

typedef struct {
    void* _interno;
} TupiAtlas;

// --- Ciclo de vida do sistema ---

void tupi_sprite_iniciar(void);
void tupi_sprite_encerrar(void);
void tupi_sprite_set_projecao(const float* mat4);

// --- Sprite ---

TupiSprite* tupi_sprite_carregar(const char* caminho);
void        tupi_sprite_destruir(TupiSprite* sprite);

// --- Objeto (modo imediato) ---

TupiObjeto tupi_objeto_criar(float x, float y, float largura, float altura,
    int coluna, int linha, float transparencia, float escala, TupiSprite* imagem);

void tupi_objeto_desenhar(TupiObjeto* obj);

// --- Atlas ---

TupiAtlas* tupi_atlas_criar_c(void);
void       tupi_atlas_destruir_c(TupiAtlas* atlas);

void tupi_atlas_registrar_animacao(TupiAtlas* atlas, const char* nome,
    unsigned int linha, unsigned int colunas,
    float cel_larg, float cel_alt, float img_larg, float img_alt);

int tupi_atlas_obter_uv(TupiAtlas* atlas, const char* nome, unsigned int frame,
    float* u0, float* v0, float* u1, float* v1);

// --- Batch ---

void tupi_objeto_enviar_batch(TupiObjeto* obj, int z_index);
void tupi_objeto_enviar_batch_atlas(TupiObjeto* obj, TupiAtlas* atlas,
    const char* animacao, unsigned int frame, int z_index);
void tupi_objeto_enviar_batch_raw(unsigned int textura_id, const float* quad,
    float alpha, int z_index);
void tupi_batch_desenhar(void);

#endif // SPRITES_DX11_H