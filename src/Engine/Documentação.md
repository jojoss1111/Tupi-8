# Documentação da Sintaxe — Tupi-8

Esta versão foi reorganizada para ficar mais útil no meio do código: menos texto solto, mais função prática, exemplos curtos e uma referência completa da API pública atual da `sintaxe.lua`.

## Como usar

```lua
local Tupi = require("sintaxe")
```

A ideia é simples: você configura a janela, escreve `'_iniciar'`, `'_rodar'` e `'_desenhar'`, e chama `Tupi.rodar()`.

```lua
function _iniciar()
end

function _rodar()
end

function _desenhar()
end

Tupi.rodar()
```

## 1) Janela e loop principal

Use estas funções para criar, controlar e fechar a janela do jogo.

### Funções

| Função | O que faz |
|---|---|
| `Tupi.janela(largura, altura, titulo, escala, semBorda, imagem)` | Cria a janela |
| `Tupi.rodando()` | Retorna `true` enquanto o jogo estiver aberto |
| `Tupi.limparTela()` | Limpa a tela |
| `Tupi.atualizar()` | Atualiza a janela no fim do frame |
| `Tupi.fechar()` | Fecha a janela |
| `Tupi.tempo()` | Tempo total desde o início |
| `Tupi.dt()` | Delta time do frame atual |
| `Tupi.largura()` / `Tupi.altura()` | Tamanho lógico da janela |
| `Tupi.larguraPx()` / `Tupi.alturaPx()` | Tamanho real em pixels |
| `Tupi.escalaJanela()` | Escala atual da janela |
| `Tupi.titulo(t)` | Troca o título da janela |
| `Tupi.decoracao(a)` | Liga/desliga bordas da janela |
| `Tupi.telaCheia(a, lb)` | Liga/desliga fullscreen |
| `Tupi.letterboxAtivo()` | Informa se o letterbox está ativo |
| `Tupi.fpsLimite(n)` | Define o limite de FPS |
| `Tupi.fpsAtual()` | Retorna o FPS atual |

Exemplo:

```lua
Tupi.janela(160, 144, "Tupi-8", 4)
Tupi.fpsLimite(60)
```

## 2) Cores e desenho de formas

### Cor base

```lua
Tupi.cor(r, g, b, a)
Tupi.usarCor({r, g, b, a})
Tupi.corFundo(r, g, b)
Tupi.cls(cor)
```

`Tupi.cls()` aceita:
- número da paleta;
- tabela `{r, g, b}`;
- valores separados `r, g, b`.

### Cores prontas

- `Tupi.BRANCO`
- `Tupi.PRETO`
- `Tupi.VERMELHO`
- `Tupi.VERDE`
- `Tupi.AZUL`
- `Tupi.AMARELO`
- `Tupi.ROXO`
- `Tupi.LARANJA`
- `Tupi.CIANO`
- `Tupi.CINZA`
- `Tupi.ROSA`

### Paleta PICO-8

`Tupi.PALETA[0]` até `Tupi.PALETA[15]`

### Formas

| Função | O que faz |
|---|---|
| `Tupi.retangulo(x, y, l, a, cor)` | Retângulo preenchido |
| `Tupi.bordaRet(x, y, l, a, esp, cor)` | Retângulo com borda |
| `Tupi.triangulo(x1, y1, x2, y2, x3, y3, cor)` | Triângulo preenchido |
| `Tupi.circulo(x, y, raio, seg, cor)` | Círculo preenchido |
| `Tupi.bordaCirc(x, y, raio, seg, esp, cor)` | Círculo com borda |
| `Tupi.linha(x1, y1, x2, y2, esp, cor)` | Linha |
| `Tupi.pixel(x, y, cor)` | Um pixel |
| `Tupi.flush()` | Envia o batch de desenho |

Exemplo:

```lua
Tupi.cor(1, 0, 0, 1)
Tupi.retangulo(10, 10, 32, 16)
Tupi.bordaCirc(80, 80, 12, 24, 2, Tupi.BRANCO)
```

