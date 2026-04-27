# Makefile — TupiEngine (C + Rust + LuaJIT / SDL2)
#
#   make                Menu interativo
#   make sdl2           Compila libtupi.so        (SDL2, Linux)
#   make win            Compila libtupi.dll        (SDL2, Windows cross-compile)
#   make rodar          Compila Linux e executa com LuaJIT
#   make dist-linux     Binario standalone Linux
#   make limpar         Remove objetos e artefatos
#   make ajuda          Lista todos os alvos

# ══════════════════════════════════════════════════════════════
#  Ferramentas
# ══════════════════════════════════════════════════════════════

CC       = gcc
CC_WIN   = x86_64-w64-mingw32-gcc
LUAJIT   = luajit
CARGO    = cargo

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

OBJ_DIR_SDL2 = .build/sdl2
OBJ_DIR_WIN  = .build/win
OBJ_DIR_DIST = .build/dist

# ══════════════════════════════════════════════════════════════
#  Fontes comuns (mesmo .c compila nos dois alvos)
# ══════════════════════════════════════════════════════════════

SRCS = $(SRC_DIR)/Renderer.c        \
       $(CAMERA_DIR)/Camera.c        \
       src/Colisores/Fisica.c        \
       src/Inputs/Inputs.c           \
       src/Colisores/ColisoesAABB.c  \
       src/Sprites/Sprites.c         \
       src/Mapas/Mapas.c

# ══════════════════════════════════════════════════════════════
#  Flags comuns de include
# ══════════════════════════════════════════════════════════════

COMMON_INCLUDES = -I$(SRC_DIR)    \
                  -I$(CAMERA_DIR) \
                  -Iinclude       \
                  -Isrc           \
                  -Isrc/Mapas     \
                  -Isrc/Sprites   \
                  -Isrc/Colisores \
                  -Isrc/Inputs

# ══════════════════════════════════════════════════════════════
#  Backend: SDL2 Linux — libtupi.so
# ══════════════════════════════════════════════════════════════

SDL2_LIB  = libtupi.so
SDL2_OBJS = $(patsubst %.c,$(OBJ_DIR_SDL2)/%.o,$(SRCS))

SDL2_CFLAGS = -O2 -Wall -Wextra -fPIC \
              $(COMMON_INCLUDES)       \
              $(shell pkg-config --cflags sdl2)

SDL2_LIBS = -L$(RUST_LIB_PATH) -ltupi_seguro \
            $(shell pkg-config --libs sdl2)   \
            -lm -ldl -lpthread

# ══════════════════════════════════════════════════════════════
#  Backend: SDL2 Windows — libtupi.dll  (cross-compile mingw64)
#
#  Requer:
#    - mingw-w64-gcc  (pacman: mingw-w64-gcc)
#    - SDL2 dev para MinGW em ./lib/win64/
#        lib/win64/include/SDL2/   <- headers
#        lib/win64/libSDL2.a       <- lib estatica (ou libSDL2.dll.a)
#        lib/win64/SDL2.dll        <- redistribuivel junto ao .dll
#    - Rust target: rustup target add x86_64-pc-windows-gnu
# ══════════════════════════════════════════════════════════════

WIN_LIB  = libtupi.dll
WIN_OBJS = $(patsubst %.c,$(OBJ_DIR_WIN)/%.o,$(SRCS))

WIN_SDL2_INC = lib/win64/include
WIN_SDL2_LIB = lib/win64

WIN_CFLAGS = -O2 -Wall -Wextra \
             $(COMMON_INCLUDES) \
             -I$(WIN_SDL2_INC)  \
             -DWIN32 -D_WIN32   \
             -D_REENTRANT

WIN_LIBS = -L$(RUST_LIB_PATH)/x86_64-pc-windows-gnu \
           -ltupi_seguro                              \
           -L$(WIN_SDL2_LIB) -lSDL2                  \
           -lm -static-libgcc

# ══════════════════════════════════════════════════════════════
#  Standalone Linux — dist-linux
# ══════════════════════════════════════════════════════════════

LOADER_SRC     = main_bytecode_loader.c
DIST_BIN_LINUX = tupi_engine

DIST_SRCS = $(LOADER_SRC) $(SRCS)
DIST_OBJS = $(patsubst %.c,$(OBJ_DIR_DIST)/%.o,$(DIST_SRCS))

DIST_CFLAGS = -O2 -Wall                 \
              -I/usr/include/luajit-2.1  \
              $(COMMON_INCLUDES)         \
              $(shell pkg-config --cflags sdl2)

