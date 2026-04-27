# Tupi-8

Motor de jogos 8-bit de código aberto, construído sobre C, Rust e LuaJIT. O projeto serve como base de desenvolvimento para engines mais completas, fornecendo uma janela de resolução fixa (160x144), splash screen, console interativo e editor de código embutidos.

---

## Linguagens e proporção

| Linguagem  | Proporção |
|------------|-----------|
| C          | 64,3 %    |
| Lua        | 27,3 %    |
| Rust       | 5,9 %     |
| Makefile   | 2,5 %     |

---

## Estrutura de diretórios

```
Tupi-8/
├── include/                         # Headers C compartilhados entre os módulos
├── src/
│   ├── Engine/
│   │   ├── TupiEngine.lua           # Módulo central da engine
│   │   ├── tupi_splashscreen.lua    # Tela de abertura animada
│   │   ├── tupi_console.lua         # Console Lua interativo
│   │   ├── tupi_editor.lua          # Editor de código embutido
│   │   ├── sintaxe.lua              # Runtime com realce de sintaxe
│   │   └── texto_normalizar.lua     # Conversão UTF-8 para Latin-1 (fonte bitmap)
│   ├── Renderizador/
│   │   ├── RendererGL.c             # Backend de renderização OpenGL 3.3
│   │   └── RendererDX11.c           # Backend de renderização DirectX 11
│   ├── Camera/
│   │   └── Camera.c                 # Câmera 2D com transformações de viewport
│   ├── Colisores/
│   │   ├── Fisica.c                 # Física básica (gravidade, velocidade)
│   │   └── ColisoesAABB.c           # Detecção de colisão por AABB
│   ├── Sprites/
│   │   ├── Sprites.c                # Renderização de sprites via OpenGL
│   │   └── Spritesdx11.c            # Renderização de sprites via DirectX 11
│   ├── Mapas/
│   │   └── Mapas.c                  # Sistema de mapas de tiles
│   ├── Inputs/
│   │   ├── Inputs.c                 # Entrada de teclado e mouse (OpenGL/GLFW)
│   │   └── Inputsdx11.c             # Entrada de teclado e mouse (DirectX 11)
│   ├── glad.c                       # Loader de extensões OpenGL (GLAD)
│   └── main_bytecode_loader.c       # Loader de bytecode LuaJIT para build standalone
├── ascii.png                        # Fonte bitmap usada na splash screen e na UI
├── editor.png                       # Spritesheet da interface do editor
├── main.lua                         # Ponto de entrada do programa
├── Cargo.toml                       # Manifesto da crate Rust (tupi_seguro v0.3.0)
└── Makefile                         # Sistema de build com menu interativo
```

---

## Documentação dos arquivos

### `main.lua`

Ponto de entrada principal da aplicação. Carrega e inicializa todos os módulos da engine, exibe a splash screen e mantém o loop principal de eventos. Gerencia a alternância entre o console interativo e o editor de código, e delega a execução ao runtime de sintaxe quando um script está ativo.

Variáveis globais expostas ao console durante a execução:

| Global    | Descrição                                                      |
|-----------|----------------------------------------------------------------|
| `Tupi`    | Referência ao módulo principal da engine                       |
| `editor`  | Referência ao editor (ex.: `editor:abrirArquivo("game.lua")`) |
| `runtime` | Referência ao runtime de execução de scripts                   |

---

### `src/Engine/TupiEngine.lua`

Módulo central que encapsula a janela, o loop de tempo, entrada de teclas, renderização de texto e controle de tela cheia. Todos os outros módulos dependem deste como base de operação.

---

### `src/Engine/tupi_splashscreen.lua`

Exibe a imagem `ascii.png` durante a inicialização por um tempo configurável (padrão: 2 segundos). Suporta interrupção antecipada via entrada do usuário.

---

### `src/Engine/tupi_console.lua`

Console Lua interativo que aceita comandos em tempo de execução. Permite inspecionar e manipular o estado da engine sem reiniciar o programa. Emite sinais de saída via `deveSair()` e possui estado de ativação independente do editor.

---

### `src/Engine/tupi_editor.lua`

Editor de código com suporte a abertura de arquivos `.lua`. Integrado ao runtime de sintaxe para execução direta do código editado. Possui prompt interno e alternância de foco com o console via sinal de estado.

---

### `src/Engine/sintaxe.lua`

Runtime de execução de scripts com realce de sintaxe visual. Executa código Lua no contexto da engine e devolve sinais de retorno (`"editor"` ou `"console"`) para controle do fluxo no `main.lua`.

---

### `src/Engine/texto_normalizar.lua`

