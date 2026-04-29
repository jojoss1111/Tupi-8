---@diagnostic disable: undefined-global
local KB      = require("src.Engine.tupi_teclado")
local Sintaxe = {}

local ESCALA_FONTE = 0.5
local CHAR_H = 8 * ESCALA_FONTE
local LINHA_H = CHAR_H + 1

local PALETA = {
    [0]  = {0.00, 0.00, 0.00, 1.0},
    [1]  = {0.11, 0.17, 0.33, 1.0},
    [2]  = {0.49, 0.15, 0.32, 1.0},
    [3]  = {0.00, 0.53, 0.33, 1.0},
    [4]  = {0.67, 0.32, 0.21, 1.0},
    [5]  = {0.37, 0.34, 0.31, 1.0},
    [6]  = {0.76, 0.76, 0.76, 1.0},
    [7]  = {1.00, 0.95, 0.91, 1.0},
    [8]  = {1.00, 0.00, 0.30, 1.0},
    [9]  = {1.00, 0.64, 0.00, 1.0},
    [10] = {1.00, 0.93, 0.15, 1.0},
    [11] = {0.00, 0.89, 0.21, 1.0},
    [12] = {0.16, 0.68, 1.00, 1.0},
    [13] = {0.51, 0.46, 0.86, 1.0},
    [14] = {1.00, 0.47, 0.66, 1.0},
    [15] = {1.00, 0.80, 0.67, 1.0},
}

local BTN_MAP = {
    [0] = "TECLA_ESQUERDA",
    [1] = "TECLA_DIREITA",
    [2] = "TECLA_CIMA",
    [3] = "TECLA_BAIXO",
    [4] = "TECLA_Z",
    [5] = "TECLA_X",
}

local Runtime = {}
Runtime.__index = Runtime

local function _clonarCor(cor)
    return {cor[1], cor[2], cor[3], cor[4] or 1.0}
end

local function _linhasTexto(texto)
    local n = 1
    for _ in tostring(texto):gmatch("\n") do
        n = n + 1
    end
    return n
end

local function _temCamposObjeto(obj)
    return type(obj) == "table" and obj.obj ~= nil
end

function Sintaxe.novo(Tupi, ascii)
    assert(Tupi,  "[Sintaxe] Tupi obrigatorio")
    assert(ascii, "[Sintaxe] caminho do ascii.png obrigatorio")

    local self = setmetatable({}, Runtime)
    self.T = Tupi
    self._fonte        = Tupi.texto.carregarFonte(ascii, 8, 8)
    self._ativo        = false
    self._arquivo      = nil
    self._origem       = "console"
    self._retorno      = nil
    self._erro         = nil
    self._env          = nil
    self._callbacks    = {}
    self._opsBoot      = {}
    self._opsFrame     = {}
    self._estadoAtivo  = nil
    self._modo         = nil
    self._sprites      = {}
    return self
end

function Runtime:estaAtivo()
    return self._ativo
end

function Runtime:consumirRetorno()
    local retorno = self._retorno
    self._retorno = nil
    return retorno
end

function Runtime:parar()
    self._ativo   = false
    self._retorno = self._origem or "console"
end

function Runtime:_novoEstado()
    return {
        cor     = _clonarCor(PALETA[7]),
        cameraX = 0,
        cameraY = 0,
        cursorX = 0,
        cursorY = 0,
    }
end

function Runtime:_resolverCor(cor, fallback)
    if type(cor) == "table" then
        return {cor[1] or 1, cor[2] or 1, cor[3] or 1, cor[4] or 1}
    end
    if type(cor) == "number" and PALETA[cor] then
        return _clonarCor(PALETA[cor])
    end
    if fallback then
        return _clonarCor(fallback)
    end
    return _clonarCor(PALETA[7])
end

function Runtime:_carregarSprite(ref)
    if type(ref) ~= "string" or ref == "" then
        error("[Sintaxe] imagem() precisa de um caminho .png")
    end
    local spr = self._sprites[ref]
    if not spr then
        spr = self.T.carregarSprite(ref)
        self._sprites[ref] = spr
    end
    return spr
end

function Runtime:_resolverSprite(ref)
    if type(ref) == "string" then
        return self:_carregarSprite(ref)
    end
    if ref ~= nil then
        return ref
    end
    error("[Sintaxe] sprite invalido")
end

function Runtime:_dimensoesSprite(sprite, opt)
    opt = opt or {}
    -- larg/alt no opt são o tamanho do QUADRO (célula do sprite sheet), não da textura inteira.
    local larg = opt.larg or opt.largura or 16
    local alt  = opt.alt  or opt.altura  or 16
    return larg, alt
end

function Runtime:_criarObjeto(spriteRef, x, y, opt)
    opt = opt or {}
    local sprite = self:_resolverSprite(spriteRef)
    local larg, alt = self:_dimensoesSprite(sprite, opt)
    local obj = self.T.criarObjeto(
        x or 0,
        y or 0,
        opt.z or opt.prof or 0,
        larg,
        alt,
        opt.col or opt.coluna or 0,
        opt.lin or opt.linha  or 0,
        opt.alfa or opt.transparencia or 1.0,
        opt.escala or 1.0,
        sprite
    )
    obj._sprite_ref = spriteRef
    return obj
end

