// RendererGL.c — TupiEngine (OpenGL 3.3 Core Profile)

#include "RendererGL.h"
#include "../Inputs/Inputs.h"
#include "../Sprites/Sprites.h"
#include "../Camera/Camera.h"
#include <stdint.h>
#include <math.h>

#if defined(__linux__) && !defined(TUPI_SEM_X11)
#  define GLFW_EXPOSE_NATIVE_X11
#  include <X11/Xlib.h>
#  include <X11/Xatom.h>
#  include <GLFW/glfw3native.h>
#endif

// --- Shaders ---

static const char* _vert_src =
    "#version 330 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "uniform mat4 uProjecao;\n"
    "void main() {\n"
    "    gl_Position = uProjecao * vec4(aPos, 0.0, 1.0);\n"
    "}\n";

static const char* _frag_src =
    "#version 330 core\n"
    "uniform vec4 uCor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = uCor;\n"
    "}\n";

// --- Estado interno ---

static GLFWwindow* _janela   = NULL;
static int         _largura  = 800;
static int         _altura   = 600;
static int         _logico_w = 800;
static int         _logico_h = 600;

static float _fundo_r = 0.1f, _fundo_g = 0.1f, _fundo_b = 0.15f;
static float _cor_r   = 1.0f, _cor_g   = 1.0f, _cor_b   = 1.0f, _cor_a = 1.0f;

static double _tempo_anterior = 0.0;
static double _delta          = 0.0;

static GLuint _shader = 0, _vao = 0, _vbo = 0;
static GLint  _loc_projecao = -1, _loc_cor = -1;

static int   _flag_sem_borda = 0;
static float _escala         = 1.0f;

/* ── Letterbox ─────────────────────────────────────────────────────────────
   Quando ativo, o jogo é renderizado numa viewport centralizada que mantém
   o aspect ratio do mundo lógico. As barras pretas são desenhadas pelo
   glClearColor(0,0,0,1) antes de setarmos a viewport recortada.
   ────────────────────────────────────────────────────────────────────────── */
static int _letterbox_ativo = 0;  /* 0 = desligado, 1 = ligado              */

/* Viewport calculada pelo letterbox (preenchida por _calcular_letterbox)    */
static int _lb_x = 0, _lb_y = 0, _lb_w = 0, _lb_h = 0;

/* Tamanho físico da janela/monitor quando em tela-cheia letterbox           */
static int _fis_tela_w = 0, _fis_tela_h = 0;

/* Calcula a viewport letterboxada para um framebuffer de (fw x fh) pixels,
   preservando a proporção (_logico_w : _logico_h).                          */
static void _calcular_letterbox(int fw, int fh) {
    _fis_tela_w = fw;
    _fis_tela_h = fh;

    float razao_alvo = (float)_logico_w / (float)_logico_h;
    float razao_tela = (float)fw         / (float)fh;

    if (razao_tela >= razao_alvo) {
        /* Barras laterais (pillarbox) */
        _lb_h = fh;
        /* roundf() em vez de truncamento: evita que _lb_w fique 1px menor
           que o ideal, o que deslocaria _lb_x por 0.5px e causaria
           sub-pixel bleeding nos tiles ao mover a câmera no eixo Y.         */
        _lb_w = (int)roundf((float)fh * razao_alvo);
        _lb_x = (fw - _lb_w) / 2;
        _lb_y = 0;
    } else {
        /* Barras superiores/inferiores (letterbox) */
        _lb_w = fw;
        /* roundf() em vez de truncamento: sem isso _lb_h pode ficar 1px
           abaixo do valor correto; (fh - _lb_h) vira ímpar e _lb_y fica
           com offset fracionário implícito na viewport GL, fazendo o
           sampler GL_NEAREST piscar na borda inferior dos tiles durante
           movimento vertical da câmera.                                      */
        _lb_h = (int)roundf((float)fw / razao_alvo);
        _lb_x = 0;
        _lb_y = (fh - _lb_h) / 2;
    }
}

#define TUPI_MAX_VERTICES 1024

// --- Getters internos (usados por Camera.c) ---

GLuint _tupi_shader_get(void)       { return _shader;       }
GLint  _tupi_loc_projecao_get(void) { return _loc_projecao; }

