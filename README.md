# 🎮 Tupi-8 — TupiEngine 8bit

> Motor de jogos 8-bit inspirado no estilo fantasy console, combinando **C**, **Rust** e **LuaJIT** num ambiente com console interativo e editor de código embutido.

---

## 📋 Sobre o Projeto

O **Tupi-8** é a versão atual e aprimorada da Tupi Engine, projetada como uma base sólida para engines mais avançadas. Ele apresenta uma janela 160×144 (resolução Game Boy), splash screen, console Lua interativo e editor de código integrado — tudo renderizado via uma fonte bitmap ASCII.

---

## 🏗️ Estrutura do Repositório

```
Tupi-8/
├── src/
│   ├── Engine/
│   │   ├── TupiEngine.lua          # Módulo principal da engine
│   │   ├── tupi_splashscreen.lua   # Tela de abertura
│   │   ├── tupi_console.lua        # Console interativo Lua
│   │   ├── tupi_editor.lua         # Editor de código embutido
│   │   ├── sintaxe.lua             # Runtime com highlight de sintaxe
│   │   └── texto_normalizar.lua    # Normalização de acentos UTF-8 → Latin-1
│   ├── Renderizador/
│   │   ├── RendererGL.c            # Backend OpenGL 3.3
│   │   └── RendererDX11.c          # Backend DirectX 11 (cross-compile)
│   ├── Camera/
│   │   └── Camera.c                # Sistema de câmera 2D
│   ├── Colisores/
│   │   ├── Fisica.c                # Física básica
│   │   └── ColisoesAABB.c          # Colisão AABB
│   ├── Sprites/
│   │   ├── Sprites.c               # Renderização de sprites (GL)
│   │   └── Spritesdx11.c           # Renderização de sprites (DX11)
│   ├── Mapas/
│   │   └── Mapas.c                 # Sistema de mapas/tiles
│   ├── Inputs/
│   │   ├── Inputs.c                # Entrada de teclado/mouse (GL)
│   │   └── Inputsdx11.c            # Entrada de teclado/mouse (DX11)
│   ├── glad.c                      # Loader OpenGL (GLAD)
│   └── main_bytecode_loader.c      # Loader para build standalone
├── include/                        # Headers C da engine
├── ascii.png                       # Fonte bitmap (tela de splash e UI)
├── editor.png                      # Spritesheet do editor
├── main.lua                        # Ponto de entrada principal (LuaJIT)
├── Cargo.toml                      # Manifesto da crate Rust (tupi_seguro)
└── Makefile                        # Build system completo
```

---

## 🧩 Tecnologias

| Linguagem / Ferramenta | Uso |
|---|---|
| **C (GCC)** | Renderizador, câmera, física, sprites, mapas, inputs |
| **Rust** | Biblioteca segura (`tupi_seguro`) para processamento de imagens |
| **LuaJIT** | Scripting principal, console e editor |
| **OpenGL 3.3** | Backend de renderização Linux |
| **DirectX 11** | Backend de renderização Windows (cross-compile) |
| **GLFW** | Criação de janela e contexto OpenGL |
| **GLAD** | Loader de extensões OpenGL |

---

## ⚙️ Dependências da Crate Rust (`Cargo.toml`)

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

A crate compila como `staticlib` e é linkada tanto no backend GL quanto no DX11.

---

## 🚀 Como Compilar

### Pré-requisitos — Linux (Arch)

```bash
sudo pacman -S --needed \
  glfw-x11 libx11 libxrandr libxi libxinerama libxcursor \
  mesa luajit base-devel rust mingw-w64-gcc
```

Ou use o atalho do próprio Makefile:

```bash
make          # abre o menu interativo
# Opção 4 → instala dependências Linux
```

### Pré-requisitos — Windows (cross-compile)

```bash
sudo pacman -S --needed mingw-w64-gcc mingw-w64-binutils rust
rustup target add x86_64-pc-windows-gnu
# Copie libluajit-5.1.a para lib/win64/
```

---

## 🔨 Comandos Make

