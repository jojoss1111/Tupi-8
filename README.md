# Tupi-8

**Tupi-8** é uma engine brasileira inspirada na ideia de engines compactas e rápidas de usar, feita para unir a velocidade do **C + SDL2 + Vulkan**, a segurança de cálculos em **Rust** e a flexibilidade de scripts com **LuaJIT**.

## Visão geral

A proposta do projeto é entregar uma base enxuta, prática e rápida de iterar. O foco está em:
- renderização eficiente com SDL2 + Vulkan;
- validação e segurança na camada Rust;
- scripts em LuaJIT para acelerar testes, gameplay e ferramentas;
- um fluxo de build simples com `make`.

## O que cada linguagem faz

### C + SDL2 + Vulkan
O núcleo em C cuida da integração com o motor gráfico e com a parte de execução em tempo real. No Makefile atual, o projeto compila uma biblioteca Linux (`libtupi.so`), uma DLL para Windows (`libtupi.dll`) e um binário standalone Linux, todos ligados com SDL2 e Vulkan. Isso deixa a engine perto do hardware, com boa performance e controle fino do pipeline.

### Rust
Rust entra como a camada de segurança e consistência. No código atual, os módulos Rust fazem validação de parâmetros, carregamento seguro de imagens, gerenciamento de atlas, batching de draw calls e checagens para evitar valores inválidos como `NaN`, `Inf` e limites fora do esperado. Isso reduz bugs difíceis de rastrear e melhora a estabilidade da engine.

### LuaJIT
LuaJIT é usado para scripting. O `Makefile` atual mostra um alvo de execução que inicia a engine com `main.lua`, o que permite testar lógica, protótipos e gameplay com rapidez sem recompilar a base inteira.

## Como o Makefile funciona

O `Makefile` atual organiza o projeto em alvos claros:

- `menu`: abre um menu interativo de build;
- `sdl2`: compila a versão Linux em `libtupi.so`;
- `win`: compila a versão Windows em `libtupi.dll`;
- `dist-linux`: gera um binário standalone para Linux;
- `rodar`: compila e executa com LuaJIT;
- `limpar`: remove artefatos de build;
- `instalar-deps-linux` e `instalar-deps-win`: exibem/instalam as dependências necessárias.

Ele também:
- compila o Rust com `cargo build --release`;
- embute shaders GLSL em headers antes do link;
- separa objetos de build por plataforma;
- faz link com SDL2, Vulkan, LuaJIT e a biblioteca Rust `tupi_seguro`.

## Estrutura da pasta `src`

### Arquivos C usados pelo build
O Makefile atual compila estes arquivos C da pasta `src`:

- `src/Renderer.c` — núcleo do renderizador;
- `src/Camera/Camera.c` — lógica da câmera;
- `src/Colisores/Fisica.c` — física e suporte a colisões;
- `src/Inputs/Inputs.c` — entrada do usuário;
- `src/Colisores/ColisoesAABB.c` — colisões AABB;
- `src/Sprites/Sprites.c` — sprites e exibição;
- `src/Mapas/Mapas.c` — mapas e tiles;
- `main_bytecode_loader.c` — usado no build `dist-linux`.

### Arquivos Rust em `src`
Os módulos Rust atuais são:

- `camera.rs` — validação segura de câmera 2D;
- `colisores.rs` — suporte Rust para colisões/validações;
- `fisica.rs` — rotinas de física;
- `lib.rs` — ponto de entrada da crate Rust;
- `mapas.rs` — validação de mapas, limites e consistência de tiles;
- `renderizador.rs` — assets, batcher, matemática de render e ordenação por Z;
- `sprites.rs` — carregamento seguro de imagens, atlas de sprites e batching.

## Por que a engine é rápida

A Tupi-8 foi pensada para ser leve e direta:
- batching de draw calls para reduzir custo de render;
- validação antecipada no Rust para evitar trabalho errado na GPU;
- shaders embutidos no build;
- organização simples para compilar e testar sem atrito.

## Por que é fácil de aprender e programar

A base do projeto usa uma divisão bem clara:
- C para o núcleo e integração;
- Rust para segurança e regras de validação;
- LuaJIT para scripts e prototipação.

Isso deixa o projeto bom para aprender arquitetura de engine e, ao mesmo tempo, rápido para produzir coisas úteis sem perder controle técnico.

## Projeto brasileiro

A Tupi-8 é uma engine brasileira, feita com identidade própria e foco em desenvolver tecnologia de jogo no nosso idioma e no nosso ecossistema.

---

### Execução rápida

```bash
make
```

Depois escolha uma opção no menu ou rode o alvo desejado diretamente, como `make sdl2`, `make win`, `make dist-linux` ou `make rodar`.
