local LAYOUTS = {}
local function L(n) return string.char(n) end

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
        {c=91,  n="'",  s="`"},
        {c=93,  n="[",  s="{"},
        {c=92,  n="]",  s="}"},
        {c=59,  n="c",  s="C"},
        {c=39,  n="~",  s="^"},
        {c=44,  n=",",  s="<"},
        {c=46,  n=".",  s=">"},
        {c=47,  n=";",  s=":"},
        {c=96,  n="'",  s='"'},
        {c=32,  n=" ",  s=" "},
        {c=226, n="\\", s="|"},
    },
    nums = {
        [0]=")", [1]="!", [2]="@", [3]="#", [4]="$",
        [5]="%", [6]='"', [7]="&", [8]="*", [9]="(",
    },
}

local ACENTOS = {
    ["~"]  = { a=L(227), A=L(195), o=L(245), O=L(213), n=L(241), N=L(209) },
    ["'"]  = { a=L(225), A=L(193), e=L(233), E=L(201), i=L(237), I=L(205),
               o=L(243), O=L(211), u=L(250), U=L(218), c="c", C="C" },
    ["`"]  = { a=L(224), A=L(192), e=L(232), E=L(200), i=L(236), I=L(204),
               o=L(242), O=L(210), u=L(249), U=L(217) },
    ["^"]  = { a=L(226), A=L(194), e=L(234), E=L(202), i=L(238), I=L(206),
               o=L(244), O=L(212), u=L(251), U=L(219) },
    ['"']  = { a=L(228), A=L(196), e=L(235), E=L(203), i=L(239), I=L(207),
               o=L(246), O=L(214), u=L(252), U=L(220) },
}

local _repeat_atraso = 0.40
local _repeat_passo  = 0.03
local _ativo = "us"
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

function KB.ehAcento(c)   return ACENTOS[c] ~= nil end

function KB.combinar(acento, vogal)
    if not acento then return vogal end
    if vogal == acento then return acento end   -- ~~ → ~
    local t = ACENTOS[acento]
    if t and t[vogal] then
        return t[vogal]
    end
    return nil
end

function KB.getAcentos() return ACENTOS end

function KB.setRepeat(atraso, passo)
    _repeat_atraso = atraso or _repeat_atraso
    _repeat_passo  = passo  or _repeat_passo
end

function KB.getRepeat() return _repeat_atraso, _repeat_passo end

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
