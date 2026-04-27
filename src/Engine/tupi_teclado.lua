local LAYOUTS = {}

-- ── US QWERTY ────────────────────────────────────────────────
LAYOUTS["us"] = {
    nome = "US QWERTY",
    simb = {
        {c=45,  n="-",  s="_"},
        {c=61,  n="=",  s="+"},
        {c=91,  n="[",  s="{"},
        {c=93,  n="]",  s="}"},
        {c=92,  n="\\", s="|"},
        {c=59,  n=";",  s=":"},
        {c=39,  n="'",  s='"'},
        {c=44,  n=",",  s="<"},
        {c=46,  n=".",  s=">"},
        {c=47,  n="/",  s="?"},
        {c=96,  n="`",  s="~"},
        {c=32,  n=" ",  s=" "},
    },
    nums = {
        [0]=")", [1]="!", [2]="@", [3]="#", [4]="$",
        [5]="%", [6]="^", [7]="&", [8]="*", [9]="(",
    },
}

LAYOUTS["abnt2"] = {
    nome = "ABNT2 (BR)",
    simb = {
        {c=45,  n="-",  s="_"},
        {c=61,  n="=",  s="+"},
        {c=91,  n="'",  s="`"},   -- tecla ´ / `
        {c=93,  n="[",  s="{"},
        {c=92,  n="]",  s="}"},
        {c=59,  n="c",  s="C"},   -- ç/Ç (fonte 8bit não tem ç; usa c/C)
        {c=39,  n="~",  s="^"},
        {c=44,  n=",",  s="<"},
        {c=46,  n=".",  s=">"},
        {c=47,  n=";",  s=":"},
        {c=96,  n="'",  s='"'},
        {c=32,  n=" ",  s=" "},
        {c=226, n="\\", s="|"},   -- barra extra entre LShift e Z
    },
    nums = {
        [0]=")", [1]="!", [2]="@", [3]="#", [4]="$",
        [5]="%", [6]='"', [7]="&", [8]="*", [9]="(",
    },
}

-- ============================================================
-- ACENTOS MORTOS (dead keys)
-- Digitar a tecla do acento + vogal gera o caractere combinado.
--   ~+a=ã  ´+e=é  ^+o=ô  `+a=à  "+u=ü
-- Digitar o mesmo acento duas vezes emite o próprio acento.
-- ============================================================

local ACENTOS = {
    ["~"]  = { a="ã", A="Ã", o="õ", O="Õ", n="ñ", N="Ñ" },
    ["'"]  = { a="á", A="Á", e="é", E="É", i="í", I="Í",
               o="ó", O="Ó", u="ú", U="Ú", c="ć", C="Ć" },
    ["`"]  = { a="à", A="À", e="è", E="È", i="ì", I="Ì",
               o="ò", O="Ò", u="ù", U="Ù" },
    ["^"]  = { a="â", A="Â", e="ê", E="Ê", i="î", I="Î",
               o="ô", O="Ô", u="û", U="Û" },
    ['"']  = { a="ä", A="Ä", e="ë", E="Ë", i="ï", I="Ï",
               o="ö", O="Ö", u="ü", U="Ü" },
}

-- ============================================================
-- KEY REPEAT — tempo de tecla contínua, estilo terminal
-- ============================================================

local _repeat_atraso = 0.40   -- segundos até começar a repetir
local _repeat_passo  = 0.03   -- segundos entre repetições (~33 cps)

-- ============================================================
-- ESTADO
-- ============================================================
local _ativo = "us"   -- padrão: US QWERTY

-- ============================================================
-- API PÚBLICA
-- ============================================================
local KB = {}

function KB.setLayout(id)
    id = tostring(id or ""):lower()
    if id == "br" then id = "abnt2" end
    if LAYOUTS[id] then
        _ativo = id
        return true
    end
    return false
end

function KB.getLayout()  return _ativo end
function KB.getNome()    return LAYOUTS[_ativo].nome end
function KB.getSimb()    return LAYOUTS[_ativo].simb end

function KB.getNumShift(d)
    local n = LAYOUTS[_ativo].nums[d]
    return n or tostring(d)
end

function KB.listar()
    local out = {}
    for id, lay in pairs(LAYOUTS) do
        out[#out+1] = { id=id, nome=lay.nome, ativo=(id==_ativo) }
    end
    table.sort(out, function(a,b) return a.id < b.id end)
    return out
end

-- ── Acentos ──────────────────────────────────────────────────

-- Retorna true se o char é uma tecla morta (acento)
function KB.ehAcento(c)   return ACENTOS[c] ~= nil end

-- Tenta combinar acento + vogal.
-- Retorna o char acentuado, ou o acento sozinho se não combina,
-- ou nil se acento for nil.
function KB.combinar(acento, vogal)
    if not acento then return vogal end
    if vogal == acento then return acento end   -- ~~ → ~
    local t = ACENTOS[acento]
    if t and t[vogal] then
        return t[vogal]
    end
    -- não combinou: emite o acento pendente e a nova tecla separados
    return nil  -- chamador trata esse caso
end

function KB.getAcentos() return ACENTOS end

-- ── Key Repeat ────────────────────────────────────────────────

-- Configura os tempos de repetição de tecla (em segundos)
function KB.setRepeat(atraso, passo)
    _repeat_atraso = atraso or _repeat_atraso
    _repeat_passo  = passo  or _repeat_passo
end

function KB.getRepeat() return _repeat_atraso, _repeat_passo end

-- Atualiza o estado de repetição de UMA tecla e retorna true quando
-- ela deve disparar (pressionou agora OU repeat atingiu o passo).
-- `estado_rep` é uma tabela {acum, proximo} guardada por quem chama.
-- `pressionou` = bool, `segurando` = bool, `dt` = delta time.
function KB.tickRepeat(estado_rep, pressionou, segurando, dt)
    if pressionou then
        estado_rep.acum    = 0
        estado_rep.proximo = _repeat_atraso
        return true
    end
    if not segurando then
        estado_rep.acum    = 0
        estado_rep.proximo = _repeat_atraso
        return false
    end
    estado_rep.acum = (estado_rep.acum or 0) + dt
    if estado_rep.acum >= estado_rep.proximo then
        estado_rep.proximo = estado_rep.proximo + _repeat_passo
        return true
    end
    return false
end

return KB
