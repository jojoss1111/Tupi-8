local N  = require("src.Engine.texto_normalizar")
local KB = require("src.Engine.tupi_teclado")

-- quebra texto em linhas de no máximo max_chars caracteres
local function _wrap(txt, max_chars)
    local linhas = {}
    for trecho in (txt .. "\n"):gmatch("([^\n]*)\n") do
        if #trecho == 0 then
            linhas[#linhas + 1] = ""
        elseif #trecho <= max_chars then
            linhas[#linhas + 1] = trecho
        else
            local atual = ""
            for palavra in (trecho .. " "):gmatch("([^ ]*) ") do
                if #atual == 0 then
                    if #palavra > max_chars then
                        local i = 1
                        while i <= #palavra do
                            linhas[#linhas + 1] = palavra:sub(i, i + max_chars - 1)
                            i = i + max_chars
                        end
                    else
                        atual = palavra
                    end
                elseif #atual + 1 + #palavra <= max_chars then
                    atual = atual .. " " .. palavra
                else
                    linhas[#linhas + 1] = atual
                    atual = palavra
                end
            end
            if #atual > 0 then linhas[#linhas + 1] = atual end
        end
    end
    return linhas
end

-- paleta de cores do console
local C = {
    PRETO    = {0.000, 0.000, 0.000},
    ESCURO   = {0.000, 0.000, 0.000},
    CINZA    = {0.250, 0.235, 0.220},
    PRATA    = {0.620, 0.610, 0.600},
    BRANCO   = {0.960, 0.940, 0.900},
    VERDE    = {0.180, 0.780, 0.380},
    CIANO    = {0.200, 0.720, 0.900},
    VERMELHO = {0.900, 0.220, 0.280},
    ROXO     = {0.600, 0.400, 0.900},
    AMARELO  = {0.950, 0.850, 0.200},
}

local MAX_BUF  = 150   -- máximo de linhas no buffer
local MAX_HIST = 32    -- máximo de entradas no histórico
local BLINK    = 0.53  -- período do cursor piscando
local ESC      = 0.5   -- escala da fonte bitmap
local CW       = 8 * ESC
local CH       = 8 * ESC
local ESPV     = CH + 1

-- comandos disponíveis no console
local CMDS = {}
CMDS["cls"]   = function(con) con._buf = {} end
CMDS["clear"] = CMDS["cls"]
CMDS["ajuda"] = function(con) con:_ajuda() end
CMDS["help"]  = CMDS["ajuda"]
CMDS["info"]  = function(con) con:_info() end
CMDS["ver"]   = CMDS["info"]
CMDS["sair"]  = function(con) con._deveSair = true end
CMDS["quit"]  = CMDS["sair"]
CMDS["exit"]  = CMDS["sair"]
CMDS["run"]   = function(con, args) con:_rodarArquivo(args) end

-- lista layouts disponíveis ou troca o layout ativo
CMDS["teclado"] = function(con, args)
    args = args:match("^%s*(.-)%s*$")
    if args == "" then
        con:_escrever("Layouts disponiveis:", C.CIANO)
        for _, lay in ipairs(KB.listar()) do
            local marcador = lay.ativo and "[*] " or "[ ] "
            con:_escrever(marcador .. lay.id .. "  " .. lay.nome, C.PRATA)
        end
        con:_escrever("uso: teclado <id>", C.CINZA)
        con:_escrever("ex:  teclado abnt2  ou  teclado br", C.CINZA)
        con:_escrever("     teclado us", C.CINZA)
    else
        if KB.setLayout(args) then
            con:_escrever("teclado: " .. KB.getNome(), C.VERDE)
            con:_escrever("(vale para editor e console)", C.CINZA)
        else
            con:_escrever("layout desconhecido: " .. args, C.VERMELHO)
            con:_escrever("use: teclado  (sem args para listar)", C.CINZA)
        end
    end
end

local Console = {}
Console.__index = Console

-- cria uma nova instância do console
function Console.novo(Tupi, ascii, runtime)
    assert(Tupi,  "[Console] Tupi obrigatorio")
    assert(ascii, "[Console] caminho do ascii.png obrigatorio")

    local self = setmetatable({}, Console)
    self.T          = Tupi
    self._runtime   = runtime
    self._buf       = {}
    self._input     = ""
    self._cursor    = 0
    self._blinkT    = 0
    self._cursorVis = true
    self._histCmd   = {}
    self._histIdx   = 0
    self._scroll    = 0
    self._ativo     = false
    self._deveSair  = false

    self._fonte = Tupi.texto.carregarFonte(ascii, 8, 8)
    N.patchTexto(Tupi.texto)

    self:_escrever("TUPIENGINE CONSOLE", C.CIANO)
    self:_escrever("Digite AJUDA para comandos.", C.PRATA)
    self:_escrever("", C.BRANCO)

    return self
end

function Console:ativar(v)    self._ativo = (v == nil) and not self._ativo or v end
function Console:estaAtivo()  return self._ativo     end
function Console:deveSair()   return self._deveSair  end

-- imprime uma linha no buffer do console
function Console:print(txt, cor)
    self:_escrever(tostring(txt or ""), cor)
end

function Console:atualizar(dt)
    if not self._ativo then return end
    local T   = self.T
    local inp = T
    dt = dt or T.janela.dt()

    -- atualiza cursor piscando
    self._blinkT = self._blinkT + dt
    if self._blinkT >= BLINK then
        self._blinkT    = self._blinkT - BLINK
        self._cursorVis = not self._cursorVis
    end

    local ctrl = inp.teclaSegurando(inp.TECLA_CTRL_ESQ)
              or inp.teclaSegurando(inp.TECLA_CTRL_DIR)

    if ctrl then
        -- scroll e atalhos com Ctrl
        if inp.teclaPressionou(inp.TECLA_CIMA) then
            self._scroll = math.min(self._scroll + 1, math.max(0, #self._buf - 1))
        end
        if inp.teclaPressionou(inp.TECLA_BAIXO) then
            self._scroll = math.max(self._scroll - 1, 0)
        end
        if inp.teclaPressionou(inp.TECLA_L) then self._buf = {} end
        if inp.teclaPressionou(inp.TECLA_A) then self._cursor = 0 end
        if inp.teclaPressionou(inp.TECLA_E) then self._cursor = #self._input end
    else
        -- histórico e movimentação do cursor
        if inp.teclaPressionou(inp.TECLA_CIMA) then
            local n = #self._histCmd
            if n > 0 then
                self._histIdx = math.min(self._histIdx + 1, n)
                self._input   = self._histCmd[n - self._histIdx + 1] or ""
                self._cursor  = #self._input
            end
        end
        if inp.teclaPressionou(inp.TECLA_BAIXO) then
            self._histIdx = math.max(self._histIdx - 1, 0)
            self._input   = self._histIdx == 0 and ""
                            or (self._histCmd[#self._histCmd - self._histIdx + 1] or "")
            self._cursor  = #self._input
        end
        if inp.teclaPressionou(inp.TECLA_ESQUERDA) then
            self._cursor = math.max(0, self._cursor - 1)
        end
        if inp.teclaPressionou(inp.TECLA_DIREITA) then
            self._cursor = math.min(#self._input, self._cursor + 1)
        end
    end

    if inp.teclaPressionou(inp.TECLA_BACKSPACE) and self._cursor > 0 then
        local s = self._input
        self._input  = s:sub(1, self._cursor - 1) .. s:sub(self._cursor + 1)
        self._cursor = self._cursor - 1
        self:_resetBlink()
    end

    if inp.teclaPressionou(inp.TECLA_ENTER) then
        self:_executar(self._input)
        self._input   = ""
        self._cursor  = 0
        self._histIdx = 0
        self._scroll  = 0
        self:_resetBlink()
    end

    self:_capturarTexto(inp)
end

function Console:desenhar()
    if not self._ativo then return end

    local T = self.T
    local W = T.largura()
    local H = T.altura()

    local LINHAS_VIS = math.floor((H - 24) / ESPV) - 2

    -- fundo preto
    T.cor(C.PRETO[1], C.PRETO[2], C.PRETO[3], 1)
    T.retangulo(0, 0, W, H)

    -- barra de título
    T.cor(C.ESCURO[1], C.ESCURO[2], C.ESCURO[3], 1)
    T.retangulo(0, 0, W, CH + 4)
    T.cor(C.CIANO[1], C.CIANO[2], C.CIANO[3], 1)
    T.texto.desenhar(4, 2, 10, "TUPI ENGINE CONSOLE", ESC, 1.0, self._fonte)

    -- indicador do layout no canto direito
    local ind   = "[" .. KB.getLayout():upper() .. "]"
    local ind_x = W - (#ind * CW) - 4
    T.cor(C.AMARELO[1], C.AMARELO[2], C.AMARELO[3], 0.85)
    T.texto.desenhar(ind_x, 2, 10, ind, ESC, 0.85, self._fonte)

    T.cor(C.CINZA[1], C.CINZA[2], C.CINZA[3], 1)
    T.retangulo(0, CH + 4, W, 1)
    T.cor(1, 1, 1, 1)

    -- linhas do buffer visíveis
    local total  = #self._buf
    local inicio = math.max(1, total - LINHAS_VIS - self._scroll + 1)
    local fim    = math.max(1, total - self._scroll)
    local y      = CH + 8

    for i = inicio, fim do
        local linha = self._buf[i]
        if linha then
            local cor = linha.cor or C.BRANCO
            T.cor(cor[1], cor[2], cor[3], 1)
            T.texto.desenhar(4, y, 5, linha.txt, ESC, 1.0, self._fonte)
        end
        y = y + ESPV
    end

    T.cor(1, 1, 1, 1)

    -- área de input na parte de baixo
    local y_prompt = H - CH - 6
    T.cor(C.CINZA[1], C.CINZA[2], C.CINZA[3], 1)
    T.retangulo(0, y_prompt - 3, W, 1)

    T.cor(C.ESCURO[1], C.ESCURO[2], C.ESCURO[3], 1)
    T.retangulo(0, y_prompt - 2, W, CH + 6)

    T.cor(C.VERDE[1], C.VERDE[2], C.VERDE[3], 1)
    T.texto.desenhar(4, y_prompt, 6, ">", ESC, 1.0, self._fonte)

    T.cor(C.BRANCO[1], C.BRANCO[2], C.BRANCO[3], 1)
    T.texto.desenhar(4 + CW + 2, y_prompt, 6, self._input, ESC, 1.0, self._fonte)

    T.cor(1, 1, 1, 1)

    -- cursor de texto
    if self._cursorVis then
        local cx = 4 + CW + 2 + self._cursor * CW
        T.cor(C.BRANCO[1], C.BRANCO[2], C.BRANCO[3], 0.85)
        T.retangulo(cx, y_prompt, 1, CH)
        T.cor(1, 1, 1, 1)
    end

    -- indicador de scroll ativo
    if self._scroll > 0 then
        T.cor(C.PRATA[1], C.PRATA[2], C.PRATA[3], 0.6)
        T.texto.desenhar(W - CW * 8 - 4, 2, 7, "[scroll]", ESC, 0.6, self._fonte)
        T.cor(1, 1, 1, 1)
    end

    T.batchDesenhar()
end

-- adiciona texto ao buffer, com quebra de linha automática
function Console:_escrever(txt, cor)
    txt = tostring(txt or "")
    local MAX_CHARS = math.floor((self.T.largura() - 8) / CW)
    if MAX_CHARS < 10 then MAX_CHARS = 10 end
    local linhas = _wrap(txt, MAX_CHARS)
    if #linhas == 0 then linhas = {""} end
    for _, parte in ipairs(linhas) do
        self._buf[#self._buf + 1] = { txt = parte, cor = cor }
        if #self._buf > MAX_BUF then table.remove(self._buf, 1) end
    end
end

-- reinicia o timer do cursor para evitar piscada logo após digitar
function Console:_resetBlink()
    self._blinkT    = 0
    self._cursorVis = true
end

function Console:_ajuda()
    self:_escrever("cls / clear    limpa tela", C.PRATA)
    self:_escrever("run [arq]      executa lua", C.PRATA)
    self:_escrever("teclado [id]   layout do teclado", C.PRATA)
    self:_escrever("info / ver     versao", C.PRATA)
    self:_escrever("sair / quit    fecha", C.PRATA)
    self:_escrever("<expr>         avalia Lua", C.PRATA)
    self:_escrever("Ctrl+Up/Dn     rola buffer", C.CINZA)
    self:_escrever("Up/Dn          historico", C.CINZA)
end

function Console:_info()
    self:_escrever("TupiEngine console 8bit", C.PRATA)
    ---@diagnostic disable-next-line: undefined-global
    self:_escrever("LuaJIT " .. (jit and jit.version or "?"), C.CINZA)
    self:_escrever("teclado: " .. KB.getNome(), C.CINZA)
end

-- interpreta e executa o comando digitado
function Console:_executar(cmd)
    cmd = cmd:match("^%s*(.-)%s*$")
    if cmd == "" then return end

    -- adiciona ao histórico se diferente do último
    if self._histCmd[#self._histCmd] ~= cmd then
        self._histCmd[#self._histCmd + 1] = cmd
        if #self._histCmd > MAX_HIST then table.remove(self._histCmd, 1) end
    end

    self:_escrever("> " .. cmd, C.CIANO)

    -- tenta como comando interno primeiro
    local tok, resto = cmd:match("^(%S+)%s*(.*)$")
    if tok then
        local fn = CMDS[tok:lower()]
        if fn then fn(self, resto or ""); return end
    end

    -- tenta avaliar como expressão Lua
    local chunk = load("return " .. cmd, "=con", "t", _G)
    if chunk then
        local ok, val = pcall(chunk)
        if ok then
            if val ~= nil then self:_escrever(tostring(val), C.ROXO) end
            return
        end
    end

    -- tenta executar como statement Lua
    local chunk2, err2 = load(cmd, "=con", "t", _G)
    if chunk2 then
        local ok2, err3 = pcall(chunk2)
        if not ok2 then self:_escrever(tostring(err3), C.VERMELHO) end
    else
        self:_escrever(tostring(err2), C.VERMELHO)
    end
end

-- roda um arquivo .lua pelo runtime ou loadfile
function Console:_rodarArquivo(path)
    path = tostring(path or ""):match("^%s*(.-)%s*$")
    if path == "" then
        local ed = rawget(_G, "editor")
        if ed and ed._arquivo then
            path = ed._arquivo
        else
            local f = io.open("run.lua", "r")
            if f then
                f:close()
                path = "run.lua"
            else
                self:_escrever("salve um arquivo no editor ou use run <arquivo.lua>", C.PRATA)
                return
            end
        end
    end
    if not self._runtime then
        self:_escrever("runtime visual indisponivel", C.VERMELHO)
        return
    end
    local ok, err = self._runtime:rodarArquivo(path, "console")
    if not ok then
        self:_escrever(tostring(err), C.VERMELHO)
    else
        self:_escrever("run: " .. path, C.VERDE)
    end
end

-- captura teclas e insere caracteres no input, delegando layout ao KB
function Console:_capturarTexto(inp)
    local shift = inp.teclaSegurando(inp.TECLA_SHIFT_ESQ)
               or inp.teclaSegurando(inp.TECLA_SHIFT_DIR)

    for _, l in ipairs{"A","B","C","D","E","F","G","H","I","J","K","L","M",
                        "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"} do
        local code = inp["TECLA_" .. l]
        if code and inp.teclaPressionou(code) then
            self:_inserir(shift and l or l:lower())
        end
    end

    for d = 0, 9 do
        local code = inp["TECLA_" .. d]
        if code and inp.teclaPressionou(code) then
            self:_inserir(shift and KB.getNumShift(d) or tostring(d))
        end
    end

    for _, s in ipairs(KB.getSimb()) do
        if inp.teclaPressionou(s.c) then
            self:_inserir(shift and s.s or s.n)
        end
    end
end

-- insere um caractere na posição do cursor
function Console:_inserir(c)
    local s = self._input
    self._input  = s:sub(1, self._cursor) .. c .. s:sub(self._cursor + 1)
    self._cursor = self._cursor + 1
    self:_resetBlink()
end

return Console