## 3) Texto

### Fonte

| Função | O que faz |
|---|---|
| `Tupi.carregarFonte(caminho, larg, alt, colunas, charInicio)` | Carrega fonte bitmap |
| `Tupi.destruirFonte(fonte)` | Libera a fonte |
| `Tupi.setFontePadrao(fonte)` | Define fonte padrão |
| `Tupi.getFontePadrao()` | Retorna fonte padrão |
| `Tupi.setCorTexto(r, g, b, a)` | Define cor do texto |

### Desenho de texto

| Função | O que faz |
|---|---|
| `Tupi.escrever(texto, x, y, z, escala, transp, cor, fonte)` | Escreve texto |
| `Tupi.escreverSombra(texto, x, y, z, dX, dY, escala, transp, escS, transpS, fonte)` | Texto com sombra |
| `Tupi.escreverCaixa(texto, x, y, z, larg, alt, escala, transp, fonte, frame, tamTile, escB, transpB, recuo)` | Texto em caixa |
| `Tupi.larguraTexto(fonte, texto, escala)` | Largura do texto |
| `Tupi.alturaTexto(fonte, texto, escala)` | Altura do texto |
| `Tupi.dimensoesTexto(fonte, texto, escala)` | Largura e altura |

Exemplo:

```lua
local fonte = Tupi.carregarFonte("fonte.png", 8, 8)
Tupi.setFontePadrao(fonte)
Tupi.escrever("Olá, mundo!", 8, 8, 10, 1, 1, Tupi.BRANCO)
```

## 4) Sprites e objetos

### Imagens

| Função | O que faz |
|---|---|
| `Tupi.imagem(caminho)` | Carrega uma imagem |
| `Tupi.destruirImagem(spr)` | Libera a imagem |

### Objetos

| Função | O que faz |
|---|---|
| `Tupi.objeto(sprite, x, y, opt)` | Cria objeto a partir de sprite |
| `Tupi.mostrar(wrapper, z)` | Envia objeto para desenho |
| `Tupi.desenharSprite(sprite, x, y, opt)` | Desenha sprite direto |

`opt` aceita:
- `larg` / `largura`
- `alt` / `altura`
- `z`
- `col` / `coluna`
- `lin` / `linha`
- `alfa` / `transparencia`
- `escala`

### Movimento e estado

| Função | O que faz |
|---|---|
| `Tupi.mover(obj, dx, dy)` | Move relativo |
| `Tupi.posicionar(obj, x, y)` | Teleporta |
| `Tupi.posicao(obj)` | Retorna posição atual |
| `Tupi.salvarPosicao(obj)` | Salva posição |
| `Tupi.ultimaPosicao(obj)` | Retorna última posição salva |
| `Tupi.voltarPosicao(obj)` | Volta à última posição |
| `Tupi.distanciaObjetos(a, b)` | Distância entre objetos |
| `Tupi.moverParaAlvo(obj, tx, ty, f)` | Move em direção ao alvo |
| `Tupi.perseguir(obj, alvo, vel)` | Persegue outro objeto |
| `Tupi.moverComColisao(obj, dx, dy, ox, oy, w, h, hbB)` | Move com colisão |
| `Tupi.escalaObj(obj, s)` | Altera escala |
| `Tupi.alfa(obj, a)` | Altera transparência |
| `Tupi.tamanho(obj, l, a)` | Altera tamanho |
| `Tupi.quadro(obj, col, lin)` | Troca quadro |
| `Tupi.espelhar(obj, h, v)` | Espelha objeto |
| `Tupi.getEspelho(obj)` | Retorna estado do espelho |
| `Tupi.setCor(r, g, b, a)` | Define cor do sprite |
| `Tupi.resetCor()` | Limpa a cor aplicada |
| `Tupi.destruir(obj, liberarSpr)` | Destrói objeto |
| `Tupi.destruido(obj)` | Verifica se foi destruído |

