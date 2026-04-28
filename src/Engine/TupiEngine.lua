local Core   = require("src.Engine.engine_core")
local Visual = require("src.Engine.engine_visual")
local Mundo  = require("src.Engine.engine_mundo")

local function _getDt()        return Core.janela.dt()      end
local function _getLargura()   return Core.janela.largura()  end
local function _getMouse()     return Core.input.mouseX(), Core.input.mouseY() end
local function _getDimensoes() return Core.janela.largura(), Core.janela.altura() end

Core.input._getDt          = _getDt
Core.colisao._getMouse     = _getMouse
Core.fisica._getDt         = _getDt
Visual.sprite._getDt       = _getDt
Visual.sprite._aplicarCor  = Core.render._aplicarCor
Visual.sprite._retRet      = Core.colisao.retRet
Visual.sprite._retRetInfo  = Core.colisao.retRetInfo
Visual.camera._getDt       = _getDt
Visual.camera._getMouse    = _getMouse
Visual.camera._getLargura  = _getLargura

local Tupi = {}

local J = Core.janela
Tupi.janela=J.janela; Tupi.rodando=J.rodando; Tupi.limpar=J.limpar
Tupi.atualizar=J.atualizar; Tupi.apresentar=J.atualizar; Tupi.fechar=J.fechar
Tupi.tempo=J.tempo; Tupi.dt=J.dt
Tupi.largura=J.largura; Tupi.altura=J.altura; Tupi.larguraPx=J.larguraPx; Tupi.alturaPx=J.alturaPx
Tupi.escala=J.escala; Tupi.setTitulo=J.setTitulo; Tupi.setDecoracao=J.setDecoracao
Tupi.telaCheia=J.telaCheia; Tupi.telaCheia_letterbox=J.telaCheia_letterbox; Tupi.letterboxAtivo=J.letterboxAtivo
Tupi.lerp=J.lerp; Tupi.aleatorio=J.aleatorio; Tupi.rad=J.rad; Tupi.graus=J.graus; Tupi.distancia=J.distancia
Tupi.fpsLimite=J.fpsLimite; Tupi.fpsAtual=J.fpsAtual

local R = Core.render
Tupi.corFundo=R.corFundo; Tupi.cor=R.cor; Tupi.usarCor=R.usarCor
Tupi.retangulo=R.retangulo; Tupi.retanguloBorda=R.retanguloBorda; Tupi.triangulo=R.triangulo
Tupi.circulo=R.circulo; Tupi.circuloBorda=R.circuloBorda; Tupi.linha=R.linha
Tupi.batchDesenhar=R.batchDesenhar
Tupi.BRANCO=R.BRANCO; Tupi.PRETO=R.PRETO; Tupi.VERMELHO=R.VERMELHO; Tupi.VERDE=R.VERDE
Tupi.AZUL=R.AZUL; Tupi.AMARELO=R.AMARELO; Tupi.ROXO=R.ROXO; Tupi.LARANJA=R.LARANJA
Tupi.CIANO=R.CIANO; Tupi.CINZA=R.CINZA; Tupi.ROSA=R.ROSA

local I = Core.input
Tupi.setTempoSegurando=I.setTempoSegurando; Tupi.getTempoSegurando=I.getTempoSegurando
Tupi.teclaPressionou=I.teclaPressionou; Tupi.teclaSoltou=I.teclaSoltou; Tupi.teclaSegurando=I.teclaSegurando
Tupi.mouseX=I.mouseX; Tupi.mouseY=I.mouseY; Tupi.mouseDX=I.mouseDX; Tupi.mouseDY=I.mouseDY
Tupi.mousePos=I.mousePos; Tupi.mouseXRaw=I.mouseXRaw; Tupi.mouseYRaw=I.mouseYRaw
Tupi.mouseClicou=I.mouseClicou; Tupi.mouseSegurando=I.mouseSegurando; Tupi.mouseSoltou=I.mouseSoltou
Tupi.scrollX=I.scrollX; Tupi.scrollY=I.scrollY
for k, v in pairs(I) do if type(v) == "number" then Tupi[k] = v end end

Tupi.col = Core.colisao