// --- Helpers de shader ---

static GLuint _compilar_shader(GLenum tipo, const char* src) {
    GLuint s = glCreateShader(tipo);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "[TupiEngine] Erro no shader: %s\n", log);
    }
    return s;
}

static GLuint _criar_programa(const char* vert, const char* frag) {
    GLuint vs = _compilar_shader(GL_VERTEX_SHADER,   vert);
    GLuint fs = _compilar_shader(GL_FRAGMENT_SHADER, frag);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "[TupiEngine] Erro ao linkar programa: %s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// --- Projeção / câmera ---

/* Aplica a viewport correta e envia a projeção ortográfica ao shader.
   Em modo letterbox, a viewport é a região recortada; fora dele é o
   framebuffer inteiro.                                                       */
static void _configurar_projecao(int largura, int altura) {
    if (_letterbox_ativo) {
        /* Barras pretas: limpa o framebuffer completo antes de recortar     */
        glViewport(0, 0, largura, altura);
        glScissor (0, 0, largura, altura);
        glDisable(GL_SCISSOR_TEST);

        _calcular_letterbox(largura, altura);
        glViewport(_lb_x, _lb_y, _lb_w, _lb_h);
        glScissor (_lb_x, _lb_y, _lb_w, _lb_h);
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, largura, altura);
    }

    TupiCamera* cam = tupi_camera_ativa();
    if (cam) {
        /* Camera.c já aplica roundf() em _recalcular() — pixel-snapping
           garantido sem necessidade de intervenção aqui.                     */
        tupi_camera_frame(cam, _logico_w, _logico_h);
    } else {
        TupiMatriz proj = tupi_projecao_ortografica(_logico_w, _logico_h);
        glUseProgram(_tupi_shader_get());
        glUniformMatrix4fv(_tupi_loc_projecao_get(), 1, GL_FALSE, proj.m);
        tupi_sprite_set_projecao(proj.m);
    }
}

static void _callback_resize(GLFWwindow* janela, int largura, int altura) {
    (void)janela;
    _largura = largura;
    _altura  = altura;
    if (_letterbox_ativo) {
        /* No modo letterbox a escala lógica não muda: o mundo continua
           sendo _logico_w x _logico_h — apenas a viewport se ajusta.       */
        _calcular_letterbox(largura, altura);
    } else {
        _escala = (_logico_w > 0) ? (float)largura / (float)_logico_w : 1.0f;
    }
    _configurar_projecao(largura, altura);
}

static void _erro_glfw(int codigo, const char* descricao) {
    fprintf(stderr, "[TupiEngine] Erro GLFW %d: %s\n", codigo, descricao);
}

static void _desenhar(GLenum modo, float* verts, int n);

// --- Flush callback ---

static void _flush_batcher(const TupiDrawCall* calls, int n) {
    for (int i = 0; i < n; i++) {
        const TupiDrawCall* dc = &calls[i];
        _cor_r = dc->cor[0]; _cor_g = dc->cor[1];
        _cor_b = dc->cor[2]; _cor_a = dc->cor[3];
        switch (dc->primitiva) {
            case TUPI_RET: _desenhar(GL_TRIANGLE_STRIP, (float*)dc->verts, 4); break;
            case TUPI_TRI: _desenhar(GL_TRIANGLES,      (float*)dc->verts, 3); break;
            case TUPI_LIN: _desenhar(GL_LINES,          (float*)dc->verts, 2); break;
            default: break;
        }
    }
}

// --- Draw call genérico ---

static void _desenhar(GLenum modo, float* verts, int n) {
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 2 * n, verts);
    glUseProgram(_shader);
    glUniform4f(_loc_cor, _cor_r, _cor_g, _cor_b, _cor_a);
    glDrawArrays(modo, 0, n);
    glBindVertexArray(0);
}

// --- Ícone da janela ---

typedef struct {
    unsigned char* pixels;
    int largura, altura, tamanho;
} TupiImagemRust;

extern TupiImagemRust* tupi_imagem_carregar_seguro(const char* caminho);
extern void            tupi_imagem_destruir(TupiImagemRust* img);

