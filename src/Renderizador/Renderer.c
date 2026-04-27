// Renderer.c — TupiEngine (SDL2)

#include "Renderer.h"
#include "../Inputs/Inputs.h"
#include "../Sprites/Sprites.h"
#include "../Camera/Camera.h"
#include <stdint.h>
#include <math.h>
#include <string.h>

// --- Estado interno ---

static SDL_Window*   _janela   = NULL;
static SDL_Renderer* _renderer = NULL;

static int   _largura  = 800;  // tamanho físico atual (px)
static int   _altura   = 600;
static int   _logico_w = 800;  // tamanho lógico do mundo
static int   _logico_h = 600;

static float _fundo_r = 0.1f, _fundo_g = 0.1f, _fundo_b = 0.15f;
static float _cor_r   = 1.0f, _cor_g   = 1.0f, _cor_b   = 1.0f, _cor_a = 1.0f;

static double _tempo_inicio   = 0.0;
static double _tempo_anterior = 0.0;
static double _delta          = 0.0;

static int   _flag_sem_borda = 0;
static float _escala         = 1.0f;
static int   _janela_aberta  = 0;

// --- Letterbox ---

static int _letterbox_ativo = 0;
static int _lb_x = 0, _lb_y = 0, _lb_w = 0, _lb_h = 0;

static void _calcular_letterbox(int fw, int fh) {
    float razao_alvo = (float)_logico_w / (float)_logico_h;
    float razao_tela = (float)fw        / (float)fh;

    if (razao_tela >= razao_alvo) {
        _lb_h = fh;
        _lb_w = (int)roundf((float)fh * razao_alvo);
        _lb_x = (fw - _lb_w) / 2;
        _lb_y = 0;
    } else {
        _lb_w = fw;
        _lb_h = (int)roundf((float)fw / razao_alvo);
        _lb_x = 0;
        _lb_y = (fh - _lb_h) / 2;
    }
}

// --- Limitador de FPS ---

static int    _fps_limite  = 0;
static float  _fps_atual   = 0.0f;
static Uint64 _tick_frame  = 0;

void tupi_fps_limite(int fps_max) {
    _fps_limite = (fps_max > 0) ? fps_max : 0;
}

float tupi_fps_atual(void) {
    return _fps_atual;
}

static void _fps_frame_inicio(void) {
    _tick_frame = SDL_GetPerformanceCounter();
}

static void _fps_frame_fim(void) {
    Uint64 freq  = SDL_GetPerformanceFrequency();
    Uint64 agora = SDL_GetPerformanceCounter();

    double frame_seg = (double)(agora - _tick_frame) / (double)freq;

    // Média exponencial — suaviza oscilações de FPS
    float fps_medido = (frame_seg > 0.0) ? (float)(1.0 / frame_seg) : 9999.0f;
    _fps_atual = _fps_atual * 0.9f + fps_medido * 0.1f;

    if (_fps_limite > 0) {
        double alvo_seg = 1.0 / (double)_fps_limite;
        double restante = alvo_seg - frame_seg;

        if (restante > 0.001) {
            // Dorme um pouco menos e faz busy-wait no final para precisão alta
            double dormir_ms = restante * 1000.0 - 1.5;
            if (dormir_ms > 0.0)
                SDL_Delay((Uint32)dormir_ms);

            while (1) {
                double elapsed = (double)(SDL_GetPerformanceCounter() - _tick_frame) / (double)freq;
                if (elapsed >= alvo_seg) break;
            }
        }
    }
}

// --- Acesso interno ---

SDL_Renderer* tupi_renderer_get(void) { return _renderer; }

// --- Projeção / viewport ---

