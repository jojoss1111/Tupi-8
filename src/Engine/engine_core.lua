local ffi = require("ffi")
local C   = require("src.Engine.engineffi")

local Core = {}

-- ─── JANELA ──────────────────────────────────────────────────────────────────
local Janela = {}

function Janela.janela(largura, altura, titulo, escala, semBorda, imagem)
    largura  = largura  or 800
    altura   = altura   or 600
    titulo   = titulo   or "TupiEngine"
    escala   = escala   or 1.0
    semBorda = semBorda and 1 or 0
    imagem   = imagem   or ""
    if C.tupi_janela_criar(largura, altura, titulo, escala, semBorda, imagem) == 0 then
        error("[TupiEngine] Falha ao criar janela!")
    end
    --- @diagnostic disable-next-line: undefined-global
    if type(tupi_c_aplicar_icone) == "function" then
        --- @diagnostic disable-next-line: undefined-global
        local icon_path = type(TUPI_ICON_PATH) == "string" and TUPI_ICON_PATH or ".engine/icon.png"
        --- @diagnostic disable-next-line: undefined-global
        tupi_c_aplicar_icone(icon_path)
    end
end

function Janela.rodando()    return C.tupi_janela_aberta()          == 1  end
function Janela.limpar()     C.tupi_janela_limpar()                       end
function Janela.atualizar()  C.tupi_janela_atualizar()                    end
Janela.apresentar = Janela.atualizar
function Janela.fechar()     C.tupi_janela_fechar()                       end
function Janela.tempo()      return tonumber(C.tupi_tempo())              end -- segundos desde o início
function Janela.dt()         return tonumber(C.tupi_delta_tempo())        end -- delta time do frame
function Janela.largura()    return tonumber(C.tupi_janela_largura())     end -- tamanho lógico
function Janela.altura()     return tonumber(C.tupi_janela_altura())      end
function Janela.larguraPx()  return tonumber(C.tupi_janela_largura_px())  end -- tamanho real em pixels
function Janela.alturaPx()   return tonumber(C.tupi_janela_altura_px())   end
function Janela.escala()     return tonumber(C.tupi_janela_escala())      end
function Janela.setTitulo(t) C.tupi_janela_set_titulo(t or "")           end
function Janela.setDecoracao(a) C.tupi_janela_set_decoracao(a and 1 or 0) end

-- letterbox=true mantém aspect ratio com barras pretas
function Janela.telaCheia(ativo, letterbox)
    if letterbox then C.tupi_janela_tela_cheia_letterbox(ativo and 1 or 0)
    else              C.tupi_janela_tela_cheia(ativo and 1 or 0) end
end
function Janela.telaCheia_letterbox(a) C.tupi_janela_tela_cheia_letterbox(a and 1 or 0) end
function Janela.letterboxAtivo() return C.tupi_janela_letterbox_ativo() == 1 end

-- Utilitários matemáticos
function Janela.lerp(a, b, t)        return a + (b - a) * t end
function Janela.aleatorio(min, max)  return min + math.random() * (max - min) end
function Janela.rad(graus)           return graus * math.pi / 180 end
function Janela.graus(rad)           return rad * 180 / math.pi end
function Janela.distancia(x1, y1, x2, y2)
    local dx, dy = x2 - x1, y2 - y1
    return math.sqrt(dx * dx + dy * dy)
end

Core.janela = Janela

-- ─── RENDER ──────────────────────────────────────────────────────────────────
local Render = {}

function Render.corFundo(r, g, b)   C.tupi_cor_fundo(r or 0, g or 0, b or 0)         end
function Render.cor(r, g, b, a)     C.tupi_cor(r or 1, g or 1, b or 1, a or 1)       end
function Render.usarCor(tc, a)      C.tupi_cor(tc[1], tc[2], tc[3], a or tc[4] or 1) end

-- Aplica cor e retorna função para resetar para branco
function Render._aplicarCor(cor)
    if cor == nil then return function() end end
    C.tupi_cor(cor[1] or 1, cor[2] or 1, cor[3] or 1, cor[4] or 1)
    return function() C.tupi_cor(1, 1, 1, 1) end
end

function Render.retangulo(x, y, l, a, cor)
    local r = Render._aplicarCor(cor); C.tupi_retangulo(x, y, l, a); r() end
function Render.retanguloBorda(x, y, l, a, esp, cor)
    local r = Render._aplicarCor(cor); C.tupi_retangulo_borda(x, y, l, a, esp or 1); r() end
function Render.triangulo(x1, y1, x2, y2, x3, y3, cor)
    local r = Render._aplicarCor(cor); C.tupi_triangulo(x1, y1, x2, y2, x3, y3); r() end
function Render.circulo(x, y, raio, seg, cor)
    local r = Render._aplicarCor(cor); C.tupi_circulo(x, y, raio, seg or 64); r() end
function Render.circuloBorda(x, y, raio, seg, esp, cor)
    local r = Render._aplicarCor(cor); C.tupi_circulo_borda(x, y, raio, seg or 64, esp or 1); r() end