| Comando | Descrição |
|---|---|
| `make` | Abre o menu interativo |
| `make gl` | Compila `libtupi.so` (OpenGL, Linux) |
| `make dx11` | Compila `libtupi_dx11.dll` (DX11, Windows) |
| `make rodar` | Compila GL e executa com LuaJIT |
| `make dist-linux` | Gera binário standalone `tupi_engine` |
| `make limpar` | Remove `.build/`, artefatos e `cargo clean` |
| `make ajuda` | Lista todos os comandos disponíveis |

### Exemplo rápido

```bash
# Compilar e rodar (Linux)
make rodar
```

---

## 🎮 Fluxo do Programa (`main.lua`)

```
Inicialização
    └─ Janela 160×144 px @ escala 4x
    └─ Normalização de acentos (UTF-8 → Latin-1 para fonte bitmap)

Splash Screen
    └─ Exibe ascii.png por 2 segundos
    └─ Ctrl+1 → alterna tela cheia (letterbox)

Loop Principal
    ├─ Ctrl+1  → tela cheia letterbox (sempre ativo)
    ├─ Ctrl+5  → alterna entre Console ↔ Editor
    ├─ Console → interpreta comandos Lua em tempo real
    │     └─ ESC → fecha o programa
    ├─ Editor  → abre e edita run.lua
    │     └─ ESC → volta ao console
    └─ Runtime → executa o código do editor visualmente
          └─ retorno "editor" ou "console"
```

---

## ⌨️ Atalhos de Teclado

| Tecla | Ação |
|---|---|
| `Ctrl + 1` | Alternar tela cheia (letterbox) |
| `Ctrl + 5` | Alternar Console ↔ Editor |
| `ESC` (no console) | Fechar o programa |
| `ESC` (no editor) | Fechar prompt interno / voltar ao console |

---

## 🖥️ Console Interativo

O console expõe variáveis globais úteis diretamente no ambiente Lua:

```lua
Tupi    -- módulo principal da engine
editor  -- ex: editor:abrirArquivo("game.lua")
runtime -- runtime de execução de scripts
```

Comandos de exemplo no console:
```
run        → roda run.lua (ou o arquivo atual no editor)
Ctrl+5     → abre o editor
Ctrl+1     → tela cheia
```

---

## 🗂️ Backends de Renderização

### OpenGL (Linux) — `libtupi.so`

Fontes compiladas: `RendererGL.c`, `Camera.c`, `Fisica.c`, `Inputs.c`, `ColisoesAABB.c`, `Sprites.c`, `Mapas.c`, `glad.c`

Links: `-lglfw -lGL -lX11 -lm -ldl -lpthread -ltupi_seguro`

### DirectX 11 (Windows) — `libtupi_dx11.dll`

Fontes compiladas: `RendererDX11.c`, `Camera.c`, `Fisica.c`, `Inputsdx11.c`, `ColisoesAABB.c`, `Spritesdx11.c`, `Mapas.c`

Cross-compilado com `x86_64-w64-mingw32-gcc`.  
Links: `-ld3d11 -ldxgi -ld3dcompiler -ltupi_seguro`

---

## 📦 Build Standalone Linux (`dist-linux`)

Gera um binário único `tupi_engine` que embute o LuaJIT (bytecode loader), linkando estaticamente GLFW e X11 quando disponíveis ou usando linkagem dinâmica como fallback.

```bash
make dist-linux
./tupi_engine
```

---

## 📁 Arquivos de Assets

| Arquivo | Descrição |
|---|---|
| `ascii.png` | Fonte bitmap usada na splash screen e na UI do console/editor |
| `editor.png` | Spritesheet de interface do editor de código |

---

## 📌 Informações do Repositório

| Item | Detalhe |
|---|---|
| Autor | [jojoss1111](https://github.com/jojoss1111) |
| Linguagens | C (64.3%), Lua (27.3%), Rust (5.9%), Makefile (2.5%) |
| Versão Rust | `tupi_seguro` v0.3.0 |
| Resolução padrão | 160×144 @ escala 4x |
| Branch principal | `main` |

---