static unsigned char* _escalar_pixels_nn(
    const unsigned char* src, int larg, int alt, int fator,
    int* out_larg, int* out_alt)
{
    int nl = larg * fator;
    int na = alt  * fator;
    unsigned char* dst = (unsigned char*)malloc((size_t)(nl * na * 4));
    if (!dst) { *out_larg = 0; *out_alt = 0; return NULL; }
    for (int y = 0; y < na; y++) {
        for (int x = 0; x < nl; x++) {
            const unsigned char* p = src + ((y / fator) * larg + (x / fator)) * 4;
            unsigned char*       q = dst + (y * nl + x) * 4;
            q[0]=p[0]; q[1]=p[1]; q[2]=p[2]; q[3]=p[3];
        }
    }
    *out_larg = nl;
    *out_alt  = na;
    return dst;
}

static int _ewmh_escrever_bloco(long* dst, const unsigned char* rgba, int larg, int alt) {
    dst[0] = (long)larg;
    dst[1] = (long)alt;
    int n = larg * alt;
    for (int i = 0; i < n; i++) {
        unsigned long r = rgba[i*4+0], g = rgba[i*4+1];
        unsigned long b = rgba[i*4+2], a = rgba[i*4+3];
        dst[2+i] = (long)((a<<24)|(r<<16)|(g<<8)|b);
    }
    return 2 + n;
}

static int _aplicar_icone_do_arquivo(const char* caminho) {
    TupiImagemRust* img = tupi_imagem_carregar_seguro(caminho);
    if (!img) {
        fprintf(stderr, "[TupiEngine] Icone: falha ao carregar '%s'\n", caminho);
        return 0;
    }

    fprintf(stderr, "[TupiEngine] Icone carregado: %dx%d px\n", img->largura, img->altura);

    int ok = 0;

#if defined(__linux__) && !defined(TUPI_SEM_X11)
    {
        Display* display = glfwGetX11Display();
        Window   xwindow = glfwGetX11Window(_janela);

        if (display && xwindow) {
            int larg0 = img->largura, alt0 = img->altura;
            int larg2=0, alt2=0, larg3=0, alt3=0;
            unsigned char* px2 = _escalar_pixels_nn(img->pixels, larg0, alt0, 2, &larg2, &alt2);
            unsigned char* px3 = _escalar_pixels_nn(img->pixels, larg0, alt0, 3, &larg3, &alt3);

            int n_total = (2 + larg0*alt0)
                        + (px2 ? 2 + larg2*alt2 : 0)
                        + (px3 ? 2 + larg3*alt3 : 0);

            long* data = (long*)malloc(sizeof(long) * n_total);
            if (data) {
                int off = 0;
                off += _ewmh_escrever_bloco(data+off, img->pixels, larg0, alt0);
                if (px2) off += _ewmh_escrever_bloco(data+off, px2, larg2, alt2);
                if (px3) off += _ewmh_escrever_bloco(data+off, px3, larg3, alt3);

                Atom net_wm_icon = XInternAtom(display, "_NET_WM_ICON", False);
                XChangeProperty(display, xwindow,
                    net_wm_icon, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)data, n_total);
                XSync(display, False);
                free(data);

                fprintf(stderr, "[TupiEngine] Icone X11: %dx%d + %dx%d + %dx%d enviados\n",
                    larg0,alt0, larg2,alt2, larg3,alt3);
                ok = 1;
            }
            if (px2) free(px2);
            if (px3) free(px3);
        } else {
            fprintf(stderr, "[TupiEngine] Icone: display=%p xwindow=%lu — X11 indisponivel\n",
                (void*)display, (unsigned long)xwindow);
        }
    }
#endif

    if (!ok) {
        GLFWimage icone = {
            .width  = img->largura,
            .height = img->altura,
            .pixels = img->pixels
        };
        glfwSetWindowIcon(_janela, 1, &icone);
        fprintf(stderr, "[TupiEngine] Icone GLFW: %s (%dx%d)\n",
                caminho, img->largura, img->altura);
        ok = 1;
    }

    tupi_imagem_destruir(img);
    return ok;
}