### Hitbox

| Função | O que faz |
|---|---|
| `Tupi.hitbox(obj, x, y, larg, alt, escalar)` | Hitbox relativa |
| `Tupi.hitboxFixa(obj, x, y, larg, alt)` | Hitbox fixa |
| `Tupi.hitboxDesenhar(hb, cor, esp)` | Desenha hitbox |
| `Tupi.resolverColisaoSolida(hbA, hbB, obj)` | Resolve colisão sólida |

Exemplo:

```lua
local spr = Tupi.imagem("player.png")
local jogador = Tupi.objeto(spr, 32, 32, {larg = 16, alt = 16})
Tupi.mostrar(jogador)
```

## 5) Animações

| Função | O que faz |
|---|---|
| `Tupi.criarAnim(sprite, larg, alt, colunas, linhas, fps, loop)` | Cria animação |
| `Tupi.tocarAnim(anim, obj, z)` | Toca animação |
| `Tupi.pararAnim(anim, obj, frame, z)` | Para em um frame |
| `Tupi.animTerminou(anim, obj)` | Verifica fim da animação |
| `Tupi.animReiniciar(anim, obj)` | Reinicia animação |
| `Tupi.animLimpar(obj)` | Remove animação do objeto |

Exemplo:

```lua
local anim = Tupi.criarAnim(spr, 16, 16, {0,1,2,3}, {0,0,0,0}, 10, true)
Tupi.tocarAnim(anim, jogador)
```

## 6) Câmera

| Função | O que faz |
|---|---|
| `Tupi.criarCamera(ax, ay, anc_x, anc_y)` | Cria câmera |
| `Tupi.destruirCamera(cam)` | Destrói câmera |
| `Tupi.ativarCamera(cam)` | Ativa câmera |
| `Tupi.cameraPosicao(cam, x, y)` | Define posição |
| `Tupi.cameraMover(cam, dx, dy)` | Move câmera |
| `Tupi.cameraZoom(cam, z)` | Define zoom |
| `Tupi.cameraRotacao(cam, a)` | Define rotação |
| `Tupi.cameraAncora(cam, ax, ay)` | Define âncora |
| `Tupi.cameraSeguir(cam, x, y, vel)` | Faz a câmera seguir |
| `Tupi.cameraPosAtual(cam)` | Retorna posição atual |
| `Tupi.cameraAlvo(cam)` | Retorna alvo atual |
| `Tupi.cameraZoomAtual(cam)` | Retorna zoom atual |
| `Tupi.cameraRotacaoAtual(cam)` | Retorna rotação atual |
| `Tupi.cameraTelaMundo(cam, sx, sy)` | Tela → mundo |
| `Tupi.cameraMundoTela(cam, wx, wy)` | Mundo → tela |
| `Tupi.cameraMouseMundo(cam)` | Mouse no mundo |

## 7) Paralaxe

| Função | O que faz |
|---|---|
| `Tupi.registrarParalax(fx, fy, z, ll, al)` | Registra camada |
| `Tupi.removerParalax(id)` | Remove camada |
| `Tupi.resetarParalax()` | Remove todas |
| `Tupi.resetarCamadaParalax(id)` | Reseta uma camada |
| `Tupi.setFatorParalax(id, fx, fy)` | Ajusta fator |
| `Tupi.totalParalax()` | Total de camadas |
| `Tupi.atualizarParalax(cam, cx, cy)` | Atualiza camadas |
| `Tupi.offsetParalax(id)` | Retorna offset |
| `Tupi.desenharParalax(id, wrapper)` | Desenha camada |
| `Tupi.desenharParalaxTile(id, wrapper, lt)` | Desenha em tiles |

## 8) Input — Teclado

### Estado das teclas

