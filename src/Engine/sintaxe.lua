---@diagnostic disable: undefined-global
local Core   = require("src.Engine.engine_core")
local Visual = require("src.Engine.engine_visual")
local Mundo  = require("src.Engine.engine_mundo")
local KB     = require("src.Engine.tupi_teclado")
local Norm   = require("src.Engine.texto_normalizar")

local Tupi = {}

-- atalhos internos para os submódulos
local J   = Core.janela
local R   = Core.render
local I   = Core.input
local Col = Core.colisao
local Fis = Core.fisica
local Spr = Visual.sprite
local Cam = Visual.camera.camera
local Par = Visual.camera.paralax
local Txt = Visual.texto

-- injeta dt e mouse nos submódulos que precisam
I._getDt              = function() return J.dt() end
Fis._getDt            = function() return J.dt() end
Spr._getDt            = function() return J.dt() end
Col._getMouse         = function() return I.mouseX(), I.mouseY() end
Visual.camera._getDt      = function() return J.dt() end
Visual.camera._getMouse   = function() return I.mouseX(), I.mouseY() end
Visual.camera._getLargura = function() return J.largura() end
Spr._retRet     = function(a, b) return Col.retRet(a, b) end
Spr._retRetInfo = function(a, b) return Col.retRetInfo(a, b) end
Spr._aplicarCor = R._aplicarCor

-- ─── JANELA ──────────────────────────────────────────────────────────────────

function Tupi.janela(largura, altura, titulo, escala, semBorda, imagem)
    J.janela(largura, altura, titulo, escala, semBorda, imagem)
end

function Tupi.rodando()           return J.rodando()          end
function Tupi.limparTela()        J.limpar()                   end
function Tupi.atualizar()         J.atualizar()                end
function Tupi.fechar()            J.fechar()                   end
function Tupi.tempo()             return J.tempo()             end
function Tupi.dt()                return J.dt()                end
function Tupi.largura()           return J.largura()           end
function Tupi.altura()            return J.altura()            end
function Tupi.larguraPx()         return J.larguraPx()         end
function Tupi.alturaPx()          return J.alturaPx()          end
function Tupi.escalaJanela()      return J.escala()            end
function Tupi.titulo(t)           J.setTitulo(t)               end
function Tupi.decoracao(a)        J.setDecoracao(a)            end
function Tupi.telaCheia(a, lb)    J.telaCheia(a, lb)           end
function Tupi.letterboxAtivo()    return J.letterboxAtivo()    end
function Tupi.fpsLimite(n)        if Visual.fpsLimite then Visual.fpsLimite(n) end  end
function Tupi.fpsAtual()          return Visual.fpsAtual and Visual.fpsAtual() or 0 end

-- ─── RENDER / FORMAS ─────────────────────────────────────────────────────────

function Tupi.corFundo(r, g, b)   R.corFundo(r, g, b)  end

-- define cor para formas e texto ao mesmo tempo
function Tupi.cor(r, g, b, a)
    R.cor(r, g, b, a)
    Txt.setCor(r, g, b, a)
end

-- aceita tabela {r,g,b,a} e aplica em formas e texto
function Tupi.usarCor(tc, a)
    local alpha = a or (tc and tc[4]) or 1.0
    if tc then Txt.setCor(tc[1], tc[2], tc[3], alpha) end
    R.usarCor(tc, a)
end

function Tupi.retangulo(x, y, l, a, cor)             R.retangulo(x, y, l, a, cor)              end
function Tupi.bordaRet(x, y, l, a, esp, cor)          R.retanguloBorda(x, y, l, a, esp, cor)    end
function Tupi.triangulo(x1,y1,x2,y2,x3,y3,cor)       R.triangulo(x1,y1,x2,y2,x3,y3,cor)       end
function Tupi.circulo(x, y, raio, seg, cor)           R.circulo(x, y, raio, seg, cor)           end
function Tupi.bordaCirc(x, y, raio, seg, esp, cor)    R.circuloBorda(x, y, raio, seg, esp, cor) end
function Tupi.linha(x1, y1, x2, y2, esp, cor)         R.linha(x1, y1, x2, y2, esp, cor)        end
function Tupi.pixel(x, y, cor)                        R.retangulo(x, y, 1, 1, cor)              end
function Tupi.flush()                                 R.batchDesenhar()                         end

-- cores predefinidas como tabelas {r,g,b,a}
Tupi.BRANCO   = R.BRANCO;   Tupi.PRETO    = R.PRETO
Tupi.VERMELHO = R.VERMELHO; Tupi.VERDE    = R.VERDE
Tupi.AZUL     = R.AZUL;     Tupi.AMARELO  = R.AMARELO
Tupi.ROXO     = R.ROXO;     Tupi.LARANJA  = R.LARANJA
Tupi.CIANO    = R.CIANO;    Tupi.CINZA    = R.CINZA
Tupi.ROSA     = R.ROSA

-- paleta estilo PICO-8, índices 0-15
Tupi.PALETA = {
    [0]={0.00,0.00,0.00,1.0}, [1]={0.11,0.17,0.33,1.0},
    [2]={0.49,0.15,0.32,1.0}, [3]={0.00,0.53,0.33,1.0},
    [4]={0.67,0.32,0.21,1.0}, [5]={0.37,0.34,0.31,1.0},
    [6]={0.76,0.76,0.76,1.0}, [7]={1.00,0.95,0.91,1.0},
    [8]={1.00,0.00,0.30,1.0}, [9]={1.00,0.64,0.00,1.0},
    [10]={1.00,0.93,0.15,1.0},[11]={0.00,0.89,0.21,1.0},
    [12]={0.16,0.68,1.00,1.0},[13]={0.51,0.46,0.86,1.0},
    [14]={1.00,0.47,0.66,1.0},[15]={1.00,0.80,0.67,1.0},
}

-- ─── TEXTO ───────────────────────────────────────────────────────────────────

function Tupi.carregarFonte(caminho, larg, alt, colunas, charInicio)
    return Txt.carregarFonte(caminho, larg or 8, alt or 8, colunas, charInicio)
end
function Tupi.destruirFonte(fonte)        Txt.destruirFonte(fonte)            end
function Tupi.setFontePadrao(fonte)       Txt.setFontePadrao(fonte)           end
function Tupi.getFontePadrao()            return Txt.getFontePadrao()         end
function Tupi.setCorTexto(r, g, b, a)     Txt.setCor(r, g, b, a)             end

-- aceita cor como {r,g,b,a}, índice de paleta (0-15) ou nil (retorna branco)
local function _resolverCorTexto(cor)
    if type(cor) == "table" then
        return cor[1] or 1, cor[2] or 1, cor[3] or 1, cor[4] or 1
    end
    if type(cor) == "number" then
        local p = Tupi.PALETA[cor]
        if p then return p[1], p[2], p[3], p[4] or 1 end
    end
    return 1, 1, 1, 1