static void _carregar_icone(const char* caminho) {
    if (caminho && caminho[0] != '\0') {
        if (_aplicar_icone_do_arquivo(caminho)) return;
        fprintf(stderr, "[TupiEngine] Dica: caminho do icone deve ser relativo ao executavel.\n");
    }
    FILE* f = fopen(".engine/icon.png", "rb");
    if (f) {
        fclose(f);
        _aplicar_icone_do_arquivo(".engine/icon.png");
    }
}

// --- Criação de janela ---

int tupi_janela_criar(int largura, int altura, const char* titulo,
                      float escala, int sem_borda, const char* icone) {
    glfwSetErrorCallback(_erro_glfw);

    if (!glfwInit()) {
        fprintf(stderr, "[TupiEngine] Falha ao inicializar GLFW!\n");
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

    const char* app_id = getenv("WAYLAND_APP_ID");
    if (!app_id || app_id[0] == '\0') app_id = titulo ? titulo : "TupiEngine";

    glfwWindowHintString(GLFW_X11_CLASS_NAME, app_id);
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, app_id);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    /* GLFW_SAMPLES = 0: desativa o buffer MSAA completamente.
       Pixel art usa GL_NEAREST — MSAA interpola entre texels vizinhos e
       causa scanlines/bleeding visíveis em tela cheia e letterbox.          */
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_DECORATED, sem_borda ? GLFW_FALSE : GLFW_TRUE);

#if defined(GLFW_PLATFORM_X11)
    glfwWindowHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    _janela = glfwCreateWindow(fis_w, fis_h, titulo ? titulo : "TupiEngine", NULL, NULL);
    if (!_janela) {
        fprintf(stderr, "[TupiEngine] Falha ao criar janela!\n");
        glfwTerminate();
        return 0;
    }

    glfwMakeContextCurrent(_janela);
    glfwSwapInterval(1);

    _carregar_icone(icone);
    glfwHideWindow(_janela);
    glfwShowWindow(_janela);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "[TupiEngine] Falha ao inicializar GLAD!\n");
        glfwTerminate();
        return 0;
    }

    printf("[TupiEngine] OpenGL %s\n", glGetString(GL_VERSION));

    glfwSetFramebufferSizeCallback(_janela, _callback_resize);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* Desabilita MSAA explicitamente: mesmo com GLFW_SAMPLES=0 alguns drivers
       Mesa/AMD ativam multisampling por conta propria. glDisable garante que
       pixel art com GL_NEAREST nao sofra interpolacao entre texels.          */
    glDisable(GL_MULTISAMPLE);

    _shader       = _criar_programa(_vert_src, _frag_src);
    _loc_projecao = glGetUniformLocation(_shader, "uProjecao");
    _loc_cor      = glGetUniformLocation(_shader, "uCor");

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * TUPI_MAX_VERTICES, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    tupi_sprite_iniciar();
    tupi_batcher_registrar_flush(_flush_batcher);
    _configurar_projecao(fis_w, fis_h);

    _tempo_anterior = glfwGetTime();
    tupi_input_iniciar(_janela);
    tupi_input_set_dimensoes(&_largura, &_altura, &_logico_w, &_logico_h);

    printf("[TupiEngine] Janela criada: %dx%d px  |  mundo: %dx%d  |  escala: %.2fx  |  '%s'\n",
           fis_w, fis_h, _logico_w, _logico_h, _escala, titulo ? titulo : "TupiEngine");
    printf("[TupiEngine] Bem-vindo a TupiEngine! Versao 0.4\n");

    return 1;
}

// --- Loop principal ---

int tupi_janela_aberta(void) {
    if (!_janela) return 0;
    return !glfwWindowShouldClose(_janela);
}