| Função | O que faz |
|---|---|
| `Tupi.botao(b)` | Segurando |
| `Tupi.pressionou(b)` | Pressionou neste frame |
| `Tupi.soltou(b)` | Soltou neste frame |
| `Tupi.tecla(code)` | Consulta direta por código |

### Formas de usar

```lua
Tupi.botao("espaco")
Tupi.pressionou("ctrl+s")
Tupi.botao(0) -- estilo PICO-8
Tupi.botao(Tupi.TECLA_ENTER)
```

### Configurações do teclado

| Função | O que faz |
|---|---|
| `Tupi.setTempoSegurando(s)` | Tempo para repetição |
| `Tupi.getTempoSegurando()` | Retorna tempo |
| `Tupi.setLayout(id)` | Layout do teclado |
| `Tupi.getLayout()` | Retorna layout |
| `Tupi.setRepeat(a, p)` | Configura repetição |
| `Tupi.getRepeat()` | Retorna repetição |

## 9) Input — Mouse

| Função | O que faz |
|---|---|
| `Tupi.mouseX()` | X do mouse |
| `Tupi.mouseY()` | Y do mouse |
| `Tupi.mouseDX()` | Delta X |
| `Tupi.mouseDY()` | Delta Y |
| `Tupi.mousePos()` | X e Y juntos |
| `Tupi.mouseXRaw()` | X bruto |
| `Tupi.mouseYRaw()` | Y bruto |
| `Tupi.mouseClicou(b)` | Clique neste frame |
| `Tupi.mouseBotao(b)` | Botão segurado |
| `Tupi.mouseSoltou(b)` | Botão soltou |
| `Tupi.scrollX()` | Scroll horizontal |
| `Tupi.scrollY()` | Scroll vertical |

Constantes:
- `Tupi.MOUSE_ESQ`
- `Tupi.MOUSE_DIR`
- `Tupi.MOUSE_MEIO`

## 10) Campo de texto

### `Tupi.teclado(x, y, prefixo, limite, cor, fonte)`

Retorna o texto atual enquanto você digita.

### `Tupi.input(x, y, prefixo, limite, cor, fonte)`

Retorna `nil` enquanto digita e retorna a string ao apertar Enter.

Exemplo:

```lua
local nome = Tupi.teclado(8, 8, "Nome: ", 20)
local valor = Tupi.input(8, 24, "> ", 30)
```

## 11) Colisão

| Função | O que faz |
|---|---|
| `Tupi.colidiu(a, b)` | Retângulo com retângulo |
| `Tupi.colisaoInfo(a, b)` | Detalhes da colisão |
| `Tupi.cirColidiu(a, b)` | Círculo com círculo |
| `Tupi.cirColisaoInfo(a, b)` | Detalhes da colisão circular |
| `Tupi.retCirculo(r, c)` | Retângulo com círculo |
| `Tupi.pontoRet(px, py, r)` | Ponto dentro do retângulo |
| `Tupi.pontoCir(px, py, c)` | Ponto dentro do círculo |
| `Tupi.mouseNoRet(r)` | Mouse dentro do retângulo |
| `Tupi.mouseNoCir(c)` | Mouse dentro do círculo |

## 12) Física

| Função | O que faz |
|---|---|
| `Tupi.corpo(x, y, massa, elastic, atrito)` | Cria corpo dinâmico |
| `Tupi.corpoEstatico(x, y)` | Cria corpo estático |
| `Tupi.atualizarCorpo(corpo, grav)` | Atualiza física |
| `Tupi.impulso(corpo, fx, fy)` | Aplica impulso |
| `Tupi.atritoCorpo(corpo)` | Aplica atrito |
| `Tupi.limitarVel(corpo, max)` | Limita velocidade |
| `Tupi.posCorpo(corpo)` | Posição do corpo |
| `Tupi.velCorpo(corpo)` | Velocidade do corpo |
| `Tupi.setPosCorpo(corpo, x, y)` | Define posição |
| `Tupi.setVelCorpo(corpo, vx, vy)` | Define velocidade |
| `Tupi.retColCorpo(corpo, larg, alt)` | Hitbox retangular |
| `Tupi.cirColCorpo(corpo, raio)` | Hitbox circular |
| `Tupi.resolverColisao(corpoA, corpoB, info)` | Resolve colisão |
| `Tupi.resolverEstatico(corpo, info)` | Resolve colisão com estático |
| `Tupi.sincronizar(objeto, corpo)` | Sincroniza sprite e corpo |