Aplica patch à função `Tupi.texto` para converter caracteres acentuados de UTF-8 para Latin-1, compatibilizando a saída textual com a fonte bitmap utilizada na interface.

---

### `src/Renderizador/RendererGL.c`

Backend de renderização para Linux utilizando OpenGL 3.3. Gerencia o contexto de janela via GLFW, o carregamento de extensões via GLAD e o pipeline de renderização 2D.

---

### `src/Renderizador/RendererDX11.c`

Backend de renderização para Windows utilizando DirectX 11. Compilado via cross-compile com `x86_64-w64-mingw32-gcc`. Oferece as mesmas funcionalidades do backend GL adaptadas para a API Direct3D.

---

### `src/Camera/Camera.c`

Implementa uma câmera 2D com controle de posição e transformações de viewport, permitindo scroll e enquadramento de cenas maiores que a resolução interna da tela.

---

### `src/Colisores/Fisica.c`

Módulo de física básica responsável por gravidade, aceleração e integração de velocidade para entidades do jogo.

---

### `src/Colisores/ColisoesAABB.c`

Detecção de colisão por Axis-Aligned Bounding Box (AABB). Fornece testes de interseção entre retângulos alinhados aos eixos, adequados para jogos 2D baseados em tiles.

---

### `src/Sprites/Sprites.c` e `Spritesdx11.c`

Renderização de sprites a partir de spritesheets. Suportam recorte por coordenadas UV, espelhamento e posicionamento em coordenadas de mundo. As variantes GL e DX11 compartilham a mesma interface lógica com implementações de backend distintas.

---

### `src/Mapas/Mapas.c`

Sistema de mapas baseado em tiles. Responsável pelo carregamento e renderização de mapas definidos por índices que referenciam um tileset.

---

### `src/Inputs/Inputs.c` e `Inputsdx11.c`

Abstração da entrada de teclado e mouse. Provê consultas de estado por tecla (pressionado, segurado, liberado) e leitura de posição do ponteiro. As variantes GL e DX11 são intercambiáveis na interface.

---

### `src/glad.c`

Loader gerado pelo GLAD para carregar ponteiros de funções OpenGL em tempo de execução. Necessário para uso de OpenGL 3.3 em sistemas sem carregamento automático de extensões.

---

### `src/main_bytecode_loader.c`

Loader em C que embute e executa bytecode LuaJIT compilado. Utilizado na build standalone (`dist-linux`) para distribuir o programa como um único binário sem dependência de arquivos `.lua` externos.

---

### `include/`

Diretório de headers C compartilhados entre os módulos. Contém declarações de funções e estruturas utilizadas pelos renderizadores, câmera, colisores, sprites, mapas e sistema de input.

---

### `Cargo.toml`

Manifesto da crate Rust `tupi_seguro` (versão 0.3.0). Compilada como `staticlib` e linkada tanto no backend OpenGL quanto no DirectX 11. A dependência `image` é responsável pelo carregamento e decodificação de imagens PNG, JPEG e BMP acessadas via FFI a partir do código C.

```toml
[package]
name    = "tupi_seguro"
version = "0.3.0"
edition = "2021"

[lib]
crate-type = ["staticlib"]

[dependencies]
image = { version = "0.25", features = ["png", "jpeg", "bmp"] }

[profile.release]
opt-level = 3
lto       = true
panic     = "abort"
```

---

### `Makefile`

Sistema de build com menu interativo colorido no terminal. Gerencia compilação incremental de objetos C por backend (separados em `.build/gl`, `.build/dx11` e `.build/dist`), compilação da crate Rust via Cargo e linkagem final das bibliotecas.

---

### `ascii.png`

Fonte bitmap em formato de spritesheet. Utilizada pelo console, pelo editor e pela splash screen para renderizar texto em estilo 8-bit. A conversão de acentos realizada por `texto_normalizar.lua` é necessária para compatibilidade com o mapeamento de caracteres desta fonte.

---

### `editor.png`

Spritesheet de interface do editor de código embutido. Contém os elementos visuais utilizados pelo módulo `tupi_editor.lua`.

---

## Dependências externas

### Linux — backend OpenGL

| Biblioteca      | Finalidade                              |
|-----------------|-----------------------------------------|
| GLFW            | Criação de janela e contexto OpenGL     |
| OpenGL / Mesa   | API de renderização                     |
| libX11          | Servidor de janelas X11                 |
| libXrandr       | Detecção de monitores e resolução       |
| libXi           | Entrada de dispositivos estendida       |
| libXinerama     | Suporte a múltiplos monitores           |
| libXcursor      | Cursor de mouse personalizado           |
| LuaJIT          | Interpretador e compilador JIT para Lua |
| GLAD (embutido) | Loader de extensões OpenGL              |

