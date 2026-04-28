// Sprites.c — TupiEngine (SDL2 + Vulkan)

#include "Sprites.h"
#include "../Renderizador/Renderer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- FFI Rust: imagem ---

typedef struct {
    unsigned char* pixels;
    int largura, altura, tamanho;
} TupiImagemRust;

extern TupiImagemRust* tupi_imagem_carregar_seguro(const char* caminho);
extern void            tupi_imagem_destruir(TupiImagemRust* img);

// --- FFI Rust: atlas ---

typedef struct TupiAtlasOpaco TupiAtlasOpaco;
typedef struct { float u0, v0, u1, v1; } TupiUV;

extern TupiAtlasOpaco* tupi_atlas_criar(void);
extern void            tupi_atlas_destruir(TupiAtlasOpaco* atlas);
extern void            tupi_atlas_registrar(TupiAtlasOpaco*, const char*, unsigned int, unsigned int, float, float, float, float);
extern int             tupi_atlas_uv(const TupiAtlasOpaco*, const char*, unsigned int, TupiUV*);

// --- FFI Rust: batch ---

typedef struct TupiBatchOpaco TupiBatchOpaco;

typedef struct {
    unsigned int textura_id;
    float        quad[16];
    float        cor[4];
    int          z_index;
} TupiItemBatch;

extern TupiBatchOpaco*      tupi_batch_criar(void);
extern void                 tupi_batch_destruir(TupiBatchOpaco*);
extern void                 tupi_batch_adicionar(TupiBatchOpaco*, unsigned int, const float* quad, const float* cor, int z_index);
extern const TupiItemBatch* tupi_batch_flush(TupiBatchOpaco*, int*);
extern void                 tupi_batch_limpar(TupiBatchOpaco*);

// --- Estado do módulo ---

static TupiBatchOpaco* _batch = NULL;

static float _projecao[16] = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1,
};
static int   _view_w = 160;
static int   _view_h = 144;
static float _cor_batch[4] = {1.0f, 1.0f, 1.0f, 1.0f};

// --- Helpers ---

// Transforma coordenada mundo → pixel lógico usando a matriz de projeção
static void _transformar_ponto(float x, float y, float* sx, float* sy) {
    float w = _projecao[3]*x + _projecao[7]*y + _projecao[15];
    if (fabsf(w) < 0.00001f) w = 1.0f;

    float clip_x = _projecao[0]*x + _projecao[4]*y + _projecao[12];
    float clip_y = _projecao[1]*x + _projecao[5]*y + _projecao[13];

    float ndc_x = clip_x / w;
    float ndc_y = clip_y / w;

    *sx = (ndc_x * 0.5f + 0.5f) * (float)_view_w;
    *sy = (1.0f - (ndc_y * 0.5f + 0.5f)) * (float)_view_h;
}

static void _calcular_uv_objeto(TupiObjeto* obj, TupiSprite* spr,
    float* u0, float* v0, float* u1, float* v1)
{
    float fw = (float)spr->largura;
    float fh = (float)spr->altura;
    *u0 = (obj->coluna * obj->largura) / fw;
    *v0 = (obj->linha  * obj->altura)  / fh;
    *u1 = *u0 + obj->largura / fw;
    *v1 = *v0 + obj->altura  / fh;
}

static void _renderizar_quad(unsigned int textura_id, const float* quad, const float* cor) {
    if (!textura_id || !quad || !cor) return;

    float transformado[16];
    for (int i = 0; i < 4; i++) {
        float sx = 0.0f;
        float sy = 0.0f;
        _transformar_ponto(quad[i*4], quad[i*4+1], &sx, &sy);
        transformado[i*4 + 0] = sx;
        transformado[i*4 + 1] = sy;
        transformado[i*4 + 2] = quad[i*4 + 2];
        transformado[i*4 + 3] = quad[i*4 + 3];
    }

    tupi_renderer_desenhar_quad(textura_id, transformado, cor);
}

