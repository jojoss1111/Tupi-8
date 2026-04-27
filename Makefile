# Makefile — TupiEngine (C + Rust + LuaJIT)
#
#   make               Menu interativo
#   make gl            Compila libtupi.so  (OpenGL 3.3, Linux)
#   make dx11          Compila libtupi_dx11.dll (DX11, cross-compile)
#   make rodar         Compila GL e executa com LuaJIT
#   make limpar        Remove objetos e artefatos
#   make ajuda         Lista todos os alvos

# ══════════════════════════════════════════════════════════════
#  Ferramentas
# ══════════════════════════════════════════════════════════════

CC      = gcc
CC_DX11 = x86_64-w64-mingw32-gcc
LUAJIT  = luajit
CARGO   = cargo
AR      = ar

# ══════════════════════════════════════════════════════════════
#  Cores do terminal
# ══════════════════════════════════════════════════════════════

RED    = \033[0;31m
GREEN  = \033[0;32m
YELLOW = \033[0;33m
CYAN   = \033[0;36m
BOLD   = \033[1m
DIM    = \033[2m
RESET  = \033[0m

# ══════════════════════════════════════════════════════════════
#  Diretórios
# ══════════════════════════════════════════════════════════════

SRC_DIR       = src/Renderizador
CAMERA_DIR    = src/Camera
RUST_DIR      = .
RUST_LIB_PATH = ./target/release

# Diretório onde os .o de cada backend ficam isolados
OBJ_DIR_GL   = .build/gl
OBJ_DIR_DX11 = .build/dx11
OBJ_DIR_DIST = .build/dist

# ══════════════════════════════════════════════════════════════
#  Backend: OpenGL — libtupi.so
# ══════════════════════════════════════════════════════════════

GL_LIB = libtupi.so

GL_SRCS = $(SRC_DIR)/RendererGL.c      \
          $(CAMERA_DIR)/Camera.c        \
          src/Colisores/Fisica.c        \
          src/Inputs/Inputs.c           \
          src/Colisores/ColisoesAABB.c  \
          src/Sprites/Sprites.c         \
          src/Mapas/Mapas.c             \
          src/glad.c

# Gera lista de .o espelhando os .c dentro de OBJ_DIR_GL
GL_OBJS = $(patsubst %.c,$(OBJ_DIR_GL)/%.o,$(GL_SRCS))

GL_CFLAGS = -O2 -Wall -Wextra -fPIC  \
            -I$(SRC_DIR)              \
            -I$(CAMERA_DIR)           \
            -Iinclude                 \
            -Isrc                     \
            -Isrc/Mapas               \
            -Isrc/Sprites             \
            -Isrc/Colisores           \
            -Isrc/Inputs

GL_LIBS = -L$(RUST_LIB_PATH) -ltupi_seguro \
          -lglfw -lGL -lX11 -lm -ldl -lpthread

# ══════════════════════════════════════════════════════════════
#  Backend: DX11 — libtupi_dx11.dll
# ══════════════════════════════════════════════════════════════

DX11_LIB = libtupi_dx11.dll

DX11_SRCS = $(SRC_DIR)/RendererDX11.c      \
            $(CAMERA_DIR)/Camera.c           \
            src/Colisores/Fisica.c           \
            src/Inputs/Inputsdx11.c          \
            src/Colisores/ColisoesAABB.c     \
            src/Sprites/Spritesdx11.c        \
            src/Mapas/Mapas.c

DX11_OBJS = $(patsubst %.c,$(OBJ_DIR_DX11)/%.o,$(DX11_SRCS))

DX11_CFLAGS = -O2 -Wall -Wextra  \
              -I$(SRC_DIR)        \
              -I$(CAMERA_DIR)     \
              -Iinclude           \
              -Isrc               \
              -Isrc/Mapas         \
              -Isrc/Sprites       \
              -Isrc/Colisores     \
              -Isrc/Inputs        \
              -DTUPI_BACKEND_DX11

DX11_LIBS = -L$(RUST_LIB_PATH) -ltupi_seguro \
            -ld3d11 -ldxgi -ld3dcompiler -lm  \
            -static-libgcc

# ══════════════════════════════════════════════════════════════
#  Standalone Linux — tupi_engine  (dist-linux)
# ══════════════════════════════════════════════════════════════

LOADER_SRC     = main_bytecode_loader.c
DIST_BIN_LINUX = tupi_engine

DIST_SRCS = $(LOADER_SRC) $(GL_SRCS)
DIST_OBJS = $(patsubst %.c,$(OBJ_DIR_DIST)/%.o,$(DIST_SRCS))

DIST_CFLAGS = -O2 -Wall                 \
              -I/usr/include/luajit-2.1  \
              -I$(SRC_DIR)               \
              -I$(CAMERA_DIR)            \
              -Iinclude                  \
              -Isrc                      \
              -Isrc/Mapas                \
              -Isrc/Sprites              \
              -Isrc/Colisores            \
              -Isrc/Inputs

# Detecta libs estáticas uma vez na avaliação do Makefile
_HAVE_STATIC_GLFW := $(shell find /usr/lib /usr/local/lib 2>/dev/null \
                              -name "libglfw3.a" | head -1)
_HAVE_STATIC_X11  := $(shell find /usr/lib /usr/local/lib 2>/dev/null \
                              -name "libX11.a"   | head -1)

ifeq ($(and $(_HAVE_STATIC_GLFW),$(_HAVE_STATIC_X11)),)
  $(info [TupiEngine] Libs estaticas nao encontradas — linkagem dinamica.)
  DIST_LIBS = -L$(RUST_LIB_PATH)        \
              -Wl,--whole-archive        \
              -lluajit-5.1               \
              -Wl,--no-whole-archive      \
              -ltupi_seguro              \
              -lglfw -lGL -lX11 -lXrandr \
              -lXi -lXinerama -lXcursor  \
              -lm -ldl -lpthread         \
              -Wl,-E
else
  $(info [TupiEngine] Libs estaticas encontradas — linkando GLFW e X11 estaticamente.)
  DIST_LIBS = -L$(RUST_LIB_PATH)                       \
              -Wl,--whole-archive                       \
              -lluajit-5.1                              \
              -Wl,--no-whole-archive                    \
              -ltupi_seguro                             \
              -Wl,-Bstatic -lglfw3 -lX11 -lXrandr -lXi \
              -lXinerama -lXcursor                      \
              -Wl,-Bdynamic                             \
              -lGL -lm -ldl -lpthread                   \
              -static-libgcc                            \
              -Wl,-E
endif

# ══════════════════════════════════════════════════════════════
#  Phony
# ══════════════════════════════════════════════════════════════

.PHONY: all menu gl dx11 dist-linux rodar compilar_rust limpar \
        instalar-deps-linux instalar-deps-windows ajuda \
        _build_gl _build_dx11 _clean _deps_linux _deps_windows

# ══════════════════════════════════════════════════════════════
#  Menu interativo
# ══════════════════════════════════════════════════════════════

all: menu

menu:
	@clear
	@printf "$(CYAN)$(BOLD)"
	@printf "╔══════════════════════════════════════════════════════╗\n"
	@printf "║             TupiEngine — Build System                ║\n"
	@printf "╚══════════════════════════════════════════════════════╝\n"
	@printf "$(RESET)\n"
	@printf "$(BOLD)  Selecione uma opcao:$(RESET)\n\n"
	@printf "  $(GREEN)$(BOLD)[1]$(RESET) Compilar OpenGL     $(DIM)(Linux — libtupi.so)$(RESET)\n"
	@printf "  $(CYAN)$(BOLD)[2]$(RESET) Compilar Windows    $(DIM)(cross-compile DX11)$(RESET)\n"
	@printf "  $(RED)$(BOLD)[3]$(RESET) Limpar artefatos\n"
	@printf "  $(YELLOW)$(BOLD)[4]$(RESET) Dependencias Linux\n"
	@printf "  $(YELLOW)$(BOLD)[5]$(RESET) Dependencias Windows\n"
	@printf "\n$(DIM)  ────────────────────────────────────────────────────$(RESET)\n\n"
	@printf "  > Digite o numero e pressione Enter: " && \
	read OPCAO; \
	case $$OPCAO in \
		1) $(MAKE) _build_gl    ;; \
		2) $(MAKE) _build_dx11  ;; \
		3) $(MAKE) _clean       ;; \
		4) $(MAKE) _deps_linux  ;; \
		5) $(MAKE) _deps_windows;; \
		*) printf "$(RED)Opcao invalida.$(RESET)\n" ;; \
	esac

_build_gl:     gl
_build_dx11:   dx11
_clean:        limpar
_deps_linux:   instalar-deps-linux
_deps_windows: instalar-deps-windows

# ══════════════════════════════════════════════════════════════
#  OpenGL — compilação incremental por objeto
# ══════════════════════════════════════════════════════════════

gl: compilar_rust $(GL_LIB)
	@printf "$(GREEN)$(BOLD)+  Pronto: $(GL_LIB)$(RESET)\n\n"

# Linka os objetos GL num .so
$(GL_LIB): $(GL_OBJS)
	@printf "$(CYAN)>  Linkando $(BOLD)$(GL_LIB)$(RESET)$(CYAN)...$(RESET)\n"
	@$(CC) -shared -fPIC -o $@ $^ $(GL_LIBS) \
		&& printf "$(GREEN)+  Link OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no link GL.$(RESET)\n\n"; exit 1; }