function Runtime:_emitir(op)
    if self._modo == "boot" or self._modo == "update" then
        self._opsDestino[#self._opsDestino + 1] = op
    elseif self._modo == "draw" then
        self:_executarOp(op, self._estadoAtivo)
    end
end

function Runtime:_executarOp(op, estado)
    local T = self.T
    if op.tipo == "cor" then
        estado.cor = self:_resolverCor(op.cor, estado.cor)
        return
    end
    if op.tipo == "camera" then
        estado.cameraX = op.x or 0
        estado.cameraY = op.y or 0
        return
    end

    local cor = self:_resolverCor(op.cor, estado.cor)
    local x   = (op.x or 0) - estado.cameraX
    local y   = (op.y or 0) - estado.cameraY

    if op.tipo == "limpar" then
        T.retangulo(0, 0, T.largura(), T.altura(), cor)
        estado.cursorX = 0
        estado.cursorY = 0
    elseif op.tipo == "escrever" then
        T.cor(cor[1], cor[2], cor[3], cor[4] or 1.0)
        T.texto.desenhar(x, y, 10, tostring(op.texto), ESCALA_FONTE, 1.0, self._fonte)
        T.cor(1, 1, 1, 1)
    elseif op.tipo == "pixel" then
        T.retangulo(x, y, 1, 1, cor)
    elseif op.tipo == "retangulo" then
        T.retangulo(x, y, op.larg or 0, op.alt or 0, cor)
    elseif op.tipo == "bordaRet" then
        T.retanguloBorda(x, y, op.larg or 0, op.alt or 0, 1, cor)
    elseif op.tipo == "circulo" then
        T.circulo(x, y, op.raio or 0, 32, cor)
    elseif op.tipo == "bordaCirc" then
        T.circuloBorda(x, y, op.raio or 0, 32, 1, cor)
    elseif op.tipo == "linha" then
        T.linha(
            x,
            y,
            (op.x2 or 0) - estado.cameraX,
            (op.y2 or 0) - estado.cameraY,
            1,
            cor
        )
    elseif op.tipo == "sprite" then
        local sprite = self:_resolverSprite(op.sprite)
        local obj    = self:_criarObjeto(sprite, x, y, {
            larg   = op.larg,
            alt    = op.alt,
            col    = op.col,
            lin    = op.lin,
            alfa   = op.alfa,
            escala = op.escala,
            z      = op.z,
        })
        T.enviarBatch(obj, op.z or 0)
    elseif op.tipo == "desenharObj" then
        local obj = op.obj and op.obj.obj and op.obj.obj[0]
        if not obj then return end
        local ox, oy = obj.x, obj.y
        obj.x = ox - estado.cameraX
        obj.y = oy - estado.cameraY
        T.enviarBatch(op.obj, op.z or op.obj.z or 0)
        obj.x = ox
        obj.y = oy
    end
end

function Runtime:_replay(ops, estado)
    for _, op in ipairs(ops) do
        self:_executarOp(op, estado)
    end
end

local BLINK_KB = 0.52
local CW_KB    = 8 * 0.5
local CH_KB    = 8 * 0.5

-- ── Captura de char bruto (sem repeat, sem acento) ───────────
-- Retorna {char, code} se alguma tecla foi pressionada AGORA,
-- ou nil. Usado internamente pelo sistema de repeat.
local function _charBruto(T)
    local shift = T.teclaSegurando(T.TECLA_SHIFT_ESQ)
               or T.teclaSegurando(T.TECLA_SHIFT_DIR)

    for _, l in ipairs{"A","B","C","D","E","F","G","H","I","J","K","L","M",
                        "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"} do
        local code = T["TECLA_" .. l]
        if code then
            if T.teclaPressionou(code) then
                return { char = shift and l or l:lower(), code = code }
            end
        end
    end
    for d = 0, 9 do
        local code = T["TECLA_" .. d]
        if code and T.teclaPressionou(code) then
            return { char = shift and KB.getNumShift(d) or tostring(d), code = code }
        end
    end
    for _, s in ipairs(KB.getSimb()) do
        if T.teclaPressionou(s.c) then
            return { char = shift and s.s or s.n, code = s.c }
        end
    end
    return nil
end

-- Captura o próximo char considerando:
--   1. Acento morto: acumula o acento e combina com a próxima tecla
--   2. Key repeat: enquanto segura uma tecla, dispara em intervalos
-- Retorna string | nil
local function _capturarCharKB(T, rt)
    local shift = T.teclaSegurando(T.TECLA_SHIFT_ESQ)
               or T.teclaSegurando(T.TECLA_SHIFT_DIR)
    local atraso, passo = KB.getRepeat()
    local rep = rt._kbRepeat or { code=nil, acum=0, proximo=atraso }
    rt._kbRepeat = rep

    -- Verifica se há nova tecla pressionada agora
    local novo = _charBruto(T)

    -- ── Key repeat: se nenhuma tecla nova, checa se a última ainda é segurada
    local charAtual = nil
    if novo then
        -- Nova tecla: reinicia repeat
        rep.code    = novo.code
        rep.char    = novo.char
        rep.acum    = 0
        rep.proximo = atraso
        charAtual   = novo.char
    elseif rep.code and T.teclaSegurando(rep.code) then
        -- Mesma tecla segurada: acumula tempo
        local dt = T.dt()
        rep.acum = rep.acum + dt
        if rep.acum >= rep.proximo then
            rep.proximo = rep.proximo + passo
            -- recalcula shift no momento do repeat
            local s2 = T.teclaSegurando(T.TECLA_SHIFT_ESQ)
                    or T.teclaSegurando(T.TECLA_SHIFT_DIR)
            -- reconstrói o char a partir do code salvo
            local c = rep.code
            if c >= 65 and c <= 90 then
                charAtual = s2 and string.char(c) or string.char(c + 32)
            elseif c >= 48 and c <= 57 then
                local d = c - 48
                charAtual = s2 and KB.getNumShift(d) or tostring(d)
            else
                for _, s in ipairs(KB.getSimb()) do
                    if s.c == c then charAtual = s2 and s.s or s.n; break end
                end
            end
        end
    else
        -- Tecla solta: reseta repeat
        rep.code    = nil
        rep.acum    = 0
        rep.proximo = atraso
    end

    if not charAtual then return nil end

    -- ── Sistema de acento morto ───────────────────────────────
    local acento = rt._kbAcento

    if KB.ehAcento(charAtual) then
        if acento == charAtual then
            -- Mesmo acento digitado duas vezes: emite o acento
            rt._kbAcento = nil
            return charAtual
        elseif acento then
            -- Acento diferente: emite o anterior e acumula o novo
            rt._kbAcento = charAtual
            return acento
        else
            -- Primeiro acento: acumula e aguarda vogal
            rt._kbAcento = charAtual
            return nil
        end
    elseif acento then
        -- Temos acento pendente + nova tecla
        rt._kbAcento = nil
        local combinado = KB.combinar(acento, charAtual)
        if combinado then
            return combinado
        else
            -- Não combinou: vai emitir o acento agora e o char no próximo frame?
            -- Melhor: emite os dois concatenados não dá pois char é 1 por vez.
            -- Solução: guarda o char para emitir no próximo tick.
            rt._kbAcentoPendente = charAtual
            return acento
        end
    end

    -- char normal sem acento
    return charAtual
end

local function _novoEstadoKB(prefixo, limite)
    return {
        texto    = "",
        cursor   = 0,
        blinkT   = 0,
        blinkVis = true,
        prefixo  = tostring(prefixo or ""),
        limite   = tonumber(limite) or 0,
    }
end

local function _desenharKB(rt, st, x, y, cor_idx, com_foco)
    -- Usa _emitir para que o draw passe pelo pipeline correto do runtime
    -- (igual ao que escrever(), retangulo() etc fazem)
    local cor = rt:_resolverCor(cor_idx, PALETA[7])

    local pref = st.prefixo
    local tx   = x + #pref * CW_KB

    if #pref > 0 then
        rt:_emitir({ tipo = "escrever", texto = pref,     x = x,  y = y, cor = cor_idx })
    end
    rt:_emitir({ tipo = "escrever", texto = st.texto, x = tx, y = y, cor = cor_idx })

    -- Cursor piscante só no campo focado
    if com_foco and st.blinkVis then
        local cx = tx + st.cursor * CW_KB
        rt:_emitir({ tipo = "retangulo", x = cx, y = y, larg = 1, alt = CH_KB, cor = cor_idx })
    end
end

function Runtime:_criarAPI()
    local rt  = self
    local T   = rt.T
    local api = {}

    local function estado()
        return rt._estadoAtivo or rt:_novoEstado()
    end

    local function resolverBotao(botao)
        if type(botao) == "number" then return T[BTN_MAP[botao]] end
        if type(botao) == "string" then return T["TECLA_" .. botao:upper()] end
        return botao
    end

    -- ── Tela ──────────────────────────────────────────────────────────────

    function api.limpar(cor)
        rt:_emitir({ tipo = "limpar", cor = cor or 0 })
    end

    function api.cor(c)
        rt:_emitir({ tipo = "cor", cor = c })
    end

    function api.camera(x, y)
        rt:_emitir({ tipo = "camera", x = x or 0, y = y or 0 })
    end

    -- ── Texto ─────────────────────────────────────────────────────────────

    function api.escrever(texto, x, y, cor)
        local st = estado()
        if x == nil then x = st.cursorX end
        if y == nil then y = st.cursorY end
        rt:_emitir({ tipo = "escrever", texto = tostring(texto), x = x, y = y, cor = cor })
        st.cursorX = 0
        st.cursorY = y + (_linhasTexto(texto) * LINHA_H)
    end

    -- ── Formas ────────────────────────────────────────────────────────────

    function api.pixel(x, y, cor)
        rt:_emitir({ tipo = "pixel", x = x, y = y, cor = cor })
    end

    function api.retangulo(x, y, larg, alt, cor)
        rt:_emitir({ tipo = "retangulo", x = x, y = y, larg = larg, alt = alt, cor = cor })
    end

    function api.bordaRet(x, y, larg, alt, cor)
        rt:_emitir({ tipo = "bordaRet", x = x, y = y, larg = larg, alt = alt, cor = cor })
    end

    function api.circulo(x, y, raio, cor)
        rt:_emitir({ tipo = "circulo", x = x, y = y, raio = raio, cor = cor })
    end

    function api.bordaCirc(x, y, raio, cor)
        rt:_emitir({ tipo = "bordaCirc", x = x, y = y, raio = raio, cor = cor })
    end

    function api.linha(x1, y1, x2, y2, cor)
        rt:_emitir({ tipo = "linha", x = x1, y = y1, x2 = x2, y2 = y2, cor = cor })
    end

    -- ── Sprites / Objetos ─────────────────────────────────────────────────

    -- imagem("caminho.png")  → carrega e devolve sprite
    function api.imagem(caminho)
        return rt:_carregarSprite(caminho)
    end

    -- objeto(sprite, x, y, {opt})  → cria objeto no mundo
    function api.objeto(spriteRef, x, y, opt)
        return rt:_criarObjeto(spriteRef, x, y, opt)
    end

    -- desenharSprite(sprite, x, y, {opt})  → desenha sprite direto na tela
    function api.desenharSprite(spriteRef, x, y, opt)
        opt = opt or {}
        local spr = rt:_resolverSprite(spriteRef)
        rt:_emitir({
            tipo   = "sprite",
            sprite = spr,
            x      = x or 0,
            y      = y or 0,
            larg   = opt.larg or opt.largura,
            alt    = opt.alt  or opt.altura,
            col    = opt.col  or opt.coluna or 0,
            lin    = opt.lin  or opt.linha  or 0,
            escala = opt.escala or 1.0,
            alfa   = opt.alfa  or opt.transparencia or 1.0,
            z      = opt.z    or opt.prof or 0,
        })
    end

    -- mostrar(obj)  → envia objeto ao batch de desenho
    function api.mostrar(obj, z)
        if not _temCamposObjeto(obj) then
            error("[Sintaxe] mostrar() espera um objeto criado com objeto()")
        end
        rt:_emitir({ tipo = "desenharObj", obj = obj, z = z })
    end

    -- ── Movimento / Transformação ─────────────────────────────────────────

    function api.mover(obj, dx, dy)        T.mover(dx or 0, dy or 0, obj)    end
    function api.posicionar(obj, x, y)     T.teleportar(x or 0, y or 0, obj) end
    function api.posicao(obj)              return T.posicaoAtual(obj)         end
    function api.escala(obj, s)            obj.obj[0].escala        = s or 1.0 end
    function api.transparencia(obj, a)     obj.obj[0].transparencia = a or 1.0 end
    function api.tamanho(obj, larg, alt)
        if larg then obj.obj[0].largura = larg end
        if alt  then obj.obj[0].altura  = alt  end
    end
    function api.quadro(obj, col, lin)
        obj.obj[0].coluna = col or 0
        obj.obj[0].linha  = lin or 0
    end
    function api.espelhar(obj, horizontal, vertical)
        T.espelhar(obj, vertical == true, horizontal == true)
    end
    function api.destruir(obj)  T.destruir(obj)  end

    -- ── Colisão ───────────────────────────────────────────────────────────

    function api.colidiu(obj1, obj2)          return T.col.retRet(obj1, obj2)  end
    function api.hitbox(obj, x, y, larg, alt) T.hitbox(obj, x, y, larg, alt)  end

    -- ── Animação ──────────────────────────────────────────────────────────

    function api.criarAnim(obj, nome, quadros, fps, loop)
        T.criarAnim(obj, nome, quadros, fps, loop ~= false)
    end
    function api.tocarAnim(obj, nome)   T.tocarAnim(obj, nome)    end
    function api.pararAnim(obj)         T.pararAnim(obj)           end
    function api.animTerminou(obj)      return T.animTerminou(obj) end

    -- ── Câmera ────────────────────────────────────────────────────────────

    function api.pegarCamera(id)              return T.camera            end
    function api.zoomCamera(cam, z)           T.cameraZoom(cam, z)       end
    function api.moverCamera(cam, x, y)       T.cameraMover(cam, x, y)   end
    function api.seguirCamera(cam, obj, suave) T.cameraSeguir(cam, obj, suave) end

    -- ── Entrada ───────────────────────────────────────────────────────────

    function api.botao(b)
        local tecla = resolverBotao(b)
        return tecla and T.teclaSegurando(tecla) or false
    end

    function api.pressionou(b)
        local tecla = resolverBotao(b)
        return tecla and T.teclaPressionou(tecla) or false
    end

    function api.soltou(b)
        local tecla = resolverBotao(b)
        return tecla and T.teclaSoltou(tecla) or false
    end

    -- ── Input de texto: teclado(x, y, prefixo, limite, cor) ──────────────
    -- Retorna a string atual do input.
    -- Múltiplos campos: só o focado recebe input. TAB troca o foco.
    -- limite=0 → sem limite. limite=N → máximo N chars.

    function api.teclado(x, y, prefixo, limite, cor)
        local chave = tostring(x) .. "," .. tostring(y)

        -- Cria ou atualiza estado do campo
        local st = rt._inputs[chave]
        if not st
        or st.prefixo ~= tostring(prefixo or "")
        or st.limite  ~= (tonumber(limite) or 0)
        then
            local txt = st and st.texto  or ""
            local cur = st and st.cursor or 0
            st = _novoEstadoKB(prefixo, limite)
            st.texto  = txt
            st.cursor = math.min(cur, #txt)
            rt._inputs[chave] = st
        end

        -- ── FASE UPDATE: captura input e aplica ao campo focado ──────────
        if rt._modo == "update" or rt._modo == "boot" then

            -- Primeira chamada do frame: captura eventos e reseta a lista de campos
            if rt._kbFrame ~= rt._frameNum then
                rt._kbFrame   = rt._frameNum
                rt._kbOrdem   = {}
                local dt = T.dt()
                -- backspace e setas com repeat próprio
                local bsRep  = rt._kbRepBs  or { acum=0, proximo=0 }
                local esqRep = rt._kbRepEsq or { acum=0, proximo=0 }
                local dirRep = rt._kbRepDir or { acum=0, proximo=0 }
                rt._kbRepBs  = bsRep
                rt._kbRepEsq = esqRep
                rt._kbRepDir = dirRep
                rt._kbEventos = {
                    dt        = dt,
                    backspace = KB.tickRepeat(bsRep,
                                    T.teclaPressionou(T.TECLA_BACKSPACE),
                                    T.teclaSegurando(T.TECLA_BACKSPACE), dt),
                    esq       = KB.tickRepeat(esqRep,
                                    T.teclaPressionou(T.TECLA_ESQUERDA),
                                    T.teclaSegurando(T.TECLA_ESQUERDA), dt),
                    dir       = KB.tickRepeat(dirRep,
                                    T.teclaPressionou(T.TECLA_DIREITA),
                                    T.teclaSegurando(T.TECLA_DIREITA), dt),
                    tab       = T.teclaPressionou(T.TECLA_TAB),
                    enter     = T.teclaPressionou(T.TECLA_ENTER),
                    space     = KB.tickRepeat(
                                    rt._kbRepSp or (function() rt._kbRepSp={acum=0,proximo=0}; return rt._kbRepSp end)(),
                                    T.teclaPressionou(T.TECLA_ESPACO),
                                    T.teclaSegurando(T.TECLA_ESPACO), dt),
                    -- char já inclui acento + repeat interno
                    char      = _capturarCharKB(T, rt),
                    -- char pendente de acento não combinado
                    charPend  = rt._kbAcentoPendente,
                }
                rt._kbAcentoPendente = nil
            end

            -- Registra este campo na ordem de aparição
            local ordem = rt._kbOrdem
            local jaRegistrado = false
            for _, c in ipairs(ordem) do
                if c == chave then jaRegistrado = true; break end
            end
            if not jaRegistrado then
                ordem[#ordem + 1] = chave
            end

            -- Primeiro campo do frame recebe foco se ainda não há foco
            if rt._kbFoco == nil then
                rt._kbFoco = chave
            end

            local ev = rt._kbEventos

            -- TAB: avança foco (processado uma vez por frame)
            if ev.tab and rt._kbTabFrame ~= rt._frameNum then
                rt._kbTabFrame = rt._frameNum
                local n = #ordem
                if n > 1 then
                    local pos = 1
                    for i, c in ipairs(ordem) do
                        if c == rt._kbFoco then pos = i; break end
                    end
                    rt._kbFoco = ordem[(pos % n) + 1]
                end
            end

            -- Atualiza blink e aplica input só no campo focado
            st.blinkT = st.blinkT + ev.dt
            if st.blinkT >= BLINK_KB then
                st.blinkT   = st.blinkT - BLINK_KB
                st.blinkVis = not st.blinkVis
            end

            if chave == rt._kbFoco then
                if ev.backspace and st.cursor > 0 then
                    st.texto  = st.texto:sub(1, st.cursor - 1) .. st.texto:sub(st.cursor + 1)
                    st.cursor = st.cursor - 1
                    st.blinkT = 0; st.blinkVis = true
                end
                if ev.esq then
                    st.cursor = math.max(0, st.cursor - 1)
                end
                if ev.dir then
                    st.cursor = math.min(#st.texto, st.cursor + 1)
                end
                if ev.space then
                    if st.limite == 0 or #st.texto < st.limite then
                        st.texto  = st.texto:sub(1, st.cursor) .. " " .. st.texto:sub(st.cursor + 1)
                        st.cursor = st.cursor + 1
                        st.blinkT = 0; st.blinkVis = true
                    end
                end
                -- char pendente de acento não combinado (emitido no frame seguinte)
                if ev.charPend then
                    if st.limite == 0 or #st.texto < st.limite then
                        st.texto  = st.texto:sub(1, st.cursor) .. ev.charPend .. st.texto:sub(st.cursor + 1)
                        st.cursor = st.cursor + 1
                        st.blinkT = 0; st.blinkVis = true
                    end
                end
                if ev.char then
                    if st.limite == 0 or #st.texto < st.limite then
                        st.texto  = st.texto:sub(1, st.cursor) .. ev.char .. st.texto:sub(st.cursor + 1)
                        st.cursor = st.cursor + 1
                        st.blinkT = 0; st.blinkVis = true
                    end
                end
            end
        end

        -- ── FASE DRAW: renderiza o campo na tela ─────────────────────────
        if rt._modo == "draw" then
            local comFoco = (rt._kbFoco == nil) or (rt._kbFoco == chave)
            _desenharKB(rt, st, x, y, cor, comFoco)
        end

        return st.texto
    end

    -- ── Input estilo terminal: input(x, y, prefixo, limite, cor) ─────────
    -- Funciona como input() do Python ou io.read() do Lua:
    -- · Retorna nil em todo frame enquanto o usuário está digitando.
    -- · Quando o usuário pressiona Enter, retorna a string e limpa o campo.
    -- · O campo continua sendo desenhado normalmente enquanto aguarda.
    -- · Múltiplos input() na mesma tela funcionam com TAB para trocar o foco.
    --
    -- Exemplo:
    --   local name
    --   function _update()
    --     name = name or input(10, 20, "Name: ", 0, 7)
    --   end
    --   function _draw()
    --     cls(0)
    --     input(10, 20, "Name: ", 0, 7)   -- draw only; input already read
    --     if name then print("Hi " .. name, 10, 40) end
    --   end

    function api.input(x, y, prefixo, limite, cor)
        local chave = tostring(x) .. "," .. tostring(y)

        local st = rt._inputs[chave]
        if not st
        or st.prefixo ~= tostring(prefixo or "")
        or st.limite  ~= (tonumber(limite) or 0)
        then
            local txt = st and st.texto  or ""
            local cur = st and st.cursor or 0
            st = _novoEstadoKB(prefixo, limite)
            st.texto  = txt
            st.cursor = math.min(cur, #txt)
            rt._inputs[chave] = st
        end

        -- ── FASE UPDATE ───────────────────────────────────────────────────
        if rt._modo == "update" or rt._modo == "boot" then

            -- Captura compartilhada com teclado() — mesmo rt._kbFrame
            if rt._kbFrame ~= rt._frameNum then
                rt._kbFrame   = rt._frameNum
                rt._kbOrdem   = {}
                local dt = T.dt()
                local bsRep  = rt._kbRepBs  or { acum=0, proximo=0 }
                local esqRep = rt._kbRepEsq or { acum=0, proximo=0 }
                local dirRep = rt._kbRepDir or { acum=0, proximo=0 }
                local spRep  = rt._kbRepSp  or { acum=0, proximo=0 }
                rt._kbRepBs  = bsRep
                rt._kbRepEsq = esqRep
                rt._kbRepDir = dirRep
                rt._kbRepSp  = spRep
                rt._kbEventos = {
                    dt        = dt,
                    backspace = KB.tickRepeat(bsRep,
                                    T.teclaPressionou(T.TECLA_BACKSPACE),
                                    T.teclaSegurando(T.TECLA_BACKSPACE), dt),
                    esq       = KB.tickRepeat(esqRep,
                                    T.teclaPressionou(T.TECLA_ESQUERDA),
                                    T.teclaSegurando(T.TECLA_ESQUERDA), dt),
                    dir       = KB.tickRepeat(dirRep,
                                    T.teclaPressionou(T.TECLA_DIREITA),
                                    T.teclaSegurando(T.TECLA_DIREITA), dt),
                    tab       = T.teclaPressionou(T.TECLA_TAB),
                    enter     = T.teclaPressionou(T.TECLA_ENTER),
                    space     = KB.tickRepeat(spRep,
                                    T.teclaPressionou(T.TECLA_ESPACO),
                                    T.teclaSegurando(T.TECLA_ESPACO), dt),
                    char      = _capturarCharKB(T, rt),
                    charPend  = rt._kbAcentoPendente,
                }
                rt._kbAcentoPendente = nil
            end

            local ordem = rt._kbOrdem
            local jaRegistrado = false
            for _, c in ipairs(ordem) do
                if c == chave then jaRegistrado = true; break end
            end
            if not jaRegistrado then ordem[#ordem + 1] = chave end

            if rt._kbFoco == nil then rt._kbFoco = chave end

            local ev = rt._kbEventos

            if ev.tab and rt._kbTabFrame ~= rt._frameNum then
                rt._kbTabFrame = rt._frameNum
                local n = #ordem
                if n > 1 then
                    local pos = 1
                    for i, c in ipairs(ordem) do
                        if c == rt._kbFoco then pos = i; break end
                    end
                    rt._kbFoco = ordem[(pos % n) + 1]
                end
            end

            st.blinkT = st.blinkT + ev.dt
            if st.blinkT >= BLINK_KB then
                st.blinkT   = st.blinkT - BLINK_KB
                st.blinkVis = not st.blinkVis
            end

            if chave == rt._kbFoco then
                -- Enter: confirma, limpa o campo, retorna o valor
                if ev.enter then
                    local confirmado = st.texto
                    st.texto  = ""
                    st.cursor = 0
                    st.blinkT = 0; st.blinkVis = true
                    return confirmado
                end
                if ev.backspace and st.cursor > 0 then
                    st.texto  = st.texto:sub(1, st.cursor - 1) .. st.texto:sub(st.cursor + 1)
                    st.cursor = st.cursor - 1
                    st.blinkT = 0; st.blinkVis = true
                end
                if ev.esq then st.cursor = math.max(0, st.cursor - 1) end
                if ev.dir then st.cursor = math.min(#st.texto, st.cursor + 1) end
                if ev.space then
                    if st.limite == 0 or #st.texto < st.limite then
                        st.texto  = st.texto:sub(1, st.cursor) .. " " .. st.texto:sub(st.cursor + 1)
                        st.cursor = st.cursor + 1
                        st.blinkT = 0; st.blinkVis = true
                    end
                end
                if ev.charPend then
                    if st.limite == 0 or #st.texto < st.limite then
                        st.texto  = st.texto:sub(1, st.cursor) .. ev.charPend .. st.texto:sub(st.cursor + 1)
                        st.cursor = st.cursor + 1
                        st.blinkT = 0; st.blinkVis = true
                    end
                end
                if ev.char then
                    if st.limite == 0 or #st.texto < st.limite then
                        st.texto  = st.texto:sub(1, st.cursor) .. ev.char .. st.texto:sub(st.cursor + 1)
                        st.cursor = st.cursor + 1
                        st.blinkT = 0; st.blinkVis = true
                    end
                end
            end
        end

        -- ── FASE DRAW ─────────────────────────────────────────────────────
        if rt._modo == "draw" then
            local comFoco = (rt._kbFoco == nil) or (rt._kbFoco == chave)
            _desenharKB(rt, st, x, y, cor, comFoco)
        end

        return nil  -- ainda aguardando Enter
    end

    function api.mouse()            return T.mousePos()                             end
    function api.mouseX()           return T.mouseX()                               end
    function api.mouseY()           return T.mouseY()                               end
    function api.mouseBotao(b)      return T.mouseSegurando(b or T.MOUSE_ESQ)       end
    function api.mouseClicou(b)     return T.mouseClicou(b or T.MOUSE_ESQ)          end
    function api.scroll()           return T.scrollY()                              end

    -- ── Tempo / Matemática ────────────────────────────────────────────────

    function api.tempo()            return T.tempo()                 end
    function api.delta()            return T.dt()                    end
    function api.fpsLimite(fps)     T.fpsLimite(fps or 0)           end
    function api.fpsAtual()         return T.fpsAtual()              end

    function api.aleatorio(a, b)
        if a == nil then return math.random() end
        if b == nil then return math.random() * a end
        return T.aleatorio(a, b)
    end

    function api.interpolar(a, b, t)        return T.lerp(a, b, t)           end
    function api.distancia(x1, y1, x2, y2)  return T.distancia(x1, y1, x2, y2) end
    function api.radianos(g)                return T.rad(g)                  end
    function api.graus(r)                   return T.graus(r)                end

    api.chao  = math.floor
    api.teto  = math.ceil
    api.abs   = math.abs
    api.sen   = math.sin
    api.cos   = math.cos
    api.tan   = math.tan
    api.raiz  = math.sqrt
    api.min   = math.min
    api.max   = math.max
    api.meio  = function(a, b, c) return math.max(a, math.min(b, c)) end
    api.pi    = math.pi

    -- ── English / PICO-8 style API ───────────────────────────────────────

    api.cls         = api.limpar
    api.color       = api.cor
    api.print       = api.escrever
    api.pset        = api.pixel
    api.rectfill    = api.retangulo
    api.rect        = api.bordaRet
    api.circfill    = api.circulo
    api.circ        = api.bordaCirc
    api.line        = api.linha

    api.image       = api.imagem
    api.object      = api.objeto
    api.draw_sprite = api.desenharSprite
    api.draw        = api.mostrar

    api.move        = api.mover
    api.set_pos     = api.posicionar
    api.get_pos     = api.posicao
    api.scale       = api.escala
    api.alpha       = api.transparencia
    api.size        = api.tamanho
    api.frame       = api.quadro
    api.flip        = api.espelhar
    api.destroy     = api.destruir

    api.overlap     = api.colidiu
    api.new_anim    = api.criarAnim
    api.anim_done   = api.animTerminou

    api.get_camera    = api.pegarCamera
    api.camera_zoom   = api.zoomCamera
    api.camera_move   = api.moverCamera
    api.camera_follow = api.seguirCamera

    api.btn        = api.botao
    api.btnp       = api.pressionou
    api.btnr       = api.soltou
    api.textfield  = api.teclado
    api.mousebtn   = api.mouseBotao
    api.mouseclick = api.mouseClicou

    api.time       = api.tempo
    api.set_fps    = api.fpsLimite
    api.get_fps    = api.fpsAtual
    api.fps        = api.fpsAtual
    api.rnd        = api.aleatorio
    api.lerp       = api.interpolar
    api.dist       = api.distancia
    api.rad        = api.radianos
    api.deg        = api.graus

    api.flr        = math.floor
    api.ceil       = math.ceil
    api.sin        = math.sin
    api.sqrt       = math.sqrt
    api.mid        = function(a, b, c) return math.max(a, math.min(b, c)) end

    api.set_repeat = KB.setRepeat
    api.set_layout = KB.setLayout

    -- ── Legacy PT-BR names kept for compatibility ────────────────────────

    api.lmp   = api.cls
    api.esc   = api.print
    api.ret   = api.rectfill
    api.bret  = api.rect
    api.bcirc = api.circ
    api.lin   = api.line
    api.pix   = api.pset
    api.img   = api.image
    api.obj   = api.object
    api.spr   = api.draw_sprite
    api.ver   = api.draw
    api.mov   = api.move
    api.pos   = api.set_pos
    api.xy    = api.get_pos
    api.btns  = api.btnr
    api.mbtn  = api.mousebtn
    api.mclk  = api.mouseclick
    api.mx    = api.mouseX
    api.my    = api.mouseY
    api.dt    = api.delta
    api.anim  = api.new_anim
    api.play  = api.tocarAnim
    api.stop  = api.pararAnim
    api.inp   = api.input

    -- configura velocidade da tecla contínua: set_repeat(atraso, passo)
    api.setRepeat  = KB.setRepeat
    api.setLayout  = KB.setLayout

    -- ── Paleta / Debug ────────────────────────────────────────────────────

    api.pal     = PALETA
    api.palette = PALETA
    api.paleta  = PALETA

    api.engine = T
    api.motor  = T
    api.Tupi   = T

    api.debug   = _G.print
    api.depurar = _G.print

    return api
end

function Runtime:_criarAmbiente()
    local api = self:_criarAPI()
    return setmetatable({}, {
        __index = function(_, chave)
            if api[chave] ~= nil then return api[chave] end
            if self.T[chave] ~= nil then return self.T[chave] end
            return _G[chave]
        end,
        __newindex = function(t, chave, valor)
            rawset(t, chave, valor)
        end,
    })
end

function Runtime:_carregarChunk(path, env)
    local chunk, err = loadfile(path)
    if not chunk then return nil, err end
    ---@diagnostic disable-next-line: deprecated
    if setfenv then
        ---@diagnostic disable-next-line: deprecated
        setfenv(chunk, env)
    end
    return chunk
end

function Runtime:_erroRuntime(msg)
    self._erro = tostring(msg)
end

function Runtime:_chamar(fn)
    local ok, err = pcall(fn)
    if not ok then
        self:_erroRuntime(err)
        return false
    end
    return true
end

function Runtime:rodarArquivo(path, origem)
    self._arquivo  = path
    self._origem   = origem or "console"
    self._retorno  = nil
    self._erro     = nil
    self._env      = self:_criarAmbiente()
    self._callbacks = {}
    self._opsBoot  = {}
    self._opsFrame = {}
    self._inputs      = {}   -- estado de cada campo teclado()
    self._frameNum    = 0    -- incrementado a cada atualizar(), identifica o frame
    self._kbFrame     = -1   -- frame em que os eventos foram capturados
    self._kbOrdem     = {}   -- ordem de aparição dos campos no frame
    self._kbFoco      = nil  -- chave do campo com foco
    self._kbEventos   = {}   -- eventos capturados uma vez por frame
    self._kbTabFrame  = -1   -- frame em que TAB foi processado
    -- key repeat por tecla
    self._kbRepBs     = { acum=0, proximo=0 }  -- backspace
    self._kbRepEsq    = { acum=0, proximo=0 }  -- seta esquerda
    self._kbRepDir    = { acum=0, proximo=0 }  -- seta direita
    self._kbRepSp     = { acum=0, proximo=0 }  -- espaço
    self._kbRepeat    = { code=nil, acum=0, proximo=0 }  -- char atual
    -- acentos mortos
    self._kbAcento        = nil   -- acento pendente aguardando vogal
    self._kbAcentoPendente = nil  -- char não combinado para emitir no próximo frame

    local chunk, err = self:_carregarChunk(path, self._env)
    if not chunk then
        self._ativo = true
        self:_erroRuntime(err)
        return false, err
    end

    self._modo       = "boot"
    self._opsDestino = self._opsBoot
    self._estadoAtivo = self:_novoEstado()
    if not self:_chamar(chunk) then
        self._ativo = true
        self._modo  = nil
        return false, self._erro
    end

    -- Main callbacks: _init / _update / _draw
    -- (still accepts the old PT-BR names for compatibility)
    self._callbacks.init   = self._env._iniciar  or self._env._init
    self._callbacks.update = self._env._rodar    or self._env._update    or self._env._atualizar
    self._callbacks.draw   = self._env._desenhar or self._env._draw

    if type(self._callbacks.init) == "function" then
        self._estadoAtivo = self:_novoEstado()
        self._opsDestino  = self._opsBoot
        if not self:_chamar(self._callbacks.init) then
            self._ativo = true
            self._modo  = nil
            return false, self._erro
        end
    end

    self._modo        = nil
    self._estadoAtivo = nil
    self._ativo       = true
    return true
end

function Runtime:atualizar()
    if not self._ativo then return end
    self._frameNum = (self._frameNum or 0) + 1
    local ctrl = self.T.teclaSegurando(self.T.TECLA_CTRL_ESQ)
              or self.T.teclaSegurando(self.T.TECLA_CTRL_DIR)
    if (ctrl and self.T.teclaPressionou(self.T.TECLA_0))
    or self.T.teclaPressionou(self.T.TECLA_ESC) then
        self:parar()
        return
    end
    if self._erro then return end

    self._opsFrame = {}
    if type(self._callbacks.update) == "function" then
        self._modo        = "update"
        self._opsDestino  = self._opsFrame
        self._estadoAtivo = self:_novoEstado()
        self:_chamar(self._callbacks.update)
        self._modo        = nil
        self._estadoAtivo = nil
    end
end

function Runtime:desenhar()
    if not self._ativo then return end

    local T = self.T
    T.cor(1, 1, 1, 1)

    if self._erro then
        T.retangulo(0, 0, T.largura(), T.altura(), PALETA[0])
        T.cor(PALETA[8][1], PALETA[8][2], PALETA[8][3], 1)
        T.texto.desenhar(4, 4, 20, "erro em run.lua", ESCALA_FONTE, 1.0, self._fonte)
        T.cor(PALETA[7][1], PALETA[7][2], PALETA[7][3], 1)
        T.texto.desenhar(4, 14, 20, tostring(self._erro), ESCALA_FONTE, 1.0, self._fonte)
        T.cor(PALETA[6][1], PALETA[6][2], PALETA[6][3], 1)
        T.texto.desenhar(4, T.altura() - 10, 20, "ctrl+0 volta", ESCALA_FONTE, 1.0, self._fonte)
        T.cor(1, 1, 1, 1)
        return
    end

    local estado = self:_novoEstado()
    self:_replay(self._opsBoot,  estado)
    self:_replay(self._opsFrame, estado)

    if type(self._callbacks.draw) == "function" then
        self._modo        = "draw"
        self._estadoAtivo = estado
        self:_chamar(self._callbacks.draw)
        self._modo        = nil
        self._estadoAtivo = nil
    end
end

return Sintaxe