_HAVE_STATIC_SDL2 := $(shell find /usr/lib /usr/local/lib 2>/dev/null \
                               -name "libSDL2.a" | head -1)

ifeq ($(_HAVE_STATIC_SDL2),)
  $(info [TupiEngine] SDL2 estatico nao encontrado — linkagem dinamica.)
  DIST_LIBS = -L$(RUST_LIB_PATH)             \
              -Wl,--whole-archive             \
              -lluajit-5.1                    \
              -Wl,--no-whole-archive          \
              -ltupi_seguro                   \
              $(shell pkg-config --libs sdl2) \
              -lm -ldl -lpthread              \
              -Wl,-E
else
  $(info [TupiEngine] SDL2 estatico encontrado — linkando estaticamente.)
  DIST_LIBS = -L$(RUST_LIB_PATH)    \
              -Wl,--whole-archive    \
              -lluajit-5.1           \
              -Wl,--no-whole-archive  \
              -ltupi_seguro          \
              -Wl,-Bstatic -lSDL2    \
              -Wl,-Bdynamic          \
              -lm -ldl -lpthread     \
              -static-libgcc         \
              -Wl,-E
endif

# ══════════════════════════════════════════════════════════════
#  Phony
# ══════════════════════════════════════════════════════════════

.PHONY: all menu sdl2 win dist-linux rodar compilar_rust compilar_rust_win \
        limpar instalar-deps-linux instalar-deps-win ajuda \
        _build_sdl2 _build_win _clean _deps_linux _deps_win

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
	@printf "  $(GREEN)$(BOLD)[1]$(RESET) Compilar Linux      $(DIM)(libtupi.so — SDL2)$(RESET)\n"
	@printf "  $(CYAN)$(BOLD)[2]$(RESET) Compilar Windows    $(DIM)(libtupi.dll — SDL2 cross-compile)$(RESET)\n"
	@printf "  $(YELLOW)$(BOLD)[3]$(RESET) Standalone Linux    $(DIM)(binario dist-linux)$(RESET)\n"
	@printf "  $(RED)$(BOLD)[4]$(RESET) Limpar artefatos\n"
	@printf "  $(YELLOW)$(BOLD)[5]$(RESET) Dependencias Linux\n"
	@printf "  $(YELLOW)$(BOLD)[6]$(RESET) Dependencias Windows $(DIM)(cross-compile)$(RESET)\n"
	@printf "\n$(DIM)  ────────────────────────────────────────────────────$(RESET)\n\n"
	@printf "  > Digite o numero e pressione Enter: " && \
	read OPCAO; \
	case $$OPCAO in \
		1) $(MAKE) _build_sdl2  ;; \
		2) $(MAKE) _build_win   ;; \
		3) $(MAKE) dist-linux   ;; \
		4) $(MAKE) _clean       ;; \
		5) $(MAKE) _deps_linux  ;; \
		6) $(MAKE) _deps_win    ;; \
		*) printf "$(RED)Opcao invalida.$(RESET)\n" ;; \
	esac

_build_sdl2: sdl2
_build_win:  win
_clean:      limpar
_deps_linux: instalar-deps-linux
_deps_win:   instalar-deps-win

# ══════════════════════════════════════════════════════════════
#  Linux SDL2 — compilação incremental
# ══════════════════════════════════════════════════════════════

sdl2: compilar_rust $(SDL2_LIB)
	@printf "$(GREEN)$(BOLD)+  Pronto: $(SDL2_LIB)$(RESET)\n\n"

$(SDL2_LIB): $(SDL2_OBJS)
	@printf "$(CYAN)>  Linkando $(BOLD)$(SDL2_LIB)$(RESET)$(CYAN)...$(RESET)\n"
	@$(CC) -shared -fPIC -o $@ $^ $(SDL2_LIBS) \
		&& printf "$(GREEN)+  Link OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no link Linux.$(RESET)\n\n"; exit 1; }

$(OBJ_DIR_SDL2)/%.o: %.c
	@mkdir -p $(dir $@)
	@printf "$(DIM)   CC  $<$(RESET)\n"
	@$(CC) $(SDL2_CFLAGS) -c $< -o $@ \
		|| { printf "$(RED)x  Erro em $<$(RESET)\n"; exit 1; }

# ══════════════════════════════════════════════════════════════
#  Windows SDL2 — cross-compile incremental
# ══════════════════════════════════════════════════════════════

