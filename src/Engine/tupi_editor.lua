---@diagnostic disable: undefined-global

local KB = require("src.Engine.tupi_teclado")

-- paleta de cores estilo PICO-8
local P8 = {
    [0]  = {0.000, 0.000, 0.000},
    [1]  = {0.114, 0.169, 0.325},
    [2]  = {0.494, 0.145, 0.325},
    [3]  = {0.000, 0.529, 0.318},
    [4]  = {0.671, 0.322, 0.212},
    [5]  = {0.373, 0.341, 0.310},
    [6]  = {0.761, 0.765, 0.780},
    [7]  = {1.000, 0.945, 0.910},
    [8]  = {1.000, 0.000, 0.302},
    [9]  = {1.000, 0.639, 0.000},
    [10] = {1.000, 0.925, 0.153},
    [11] = {0.000, 0.894, 0.212},
    [12] = {0.161, 0.678, 1.000},
    [13] = {1.000, 0.467, 0.659},
    [14] = {1.000, 0.467, 0.659},
    [15] = {1.000, 0.800, 0.600},
}

local function p8(n) return P8[n] or P8[6] end

-- cores do editor
local C = {
    FUNDO        = {0.03,  0.05,  0.09 },
    FUNDO_LINE   = {0.10,  0.12,  0.18 },
    FUNDO_SEL    = {0.16,  0.22,  0.31 },
    GUTTER       = {0.07,  0.08,  0.13 },
    BARRA        = {0.05,  0.07,  0.12 },
    SEPARATOR    = {0.18,  0.24,  0.34 },

    TEXTO        = p8(6),
    OPACO        = p8(5),
    NUM_ATIVO    = p8(9),

    SYN_FLUXO    = p8(8),
    SYN_ESCOPO   = p8(8),
    SYN_FUNCAO   = p8(9),
    SYN_CHAMADA  = p8(3),
    SYN_STRING   = p8(10),
    SYN_BUILTIN  = p8(12),
    SYN_NUMEROS  = p8(11),
    SYN_COMMENT  = p8(5),
    SYN_OPERADOR = p8(13),
    SYN_VARIAVEL = p8(15),
    SYN_BOOLEANO = p8(2),

    SYN_PALAVRA  = p8(8),

    CURSOR       = p8(10),
    STATUS_OK    = p8(11),
    STATUS_ERR   = p8(8),
    STATUS_INFO  = p8(12),
    PROMPT_BG    = {0.06,  0.08,  0.14 },
    PROMPT_BRD   = {0.23,  0.39,  0.62 },
    CIANO        = p8(12),
}

-- palavras-chave de fluxo do Lua
local KW_FLUXO = {}
for _, k in ipairs({
    "and","break","do","else","elseif","end","for",
    "goto","if","in","not","or","repeat","return","then",
    "until","while",
}) do KW_FLUXO[k] = true end

local KW_ESCOPO = { ["local"]=true, ["function"]=true }

local BOOL_NIL = { ["true"]=true, ["false"]=true, ["nil"]=true }

-- funções builtin destacadas pelo syntax highlight
local BUILTIN = {}
for _, b in ipairs({
    "print","pairs","ipairs","next","type","tostring","tonumber",
    "error","assert","pcall","xpcall","rawget","rawset","rawequal","rawlen",
    "select","unpack","table","string","math","io","os","coroutine",
    "require","load","loadfile","dofile","collectgarbage","gcinfo",
    "setmetatable","getmetatable","setfenv","getfenv",

    "limpar","cor","camera","cls","color",
    "escrever","print",
    "pixel","retangulo","bordaRet","circulo","bordaCirc","linha",
    "pset","rectfill","rect","circfill","circ","line",
    "imagem","objeto","desenharSprite","mostrar",
    "image","object","draw_sprite","draw",
    "mover","posicionar","posicao","escala","transparencia","tamanho","quadro",
    "espelhar","destruir",
    "move","set_pos","get_pos","scale","alpha","size","frame","flip","destroy",
    "colidiu","hitbox","overlap",
    "criarAnim","tocarAnim","pararAnim","animTerminou",
    "new_anim","play","stop","anim_done",
    "pegarCamera","zoomCamera","moverCamera","seguirCamera",
    "get_camera","camera_zoom","camera_move","camera_follow",
    "botao","pressionou","soltou","teclado","input","setRepeat","setLayout",
    "btn","btnp","btnr","textfield","set_repeat","set_layout",
    "mouse","mouseX","mouseY","mouseBotao","mouseClicou","scroll",
    "mousebtn","mouseclick","mx","my",
    "tempo","delta","aleatorio","interpolar","distancia","radianos","graus",
    "chao","teto","abs","sen","cos","tan","raiz","min","max","meio","pi",
    "time","rnd","lerp","dist","rad","deg","flr","ceil","sin","sqrt","mid",
    "depurar","debug",

    "lmp","esc","ret","bret","circ","bcirc","lin","pix",
    "img","obj","spr","ver",
    "mov","pos","xy",
    "btn","btnp","btns","mbtn","mclk","mx","my","dt","rnd","lerp","dist","rad",
    "anim","play","stop","inp",

    "pal","paleta","motor","Tupi","engine",
}) do BUILTIN[b] = true end

