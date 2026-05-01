local M = {}
---@diagnostic disable-next-line: deprecated
local unpack = table.unpack or unpack

-- converte caracteres UTF-8 comuns para seus equivalentes Latin-1
local UTF8_PARA_LATIN1 = {

    ["à"]=string.char(224), ["á"]=string.char(225), ["â"]=string.char(226),
    ["ã"]=string.char(227), ["ä"]=string.char(228), ["å"]=string.char(229),
    ["è"]=string.char(232), ["é"]=string.char(233), ["ê"]=string.char(234),
    ["ë"]=string.char(235),
    ["ì"]=string.char(236), ["í"]=string.char(237), ["î"]=string.char(238),
    ["ï"]=string.char(239),
    ["ò"]=string.char(242), ["ó"]=string.char(243), ["ô"]=string.char(244),
    ["õ"]=string.char(245), ["ö"]=string.char(246),
    ["ù"]=string.char(249), ["ú"]=string.char(250), ["û"]=string.char(251),
    ["ü"]=string.char(252),
    ["ý"]=string.char(253), ["ÿ"]=string.char(255),
    ["ç"]=string.char(231), ["ñ"]=string.char(241),

    ["À"]=string.char(192), ["Á"]=string.char(193), ["Â"]=string.char(194),
    ["Ã"]=string.char(195), ["Ä"]=string.char(196), ["Å"]=string.char(197),
    ["È"]=string.char(200), ["É"]=string.char(201), ["Ê"]=string.char(202),
    ["Ë"]=string.char(203),
    ["Ì"]=string.char(204), ["Í"]=string.char(205), ["Î"]=string.char(206),
    ["Ï"]=string.char(207),
    ["Ò"]=string.char(210), ["Ó"]=string.char(211), ["Ô"]=string.char(212),
    ["Õ"]=string.char(213), ["Ö"]=string.char(214),
    ["Ù"]=string.char(217), ["Ú"]=string.char(218), ["Û"]=string.char(219),
    ["Ü"]=string.char(220),
    ["Ý"]=string.char(221), ["Ç"]=string.char(199), ["Ñ"]=string.char(209),

    -- aspas e pontuação tipográfica → equivalentes ASCII
    ["\xE2\x80\x99"]="'",  ["\xE2\x80\x98"]="'",
    ["\xE2\x80\x9C"]='"',  ["\xE2\x80\x9D"]='"',
    ["\xE2\x80\x93"]="-",  ["\xE2\x80\x94"]="-",
    ["\xE2\x80\xA6"]="...",
}

-- mapa extra para caracteres fora do Latin-1, preenchido pelo usuário se necessário
M.MAPA_EXTRA = {}

-- true se o byte é um byte de continuação UTF-8 (10xxxxxx)
local function _cont(b)
    return b and b >= 0x80 and b <= 0xBF
end

-- retorna quantos bytes o caractere UTF-8 ocupa a partir da posição i
local function _utf8_len_valido(s, i)
    local b1 = string.byte(s, i)
    if not b1 then return nil end
    if b1 < 0x80 then return 1 end

    local b2 = string.byte(s, i + 1)
    local b3 = string.byte(s, i + 2)
    local b4 = string.byte(s, i + 3)

    if b1 >= 0xC2 and b1 <= 0xDF and _cont(b2) then
        return 2
    end
    if b1 >= 0xE0 and b1 <= 0xEF and _cont(b2) and _cont(b3) then
        return 3
    end
    if b1 >= 0xF0 and b1 <= 0xF4 and _cont(b2) and _cont(b3) and _cont(b4) then
        return 4
    end
    return 1
end

-- iterador: percorre a string caractere por caractere respeitando UTF-8
local function _utf8_chars(s)
    local i = 1
    return function()
        if i > #s then return nil end
        local len = _utf8_len_valido(s, i)
        local ch = s:sub(i, i + len - 1)
        i = i + len
        return ch
    end
end

-- converte string UTF-8 para Latin-1, descartando o que não tem mapeamento
function M.limpar(s)
    if type(s) ~= "string" then return tostring(s) end
    local out = {}
    for ch in _utf8_chars(s) do
        if #ch == 1 then
            out[#out + 1] = ch
        else
            out[#out + 1] = UTF8_PARA_LATIN1[ch] or ""
        end
    end
    return table.concat(out)
end

-- igual a limpar(), mas também consulta MAPA_EXTRA antes de descartar
function M.limparExtra(s)
    if type(s) ~= "string" then return tostring(s) end
    local out = {}
    for ch in _utf8_chars(s) do
        if #ch == 1 then
            out[#out + 1] = ch
        elseif M.MAPA_EXTRA[ch] then
            out[#out + 1] = string.char(M.MAPA_EXTRA[ch])
        elseif UTF8_PARA_LATIN1[ch] then
            out[#out + 1] = UTF8_PARA_LATIN1[ch]
        end
    end
    return table.concat(out)
end

-- monkey-patch em Tupi.texto para normalizar strings automaticamente antes de desenhar
-- modo: "latin1" (padrão) ou "extra" (usa MAPA_EXTRA também)
function M.patchTexto(Texto, modo)
    assert(type(Texto) == "table", "[texto_normalizar] passe Tupi.texto")
    if Texto._normalizado then return Texto end
    modo = modo or "latin1"
    local fn = (modo == "extra") and M.limparExtra or M.limpar

    -- funções que recebem a string no argumento de índice idx
    local alvos = {
        { "desenhar",       4 },
        { "desenharSombra", 6 },
        { "desenharCaixa",  6 },
        { "largura",        2 },
        { "altura",         2 },
        { "dimensoes",      2 },
    }

    for _, v in ipairs(alvos) do
        local nome, idx = v[1], v[2]
        local original = Texto[nome]
        if type(original) == "function" then
            -- substitui a função original por uma versão que normaliza o argumento de texto
            Texto[nome] = function(...)
                local args = { ... }
                if type(args[idx]) == "string" then
                    args[idx] = fn(args[idx])
                end
                return original(unpack(args))
            end
        end
    end

    Texto._normalizado = true
    return Texto
end

return M