### Windows — backend DirectX 11 (cross-compile)

| Biblioteca        | Finalidade                          |
|-------------------|-------------------------------------|
| d3d11             | API Direct3D 11                     |
| dxgi              | Interface de gráficos DirectX       |
| d3dcompiler       | Compilador de shaders HLSL          |
| mingw-w64-gcc     | Compilador C para Windows via Linux |
| LuaJIT (win64)    | `lib/win64/libluajit-5.1.a`         |

---

## Compilação

### Dependências — Linux (Arch)

```bash
sudo pacman -S --needed \
  glfw-x11 libx11 libxrandr libxi libxinerama libxcursor \
  mesa luajit base-devel rust mingw-w64-gcc
```

Ou via Makefile (opção 4 no menu interativo):

```bash
make
```

### Dependências — Windows (cross-compile)

```bash
sudo pacman -S --needed mingw-w64-gcc mingw-w64-binutils rust
rustup target add x86_64-pc-windows-gnu
# Copiar libluajit-5.1.a para lib/win64/
```

### Comandos de build

| Comando           | Resultado                                           |
|-------------------|-----------------------------------------------------|
| `make`            | Abre o menu interativo                              |
| `make gl`         | Compila `libtupi.so` (OpenGL, Linux)                |
| `make dx11`       | Compila `libtupi_dx11.dll` (DirectX 11, Windows)    |
| `make rodar`      | Compila `libtupi.so` e executa com LuaJIT           |
| `make dist-linux` | Gera o binário standalone `tupi_engine`             |
| `make limpar`     | Remove `.build/`, artefatos e executa `cargo clean` |
| `make ajuda`      | Lista todos os comandos disponíveis                 |

---

## Fluxo de execução

```
Inicialização
  Janela 160x144 @ escala 4x
  Patch de acentos (UTF-8 para Latin-1)

Splash Screen
  Exibe ascii.png por 2 segundos
  Ctrl+1  → alterna tela cheia (letterbox)

Loop Principal
  Ctrl+1  → alterna tela cheia (disponível em qualquer estado)
  Ctrl+5  → alterna entre Console e Editor

  [Console ativo]
    Aceita comandos Lua em tempo real
    ESC → encerra o programa

  [Editor ativo]
    Edita run.lua ou outro arquivo aberto
    ESC → fecha prompt interno ou volta ao console

  [Runtime ativo]
    Executa o script carregado no editor
    Retorno "editor"  → volta ao editor
    Retorno "console" → volta ao console
```

---

## Atalhos de teclado

| Tecla          | Contexto         | Ação                                       |
|----------------|------------------|--------------------------------------------|
| `Ctrl + 1`     | Qualquer         | Alternar modo tela cheia (letterbox)       |
| `Ctrl + 5`     | Console / Editor | Alternar entre Console e Editor            |
| `ESC`          | Console          | Encerrar o programa                        |
| `ESC`          | Editor           | Fechar prompt interno / voltar ao console  |

---

## Referência de funções da engine

As funções abaixo são expostas pelo módulo `TupiEngine.lua` e acessíveis globalmente via `Tupi` no console e nos scripts de jogo.

### Janela e ciclo de vida

| Função                                | Retorno   | Descrição                                                   |
|---------------------------------------|-----------|-------------------------------------------------------------|
| `Tupi.janela(w, h, titulo, escala)`   | —         | Cria a janela com resolução interna e fator de escala       |
| `Tupi.rodando()`                      | `boolean` | `true` enquanto a janela estiver aberta                     |
| `Tupi.fechar()`                       | —         | Fecha a janela e encerra o programa                         |
| `Tupi.atualizar()`                    | —         | Troca os buffers e processa eventos da fila de entrada      |
| `Tupi.limpar()`                       | —         | Limpa o framebuffer para o próximo frame                    |
| `Tupi.dt()`                           | `number`  | Delta time do frame atual em segundos                       |

### Tela cheia e viewport

| Função                            | Retorno   | Descrição                                              |
|-----------------------------------|-----------|--------------------------------------------------------|
| `Tupi.telaCheia_letterbox(bool)`  | —         | Ativa ou desativa o modo tela cheia com letterbox      |
| `Tupi.letterboxAtivo()`           | `boolean` | `true` se o modo letterbox estiver ativo               |

### Entrada de teclado

| Função                          | Retorno   | Descrição                                              |
|---------------------------------|-----------|--------------------------------------------------------|
| `Tupi.teclaPressionou(tecla)`   | `boolean` | `true` se a tecla foi pressionada neste frame          |
| `Tupi.teclaSegurando(tecla)`    | `boolean` | `true` enquanto a tecla estiver mantida pressionada    |
| `Tupi.teclaLiberou(tecla)`      | `boolean` | `true` se a tecla foi liberada neste frame             |