local S = Visual.sprite
Tupi.carregarSprite=S.carregarSprite; Tupi.destruirSprite=S.destruirSprite
Tupi.criarObjeto=S.criarObjeto; Tupi.desenharObjeto=S.desenharObjeto
Tupi.desenhar=S.desenhar; Tupi.espelhar=S.espelhar; Tupi.getEspelho=S.getEspelho; Tupi.enviarBatch=S.enviarBatch
Tupi.criarAnim=S.criarAnim; Tupi.tocarAnim=S.tocarAnim; Tupi.pararAnim=S.pararAnim
Tupi.animTerminou=S.animTerminou; Tupi.animReiniciar=S.animReiniciar; Tupi.animLimparObjeto=S.animLimparObjeto
Tupi.mover=S.mover; Tupi.teleportar=S.teleportar; Tupi.salvarPosicao=S.salvarPosicao
Tupi.posicaoAtual=S.posicaoAtual; Tupi.ultimaPosicao=S.ultimaPosicao; Tupi.voltarPosicao=S.voltarPosicao
Tupi.distanciaObjetos=S.distanciaObjetos; Tupi.moverParaAlvo=S.moverParaAlvo; Tupi.perseguir=S.perseguir
Tupi.hitbox=S.hitbox; Tupi.hitboxFixa=S.hitboxFixa; Tupi.hitboxDesenhar=S.hitboxDesenhar
Tupi.moverComColisao=S.moverComColisao; Tupi.resolverColisaoSolida=S.resolverColisaoSolida
Tupi.destruir=S.destruir; Tupi.destruido=S.destruido

Tupi.camera=Visual.camera.camera; Tupi.paralax=Visual.camera.paralax
Tupi.cameraCriar=Visual.camera.camera.criar; Tupi.cameraDestruir=Visual.camera.camera.destruir
Tupi.cameraAtivar=Visual.camera.camera.ativar; Tupi.cameraPos=Visual.camera.camera.pos
Tupi.cameraMover=Visual.camera.camera.mover; Tupi.cameraZoom=Visual.camera.camera.zoom
Tupi.cameraRotacao=Visual.camera.camera.rotacao; Tupi.cameraAncora=Visual.camera.camera.ancora
Tupi.cameraSeguir=Visual.camera.camera.seguir; Tupi.cameraPosAtual=Visual.camera.camera.posAtual
Tupi.cameraAlvoAtual=Visual.camera.camera.alvoAtual; Tupi.cameraZoomAtual=Visual.camera.camera.zoomAtual
Tupi.cameraRotacaoAtual=Visual.camera.camera.rotacaoAtual
Tupi.cameraTelaMundo=Visual.camera.camera.telaMundo; Tupi.cameraMundoTela=Visual.camera.camera.mundoTela
Tupi.cameraMouseMundo=Visual.camera.camera.mouseMundo
Tupi.paralaxRegistrar=Visual.camera.paralax.registrar; Tupi.paralaxRemover=Visual.camera.paralax.remover
Tupi.paralaxAtualizar=Visual.camera.paralax.atualizar; Tupi.paralaxOffset=Visual.camera.paralax.offset
Tupi.paralaxDesenhar=Visual.camera.paralax.desenhar; Tupi.paralaxDesenharTile=Visual.camera.paralax.desenharTile
Tupi.paralaxResetar=Visual.camera.paralax.resetar; Tupi.paralaxResetarCamada=Visual.camera.paralax.resetarCamada
Tupi.paralaxSetFator=Visual.camera.paralax.setFator; Tupi.paralaxTotalAtivas=Visual.camera.paralax.totalAtivas

Tupi.fisica = Core.fisica

Tupi.texto = Visual.texto

Tupi.Fade = Visual.fade

Tupi.cenas = Mundo.cenas

Tupi.carregarMapa = Mundo.mapa.carregar
function Tupi.atualizarMapa(inst, dt)
    assert(inst and inst.atualizar); inst:atualizar(dt or J.dt())
end
function Tupi.desenharMapa(inst, z)
    assert(inst and inst.desenhar); inst:desenhar(z or 0)
end
function Tupi.destruirMapa(inst)
    if inst and inst.destruir then inst:destruir() end
end
function Tupi.hitboxTile(inst, col, lin)   assert(inst and inst.hitboxTile);   return inst:hitboxTile(col,lin) end
function Tupi.isSolido(inst, col, lin)     assert(inst and inst.isSolido);     return inst:isSolido(col,lin)   end
function Tupi.isTrigger(inst, col, lin)    assert(inst and inst.isTrigger);    return inst:isTrigger(col,lin)  end
function Tupi.tileEmPonto(inst, px, py)    assert(inst and inst.tileEmPonto);  return inst:tileEmPonto(px,py)  end

Tupi.mundos = Mundo.mundos

Visual.patchCor(Tupi)
return Tupi