end

-- escrever(texto, x, y [, z [, escala [, transp [, cor [, fonte]]]]]])
function Tupi.escrever(texto, x, y, z, escala, transp, cor, fonte)
    if not fonte and not Txt.getFontePadrao() then
        error("[Tupi] Nenhuma fonte carregada.\n"..
              "  Use Tupi.carregarFonte() e Tupi.setFontePadrao() antes de escrever,\n"..
              "  ou coloque uma fonte em '.engine/font.png' para carregamento automático.", 2)
    end
    if cor ~= nil then
        Txt.setCor(_resolverCorTexto(cor))
    end
    Txt.desenhar(x or 0, y or 0, z or 10, Norm.limpar(tostring(texto)), escala or 1.0, transp or 1.0, fonte)
    if cor ~= nil then
        Txt.setCor(1, 1, 1, 1) -- restaura cor branca após desenhar
    end
end

-- texto com sombra: escreverSombra(texto, x, y [, z [, dX [, dY [, escala [, transp [, escS [, transpS [, fonte]]]]]]]])
function Tupi.escreverSombra(texto, x, y, z, dX, dY, escala, transp, escS, transpS, fonte)
    Txt.desenharSombra(x or 0, y or 0, z or 10, dX, dY, Norm.limpar(tostring(texto)), escala, transp, escS, transpS, fonte)
end

-- texto dentro de uma caixa com borda: escreverCaixa(texto, x, y, z, larg, alt, escala, transp, fonte, frame, tamTile, escBorda, transpBorda, recuo)
function Tupi.escreverCaixa(texto, x, y, z, larg, alt, escala, transp, fonte, frame, tamTile, escB, transpB, recuo)
    Txt.desenharCaixa(x or 0, y or 0, z or 10, larg, alt, Norm.limpar(tostring(texto)), escala, transp, fonte, frame, tamTile, escB, transpB, recuo)
end

function Tupi.larguraTexto(fonte, texto, escala)    return Txt.largura(fonte, Norm.limpar(tostring(texto)), escala)   end
function Tupi.alturaTexto(fonte, texto, escala)     return Txt.altura(fonte, Norm.limpar(tostring(texto)), escala)    end
function Tupi.dimensoesTexto(fonte, texto, escala)  return Txt.dimensoes(fonte, Norm.limpar(tostring(texto)), escala) end

-- ─── SPRITES E OBJETOS ───────────────────────────────────────────────────────

function Tupi.imagem(caminho)              return Spr.carregarSprite(caminho)       end
function Tupi.destruirImagem(spr)          Spr.destruirSprite(spr)                  end

-- cria um objeto a partir de um sprite
-- opt: {larg, alt, z, col, lin, alfa/transparencia, escala}
function Tupi.objeto(sprite, x, y, opt)
    opt = opt or {}
    return Spr.criarObjeto(
        x or 0, y or 0,
        opt.z   or 0,
        opt.larg or opt.largura or 16,
        opt.alt  or opt.altura  or 16,
        opt.col  or opt.coluna  or 0,
        opt.lin  or opt.linha   or 0,
        opt.alfa or opt.transparencia or 1.0,
        opt.escala or 1.0,
        sprite
    )
end

function Tupi.mostrar(wrapper, z)          Spr.enviarBatch(wrapper, z)              end
function Tupi.desenharSprite(sprite, x, y, opt)
    local w = Tupi.objeto(sprite, x, y, opt)
    Spr.enviarBatch(w, opt and opt.z or 0)
end

-- transformações de objeto
function Tupi.mover(obj, dx, dy)           Spr.mover(dx, dy, obj)                  end
function Tupi.posicionar(obj, x, y)        Spr.teleportar(x, y, obj)               end
function Tupi.posicao(obj)                 return Spr.posicaoAtual(obj)             end
function Tupi.salvarPosicao(obj)           Spr.salvarPosicao(obj)                  end
function Tupi.ultimaPosicao(obj)           return Spr.ultimaPosicao(obj)            end
function Tupi.voltarPosicao(obj)           Spr.voltarPosicao(obj)                  end
function Tupi.distanciaObjetos(a, b)       return Spr.distanciaObjetos(a, b)        end
function Tupi.moverParaAlvo(obj, tx, ty, f) Spr.moverParaAlvo(tx, ty, f, obj)      end
function Tupi.perseguir(obj, alvo, vel)    Spr.perseguir(alvo, vel, obj)            end
function Tupi.moverComColisao(obj, dx, dy, ox, oy, w, h, hbB)
    return Spr.moverComColisao(dx, dy, obj, ox, oy, w, h, hbB)
end
function Tupi.escalaObj(obj, s)            obj.obj[0].escala        = s or 1.0      end
function Tupi.alfa(obj, a)                 obj.obj[0].transparencia = a or 1.0      end
function Tupi.tamanho(obj, l, a)
    if l then obj.obj[0].largura = l end
    if a then obj.obj[0].altura  = a end
end
function Tupi.quadro(obj, col, lin)
    obj.obj[0].coluna = col or 0
    obj.obj[0].linha  = lin or 0
end
function Tupi.espelhar(obj, h, v)          Spr.espelhar(obj, v, h)                 end
function Tupi.getEspelho(obj)              return Spr.getEspelho(obj)               end
function Tupi.setCor(r, g, b, a)           Spr.setCor(r, g, b, a)                  end
function Tupi.resetCor()                   Spr.resetCor()                           end
function Tupi.destruir(obj, liberarSpr)    Spr.destruir(obj, liberarSpr)           end
function Tupi.destruido(obj)               return Spr.destruido(obj)                end

-- hitbox relativa ao objeto (escala opcional)
function Tupi.hitbox(obj, x, y, larg, alt, escalar)
    return Spr.hitbox(obj, x or 0, y or 0, larg or obj.obj[0].largura, alt or obj.obj[0].altura, escalar)
end
-- hitbox com posição fixa no mundo
function Tupi.hitboxFixa(obj, x, y, larg, alt)
    return Spr.hitboxFixa(obj, x or 0, y or 0, larg or obj.obj[0].largura, alt or obj.obj[0].altura)
end
function Tupi.hitboxDesenhar(hb, cor, esp)           Spr.hitboxDesenhar(hb, cor, esp)         end
function Tupi.resolverColisaoSolida(hbA, hbB, obj)   return Spr.resolverColisaoSolida(hbA, hbB, obj) end

-- ─── ANIMAÇÃO ────────────────────────────────────────────────────────────────

-- criarAnim(sprite, larg, alt, colunas, linhas, fps [, loop])
-- colunas e linhas são arrays de índices: ex. {0,1,2}
function Tupi.criarAnim(sprite, larg, alt, colunas, linhas, fps, loop)
    return Spr.criarAnim(sprite, larg, alt, colunas, linhas, fps, loop)