### Constantes de tecla

| Constante             | Tecla correspondente  |
|-----------------------|-----------------------|
| `Tupi.TECLA_ESC`      | Escape                |
| `Tupi.TECLA_1`        | Número 1              |
| `Tupi.TECLA_5`        | Número 5              |
| `Tupi.TECLA_CTRL_ESQ` | Control esquerdo      |
| `Tupi.TECLA_CTRL_DIR` | Control direito       |

### Texto e fonte bitmap

| Função                           | Retorno | Descrição                                                  |
|----------------------------------|---------|------------------------------------------------------------|
| `Tupi.texto(str, x, y, cor)`     | —       | Renderiza uma string na posição (x, y) com a cor indicada  |

### Módulo `tupi_splashscreen`

| Função / Método                        | Retorno   | Descrição                                          |
|----------------------------------------|-----------|----------------------------------------------------|
| `Splash.novo(Tupi, imagem, duracao)`   | instância | Cria uma instância da splash screen                |
| `splash:terminou()`                    | `boolean` | `true` quando o tempo de exibição expirou          |
| `splash:atualizar()`                   | —         | Avança o timer interno da splash                   |
| `splash:desenhar()`                    | —         | Renderiza a imagem de splash no frame atual        |

### Módulo `tupi_console`

| Função / Método                        | Retorno   | Descrição                                          |
|----------------------------------------|-----------|----------------------------------------------------|
| `Console.novo(Tupi, font, runtime)`    | instância | Cria uma instância do console interativo           |
| `con:ativar(bool)`                     | —         | Ativa ou desativa o console                        |
| `con:atualizar(dt)`                    | —         | Processa entrada e lógica interna do console       |
| `con:desenhar()`                       | —         | Renderiza o console no frame atual                 |
| `con:print(str, cor)`                  | —         | Exibe uma mensagem de texto no console             |
| `con:deveSair()`                       | `boolean` | `true` se o comando de encerramento foi emitido    |

### Módulo `tupi_editor`

| Função / Método                                    | Retorno   | Descrição                                        |
|----------------------------------------------------|-----------|--------------------------------------------------|
| `Editor.novo(Tupi, font, editorImg, runtime)`      | instância | Cria uma instância do editor de código           |
| `ed:ativar(bool)`                                  | —         | Ativa ou desativa o editor                       |
| `ed:estaAtivo()`                                   | `boolean` | `true` se o editor estiver ativo                 |
| `ed:atualizar(dt)`                                 | —         | Processa entrada e lógica interna do editor      |
| `ed:desenhar()`                                    | —         | Renderiza o editor no frame atual                |
| `ed:abrirArquivo(caminho)`                         | —         | Abre um arquivo `.lua` para edição               |

### Módulo `sintaxe` (runtime)

| Função / Método               | Retorno              | Descrição                                                      |
|-------------------------------|----------------------|----------------------------------------------------------------|
| `Sintaxe.novo(Tupi, font)`    | instância            | Cria uma instância do runtime de execução                      |
| `runtime:estaAtivo()`         | `boolean`            | `true` se um script estiver em execução                        |
| `runtime:atualizar()`         | —                    | Avança a execução do script atual                              |
| `runtime:desenhar()`          | —                    | Renderiza a saída visual do script                             |
| `runtime:consumirRetorno()`   | `string` ou `nil`    | Retorna `"editor"`, `"console"` ou `nil` ao término            |

---

## Build standalone (`dist-linux`)

O alvo `dist-linux` detecta automaticamente a presença de bibliotecas estáticas (`libglfw3.a` e `libX11.a`) em `/usr/lib` e `/usr/local/lib`. Quando encontradas, GLFW e X11 são linkadas estaticamente para reduzir dependências em tempo de execução. Caso contrário, a linkagem é realizada de forma dinâmica com aviso no terminal.

O bytecode LuaJIT é embutido no binário pelo módulo `main_bytecode_loader.c`, eliminando a necessidade de arquivos `.lua` externos na distribuição final.

---

## Observações

- A resolução interna fixa de 160x144 pixels remete ao Game Boy original. A escala padrão de 4x resulta em uma janela de 640x576 pixels.
- O modo letterbox preserva a proporção da tela interna ao redimensionar a janela, adicionando barras pretas nas bordas quando necessário.
- O console expõe o ambiente Lua completo em tempo de execução, permitindo inspecionar e modificar o estado da engine durante o desenvolvimento sem reiniciar o programa.
- Não há licença declarada no repositório. Contate o autor antes de utilizar o código em projetos derivados.