win: compilar_rust_win $(WIN_LIB)
	@printf "$(GREEN)$(BOLD)+  Pronto: $(WIN_LIB)$(RESET)\n"
	@printf "$(DIM)   Lembre de distribuir SDL2.dll junto ao executavel.$(RESET)\n\n"

$(WIN_LIB): $(WIN_OBJS)
	@printf "$(CYAN)>  Linkando $(BOLD)$(WIN_LIB)$(RESET)$(CYAN)...$(RESET)\n"
	@$(CC_WIN) -shared -o $@ $^ $(WIN_LIBS) \
		&& printf "$(GREEN)+  Link OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no link Windows.$(RESET)\n\n"; exit 1; }

$(OBJ_DIR_WIN)/%.o: %.c
	@mkdir -p $(dir $@)
	@printf "$(DIM)   CC  $<$(RESET)\n"
	@$(CC_WIN) $(WIN_CFLAGS) -c $< -o $@ \
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
	@printf "$(CYAN)>  Compilando Rust (Linux)...$(RESET)\n"
	@cd $(RUST_DIR) && $(CARGO) build --release \
		&& printf "$(GREEN)+  Rust OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no build Rust.$(RESET)\n"; exit 1; }

compilar_rust_win:
	@printf "$(CYAN)>  Compilando Rust (Windows target)...$(RESET)\n"
	@cd $(RUST_DIR) && $(CARGO) build --release --target x86_64-pc-windows-gnu \
		&& printf "$(GREEN)+  Rust Win OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no build Rust Win.$(RESET)\n"; exit 1; }

# ══════════════════════════════════════════════════════════════
#  Rodar
# ══════════════════════════════════════════════════════════════

rodar: sdl2
	@printf "$(GREEN)>  Iniciando LuaJIT...$(RESET)\n\n"
	@DISPLAY=:0 GDK_BACKEND=x11 $(LUAJIT) main.lua

# ══════════════════════════════════════════════════════════════
#  Limpeza
# ══════════════════════════════════════════════════════════════

limpar:
	@printf "\n$(RED)  Limpando artefatos...$(RESET)\n"
	@rm -rf .build $(SDL2_LIB) $(WIN_LIB) $(DIST_BIN_LINUX)
	@cd $(RUST_DIR) && $(CARGO) clean
	@printf "$(GREEN)+  Tudo limpo.$(RESET)\n\n"

# ══════════════════════════════════════════════════════════════
#  Dependências
# ══════════════════════════════════════════════════════════════

instalar-deps-linux:
	@printf "\n$(CYAN)$(BOLD)  Dependencias — Linux (Arch)$(RESET)\n\n"
	@printf "  sdl2  luajit  base-devel  rust\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		sudo pacman -S --needed sdl2 luajit base-devel rust \
			&& printf "$(GREEN)+  Instalado.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi

instalar-deps-win:
	@printf "\n$(CYAN)$(BOLD)  Dependencias — Windows cross-compile$(RESET)\n\n"
	@printf "  1. Instalar mingw-w64:\n"
	@printf "     $(DIM)sudo pacman -S --needed mingw-w64-gcc$(RESET)\n\n"
	@printf "  2. Adicionar target Rust:\n"
	@printf "     $(DIM)rustup target add x86_64-pc-windows-gnu$(RESET)\n\n"
	@printf "  3. Baixar SDL2 dev para MinGW em:\n"
	@printf "     $(DIM)https://github.com/libsdl-org/SDL/releases$(RESET)\n"
	@printf "     Extrair e colocar em:\n"
	@printf "     $(DIM)lib/win64/include/SDL2/  <- headers$(RESET)\n"
	@printf "     $(DIM)lib/win64/libSDL2.a      <- lib$(RESET)\n"
	@printf "     $(DIM)lib/win64/SDL2.dll        <- redistribuivel$(RESET)\n\n"
	@printf "  > Instalar mingw-w64 agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		sudo pacman -S --needed mingw-w64-gcc \
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
	@printf "  $(GREEN)make sdl2$(RESET)             Compila $(SDL2_LIB)  (Linux)\n"
	@printf "  $(CYAN)make win$(RESET)              Compila $(WIN_LIB) (Windows)\n"
	@printf "  $(GREEN)make rodar$(RESET)            Compila e executa no Linux\n"
	@printf "  $(YELLOW)make dist-linux$(RESET)       Binario standalone Linux\n"
	@printf "  $(RED)make limpar$(RESET)           Remove .build/ e artefatos\n\n"