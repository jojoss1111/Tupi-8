// Spritesdx11.c
// TupiEngine - Sistema de Sprites e Sprite Sheets [backend: Direct3D 11]

#include "Spritesdx11.h"
#include "../Renderizador/RendererDX11.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- FFI Rust ---

typedef struct {
    unsigned char* pixels;
    int largura, altura, tamanho;
} TupiImagemRust;

extern TupiImagemRust* tupi_imagem_carregar_seguro(const char* caminho);
extern void            tupi_imagem_destruir(TupiImagemRust* img);

typedef struct TupiAtlasOpaco TupiAtlasOpaco;
typedef struct { float u0, v0, u1, v1; } TupiUV;

extern TupiAtlasOpaco* tupi_atlas_criar(void);
extern void            tupi_atlas_destruir(TupiAtlasOpaco* atlas);
extern void            tupi_atlas_registrar(TupiAtlasOpaco*, const char*,
                           unsigned int, unsigned int, float, float, float, float);
extern int             tupi_atlas_uv(const TupiAtlasOpaco*, const char*,
                           unsigned int, TupiUV*);

typedef struct TupiBatchOpaco TupiBatchOpaco;
typedef struct {
    unsigned int textura_id;
    float        quad[16];
    float        alpha;
    int          z_index;
} TupiItemBatch;

extern TupiBatchOpaco*      tupi_batch_criar(void);
extern void                 tupi_batch_destruir(TupiBatchOpaco*);
extern void                 tupi_batch_adicionar(TupiBatchOpaco*, unsigned int,
                                const float*, float, int);
extern const TupiItemBatch* tupi_batch_flush(TupiBatchOpaco*, int*);
extern void                 tupi_batch_limpar(TupiBatchOpaco*);
extern int                  tupi_batch_contagem(const TupiBatchOpaco*);

// --- Shaders HLSL ---

static const char* _vert_sprite_hlsl =
    "cbuffer CBFrame : register(b0) {\n"
    "    float4x4 uProjecao;\n"
    "};\n"
    "struct VSIn  { float2 pos : POSITION; float2 uv : TEXCOORD0; };\n"
    "struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
    "VSOut VS(VSIn v) {\n"
    "    VSOut o;\n"
    "    o.pos = mul(uProjecao, float4(v.pos, 0.0f, 1.0f));\n"
    "    o.uv  = v.uv;\n"
    "    return o;\n"
    "}\n";

static const char* _frag_sprite_hlsl =
    "cbuffer CBAlpha : register(b1) {\n"
    "    float uAlpha;\n"
    "    float3 _pad;\n"
    "};\n"
    "Texture2D    uTextura : register(t0);\n"
    "SamplerState uSampler : register(s0);\n"
    "struct PSIn  { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
    "float4 PS(PSIn p) : SV_Target {\n"
    "    float4 cor = uTextura.Sample(uSampler, p.uv);\n"
    "    return float4(cor.rgb, cor.a * uAlpha);\n"
    "}\n";

// --- Estado interno ---

extern ID3D11Device*        _device;
extern ID3D11DeviceContext* _ctx;

static ID3D11VertexShader*        _vs_spr   = NULL;
static ID3D11PixelShader*         _ps_spr   = NULL;
static ID3D11InputLayout*         _layout   = NULL;
static ID3D11Buffer*              _vbo_spr  = NULL;
static ID3D11Buffer*              _cb_proj  = NULL;
static ID3D11Buffer*              _cb_alpha = NULL;
static ID3D11SamplerState*        _sampler  = NULL;
static ID3D11BlendState*          _blend    = NULL;

static float          _projecao[16] = {0};
static TupiBatchOpaco* _batch       = NULL;

#define SPRITE_STRIDE   (sizeof(float) * 4)
#define SPRITE_VBO_SIZE (SPRITE_STRIDE * 4 * 512)

// --- Helpers ---