static void _configurar_projecao(int largura, int altura) {
    if (_letterbox_ativo) {
        _calcular_letterbox(largura, altura);
        SDL_RenderSetViewport(_renderer, &(SDL_Rect){_lb_x, _lb_y, _lb_w, _lb_h});
        SDL_RenderSetScale(_renderer,
            (float)_lb_w / (float)_logico_w,
            (float)_lb_h / (float)_logico_h);
    } else {
        SDL_RenderSetViewport(_renderer, NULL);
        SDL_RenderSetScale(_renderer,
            (float)largura  / (float)_logico_w,
            (float)altura   / (float)_logico_h);
    }

    TupiCamera* cam = tupi_camera_ativa();
    if (cam) {
        tupi_camera_frame(cam, _logico_w, _logico_h);
    } else {
        TupiMatriz proj = tupi_projecao_ortografica(_logico_w, _logico_h);
        tupi_sprite_set_viewport(_logico_w, _logico_h);
        tupi_sprite_set_projecao(proj.m);
    }
}

// --- Renderização de primitivos (SDL_RenderGeometry) ---

#define TUPI_MAX_VERTICES 1024

static inline Uint8 _f2u(float v) {
    return (Uint8)(v < 0.0f ? 0 : v > 1.0f ? 255 : v * 255.0f);
}

// Polígono preenchido (triangle-fan) ou apenas bordas (line-loop)
static void _sdl_desenhar_poly(float* verts, int n_pts, int filled) {
    if (n_pts < 2) return;

    SDL_Color cor = { _f2u(_cor_r), _f2u(_cor_g), _f2u(_cor_b), _f2u(_cor_a) };
    SDL_SetRenderDrawColor(_renderer, cor.r, cor.g, cor.b, cor.a);
    SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);

    if (!filled) {
        for (int i = 0; i < n_pts; i++) {
            int j = (i + 1) % n_pts;
            SDL_RenderDrawLineF(_renderer,
                verts[i*2], verts[i*2+1],
                verts[j*2], verts[j*2+1]);
        }
        return;
    }

    SDL_Vertex sv[TUPI_MAX_VERTICES];
    for (int i = 0; i < n_pts; i++) {
        sv[i].position.x = verts[i*2];
        sv[i].position.y = verts[i*2+1];
        sv[i].color      = cor;
        sv[i].tex_coord  = (SDL_FPoint){0, 0};
    }

    // Triangle-fan: (0,1,2), (0,2,3), ...
    int n_tri = n_pts - 2;
    if (n_tri <= 0) return;
    int indices[TUPI_MAX_VERTICES * 3];
    for (int i = 0; i < n_tri; i++) {
        indices[i*3+0] = 0;
        indices[i*3+1] = i + 1;
        indices[i*3+2] = i + 2;
    }
    SDL_RenderGeometry(_renderer, NULL, sv, n_pts, indices, n_tri * 3);
}

// Linha com espessura: quad alinhado ao segmento quando > 1px
static void _desenhar_linha_sdl(float x1, float y1, float x2, float y2, float espessura) {
    if (espessura <= 1.5f) {
        SDL_SetRenderDrawColor(_renderer,
            _f2u(_cor_r), _f2u(_cor_g), _f2u(_cor_b), _f2u(_cor_a));
        SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderDrawLineF(_renderer, x1, y1, x2, y2);
        return;
    }

    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) return;
    float nx = -dy / len * (espessura * 0.5f);
    float ny =  dx / len * (espessura * 0.5f);

    float verts[8] = {
        x1+nx, y1+ny,
        x1-nx, y1-ny,
        x2-nx, y2-ny,
        x2+nx, y2+ny,
    };
    _sdl_desenhar_poly(verts, 4, 1);
}

// Flush do batcher de formas 2D (registrado via tupi_batcher_registrar_flush)
static void _flush_batcher(const TupiDrawCall* calls, int n) {
    for (int i = 0; i < n; i++) {
        const TupiDrawCall* dc = &calls[i];
        _cor_r = dc->cor[0]; _cor_g = dc->cor[1];
        _cor_b = dc->cor[2]; _cor_a = dc->cor[3];

        float* v = (float*)dc->verts;
        switch (dc->primitiva) {
            case TUPI_RET: {
                // Reordena TL/TR/BL/BR → winding correto para polígono
                float quad[8] = {
                    v[0], v[1],
                    v[2], v[3],
                    v[6], v[7],
                    v[4], v[5],
                };
                _sdl_desenhar_poly(quad, 4, 1);
                break;
            }
            case TUPI_TRI:
                _sdl_desenhar_poly(v, 3, 1);
                break;
            case TUPI_LIN:
                _desenhar_linha_sdl(v[0], v[1], v[2], v[3], 1.0f);
                break;
            default: break;
        }
    }
}