-- tokeniza uma linha para syntax highlight
local function tokenizar(linha)
    local tokens = {}
    local i = 1
    local n = #linha

    local function push(txt, cor)
        if txt and #txt > 0 then tokens[#tokens+1] = { txt=txt, cor=cor } end
    end

    while i <= n do
        local c = linha:sub(i, i)

        if linha:sub(i, i+1) == "--" then
            push(linha:sub(i), C.SYN_COMMENT)
            break

        elseif c == '"' then
            local j = i + 1
            while j <= n do
                if linha:sub(j,j) == '"' and linha:sub(j-1,j-1) ~= '\\' then break end
                j = j + 1
            end
            push(linha:sub(i, j), C.SYN_STRING)
            i = j + 1

        elseif c == "'" then
            local j = i + 1
            while j <= n do
                if linha:sub(j,j) == "'" and linha:sub(j-1,j-1) ~= '\\' then break end
                j = j + 1
            end
            push(linha:sub(i, j), C.SYN_STRING)
            i = j + 1

        elseif c:match("%d") or (c == "." and linha:sub(i+1,i+1):match("%d")) then
            local j = i
            while j <= n and linha:sub(j,j):match("[%d%.xXaAbBcCdDeEfF_]") do j=j+1 end
            push(linha:sub(i, j-1), C.SYN_NUMEROS)
            i = j

        elseif c:match("[%a_]") then
            local j = i
            while j <= n and linha:sub(j,j):match("[%w_]") do j=j+1 end
            local word  = linha:sub(i, j-1)
            local after = linha:sub(j):match("^%s*%(")

            if BOOL_NIL[word] then
                push(word, C.SYN_BOOLEANO)
                i = j

            elseif KW_ESCOPO[word] then
                push(word, C.SYN_FLUXO)
                if word == "function" then
                    local resto  = linha:sub(j)
                    local espaco = resto:match("^(%s+)")
                    local nome   = espaco and resto:sub(#espaco + 1):match("^([%a_][%w_%.%:]*)")
                    if espaco and nome then
                        push(espaco, C.TEXTO)
                        push(nome,   C.SYN_FUNCAO)
                        i = j + #espaco + #nome
                    else
                        i = j
                    end
                else
                    i = j
                end

            elseif KW_FLUXO[word] then
                push(word, C.SYN_FLUXO)
                i = j

            elseif BUILTIN[word] then
                push(word, C.SYN_BUILTIN)
                i = j

            elseif after then
                push(word, C.SYN_CHAMADA)
                i = j

            else
                push(word, C.SYN_VARIAVEL)
                i = j
            end

        elseif c:match("[%+%-%*/%%=%<%>%~%^%#%&%|%.%:%,%;%(%)%[%]%{%}]") then
            local op2 = linha:sub(i, i+1)
            if op2 == "==" or op2 == "~=" or op2 == "<=" or op2 == ">="
            or op2 == ".." or op2 == "::" then
                push(op2, C.SYN_OPERADOR)
                i = i + 2
            else
                push(c, C.SYN_OPERADOR)
                i = i + 1
            end

        else
            push(c, C.TEXTO)
            i = i + 1
        end
    end

    return tokens
end

local ESC  = 0.5   -- escala da fonte bitmap
local CW   = 8 * ESC
local CH   = 8 * ESC
local ESPV = CH + 1
local GW   = CW * 4
local BLINK = 0.52
local BASE_W = 160
local BASE_H = 144
local REPETE_ATRASO = 0.28  -- atraso antes de começar repetição de tecla
local REPETE_PASSO  = 0.04  -- intervalo entre repetições
local INDENT = "  "

-- áreas de layout fixas (em pixels na resolução base)
local LAYOUT = {
    linhas = { x = 3,  y = 2,   w = 20,  h = 109 },
    codigo = { x = 29, y = 2,   w = 128, h = 126 },
    status = { x = 29, y = 131, w = 128, h = 11  },
}

local MAX_UNDO = 64  -- tamanho máximo da pilha de undo

local function _trim(txt)
    return tostring(txt or ""):match("^%s*(.-)%s*$")
end

-- retorna a coluna da primeira letra não-espaço da linha
local function _primeiraColunaTexto(linha)
    local _, fim = tostring(linha or ""):find("^%s*")
    return fim or 0
end

-- retorna true se a linha abre um bloco e precisa de indent na próxima
local function _abrirIndentLua(prefixo)
    local txt = _trim((prefixo or ""):gsub("%-%-.*$", ""))
    if txt == "" then return false end
    if txt == "repeat" or txt == "else" then return true end
    if txt:match("^elseif%s+.-%s+then$") then return true end
    if txt:match("then$") or txt:match("do$") then return true end
    if txt:match("^function%s") or txt:match("^local%s+function%s") then return true end
    if txt:match("=%s*function%s*%b()%s*$") then return true end
    return false
end

-- conteúdo padrão de um novo arquivo
local function _templateNovoArquivo()
    return {
        "-- cartucho lua",
        "",
        "function _init()",
        "  -- runs once on startup",
        "end",
        "",
        "function _update()",
        "  -- game logic (every frame)",
        "end",
        "",
        "function _draw()",
        "  cls(1)",
        "  print(\"hello world\", 8, 8, 7)",
        "end",
    }
end

-- garante extensão .lua no caminho
local function _ajustarNomeLua(path)
    path = tostring(path or ""):match("^%s*(.-)%s*$")
    if path == "" then return nil end
    if not path:match("%.lua$") then
        path = path .. ".lua"
    end
    return path
end

-- extrai só o nome do arquivo de um caminho completo
local function _nomeArquivo(path)
    if not path or path == "" then return "(sem_nome.lua)" end
    return path:match("([^/\\]+)$") or path
end

local Editor = {}
Editor.__index = Editor

-- cria uma nova instância do editor
function Editor.novo(Tupi, ascii, basePng, runtime)
    assert(Tupi,  "[Editor] Tupi obrigatorio")
    assert(ascii, "[Editor] caminho do ascii.png obrigatorio")

    local self = setmetatable({}, Editor)
    self.T        = Tupi
    self._runtime = runtime
    self._fonte   = Tupi.texto.carregarFonte(ascii, 8, 8)
    self._ativo   = false
    self._spriteBase = Tupi.carregarSprite(basePng or "editor.png")
    self._objBase    = Tupi.criarObjeto(0, 0, -10, BASE_W, BASE_H, 0, 0, 1.0, 1.0, self._spriteBase)

    self._linhas  = _templateNovoArquivo()
    self._curLin  = 1
    self._curCol  = 0
    self._scrollY = 0
    self._scrollX = 0

    self._selAtiva = false
    self._selLin1  = 1
    self._selCol1  = 0
    self._selLin2  = 1
    self._selCol2  = 0

    self._clip = ""

    self._undo = {}
    self._redo = {}

    self._arquivo = nil
    self._sujo    = false

    self._blinkT    = 0
    self._cursorVis = true

    self._prompt = {
        ativo  = false,
        modo   = "",
        texto  = "",
        cursor = 0,
        msg    = "",
        scrollX = 0,
        acento = nil,
    }

    self._status = { msg="", cor=C.STATUS_INFO, timer=0 }
    self._acentoPendente = nil
    self._tokenCache = {}
    self._repeat     = {}

    self:_msg("Ctrl+N novo  Ctrl+R nome  Ctrl+S roda  Ctrl+0 sai", C.STATUS_INFO)

    return self
end

function Editor:ativar(v)    self._ativo = (v == nil) and not self._ativo or v end
function Editor:estaAtivo()  return self._ativo end

-- abre um arquivo do disco e carrega no editor
function Editor:abrirArquivo(path)
    local f, err = io.open(path, "r")
    if not f then self:_msg("Erro: " .. tostring(err), C.STATUS_ERR); return false end
    local conteudo = f:read("*a"); f:close()
    self._linhas = {}
    for linha in (conteudo .. "\n"):gmatch("([^\n]*)\n") do
        self._linhas[#self._linhas + 1] = linha
    end
    if #self._linhas == 0 then self._linhas = { "" } end
    self._arquivo = path
    self._curLin = 1; self._curCol = 0
    self._scrollY = 0; self._scrollX = 0
    self._undo = {}; self._redo = {}
    self._tokenCache = {}
    self._sujo = false
    self:_resetarEstadoTemporal()
    self:_msg("Aberto: " .. path, C.STATUS_OK)
    return true
end

-- salva o conteúdo atual no disco
function Editor:salvar(path)
    path = path or self._arquivo
    if not path then self:_msg("Nenhum arquivo definido", C.STATUS_ERR); return false end
    local f, err = io.open(path, "w")
    if not f then self:_msg("Erro ao salvar: " .. tostring(err), C.STATUS_ERR); return false end
    f:write(table.concat(self._linhas, "\n"))
    f:close()
    self._arquivo = path
    self._sujo = false
    self:_msg("Salvo: " .. path, C.STATUS_OK)
    return true
end

-- salva e executa o arquivo atual
function Editor:salvarERodar()
    if not self._arquivo then
        self:_abrirPrompt("renomear")
        self:_msg("Defina um nome com Ctrl+R", C.STATUS_INFO)
        return false
    end
    if not self:salvar(self._arquivo) then return false end
    self:executar()
    return true
end

-- executa o arquivo atual pelo runtime ou loadfile
function Editor:executar()
    local path = self._arquivo
    if not path then
        self:_msg("Defina um nome antes de rodar", C.STATUS_ERR)
        return false
    end
    if self._sujo then
        if not self:salvar(path) then return false end
    end
    if self._runtime then
        local ok, err2 = self._runtime:rodarArquivo(path, "editor")
        if ok then
            self:_msg("Run: " .. _nomeArquivo(path), C.STATUS_OK)
            return true
        else
            self:_msg("Erro: " .. tostring(err2), C.STATUS_ERR)
            return false
        end
    end
    local chunk, err = loadfile(path)
    if not chunk then
        self:_msg("Erro: " .. tostring(err), C.STATUS_ERR)
        return false
    else
        local ok, err2 = pcall(chunk)
        if ok then
            self:_msg("Run: " .. _nomeArquivo(path), C.STATUS_OK)
            return true
        else
            self:_msg("Erro: " .. tostring(err2), C.STATUS_ERR)
            return false
        end
    end
end

-- retorna todo o conteúdo como string
function Editor:getTexto()
    return table.concat(self._linhas, "\n")
end

function Editor:_limparSelecao()
    self._selAtiva = false
    self._selLin1  = self._curLin
    self._selCol1  = self._curCol
    self._selLin2  = self._curLin
    self._selCol2  = self._curCol
end

-- retorna true se há uma seleção não-vazia
function Editor:_temSelecao()
    return self._selAtiva
       and (self._selLin1 ~= self._selLin2 or self._selCol1 ~= self._selCol2)
end

-- reseta estado temporário (seleção, acento pendente, prompt)
function Editor:_resetarEstadoTemporal()
    self:_limparSelecao()
    self._acentoPendente = nil
    self._repeat = {}
    self._prompt.ativo = false
    self._prompt.acento = nil
    self._prompt.scrollX = 0
end

-- captura o estado atual para undo/redo
function Editor:_capturarEstado()
    return {
        linhas = self:_copiarLinhas(),
        curLin = self._curLin,
        curCol = self._curCol,
        scrollY = self._scrollY,
        scrollX = self._scrollX,
        selAtiva = self._selAtiva,
        selLin1 = self._selLin1,
        selCol1 = self._selCol1,
        selLin2 = self._selLin2,
        selCol2 = self._selCol2,
        sujo = self._sujo,
    }
end

-- restaura o editor para um estado capturado anteriormente
function Editor:_restaurarEstado(estado)
    self._linhas  = estado.linhas
    self._curLin  = estado.curLin
    self._curCol  = estado.curCol
    self._scrollY = estado.scrollY or 0
    self._scrollX = estado.scrollX or 0
    self._selAtiva = estado.selAtiva or false
    self._selLin1  = estado.selLin1 or self._curLin
    self._selCol1  = estado.selCol1 or self._curCol
    self._selLin2  = estado.selLin2 or self._curLin
    self._selCol2  = estado.selCol2 or self._curCol
    self._sujo = estado.sujo or false
    self._tokenCache = {}
    self:_ajustarScroll()
    self:_resetBlink()
end

function Editor:atualizar(dt)
    if not self._ativo then return end
    local T = self.T
    dt = dt or T.janela.dt()

    -- atualiza cursor piscando
    self._blinkT = self._blinkT + dt
    if self._blinkT >= BLINK then
        self._blinkT    = self._blinkT - BLINK
        self._cursorVis = not self._cursorVis
    end

    if self._status.timer > 0 then
        self._status.timer = self._status.timer - dt
    end

    if self._prompt.ativo then
        self:_atualizarPrompt(dt)
        return
    end

    local ctrl  = T.teclaSegurando(T.TECLA_CTRL_ESQ) or T.teclaSegurando(T.TECLA_CTRL_DIR)
    local shift = T.teclaSegurando(T.TECLA_SHIFT_ESQ) or T.teclaSegurando(T.TECLA_SHIFT_DIR)

    if T.teclaPressionou(T.TECLA_F5) then self:executar(); return end

    if ctrl then
        if self:_teclaRepetiu(T.TECLA_0, dt, 0.0, 0.5) then
            self._ativo = false; return
        end
        if T.teclaPressionou(T.TECLA_S) then self:salvarERodar() end
        if T.teclaPressionou(T.TECLA_O) then self:_abrirPrompt("abrir") end
        if T.teclaPressionou(T.TECLA_N) then self:_novoArquivo() end
        if T.teclaPressionou(T.TECLA_R) then self:_abrirPrompt("renomear") end
        if T.teclaPressionou(T.TECLA_Z) then self:_undo_pop() end
        if T.teclaPressionou(T.TECLA_Y) then self:_redo_pop() end
        if T.teclaPressionou(T.TECLA_C) then self:_copiar() end
        if T.teclaPressionou(T.TECLA_X) then self:_recortar() end
        if T.teclaPressionou(T.TECLA_V) then self:_colar() end
        if T.teclaPressionou(T.TECLA_A) then self:_selecionarTudo() end
        if T.teclaPressionou(T.TECLA_D) then self:_duplicarLinha() end
        if T.teclaPressionou(T.TECLA_G) then self:_abrirPrompt("goto") end
        if T.teclaPressionou(T.TECLA_F) then self:_abrirPrompt("busca") end
        if self:_teclaRepetiu(T.TECLA_CIMA,  dt, 0.18, 0.06) then
            self._scrollY = math.max(0, self._scrollY - 1)
        end
        if self:_teclaRepetiu(T.TECLA_BAIXO, dt, 0.18, 0.06) then
            self._scrollY = math.min(math.max(0, #self._linhas - 1), self._scrollY + 1)
        end
        return
    end

    local moveu = false
    if self:_teclaRepetiu(T.TECLA_CIMA, dt) then
        if shift and not self._selAtiva then self:_iniciarSelecao() end
        self._curLin = math.max(1, self._curLin - 1)
        self._curCol = math.min(self._curCol, #self._linhas[self._curLin])
        moveu = true
    end
    if self:_teclaRepetiu(T.TECLA_BAIXO, dt) then
        if shift and not self._selAtiva then self:_iniciarSelecao() end
        self._curLin = math.min(#self._linhas, self._curLin + 1)
        self._curCol = math.min(self._curCol, #self._linhas[self._curLin])
        moveu = true
    end
    if self:_teclaRepetiu(T.TECLA_ESQUERDA, dt) then
        if shift and not self._selAtiva then self:_iniciarSelecao() end
        if self._curCol > 0 then
            self._curCol = self._curCol - 1
        elseif self._curLin > 1 then
            self._curLin = self._curLin - 1
            self._curCol = #self._linhas[self._curLin]
        end
        moveu = true
    end
    if self:_teclaRepetiu(T.TECLA_DIREITA, dt) then
        if shift and not self._selAtiva then self:_iniciarSelecao() end
        local maxCol = #self._linhas[self._curLin]
        if self._curCol < maxCol then
            self._curCol = self._curCol + 1
        elseif self._curLin < #self._linhas then
            self._curLin = self._curLin + 1
            self._curCol = 0
        end
        moveu = true
    end
    if T.TECLA_HOME and T.teclaPressionou(T.TECLA_HOME) then
        if shift and not self._selAtiva then self:_iniciarSelecao() end
        local alvo = _primeiraColunaTexto(self._linhas[self._curLin])
        if self._curCol == alvo then alvo = 0 end
        self._curCol = alvo
        moveu = true
    end
    if T.TECLA_END and T.teclaPressionou(T.TECLA_END) then
        if shift and not self._selAtiva then self:_iniciarSelecao() end
        self._curCol = #self._linhas[self._curLin]; moveu = true
    end
    if T.TECLA_PGUP and T.teclaPressionou(T.TECLA_PGUP) then
        local vis = self:_linhasVisiveis()
        self._curLin  = math.max(1, self._curLin - vis)
        self._scrollY = math.max(0, self._scrollY - vis)
        self._curCol  = math.min(self._curCol, #self._linhas[self._curLin])
        moveu = true
    end
    if T.TECLA_PGDN and T.teclaPressionou(T.TECLA_PGDN) then
        local vis = self:_linhasVisiveis()
        self._curLin  = math.min(#self._linhas, self._curLin + vis)
        self._scrollY = math.min(math.max(0, #self._linhas - 1), self._scrollY + vis)
        self._curCol  = math.min(self._curCol, #self._linhas[self._curLin])
        moveu = true
    end

    if moveu then
        if self._selAtiva then self:_atualizarSelecaoFim() end
        if not shift then self._selAtiva = false end
        self:_ajustarScroll()
        self:_resetBlink()
        return
    end

    if T.teclaPressionou(T.TECLA_ENTER) then
        self:_pushUndo()
        if self:_temSelecao() then self:_deletarSelecao() end
        local lin    = self._linhas[self._curLin]
        local antes  = lin:sub(1, self._curCol)
        local depois = lin:sub(self._curCol + 1)
        local indent = antes:match("^(%s*)")
        local resto = _trim(depois)
        -- reduz indent se a próxima linha fecha um bloco
        if resto == "end" or resto == "else" or resto == "until" or resto:match("^elseif%s") then
            indent = indent:sub(1, math.max(0, #indent - #INDENT))
        elseif _abrirIndentLua(antes) then
            indent = indent .. INDENT
        end
        self._linhas[self._curLin] = antes
        table.insert(self._linhas, self._curLin + 1, indent .. depois)
        self._curLin = self._curLin + 1
        self._curCol = #indent
        self:_ajustarScroll()
        self:_resetBlink()
        self._tokenCache = {}
        self._sujo = true
        return
    end

    if self:_teclaRepetiu(T.TECLA_BACKSPACE, dt) then
        self:_apagarAtras(); return
    end

    if T.TECLA_DELETE and self:_teclaRepetiu(T.TECLA_DELETE, dt) then
        self:_apagarFrente(); return
    end

    if T.teclaPressionou(T.TECLA_TAB) then
        if self:_tabular(shift) then return end
    end

    self:_capturarTexto(dt)
end

function Editor:desenhar()
    if not self._ativo then return end

    local T = self.T
    local f = self._fonte
    local areaCodigo = LAYOUT.codigo
    local areaLinhas = LAYOUT.linhas
    local areaStatus = LAYOUT.status

    local linhasVis = self:_linhasVisiveis()
    local colsVis   = math.floor((areaCodigo.w - 4) / CW) - 1

    T.cor(1, 1, 1, 1)
    T.enviarBatch(self._objBase, -10)
    T.batchDesenhar()

    local yBase = areaCodigo.y + 2
    for i = 1, linhasVis do
        local linIdx = self._scrollY + i
        if linIdx > #self._linhas then break end
        local y = yBase + (i - 1) * ESPV

        -- destaca a linha do cursor
        if linIdx == self._curLin then
            T.cor(C.FUNDO_LINE[1], C.FUNDO_LINE[2], C.FUNDO_LINE[3], 1)
            T.retangulo(areaCodigo.x + 1, y - 1, areaCodigo.w - 2, ESPV)
        end

        -- número de linha no gutter
        local numStr = tostring(linIdx)
        local nx     = areaLinhas.x + areaLinhas.w - 2 - #numStr * CW
        local corNum = (linIdx == self._curLin) and C.NUM_ATIVO or C.OPACO
        T.cor(corNum[1], corNum[2], corNum[3], 1)
        T.texto.desenhar(nx, y, 5, numStr, ESC, 1.0, f)

        -- destaca a seleção de texto
        if self._selAtiva then
            local s1l, s1c, s2l, s2c = self:_selOrdenada()
            if linIdx >= s1l and linIdx <= s2l then
                local xSelStart = areaCodigo.x + 2
                local xSelEnd   = areaCodigo.x + areaCodigo.w - 2
                if linIdx == s1l then xSelStart = areaCodigo.x + 2 + (s1c - self._scrollX) * CW end
                if linIdx == s2l then xSelEnd   = areaCodigo.x + 2 + (s2c - self._scrollX) * CW end
                xSelStart = math.max(areaCodigo.x + 2, xSelStart)
                xSelEnd   = math.min(areaCodigo.x + areaCodigo.w - 2, xSelEnd)
                if xSelEnd > xSelStart then
                    T.cor(C.FUNDO_SEL[1], C.FUNDO_SEL[2], C.FUNDO_SEL[3], 0.7)
                    T.retangulo(xSelStart, y - 1, xSelEnd - xSelStart, ESPV)
                end
            end
        end

        -- desenha os tokens com syntax highlight
        local tokens = self:_getTokens(linIdx)
        local xChar  = areaCodigo.x + 2
        local colChar = 0
        for _, tok in ipairs(tokens) do
            local txtVis = tok.txt
            if colChar + #txtVis > self._scrollX then
                local cortado  = txtVis:sub(math.max(1, self._scrollX - colChar + 1))
                local maxChars = colsVis - math.max(0, colChar - self._scrollX)
                if maxChars > 0 then
                    local fatia = cortado:sub(1, maxChars)
                    local cx    = xChar + math.max(0, colChar - self._scrollX) * CW
                    local cor   = tok.cor or C.TEXTO
                    T.cor(cor[1], cor[2], cor[3], 1)
                    T.texto.desenhar(cx, y, 5, fatia, ESC, 1.0, f)
                    T.batchDesenhar()
                end
            end
            colChar = colChar + #tok.txt
        end
        T.cor(1, 1, 1, 1)
    end

    -- barra de status: nome do arquivo e posição do cursor
    local barY = areaStatus.y + 2
    local nome = self._arquivo and
        (self._sujo and "*" .. _nomeArquivo(self._arquivo) or _nomeArquivo(self._arquivo))
        or "(sem_nome.lua)"
    T.cor(C.CIANO[1], C.CIANO[2], C.CIANO[3], 1)
    T.texto.desenhar(areaStatus.x + 2, barY, 6, nome:sub(1, 12), ESC, 1.0, f)
    T.batchDesenhar()

    local pos = ("L%d C%d"):format(self._curLin, self._curCol + 1)
    local posX = areaStatus.x + areaStatus.w - (#pos * CW) - 2
    T.cor(C.OPACO[1], C.OPACO[2], C.OPACO[3], 1)
    T.texto.desenhar(posX, barY, 6, pos, ESC, 1.0, f)
    T.batchDesenhar()

    -- mensagem de status (atalhos ou feedback de ação)
    local msg      = "Ctrl+N novo  Ctrl+R nome  Ctrl+S run  Ctrl+0 sair"
    local corMsg   = C.OPACO
    local alphaMsg = 1.0
    if self._status.timer > 0 then
        corMsg   = self._status.cor
        alphaMsg = math.min(self._status.timer * 2, 1.0)
        msg      = self._status.msg
    end
    local msgMax = math.max(0, math.floor((posX - (areaStatus.x + 56)) / CW) - 1)
    msg = msg:sub(1, msgMax)
    T.cor(corMsg[1], corMsg[2], corMsg[3], alphaMsg)
    T.texto.desenhar(areaStatus.x + 56, barY, 6, msg, ESC, 1.0, f)
    T.batchDesenhar()

    if self._prompt.ativo then self:_desenharPrompt() end

    T.cor(1, 1, 1, 1)
    T.batchDesenhar()

    -- cursor em formato de I-beam
    if self._cursorVis and not self._prompt.ativo then
        local cy = yBase + (self._curLin - self._scrollY - 1) * ESPV
        local cx = areaCodigo.x + 2 + (self._curCol - self._scrollX) * CW
        if cy >= yBase - 1 and cy < areaCodigo.y + areaCodigo.h - CH
        and cx >= areaCodigo.x and cx <= areaCodigo.x + areaCodigo.w then
            T.cor(C.CURSOR[1], C.CURSOR[2], C.CURSOR[3], 0.9)
            T.retangulo(cx,     cy,          1,  CH)
            T.retangulo(cx - 1, cy,          3,  1)
            T.retangulo(cx - 1, cy + CH - 1, 3,  1)
            T.cor(1, 1, 1, 1)
        end
    end
end

-- abre o prompt flutuante no modo indicado (abrir, renomear, goto, busca)
function Editor:_abrirPrompt(modo)
    self._prompt.ativo  = true
    self._prompt.modo   = modo
    self._prompt.texto  = ""
    self._prompt.cursor = 0
    self._prompt.scrollX = 0
    self._prompt.acento = nil
    local labels = {
        abrir    = "Abrir arquivo:",
        renomear = "Nome do arquivo:",
        ["goto"] = "Ir para linha:",
        busca    = "Buscar:",
    }
    self._prompt.label = labels[modo] or ""
    if modo == "renomear" and self._arquivo then
        self._prompt.texto  = self._arquivo
        self._prompt.cursor = #self._prompt.texto
    end
    self:_ajustarScrollPrompt()
end

function Editor:_atualizarPrompt(dt)
    local T = self.T
    local p = self._prompt

    local ctrl = T.teclaSegurando(T.TECLA_CTRL_ESQ) or T.teclaSegurando(T.TECLA_CTRL_DIR)
    if ctrl and self:_teclaRepetiu(T.TECLA_0, dt, 0.0, 0.5) then p.ativo = false; return end
    if T.TECLA_ESC and T.teclaPressionou(T.TECLA_ESC) then p.ativo = false; return end

    if self:_teclaRepetiu(T.TECLA_BACKSPACE, dt) and p.cursor > 0 then
        p.texto  = p.texto:sub(1, p.cursor-1) .. p.texto:sub(p.cursor+1)
        p.cursor = p.cursor - 1
    end
    if T.TECLA_DELETE and self:_teclaRepetiu(T.TECLA_DELETE, dt) and p.cursor < #p.texto then
        p.texto = p.texto:sub(1, p.cursor) .. p.texto:sub(p.cursor + 2)
    end
    if self:_teclaRepetiu(T.TECLA_ESQUERDA, dt) then p.cursor = math.max(0, p.cursor - 1) end
    if self:_teclaRepetiu(T.TECLA_DIREITA,  dt) then p.cursor = math.min(#p.texto, p.cursor + 1) end
    if T.TECLA_HOME and T.teclaPressionou(T.TECLA_HOME) then p.cursor = 0 end
    if T.TECLA_END and T.teclaPressionou(T.TECLA_END) then p.cursor = #p.texto end

    if T.teclaPressionou(T.TECLA_ENTER) then
        p.ativo = false
        local texto = _trim(p.texto)
        if p.modo == "abrir" then
            self:abrirArquivo(texto)
        elseif p.modo == "renomear" then
            local nome = _ajustarNomeLua(texto)
            if not nome then
                self:_msg("Nome invalido", C.STATUS_ERR)
            else
                self._arquivo = nome
                self:_msg("Arquivo: " .. _nomeArquivo(nome), C.STATUS_OK)
            end
        elseif p.modo == "goto" then
            local n = tonumber(texto)
            if n then
                self._curLin = math.max(1, math.min(#self._linhas, n))
                self._curCol = 0; self:_ajustarScroll()
            end
        elseif p.modo == "busca" then
            self:_buscar(texto)
        end
        return
    end

    self:_capturarTextoPrompt(dt)
    self:_ajustarScrollPrompt()
end

function Editor:_desenharPrompt()
    local T = self.T
    local W = T.largura()
    local H = T.altura()
    local f = self._fonte
    local p = self._prompt
    local py = math.floor(H / 2) - CH - 2
    local bw = math.floor(W * 0.8)
    local bx = math.floor((W - bw) / 2)

    T.cor(C.PROMPT_BG[1],  C.PROMPT_BG[2],  C.PROMPT_BG[3],  1)
    T.retangulo(bx - 2, py - 2, bw + 4, CH * 3 + 6)
    T.cor(C.PROMPT_BRD[1], C.PROMPT_BRD[2], C.PROMPT_BRD[3], 1)
    T.retanguloBorda(bx - 2, py - 2, bw + 4, CH * 3 + 6)

    T.cor(C.CIANO[1], C.CIANO[2], C.CIANO[3], 1)
    T.texto.desenhar(bx, py, 6, p.label, ESC, 1.0, f)

    T.cor(C.TEXTO[1], C.TEXTO[2], C.TEXTO[3], 1)
    local maxChars = math.max(1, math.floor((bw - 4) / CW))
    local vis = p.texto:sub(p.scrollX + 1, p.scrollX + maxChars)
    T.texto.desenhar(bx, py + CH + 2, 6, vis, ESC, 1.0, f)

    if self._cursorVis then
        T.cor(C.CURSOR[1], C.CURSOR[2], C.CURSOR[3], 0.9)
        T.retangulo(bx + (p.cursor - p.scrollX) * CW, py + CH + 1, 1, CH + 1)
    end

    T.cor(C.OPACO[1], C.OPACO[2], C.OPACO[3], 1)
    T.texto.desenhar(bx, py + CH * 2 + 4, 6, "Enter confirma  Esc cancela", ESC, 1.0, f)
    T.cor(1, 1, 1, 1)
end

-- cria um novo arquivo com o template padrão
function Editor:_novoArquivo()
    self._linhas   = _templateNovoArquivo()
    self._curLin   = 1; self._curCol = 0
    self._scrollY  = 0; self._scrollX = 0
    self._arquivo  = nil; self._sujo = false
    self._undo     = {}; self._redo = {}
    self._tokenCache = {}
    self:_resetarEstadoTemporal()
    self:_msg("Novo arquivo Lua", C.STATUS_INFO)
end

-- apaga o caractere antes do cursor (ou a seleção)
function Editor:_apagarAtras()
    if self:_temSelecao() then
        self:_pushUndo()
        self:_deletarSelecao()
    elseif self._curCol > 0 then
        self:_pushUndo()
        local lin = self._linhas[self._curLin]
        local antes = lin:sub(1, self._curCol)
        if antes:match("^%s+$") then
            -- apaga bloco de indentação
            local remover = ((self._curCol - 1) % #INDENT) + 1
            local ini = self._curCol - remover + 1
            self._linhas[self._curLin] = lin:sub(1, ini - 1) .. lin:sub(self._curCol + 1)
            self._curCol = ini - 1
        else
            self._linhas[self._curLin] = lin:sub(1, self._curCol - 1) .. lin:sub(self._curCol + 1)
            self._curCol = self._curCol - 1
        end
        self:_invalidarToken(self._curLin)
    elseif self._curLin > 1 then
        self:_pushUndo()
        local lin  = self._linhas[self._curLin]
        local prev = self._linhas[self._curLin - 1]
        self._curCol = #prev
        self._linhas[self._curLin - 1] = prev .. lin
        table.remove(self._linhas, self._curLin)
        self._curLin = self._curLin - 1
        self._tokenCache = {}
    else
        return false
    end
    self:_ajustarScroll()
    self:_resetBlink()
    self._sujo = true
    return true
end

-- apaga o caractere à frente do cursor (ou a seleção)
function Editor:_apagarFrente()
    if self:_temSelecao() then
        self:_pushUndo()
        self:_deletarSelecao()
    else
        local lin = self._linhas[self._curLin]
        if self._curCol < #lin then
            self:_pushUndo()
            self._linhas[self._curLin] = lin:sub(1, self._curCol) .. lin:sub(self._curCol + 2)
            self:_invalidarToken(self._curLin)
        elseif self._curLin < #self._linhas then
            self:_pushUndo()
            self._linhas[self._curLin] = lin .. self._linhas[self._curLin + 1]
            table.remove(self._linhas, self._curLin + 1)
            self._tokenCache = {}
        else
            return false
        end
    end
    self:_ajustarScroll()
    self:_resetBlink()
    self._sujo = true
    return true
end

-- empurra o estado atual para a pilha de undo
function Editor:_pushUndo()
    self._undo[#self._undo + 1] = self:_capturarEstado()
    if #self._undo > MAX_UNDO then table.remove(self._undo, 1) end
    self._redo = {}
end

function Editor:_undo_pop()
    if #self._undo == 0 then self:_msg("Nada para desfazer", C.STATUS_INFO); return end
    self._redo[#self._redo + 1] = self:_capturarEstado()
    local estado = table.remove(self._undo)
    self:_restaurarEstado(estado)
    self:_msg("Desfeito", C.STATUS_INFO)
end

function Editor:_redo_pop()
    if #self._redo == 0 then self:_msg("Nada para refazer", C.STATUS_INFO); return end
    self._undo[#self._undo + 1] = self:_capturarEstado()
    local estado = table.remove(self._redo)
    self:_restaurarEstado(estado)
    self:_msg("Refeito", C.STATUS_INFO)
end

-- retorna uma cópia rasa das linhas do editor
function Editor:_copiarLinhas()
    local t = {}
    for i, l in ipairs(self._linhas) do t[i] = l end
    return t
end

function Editor:_iniciarSelecao()
    self._selAtiva = true
    self._selLin1  = self._curLin
    self._selCol1  = self._curCol
    self._selLin2  = self._curLin
    self._selCol2  = self._curCol
end

-- atualiza o ponto final da seleção para a posição atual do cursor
function Editor:_atualizarSelecaoFim()
    self._selLin2 = self._curLin
    self._selCol2 = self._curCol
end

-- retorna a seleção ordenada do início para o fim
function Editor:_selOrdenada()
    local l1, c1 = self._selLin1, self._selCol1
    local l2, c2 = self._selLin2, self._selCol2
    if l1 > l2 or (l1 == l2 and c1 > c2) then
        l1, c1, l2, c2 = l2, c2, l1, c1
    end
    return l1, c1, l2, c2
end

function Editor:_selecionarTudo()
    self._selAtiva = true
    self._selLin1  = 1; self._selCol1 = 0
    self._selLin2  = #self._linhas
    self._selCol2  = #self._linhas[#self._linhas]
    self._curLin   = self._selLin2
    self._curCol   = self._selCol2
    self:_ajustarScroll()
end

-- retorna o texto selecionado como string
function Editor:_textoSelecionado()
    if not self:_temSelecao() then return "" end
    local l1, c1, l2, c2 = self:_selOrdenada()
    if l1 == l2 then
        return self._linhas[l1]:sub(c1 + 1, c2)
    end
    local t = { self._linhas[l1]:sub(c1 + 1) }
    for i = l1 + 1, l2 - 1 do t[#t+1] = self._linhas[i] end
    t[#t+1] = self._linhas[l2]:sub(1, c2)
    return table.concat(t, "\n")
end

-- apaga o trecho selecionado e move o cursor para o início
function Editor:_deletarSelecao()
    if not self:_temSelecao() then
        self:_limparSelecao()
        return false
    end
    local l1, c1, l2, c2 = self:_selOrdenada()
    local multilinha = l1 ~= l2
    local antes  = self._linhas[l1]:sub(1, c1)
    local depois = self._linhas[l2]:sub(c2 + 1)
    self._linhas[l1] = antes .. depois
    for i = l2, l1 + 1, -1 do table.remove(self._linhas, i) end
    self._curLin  = l1; self._curCol = c1
    self:_limparSelecao()
    if multilinha then
        self._tokenCache = {}
    else
        self:_invalidarToken(l1)
    end
    return true
end

function Editor:_copiar()
    self._clip = self:_textoSelecionado()
    if self._clip == "" then
        self._clip = self._linhas[self._curLin] or ""
    end
    if self._clip ~= "" then self:_msg("Copiado", C.STATUS_INFO) end
end

function Editor:_recortar()
    self._clip = self:_textoSelecionado()
    if self._clip ~= "" then
        self:_pushUndo()
        self:_deletarSelecao()
        self._sujo = true
        self:_msg("Recortado", C.STATUS_INFO)
        return
    end
    if #self._linhas == 1 and self._linhas[1] == "" then return end
    self:_pushUndo()
    self._clip = self._linhas[self._curLin] or ""
    table.remove(self._linhas, self._curLin)
    if #self._linhas == 0 then self._linhas = { "" } end
    self._curLin = math.min(self._curLin, #self._linhas)
    self._curCol = math.min(self._curCol, #self._linhas[self._curLin])
    self:_limparSelecao()
    self._tokenCache = {}
    self:_ajustarScroll()
    self._sujo = true
    self:_msg("Linha recortada", C.STATUS_INFO)
end

-- insere texto na posição do cursor, suportando múltiplas linhas
function Editor:_inserirTextoNoCursor(texto)
    if texto == "" then return false end
    if self:_temSelecao() then self:_deletarSelecao() end
    local linhas = {}
    for l in (texto .. "\n"):gmatch("([^\n]*)\n") do
        linhas[#linhas + 1] = l
    end
    if #linhas == 0 then return false end
    local lin    = self._linhas[self._curLin]
    local antes  = lin:sub(1, self._curCol)
    local depois = lin:sub(self._curCol + 1)
    if #linhas == 1 then
        self._linhas[self._curLin] = antes .. linhas[1] .. depois
        self._curCol = self._curCol + #linhas[1]
        self:_invalidarToken(self._curLin)
    else
        self._linhas[self._curLin] = antes .. linhas[1]
        for i = 2, #linhas - 1 do
            table.insert(self._linhas, self._curLin + i - 1, linhas[i])
        end
        local ultima = self._curLin + #linhas - 1
        table.insert(self._linhas, ultima, linhas[#linhas] .. depois)
        self._curLin = ultima
        self._curCol = #linhas[#linhas]
        self._tokenCache = {}
    end
    self:_ajustarScroll()
    self:_resetBlink()
    self._sujo = true
    return true
end

function Editor:_colar()
    if self._clip == "" then return end
    self:_pushUndo()
    self:_inserirTextoNoCursor(self._clip)
end

function Editor:_duplicarLinha()
    if self:_temSelecao() then
        local texto = self:_textoSelecionado()
        local _, _, l2, c2 = self:_selOrdenada()
        self:_pushUndo()
        self._curLin = l2
        self._curCol = c2
        self:_limparSelecao()
        self:_inserirTextoNoCursor(texto)
        self:_msg("Selecao duplicada", C.STATUS_INFO)
        return
    end
    self:_pushUndo()
    local lin = self._linhas[self._curLin]
    table.insert(self._linhas, self._curLin + 1, lin)
    self._curLin = self._curLin + 1
    self:_ajustarScroll()
    self._tokenCache = {}
    self._sujo = true
end

-- busca um termo a partir da posição atual, com wrap
function Editor:_buscar(termo)
    if termo == "" then return end
    for i = self._curLin, #self._linhas do
        local startCol = (i == self._curLin) and self._curCol + 1 or 1
        local pos = self._linhas[i]:find(termo, startCol, true)
        if pos then
            self._curLin = i; self._curCol = pos - 1
            self:_ajustarScroll()
            self:_msg("Encontrado: L" .. i, C.STATUS_OK)
            return
        end
    end
    for i = 1, self._curLin do
        local pos = self._linhas[i]:find(termo, 1, true)
        if pos then
            self._curLin = i; self._curCol = pos - 1
            self:_ajustarScroll()
            self:_msg("Encontrado (wrap): L" .. i, C.STATUS_OK)
            return
        end
    end
    self:_msg("Não encontrado: " .. termo, C.STATUS_ERR)
end

-- insere um caractere no campo de texto do prompt
function Editor:_inserirTextoPrompt(c)
    local p = self._prompt
    p.texto  = p.texto:sub(1, p.cursor) .. c .. p.texto:sub(p.cursor + 1)
    p.cursor = p.cursor + #c
end

-- captura teclas no prompt, com suporte a acentos mortos
function Editor:_capturarTextoPrompt(dt)
    local T = self.T
    local p = self._prompt
    local shift = T.teclaSegurando(T.TECLA_SHIFT_ESQ) or T.teclaSegurando(T.TECLA_SHIFT_DIR)

    local function tentarInserir(char_digitado)
        if p.acento then
            local combinado = KB.combinar(p.acento, char_digitado)
            if combinado then
                self:_inserirTextoPrompt(combinado)
            else
                self:_inserirTextoPrompt(p.acento)
                self:_inserirTextoPrompt(char_digitado)
            end
            p.acento = nil
        elseif KB.ehAcento(char_digitado) then
            p.acento = char_digitado
        else
            self:_inserirTextoPrompt(char_digitado)
        end
    end

    for _, l in ipairs({"A","B","C","D","E","F","G","H","I","J","K","L","M",
                         "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"}) do
        local code = T["TECLA_" .. l]
        if code and self:_teclaRepetiu(code, dt) then
            tentarInserir(shift and l or l:lower())
        end
    end

    for d = 0, 9 do
        local code = T["TECLA_" .. d]
        if code and self:_teclaRepetiu(code, dt) then
            tentarInserir(shift and KB.getNumShift(d) or tostring(d))
        end
    end

    for _, s in ipairs(KB.getSimb()) do
        if self:_teclaRepetiu(s.c, dt) then
            tentarInserir(shift and s.s or s.n)
        end
    end
end

-- ajusta o scroll horizontal do prompt para manter o cursor visível
function Editor:_ajustarScrollPrompt(maxChars)
    local p = self._prompt
    maxChars = maxChars or math.max(1, math.floor((math.floor(self.T.largura() * 0.8) - 4) / CW))
    if p.cursor < p.scrollX then
        p.scrollX = p.cursor
    elseif p.cursor >= p.scrollX + maxChars then
        p.scrollX = p.cursor - maxChars + 1
    end
    p.scrollX = math.max(0, p.scrollX)
end

-- adiciona ou remove indentação nas linhas selecionadas
function Editor:_tabular(shift)
    if self:_temSelecao() then
        local l1, c1, l2, c2 = self:_selOrdenada()
        local curLin, curCol = self._curLin, self._curCol
        local remocoes = {}
        local mudou = false
        if shift then
            for i = l1, l2 do
                local lin = self._linhas[i]
                local remover = 0
                if lin:sub(1, #INDENT) == INDENT then
                    remover = #INDENT
                elseif lin:sub(1, 1) == " " then
                    remover = 1
                end
                remocoes[i] = remover
                if remover > 0 then mudou = true end
            end
            if not mudou then return false end
        else
            mudou = true
        end

        self:_pushUndo()
        for i = l1, l2 do
            local lin = self._linhas[i]
            if shift then
                local remover = remocoes[i] or 0
                if remover > 0 then
                    self._linhas[i] = lin:sub(remover + 1)
                    if i == l1 then c1 = math.max(0, c1 - remover) end
                    if i == l2 then c2 = math.max(0, c2 - remover) end
                    if i == curLin then curCol = math.max(0, curCol - remover) end
                end
            else
                self._linhas[i] = INDENT .. lin
                if i == l1 then c1 = c1 + #INDENT end
                if i == l2 then c2 = c2 + #INDENT end
                if i == curLin then curCol = curCol + #INDENT end
            end
            self:_invalidarToken(i)
        end
        self._curLin = curLin
        self._curCol = curCol
        self._selAtiva = true
        self._selLin1 = l1
        self._selCol1 = c1
        self._selLin2 = l2
        self._selCol2 = c2
        self:_ajustarScroll()
        self:_resetBlink()
        self._sujo = true
        return true
    end

    local lin = self._linhas[self._curLin]
    if shift then
        local remover = 0
        if lin:sub(1, #INDENT) == INDENT then
            remover = #INDENT
        elseif lin:sub(1, 1) == " " then
            remover = 1
        end
        if remover == 0 then return false end
        self:_pushUndo()
        self._linhas[self._curLin] = lin:sub(remover + 1)
        self._curCol = math.max(0, self._curCol - remover)
    else
        self:_pushUndo()
        self._linhas[self._curLin] = lin:sub(1, self._curCol) .. INDENT .. lin:sub(self._curCol + 1)
        self._curCol = self._curCol + #INDENT
    end
    self:_invalidarToken(self._curLin)
    self:_ajustarScroll()
    self:_resetBlink()
    self._sujo = true
    return true
end

-- símbolos e números vêm do módulo KB; use "teclado <id>" no console para trocar layout
function Editor:_capturarTexto(dt)
    local T = self.T
    local shift    = T.teclaSegurando(T.TECLA_SHIFT_ESQ) or T.teclaSegurando(T.TECLA_SHIFT_DIR)
    local qualquer = false

    -- tenta compor acentos mortos antes de inserir
    local function tentarInserir(char_digitado)
        if self._acentoPendente then
            local combinado = KB.combinar(self._acentoPendente, char_digitado)
            if combinado then
                self:_inserirChar(combinado)
            else
                self:_inserirChar(self._acentoPendente)
                self:_inserirChar(char_digitado)
            end
            self._acentoPendente = nil
            qualquer = true
        elseif KB.ehAcento(char_digitado) then
            self._acentoPendente = char_digitado
        else
            self:_inserirChar(char_digitado)
            qualquer = true
        end
    end

    for _, l in ipairs({"A","B","C","D","E","F","G","H","I","J","K","L","M",
                         "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"}) do
        local code = T["TECLA_" .. l]
        if code and self:_teclaRepetiu(code, dt) then
            tentarInserir(shift and l or l:lower())
        end
    end

    for d = 0, 9 do
        local code = T["TECLA_" .. d]
        if code and self:_teclaRepetiu(code, dt) then
            tentarInserir(shift and KB.getNumShift(d) or tostring(d))
        end
    end

    for _, s in ipairs(KB.getSimb()) do
        if self:_teclaRepetiu(s.c, dt) then
            tentarInserir(shift and s.s or s.n)
        end
    end

    if qualquer then self._sujo = true end
end

-- insere um único caractere na posição do cursor
function Editor:_inserirChar(c)
    self:_pushUndo()
    if self:_temSelecao() then self:_deletarSelecao() end
    local lin = self._linhas[self._curLin]
    self._linhas[self._curLin] = lin:sub(1, self._curCol) .. c .. lin:sub(self._curCol + 1)
    self._curCol = self._curCol + #c
    self:_invalidarToken(self._curLin)
    self:_ajustarScroll()
    self:_resetBlink()
    self._sujo = true
end

-- retorna quantas linhas cabem na área de código
function Editor:_linhasVisiveis()
    return math.floor((LAYOUT.codigo.h - 4) / ESPV)
end

-- ajusta scrollY e scrollX para manter o cursor visível
function Editor:_ajustarScroll()
    local vis = self:_linhasVisiveis()
    if self._curLin <= self._scrollY then
        self._scrollY = self._curLin - 1
    elseif self._curLin > self._scrollY + vis then
        self._scrollY = self._curLin - vis
    end
    self._scrollY = math.max(0, self._scrollY)

    local colsVis = math.floor((LAYOUT.codigo.w - 4) / CW) - 1
    if self._curCol < self._scrollX then
        self._scrollX = self._curCol
    elseif self._curCol >= self._scrollX + colsVis then
        self._scrollX = self._curCol - colsVis + 1
    end
    self._scrollX = math.max(0, self._scrollX)
end

-- reinicia o timer do cursor para evitar piscada logo após digitar
function Editor:_resetBlink()
    self._blinkT    = 0
    self._cursorVis = true
end

-- retorna true se a tecla foi pressionada ou está em repetição automática
function Editor:_teclaRepetiu(code, dt, atraso, passo)
    local T = self.T
    atraso = atraso or REPETE_ATRASO
    passo  = passo  or REPETE_PASSO

    if T.teclaPressionou(code) then
        self._repeat[code] = { tempo = 0, proximo = atraso }
        return true
    end

    if not T.teclaSegurando(code) then
        self._repeat[code] = nil
        return false
    end

    local st = self._repeat[code]
    if not st then
        st = { tempo = 0, proximo = atraso }
        self._repeat[code] = st
    end

    st.tempo = st.tempo + (dt or 0)
    if st.tempo >= st.proximo then
        st.proximo = st.proximo + passo
        return true
    end
    return false
end

-- exibe uma mensagem temporária na barra de status
function Editor:_msg(txt, cor)
    self._status.msg   = txt
    self._status.cor   = cor or C.STATUS_INFO
    self._status.timer = 3.0
end

-- invalida o cache de tokens de uma linha para reprocessar o highlight
function Editor:_invalidarToken(linIdx)
    self._tokenCache[linIdx] = nil
end

-- retorna os tokens da linha, usando cache se disponível
function Editor:_getTokens(linIdx)
    if not self._tokenCache[linIdx] then
        self._tokenCache[linIdx] = tokenizar(self._linhas[linIdx] or "")
    end
    return self._tokenCache[linIdx]
end

return Editor