void tupi_janela_limpar(void) {
    double agora = glfwGetTime();
    _delta = agora - _tempo_anterior;
    _tempo_anterior = agora;

    if (_letterbox_ativo) {
        /* Limpa o framebuffer inteiro com preto (barras) e depois restringe
           o scissor para a região do jogo antes de pintar a cor de fundo.  */
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_SCISSOR_TEST);
        glScissor(_lb_x, _lb_y, _lb_w, _lb_h);
        glClearColor(_fundo_r, _fundo_g, _fundo_b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        glClearColor(_fundo_r, _fundo_g, _fundo_b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    TupiCamera* cam = tupi_camera_ativa();
    if (cam) {
        /* Camera.c já aplica roundf() em _recalcular() — pixel-snapping
           garantido sem necessidade de intervenção aqui.                     */
        tupi_camera_frame(cam, _logico_w, _logico_h);
    } else {
        TupiMatriz proj = tupi_projecao_ortografica(_logico_w, _logico_h);
        glUseProgram(_tupi_shader_get());
        glUniformMatrix4fv(_tupi_loc_projecao_get(), 1, GL_FALSE, proj.m);
        tupi_sprite_set_projecao(proj.m);
    }

    /* Reseta cor do batch para branco no início de cada frame.
       Assim T.cor() feita antes do limpar() não vaza para o frame seguinte. */
    tupi_sprite_reset_cor();
    _cor_r = _cor_g = _cor_b = _cor_a = 1.0f;
}

void tupi_janela_atualizar(void) {
    tupi_batch_desenhar();   /* flush do batch de sprites (texturas) */
    tupi_batcher_flush();    /* flush do batcher de formas 2D        */
    glfwSwapBuffers(_janela);
    tupi_input_salvar_estado();
    glfwPollEvents();
    tupi_input_atualizar_mouse();
}

void tupi_janela_fechar(void) {
    tupi_sprite_encerrar();
    if (_shader) { glDeleteProgram(_shader);       _shader = 0; }
    if (_vbo)    { glDeleteBuffers(1, &_vbo);      _vbo    = 0; }
    if (_vao)    { glDeleteVertexArrays(1, &_vao); _vao    = 0; }
    if (_janela) { glfwDestroyWindow(_janela);     _janela = NULL; }
    glfwTerminate();
    printf("[TupiEngine] Janela fechada. Ate mais!\n");
}

// --- Controles em runtime ---

void tupi_janela_set_titulo(const char* titulo) {
    if (_janela && titulo) glfwSetWindowTitle(_janela, titulo);
}

void tupi_janela_set_decoracao(int ativo) {
    if (!_janela) return;
    glfwSetWindowAttrib(_janela, GLFW_DECORATED, ativo ? GLFW_TRUE : GLFW_FALSE);
    _flag_sem_borda = !ativo;
}

/* ── tupi_janela_tela_cheia ─────────────────────────────────────────────────
   ativo = 1  → entra em tela cheia nativa (estica pra resolução do monitor).
   ativo = 0  → volta para janela.
   Para tela cheia sem distorção, use tupi_janela_tela_cheia_letterbox().
   ────────────────────────────────────────────────────────────────────────── */
void tupi_janela_tela_cheia(int ativo) {
    if (!_janela) return;
    if (ativo) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* modo = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(_janela, monitor, 0, 0,
                             modo->width, modo->height, modo->refreshRate);
    } else {
        _letterbox_ativo = 0;
        glDisable(GL_SCISSOR_TEST);
        glfwSetWindowMonitor(_janela, NULL, 100, 100, _largura, _altura, 0);
    }
}

/* ── tupi_janela_tela_cheia_letterbox ───────────────────────────────────────
   Entra em tela cheia PRESERVANDO o aspect ratio do mundo lógico.
   A cena continua sendo desenhada em _logico_w x _logico_h (ex: 160x144).
   O OpenGL escala por pixel inteiro (nearest) ou suavizado conforme o driver,
   e adiciona barras pretas nas laterais ou topo/base conforme necessário.

   ativo = 1  → tela cheia com letterbox
   ativo = 0  → volta para janela normal sem letterbox
   ────────────────────────────────────────────────────────────────────────── */
void tupi_janela_tela_cheia_letterbox(int ativo) {
    if (!_janela) return;

    if (ativo) {
        _letterbox_ativo = 1;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* modo = glfwGetVideoMode(monitor);

        /* Entra em tela cheia com a resolução nativa do monitor             */
        glfwSetWindowMonitor(_janela, monitor, 0, 0,
                             modo->width, modo->height, modo->refreshRate);

        /* _callback_resize já será disparado, mas chamamos explicitamente
           para garantir que a viewport seja configurada antes do 1º frame.  */
        _calcular_letterbox(modo->width, modo->height);
        _configurar_projecao(modo->width, modo->height);

        printf("[TupiEngine] Letterbox ativo: monitor %dx%d → jogo %dx%d "
               "| viewport (%d,%d) %dx%d\n",
               modo->width, modo->height, _logico_w, _logico_h,
               _lb_x, _lb_y, _lb_w, _lb_h);
    } else {
        _letterbox_ativo = 0;
        glDisable(GL_SCISSOR_TEST);

        /* Volta para janela com o tamanho físico original                   */
        int fis_w = (int)(_logico_w * _escala);
        int fis_h = (int)(_logico_h * _escala);
        glfwSetWindowMonitor(_janela, NULL, 100, 100, fis_w, fis_h, 0);
    }
}

/* Getter: informa se o letterbox está ativo (útil para o lado Lua).         */
int tupi_janela_letterbox_ativo(void) {
    return _letterbox_ativo;
}

float  tupi_janela_escala(void)      { return _escala;   }
double tupi_tempo(void)              { return glfwGetTime(); }
double tupi_delta_tempo(void)        { return _delta;    }
int    tupi_janela_largura(void)     { return _logico_w; }
int    tupi_janela_altura(void)      { return _logico_h; }
int    tupi_janela_largura_px(void)  { return _largura;  }
int    tupi_janela_altura_px(void)   { return _altura;   }

// --- Cor ---

void tupi_cor_fundo(float r, float g, float b) {
    _fundo_r = r; _fundo_g = g; _fundo_b = b;
}

void tupi_cor(float r, float g, float b, float a) {
    _cor_r = r; _cor_g = g; _cor_b = b; _cor_a = a;
    /* Sincroniza com o batch de sprites/texto para que T.cor() funcione
       tanto para primitivos quanto para fontes bitmap e sprites tinturizados. */
    tupi_sprite_set_cor(r, g, b, a);
}

// --- Formas 2D ---

void tupi_retangulo(float x, float y, float largura, float altura) {
    float ts[8] = {
        x,           y,
        x + largura, y,
        x,           y + altura,
        x + largura, y + altura,
    };
    _desenhar(GL_TRIANGLE_STRIP, ts, 4);
}

void tupi_retangulo_borda(float x, float y, float largura, float altura, float espessura) {
    glLineWidth(espessura);
    float v[8] = { x, y, x+largura, y, x+largura, y+altura, x, y+altura };
    _desenhar(GL_LINE_LOOP, v, 4);
    glLineWidth(1.0f);
}

void tupi_triangulo(float x1, float y1, float x2, float y2, float x3, float y3) {
    float v[6] = { x1, y1, x2, y2, x3, y3 };
    _desenhar(GL_TRIANGLES, v, 3);
}

void tupi_circulo(float x, float y, float raio, int segmentos) {
    if (segmentos > TUPI_MAX_VERTICES - 2) segmentos = TUPI_MAX_VERTICES - 2;
    float verts[TUPI_MAX_VERTICES * 2];
    int n = 0;
    verts[n++] = x; verts[n++] = y;
    for (int i = 0; i <= segmentos; i++) {
        float a = (float)i / (float)segmentos * 2.0f * (float)M_PI;
        verts[n++] = x + cosf(a) * raio;
        verts[n++] = y + sinf(a) * raio;
    }
    _desenhar(GL_TRIANGLE_FAN, verts, segmentos + 2);
}

void tupi_circulo_borda(float x, float y, float raio, int segmentos, float espessura) {
    if (segmentos > TUPI_MAX_VERTICES) segmentos = TUPI_MAX_VERTICES;
    float verts[TUPI_MAX_VERTICES * 2];
    int n = 0;
    for (int i = 0; i < segmentos; i++) {
        float a = (float)i / (float)segmentos * 2.0f * (float)M_PI;
        verts[n++] = x + cosf(a) * raio;
        verts[n++] = y + sinf(a) * raio;
    }
    glLineWidth(espessura);
    _desenhar(GL_LINE_LOOP, verts, segmentos);
    glLineWidth(1.0f);
}

void tupi_linha(float x1, float y1, float x2, float y2, float espessura) {
    float v[4] = { x1, y1, x2, y2 };
    glLineWidth(espessura);
    _desenhar(GL_LINES, v, 2);
    glLineWidth(1.0f);
}