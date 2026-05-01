local Tupi = require("src.Engine.sintaxe")
Tupi.janela(160, 144, "Meu Jogo", 5.0)

local player = {
    x = 20, y = 30, larg = 8, alt = 8
}
local s
local velocidade = 60
local virou = false
local mapa
local vila = {
    8, 8, 8, 8, 8,
    8, 2, 2, 2, 8,
    8, 2, 2, 2, 8,
    8, 2, 2, 2, 8,
    8, 8, 8, 8, 8,
}

function _iniciar()
    s = Tupi.imagem("tileset.png")
    player = Tupi.objeto(s, player.x, player.y, {larg=player.larg, alt=player.alt})
    mapa = Tupi.mapc("tileset.png", 8, 8, 5, 5)
    Tupi.mapa(mapa, vila)
end
function _rodar()
    local vel = velocidade * Tupi.dt()
    if Tupi.botao("a") then Tupi.mover(player, -vel, 0); virou = false
    elseif Tupi.botao("d") then Tupi.mover(player, vel, 0); virou = true
    elseif Tupi.botao("w") then Tupi.mover(player, 0, -vel)
    elseif Tupi.botao("s") then Tupi.mover(player, 0, vel)end
    Tupi.espelhar(player, virou, false)

end
function _desenhar()
    Tupi.mapd(mapa, 1)
    Tupi.draw(player)
    Tupi.fpsLimite(64)
end

Tupi.rodar()