end
-- atualiza frame e envia ao batch
function Tupi.tocarAnim(anim, obj, z)         Spr.tocarAnim(anim, obj, z)         end
-- congela a animação; frame aceita número 0-based ou {col,lin}
function Tupi.pararAnim(anim, obj, frame, z)   Spr.pararAnim(anim, obj, frame, z) end
function Tupi.animTerminou(anim, obj)          return Spr.animTerminou(anim, obj)  end
function Tupi.animReiniciar(anim, obj)         Spr.animReiniciar(anim, obj)        end
function Tupi.animLimpar(obj)                  Spr.animLimparObjeto(obj)           end

-- ─── CÂMERA ──────────────────────────────────────────────────────────────────

function Tupi.criarCamera(ax, ay, anc_x, anc_y)   return Cam.criar(ax, ay, anc_x, anc_y)  end
function Tupi.destruirCamera(cam)                  Cam.destruir(cam)                        end
function Tupi.ativarCamera(cam)                    Cam.ativar(cam)                          end
function Tupi.cameraPosicao(cam, x, y)             Cam.pos(cam, x, y)                       end
function Tupi.cameraMover(cam, dx, dy)             Cam.mover(cam, dx, dy)                   end
function Tupi.cameraZoom(cam, z)                   Cam.zoom(cam, z)                         end
function Tupi.cameraRotacao(cam, a)                Cam.rotacao(cam, a)                      end
function Tupi.cameraAncora(cam, ax, ay)            Cam.ancora(cam, ax, ay)                  end
function Tupi.cameraSeguir(cam, x, y, vel)         Cam.seguir(cam, x, y, vel)               end
function Tupi.cameraPosAtual(cam)                  return Cam.posAtual(cam)                 end
function Tupi.cameraAlvo(cam)                      return Cam.alvoAtual(cam)                end
function Tupi.cameraZoomAtual(cam)                 return Cam.zoomAtual(cam)                end
function Tupi.cameraRotacaoAtual(cam)              return Cam.rotacaoAtual(cam)             end
function Tupi.cameraTelaMundo(cam, sx, sy)         return Cam.telaMundo(cam, sx, sy)        end
function Tupi.cameraMundoTela(cam, wx, wy)         return Cam.mundoTela(cam, wx, wy)        end
function Tupi.cameraMouseMundo(cam)                return Cam.mouseMundo(cam)               end

-- ─── PARALAXE ────────────────────────────────────────────────────────────────

function Tupi.registrarParalax(fx, fy, z, ll, al)  return Par.registrar(fx, fy, z, ll, al)  end
function Tupi.removerParalax(id)                    Par.remover(id)                           end
function Tupi.resetarParalax()                      Par.resetar()                             end
function Tupi.resetarCamadaParalax(id)              Par.resetarCamada(id)                     end
function Tupi.setFatorParalax(id, fx, fy)           Par.setFator(id, fx, fy)                  end
function Tupi.totalParalax()                        return Par.totalAtivas()                  end
function Tupi.atualizarParalax(cam, cx, cy)         Par.atualizar(cam, cx, cy)                end
function Tupi.offsetParalax(id)                     return Par.offset(id)                     end
function Tupi.desenharParalax(id, wrapper)          Par.desenhar(id, wrapper)                 end
function Tupi.desenharParalaxTile(id, wrapper, lt)  Par.desenharTile(id, wrapper, lt)         end

-- ─── INPUT — TECLADO ─────────────────────────────────────────────────────────

-- expõe Tupi.TECLA_A, Tupi.TECLA_CIMA etc.
for k, v in pairs(I) do
    if type(k) == "string" and k:sub(1,5) == "TECLA" then Tupi[k] = v end
end
Tupi.MOUSE_ESQ  = I.MOUSE_ESQ
Tupi.MOUSE_DIR  = I.MOUSE_DIR
Tupi.MOUSE_MEIO = I.MOUSE_MEIO

-- mapeamento estilo PICO-8: 0=esq 1=dir 2=cima 3=baixo 4=Z 5=X
local _BTN = {
    [0]="TECLA_ESQUERDA",[1]="TECLA_DIREITA",
    [2]="TECLA_CIMA",    [3]="TECLA_BAIXO",
    [4]="TECLA_Z",       [5]="TECLA_X",
}

-- ─── SISTEMA DE COMBINAÇÃO DE TECLAS ─────────────────────────────────────────
-- aceita nome simples, combo "ctrl+a" / "shift+f5" / "ctrl+shift+s" etc.

local _ALIAS = {
    -- modificadores
    ctrl       = "TECLA_CTRL_ESQ",  control    = "TECLA_CTRL_ESQ",
    ctrl_esq   = "TECLA_CTRL_ESQ",  ctrl_dir   = "TECLA_CTRL_DIR",
    shift      = "TECLA_SHIFT_ESQ", shift_esq  = "TECLA_SHIFT_ESQ",
    shift_dir  = "TECLA_SHIFT_DIR",
    alt        = "TECLA_ALT_ESQ",   alt_esq    = "TECLA_ALT_ESQ",
    alt_dir    = "TECLA_ALT_DIR",
    -- navegação
    cima       = "TECLA_CIMA",      up         = "TECLA_CIMA",
    baixo      = "TECLA_BAIXO",     down       = "TECLA_BAIXO",
    esquerda   = "TECLA_ESQUERDA",  left       = "TECLA_ESQUERDA",
    direita    = "TECLA_DIREITA",   right      = "TECLA_DIREITA",
    home       = "TECLA_HOME",      end_       = "TECLA_END",
    fim        = "TECLA_END",
    pgup       = "TECLA_PGUP",      pgdn       = "TECLA_PGDN",
    insert     = "TECLA_INSERT",    ins        = "TECLA_INSERT",
    delete     = "TECLA_DELETE",    del        = "TECLA_DELETE",
    -- especiais
    espaco     = "TECLA_ESPACO",    space      = "TECLA_ESPACO",
    enter      = "TECLA_ENTER",     return_    = "TECLA_ENTER",
    tab        = "TECLA_TAB",
    backspace  = "TECLA_BACKSPACE", bs         = "TECLA_BACKSPACE",
    esc        = "TECLA_ESC",       escape     = "TECLA_ESC",
}

-- resolve um token de combo ("ctrl", "a", "f5"…) para código numérico
local function _resolverParte(parte)
    local p = parte:lower()
    if _ALIAS[p] then return I[_ALIAS[p]] end
    -- teclas de função f1..f12
    local fn = p:match("^f(%d+)$")
    if fn then
        local n = tonumber(fn)
        if n and n >= 1 and n <= 12 then
            return I["TECLA_F"..n]
        end
    end
    -- letra única A-Z
    if #p == 1 and p:match("^[a-z]$") then
        return I["TECLA_"..p:upper()]
    end
    -- dígito 0-9
    if #p == 1 and p:match("^%d$") then
        return I["TECLA_"..p]
    end
    -- numpad: num0..num9
    local np = p:match("^num(%d)$")
    if np then return I["TECLA_NUM"..np] end
    -- constante literal TECLA_*
    local raw = I[p:upper()]
    if raw then return raw end
    return nil
