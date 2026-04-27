local N = require("src.Engine.texto_normalizar")

-- paleta de cores da splash
local C = {
    PRETO  = {0.000, 0.000, 0.000},
    ESCURO = {0.040, 0.045, 0.080},
    CINZA  = {0.250, 0.235, 0.220},
    PRATA  = {0.550, 0.540, 0.530},
    BRANCO = {0.960, 0.940, 0.900},
    CIANO  = {0.200, 0.720, 0.900},
    VERDE  = {0.180, 0.780, 0.380},
}

-- arte ASCII do logo
local LOGO = {
    " _____ _   _ ___ ___ ",
    "|_   _| | | | _ \\_ _|",
    "  | | | |_| |  _/| | ",
    "  |_|  \\___/|_| |___|",
}
local LOGO_SUB = "ENGINE EDITION"

-- sequência de mensagens de boot com seus timestamps e cores
local BOOT = {
    {t=0.10, txt="inicializando opengl...", cor=nil},
    {t=0.28, txt="carregando renderer...",  cor=nil},
    {t=0.46, txt="verificando ffi...",      cor=nil},
    {t=0.62, txt="luajit: ok",             cor="VERDE"},
    {t=0.76, txt="rust core: ok",          cor="VERDE"},
    {t=0.90, txt="batch renderer: ok",     cor="VERDE"},
    {t=1.05, txt="fonte bitmap: ok",       cor="VERDE"},
    {t=1.18, txt="pronto.",                cor="CIANO"},
}

local ESC  = 0.5   -- escala da fonte bitmap
local CW   = 8 * ESC
local CH   = 8 * ESC

local Splash = {}
Splash.__index = Splash

-- cria uma nova splash screen
function Splash.novo(Tupi, ascii, duracao)
    assert(Tupi,  "[Splash] Tupi obrigatorio")
    assert(ascii, "[Splash] caminho do ascii.png obrigatorio")

    local self    = setmetatable({}, Splash)
    self.T        = Tupi
    self._t       = 0
    self._dur     = duracao or 2.0
    self._done    = false
    self._pronto  = false
    self._fonte   = Tupi.texto.carregarFonte(ascii, 8, 8)
    N.patchTexto(Tupi.texto)

    return self
end

-- retorna true quando a splash foi concluída
function Splash:terminou() return self._done end

function Splash:atualizar()
    if self._done then return end
    local T  = self.T
    if not self._pronto then
        -- avança o timer até o fim da duração
        self._t = math.min(self._t + T.dt(), self._dur)
        if self._t >= self._dur then
            self._pronto = true
        end
        return
    end

    -- aguarda Enter para confirmar
    if T.teclaPressionou(T.TECLA_ENTER) then
        self._done = true
    end
end

function Splash:desenhar()
    local T    = self.T
    local W    = T.largura()
    local H    = T.altura()
    local t    = self._t
    local f    = self._fonte
    local prog = math.min(t / (self._dur * 0.7), 1.0)

    local alpha_in = math.min(t / 0.3, 1.0)

    -- fundo preto
    T.cor(C.PRETO[1], C.PRETO[2], C.PRETO[3], 1)
    T.retangulo(0, 0, W, H)

    -- logo centralizado com slide de entrada
    local logo_cw   = #LOGO[1] * CW
    local logo_ch   = #LOGO * (CH + 1)
    local lx        = math.floor((W - logo_cw) * 0.5)

    local slide     = (1 - _easeOut(math.min(t / 0.4, 1))) * 16
    local ly        = math.floor(H * 0.20) + slide

    for i, linha in ipairs(LOGO) do
        local fator = 1 - (i - 1) / #LOGO * 0.35
        T.cor(C.CIANO[1] * fator, C.CIANO[2] * fator, C.CIANO[3] * fator, alpha_in)
        T.texto.desenhar(lx, ly + (i - 1) * (CH + 1), 5, linha, ESC, alpha_in, f)
    end

    -- subtítulo abaixo do logo
    local sub_x = math.floor((W - #LOGO_SUB * CW) * 0.5)
    local sub_y = ly + logo_ch + 2
    T.cor(C.PRATA[1], C.PRATA[2], C.PRATA[3], alpha_in * 0.8)
    T.texto.desenhar(sub_x, sub_y, 5, LOGO_SUB, ESC, alpha_in * 0.8, f)

    T.cor(1, 1, 1, 1)

    -- barra de progresso
    local bar_w = math.floor(W * 0.55)
    local bar_h = 2
    local bar_x = math.floor((W - bar_w) * 0.5)
    local bar_y = sub_y + CH + 8

    T.cor(C.ESCURO[1], C.ESCURO[2], C.ESCURO[3], 1)
    T.retangulo(bar_x, bar_y, bar_w, bar_h)

    local fill = math.floor(bar_w * prog)
    if fill > 0 then
        T.cor(C.CIANO[1], C.CIANO[2], C.CIANO[3], alpha_in)
        T.retangulo(bar_x, bar_y, fill, bar_h)

        T.cor(C.BRANCO[1], C.BRANCO[2], C.BRANCO[3], alpha_in * 0.6)
        T.retangulo(bar_x + fill - 1, bar_y, 1, bar_h)
    end

    T.cor(1, 1, 1, 1)

    -- mensagens de boot aparecem conforme o tempo avança
    local msg_x = math.floor(W * 0.15)
    local msg_y = bar_y + bar_h + 6

    for i, m in ipairs(BOOT) do
        if t >= m.t then
            local delay  = m.t
            local a_msg  = math.min((t - delay) / 0.1, 1.0) * alpha_in
            local pal    = m.cor and C[m.cor] or C.PRATA
            T.cor(pal[1], pal[2], pal[3], a_msg)
            T.texto.desenhar(msg_x, msg_y + (i - 1) * (CH + 2), 4, m.txt, ESC, a_msg, f)
        end
    end

    T.cor(1, 1, 1, 1)

    -- instruções na parte inferior
    if self._pronto then
        local pulso = 0.55 + (math.sin(T.tempo() * 4.0) * 0.20)
        T.cor(C.BRANCO[1], C.BRANCO[2], C.BRANCO[3], pulso)
        T.texto.desenhar(4, H - CH - 6, 8, "pressione enter", ESC, pulso, f)
    elseif t > 0.5 then
        local a_skip = math.min((t - 0.5) / 0.3, 1.0) * 0.4
        T.cor(C.CINZA[1], C.CINZA[2], C.CINZA[3], a_skip)
        T.texto.desenhar(4, H - CH - 2, 8, "carregando...", ESC, a_skip, f)
        T.cor(1, 1, 1, 1)
    end

    -- fade para preto no final da animação
    local fade_inicio = self._dur * 0.82
    if (not self._pronto) and t > fade_inicio then
        local fade_a = (t - fade_inicio) / (self._dur - fade_inicio)
        T.cor(C.PRETO[1], C.PRETO[2], C.PRETO[3], math.min(fade_a, 1.0))
        T.retangulo(0, 0, W, H)
        T.cor(1, 1, 1, 1)
    end

    T.batchDesenhar()
end

-- easing quadrático de saída (desacelera no final)
function _easeOut(t)
    return 1 - (1 - t) * (1 - t)
end

return Splash