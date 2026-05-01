# 📚 Documentação da Engine Tupi — Guia para Iniciantes

> Este guia explica, do zero, como usar a engine **Tupi** para criar jogos em Lua.  
> Não precisa de experiência prévia em programação — cada função é explicada com exemplos práticos.

---

## Índice

1. [O que é a engine Tupi?](#o-que-é-a-engine-tupi)
2. [Estrutura básica de um jogo](#estrutura-básica-de-um-jogo)
3. [Janela](#janela)
4. [Cores e Desenho](#cores-e-desenho)
5. [Texto na tela](#texto-na-tela)
6. [Imagens e Objetos (Sprites)](#imagens-e-objetos-sprites)
7. [Animações](#animações)
8. [Câmera](#câmera)
9. [Paralaxe](#paralaxe)
10. [Input — Teclado](#input--teclado)
11. [Input — Mouse](#input--mouse)
12. [Campo de Texto](#campo-de-texto)
13. [Colisão](#colisão)
14. [Física](#física)
15. [Mapas de Tiles](#mapas-de-tiles)
16. [Mundos (Cenas)](#mundos-cenas)
17. [Fade (Transição de tela)](#fade-transição-de-tela)
18. [Matemática](#matemática)
19. [Aliases rápidos](#aliases-rápidos)

---

## O que é a engine Tupi?

A engine **Tupi** é uma biblioteca para Lua que facilita a criação de jogos 2D. Ela reúne num só lugar tudo que você precisa: janela, gráficos, sprites, input, física, mapas e muito mais.

Ao chamar `Tupi.rodar()`, o engine:
1. Chama sua função `_iniciar()` uma vez (para configurar tudo).
2. Fica chamando `_rodar()` e `_desenhar()` repetidamente, frame a frame, até o jogo fechar.

---

## Estrutura básica de um jogo

```lua
local Tupi = require("sintaxe")

-- Configuração inicial da janela
Tupi.janela(320, 240, "Meu Jogo", 2)

-- Chamada uma vez no começo
function _iniciar()
    -- carregue imagens, crie objetos, etc.
end

-- Chamada todo frame — lógica do jogo
function _rodar()
    -- movimento, input, colisão...
end

-- Chamada todo frame — desenho
function _desenhar()
    Tupi.limparTela()
    -- desenhe sprites, formas, texto...
end

-- Inicia o loop!
Tupi.rodar()
```

---

## Janela

Funções para criar e controlar a janela do jogo.

### `Tupi.janela(largura, altura, titulo, escala, semBorda, imagem)`

Cria a janela do jogo. **Deve ser chamada antes de `Tupi.rodar()`.**

| Parâmetro | Tipo | Descrição |
|-----------|------|-----------|
| `largura` | número | Largura em pixels (ex: `320`) |
| `altura` | número | Altura em pixels (ex: `240`) |
| `titulo` | string | Título que aparece na barra da janela |
| `escala` | número | Quanto ampliar a janela (ex: `2` = dobrar) |
| `semBorda` | bool | `true` para janela sem barra de título |
| `imagem` | string | Caminho para ícone da janela (opcional) |

```lua
Tupi.janela(320, 240, "Meu Jogo", 2)
-- Cria uma janela de 320x240, ampliada 2x (fica 640x480 na tela)
```

### Funções de janela

| Função | Retorna | O que faz |
|--------|---------|-----------|
| `Tupi.rodando()` | bool | `true` enquanto o jogo estiver aberto |
| `Tupi.limparTela()` | — | Limpa a tela antes de desenhar |
| `Tupi.atualizar()` | — | Apresenta o frame na tela |
| `Tupi.fechar()` | — | Fecha o jogo |
| `Tupi.tempo()` | número | Segundos desde o início |
| `Tupi.dt()` | número | Tempo do último frame em segundos (delta time) |
| `Tupi.largura()` | número | Largura lógica da janela em tiles/pixels |
| `Tupi.altura()` | número | Altura lógica da janela em tiles/pixels |
| `Tupi.larguraPx()` | número | Largura real em pixels na tela |
| `Tupi.alturaPx()` | número | Altura real em pixels na tela |
| `Tupi.escalaJanela()` | número | Fator de escala atual |
| `Tupi.titulo(t)` | — | Muda o título da janela |
| `Tupi.telaCheia(ativo, lb)` | — | Liga/desliga tela cheia |
| `Tupi.fpsLimite(n)` | — | Define limite de FPS |
| `Tupi.fpsAtual()` | número | FPS atual do jogo |

> 💡 **O que é `dt`?** Delta time é o tempo que passou desde o último frame. Usar `dt` no movimento garante que o jogo rode na mesma velocidade em qualquer computador. Ex: `x = x + velocidade * Tupi.dt()`

---

## Cores e Desenho

### Definindo cores

```lua
Tupi.cor(r, g, b, a)
-- r, g, b, a são valores de 0.0 a 1.0
-- a = transparência (1.0 = sólido, 0.0 = invisível)

Tupi.cor(1, 0, 0, 1)  -- vermelho puro
Tupi.cor(0, 1, 0, 1)  -- verde puro
Tupi.cor(0, 0, 1, 1)  -- azul puro
```

```lua
Tupi.usarCor(tabela, alpha)
-- Aplica uma cor definida como tabela {r, g, b, a}

Tupi.usarCor({1, 0.5, 0, 1})  -- laranja
```

```lua
Tupi.corFundo(r, g, b)
-- Define a cor de fundo da tela (usada ao limpar)

Tupi.corFundo(0.1, 0.1, 0.2)  -- azul escuro
```

### Cores predefinidas

Você pode usar essas cores diretamente como tabelas `{r, g, b, a}`:

```lua
Tupi.BRANCO    Tupi.PRETO
Tupi.VERMELHO  Tupi.VERDE
Tupi.AZUL      Tupi.AMARELO
Tupi.ROXO      Tupi.LARANJA
Tupi.CIANO     Tupi.CINZA
Tupi.ROSA
```

```lua
Tupi.usarCor(Tupi.VERMELHO)  -- aplica vermelho
```

### Paleta PICO-8 (16 cores, índices 0–15)

```lua
Tupi.PALETA[0]   -- preto
Tupi.PALETA[7]   -- branco
Tupi.PALETA[8]   -- vermelho vivo
Tupi.PALETA[11]  -- verde vivo
-- ... até índice 15
```

### Formas geométricas

#### Retângulo preenchido
```lua
Tupi.retangulo(x, y, largura, altura, cor)

Tupi.retangulo(10, 20, 50, 30, Tupi.AZUL)
-- Desenha um retângulo azul na posição (10,20) com 50x30 pixels
```

#### Retângulo com borda
```lua
Tupi.bordaRet(x, y, largura, altura, espessura, cor)

Tupi.bordaRet(10, 20, 50, 30, 2, Tupi.BRANCO)
-- Borda branca de 2px
```

#### Círculo preenchido
```lua
Tupi.circulo(x, y, raio, segmentos, cor)

Tupi.circulo(100, 100, 20, 32, Tupi.VERDE)
-- Círculo verde com raio 20, centrado em (100,100)
```

#### Círculo com borda
```lua
Tupi.bordaCirc(x, y, raio, segmentos, espessura, cor)
```

#### Linha
```lua
Tupi.linha(x1, y1, x2, y2, espessura, cor)

Tupi.linha(0, 0, 100, 100, 1, Tupi.BRANCO)
-- Linha branca do (0,0) ao (100,100)
```

#### Triângulo
```lua
Tupi.triangulo(x1, y1, x2, y2, x3, y3, cor)
```

#### Pixel
```lua
Tupi.pixel(x, y, cor)
-- Desenha um único pixel
```

#### Flush (enviar ao render)
```lua
Tupi.flush()
-- Força o envio do batch de desenhos para a tela
-- Normalmente chamado automaticamente
```

---

## Texto na tela

Para escrever texto, primeiro carregue uma fonte bitmap (uma imagem `.png` com os caracteres).

### Carregando fonte

```lua
local fonte = Tupi.carregarFonte(caminho, largChar, altChar, colunas, charInicio)

local fonte = Tupi.carregarFonte("fonte.png", 8, 8)
-- Carrega uma fonte com caracteres de 8x8 pixels
```

```lua
Tupi.setFontePadrao(fonte)
-- Define como fonte padrão (usada quando você não especifica uma)
```

### Escrevendo texto

```lua
Tupi.escrever(texto, x, y, z, escala, transparencia, cor, fonte)
```

| Parâmetro | Tipo | Padrão | Descrição |
|-----------|------|--------|-----------|
| `texto` | string | — | O texto a escrever |
| `x`, `y` | número | `0, 0` | Posição na tela |
| `z` | número | `10` | Profundidade (camada) |
| `escala` | número | `1.0` | Tamanho (ex: `2` = dobrar) |
| `transparencia` | número | `1.0` | Opacidade (0 a 1) |
| `cor` | tabela ou número | branco | Cor do texto |
| `fonte` | fonte | padrão | Fonte específica |

```lua
Tupi.escrever("Olá, Mundo!", 10, 10)
Tupi.escrever("Pontuação: "..pontos, 5, 5, 10, 1.0, 1.0, Tupi.AMARELO)
Tupi.escrever("texto", 10, 10, 10, 1.0, 1.0, 8)  -- cor da paleta índice 8
```

### Texto com sombra

```lua
Tupi.escreverSombra(texto, x, y, z, dX, dY, escala, transp, escSombra, transpSombra, fonte)
-- dX, dY = deslocamento da sombra (ex: 1, 1)
```

### Texto em caixa

```lua
Tupi.escreverCaixa(texto, x, y, z, larg, alt, escala, transp, fonte, frame, tamTile, escBorda, transpBorda, recuo)
-- Escreve texto dentro de uma caixa com borda decorada
```

### Medindo texto

```lua
local w = Tupi.larguraTexto(fonte, "Olá", 1.0)
local h = Tupi.alturaTexto(fonte, "Olá", 1.0)
local w, h = Tupi.dimensoesTexto(fonte, "Olá", 1.0)
```

---

## Imagens e Objetos (Sprites)

### Carregando uma imagem

```lua
local spr = Tupi.imagem("jogador.png")
-- Carrega a imagem e retorna um sprite

Tupi.destruirImagem(spr)  -- libera da memória quando não precisar mais
```

### Criando um objeto

Um **objeto** é um sprite com posição, tamanho e outras propriedades.

```lua
local obj = Tupi.objeto(sprite, x, y, opcoes)
```

| Opção | Padrão | Descrição |
|-------|--------|-----------|
| `larg` / `largura` | `16` | Largura do frame em pixels |
| `alt` / `altura` | `16` | Altura do frame em pixels |
| `z` | `0` | Profundidade (camada de desenho) |
| `col` / `coluna` | `0` | Coluna do frame no spritesheet |
| `lin` / `linha` | `0` | Linha do frame no spritesheet |
| `alfa` / `transparencia` | `1.0` | Transparência |
| `escala` | `1.0` | Tamanho |

```lua
local spr = Tupi.imagem("personagem.png")
local jogador = Tupi.objeto(spr, 100, 100, {larg=16, alt=16})
```

### Desenhando um objeto

```lua
Tupi.mostrar(objeto, z)
-- Envia o objeto para ser desenhado neste frame

Tupi.mostrar(jogador)
```

### Desenhando um sprite direto (sem criar objeto)

```lua
Tupi.desenharSprite(sprite, x, y, opcoes)
```

### Movendo e posicionando

```lua
Tupi.mover(objeto, dx, dy)
-- Move o objeto por dx, dy pixels (relativo à posição atual)

Tupi.posicionar(objeto, x, y)
-- Teleporta o objeto para a posição exata (x, y)

local x, y = Tupi.posicao(objeto)
-- Retorna a posição atual
```

### Outras transformações

```lua
Tupi.escalaObj(objeto, escala)    -- muda o tamanho (1.0 = normal, 2.0 = dobrado)
Tupi.alfa(objeto, valor)          -- transparência de 0.0 a 1.0
Tupi.tamanho(objeto, largura, altura)  -- muda o tamanho do frame
Tupi.quadro(objeto, coluna, linha)     -- muda o frame no spritesheet
Tupi.espelhar(objeto, horizontal, vertical)  -- espelha o sprite
Tupi.destruir(objeto, liberarSprite)   -- remove o objeto
```

### Movimento suave e perseguição

```lua
Tupi.moverParaAlvo(objeto, alvoX, alvoY, fator)
-- Move gradualmente em direção ao alvo (fator: velocidade 0.0 a 1.0)

Tupi.perseguir(objeto, alvo, velocidade)
-- Move o objeto em direção a outro objeto
```

### Posição salva

```lua
Tupi.salvarPosicao(objeto)
-- Salva a posição atual

Tupi.voltarPosicao(objeto)
-- Volta para a posição salva (útil para desfazer colisões)

local x, y = Tupi.ultimaPosicao(objeto)
```

### Distância entre objetos

```lua
local dist = Tupi.distanciaObjetos(objetoA, objetoB)
```

### Hitbox (área de colisão)

```lua
-- Hitbox relativa ao objeto
local hb = Tupi.hitbox(objeto, offsetX, offsetY, largura, altura, escalar)

-- Hitbox em posição fixa no mundo
local hb = Tupi.hitboxFixa(objeto, x, y, largura, altura)

-- Desenhar hitbox (útil para debug)
Tupi.hitboxDesenhar(hitbox, cor, espessura)
```

---

## Animações

Animações funcionam trocando frames de um spritesheet ao longo do tempo.

### Criando uma animação

```lua
local anim = Tupi.criarAnim(sprite, larg, alt, colunas, linhas, fps, loop)
```

| Parâmetro | Descrição |
|-----------|-----------|
| `sprite` | Sprite carregado com `Tupi.imagem()` |
| `larg`, `alt` | Tamanho de cada frame |
| `colunas` | Array com índices das colunas — ex: `{0, 1, 2, 3}` |
| `linhas` | Array com índices das linhas — ex: `{0, 0, 0, 0}` |
| `fps` | Frames por segundo da animação |
| `loop` | `true` para repetir, `false` para tocar uma vez |

```lua
local spr = Tupi.imagem("heroi.png")
local animCorrer = Tupi.criarAnim(spr, 16, 16, {0,1,2,3}, {0,0,0,0}, 8, true)
```

### Tocando a animação

```lua
Tupi.tocarAnim(anim, objeto, z)
-- Chame dentro de _desenhar() a cada frame
```

### Parando a animação

```lua
Tupi.pararAnim(anim, objeto, frame, z)
-- frame pode ser um número (0-based) ou {coluna, linha}
```

### Verificando estado

```lua
if Tupi.animTerminou(anim, objeto) then
    -- Animação chegou ao fim (útil para animações sem loop)
end

Tupi.animReiniciar(anim, objeto)  -- volta ao frame inicial
Tupi.animLimpar(objeto)            -- remove animação do objeto
```

---

## Câmera

A câmera controla qual parte do mundo está visível na tela.

### Criando e ativando

```lua
local cam = Tupi.criarCamera(ancX, ancY, ancW, ancH)
-- ancX, ancY: posição inicial; ancW, ancH: âncora (ponto de referência)

Tupi.ativarCamera(cam)
-- Ativa a câmera (só uma pode estar ativa por vez)
```

### Controlando a câmera

```lua
Tupi.cameraPosicao(cam, x, y)      -- move para posição exata
Tupi.cameraMover(cam, dx, dy)       -- move relativamente
Tupi.cameraZoom(cam, fator)         -- zoom (1.0 = normal, 2.0 = ampliado)
Tupi.cameraRotacao(cam, angulo)     -- rotaciona (em radianos)
Tupi.cameraAncora(cam, ax, ay)      -- define ponto de âncora
Tupi.cameraSeguir(cam, x, y, vel)   -- segue suavemente uma posição
```

### Lendo o estado

```lua
local x, y = Tupi.cameraPosAtual(cam)
local zoom  = Tupi.cameraZoomAtual(cam)
local rot   = Tupi.cameraRotacaoAtual(cam)
```

### Convertendo coordenadas

```lua
-- Posição na tela → posição no mundo
local wx, wy = Tupi.cameraTelaMundo(cam, sx, sy)

-- Posição no mundo → posição na tela
local sx, sy = Tupi.cameraMundoTela(cam, wx, wy)

-- Posição do mouse no mundo
local mx, my = Tupi.cameraMouseMundo(cam)
```

---

## Paralaxe

Paralaxe cria a ilusão de profundidade movendo camadas de fundo em velocidades diferentes.

```lua
local id = Tupi.registrarParalax(fatorX, fatorY, z, largura, altura)
-- fatorX/fatorY: 0.0 = fundo fixo, 1.0 = segue a câmera junto

Tupi.atualizarParalax(cam, camX, camY)
-- Atualiza todas as camadas (chame em _rodar())

Tupi.desenharParalax(id, wrapper)
-- Desenha uma camada (chame em _desenhar())

Tupi.removerParalax(id)       -- remove uma camada
Tupi.resetarParalax()          -- remove todas as camadas
Tupi.setFatorParalax(id, fx, fy)  -- muda o fator de uma camada
```

---

## Input — Teclado

### Verificando teclas

```lua
Tupi.botao(tecla)
-- Retorna true enquanto a tecla estiver pressionada (modo "segurar")

Tupi.pressionou(tecla)
-- Retorna true apenas no frame em que a tecla foi pressionada

Tupi.soltou(tecla)
-- Retorna true apenas no frame em que a tecla foi solta
```

### Formas de especificar uma tecla

**1. Nome da tecla como string:**
```lua
if Tupi.pressionou("espaco") then pular() end
if Tupi.botao("esquerda")    then moverEsq() end
if Tupi.botao("a")           then atirar() end
if Tupi.botao("f5")          then salvar() end
```

**2. Combinações de teclas:**
```lua
if Tupi.pressionou("ctrl+s")       then salvar() end
if Tupi.pressionou("ctrl+shift+z") then refazer() end
if Tupi.botao("shift+esquerda")    then correr() end
```

**3. Estilo PICO-8 (números 0–5):**
```lua
-- 0=esquerda, 1=direita, 2=cima, 3=baixo, 4=Z, 5=X
if Tupi.botao(0) then moverEsq() end
if Tupi.botao(2) then pular() end
```

**4. Constante direta:**
```lua
if Tupi.botao(Tupi.TECLA_ENTER) then confirmar() end
```

### Aliases de teclas disponíveis

| String | Tecla |
|--------|-------|
| `"espaco"`, `"space"` | Espaço |
| `"enter"` | Enter |
| `"esc"`, `"escape"` | Escape |
| `"cima"`, `"up"` | Seta cima |
| `"baixo"`, `"down"` | Seta baixo |
| `"esquerda"`, `"left"` | Seta esquerda |
| `"direita"`, `"right"` | Seta direita |
| `"ctrl"`, `"control"` | Ctrl esquerdo |
| `"shift"` | Shift esquerdo |
| `"alt"` | Alt esquerdo |
| `"tab"` | Tab |
| `"backspace"`, `"bs"` | Backspace |
| `"del"`, `"delete"` | Delete |
| `"f1"` … `"f12"` | Teclas de função |
| `"a"` … `"z"` | Letras |
| `"0"` … `"9"` | Números |
| `"num0"` … `"num9"` | Teclado numérico |

### Configurações do teclado

```lua
Tupi.setLayout("abnt")   -- layout do teclado (ex: "abnt", "us")
Tupi.getLayout()          -- retorna layout atual

Tupi.setRepeat(atraso, passo)
-- atraso: segundos antes de repetir ao segurar
-- passo: intervalo entre repetições
```

---

## Input — Mouse

```lua
local x = Tupi.mouseX()     -- posição X do cursor
local y = Tupi.mouseY()     -- posição Y do cursor
local x, y = Tupi.mousePos()  -- retorna x e y de uma vez

local dx = Tupi.mouseDX()   -- quanto o mouse se moveu em X neste frame
local dy = Tupi.mouseDY()   -- quanto o mouse se moveu em Y neste frame
```

### Botões do mouse

```lua
Tupi.mouseClicou(botao)    -- true só no frame do clique
Tupi.mouseBotao(botao)     -- true enquanto o botão estiver pressionado
Tupi.mouseSoltou(botao)    -- true só no frame em que soltou

-- botao: Tupi.MOUSE_ESQ (0), Tupi.MOUSE_DIR (1), Tupi.MOUSE_MEIO (2)
```

```lua
if Tupi.mouseClicou(Tupi.MOUSE_ESQ) then
    print("Clicou com o botão esquerdo!")
end
```

### Scroll (roda do mouse)

```lua
local sx = Tupi.scrollX()   -- scroll horizontal
local sy = Tupi.scrollY()   -- scroll vertical (positivo = rolar para baixo)
```

---

## Campo de Texto

Permite que o jogador digite texto na tela.

### `Tupi.teclado(x, y, prefixo, limite, cor, fonte)`

Retorna o texto digitado **a cada frame** (atualização contínua).

```lua
local nome = Tupi.teclado(50, 100, "Nome: ", 20)
-- Mostra "Nome: " seguido do campo e retorna o texto atual
```

### `Tupi.input(x, y, prefixo, limite, cor, fonte)`

Retorna `nil` enquanto o jogador digita, e o texto completo ao pressionar **Enter**.

```lua
local resultado = Tupi.input(50, 100, "> ", 30)
if resultado then
    print("Jogador digitou: " .. resultado)
end
```

| Parâmetro | Descrição |
|-----------|-----------|
| `x`, `y` | Posição do campo na tela |
| `prefixo` | Texto fixo antes do campo (ex: `"Nome: "`) |
| `limite` | Número máximo de caracteres (`0` = sem limite) |
| `cor` | Cor do texto |
| `fonte` | Fonte a usar |

> 💡 Múltiplos campos de input na mesma tela são navegáveis com **Tab**.

---

## Colisão

Funções para detectar se objetos ou formas estão se tocando.

### Retângulo com Retângulo

```lua
if Tupi.colidiu(hitboxA, hitboxB) then
    -- colisão detectada!
end

local info = Tupi.colisaoInfo(hitboxA, hitboxB)
-- Retorna detalhes sobre a colisão (profundidade, direção, etc.)
```

### Círculo com Círculo

```lua
if Tupi.cirColidiu(cirA, cirB) then ... end
local info = Tupi.cirColisaoInfo(cirA, cirB)
```

### Retângulo com Círculo

```lua
if Tupi.retCirculo(ret, circ) then ... end
```

### Ponto dentro de forma

```lua
if Tupi.pontoRet(px, py, retangulo) then ... end
if Tupi.pontoCir(px, py, circulo)   then ... end
```

### Mouse dentro de forma

```lua
if Tupi.mouseNoRet(retangulo) then
    -- cursor do mouse está sobre o retângulo
end

if Tupi.mouseNoCir(circulo) then ... end
```

---

## Física

Sistema simples de física com gravidade, impulso e colisão sólida.

### Criando corpos físicos

```lua
local corpo = Tupi.corpo(x, y, massa, elasticidade, atrito)
-- Corpo dinâmico (afetado por gravidade e forças)

local corpoFixo = Tupi.corpoEstatico(x, y)
-- Corpo estático (não se move, mas colide com outros)
```

### Atualizando a física

```lua
Tupi.atualizarCorpo(corpo, gravidade)
-- Aplica gravidade e atualiza velocidade/posição
-- gravidade padrão varia; use um número como 500 para teste
```

### Aplicando forças

```lua
Tupi.impulso(corpo, forceX, forceY)
-- Aplica um impulso instantâneo (ex: pular: Tupi.impulso(corpo, 0, -400))

Tupi.atritoCorpo(corpo)
-- Aplica atrito para desacelerar o movimento

Tupi.limitarVel(corpo, velocidadeMaxima)
-- Evita que o corpo ultrapasse uma velocidade máxima
```

### Lendo/definindo estado

```lua
local x, y = Tupi.posCorpo(corpo)       -- posição atual
local vx, vy = Tupi.velCorpo(corpo)     -- velocidade atual

Tupi.setPosCorpo(corpo, x, y)           -- teleporta
Tupi.setVelCorpo(corpo, vx, vy)         -- define velocidade diretamente
```

### Hitboxes para corpos físicos

```lua
local hbRet = Tupi.retColCorpo(corpo, largura, altura)
local hbCir = Tupi.cirColCorpo(corpo, raio)
```

### Resolvendo colisão

```lua
local info = Tupi.colisaoInfo(hbA, hbB)
Tupi.resolverColisao(corpoA, corpoB, info)
-- Empurra os corpos para longe um do outro

Tupi.resolverEstatico(corpo, info)
-- Empurra o corpo para longe de uma superfície estática
```

### Sincronizando sprite com corpo

```lua
Tupi.sincronizar(objeto, corpo)
-- Atualiza a posição do objeto sprite para a posição do corpo físico
```

---

## Mapas de Tiles

Permite criar mundos usando grades de tiles (imagens repetidas).

### Fluxo de uso básico

```lua
-- 1. Criar o mapa
local m = Tupi.mapc("tileset.png", 16, 16, 20, 15)
--           imagem,   larg tile, alt tile, colunas, linhas

-- 2. (Opcional) Definir flags de colisão ANTES de carregar os dados
Tupi.mflag(m, 1, {solido=true})    -- tile 1 é sólido
Tupi.mflag(m, 2, {trigger=true})   -- tile 2 é trigger (não bloqueia, mas detecta)

-- 3. Carregar os dados do mapa (tile 0 = vazio)
Tupi.mapa(m, {
    0, 0, 0, 0, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 0, 0,
})

-- No loop:
function _rodar()
    Tupi.mapu(m)       -- atualiza animações dos tiles
end

function _desenhar()
    Tupi.mapd(m)       -- desenha o mapa
end
```

### Referência de funções

| Função | O que faz |
|--------|-----------|
| `Tupi.mapc(img, tw, th, cols, lins)` | Cria o mapa |
| `Tupi.mapa(m, dados)` | Define a grade (table ou string) |
| `Tupi.mflag(m, id, opts)` | Define flags de tile (`solido`, `trigger`, `passagem`) |
| `Tupi.mframes(m, id, frames, fps, loop)` | Anima um tile |
| `Tupi.mapd(m, z)` | Desenha o mapa |
| `Tupi.mapu(m, dt)` | Atualiza animações |
| `Tupi.mget(m, col, lin)` | Retorna o ID do tile na grade |
| `Tupi.mset(m, col, lin, id)` | Muda um tile em runtime |
| `Tupi.msolido(m, px, py)` | `true` se o tile nessa posição em px é sólido |
| `Tupi.mtrigger(m, px, py)` | `true` se o tile nessa posição em px é trigger |
| `Tupi.mcel(m, col, lin)` | Info completa de uma célula |
| `Tupi.mdestruir(m)` | Libera o mapa da memória |

> 💡 **Aliases em inglês:** `map_create`, `map_data`, `map_flag`, `map_draw`, `map_update`, `map_solid`, `map_trigger`, `map_cell`, `map_get`, `map_set`

---

## Mundos (Cenas)

Mundos são cenas ou fases do jogo. Permitem carregar e descarregar partes do jogo sob demanda.

```lua
Tupi.trocarMundo(arquivo, nome)       -- vai para outro mundo
Tupi.mundoAtual()                     -- retorna o mundo ativo
Tupi.infoMundoAtual()                 -- retorna informações do mundo ativo
Tupi.eMundoAtual(arquivo, nome)       -- true se este é o mundo ativo
Tupi.precarregarMundo(arquivo, nome)  -- carrega em background sem ativar
Tupi.descarregarMundo(arquivo, nome)  -- libera da memória
Tupi.aoSairMundo(funcao)              -- callback ao sair do mundo
Tupi.aoEntrarMundo(funcao)            -- callback ao entrar no mundo
Tupi.destruirTodosMundos()            -- limpa tudo
```

---

## Fade (Transição de tela)

Cria efeitos de fade in/out entre cenas.

```lua
local fade = Tupi.criarFade(largura, altura, duracao)
-- largura, altura: geralmente as dimensões da janela
-- duracao: tempo em segundos para o fade
```

---

## Matemática

### Funções úteis

```lua
Tupi.lerp(a, b, t)
-- Interpolação linear entre a e b pelo fator t (0.0 a 1.0)
-- Ex: Tupi.lerp(0, 100, 0.5) → 50

Tupi.aleatorio(min, max)
-- Número aleatório entre min e max
-- Tupi.aleatorio()        → float entre 0 e 1
-- Tupi.aleatorio(10)      → float entre 0 e 10
-- Tupi.aleatorio(1, 6)    → int entre 1 e 6 (como um dado)

Tupi.radianos(graus)    -- converte graus para radianos
Tupi.graus(radianos)    -- converte radianos para graus

Tupi.distancia(x1, y1, x2, y2)   -- distância entre dois pontos
```

### Atalhos matemáticos

| Nome | Equivalente | O que faz |
|------|-------------|-----------|
| `Tupi.flr(n)` | `math.floor(n)` | Arredonda para baixo |
| `Tupi.ceil(n)` | `math.ceil(n)` | Arredonda para cima |
| `Tupi.abs(n)` | `math.abs(n)` | Valor absoluto |
| `Tupi.raiz(n)` | `math.sqrt(n)` | Raiz quadrada |
| `Tupi.sen(n)` | `math.sin(n)` | Seno |
| `Tupi.cos(n)` | `math.cos(n)` | Cosseno |
| `Tupi.tan(n)` | `math.tan(n)` | Tangente |
| `Tupi.max(a, b)` | `math.max(a, b)` | Maior valor |
| `Tupi.min(a, b)` | `math.min(a, b)` | Menor valor |
| `Tupi.pi` | `math.pi` | Pi (≈ 3.14159…) |
| `Tupi.rnd(n)` | — | Float aleatório de 0 a n |
| `Tupi.mid(a, b, c)` | — | Limita b entre a e c (clamp) |

```lua
-- Exemplo: manter o jogador dentro da tela
jogador.x = Tupi.mid(0, jogador.x, Tupi.largura() - 16)
```

---

## Aliases rápidos

Após chamar `Tupi.rodar()`, todas as funções ficam disponíveis no escopo global com nomes curtos:

| Global | Equivalente Tupi |
|--------|-----------------|
| `largura()` | `Tupi.largura()` |
| `altura()` | `Tupi.altura()` |
| `dt()` | `Tupi.dt()` |
| `cor(r,g,b,a)` | `Tupi.cor(r,g,b,a)` |
| `escrever(...)` | `Tupi.escrever(...)` |
| `print(...)` | `Tupi.escrever(...)` |
| `ret(...)` | `Tupi.retangulo(...)` |
| `circ(...)` | `Tupi.circulo(...)` |
| `lin(...)` | `Tupi.linha(...)` |
| `img(p)` | `Tupi.imagem(p)` |
| `obj(s,x,y,o)` | `Tupi.objeto(s,x,y,o)` |
| `ver(o)` | `Tupi.mostrar(o)` |
| `mover(o,dx,dy)` | `Tupi.mover(o,dx,dy)` |
| `btn(b)` | `Tupi.botao(b)` |
| `btnp(b)` | `Tupi.pressionou(b)` |
| `btnr(b)` | `Tupi.soltou(b)` |
| `mx()` | `Tupi.mouseX()` |
| `my()` | `Tupi.mouseY()` |
| `mclk(b)` | `Tupi.mouseClicou(b)` |
| `colidiu(a,b)` | `Tupi.colidiu(a,b)` |
| `mapc(...)` | `Tupi.mapc(...)` |
| `mapa(...)` | `Tupi.mapa(...)` |
| `mapd(m)` | `Tupi.mapd(m)` |
| `rnd(n)` | `Tupi.rnd(n)` |
| `flr(n)` | `math.floor(n)` |
| `BRANCO`, `VERMELHO`, etc. | Cores predefinidas |
| `PALETA`, `pal` | Paleta PICO-8 |

---

## Exemplo completo — Jogo mínimo

```lua
local Tupi = require("sintaxe")

Tupi.janela(320, 240, "Meu Primeiro Jogo", 2)

local spr, jogador, x, y

function _iniciar()
    spr     = Tupi.imagem("personagem.png")
    x, y    = 150, 100
    jogador = Tupi.objeto(spr, x, y, {larg=16, alt=16})
    Tupi.corFundo(0.05, 0.05, 0.15)
end

function _rodar()
    local vel = 60 * Tupi.dt()   -- pixels por segundo

    if Tupi.botao("esquerda") then x = x - vel end
    if Tupi.botao("direita")  then x = x + vel end
    if Tupi.botao("cima")     then y = y - vel end
    if Tupi.botao("baixo")    then y = y + vel end

    -- manter dentro da tela
    x = Tupi.mid(0, x, Tupi.largura() - 16)
    y = Tupi.mid(0, y, Tupi.altura()  - 16)

    Tupi.posicionar(jogador, x, y)
end

function _desenhar()
    Tupi.limparTela()
    Tupi.mostrar(jogador)
    Tupi.escrever("Use as setas!", 5, 5, 10, 1.0, 1.0, Tupi.AMARELO)
end

Tupi.rodar()
```

---

*Documentação gerada para a engine Tupi — sintaxe.lua*