function Render.linha(x1, y1, x2, y2, esp, cor)
    local r = Render._aplicarCor(cor); C.tupi_linha(x1, y1, x2, y2, esp or 1); r() end

function Render.batchDesenhar() C.tupi_batch_desenhar() end

-- Cores predefinidas {r, g, b, a}
Render.BRANCO   = {1,   1,   1,   1};  Render.PRETO    = {0,   0,   0,   1}
Render.VERMELHO = {1,   0,   0,   1};  Render.VERDE    = {0,   1,   0,   1}
Render.AZUL     = {0,   0,   1,   1};  Render.AMARELO  = {1,   1,   0,   1}
Render.ROXO     = {0.6, 0,   1,   1};  Render.LARANJA  = {1,   0.5, 0,   1}
Render.CIANO    = {0,   1,   1,   1};  Render.CINZA    = {0.5, 0.5, 0.5, 1}
Render.ROSA     = {1,   0.4, 0.7, 1}

Core.render = Render

-- ─── INPUT ───────────────────────────────────────────────────────────────────
local Input = {}

-- Códigos de teclas
Input.TECLA_ESPACO=32;  Input.TECLA_ENTER=257;   Input.TECLA_TAB=258
Input.TECLA_BACKSPACE=259; Input.TECLA_ESC=256
Input.TECLA_SHIFT_ESQ=340; Input.TECLA_SHIFT_DIR=344
Input.TECLA_CTRL_ESQ=341;  Input.TECLA_CTRL_DIR=345
Input.TECLA_ALT_ESQ=342;   Input.TECLA_ALT_DIR=346
Input.TECLA_CIMA=265;  Input.TECLA_BAIXO=264
Input.TECLA_ESQUERDA=263; Input.TECLA_DIREITA=262
Input.TECLA_HOME=268;  Input.TECLA_END=269
Input.TECLA_PGUP=266;  Input.TECLA_PGDN=267
Input.TECLA_DELETE=261; Input.TECLA_INSERT=260
for i, l in ipairs{"A","B","C","D","E","F","G","H","I","J","K","L","M",
                    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"} do
    Input["TECLA_"..l] = 64 + i
end
for i = 0, 9 do Input["TECLA_"..i] = 48 + i; Input["TECLA_NUM"..i] = 320 + i end
for i = 1, 12 do Input["TECLA_F"..i] = 289 + i end
Input.MOUSE_ESQ=0; Input.MOUSE_DIR=1; Input.MOUSE_MEIO=2

local _tecla_tempo_min   = 0.0
local _tecla_tempo_atual = {}

function Input.setTempoSegurando(s) _tecla_tempo_min = math.max(0, s or 0) end
function Input.getTempoSegurando()  return _tecla_tempo_min end

function Input.teclaPressionou(t) return C.tupi_tecla_pressionou(t) == 1 end -- true no frame que pressionou
function Input.teclaSoltou(t)     return C.tupi_tecla_soltou(t)     == 1 end -- true no frame que soltou

-- true enquanto segurada; respeita o tempo mínimo configurado
function Input.teclaSegurando(t)
    if C.tupi_tecla_segurando(t) ~= 1 then _tecla_tempo_atual[t] = nil; return false end
    if _tecla_tempo_min <= 0 then return true end
    local dt = Input._getDt and Input._getDt() or 0
    local acum = (_tecla_tempo_atual[t] or 0) + dt
    _tecla_tempo_atual[t] = acum
    return acum >= _tecla_tempo_min
end

function Input.mouseX()   return tonumber(C.tupi_mouse_mundo_x())  end -- coordenada no mundo
function Input.mouseY()   return tonumber(C.tupi_mouse_mundo_y())  end
function Input.mouseDX()  return tonumber(C.tupi_mouse_mundo_dx()) end -- delta do frame
function Input.mouseDY()  return tonumber(C.tupi_mouse_mundo_dy()) end
function Input.mousePos() return tonumber(C.tupi_mouse_mundo_x()), tonumber(C.tupi_mouse_mundo_y()) end
function Input.mouseXRaw() return tonumber(C.tupi_mouse_x()) end -- coordenada de tela (pixels)
function Input.mouseYRaw() return tonumber(C.tupi_mouse_y()) end
function Input.mouseClicou(b)    return C.tupi_mouse_clicou(b    or 0) == 1 end
function Input.mouseSegurando(b) return C.tupi_mouse_segurando(b or 0) == 1 end
function Input.mouseSoltou(b)    return C.tupi_mouse_soltou(b    or 0) == 1 end
function Input.scrollX() return tonumber(C.tupi_scroll_x()) end
function Input.scrollY() return tonumber(C.tupi_scroll_y()) end

Core.input = Input

-- ─── COLISÃO ─────────────────────────────────────────────────────────────────
local Colisao = {}

local function _ret(t) return ffi.new("TupiRetCol",  t.x, t.y, t.largura, t.altura) end
local function _cir(t) return ffi.new("TupiCircCol", t.x, t.y, t.raio)              end
-- Converte resultado C para tabela Lua
local function _info(c)
    return { colidindo = c.colidindo == 1, dx = tonumber(c.dx), dy = tonumber(c.dy) }
end

function Colisao.retRet(a, b)        return C.tupi_ret_ret(_ret(a), _ret(b)) == 1            end
function Colisao.retRetInfo(a, b)    return _info(C.tupi_ret_ret_info(_ret(a), _ret(b)))      end
function Colisao.cirCir(a, b)        return C.tupi_cir_cir(_cir(a), _cir(b)) == 1            end
function Colisao.cirCirInfo(a, b)    return _info(C.tupi_cir_cir_info(_cir(a), _cir(b)))      end
function Colisao.retCir(r, c)        return C.tupi_ret_cir(_ret(r), _cir(c)) == 1            end
function Colisao.pontoRet(px, py, r) return C.tupi_ponto_ret(px, py, _ret(r)) == 1           end
function Colisao.pontoCir(px, py, c) return C.tupi_ponto_cir(px, py, _cir(c)) == 1           end

function Colisao.mouseNoRet(r)
    local mx, my = Colisao._getMouse and Colisao._getMouse() or 0, 0
    return Colisao.pontoRet(mx, my, r)
end
function Colisao.mouseNoCir(c)
    local mx, my = Colisao._getMouse and Colisao._getMouse() or 0, 0
    return Colisao.pontoCir(mx, my, c)
end

Core.colisao = Colisao

-- ─── FÍSICA ──────────────────────────────────────────────────────────────────
local Fisica = {}

-- Corpo dinâmico com massa, elasticidade e atrito
function Fisica.corpo(x, y, massa, elasticidade, atrito)
    local c = ffi.new("TupiCorpo[1]")
    c[0].x = x or 0; c[0].y = y or 0
    c[0].velX = 0; c[0].velY = 0
    c[0].aceleracaoX = 0; c[0].aceleracaoY = 0
    c[0].massa        = massa        or 1.0
    c[0].elasticidade = elasticidade or 0.3
    c[0].atrito       = atrito       or 0.1
    return c
end

-- Corpo imóvel (massa=0)
function Fisica.corpoEstatico(x, y)
    local c = ffi.new("TupiCorpo[1]")
    c[0].x = x or 0; c[0].y = y or 0
    c[0].velX = 0; c[0].velY = 0
    c[0].aceleracaoX = 0; c[0].aceleracaoY = 0
    c[0].massa = 0.0; c[0].elasticidade = 0.0; c[0].atrito = 1.0
    return c
end

function Fisica.atualizar(corpo, gravidade)
    local dt = Fisica._getDt and Fisica._getDt() or 0
    C.tupi_fisica_atualizar(corpo, dt, gravidade or 500.0)
end

function Fisica.impulso(corpo, fx, fy)    C.tupi_fisica_impulso(corpo, fx or 0, fy or 0) end
function Fisica.atrito(corpo)
    local dt = Fisica._getDt and Fisica._getDt() or 0
    C.tupi_aplicar_atrito(corpo, dt)
end
function Fisica.limitarVel(corpo, maxVel) C.tupi_limitar_velocidade(corpo, maxVel or 800.0) end
function Fisica.pos(corpo)  return tonumber(corpo[0].x),    tonumber(corpo[0].y)    end
function Fisica.vel(corpo)  return tonumber(corpo[0].velX), tonumber(corpo[0].velY) end
function Fisica.setPosicao(corpo, x, y) corpo[0].x = x or corpo[0].x; corpo[0].y = y or corpo[0].y end
function Fisica.setVel(corpo, vx, vy)  corpo[0].velX = vx or 0; corpo[0].velY = vy or 0 end

-- Hitbox a partir da posição do corpo
function Fisica.retCol(corpo, largura, altura)
    local r = C.tupi_corpo_ret(corpo, largura or 0, altura or 0)
    return { x=tonumber(r.x), y=tonumber(r.y), largura=tonumber(r.largura), altura=tonumber(r.altura) }
end
function Fisica.cirCol(corpo, raio)
    local c = C.tupi_corpo_cir(corpo, raio or 0)
    return { x=tonumber(c.x), y=tonumber(c.y), raio=tonumber(c.raio) }
end

-- Resolve colisão entre dois dinâmicos ou entre corpo e estático
function Fisica.resolverColisao(a, b, info)
    C.tupi_resolver_colisao(a, b, ffi.new("TupiColisao", info.colidindo and 1 or 0, info.dx or 0, info.dy or 0))
end
function Fisica.resolverEstatico(corpo, info)
    C.tupi_resolver_estatico(corpo, ffi.new("TupiColisao", info.colidindo and 1 or 0, info.dx or 0, info.dy or 0))
end

-- Copia posição do corpo para o sprite wrapper
function Fisica.sincronizar(wrapper, corpo)
    wrapper.obj[0].x = corpo[0].x
    wrapper.obj[0].y = corpo[0].y
end

Core.fisica = Fisica

return Core