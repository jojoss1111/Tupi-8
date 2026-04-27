local ffi = require("ffi")

local function resolverRaizProjeto()
   local source = debug.getinfo(1, "S").source
   if type(source) ~= "string" or source:sub(1, 1) ~= "@" then
      return "."
   end

   local arquivo = source:sub(2)
   local dirAtual = arquivo:match("^(.*)[/\\][^/\\]+$") or "."
   return dirAtual .. "/../.."
end

ffi.cdef[[
   int    tupi_janela_criar(int largura, int altura, const char* titulo,
                            float escala, int sem_borda, const char* icone);

   void   tupi_janela_set_titulo(const char* titulo);
   void   tupi_janela_set_decoracao(int ativo);
   void   tupi_janela_tela_cheia(int ativo);

   /* Tela cheia sem distorção: mantém aspect ratio do mundo lógico e
      adiciona barras pretas (letterbox/pillarbox) nas bordas.               */
   void   tupi_janela_tela_cheia_letterbox(int ativo);
   int    tupi_janela_letterbox_ativo(void);

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

   void tupi_cor_fundo(float r, float g, float b);
   void tupi_cor(float r, float g, float b, float a);

   void tupi_retangulo(float x, float y, float largura, float altura);
   void tupi_retangulo_borda(float x, float y, float largura, float altura, float espessura);
   void tupi_triangulo(float x1, float y1, float x2, float y2, float x3, float y3);
   void tupi_circulo(float x, float y, float raio, int segmentos);
   void tupi_circulo_borda(float x, float y, float raio, int segmentos, float espessura);
   void tupi_linha(float x1, float y1, float x2, float y2, float espessura);

   int tupi_tecla_pressionou(int tecla);
   int tupi_tecla_segurando(int tecla);
   int tupi_tecla_soltou(int tecla);

   double tupi_mouse_x(void);
   double tupi_mouse_y(void);
   double tupi_mouse_dx(void);
   double tupi_mouse_dy(void);

   double tupi_mouse_mundo_x(void);
   double tupi_mouse_mundo_y(void);
   double tupi_mouse_mundo_dx(void);
   double tupi_mouse_mundo_dy(void);

   int tupi_mouse_clicou(int botao);
   int tupi_mouse_segurando(int botao);
   int tupi_mouse_soltou(int botao);

   double tupi_scroll_x(void);
   double tupi_scroll_y(void);

   typedef struct { float x, y, largura, altura; } TupiRetCol;
   typedef struct { float x, y, raio;             } TupiCircCol;
   typedef struct { int colidindo; float dx, dy;  } TupiColisao;

   int         tupi_ret_ret     (TupiRetCol a, TupiRetCol b);
   TupiColisao tupi_ret_ret_info(TupiRetCol a, TupiRetCol b);
   int         tupi_cir_cir     (TupiCircCol a, TupiCircCol b);
   TupiColisao tupi_cir_cir_info(TupiCircCol a, TupiCircCol b);
   int         tupi_ret_cir     (TupiRetCol r, TupiCircCol c);
   int         tupi_ponto_ret   (float px, float py, TupiRetCol r);
   int         tupi_ponto_cir   (float px, float py, TupiCircCol c);

   typedef struct {
      unsigned int textura;
      int largura;
      int altura;
   } TupiSprite;

   typedef struct {
      float x, y;
      float largura, altura;
      int   coluna, linha;
      float transparencia;
      float escala;
      TupiSprite* imagem;
   } TupiObjeto;

   TupiSprite* tupi_sprite_carregar(const char* caminho);
   void        tupi_sprite_destruir(TupiSprite* sprite);

   TupiObjeto tupi_objeto_criar(
      float x, float y,
      float largura, float altura,
      int coluna, int linha,
      float transparencia,
      float escala,
      TupiSprite* imagem
   );

   void tupi_objeto_desenhar(TupiObjeto* obj);
   void tupi_objeto_enviar_batch(TupiObjeto* obj, int z_index);
   void tupi_objeto_enviar_batch_raw(
       unsigned int textura_id,
       const float* quad,
       float        r,
       float        g,
       float        b,
       float        a,
       int          z_index
   );
   void tupi_batch_desenhar(void);

   typedef struct {
      TupiObjeto* obj;
      int z_index;
   } TupiDrawCall;

   void tupi_batcher_adicionar(TupiDrawCall dc);
   void tupi_batcher_adicionar_z(TupiDrawCall dc, int z);
   void tupi_batcher_flush(void);
   int  tupi_batcher_tamanho(void);

   /* ──────────────────────────────────────────────
      CÂMERA — sistema baseado em objeto
      ────────────────────────────────────────────── */

   typedef struct {
      float alvo_x;
      float alvo_y;
      float ancora_x;   /* -1 = centro da janela */
      float ancora_y;   /* -1 = centro da janela */
      float zoom;
      float rotacao;
      float _cam_x;     /* interno — não editar */
      float _cam_y;     /* interno — não editar */
      int   _ativo;     /* interno — não editar */
   } TupiCamera;

   /* Ciclo de vida */
   TupiCamera  tupi_camera_criar   (float alvo_x, float alvo_y,
                                    float ancora_x, float ancora_y);
   void        tupi_camera_destruir(TupiCamera* cam);

   /* Renderer — chame uma vez por frame com a câmera ativa */
   void        tupi_camera_frame   (TupiCamera* cam, int largura, int altura);
   void        tupi_camera_ativar  (TupiCamera* cam);
   TupiCamera* tupi_camera_ativa   (void);

   /* Movimentação */
   void  tupi_camera_pos     (TupiCamera* cam, float x, float y);
   void  tupi_camera_mover   (TupiCamera* cam, float dx, float dy);
   void  tupi_camera_zoom    (TupiCamera* cam, float z);
   void  tupi_camera_rotacao (TupiCamera* cam, float angulo);
   void  tupi_camera_ancora  (TupiCamera* cam, float ax, float ay);
   void  tupi_camera_seguir  (TupiCamera* cam,
                               float alvo_x, float alvo_y,
                               float lerp_fator, float dt);

   /* Getters */
   float tupi_camera_get_x      (const TupiCamera* cam);
   float tupi_camera_get_y      (const TupiCamera* cam);
   float tupi_camera_get_zoom   (const TupiCamera* cam);
   float tupi_camera_get_rotacao(const TupiCamera* cam);

   /* Conversão de coordenadas (wrappers Lua usam câmera ativa) */
   void  tupi_camera_tela_mundo_lua(float sx, float sy, float* wx, float* wy);
   void  tupi_camera_mundo_tela_lua(float wx, float wy, float* sx, float* sy);

   /* Validação (Rust) */
   int   tupi_camera_validar(float x, float y, float zoom, float rotacao);

   /* ──────────────────────────────────────────────
      PARALAXE
      ────────────────────────────────────────────── */

   typedef struct {
      float offset_x;
      float offset_y;
      int   z_layer;
      int   valido;
   } TupiParalaxOffset;

   int               tupi_paralax_registrar(
                        float fator_x, float fator_y,
                        int   z_layer,
                        float largura_loop, float altura_loop);
   int               tupi_paralax_remover        (int id);
   void              tupi_paralax_atualizar       (float cam_x, float cam_y);
   TupiParalaxOffset tupi_paralax_offset          (int id);
   void              tupi_paralax_resetar         (void);
   int               tupi_paralax_resetar_camada  (int id);
   int               tupi_paralax_set_fator       (int id, float fator_x, float fator_y);
   int               tupi_paralax_total_ativas    (void);

   /* ──────────────────────────────────────────────
      FÍSICA
      ────────────────────────────────────────────── */

   typedef struct {
      float x, y;
      float velX, velY;
      float aceleracaoX, aceleracaoY;
      float massa;
      float elasticidade;
      float atrito;
   } TupiCorpo;

   void tupi_fisica_atualizar(TupiCorpo* c, float dt, float gravidade);
   void tupi_fisica_impulso  (TupiCorpo* c, float fx, float fy);

   TupiRetCol  tupi_corpo_ret(TupiCorpo* c, float largura, float altura);
   TupiCircCol tupi_corpo_cir(TupiCorpo* c, float raio);

   void tupi_resolver_colisao (TupiCorpo* a, TupiCorpo* b, TupiColisao info);
   void tupi_resolver_estatico(TupiCorpo* a, TupiColisao info);

   void tupi_aplicar_atrito    (TupiCorpo* c, float dt);
   void tupi_limitar_velocidade(TupiCorpo* c, float maxVel);
]]

local tupiC
--- @diagnostic disable-next-line: undefined-global
if TUPI_STANDALONE then
    tupiC = ffi.C
else
    local lib_path = resolverRaizProjeto() .. "/libtupi.so"
    local ok, result = pcall(ffi.load, lib_path)
    if not ok then
        error(
            "[TupiEngine] Não foi possível carregar '" .. lib_path .. "'.\n" ..
            "  → Rode 'make gl' para gerar a biblioteca antes de 'luajit main.lua'.\n" ..
            "  → Para distribuir sem dependências externas, use 'make dist-linux'.\n" ..
            "  Detalhe: " .. tostring(result)
        )
    end
    tupiC = result
end

return tupiC