// --- Ícone da janela ---

typedef struct {
    unsigned char* pixels;
    int largura, altura, tamanho;
} TupiImagemRust;

extern TupiImagemRust* tupi_imagem_carregar_seguro(const char* caminho);
extern void            tupi_imagem_destruir(TupiImagemRust* img);

static void _carregar_icone(const char* caminho) {
    const char* alvo = (caminho && caminho[0]) ? caminho : ".engine/icon.png";

    TupiImagemRust* img = tupi_imagem_carregar_seguro(alvo);
    if (!img) {
        fprintf(stderr, "[Renderer] Icone nao encontrado: '%s'\n", alvo);
        return;
    }

    SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(
        img->pixels, img->largura, img->altura,
        32, img->largura * 4,
        0x000000FFu, 0x0000FF00u, 0x00FF0000u, 0xFF000000u
    );

    if (surf) {
        SDL_SetWindowIcon(_janela, surf);
        SDL_FreeSurface(surf);
    } else {
        fprintf(stderr, "[Renderer] Falha ao definir icone: %s\n", SDL_GetError());
    }

    tupi_imagem_destruir(img);
}

// --- Criação da janela ---

int tupi_janela_criar(int largura, int altura, const char* titulo,
                      float escala, int sem_borda, const char* icone) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "[Renderer] Falha ao inicializar SDL2: %s\n", SDL_GetError());
        return 0;
    }

    _escala         = (escala > 0.0f) ? escala : 1.0f;
    _flag_sem_borda = sem_borda;

    int fis_w = (int)(largura * _escala);
    int fis_h = (int)(altura  * _escala);
    _logico_w = largura;
    _logico_h = altura;
    _largura  = fis_w;
    _altura   = fis_h;

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (sem_borda) flags |= SDL_WINDOW_BORDERLESS;

    _janela = SDL_CreateWindow(
        titulo ? titulo : "TupiEngine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        fis_w, fis_h, flags
    );
    if (!_janela) {
        fprintf(stderr, "[Renderer] Falha ao criar janela: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    // Tenta renderer acelerado; cai para software se não disponível
    _renderer = SDL_CreateRenderer(_janela, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!_renderer) {
        fprintf(stderr, "[Renderer] Acelerado indisponivel — usando software\n");
        _renderer = SDL_CreateRenderer(_janela, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!_renderer) {
        fprintf(stderr, "[Renderer] Falha ao criar renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(_janela);
        SDL_Quit();
        return 0;
    }

    SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);

    _carregar_icone(icone);
    tupi_sprite_iniciar();
    tupi_batcher_registrar_flush(_flush_batcher);
    _configurar_projecao(fis_w, fis_h);

    _tempo_inicio   = (double)SDL_GetPerformanceCounter() /
                      (double)SDL_GetPerformanceFrequency();
    _tempo_anterior = _tempo_inicio;
    _tick_frame     = SDL_GetPerformanceCounter();

    tupi_input_iniciar(_janela);
    tupi_input_set_dimensoes(&_largura, &_altura, &_logico_w, &_logico_h);

    _janela_aberta = 1;

    printf("[TupiEngine] SDL2 %d.%d.%d\n",
           SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    printf("[TupiEngine] Janela: %dx%d px | mundo: %dx%d | escala: %.2fx\n",
           fis_w, fis_h, _logico_w, _logico_h, _escala);

    return 1;
}

// --- Loop principal ---

int tupi_janela_aberta(void) {
    return _janela_aberta;
}

void tupi_janela_limpar(void) {
    _fps_frame_inicio();

    Uint64 freq  = SDL_GetPerformanceFrequency();
    double agora = (double)SDL_GetPerformanceCounter() / (double)freq;
    _delta          = agora - _tempo_anterior;
    _tempo_anterior = agora;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                _janela_aberta = 0;
                break;
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED ||
                    ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    int w, h;
                    SDL_GetRendererOutputSize(_renderer, &w, &h);
                    _largura = w;
                    _altura  = h;
                    if (!_letterbox_ativo)
                        _escala = (_logico_w > 0) ? (float)w / (float)_logico_w : 1.0f;
                    _configurar_projecao(w, h);
                }
                break;
            default:
                break;
        }
        tupi_input_processar_evento(&ev);
    }

    if (_letterbox_ativo) {
        // Barras pretas + cor de fundo na área do jogo
        SDL_RenderSetViewport(_renderer, NULL);
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);

        SDL_RenderSetViewport(_renderer, &(SDL_Rect){_lb_x, _lb_y, _lb_w, _lb_h});
        SDL_SetRenderDrawColor(_renderer,
            _f2u(_fundo_r), _f2u(_fundo_g), _f2u(_fundo_b), 255);
        SDL_RenderClear(_renderer);
    } else {
        SDL_SetRenderDrawColor(_renderer,
            _f2u(_fundo_r), _f2u(_fundo_g), _f2u(_fundo_b), 255);
        SDL_RenderClear(_renderer);
    }

    TupiCamera* cam = tupi_camera_ativa();
    if (cam) {
        tupi_camera_frame(cam, _logico_w, _logico_h);
    } else {
        TupiMatriz proj = tupi_projecao_ortografica(_logico_w, _logico_h);
        tupi_sprite_set_viewport(_logico_w, _logico_h);
        tupi_sprite_set_projecao(proj.m);
    }

    tupi_sprite_reset_cor();
    _cor_r = _cor_g = _cor_b = _cor_a = 1.0f;
}