# Compila cada .c → .o (GL)
# O diretório do .o é criado automaticamente antes de compilar.
$(OBJ_DIR_GL)/%.o: %.c
	@mkdir -p $(dir $@)
	@printf "$(DIM)   CC  $<$(RESET)\n"
	@$(CC) $(GL_CFLAGS) -c $< -o $@ \
		|| { printf "$(RED)x  Erro em $<$(RESET)\n"; exit 1; }

# ══════════════════════════════════════════════════════════════
#  DX11 — compilação incremental por objeto
# ══════════════════════════════════════════════════════════════

dx11: compilar_rust $(DX11_LIB)
	@printf "$(GREEN)$(BOLD)+  Pronto: $(DX11_LIB)$(RESET)\n\n"

$(DX11_LIB): $(DX11_OBJS)
	@printf "$(CYAN)>  Linkando $(BOLD)$(DX11_LIB)$(RESET)$(CYAN)...$(RESET)\n"
	@$(CC_DX11) -shared -o $@ $^ $(DX11_LIBS) \
		&& printf "$(GREEN)+  Link OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no link DX11.$(RESET)\n\n"; exit 1; }

$(OBJ_DIR_DX11)/%.o: %.c
	@mkdir -p $(dir $@)
	@printf "$(DIM)   CC  $<$(RESET)\n"
	@$(CC_DX11) $(DX11_CFLAGS) -c $< -o $@ \
		|| { printf "$(RED)x  Erro em $<$(RESET)\n"; exit 1; }