// --- Ciclo de vida ---

void tupi_sprite_iniciar(void) {
    if (!_batch) _batch = tupi_batch_criar();
}

void tupi_sprite_encerrar(void) {
    if (_batch) {
        tupi_batch_destruir(_batch);
        _batch = NULL;
    }
}

void tupi_sprite_set_projecao(const float* mat4) {
    if (!mat4) return;
    for (int i = 0; i < 16; i++) _projecao[i] = mat4[i];
}

void tupi_sprite_set_viewport(int largura, int altura) {
    if (largura > 0) _view_w = largura;
    if (altura  > 0) _view_h = altura;
}

void tupi_sprite_set_cor(float r, float g, float b, float a) {
    _cor_batch[0] = r; _cor_batch[1] = g;
    _cor_batch[2] = b; _cor_batch[3] = a;
}

void tupi_sprite_reset_cor(void) {
    _cor_batch[0] = _cor_batch[1] = _cor_batch[2] = _cor_batch[3] = 1.0f;
}

// --- Sprite ---

TupiSprite* tupi_sprite_carregar(const char* caminho) {
    TupiImagemRust* img = tupi_imagem_carregar_seguro(caminho);
    if (!img) {
        fprintf(stderr, "[Sprites] Falha ao carregar '%s'\n", caminho);
        return NULL;
    }

    unsigned int handle = tupi_renderer_textura_criar_rgba8(img->pixels, img->largura, img->altura);
    if (handle == 0) {
        fprintf(stderr, "[Sprites] Falha ao criar textura Vulkan para '%s'\n", caminho);
        tupi_imagem_destruir(img);
        return NULL;
    }

    TupiSprite* sprite = (TupiSprite*)malloc(sizeof(TupiSprite));
    if (!sprite) {
        tupi_renderer_textura_destruir(handle);
        tupi_imagem_destruir(img);
        return NULL;
    }

    sprite->textura = handle;
    sprite->largura = img->largura;
    sprite->altura  = img->altura;
    tupi_imagem_destruir(img);
    return sprite;
}

void tupi_sprite_destruir(TupiSprite* sprite) {
    if (!sprite) return;
    tupi_renderer_textura_destruir(sprite->textura);
    free(sprite);
}

// --- Atlas ---

TupiAtlas* tupi_atlas_criar_c(void) {
    TupiAtlas* a = (TupiAtlas*)malloc(sizeof(TupiAtlas));
    if (!a) return NULL;
    a->_interno = tupi_atlas_criar();
    return a;
}

void tupi_atlas_destruir_c(TupiAtlas* atlas) {
    if (!atlas) return;
    tupi_atlas_destruir(atlas->_interno);
    free(atlas);
}

void tupi_atlas_registrar_animacao(
    TupiAtlas* atlas, const char* nome,
    unsigned int linha, unsigned int colunas,
    float cel_larg, float cel_alt,
    float img_larg, float img_alt)
{
    if (!atlas || !atlas->_interno) return;
    tupi_atlas_registrar(atlas->_interno, nome, linha, colunas, cel_larg, cel_alt, img_larg, img_alt);
}

int tupi_atlas_obter_uv(
    TupiAtlas* atlas, const char* nome, unsigned int frame,
    float* u0, float* v0, float* u1, float* v1)
{
    if (!atlas || !atlas->_interno) return 0;
    TupiUV uv = {0};
    int ok = tupi_atlas_uv(atlas->_interno, nome, frame, &uv);
    if (ok) { *u0 = uv.u0; *v0 = uv.v0; *u1 = uv.u1; *v1 = uv.v1; }
    return ok;
}

// --- Objeto ---

TupiObjeto tupi_objeto_criar(
    float x, float y, float largura, float altura,
    int coluna, int linha, float transparencia, float escala, TupiSprite* imagem)
{
    TupiObjeto obj = {x, y, largura, altura, coluna, linha, transparencia, escala, imagem};
    return obj;
}