static ID3DBlob* _compilar_hlsl(const char* src, const char* entry,
                                 const char* perfil) {
    ID3DBlob* blob  = NULL;
    ID3DBlob* erros = NULL;
    HRESULT hr = D3DCompile(src, strlen(src), NULL, NULL, NULL,
                             entry, perfil, D3DCOMPILE_ENABLE_STRICTNESS, 0,
                             &blob, &erros);
    if (FAILED(hr)) {
        if (erros) {
            fprintf(stderr, "[TupiSprite] Erro HLSL (%s): %s\n",
                    entry, (const char*)erros->lpVtbl->GetBufferPointer(erros));
            erros->lpVtbl->Release(erros);
        }
        return NULL;
    }
    if (erros) erros->lpVtbl->Release(erros);
    return blob;
}

static ID3D11Buffer* _criar_cb(UINT bytes) {
    D3D11_BUFFER_DESC d = {
        .ByteWidth      = bytes,
        .Usage          = D3D11_USAGE_DYNAMIC,
        .BindFlags      = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };
    ID3D11Buffer* b = NULL;
    _device->lpVtbl->CreateBuffer(_device, &d, NULL, &b);
    return b;
}

static void _dx11_draw_quad(const float* quad, ID3D11ShaderResourceView* srv, float alpha) {
    if (_cb_alpha) {
        float dados[4] = { alpha, 0, 0, 0 };
        D3D11_MAPPED_SUBRESOURCE ms;
        if (SUCCEEDED(_ctx->lpVtbl->Map(_ctx, (ID3D11Resource*)_cb_alpha,
                                         0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
            memcpy(ms.pData, dados, sizeof(dados));
            _ctx->lpVtbl->Unmap(_ctx, (ID3D11Resource*)_cb_alpha, 0);
        }
    }
    D3D11_MAPPED_SUBRESOURCE ms;
    if (SUCCEEDED(_ctx->lpVtbl->Map(_ctx, (ID3D11Resource*)_vbo_spr,
                                     0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
        memcpy(ms.pData, quad, sizeof(float) * 16);
        _ctx->lpVtbl->Unmap(_ctx, (ID3D11Resource*)_vbo_spr, 0);
    }
    _ctx->lpVtbl->PSSetShaderResources(_ctx, 0, 1, &srv);
    UINT stride = (UINT)SPRITE_STRIDE, offset = 0;
    _ctx->lpVtbl->IASetVertexBuffers(_ctx, 0, 1, &_vbo_spr, &stride, &offset);
    _ctx->lpVtbl->IASetPrimitiveTopology(_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    _ctx->lpVtbl->Draw(_ctx, 4, 0);
}

static void _bind_pipeline(void) {
    _ctx->lpVtbl->IASetInputLayout(_ctx, _layout);
    _ctx->lpVtbl->VSSetShader(_ctx, _vs_spr, NULL, 0);
    _ctx->lpVtbl->PSSetShader(_ctx, _ps_spr, NULL, 0);
    _ctx->lpVtbl->VSSetConstantBuffers(_ctx, 0, 1, &_cb_proj);
    _ctx->lpVtbl->PSSetConstantBuffers(_ctx, 1, 1, &_cb_alpha);
    _ctx->lpVtbl->PSSetSamplers(_ctx, 0, 1, &_sampler);
    float bf[4] = {0};
    _ctx->lpVtbl->OMSetBlendState(_ctx, _blend, bf, 0xFFFFFFFF);
}

// --- Inicialização / encerramento ---

void tupi_sprite_iniciar(void) {
    ID3DBlob* vs_blob = _compilar_hlsl(_vert_sprite_hlsl, "VS", "vs_4_0");
    ID3DBlob* ps_blob = _compilar_hlsl(_frag_sprite_hlsl, "PS", "ps_4_0");
    if (!vs_blob || !ps_blob) {
        fprintf(stderr, "[TupiSprite] Falha ao compilar shaders de sprite!\n");
        return;
    }

    _device->lpVtbl->CreateVertexShader(_device,
        vs_blob->lpVtbl->GetBufferPointer(vs_blob),
        vs_blob->lpVtbl->GetBufferSize(vs_blob), NULL, &_vs_spr);
    _device->lpVtbl->CreatePixelShader(_device,
        ps_blob->lpVtbl->GetBufferPointer(ps_blob),
        ps_blob->lpVtbl->GetBufferSize(ps_blob), NULL, &_ps_spr);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,                D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float)*2,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    _device->lpVtbl->CreateInputLayout(_device,
        layout, 2,
        vs_blob->lpVtbl->GetBufferPointer(vs_blob),
        vs_blob->lpVtbl->GetBufferSize(vs_blob), &_layout);

    vs_blob->lpVtbl->Release(vs_blob);
    ps_blob->lpVtbl->Release(ps_blob);

    D3D11_BUFFER_DESC vbd = {
        .ByteWidth      = SPRITE_VBO_SIZE,
        .Usage          = D3D11_USAGE_DYNAMIC,
        .BindFlags      = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };
    _device->lpVtbl->CreateBuffer(_device, &vbd, NULL, &_vbo_spr);

    _cb_proj  = _criar_cb(64);
    _cb_alpha = _criar_cb(16);

    D3D11_SAMPLER_DESC sd = {
        .Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT,
        .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
    };
    _device->lpVtbl->CreateSamplerState(_device, &sd, &_sampler);

    D3D11_BLEND_DESC bd = {0};
    bd.RenderTarget[0] = (D3D11_RENDER_TARGET_BLEND_DESC){
        .BlendEnable           = TRUE,
        .SrcBlend              = D3D11_BLEND_SRC_ALPHA,
        .DestBlend             = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOp               = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha         = D3D11_BLEND_ONE,
        .DestBlendAlpha        = D3D11_BLEND_ZERO,
        .BlendOpAlpha          = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
    };
    _device->lpVtbl->CreateBlendState(_device, &bd, &_blend);

    _batch = tupi_batch_criar();

    printf("[TupiSprite] Sistema DX11 iniciado (imagem Rust + atlas + batch)\n");
}

void tupi_sprite_encerrar(void) {
    if (_batch)    { tupi_batch_destruir(_batch);           _batch    = NULL; }
    if (_blend)    { _blend->lpVtbl->Release(_blend);       _blend    = NULL; }
    if (_sampler)  { _sampler->lpVtbl->Release(_sampler);   _sampler  = NULL; }
    if (_cb_alpha) { _cb_alpha->lpVtbl->Release(_cb_alpha); _cb_alpha = NULL; }
    if (_cb_proj)  { _cb_proj->lpVtbl->Release(_cb_proj);   _cb_proj  = NULL; }
    if (_vbo_spr)  { _vbo_spr->lpVtbl->Release(_vbo_spr);   _vbo_spr  = NULL; }
    if (_layout)   { _layout->lpVtbl->Release(_layout);     _layout   = NULL; }
    if (_ps_spr)   { _ps_spr->lpVtbl->Release(_ps_spr);     _ps_spr   = NULL; }
    if (_vs_spr)   { _vs_spr->lpVtbl->Release(_vs_spr);     _vs_spr   = NULL; }
}

void tupi_sprite_set_projecao(const float* mat4) {
    for (int i = 0; i < 16; i++) _projecao[i] = mat4[i];
    if (_cb_proj && _ctx) {
        D3D11_MAPPED_SUBRESOURCE ms;
        if (SUCCEEDED(_ctx->lpVtbl->Map(_ctx, (ID3D11Resource*)_cb_proj,
                                         0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
            memcpy(ms.pData, mat4, 64);
            _ctx->lpVtbl->Unmap(_ctx, (ID3D11Resource*)_cb_proj, 0);
        }
    }
}

// --- Sprite (Sistema 1) ---

TupiSprite* tupi_sprite_carregar(const char* caminho) {
    TupiImagemRust* img = tupi_imagem_carregar_seguro(caminho);
    if (!img) {
        fprintf(stderr, "[TupiSprite] Falha ao carregar '%s'\n", caminho);
        return NULL;
    }

    D3D11_TEXTURE2D_DESC td = {
        .Width      = (UINT)img->largura,
        .Height     = (UINT)img->altura,
        .MipLevels  = 1,
        .ArraySize  = 1,
        .Format     = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1 },
        .Usage      = D3D11_USAGE_IMMUTABLE,
        .BindFlags  = D3D11_BIND_SHADER_RESOURCE,
    };
    D3D11_SUBRESOURCE_DATA sd = {
        .pSysMem     = img->pixels,
        .SysMemPitch = (UINT)(img->largura * 4),
    };

    ID3D11Texture2D*          tex2d = NULL;
    ID3D11ShaderResourceView* srv   = NULL;

    HRESULT hr = _device->lpVtbl->CreateTexture2D(_device, &td, &sd, &tex2d);
    if (FAILED(hr)) {
        fprintf(stderr, "[TupiSprite] Falha ao criar textura D3D11 para '%s'\n", caminho);
        tupi_imagem_destruir(img);
        return NULL;
    }

    _device->lpVtbl->CreateShaderResourceView(_device,
        (ID3D11Resource*)tex2d, NULL, &srv);
    tex2d->lpVtbl->Release(tex2d);

    int larg = img->largura, alt = img->altura;
    tupi_imagem_destruir(img);

    TupiSprite* sprite = (TupiSprite*)malloc(sizeof(TupiSprite));
    sprite->_srv    = srv;
    sprite->largura = larg;
    sprite->altura  = alt;

    printf("[TupiSprite] '%s' carregado via Rust (%dx%d)\n", caminho, larg, alt);
    return sprite;
}

void tupi_sprite_destruir(TupiSprite* sprite) {
    if (!sprite) return;
    if (sprite->_srv)
        ((ID3D11ShaderResourceView*)sprite->_srv)->lpVtbl->Release(
            (ID3D11ShaderResourceView*)sprite->_srv);
    free(sprite);
}

// --- Atlas (Sistema 2) ---

TupiAtlas* tupi_atlas_criar_c(void) {
    TupiAtlas* a = (TupiAtlas*)malloc(sizeof(TupiAtlas));
    a->_interno = tupi_atlas_criar();
    return a;
}

void tupi_atlas_destruir_c(TupiAtlas* atlas) {
    if (!atlas) return;
    tupi_atlas_destruir(atlas->_interno);
    free(atlas);
}

void tupi_atlas_registrar_animacao(TupiAtlas* atlas, const char* nome,
    unsigned int linha, unsigned int colunas,
    float cel_larg, float cel_alt, float img_larg, float img_alt) {
    if (!atlas || !atlas->_interno) return;
    tupi_atlas_registrar(atlas->_interno, nome, linha, colunas,
                         cel_larg, cel_alt, img_larg, img_alt);
}

int tupi_atlas_obter_uv(TupiAtlas* atlas, const char* nome, unsigned int frame,
    float* u0, float* v0, float* u1, float* v1) {
    if (!atlas || !atlas->_interno) return 0;
    TupiUV uv = {0};
    int ok = tupi_atlas_uv(atlas->_interno, nome, frame, &uv);
    if (ok) { *u0=uv.u0; *v0=uv.v0; *u1=uv.u1; *v1=uv.v1; }
    return ok;
}

// --- Batch (Sistema 3) ---

void tupi_objeto_enviar_batch(TupiObjeto* obj, int z_index) {
    if (!obj || !obj->imagem || !_batch) return;
    TupiSprite* spr = obj->imagem;
    float u0 = (obj->coluna * obj->largura) / (float)spr->largura;
    float v0 = (obj->linha  * obj->altura)  / (float)spr->altura;
    float u1 = u0 + obj->largura / (float)spr->largura;
    float v1 = v0 + obj->altura  / (float)spr->altura;
    float sw = obj->largura * obj->escala;
    float sh = obj->altura  * obj->escala;
    float quad[16] = {
        obj->x,      obj->y,      u0, v0,
        obj->x + sw, obj->y,      u1, v0,
        obj->x,      obj->y + sh, u0, v1,
        obj->x + sw, obj->y + sh, u1, v1,
    };
    unsigned int id = (unsigned int)(uintptr_t)spr->_srv;
    tupi_batch_adicionar(_batch, id, quad, obj->transparencia, z_index);
}

void tupi_objeto_enviar_batch_atlas(TupiObjeto* obj, TupiAtlas* atlas,
    const char* animacao, unsigned int frame, int z_index) {
    if (!obj || !obj->imagem || !atlas || !_batch) return;
    TupiSprite* spr = obj->imagem;
    TupiUV uv = {0};
    if (!tupi_atlas_uv(atlas->_interno, animacao, frame, &uv)) {
        fprintf(stderr, "[TupiSprite] Atlas: animacao '%s' nao encontrada\n", animacao);
        return;
    }
    float sw = obj->largura * obj->escala;
    float sh = obj->altura  * obj->escala;
    float quad[16] = {
        obj->x,      obj->y,      uv.u0, uv.v0,
        obj->x + sw, obj->y,      uv.u1, uv.v0,
        obj->x,      obj->y + sh, uv.u0, uv.v1,
        obj->x + sw, obj->y + sh, uv.u1, uv.v1,
    };
    unsigned int id = (unsigned int)(uintptr_t)spr->_srv;
    tupi_batch_adicionar(_batch, id, quad, obj->transparencia, z_index);
}

void tupi_objeto_enviar_batch_raw(unsigned int textura_id, const float* quad,
    float alpha, int z_index) {
    if (!_batch || !quad) return;
    tupi_batch_adicionar(_batch, textura_id, quad, alpha, z_index);
}

void tupi_batch_desenhar(void) {
    if (!_batch || !_ctx) return;

    int contagem = 0;
    const TupiItemBatch* itens = tupi_batch_flush(_batch, &contagem);
    if (contagem <= 0 || !itens) { tupi_batch_limpar(_batch); return; }

    _bind_pipeline();

    for (int i = 0; i < contagem; i++) {
        const TupiItemBatch* item = &itens[i];
        ID3D11ShaderResourceView* srv =
            (ID3D11ShaderResourceView*)(uintptr_t)item->textura_id;
        _dx11_draw_quad(item->quad, srv, item->alpha);
    }

    tupi_batch_limpar(_batch);
}

// --- Objeto (modo imediato) ---

TupiObjeto tupi_objeto_criar(float x, float y, float largura, float altura,
    int coluna, int linha, float transparencia, float escala, TupiSprite* imagem) {
    TupiObjeto obj;
    obj.x = x; obj.y = y;
    obj.largura = largura; obj.altura = altura;
    obj.coluna = coluna;   obj.linha  = linha;
    obj.transparencia = transparencia;
    obj.escala = escala;
    obj.imagem = imagem;
    return obj;
}

void tupi_objeto_desenhar(TupiObjeto* obj) {
    if (!obj || !obj->imagem || !_ctx) return;
    TupiSprite* spr = obj->imagem;
    float u0 = (obj->coluna * obj->largura) / (float)spr->largura;
    float v0 = (obj->linha  * obj->altura)  / (float)spr->altura;
    float u1 = u0 + obj->largura / (float)spr->largura;
    float v1 = v0 + obj->altura  / (float)spr->altura;
    float sw = obj->largura * obj->escala;
    float sh = obj->altura  * obj->escala;
    float quad[16] = {
        obj->x,      obj->y,      u0, v0,
        obj->x + sw, obj->y,      u1, v0,
        obj->x,      obj->y + sh, u0, v1,
        obj->x + sw, obj->y + sh, u1, v1,
    };
    _bind_pipeline();
    _dx11_draw_quad(quad, (ID3D11ShaderResourceView*)spr->_srv, obj->transparencia);
}