Exemplo:

```lua
local corpo = Tupi.corpo(32, 32, 1, 0.1, 0.05)
Tupi.impulso(corpo, 0, -200)
Tupi.atualizarCorpo(corpo, 500)
```

## 13) Mapas

### Criar e carregar

| Função | O que faz |
|---|---|
| `Tupi.mapc(png, tw, th, cols, lins)` | Cria mapa com atlas |
| `Tupi.mapa(m, dados)` | Carrega o grid |
| `Tupi.mflag(m, tile_id, opts)` | Define flags do tile |
| `Tupi.mframes(m, tile_id, frames, fps, loop)` | Anima tile |
| `Tupi.mapd(m, z)` | Desenha mapa |
| `Tupi.mapu(m, dt)` | Atualiza animações |
| `Tupi.mget(m, col, lin)` | Lê tile por grade |
| `Tupi.mset(m, col, lin, tid)` | Escreve tile por grade |
| `Tupi.msolido(m, px, py)` | Tile sólido no ponto |
| `Tupi.mtrigger(m, px, py)` | Tile trigger no ponto |
| `Tupi.mcel(m, c, l)` | Dados completos da célula |
| `Tupi.mhitbox(m, c, l)` | Hitbox do tile |
| `Tupi.mdef(m, c, l)` | Definição da grade |
| `Tupi.mdefPonto(m, px, py)` | Definição por ponto |
| `Tupi.mtileEmPonto(m, px, py)` | ID do tile no ponto |
| `Tupi.mdestruir(m)` | Destrói mapa |

### Exemplo básico

```lua
local mapa = Tupi.mapc("tiles.png", 16, 16, 20, 15)
Tupi.mapa(mapa, dados)
Tupi.mapd(mapa, 0)
```

### Aliases do mapa

| Alias | Função real |
|---|---|
| `Tupi.map_create` | `Tupi.mapc` |
| `Tupi.map_data` | `Tupi.mapa` |
| `Tupi.map_flag` | `Tupi.mflag` |
| `Tupi.map_draw` | `Tupi.mapd` |
| `Tupi.map_update` | `Tupi.mapu` |
| `Tupi.map_solid` | `Tupi.msolido` |
| `Tupi.map_trigger` | `Tupi.mtrigger` |
| `Tupi.map_cell` | `Tupi.mcel` |
| `Tupi.map_get` | `Tupi.mget` |
| `Tupi.map_set` | `Tupi.mset` |

## 14) Mundos

| Função | O que faz |
|---|---|
| `Tupi.trocarMundo(nome, args)` | Troca o mundo atual |
| `Tupi.mundoAtual()` | Nome do mundo atual |
| `Tupi.infoMundoAtual()` | Informações do mundo atual |
| `Tupi.eMundoAtual(a, n)` | Verifica mundo atual |
| `Tupi.precarregarMundo(a, n)` | Precarrega mundo |
| `Tupi.descarregarMundo(a, n)` | Descarrega mundo |
| `Tupi.aoSairMundo(fn)` | Callback ao sair |
| `Tupi.aoEntrarMundo(fn)` | Callback ao entrar |
| `Tupi.destruirTodosMundos()` | Limpa todos os mundos |

## 15) Fade

| Função | O que faz |
|---|---|
| `Tupi.criarFade(larg, alt, duracao)` | Cria fade |
| `Tupi.lerp(a, b, t)` | Interpolação linear |

## 16) Matemática

