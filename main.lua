-- main.lua — TupiEngine 8bit: Splash → Console/Editor

local Tupi   = require("src.Engine.TupiEngine")
local Splash = require("src.Engine.tupi_splashscreen")
local Console= require("src.Engine.tupi_console")
local Editor = require("src.Engine.tupi_editor")
local Sintaxe= require("src.Engine.sintaxe")
local N      = require("src.Engine.texto_normalizar")

-- ============================================================
-- JANELA
-- ============================================================
Tupi.janela(160, 144, "TupiEngine — 8bit", 4.0)
Tupi.telaCheia_letterbox(false)

-- Normaliza acentos para a fonte bitmap (UTF-8 → Latin-1)
N.patchTexto(Tupi.texto)

-- ============================================================
-- SPLASH SCREEN
-- ============================================================
local splash = Splash.novo(Tupi, "ascii.png", 2.0)
while Tupi.rodando() and not splash:terminou() do
    Tupi.limpar()
    local ctrl = Tupi.teclaSegurando(Tupi.TECLA_CTRL_ESQ)
              or Tupi.teclaSegurando(Tupi.TECLA_CTRL_DIR)
    if ctrl and Tupi.teclaPressionou(Tupi.TECLA_1) then
        Tupi.telaCheia_letterbox(not Tupi.letterboxAtivo())
    end
    splash:atualizar()
    splash:desenhar()
    Tupi.atualizar()
end
splash = nil
Tupi.atualizar()


-- ============================================================
-- CONSOLE + EDITOR
-- ============================================================
local runtime = Sintaxe.novo(Tupi, "ascii.png")
local con = Console.novo(Tupi, "ascii.png", runtime)
local ed  = Editor.novo(Tupi,  "ascii.png", "editor.png", runtime)
ed:abrirArquivo("run.lua")

-- Após a splash, entra no console. Ctrl+5 alterna para o editor.
con:ativar(true)
ed:ativar(false)

-- Expõe globals úteis no console
_G.Tupi   = Tupi
_G.editor = ed     -- ex: editor:abrirArquivo("game.lua")
_G.runtime = runtime

con:print("Ctrl+5  > abre editor", nil)
con:print("run     > roda run.lua ou o arquivo atual", nil)
con:print("Ctrl+1  > tela cheia", nil)

-- ============================================================
-- LOOP PRINCIPAL
-- ============================================================
while Tupi.rodando() do
    Tupi.limpar()
    local dt = Tupi.dt()

    local ctrl = Tupi.teclaSegurando(Tupi.TECLA_CTRL_ESQ)
              or Tupi.teclaSegurando(Tupi.TECLA_CTRL_DIR)

    -- ── Ctrl+1 — tela cheia letterbox (sempre ativo) ─────────
    if ctrl and Tupi.teclaPressionou(Tupi.TECLA_1) then
        Tupi.telaCheia_letterbox(not Tupi.letterboxAtivo())
        goto continuar
    end

    -- ── Ctrl+5 — alterna console ↔ editor ────────────────────
    if (not runtime:estaAtivo()) and ctrl and Tupi.teclaPressionou(Tupi.TECLA_5) then
        if ed:estaAtivo() then
            ed:ativar(false)
            con:ativar(true)
        else
            con:ativar(false)
            ed:ativar(true)
        end
        goto continuar
    end

    -- ── RUNTIME VISUAL ────────────────────────────────────────
    if runtime:estaAtivo() then
        runtime:atualizar()
        runtime:desenhar()
        local retorno = runtime:consumirRetorno()
        if retorno then
            if retorno == "editor" then
                ed:ativar(true)
                con:ativar(false)
            else
                ed:ativar(false)
                con:ativar(true)
            end
        end
    -- ── MÓDULO ATIVO ──────────────────────────────────────────
    elseif ed:estaAtivo() then
        -- ESC dentro do editor é tratado pelo próprio editor
        -- (fecha prompt interno; se não há prompt, volta ao console)
        ed:atualizar(dt)
        ed:desenhar()
        if not ed:estaAtivo() then
            con:ativar(true)
        end
    else
        -- No console: ESC fecha o programa
        if Tupi.teclaPressionou(Tupi.TECLA_ESC) then break end
        con:atualizar(dt)
        con:desenhar()
        if con:deveSair() then break end
    end

    ::continuar::
    Tupi.atualizar()
end

Tupi.fechar()