end

-- divide "ctrl+a" em partes e retorna tabela de códigos, ou nil se inválido
local function _resolverCombo(combo)
    if type(combo) == "number" then return {combo} end
    if type(combo) ~= "string" then return nil end
    local partes = {}
    for parte in (combo.."+"):gmatch("([^+]+)%+") do
        local cod = _resolverParte(parte)
        if not cod then return nil end
        partes[#partes + 1] = cod
    end
    return #partes > 0 and partes or nil
end

-- true se todas as teclas da combo estão seguradas
local function _todasSeguradas(codigos)
    for _, cod in ipairs(codigos) do
        if I.teclaSegurando(cod) ~= true then return false end
    end
    return true
end

-- true se modificadores segurados E tecla principal pressionada neste frame
local function _comboPressionou(codigos)
    if #codigos == 1 then
        return I.teclaPressionou(codigos[1])
    end
    local principal = codigos[#codigos]
    if not I.teclaPressionou(principal) then return false end
    for i = 1, #codigos - 1 do
        if not I.teclaSegurando(codigos[i]) then return false end
    end
    return true
end

-- true se modificadores segurados E tecla principal soltou neste frame
local function _comboSoltou(codigos)
    if #codigos == 1 then
        return I.teclaSoltou(codigos[1])
    end
    local principal = codigos[#codigos]
    if not I.teclaSoltou(principal) then return false end
    for i = 1, #codigos - 1 do
        if not I.teclaSegurando(codigos[i]) then return false end
    end
    return true
end

-- aceita número PICO-8 ou string combo e devolve tabela de códigos
local function _resolveTeclaOuCombo(b)
    if type(b) == "number" then
        local nome = _BTN[b]
        return nome and {I[nome]} or {b}
    end
    return _resolverCombo(b)
end

-- ─── API PÚBLICA DE INPUT ─────────────────────────────────────────────────────
-- aceita: número PICO-8, string simples, string combo, código bruto

function Tupi.botao(b)
    local codigos = _resolveTeclaOuCombo(b)
    if not codigos then return false end
    return _todasSeguradas(codigos)
end

function Tupi.pressionou(b)
    local codigos = _resolveTeclaOuCombo(b)
    if not codigos then return false end
    return _comboPressionou(codigos)
end

function Tupi.soltou(b)
    local codigos = _resolveTeclaOuCombo(b)
    if not codigos then return false end
    return _comboSoltou(codigos)
end

-- código numérico direto, sem resolução de combo
function Tupi.tecla(code)    return I.teclaSegurando(code)               end

function Tupi.setTempoSegurando(s)   I.setTempoSegurando(s)              end
function Tupi.getTempoSegurando()    return I.getTempoSegurando()        end
function Tupi.setLayout(id)          KB.setLayout(id)                    end
function Tupi.getLayout()            return KB.getLayout()               end
function Tupi.setRepeat(a, p)        KB.setRepeat(a, p)                  end
function Tupi.getRepeat()            return KB.getRepeat()               end

-- ─── INPUT — MOUSE ───────────────────────────────────────────────────────────

function Tupi.mouseX()        return I.mouseX()               end
function Tupi.mouseY()        return I.mouseY()               end
function Tupi.mouseDX()       return I.mouseDX()              end
function Tupi.mouseDY()       return I.mouseDY()              end
function Tupi.mousePos()      return I.mousePos()             end
function Tupi.mouseXRaw()     return I.mouseXRaw()            end
function Tupi.mouseYRaw()     return I.mouseYRaw()            end
function Tupi.mouseClicou(b)  return I.mouseClicou(b or 0)   end
function Tupi.mouseBotao(b)   return I.mouseSegurando(b or 0) end
function Tupi.mouseSoltou(b)  return I.mouseSoltou(b or 0)   end
function Tupi.scrollX()       return I.scrollX()              end
function Tupi.scrollY()       return I.scrollY()              end

-- ─── INPUT DE TEXTO ──────────────────────────────────────────────────────────
-- teclado() retorna o texto atual a cada frame
-- input()   retorna nil enquanto digita, e a string ao pressionar Enter

local _kb = {
    inputs={}, frameNum=0, kbFrame=-1,
    kbOrdem={}, kbFoco=nil, kbEventos={}, kbTabFrame=-1,
    kbAcento=nil, kbAcentoPendente=nil,
    repBs={acum=0,proximo=0}, repEsq={acum=0,proximo=0},
    repDir={acum=0,proximo=0}, repSp={acum=0,proximo=0},
    repChar={code=nil,acum=0,proximo=0},
}
local BLINK_T=0.52 -- intervalo do cursor piscante em segundos
local CW=8*0.5     -- largura de um caractere na fonte padrão
local CH=8*0.5     -- altura de um caractere na fonte padrão

-- detecta qual tecla de caractere foi pressionada neste frame (sem shift/acento)
local function _charBruto()
    local shift = I.teclaSegurando(I.TECLA_SHIFT_ESQ) or I.teclaSegurando(I.TECLA_SHIFT_DIR)
    for _, l in ipairs{"A","B","C","D","E","F","G","H","I","J","K","L","M",
                       "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"} do
        local code = I["TECLA_"..l]
        if code and I.teclaPressionou(code) then
            return { char=shift and l or l:lower(), code=code }
        end
    end
    for d=0,9 do
        local code = I["TECLA_"..d]
        if code and I.teclaPressionou(code) then
            return { char=shift and KB.getNumShift(d) or tostring(d), code=code }
        end
    end
    for _, s in ipairs(KB.getSimb()) do
        if I.teclaPressionou(s.c) then
            return { char=shift and s.s or s.n, code=s.c }
        end
    end
    return nil
end

-- captura caractere com suporte a repetição e composição de acentos
local function _capturarChar()
    local atraso, passo = KB.getRepeat()
    local rep = _kb.repChar
    local novo = _charBruto()
    local charAtual = nil

    if novo then
        rep.code=novo.code; rep.char=novo.char; rep.acum=0; rep.proximo=atraso
        charAtual=novo.char
    elseif rep.code and I.teclaSegurando(rep.code) then
        local dt=J.dt(); rep.acum=rep.acum+dt
        if rep.acum>=rep.proximo then
            rep.proximo=rep.proximo+passo
            local s2=I.teclaSegurando(I.TECLA_SHIFT_ESQ) or I.teclaSegurando(I.TECLA_SHIFT_DIR)
            local c=rep.code
            if c>=65 and c<=90 then
                ---@diagnostic disable: param-type-mismatch
                charAtual=s2 and string.char(c) or string.char(c+32)
            elseif c>=48 and c<=57 then
                charAtual=s2 and KB.getNumShift(c-48) or tostring(c-48)
            else
                for _,s in ipairs(KB.getSimb()) do
                    if s.c==c then charAtual=s2 and s.s or s.n; break end
                end
            end
        end
    else
        rep.code=nil; rep.acum=0; rep.proximo=atraso
    end

    if not charAtual then return nil end

    -- combinação de acento: aguarda próxima tecla para compor (ex: ´ + a = á)
    local acento=_kb.kbAcento
    if KB.ehAcento(charAtual) then
        if acento==charAtual then _kb.kbAcento=nil; return charAtual
        elseif acento then _kb.kbAcento=charAtual; return acento
        else _kb.kbAcento=charAtual; return nil end
    elseif acento then
        _kb.kbAcento=nil
        local combinado=KB.combinar(acento, charAtual)
        if combinado then return combinado end
        _kb.kbAcentoPendente=charAtual; return acento
    end
    return charAtual
end

-- cria o estado inicial de um campo de texto
local function _novoEstadoKB(prefixo, limite)
    return {texto="",cursor=0,blinkT=0,blinkVis=true,
            prefixo=tostring(prefixo or ""),limite=tonumber(limite) or 0}
end

-- captura todos os eventos de teclado uma vez por frame
local function _capturarEventosKB()
    if _kb.kbFrame==_kb.frameNum then return end
    _kb.kbFrame=_kb.frameNum; _kb.kbOrdem={}
    local dt=J.dt()
    _kb.kbEventos={
        dt=dt,
        backspace=KB.tickRepeat(_kb.repBs,  I.teclaPressionou(I.TECLA_BACKSPACE), I.teclaSegurando(I.TECLA_BACKSPACE), dt),
        esq      =KB.tickRepeat(_kb.repEsq, I.teclaPressionou(I.TECLA_ESQUERDA),  I.teclaSegurando(I.TECLA_ESQUERDA),  dt),
        dir      =KB.tickRepeat(_kb.repDir, I.teclaPressionou(I.TECLA_DIREITA),   I.teclaSegurando(I.TECLA_DIREITA),   dt),
        tab      =I.teclaPressionou(I.TECLA_TAB),
        enter    =I.teclaPressionou(I.TECLA_ENTER),
        space    =KB.tickRepeat(_kb.repSp,  I.teclaPressionou(I.TECLA_ESPACO),    I.teclaSegurando(I.TECLA_ESPACO),    dt),
        char     =_capturarChar(),
        charPend =_kb.kbAcentoPendente,
    }
    _kb.kbAcentoPendente=nil
end

-- registra o campo na ordem de foco e define foco inicial
local function _registrarCampo(chave)
    for _,c in ipairs(_kb.kbOrdem) do if c==chave then return end end
    _kb.kbOrdem[#_kb.kbOrdem+1]=chave
    if _kb.kbFoco==nil then _kb.kbFoco=chave end
end

-- avança o foco para o próximo campo ao pressionar Tab
local function _avancarFoco()
    local ev=_kb.kbEventos
    if not ev.tab or _kb.kbTabFrame==_kb.frameNum then return end
    _kb.kbTabFrame=_kb.frameNum
    local n=#_kb.kbOrdem
    if n>1 then
        local pos=1
        for i,c in ipairs(_kb.kbOrdem) do if c==_kb.kbFoco then pos=i;break end end
        _kb.kbFoco=_kb.kbOrdem[(pos%n)+1]
    end
end

-- aplica backspace, movimento de cursor e inserção de caractere
local function _aplicarInputCampo(st, ev)
    if ev.backspace and st.cursor>0 then
        st.texto=st.texto:sub(1,st.cursor-1)..st.texto:sub(st.cursor+1)
        st.cursor=st.cursor-1; st.blinkT=0; st.blinkVis=true
    end
    if ev.esq then st.cursor=math.max(0,st.cursor-1) end
    if ev.dir then st.cursor=math.min(#st.texto,st.cursor+1) end
    local function _ins(ch)
        if st.limite==0 or #st.texto<st.limite then
            st.texto=st.texto:sub(1,st.cursor)..ch..st.texto:sub(st.cursor+1)
            st.cursor=st.cursor+1; st.blinkT=0; st.blinkVis=true
        end
    end
    if ev.space    then _ins(" ")         end
    if ev.charPend then _ins(ev.charPend) end
    if ev.char     then _ins(ev.char)     end
end

-- desenha o campo de texto com prefixo e cursor piscante
local function _desenharCampo(st, x, y, cor, comFoco, fonte)
    local pref=st.prefixo
    local tx=x+#pref*CW
    if cor ~= nil then
        Txt.setCor(_resolverCorTexto(cor))
    end
    if #pref>0 then Txt.desenhar(x, y, 20, pref, 0.5, 1.0, fonte) end
    Txt.desenhar(tx, y, 20, st.texto, 0.5, 1.0, fonte)
    if comFoco and st.blinkVis then
        local cx=tx+st.cursor*CW
        R.retangulo(cx, y, 1, CH)
    end
    Txt.setCor(1,1,1,1)
end

-- campo de texto contínuo: retorna o texto digitado a cada frame
function Tupi.teclado(x, y, prefixo, limite, cor, fonte)
    local chave=tostring(x)..","..tostring(y)
    local st=_kb.inputs[chave]
    if not st or st.prefixo~=tostring(prefixo or "") or st.limite~=(tonumber(limite) or 0) then
        local txt=st and st.texto or ""; local cur=st and st.cursor or 0
        st=_novoEstadoKB(prefixo,limite); st.texto=txt; st.cursor=math.min(cur,#txt)
        _kb.inputs[chave]=st
    end
    _capturarEventosKB(); _registrarCampo(chave); _avancarFoco()
    local ev=_kb.kbEventos
    st.blinkT=st.blinkT+ev.dt
    if st.blinkT>=BLINK_T then st.blinkT=st.blinkT-BLINK_T; st.blinkVis=not st.blinkVis end
    if chave==_kb.kbFoco then _aplicarInputCampo(st,ev) end
    local comFoco=(_kb.kbFoco==nil) or (_kb.kbFoco==chave)
    _desenharCampo(st,x,y,cor,comFoco,fonte)
    return st.texto
end

-- campo de texto com confirmação: retorna nil enquanto digita, string ao Enter
function Tupi.input(x, y, prefixo, limite, cor, fonte)
    local chave=tostring(x)..","..tostring(y)
    local st=_kb.inputs[chave]
    if not st or st.prefixo~=tostring(prefixo or "") or st.limite~=(tonumber(limite) or 0) then
        local txt=st and st.texto or ""; local cur=st and st.cursor or 0
        st=_novoEstadoKB(prefixo,limite); st.texto=txt; st.cursor=math.min(cur,#txt)
        _kb.inputs[chave]=st
    end
    _capturarEventosKB(); _registrarCampo(chave); _avancarFoco()
    local ev=_kb.kbEventos
    st.blinkT=st.blinkT+ev.dt
    if st.blinkT>=BLINK_T then st.blinkT=st.blinkT-BLINK_T; st.blinkVis=not st.blinkVis end
    local resultado=nil
    if chave==_kb.kbFoco then
        if ev.enter then
            -- Enter confirma: retorna texto e limpa o campo
            resultado=st.texto; st.texto=""; st.cursor=0; st.blinkT=0; st.blinkVis=true
        else
            _aplicarInputCampo(st,ev)
        end
    end
    local comFoco=(_kb.kbFoco==nil) or (_kb.kbFoco==chave)
    _desenharCampo(st,x,y,cor,comFoco,fonte)
    return resultado
end

-- ─── COLISÃO ─────────────────────────────────────────────────────────────────

function Tupi.colidiu(a, b)          return Col.retRet(a, b)           end
function Tupi.colisaoInfo(a, b)      return Col.retRetInfo(a, b)       end
function Tupi.cirColidiu(a, b)       return Col.cirCir(a, b)           end
function Tupi.cirColisaoInfo(a, b)   return Col.cirCirInfo(a, b)       end
function Tupi.retCirculo(r, c)       return Col.retCir(r, c)           end
function Tupi.pontoRet(px, py, r)    return Col.pontoRet(px, py, r)    end
function Tupi.pontoCir(px, py, c)    return Col.pontoCir(px, py, c)    end
function Tupi.mouseNoRet(r)          return Col.mouseNoRet(r)          end
function Tupi.mouseNoCir(c)          return Col.mouseNoCir(c)          end

-- ─── FÍSICA ──────────────────────────────────────────────────────────────────

function Tupi.corpo(x, y, massa, elastic, atrito)  return Fis.corpo(x, y, massa, elastic, atrito) end
function Tupi.corpoEstatico(x, y)                  return Fis.corpoEstatico(x, y)                 end
function Tupi.atualizarCorpo(corpo, grav)          Fis.atualizar(corpo, grav)                     end
function Tupi.impulso(corpo, fx, fy)               Fis.impulso(corpo, fx, fy)                     end
function Tupi.atritoCorpo(corpo)                   Fis.atrito(corpo)                              end
function Tupi.limitarVel(corpo, max)               Fis.limitarVel(corpo, max)                     end
function Tupi.posCorpo(corpo)                      return Fis.pos(corpo)                          end
function Tupi.velCorpo(corpo)                      return Fis.vel(corpo)                          end
function Tupi.setPosCorpo(corpo, x, y)             Fis.setPosicao(corpo, x, y)                    end
function Tupi.setVelCorpo(corpo, vx, vy)           Fis.setVel(corpo, vx, vy)                      end
function Tupi.retColCorpo(corpo, larg, alt)        return Fis.retCol(corpo, larg, alt)            end
function Tupi.cirColCorpo(corpo, raio)             return Fis.cirCol(corpo, raio)                 end
function Tupi.resolverColisao(a, b, info)          Fis.resolverColisao(a, b, info)                end
function Tupi.resolverEstatico(corpo, info)        Fis.resolverEstatico(corpo, info)              end
function Tupi.sincronizar(wrapper, corpo)          Fis.sincronizar(wrapper, corpo)                end

-- ─── MAPA ────────────────────────────────────────────────────────────────────
--
-- Toda a lógica pesada (FFI, registrar defs, build, validação) foi
-- movida para engine_mundo (Mundo.mapa). Aqui ficam apenas wrappers
-- finos que mantêm os nomes originais da API pública.
--
-- Fluxo normal de uso:
--   local m = Tupi.mapc("tileset.png", 16, 16, 20, 15)
--   Tupi.mflag(m, 1, {solido=true})      ← opcional, antes de mapa()
--   Tupi.mapa(m, { 0,1,2,1,0, ... })     ← define grade e compila
--   -- loop:
--   Tupi.mapu(m, dt)
--   Tupi.mapd(m)

-- _mapaValido: checa se m é um InstMeta válido do engine_mundo
local function _mapaValido(m)
    return type(m)=="table" and type(m._colunas)=="number" and m._colunas > 0
end

-- converte string de números em array flat (ex: "1 2 -1 3")
local function _mapa_parse(s)
    local arr = {}
    for tok in s:gmatch("[%-]?%d+") do arr[#arr+1] = tonumber(tok) end
    return arr
end

-- mapc(png, tw, th, cols, lins) → instância InstMeta (engine_mundo)
-- tw/th = tamanho do tile em px; cols/lins = dimensões em tiles
function Tupi.mapc(png, tw, th, cols, lins)
    assert(type(png)=="string" and png~="",    "[mapc] png inválido")
    assert(type(tw)=="number"  and tw>0,       "[mapc] tw inválido")
    assert(type(th)=="number"  and th>0,       "[mapc] th inválido")
    assert(type(cols)=="number" and cols>0,    "[mapc] cols inválido")
    assert(type(lins)=="number" and lins>0,    "[mapc] lins inválido")
    local m = Mundo.mapa.novo()
    m:criarMapa(cols, lins, tw, th)
    m:atlas(png)
    return m
end

-- mapa(m, dados) — define o grid e compila via engine_mundo:build()
-- dados: table flat {0,1,2,...} ou string com números separados por espaço/newline
-- Tile 0 = vazio; tile >= 1 = id no tileset
-- IMPORTANTE: chame mflag() ANTES de mapa() para evitar recompilação desnecessária
function Tupi.mapa(m, dados)
    assert(_mapaValido(m), "[mapa] primeiro argumento deve ser um mapa criado com mapc()")
    local arr
    if type(dados)=="string" then
        arr = _mapa_parse(dados)
    elseif type(dados)=="table" then
        arr = dados
    else
        error("[mapa] dados deve ser table ou string, recebido: "..type(dados))
    end
    local esp = m._colunas * m._linhas
    assert(#arr==esp, string.format(
        "[mapa] array tem %d elementos, esperado %d (%dx%d)",
        #arr, esp, m._colunas, m._linhas))
    m:carregarArray(arr)
    m:build()
end

-- mflag(m, tile_id, {solido, trigger, passagem}) — define flags de colisão
-- Chame ANTES de mapa() — engine_mundo só armazena a flag sem recompilar
-- Se chamar depois de mapa(), recompila automaticamente via build()
function Tupi.mflag(m, tile_id, opts)
    assert(_mapaValido(m),
        "[mflag] primeiro argumento deve ser um mapa criado com mapc()")
    assert(type(tile_id)=="number" and tile_id>=1,
        "[mflag] tile_id deve ser número >= 1 (tile 0 é vazio e não aceita flags), recebido: "..tostring(tile_id))
    assert(opts==nil or type(opts)=="table",
        "[mflag] opts deve ser table ou nil, recebido: "..type(opts))
    m:flags(tile_id, opts or {})
    if m._valido then m:build() end
end

-- define animação de um tile: mframes(m, tile_id, frames, fps [, loop])
function Tupi.mframes(m, tile_id, frames, fps, loop)
    assert(_mapaValido(m), "[mframes] mapa inválido")
    m:frames(tile_id, frames, fps, loop)
    if m._valido then m:build() end
end

-- mapd(m [, z]) — desenha o mapa (z = z-index base, padrão 0)
function Tupi.mapd(m, z)
    assert(_mapaValido(m), "[mapd] primeiro argumento deve ser um mapa criado com mapc()")
    m:desenhar(z or 0)
end

-- mapu(m [, dt]) — atualiza animações do mapa (chamar em _rodar/_update)
function Tupi.mapu(m, dt)
    assert(_mapaValido(m), "[mapu] primeiro argumento deve ser um mapa criado com mapc()")
    m:atualizar(dt or J.dt())
end

-- mget(m, col, lin) → tile_id numérico (0 = vazio ou fora dos limites)
function Tupi.mget(m, col, lin)
    assert(_mapaValido(m), "[mget] mapa inválido")
    return m:tileIdEmGrade(col, lin)
end

-- mset(m, col, lin, tile_id) — altera célula em runtime e recompila
function Tupi.mset(m, col, lin, tid)
    assert(_mapaValido(m), "[mset] mapa inválido")
    if col<0 or col>=m._colunas or lin<0 or lin>=m._linhas then return end
    m._array[lin * m._colunas + col + 1] = tid or 0
    m:build()
end

-- msolido(m, px, py) → bool — tile sob o ponto (em pixels) é sólido?
function Tupi.msolido(m, px, py)
    assert(_mapaValido(m), "[msolido] mapa inválido")
    local col = math.floor(px / m._larg_tile)
    local lin = math.floor(py / m._alt_tile)
    return m:isSolido(col, lin)
end

-- mtrigger(m, px, py) → bool — tile sob o ponto (em pixels) é trigger?
function Tupi.mtrigger(m, px, py)
    assert(_mapaValido(m), "[mtrigger] mapa inválido")
    local col = math.floor(px / m._larg_tile)
    local lin = math.floor(py / m._alt_tile)
    return m:isTrigger(col, lin)
end

-- mcel(m, col, lin) → {id, solido, trigger, passagem, flags}
function Tupi.mcel(m, c, l)
    assert(_mapaValido(m), "[mcel] mapa inválido")
    local tid = m:tileIdEmGrade(c, l)
    local hb  = m._valido and m:hitboxTile(c, l) or nil
    return {
        id       = tid,
        solido   = hb and hb.solido   or false,
        trigger  = hb and hb.trigger  or false,
        passagem = m:isPassagem(c, l),
        flags    = hb and hb.flags or 0,
    }
end

function Tupi.mhitbox(m, c, l)        return m:hitboxTile(c, l)           end
function Tupi.mdef(m, c, l)           return m:definicaoEmGrade(c, l)     end
function Tupi.mdefPonto(m, px, py)    return m:definicaoEmPonto(px, py)   end
function Tupi.mtileEmPonto(m, px, py) return m:tileEmPonto(px, py)        end
function Tupi.mdestruir(m)            assert(_mapaValido(m), "[mdestruir] mapa inválido"); m:destruir() end

-- Aliases em inglês (estilo PICO-8)
Tupi.map_create  = Tupi.mapc
Tupi.map_data    = Tupi.mapa
Tupi.map_flag    = Tupi.mflag
Tupi.map_draw    = Tupi.mapd
Tupi.map_update  = Tupi.mapu
Tupi.map_solid   = Tupi.msolido
Tupi.map_trigger = Tupi.mtrigger
Tupi.map_cell    = Tupi.mcel
Tupi.map_get     = Tupi.mget
Tupi.map_set     = Tupi.mset

-- ─── MUNDOS ──────────────────────────────────────────────────────────────────

local Mds=Mundo.mundos
function Tupi.trocarMundo(a,n)        Mds.trocar(a,n)          end
function Tupi.mundoAtual()            return Mds.atual()        end
function Tupi.infoMundoAtual()        return Mds.infoAtual()    end
function Tupi.eMundoAtual(a,n)        return Mds.eAtual(a,n)    end
function Tupi.precarregarMundo(a,n)   Mds.precarregar(a,n)      end
function Tupi.descarregarMundo(a,n)   Mds.descarregar(a,n)      end
function Tupi.aoSairMundo(fn)         Mds.aoSair(fn)            end
function Tupi.aoEntrarMundo(fn)       Mds.aoEntrar(fn)          end
function Tupi.destruirTodosMundos()   Mds.destruirTudo()        end

-- ─── FADE ────────────────────────────────────────────────────────────────────

function Tupi.criarFade(larg, alt, duracao) return Visual.fade.novo(Tupi,larg,alt,duracao) end

-- ─── MATEMÁTICA ──────────────────────────────────────────────────────────────

function Tupi.lerp(a,b,t)          return J.lerp(a,b,t)          end
function Tupi.aleatorio(min,max)
    if min==nil then return math.random() end
    if max==nil then return math.random()*min end
    return J.aleatorio(min,max)
end
function Tupi.radianos(g)          return J.rad(g)               end
function Tupi.graus(r)             return J.graus(r)             end
function Tupi.distancia(x1,y1,x2,y2) return J.distancia(x1,y1,x2,y2) end

-- atalhos matemáticos diretos
Tupi.flr  = math.floor;  Tupi.chao = math.floor
Tupi.ceil = math.ceil;   Tupi.teto = math.ceil
Tupi.abs  = math.abs;    Tupi.raiz = math.sqrt
Tupi.sen  = math.sin;    Tupi.sin  = math.sin
Tupi.cos  = math.cos;    Tupi.tan  = math.tan
Tupi.sqrt = math.sqrt;   Tupi.pi   = math.pi
Tupi.max  = math.max;    Tupi.min  = math.min
Tupi.rnd  = function(n) if n==nil then return math.random() end; return math.random()*n end
Tupi.mid  = function(a,b,c) return math.max(a,math.min(b,c)) end
Tupi.meio = Tupi.mid

-- acesso direto ao normalizador de texto
Tupi.norm = Norm

-- ─── LOOP PRINCIPAL ──────────────────────────────────────────────────────────

-- injeta todas as funções Tupi no escopo global _G
local function _injetarGlobais()
    -- janela
    _G.largura=Tupi.largura; _G.altura=Tupi.altura
    _G.tempo=Tupi.tempo;     _G.dt=Tupi.dt

    -- render
    _G.cor=Tupi.cor;         _G.usarCor=Tupi.usarCor
    _G.corFundo=Tupi.corFundo
    _G.escrever=Tupi.escrever
    _G.escreverSombra=Tupi.escreverSombra
    _G.escreverCaixa=Tupi.escreverCaixa
    _G.ret=Tupi.retangulo;   _G.bret=Tupi.bordaRet
    _G.circ=Tupi.circulo;    _G.bcirc=Tupi.bordaCirc
    _G.lin=Tupi.linha;       _G.tri=Tupi.triangulo
    _G.pix=Tupi.pixel;       _G.flush=Tupi.flush
    _G.cls=Tupi.corFundo;    _G.print=Tupi.escrever
    _G.rectfill=Tupi.retangulo; _G.rect=Tupi.bordaRet
    _G.circfill=Tupi.circulo

    -- texto
    _G.carregarFonte=Tupi.carregarFonte
    _G.setFontePadrao=Tupi.setFontePadrao
    _G.setCorTexto=Tupi.setCorTexto

    -- sprites / objetos
    _G.img=Tupi.imagem;      _G.obj=Tupi.objeto
    _G.ver=Tupi.mostrar;     _G.spr=Tupi.desenharSprite
    _G.mover=Tupi.mover;     _G.posicionar=Tupi.posicionar
    _G.posicao=Tupi.posicao; _G.espelhar=Tupi.espelhar
    _G.destruir=Tupi.destruir; _G.hitbox=Tupi.hitbox

    -- animação
    _G.criarAnim=Tupi.criarAnim; _G.tocarAnim=Tupi.tocarAnim
    _G.pararAnim=Tupi.pararAnim; _G.animTerminou=Tupi.animTerminou

    -- input teclado
    _G.btn=Tupi.botao;     _G.btnp=Tupi.pressionou; _G.btnr=Tupi.soltou
    _G.tecla=Tupi.tecla

    -- input texto
    _G.teclado=Tupi.teclado; _G.input=Tupi.input

    -- mouse
    _G.mx=Tupi.mouseX;    _G.my=Tupi.mouseY
    _G.mclk=Tupi.mouseClicou; _G.mbtn=Tupi.mouseBotao
    _G.scroll=Tupi.scrollY

    -- colisão
    _G.colidiu=Tupi.colidiu;   _G.colisaoInfo=Tupi.colisaoInfo
    _G.pontoRet=Tupi.pontoRet; _G.pontoCir=Tupi.pontoCir
    _G.mouseNoRet=Tupi.mouseNoRet; _G.mouseNoCir=Tupi.mouseNoCir

    -- mapa
    _G.mapc=Tupi.mapc;  _G.mapa=Tupi.mapa;   _G.mflag=Tupi.mflag
    _G.mframes=Tupi.mframes; _G.mapd=Tupi.mapd; _G.mapu=Tupi.mapu
    _G.mget=Tupi.mget;  _G.mset=Tupi.mset;   _G.mcel=Tupi.mcel
    _G.msolido=Tupi.msolido; _G.mtrigger=Tupi.mtrigger
    _G.mhitbox=Tupi.mhitbox

    -- matemática curta
    _G.rnd=Tupi.rnd;    _G.lerp=Tupi.lerp;   _G.dist=Tupi.distancia
    _G.rad=Tupi.radianos; _G.deg=Tupi.graus
    _G.flr=math.floor;  _G.mid=Tupi.mid;     _G.abs=math.abs
    _G.sqrt=math.sqrt;  _G.sen=math.sin;     _G.cos=math.cos

    -- paleta e cores nomeadas
    _G.PALETA=Tupi.PALETA;  _G.pal=Tupi.PALETA
    _G.BRANCO=Tupi.BRANCO;  _G.PRETO=Tupi.PRETO
    _G.VERMELHO=Tupi.VERMELHO; _G.VERDE=Tupi.VERDE
    _G.AZUL=Tupi.AZUL;      _G.AMARELO=Tupi.AMARELO
    _G.ROXO=Tupi.ROXO;      _G.LARANJA=Tupi.LARANJA
    _G.CIANO=Tupi.CIANO;    _G.CINZA=Tupi.CINZA
    _G.ROSA=Tupi.ROSA

    _G.Tupi=Tupi
end

-- caminhos candidatos para fonte bitmap padrão do engine
local _FONTES_PADRAO = {
    { path="ascii.png",larg=8,  alt=8},
}

-- tenta carregar uma fonte padrão caso nenhuma tenha sido definida
local function _garantirFontePadrao()
    if Txt.getFontePadrao() then return true end
    for _, f in ipairs(_FONTES_PADRAO) do
        local ok, fonte = pcall(Txt.carregarFonte, f.path, f.larg, f.alt)
        if ok and fonte then
            Txt.setFontePadrao(fonte)
            return true
        end
    end
    return false
end

-- inicia o loop principal: chama _iniciar(), depois _rodar() e _desenhar() a cada frame
function Tupi.rodar()
    _injetarGlobais()
    _garantirFontePadrao()
    if type(_G._iniciar)=="function" then _G._iniciar() end
    while J.rodando() do
        J.limpar()
        _kb.frameNum=_kb.frameNum+1
        if type(_G._rodar)=="function"    then _G._rodar()    end
        if type(_G._desenhar)=="function" then _G._desenhar() end
        R.batchDesenhar()
        J.atualizar()
    end
    J.fechar()
end

-- ─── ALIASES NO MÓDULO ───────────────────────────────────────────────────────

Tupi.apresentar=Tupi.atualizar;  Tupi.cls=Tupi.corFundo
Tupi.print=Tupi.escrever;        Tupi.image=Tupi.imagem
Tupi.object=Tupi.objeto;         Tupi.draw=Tupi.mostrar
Tupi.draw_sprite=Tupi.desenharSprite; Tupi.move=Tupi.mover
Tupi.set_pos=Tupi.posicionar;    Tupi.get_pos=Tupi.posicao
Tupi.alpha=Tupi.alfa;            Tupi.destroy=Tupi.destruir
Tupi.overlap=Tupi.colidiu;       Tupi.new_anim=Tupi.criarAnim
Tupi.anim_done=Tupi.animTerminou; Tupi.time=Tupi.tempo
Tupi.set_fps=Tupi.fpsLimite;     Tupi.get_fps=Tupi.fpsAtual
Tupi.rad=Tupi.radianos;          Tupi.deg=Tupi.graus
Tupi.dist=Tupi.distancia;        Tupi.btn=Tupi.botao
Tupi.btnp=Tupi.pressionou;       Tupi.btnr=Tupi.soltou
Tupi.key=Tupi.botao;             Tupi.keyp=Tupi.pressionou
Tupi.keyr=Tupi.soltou
Tupi.mc=Tupi.mapc;  Tupi.md=Tupi.mapd;  Tupi.mu=Tupi.mapu
Tupi.mf=Tupi.mflag; Tupi.ms=Tupi.msolido; Tupi.mt=Tupi.mtrigger
Tupi.set_repeat=KB.setRepeat;    Tupi.set_layout=KB.setLayout

return Tupi