<div align="center">
  <img src="./logo.png" alt="Logo da Tupi Engine" width="180">

# Tupi-8

```text
████████╗██╗   ██╗██████╗ ██╗     █████╗ 
╚══██╔══╝██║   ██║██╔══██╗██║    ██╔══██╗
   ██║   ██║   ██║██████╔╝██║    ╚█████╔╝
   ██║   ██║   ██║██╔═══╝ ██║    ██╔══██╗
   ██║   ╚██████╔╝██║     ██║    ╚█████╔╝
   ╚═╝    ╚═════╝ ╚═╝     ╚═╝     ╚════╝ 
```

<p>
  <strong>Engine brasileira focada em performance, segurança e prototipação rápida.</strong>
</p>

<p>
  <img src="https://img.shields.io/badge/status-em%20desenvolvimento-22c55e?style=for-the-badge" alt="Status">
  <img src="https://img.shields.io/badge/plataformas-Linux%20%7C%20Windows-0f172a?style=for-the-badge" alt="Plataformas">
  <img src="https://img.shields.io/badge/build-Makefile-f59e0b?style=for-the-badge&logo=gnu" alt="Build">
</p>

<p>
  <img src="https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white" alt="C">
  <img src="https://img.shields.io/badge/Rust-000000?style=for-the-badge&logo=rust&logoColor=white" alt="Rust">
  <img src="https://img.shields.io/badge/LuaJIT-2C2D72?style=for-the-badge&logo=lua&logoColor=white" alt="LuaJIT">
  <img src="https://img.shields.io/badge/SDL2-7A4EAB?style=for-the-badge&logo=sdl&logoColor=white" alt="SDL2">
  <img src="https://img.shields.io/badge/Vulkan-A41E22?style=for-the-badge&logo=vulkan&logoColor=white" alt="Vulkan">
</p>
</div>

---

## Visao geral

**Tupi-8** e uma engine brasileira inspirada em motores compactos, rapidos e faceis de iterar. Ela combina a velocidade do **C + SDL2 + Vulkan**, a confiabilidade de **Rust** e a flexibilidade de scripts com **LuaJIT**.

### O foco da engine

- renderizacao eficiente com SDL2 + Vulkan
- validacao e seguranca na camada Rust
- scripts em LuaJIT para testes, gameplay e ferramentas
- fluxo de build simples com `make`

## Stack da Tupi-8

| Camada | Tecnologia | Funcao |
| --- | --- | --- |
| Core | C | Loop principal, integracao de baixo nivel e runtime |
| Render | SDL2 + Vulkan | Janela, contexto, renderizacao e pipeline grafico |
| Seguranca | Rust | Validacoes, consistencia de dados e suporte seguro |
| Script | LuaJIT | Gameplay, prototipos e iteracao rapida |
| Build | Make + Cargo | Compilacao Linux, Windows e distribuicao |

## O que cada parte faz

### C + SDL2 + Vulkan

O nucleo em C cuida da execucao em tempo real e da integracao com o renderizador. No projeto atual, essa base gera:

- `libtupi.so` para Linux
- `libtupi.dll` para Windows
- `tupi_engine` como binario standalone Linux

### Rust

Rust entra como camada de seguranca e consistencia. Ele ajuda em validacoes, carregamento seguro de imagens, atlas de sprites, batching e checagens contra valores invalidos como `NaN`, `Inf` e limites fora do esperado.

### LuaJIT

LuaJIT acelera a prototipacao. O alvo `make rodar` inicia a engine com `main.lua`, o que deixa testes de logica e gameplay muito mais rapidos.

O alvo `make dist-linux` usa uma tecnica de **sledging**: ele compila um runner standalone, empacota `main.lua` + os modulos de `src/Engine/*.lua` e anexa esse payload ao final do executavel. No startup, o runner abre o proprio binario via `/proc/self/exe`, encontra o footer com o offset/tamanho do payload e executa tudo com `luaL_loadbuffer`.

Para um fluxo mais proximo de exportacao de engine, existe tambem `make export-linux OUTDIR=/caminho/desejado`. Esse alvo gera uma pasta com:

- `bin/tupi_engine`
- `scripts/game.tupack` com todos os scripts Lua em um unico binario
- `assets/` com os PNGs copiados preservando a estrutura relativa
- `lib/` com bibliotecas estaticas do core exportadas

Nesse modo, o runner procura automaticamente `../scripts/game.tupack` e usa `../assets` como raiz de assets, o que deixa a pasta exportada portavel entre distros Linux sem depender de layout fixo do projeto.

## Build rapido

```bash
make
```

O menu principal permite:

- compilar para Linux
- compilar para Windows
- gerar binario standalone Linux
- limpar artefatos
- instalar dependencias Linux
- instalar dependencias Windows

Tambem da para chamar os alvos direto:

```bash
make sdl2
make win
make dist-linux
make rodar
```

## Dependencias

O `Makefile` agora detecta automaticamente diferentes gerenciadores de pacotes no Linux e tambem oferece fluxo para Windows e cross-compile.

### Linux

Suporte atual para:

- `apt`
- `dnf`
- `pacman`
- `zypper`
- `apk`

Para instalar:

```bash
make instalar-deps-linux
```

### Windows

Para instalar a base do ambiente Windows:

```bash
make instalar-deps-win
```

## Estrutura principal

### Fontes C usados no build

- `src/Renderizador/Renderer.c`
- `src/Camera/Camera.c`
- `src/Colisores/Fisica.c`
- `src/Inputs/Inputs.c`
- `src/Colisores/ColisoesAABB.c`
- `src/Sprites/Sprites.c`
- `src/Mapas/Mapas.c`
- `main_bytecode_loader.c`

### Empacotamento standalone

- `src/bin/tupi_pack.rs` cria o payload Lua, tanto para append no executavel quanto para gerar um arquivo externo `.tupack`
- `main_bytecode_loader.c` le o proprio binario ou um pacote Lua externo e instala um searcher Lua para os modulos embutidos
- o modo standalone define `TUPI_STANDALONE = true`, entao `engineffi.lua` usa `ffi.C` em vez de tentar abrir `libtupi.so`

### Modulos Rust atuais

- `src/camera.rs`
- `src/colisores.rs`
- `src/fisica.rs`
- `src/lib.rs`
- `src/mapas.rs`
- `src/renderizador.rs`
- `src/sprites.rs`

## Por que a Tupi-8 e rapida

- batching de draw calls para reduzir custo de render
- validacao antecipada na camada Rust
- shaders embutidos no build
- estrutura simples para compilar e iterar sem atrito

## Por que a Tupi-8 e boa para aprender

- separa bem o papel de cada linguagem
- aproxima o dev de conceitos reais de engine
- permite prototipar rapido sem perder controle tecnico
- mantem uma base pequena e facil de estudar

## Identidade do projeto

Tupi-8 e uma engine brasileira, feita com identidade propria e com foco em desenvolver tecnologia de jogos no nosso idioma, no nosso contexto e no nosso ecossistema.

---

<div align="center">
  <strong>Tupi Engine</strong><br>
  Performance de baixo nivel com uma alma brasileira.
</div>