void tupi_objeto_desenhar(TupiObjeto* obj) {
    if (!obj || !obj->imagem) return;

    float u0, v0, u1, v1;
    _calcular_uv_objeto(obj, obj->imagem, &u0, &v0, &u1, &v1);

    float sw = obj->largura * obj->escala;
    float sh = obj->altura  * obj->escala;
    float x0 = roundf(obj->x), y0 = roundf(obj->y);

    float quad[16] = {
        x0,    y0,    u0, v0,
        x0+sw, y0,    u1, v0,
        x0,    y0+sh, u0, v1,
        x0+sw, y0+sh, u1, v1,
    };
    float cor[4] = {_cor_batch[0], _cor_batch[1], _cor_batch[2], _cor_batch[3] * obj->transparencia};
    _renderizar_quad(obj->imagem->textura, quad, cor);
}

// --- Batch ---

void tupi_objeto_enviar_batch(TupiObjeto* obj, int z_index) {
    if (!obj || !obj->imagem || !_batch) return;

    float u0, v0, u1, v1;
    _calcular_uv_objeto(obj, obj->imagem, &u0, &v0, &u1, &v1);

    float sw = obj->largura * obj->escala;
    float sh = obj->altura  * obj->escala;
    float x0 = roundf(obj->x), y0 = roundf(obj->y);

    float quad[16] = {
        x0,    y0,    u0, v0,
        x0+sw, y0,    u1, v0,
        x0,    y0+sh, u0, v1,
        x0+sw, y0+sh, u1, v1,
    };
    float cor[4] = {_cor_batch[0], _cor_batch[1], _cor_batch[2], _cor_batch[3] * obj->transparencia};
    tupi_batch_adicionar(_batch, obj->imagem->textura, quad, cor, z_index);
}

void tupi_objeto_enviar_batch_raw(
    unsigned int textura_id,
    const float* quad,
    float r, float g, float b, float a,
    int z_index)
{
    if (!_batch || !quad) return;
    float cor[4] = {r, g, b, a};
    tupi_batch_adicionar(_batch, textura_id, quad, cor, z_index);
}

void tupi_objeto_enviar_batch_atlas(
    TupiObjeto* obj, TupiAtlas* atlas,
    const char* animacao, unsigned int frame, int z_index)
{
    if (!obj || !obj->imagem || !atlas || !_batch) return;

    TupiUV uv = {0};
    if (!tupi_atlas_uv(atlas->_interno, animacao, frame, &uv)) {
        fprintf(stderr, "[Sprites] Atlas: animacao '%s' nao encontrada\n", animacao);
        return;
    }

    float sw = obj->largura * obj->escala;
    float sh = obj->altura  * obj->escala;
    float x0 = roundf(obj->x), y0 = roundf(obj->y);

    float quad[16] = {
        x0,    y0,    uv.u0, uv.v0,
        x0+sw, y0,    uv.u1, uv.v0,
        x0,    y0+sh, uv.u0, uv.v1,
        x0+sw, y0+sh, uv.u1, uv.v1,
    };
    float cor[4] = {_cor_batch[0], _cor_batch[1], _cor_batch[2], _cor_batch[3] * obj->transparencia};
    tupi_batch_adicionar(_batch, obj->imagem->textura, quad, cor, z_index);
}

void tupi_batch_desenhar(void) {
    if (!_batch) return;
    int contagem = 0;
    const TupiItemBatch* itens = tupi_batch_flush(_batch, &contagem);
    if (contagem <= 0 || !itens) {
        tupi_batch_limpar(_batch);
        return;
    }

    for (int i = 0; i < contagem; i++) {
        _renderizar_quad(itens[i].textura_id, itens[i].quad, itens[i].cor);
    }
    tupi_batch_limpar(_batch);
}