void tupi_janela_atualizar(void) {
    tupi_batch_desenhar();    // flush sprites (texturas)
    tupi_batcher_flush();     // flush formas 2D
    SDL_RenderPresent(_renderer);
    tupi_input_salvar_estado();
    tupi_input_atualizar_mouse();
    _fps_frame_fim();
}

void tupi_janela_fechar(void) {
    tupi_sprite_encerrar();
    if (_renderer) { SDL_DestroyRenderer(_renderer); _renderer = NULL; }
    if (_janela)   { SDL_DestroyWindow(_janela);     _janela   = NULL; }
    SDL_Quit();
    _janela_aberta = 0;
    printf("[TupiEngine] Encerrado.\n");
}

// --- Controles em runtime ---

void tupi_janela_set_titulo(const char* titulo) {
    if (_janela && titulo) SDL_SetWindowTitle(_janela, titulo);
}

void tupi_janela_set_decoracao(int ativo) {
    if (!_janela) return;
    SDL_SetWindowBordered(_janela, ativo ? SDL_TRUE : SDL_FALSE);
    _flag_sem_borda = !ativo;
}

void tupi_janela_tela_cheia(int ativo) {
    if (!_janela) return;
    if (ativo) {
        SDL_SetWindowFullscreen(_janela, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        _letterbox_ativo = 0;
        SDL_SetWindowFullscreen(_janela, 0);
        SDL_RenderSetViewport(_renderer, NULL);
    }
}

void tupi_janela_tela_cheia_letterbox(int ativo) {
    if (!_janela) return;

    if (ativo) {
        _letterbox_ativo = 1;
        SDL_SetWindowFullscreen(_janela, SDL_WINDOW_FULLSCREEN_DESKTOP);

        int fw, fh;
        SDL_GetRendererOutputSize(_renderer, &fw, &fh);
        _calcular_letterbox(fw, fh);
        _configurar_projecao(fw, fh);
    } else {
        _letterbox_ativo = 0;
        SDL_SetWindowFullscreen(_janela, 0);
        SDL_RenderSetViewport(_renderer, NULL);

        int fis_w = (int)(_logico_w * _escala);
        int fis_h = (int)(_logico_h * _escala);
        SDL_SetWindowSize(_janela, fis_w, fis_h);
        _configurar_projecao(fis_w, fis_h);
    }
}

int tupi_janela_letterbox_ativo(void) { return _letterbox_ativo; }

// --- Getters ---

float  tupi_janela_escala(void)     { return _escala;   }
double tupi_tempo(void) {
    return (double)SDL_GetPerformanceCounter() /
           (double)SDL_GetPerformanceFrequency() - _tempo_inicio;
}
double tupi_delta_tempo(void)       { return _delta;    }
int    tupi_janela_largura(void)    { return _logico_w; }
int    tupi_janela_altura(void)     { return _logico_h; }
int    tupi_janela_largura_px(void) { return _largura;  }
int    tupi_janela_altura_px(void)  { return _altura;   }

// --- Cor ---

void tupi_cor_fundo(float r, float g, float b) {
    _fundo_r = r; _fundo_g = g; _fundo_b = b;
}

void tupi_cor(float r, float g, float b, float a) {
    _cor_r = r; _cor_g = g; _cor_b = b; _cor_a = a;
    tupi_sprite_set_cor(r, g, b, a);
}

// --- Formas 2D ---

void tupi_retangulo(float x, float y, float largura, float altura) {
    float verts[8] = {
        x,         y,
        x+largura, y,
        x+largura, y+altura,
        x,         y+altura,
    };
    _sdl_desenhar_poly(verts, 4, 1);
}

void tupi_retangulo_borda(float x, float y, float largura, float altura, float espessura) {
    if (espessura <= 1.5f) {
        SDL_SetRenderDrawColor(_renderer,
            _f2u(_cor_r), _f2u(_cor_g), _f2u(_cor_b), _f2u(_cor_a));
        SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
        SDL_FRect r = { x, y, largura, altura };
        SDL_RenderDrawRectF(_renderer, &r);
        return;
    }
    _desenhar_linha_sdl(x,         y,          x+largura, y,          espessura);
    _desenhar_linha_sdl(x+largura, y,          x+largura, y+altura,   espessura);
    _desenhar_linha_sdl(x+largura, y+altura,   x,         y+altura,   espessura);
    _desenhar_linha_sdl(x,         y+altura,   x,         y,          espessura);
}

void tupi_triangulo(float x1, float y1, float x2, float y2, float x3, float y3) {
    float v[6] = { x1, y1, x2, y2, x3, y3 };
    _sdl_desenhar_poly(v, 3, 1);
}

void tupi_circulo(float x, float y, float raio, int segmentos) {
    if (segmentos > TUPI_MAX_VERTICES - 1) segmentos = TUPI_MAX_VERTICES - 1;
    float verts[TUPI_MAX_VERTICES * 2];
    for (int i = 0; i <= segmentos; i++) {
        float a = (float)i / (float)segmentos * 2.0f * (float)M_PI;
        verts[i*2+0] = x + cosf(a) * raio;
        verts[i*2+1] = y + sinf(a) * raio;
    }
    _sdl_desenhar_poly(verts, segmentos, 1);
}

void tupi_circulo_borda(float x, float y, float raio, int segmentos, float espessura) {
    if (segmentos > TUPI_MAX_VERTICES) segmentos = TUPI_MAX_VERTICES;
    float verts[TUPI_MAX_VERTICES * 2];
    for (int i = 0; i < segmentos; i++) {
        float a = (float)i / (float)segmentos * 2.0f * (float)M_PI;
        verts[i*2+0] = x + cosf(a) * raio;
        verts[i*2+1] = y + sinf(a) * raio;
    }
    if (espessura <= 1.5f) {
        _sdl_desenhar_poly(verts, segmentos, 0);
    } else {
        for (int i = 0; i < segmentos; i++) {
            int j = (i + 1) % segmentos;
            _desenhar_linha_sdl(verts[i*2], verts[i*2+1],
                                verts[j*2], verts[j*2+1], espessura);
        }
    }
}

void tupi_linha(float x1, float y1, float x2, float y2, float espessura) {
    _desenhar_linha_sdl(x1, y1, x2, y2, espessura);
}