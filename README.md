# Tupi 8

Tupi 8 é uma engine/game fantasy console brasileira inspirada no espírito do **PICO-8**: pequena, direta, educativa e divertida de explorar.  
O projeto foi pensado para **jovens programadores**, estudantes e desenvolvedores interessados em entender como uma engine pode ser construída misturando desempenho, segurança e flexibilidade.

## Proposta

A ideia do Tupi 8 é oferecer uma base enxuta para jogos e experimentos visuais em estilo retrô, com:

- `C` para as partes mais próximas do hardware e com foco em performance.
- `Rust` para segurança de memória, validações e suporte a estruturas internas.
- `Lua` para scripts, gameplay, editor, console e iteração rápida.

## Inspiração

O Tupi 8 se inspira no **PICO-8** no sentido de ser uma ferramenta acessível, criativa e voltada ao aprendizado.  
Ao mesmo tempo, ele segue identidade própria: é um projeto **feito no Brasil**, em português, e pensado para servir como ponte entre curiosidade, estudo e desenvolvimento real de engine.

## O que é preciso para rodar

### Linux

Você vai precisar de:

- `gcc`
- `make`
- `rust` e `cargo`
- `luajit`
- `SDL2`
- `Vulkan loader / driver com suporte a Vulkan`
- `glslc` ou toolchain `shaderc` para compilar os shaders SPIR-V

Em sistemas Arch Linux, o `Makefile` já sugere:

- `sdl2`
- `vulkan-icd-loader`
- `shaderc`
- `luajit`
- `base-devel`
- `rust`

## Como compilar e rodar

### Compilar a biblioteca principal

```bash
make sdl2
```

### Rodar o projeto

```bash
luajit main.lua
```

### Compilar binário standalone Linux

```bash
make dist-linux
```

### Compilar para Windows

```bash
make win
```

Observações:

- O backend gráfico atual usa **Vulkan**.
- O SDL2 continua sendo usado para **janela**, **eventos** e **integração com `VkSurfaceKHR`**.
- Para rodar corretamente, a máquina precisa ter **suporte a Vulkan** disponível.

## Estrutura do projeto

### Arquivos da raiz

- `Makefile`: compila a engine, os shaders, a parte Rust e os alvos Linux/Windows.
- `Cargo.toml`: configuração da crate Rust `tupi_seguro`.
- `Cargo.lock`: trava versões das dependências Rust.
- `main.lua`: ponto de entrada da aplicação; abre a janela, splash, console e editor.
- `run.lua`: script de teste/execução carregado pelo editor ou runtime.
- `sintaxe.lua`: arquivo Lua auxiliar ligado à parte de sintaxe/execução.
- `libtupi.so`: biblioteca compartilhada gerada no build Linux.
- `ascii.png`: fonte/atlas visual usado pela interface e splash.
- `editor.png`: recurso visual do editor.
- `logo.png`: imagem de identidade do projeto.
- `tileset.png`: recurso gráfico usado em testes e mapas.

### Pasta `src/`

Ela reúne os módulos em C, Rust e Lua.

### Núcleo C da engine

- `src/Renderizador/Renderer.h`: API pública do renderer, janela, cores, formas 2D e acesso interno.
- `src/Renderizador/Renderer.c`: backend principal de renderização com SDL2 + Vulkan, swapchain, pipeline, batch 2D e gerenciamento de frame.
- `src/Renderizador/shaders/tupi2d.vert`: shader de vértice da renderização 2D.
- `src/Renderizador/shaders/tupi2d.frag`: shader de fragmento da renderização 2D.
- `src/Renderizador/shaders/*.spv`: versões compiladas dos shaders em SPIR-V usadas pelo Vulkan.
- `src/Sprites/Sprites.h`: interface de sprites, atlas e batch de objetos.
- `src/Sprites/Sprites.c`: upload de texturas, desenho de sprites e integração do batch com o renderer.
- `src/Camera/Camera.h`: estrutura e API da câmera.
- `src/Camera/Camera.c`: movimentação da câmera, projeção e conversão entre mundo e tela.
- `src/Inputs/Inputs.h`: interface do sistema de input.
- `src/Inputs/Inputs.c`: captura de teclado, mouse, scroll e estado dos botões.
- `src/Colisores/ColisoesAABB.h`: API de colisões geométricas.
- `src/Colisores/ColisoesAABB.c`: testes de colisão entre retângulos, círculos e pontos.
- `src/Colisores/Fisica.h`: interface do módulo de física.
- `src/Colisores/Fisica.c`: integração física básica e resolução de movimento.
- `src/Mapas/Mapas.h`: interface do sistema de mapas.
- `src/Mapas/Mapas.c`: lógica de mapas e suporte ao mundo renderizado.

### Camada Rust

- `src/lib.rs`: registra e expõe os módulos Rust da engine.
- `src/renderizador.rs`: carregamento seguro de assets, batcher e operações matemáticas do renderer.
- `src/sprites.rs`: estruturas auxiliares relacionadas a sprites e batch em Rust.
- `src/camera.rs`: validações e suporte da câmera no lado Rust.
- `src/colisores.rs`: lógica segura de colisão.
- `src/fisica.rs`: suporte Rust para física.
- `src/mapas.rs`: estruturas e helpers de mapas.

O papel do Rust aqui é reforçar a confiabilidade da engine, reduzir riscos de memória e concentrar partes críticas com uma camada mais segura.

### Camada Lua

- `src/Engine/TupiEngine.lua`: módulo principal consumido pelos scripts Lua.
- `src/Engine/engineffi.lua`: ponte FFI entre LuaJIT e a biblioteca C.
- `src/Engine/engine_core.lua`: funções centrais da API exposta ao usuário.
- `src/Engine/engine_visual.lua`: recursos visuais usados pelo runtime.
- `src/Engine/engine_mundo.lua`: utilidades ligadas ao mundo, cena e comportamento.
- `src/Engine/tupi_splashscreen.lua`: splash screen de abertura.
- `src/Engine/tupi_console.lua`: console embutido da engine.
- `src/Engine/tupi_editor.lua`: editor interno para scripts e testes.
- `src/Engine/tupi_teclado.lua`: suporte e mapeamentos de teclado.
- `src/Engine/sintaxe.lua`: execução e organização do runtime de scripts.
- `src/Engine/texto_normalizar.lua`: normalização de texto para a fonte bitmap.

O Lua é a camada de maior agilidade: onde ficam os scripts, ferramentas internas, comportamento do editor e a lógica mais fácil de iterar.

## Resumo da arquitetura

O fluxo geral do Tupi 8 funciona assim:

1. `Lua` controla a experiência de uso, editor, console e scripts.
2. `engineffi.lua` conversa com a biblioteca nativa.
3. `C` cuida da janela, input, renderização e integração com Vulkan.
4. `Rust` oferece estruturas seguras, utilidades internas e suporte a partes críticas.

## Para quem é esta engine

O Tupi 8 foi feito para:

- jovens programadores que querem aprender como uma engine funciona por dentro;
- estudantes que desejam estudar `C`, `Rust` e `Lua` no mesmo projeto;
- desenvolvedores interessados em renderização, runtime, ferramentas e arquitetura de engine;
- pessoas que gostam da filosofia de consoles fantasy como o PICO-8, mas querem explorar uma base própria e aberta.

## Identidade do projeto

O Tupi 8 é uma **engine brasileira**, criada com foco em aprendizado, experimentação e formação técnica.  
Ele busca ser ao mesmo tempo um ambiente criativo e um projeto de estudo sério para quem quer crescer na área de desenvolvimento de jogos, engines e programação de baixo nível.