| Função | O que faz |
|---|---|
| `Tupi.aleatorio(min, max)` | Número aleatório |
| `Tupi.radianos(g)` | Graus para radianos |
| `Tupi.graus(r)` | Radianos para graus |
| `Tupi.distancia(x1, y1, x2, y2)` | Distância entre pontos |
| `Tupi.flr` / `Tupi.chao` | `math.floor` |
| `Tupi.ceil` / `Tupi.teto` | `math.ceil` |
| `Tupi.abs` | `math.abs` |
| `Tupi.raiz` / `Tupi.sqrt` | `math.sqrt` |
| `Tupi.sen` / `Tupi.sin` | `math.sin` |
| `Tupi.cos` | `math.cos` |
| `Tupi.tan` | `math.tan` |
| `Tupi.pi` | `math.pi` |
| `Tupi.max` | `math.max` |
| `Tupi.min` | `math.min` |
| `Tupi.mid(a, b, c)` | Valor entre mínimo e máximo |
| `Tupi.rnd(n)` | Aleatório até `n` |
| `Tupi.clamp` | Limite entre valores |

## 17) Aliases rápidos

Esses nomes existem para deixar a sintaxe mais curta.

| Alias | Função |
|---|---|
| `Tupi.apresentar` | `Tupi.atualizar` |
| `Tupi.print` | `Tupi.escrever` |
| `Tupi.image` | `Tupi.imagem` |
| `Tupi.object` | `Tupi.objeto` |
| `Tupi.draw` | `Tupi.mostrar` |
| `Tupi.draw_sprite` | `Tupi.desenharSprite` |
| `Tupi.move` | `Tupi.mover` |
| `Tupi.set_pos` | `Tupi.posicionar` |
| `Tupi.get_pos` | `Tupi.posicao` |
| `Tupi.alpha` | `Tupi.alfa` |
| `Tupi.destroy` | `Tupi.destruir` |
| `Tupi.overlap` | `Tupi.colidiu` |
| `Tupi.new_anim` | `Tupi.criarAnim` |
| `Tupi.anim_done` | `Tupi.animTerminou` |
| `Tupi.time` | `Tupi.tempo` |
| `Tupi.set_fps` | `Tupi.fpsLimite` |
| `Tupi.get_fps` | `Tupi.fpsAtual` |
| `Tupi.rad` | `Tupi.radianos` |
| `Tupi.deg` | `Tupi.graus` |
| `Tupi.dist` | `Tupi.distancia` |
| `Tupi.btn` | `Tupi.botao` |
| `Tupi.btnp` | `Tupi.pressionou` |
| `Tupi.btnr` | `Tupi.soltou` |
| `Tupi.key` | `Tupi.botao` |
| `Tupi.keyp` | `Tupi.pressionou` |
| `Tupi.keyr` | `Tupi.soltou` |
| `Tupi.mc` | `Tupi.mapc` |
| `Tupi.md` | `Tupi.mapd` |
| `Tupi.mu` | `Tupi.mapu` |
| `Tupi.mf` | `Tupi.mflag` |
| `Tupi.ms` | `Tupi.msolido` |
| `Tupi.mt` | `Tupi.mtrigger` |
| `Tupi.set_repeat` | `Tupi.setRepeat` |
| `Tupi.set_layout` | `Tupi.setLayout` |

## 18) Referência curta de uso

```lua
Tupi.janela(160, 144, "Jogo", 4)
Tupi.cls(0)
Tupi.retangulo(10, 10, 20, 20, Tupi.BRANCO)
Tupi.escrever("Teste", 8, 8)
Tupi.botao("espaco")
Tupi.mouseX()
Tupi.mapd(mapa)
Tupi.rodar()
```

## 19) Observação importante

Essa documentação segue a sintaxe atual da `sintaxe.lua`, com foco no que realmente faz sentido chamar durante o jogo. Funções internas e detalhes de implementação ficaram fora de propósito aqui, para manter a documentação limpa e prática.