# ══════════════════════════════════════════════════════════════
#  Standalone Linux — dist-linux
# ══════════════════════════════════════════════════════════════

dist-linux: compilar_rust $(DIST_OBJS)
	@printf "\n$(CYAN)$(BOLD)  Linkando binario standalone...$(RESET)\n"
	@$(CC) -o $(DIST_BIN_LINUX) $(DIST_OBJS) $(DIST_LIBS) \
		&& printf "$(GREEN)+  Pronto: $(BOLD)$(DIST_BIN_LINUX)$(RESET)\n\n" \
		|| { printf "$(RED)x  Falha no link standalone.$(RESET)\n\n"; exit 1; }

$(OBJ_DIR_DIST)/%.o: %.c
	@mkdir -p $(dir $@)
	@printf "$(DIM)   CC  $<$(RESET)\n"
	@$(CC) $(DIST_CFLAGS) -c $< -o $@ \
		|| { printf "$(RED)x  Erro em $<$(RESET)\n"; exit 1; }

# ══════════════════════════════════════════════════════════════
#  Rust
# ══════════════════════════════════════════════════════════════

compilar_rust:
	@printf "$(CYAN)>  Compilando Rust...$(RESET)\n"
	@cd $(RUST_DIR) && $(CARGO) build --release \
		&& printf "$(GREEN)+  Rust OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no build Rust.$(RESET)\n"; exit 1; }

# ══════════════════════════════════════════════════════════════
#  Rodar
# ══════════════════════════════════════════════════════════════

rodar: gl
	@printf "$(GREEN)>  Iniciando LuaJIT...$(RESET)\n\n"
	@DISPLAY=:0 GDK_BACKEND=x11 $(LUAJIT) main.lua

# ══════════════════════════════════════════════════════════════
#  Limpeza
# ══════════════════════════════════════════════════════════════

limpar:
	@printf "\n$(RED)  Limpando artefatos...$(RESET)\n"
	@rm -rf .build $(GL_LIB) $(DX11_LIB) $(DIST_BIN_LINUX)
	@cd $(RUST_DIR) && $(CARGO) clean
	@printf "$(GREEN)+  Tudo limpo.$(RESET)\n\n"

# ══════════════════════════════════════════════════════════════
#  Dependências
# ══════════════════════════════════════════════════════════════

instalar-deps-linux:
	@printf "\n$(CYAN)$(BOLD)  Dependencias — Linux (Arch)$(RESET)\n\n"
	@printf "  glfw-x11  libx11  libxrandr  libxi  libxinerama  libxcursor\n"
	@printf "  mesa  luajit  base-devel  rust  mingw-w64-gcc\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		sudo pacman -S --needed \
			glfw-x11 libx11 libxrandr libxi libxinerama libxcursor \
			mesa luajit base-devel rust mingw-w64-gcc \
			&& printf "$(GREEN)+  Instalado.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi

instalar-deps-windows:
	@printf "\n$(CYAN)$(BOLD)  Dependencias — Windows (cross-compile)$(RESET)\n\n"
	@printf "  mingw-w64-gcc  mingw-w64-binutils  rust\n"
	@printf "  $(DIM)rustup target add x86_64-pc-windows-gnu$(RESET)\n\n"
	@printf "  LuaJIT Windows -> lib/win64/libluajit-5.1.a\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		sudo pacman -S --needed mingw-w64-gcc mingw-w64-binutils rust \
			&& rustup target add x86_64-pc-windows-gnu \
			&& printf "$(GREEN)+  Pronto.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi

# ══════════════════════════════════════════════════════════════
#  Ajuda
# ══════════════════════════════════════════════════════════════

ajuda:
	@printf "\n$(CYAN)$(BOLD)  TupiEngine — comandos disponíveis:$(RESET)\n\n"
	@printf "  $(GREEN)make$(RESET)                  Menu interativo\n"
	@printf "  $(GREEN)make gl$(RESET)               Compila $(GL_LIB)\n"
	@printf "  $(CYAN)make dx11$(RESET)             Compila $(DX11_LIB)\n"
	@printf "  $(GREEN)make rodar$(RESET)            Compila GL e executa\n"
	@printf "  $(YELLOW)make dist-linux$(RESET)       Binario standalone Linux\n"
	@printf "  $(RED)make limpar$(RESET)           Remove .build/ e artefatos\n\n"