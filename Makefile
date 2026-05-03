SHELL := /bin/bash

CC       = gcc
CC_WIN   = x86_64-w64-mingw32-gcc
LUAJIT   = luajit
CARGO    = cargo
GLSLC    = glslc
XXD      = xxd
AR       = ar
PKG_CONFIG = pkg-config

RED    = \033[0;31m
GREEN  = \033[0;32m
YELLOW = \033[0;33m
CYAN   = \033[0;36m
BOLD   = \033[1m
DIM    = \033[2m
RESET  = \033[0m

UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
HOST_OS := Linux

ifeq ($(OS),Windows_NT)
HOST_OS := Windows
endif
ifneq (,$(findstring MINGW,$(UNAME_S)))
HOST_OS := Windows
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
HOST_OS := Windows
endif
ifneq (,$(findstring CYGWIN,$(UNAME_S)))
HOST_OS := Windows
endif

SUDO := $(shell if command -v sudo >/dev/null 2>&1; then printf 'sudo '; fi)
LINUX_PKG_MANAGER := $(shell \
	if command -v apt-get >/dev/null 2>&1; then echo apt; \
	elif command -v dnf >/dev/null 2>&1; then echo dnf; \
	elif command -v pacman >/dev/null 2>&1; then echo pacman; \
	elif command -v zypper >/dev/null 2>&1; then echo zypper; \
	elif command -v apk >/dev/null 2>&1; then echo apk; \
	else echo unknown; fi)
WIN_INSTALLER := $(shell \
	if command -v winget >/dev/null 2>&1; then echo winget; \
	elif command -v choco >/dev/null 2>&1; then echo choco; \
	elif command -v scoop >/dev/null 2>&1; then echo scoop; \
	else echo unknown; fi)

SRC_DIR       = src/Renderizador
CAMERA_DIR    = src/Camera
RUST_DIR      = .
RUST_LIB_PATH = ./target/release
OUTDIR        ?= $(CURDIR)/dist/linux-export
GAME_NAME     ?= MeuJogo

OBJ_DIR_SDL2 = .build/sdl2
OBJ_DIR_WIN  = .build/win
OBJ_DIR_DIST = .build/dist

SHADER_DIR      = src/Renderizador/shaders
SHADERS_GLSL    = $(SHADER_DIR)/tupi2d.vert $(SHADER_DIR)/tupi2d.frag
SHADERS_SPV     = $(SHADERS_GLSL:=.spv)
SHADER_VERT_SPV = $(SHADER_DIR)/tupi2d.vert.spv
SHADER_FRAG_SPV = $(SHADER_DIR)/tupi2d.frag.spv
SHADER_VERT_HDR = $(SHADER_DIR)/tupi2d_vert_spv.h
SHADER_FRAG_HDR = $(SHADER_DIR)/tupi2d_frag_spv.h
SHADERS_EMBED   = $(SHADER_VERT_HDR) $(SHADER_FRAG_HDR)

SRCS = $(SRC_DIR)/Renderer.c       \
       $(CAMERA_DIR)/Camera.c      \
       src/Colisores/Fisica.c      \
       src/Inputs/Inputs.c         \
       src/Colisores/ColisoesAABB.c \
       src/Sprites/Sprites.c       \
       src/Mapas/Mapas.c

COMMON_INCLUDES = -I$(SRC_DIR)    \
                  -I$(CAMERA_DIR) \
                  -Iinclude       \
                  -Isrc           \
                  -Isrc/Mapas     \
                  -Isrc/Sprites   \
                  -Isrc/Colisores \
                  -Isrc/Inputs

SDL2_LIB  = libtupi.so
SDL2_OBJS = $(patsubst %.c,$(OBJ_DIR_SDL2)/%.o,$(SRCS))

SDL2_CFLAGS = -O2 -Wall -Wextra -fPIC \
              $(COMMON_INCLUDES)      \
              $(shell pkg-config --cflags sdl2)

SDL2_LIBS = -L$(RUST_LIB_PATH) -ltupi_seguro \
            $(shell pkg-config --libs sdl2)  \
            $(shell pkg-config --libs vulkan) \
            -lm -ldl -lpthread

WIN_LIB  = libtupi.dll
WIN_OBJS = $(patsubst %.c,$(OBJ_DIR_WIN)/%.o,$(SRCS))

WIN_SDL2_INC   = lib/win64/include
WIN_SDL2_LIB   = lib/win64
WIN_VULKAN_LIB = vulkan-1

WIN_CFLAGS = -O2 -Wall -Wextra \
             $(COMMON_INCLUDES) \
             -I$(WIN_SDL2_INC) \
             -DWIN32 -D_WIN32 \
             -D_REENTRANT

WIN_LIBS = -L$(RUST_LIB_PATH)/x86_64-pc-windows-gnu \
           -ltupi_seguro \
           -L$(WIN_SDL2_LIB) -lSDL2 \
           -l$(WIN_VULKAN_LIB) \
           -lm -static-libgcc

LOADER_SRC     = main_bytecode_loader.c
DIST_BIN_LINUX = tupi_engine
PACKER_BIN     = ./target/release/tupi_pack
LUA_MODULE_SRCS = $(sort $(wildcard src/Engine/*.lua))
LUA_ARCHIVE_NAME = game.tuzip
ALL_ASSETS = $(shell find . -type f \( \
    -name '*.png' -o -name '*.jpg' -o -name '*.jpeg' -o -name '*.bmp' \
    -o -name '*.gif' -o -name '*.webp' \
    -o -name '*.wav' -o -name '*.ogg' -o -name '*.mp3' -o -name '*.flac' \
    -o -name '*.ttf' -o -name '*.otf' \
    -o -name '*.json' -o -name '*.csv' -o -name '*.tmx' -o -name '*.tsx' \
    \) \
    -not -path './target/*' \
    -not -path './.build/*' \
    -not -path './.engine/*' \
    | sort)

ALL_ASSETS_DIR ?= assets

EXPORT_BIN_DIR    = $(OUTDIR)/bin
EXPORT_SCRIPT_DIR = $(OUTDIR)/scripts
EXPORT_ASSET_DIR  = $(OUTDIR)/assets
EXPORT_LIB_DIR    = $(OUTDIR)/lib

DIST_SRCS = $(LOADER_SRC) $(SRCS)
DIST_OBJS = $(patsubst %.c,$(OBJ_DIR_DIST)/%.o,$(DIST_SRCS))
DIST_CORE_OBJS = $(patsubst %.c,$(OBJ_DIR_DIST)/%.o,$(SRCS))
DIST_STATIC_LIB = $(OBJ_DIR_DIST)/libtupi_engine_core.a

LUAJIT_PKG := $(shell \
	if $(PKG_CONFIG) --exists luajit; then echo luajit; \
	elif $(PKG_CONFIG) --exists luajit-5.1; then echo luajit-5.1; \
	elif $(PKG_CONFIG) --exists luajit-2.1; then echo luajit-2.1; \
	else echo; fi)

SDL2_CFLAGS_PKG  = $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL2_LIBS_PKG    = $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null)
VULKAN_CFLAGS_PKG = $(shell $(PKG_CONFIG) --cflags vulkan 2>/dev/null)
VULKAN_LIBS_PKG   = $(shell if $(PKG_CONFIG) --exists vulkan; then $(PKG_CONFIG) --libs vulkan; else echo -lvulkan; fi)

ifeq ($(strip $(LUAJIT_PKG)),)
LUAJIT_CFLAGS = -I/usr/include/luajit-2.1 -I/usr/local/include/luajit-2.1
LUAJIT_LIBS   = -lluajit-5.1
else
LUAJIT_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(LUAJIT_PKG))
LUAJIT_LIBS   = $(shell $(PKG_CONFIG) --libs $(LUAJIT_PKG))
endif

DIST_CFLAGS = -O2 -Wall \
              $(COMMON_INCLUDES) \
              $(LUAJIT_CFLAGS) \
              $(SDL2_CFLAGS_PKG) \
              $(VULKAN_CFLAGS_PKG) \
              $(shell pkg-config --cflags libzip)

# Flags para dist-linux (opção 3) e bundle-linux (opção 4):
# Ambos usam o mesmo binário — a diferença é o conteúdo do ZIP anexado.
# -Wl,-E exporta todos os símbolos (necessário para plugins Lua via dlopen).
# NÃO usamos rpath aqui: no modo bundle as libs chegam via LD_LIBRARY_PATH
# setado pelo próprio executável antes de se re-executar.
DIST_LIBS = -L$(RUST_LIB_PATH) \
            -ltupi_seguro \
            $(LUAJIT_LIBS) \
            $(SDL2_LIBS_PKG) \
            $(VULKAN_LIBS_PKG) \
            $(shell pkg-config --libs libzip) \
            -lm -ldl -lpthread \
            -Wl,-E

# Flags para export-linux (opção 5): rpath aponta para lib/ bundlada na pasta.
DIST_LIBS_EXPORT = -L$(RUST_LIB_PATH) \
                   -ltupi_seguro \
                   $(LUAJIT_LIBS) \
                   $(SDL2_LIBS_PKG) \
                   $(VULKAN_LIBS_PKG) \
                   $(shell pkg-config --libs libzip) \
                   -lm -ldl -lpthread \
                   -Wl,-E \
                   -Wl,-rpath,'$$ORIGIN/../lib'

LINUX_DEPS_APT = build-essential libsdl2-dev libvulkan-dev libshaderc-dev luajit libluajit-5.1-dev libzip-dev pkg-config curl rustc cargo
LINUX_DEPS_DNF = gcc gcc-c++ make SDL2-devel vulkan-loader-devel shaderc-devel luajit luajit-devel libzip-devel pkgconf-pkg-config rust cargo curl
LINUX_DEPS_PACMAN = base-devel sdl2 vulkan-icd-loader shaderc luajit libzip rust pkgconf curl
LINUX_DEPS_ZYPPER = gcc gcc-c++ make SDL2-devel vulkan-loader-devel shaderc-devel luajit-devel libzip-devel pkg-config rust cargo curl
LINUX_DEPS_APK = build-base sdl2-dev vulkan-loader-dev shaderc-dev luajit luajit-dev libzip-dev pkgconf rust cargo curl

WIN_CROSS_DEPS_APT = mingw-w64 gcc-mingw-w64-x86-64
WIN_CROSS_DEPS_DNF = mingw64-gcc
WIN_CROSS_DEPS_PACMAN = mingw-w64-gcc
WIN_CROSS_DEPS_ZYPPER = cross-x86_64-w64-mingw32-gcc

.PHONY: all menu sdl2 win dist-linux bundle-linux export-linux rodar compilar_rust compilar_rust_win compilar_packer shaders \
        limpar instalar-deps-linux instalar-deps-win ajuda \
        _build_sdl2 _build_win _clean _deps_linux _deps_win

all: menu

menu:
	@clear
	@printf "$(CYAN)$(BOLD)"
	@printf "╔══════════════════════════════════════════════════════╗\n"
	@printf "║             TupiEngine - Build System               ║\n"
	@printf "╚══════════════════════════════════════════════════════╝\n"
	@printf "$(RESET)\n"
	@printf "$(BOLD)  Selecione uma opcao:$(RESET)\n\n"
	@printf "  $(GREEN)$(BOLD)[1]$(RESET) Compilar Linux      $(DIM)(libtupi.so - SDL2)$(RESET)\n"
	@printf "  $(CYAN)$(BOLD)[2]$(RESET) Compilar Windows    $(DIM)(libtupi.dll - SDL2 cross-compile)$(RESET)\n"
	@printf "  $(YELLOW)$(BOLD)[3]$(RESET) Standalone Linux  $(DIM)(binario com Lua embutido)$(RESET)\n"
	@printf "  $(YELLOW)$(BOLD)[4]$(RESET) Bundle Linux      $(DIM)(UM executavel: ZIP-append, como Godot/Love2d)$(RESET)\n"
	@printf "  $(YELLOW)$(BOLD)[5]$(RESET) Export Linux      $(DIM)(pasta portatil - roda em qualquer distro)$(RESET)\n"
	@printf "  $(RED)$(BOLD)[6]$(RESET) Limpar artefatos\n"
	@printf "  $(YELLOW)$(BOLD)[7]$(RESET) Dependencias Linux\n"
	@printf "  $(YELLOW)$(BOLD)[8]$(RESET) Dependencias Windows\n"
	@printf "\n$(DIM)  ----------------------------------------------------$(RESET)\n\n"
	@printf "  > Digite o numero e pressione Enter: " && \
	read OPCAO; \
	case $$OPCAO in \
		1) $(MAKE) _build_sdl2 ;; \
		2) $(MAKE) _build_win ;; \
		3) $(MAKE) dist-linux ;; \
		4) \
			printf "\n$(CYAN)$(BOLD)  Bundle Linux$(RESET)\n\n"; \
			printf "  Nome do executavel final $(DIM)(Enter = MeuJogo)$(RESET): "; \
			read GAME_NAME; \
			[ -z "$$GAME_NAME" ] && GAME_NAME="MeuJogo"; \
			printf "  Diretorio de destino $(DIM)(Enter = ~/Desktop)$(RESET): "; \
			read GAME_DIR; \
			[ -z "$$GAME_DIR" ] && GAME_DIR="$$HOME/Desktop"; \
			GAME_DIR=$$(eval echo "$$GAME_DIR"); \
			printf "\n$(DIM)  Gerando bundle em: $$GAME_DIR/$$GAME_NAME$(RESET)\n\n"; \
			$(MAKE) bundle-linux GAME_NAME="$$GAME_NAME" OUTDIR="$$GAME_DIR" ;; \
		5) \
			printf "\n$(CYAN)$(BOLD)  Export Linux$(RESET)\n\n"; \
			printf "  Nome do jogo $(DIM)(nome da pasta de saida)$(RESET): "; \
			read GAME_NAME; \
			if [ -z "$$GAME_NAME" ]; then \
				printf "$(RED)x  Nome nao pode ser vazio.$(RESET)\n"; \
			else \
				printf "  Diretorio de destino $(DIM)(Enter = ~/Desktop)$(RESET): "; \
				read GAME_DIR; \
				[ -z "$$GAME_DIR" ] && GAME_DIR="$$HOME/Desktop"; \
				GAME_DIR=$$(eval echo "$$GAME_DIR"); \
				FINAL_DIR="$$GAME_DIR/$$GAME_NAME"; \
				printf "\n$(DIM)  Exportando para: $$FINAL_DIR$(RESET)\n\n"; \
				$(MAKE) export-linux OUTDIR="$$FINAL_DIR" GAME_NAME="$$GAME_NAME"; \
			fi ;; \
		6) $(MAKE) _clean ;; \
		7) $(MAKE) _deps_linux ;; \
		8) $(MAKE) _deps_win ;; \
		*) printf "$(RED)Opcao invalida.$(RESET)\n" ;; \
	esac

_build_sdl2: sdl2
_build_win: win
_clean: limpar
_deps_linux: instalar-deps-linux
_deps_win: instalar-deps-win

sdl2: compilar_rust shaders $(SDL2_LIB)
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

$(OBJ_DIR_SDL2)/src/Renderizador/Renderer.o: $(SHADERS_EMBED)

win: compilar_rust_win shaders $(WIN_LIB)
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

$(OBJ_DIR_WIN)/src/Renderizador/Renderer.o: $(SHADERS_EMBED)

# ---------------------------------------------------------------------------
# dist-linux (opção 3) — executável com scripts Lua embutidos, sem libs.
# Requer SDL2/Vulkan/LuaJIT instalados no sistema do usuário final.
# ---------------------------------------------------------------------------
dist-linux: compilar_rust compilar_packer shaders $(DIST_OBJS) main.lua $(LUA_MODULE_SRCS)
	@printf "\n$(CYAN)$(BOLD)  Linkando binario standalone...$(RESET)\n"
	@$(CC) -o $(DIST_BIN_LINUX) $(DIST_OBJS) $(DIST_LIBS) \
		&& printf "$(GREEN)+  Link OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no link standalone.$(RESET)\n\n"; exit 1; }
	@printf "$(CYAN)>  Anexando scripts Lua ao executavel...$(RESET)\n"
	@$(PACKER_BIN) append $(DIST_BIN_LINUX) main.lua $(LUA_MODULE_SRCS) \
		&& printf "$(GREEN)+  Pronto: $(BOLD)$(DIST_BIN_LINUX)$(RESET)$(GREEN) com Lua embutido.$(RESET)\n\n" \
		|| { printf "$(RED)x  Falha ao anexar scripts ao executavel.$(RESET)\n\n"; exit 1; }

# ---------------------------------------------------------------------------
# bundle-linux (opção 4) — UM único arquivo executável portátil.
#
# Técnica ZIP-append (idêntica à Godot e Love2d):
#   ELF + ZIP[ __main__, scripts/*, lib/*.so, assets/* ] + TRAILER
#
# O próprio main_bytecode_loader.c detecta o prefixo "lib/" no ZIP e,
# na primeira execução, extrai as .so para /tmp/tupi_<hash>/, seta
# LD_LIBRARY_PATH e se re-executa via execv(). Na segunda passagem
# (TUPI_LIBS_EXTRACTED=1) roda normalmente.
#
# Não há bootstrapper separado — o executável é self-contained.
#
# Uso:
#   make bundle-linux GAME_NAME=MeuJogo OUTDIR=~/Desktop
# ---------------------------------------------------------------------------
bundle-linux: compilar_rust compilar_packer shaders $(DIST_OBJS) main.lua $(LUA_MODULE_SRCS)
	@printf "\n$(CYAN)$(BOLD)  [Bundle] Gerando executavel unico (ZIP-append)...$(RESET)\n"
	@mkdir -p "$(OUTDIR)" .build/bundle_libs

	@# --- 1. Linka o executável final (SDL2 linkado dinamicamente) ---
	@printf "$(CYAN)>  Linkando executavel bundle...$(RESET)\n"
	@$(CC) -o "$(OUTDIR)/$(GAME_NAME)" $(DIST_OBJS) $(DIST_LIBS) \
		&& printf "$(GREEN)+  Link OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no link bundle.$(RESET)\n\n"; exit 1; }

	@# --- 2. Coleta as .so que o executável precisa ---
	@printf "$(CYAN)>  Coletando bibliotecas dinamicas...$(RESET)\n"
	@rm -rf .build/bundle_libs && mkdir -p .build/bundle_libs
	@ldd "$(OUTDIR)/$(GAME_NAME)" 2>/dev/null | \
		awk '/=>/ { print $$3 }' | grep -v '^$$' | \
		grep -Ev '/(libc|libm|libdl|libpthread|libgcc_s|libstdc\+\+|ld-linux)[^/]*\.so' | \
		while read SO; do \
			[ -f "$$SO" ] || continue; \
			SONAME=$$(basename "$$SO"); \
			cp -L "$$SO" ".build/bundle_libs/$$SONAME" 2>/dev/null && \
				printf "$(DIM)   bundled: $$SONAME$(RESET)\n" || true; \
		done
	@printf "$(GREEN)+  Libs coletadas.$(RESET)\n"

	@# --- 3. Coleta assets (automático: todos os formatos, sem diretório temporário) ---
	@printf "$(CYAN)>  Coletando assets...$(RESET)\n"
	@ASSET_LIST="$(ALL_ASSETS)"; \
	if [ -n "$$ASSET_LIST" ]; then \
		for f in $$ASSET_LIST; do printf "$(DIM)   incluido: $${f#./}$(RESET)\n"; done; \
		printf "$(GREEN)+  $$(echo $$ASSET_LIST | wc -w) asset(s) encontrado(s).$(RESET)\n"; \
	else \
		printf "$(YELLOW)!  Nenhum asset encontrado.$(RESET)\n"; \
	fi

	@# --- 4. Empacota: ELF + ZIP[ __main__, scripts/*, lib/*.so, assets/* ] ---
	@# Assets passados com path relativo ao projeto → packer normaliza para assets/<rel>.
	@# Resultado: assets/ascii.png, assets/tilesets/grama.png, etc.
	@printf "$(CYAN)>  Empacotando scripts + libs + assets no executavel...$(RESET)\n"
	@LIB_COUNT=$$(find .build/bundle_libs -name '*.so*' -type f 2>/dev/null | wc -l); \
	CMD="$(PACKER_BIN) append \"$(OUTDIR)/$(GAME_NAME)\" main.lua $(LUA_MODULE_SRCS)"; \
	[ "$$LIB_COUNT" -gt 0 ] && CMD="$$CMD --libs .build/bundle_libs"; \
	[ -n "$(ALL_ASSETS)" ]   && CMD="$$CMD --assets $(ALL_ASSETS)"; \
	eval $$CMD \
		&& printf "$(GREEN)+  ZIP anexado ao executavel OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha ao anexar ZIP.$(RESET)\n\n"; exit 1; }

	@chmod +x "$(OUTDIR)/$(GAME_NAME)"
	@printf "\n$(GREEN)$(BOLD)+  '$(GAME_NAME)' pronto em: $(OUTDIR)/$(GAME_NAME)$(RESET)\n"
	@printf "$(DIM)   Arquivo unico — copie para onde quiser e execute.\n"
	@printf "   Na primeira execucao extrai libs em /tmp/tupi_<hash>/ e reinicia.\n"
	@printf "   Execucoes seguintes iniciam diretamente (tmpdir reutilizado).$(RESET)\n\n"

# ---------------------------------------------------------------------------
# export-linux (opção 5) — pasta portátil que roda em qualquer distro x86_64
# ---------------------------------------------------------------------------
export-linux: compilar_rust compilar_packer shaders $(DIST_OBJS) $(DIST_STATIC_LIB) main.lua $(LUA_MODULE_SRCS)
	@printf "\n$(CYAN)$(BOLD)  Exportando '$(GAME_NAME)' para $(OUTDIR)...$(RESET)\n"
	@mkdir -p "$(EXPORT_BIN_DIR)" "$(EXPORT_SCRIPT_DIR)" "$(EXPORT_ASSET_DIR)" "$(EXPORT_LIB_DIR)"

	@printf "$(CYAN)>  Linkando runner com rpath portatil...$(RESET)\n"
	@$(CC) -o "$(EXPORT_BIN_DIR)/$(DIST_BIN_LINUX)" $(DIST_OBJS) $(DIST_LIBS_EXPORT) \
		&& printf "$(GREEN)+  Runner exportado.$(RESET)\n" \
		|| { printf "$(RED)x  Falha ao linkar runner do export Linux.$(RESET)\n\n"; exit 1; }

	@$(PACKER_BIN) archive "$(EXPORT_SCRIPT_DIR)/$(LUA_ARCHIVE_NAME)" main.lua $(LUA_MODULE_SRCS) \
		&& printf "$(GREEN)+  Scripts Lua compactados em $(EXPORT_SCRIPT_DIR)/$(LUA_ARCHIVE_NAME).$(RESET)\n" \
		|| { printf "$(RED)x  Falha ao gerar arquivo ZIP de scripts Lua.$(RESET)\n\n"; exit 1; }

	@cp "$(DIST_STATIC_LIB)" "$(EXPORT_LIB_DIR)/libtupi_engine_core.a"
	@cp "$(RUST_LIB_PATH)/libtupi_seguro.a" "$(EXPORT_LIB_DIR)/libtupi_seguro.a"

	@printf "$(CYAN)>  Copiando bibliotecas dinamicas para lib/ ...$(RESET)\n"
	@ldd "$(EXPORT_BIN_DIR)/$(DIST_BIN_LINUX)" 2>/dev/null | \
		awk '/=>/ { print $$3 }' | \
		grep -v '^$$' | \
		grep -Ev '/(libc|libm|libdl|libpthread|libgcc_s|libstdc\+\+|ld-linux)[^/]*\.so' | \
		while read SO; do \
			[ -f "$$SO" ] || continue; \
			SONAME=$$(basename "$$SO"); \
			cp -L "$$SO" "$(EXPORT_LIB_DIR)/$$SONAME" 2>/dev/null && \
				printf "$(DIM)   bundled: $$SONAME$(RESET)\n" || true; \
		done
	@printf "$(GREEN)+  Bibliotecas bundladas em $(EXPORT_LIB_DIR).$(RESET)\n"

	@printf "$(CYAN)>  Gerando launcher shell...$(RESET)\n"
	@{ \
		printf '#!/bin/sh\n'; \
		printf '# Launcher gerado pelo TupiEngine - nao edite manualmente.\n'; \
		printf 'SCRIPT_DIR="$$(cd "$$(dirname "$$0")" && pwd)"\n'; \
		printf 'export LD_LIBRARY_PATH="$$SCRIPT_DIR/../lib:$$LD_LIBRARY_PATH"\n'; \
		printf 'export TUPI_ASSET_DIR="$$SCRIPT_DIR/../assets"\n'; \
		printf 'export TUPI_SCRIPT_ARCHIVE="$$SCRIPT_DIR/../scripts/$(LUA_ARCHIVE_NAME)"\n'; \
		printf 'exec "$$SCRIPT_DIR/$(DIST_BIN_LINUX)" "$$@"\n'; \
	} > "$(EXPORT_BIN_DIR)/$(DIST_BIN_LINUX).sh"
	@chmod +x "$(EXPORT_BIN_DIR)/$(DIST_BIN_LINUX).sh"
	@printf "$(GREEN)+  Launcher: bin/$(DIST_BIN_LINUX).sh$(RESET)\n"

	@for asset in $(ALL_ASSETS); do \
		rel=$${asset#./}; \
		dest="$(EXPORT_ASSET_DIR)/$$rel"; \
		mkdir -p "$$(dirname "$$dest")"; \
		cp "$$asset" "$$dest"; \
	done
	@printf "$(GREEN)+  Assets copiados para $(EXPORT_ASSET_DIR).$(RESET)\n"

	@printf "\n$(GREEN)$(BOLD)+  '$(GAME_NAME)' exportado com sucesso!$(RESET)\n"
	@printf "$(DIM)   Destino: $(OUTDIR)$(RESET)\n"
	@printf "$(DIM)   Distribua a pasta inteira. O jogo inicia via:$(RESET)\n"
	@printf "$(CYAN)   ./bin/$(DIST_BIN_LINUX).sh$(RESET)\n\n"

$(OBJ_DIR_DIST)/%.o: %.c
	@mkdir -p $(dir $@)
	@printf "$(DIM)   CC  $<$(RESET)\n"
	@$(CC) $(DIST_CFLAGS) -c $< -o $@ \
		|| { printf "$(RED)x  Erro em $<$(RESET)\n"; exit 1; }

$(OBJ_DIR_DIST)/src/Renderizador/Renderer.o: $(SHADERS_EMBED)

$(DIST_STATIC_LIB): $(DIST_CORE_OBJS)
	@printf "$(CYAN)>  Gerando biblioteca estatica do core...$(RESET)\n"
	@$(AR) rcs $@ $^ \
		&& printf "$(GREEN)+  Biblioteca estatica OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha ao gerar biblioteca estatica.$(RESET)\n"; exit 1; }

compilar_rust:
	@printf "$(CYAN)>  Compilando Rust (Linux)...$(RESET)\n"
	@cd $(RUST_DIR) && $(CARGO) build --release \
		&& printf "$(GREEN)+  Rust OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no build Rust.$(RESET)\n"; exit 1; }

compilar_packer:
	@printf "$(CYAN)>  Compilando packer...$(RESET)\n"
	@cd $(RUST_DIR) && $(CARGO) build --release --bin tupi_pack \
		&& printf "$(GREEN)+  Packer OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no build do packer.$(RESET)\n"; exit 1; }

compilar_rust_win:
	@printf "$(CYAN)>  Compilando Rust (Windows target)...$(RESET)\n"
	@cd $(RUST_DIR) && $(CARGO) build --release --target x86_64-pc-windows-gnu \
		&& printf "$(GREEN)+  Rust Win OK.$(RESET)\n" \
		|| { printf "$(RED)x  Falha no build Rust Win.$(RESET)\n"; exit 1; }

rodar: sdl2
	@printf "$(GREEN)>  Iniciando LuaJIT...$(RESET)\n\n"
	@DISPLAY=:0 GDK_BACKEND=x11 $(LUAJIT) main.lua

limpar:
	@printf "\n$(RED)  Limpando artefatos...$(RESET)\n"
	@rm -rf .build dist $(SDL2_LIB) $(WIN_LIB) $(DIST_BIN_LINUX) $(SHADERS_SPV) $(SHADERS_EMBED)
	@rm -rf .build/bundle_libs
	@cd $(RUST_DIR) && $(CARGO) clean
	@printf "$(GREEN)+  Tudo limpo.$(RESET)\n\n"

ajuda:
	@printf "\n$(CYAN)$(BOLD)  TupiEngine - comandos disponiveis:$(RESET)\n\n"
	@printf "  $(GREEN)make$(RESET)                  Menu interativo\n"
	@printf "  $(GREEN)make sdl2$(RESET)             Compila $(SDL2_LIB) (Linux)\n"
	@printf "  $(CYAN)make win$(RESET)               Compila $(WIN_LIB) (Windows)\n"
	@printf "  $(GREEN)make rodar$(RESET)            Compila e executa no Linux\n"
	@printf "  $(YELLOW)make dist-linux$(RESET)      Binario standalone Linux (sem libs bundladas)\n"
	@printf "  $(YELLOW)make bundle-linux$(RESET)    Arquivo unico portatil (ZIP-append, como Godot)\n"
	@printf "  $(YELLOW)make export-linux$(RESET)    Pasta portatil Linux (roda em qualquer distro)\n"
	@printf "  $(YELLOW)make instalar-deps-linux$(RESET) Instala dependencias Linux\n"
	@printf "  $(YELLOW)make instalar-deps-win$(RESET)   Instala dependencias Windows\n"
	@printf "  $(RED)make limpar$(RESET)             Remove artefatos\n\n"

shaders: $(SHADERS_SPV) $(SHADERS_EMBED)
	@printf "$(GREEN)+  Shaders SPIR-V atualizados.$(RESET)\n"

$(SHADER_DIR)/%.vert.spv: $(SHADER_DIR)/%.vert
	@printf "$(DIM)   GLSLC $<$(RESET)\n"
	@$(GLSLC) -fshader-stage=vert $< -o $@ \
		|| { printf "$(RED)x  Erro ao compilar shader $<$(RESET)\n"; exit 1; }

$(SHADER_DIR)/%.frag.spv: $(SHADER_DIR)/%.frag
	@printf "$(DIM)   GLSLC $<$(RESET)\n"
	@$(GLSLC) -fshader-stage=frag $< -o $@ \
		|| { printf "$(RED)x  Erro ao compilar shader $<$(RESET)\n"; exit 1; }

$(SHADER_VERT_HDR): $(SHADER_VERT_SPV)
	@printf "$(DIM)   XXD   $<$(RESET)\n"
	@$(XXD) -i -n tupi2d_vert_spv $< > $@

$(SHADER_FRAG_HDR): $(SHADER_FRAG_SPV)
	@printf "$(DIM)   XXD   $<$(RESET)\n"
	@$(XXD) -i -n tupi2d_frag_spv $< > $@

instalar-deps-linux:
ifeq ($(HOST_OS),Windows)
	@printf "\n$(YELLOW)$(BOLD)  Host Windows detectado$(RESET)\n\n"
	@printf "  Use um terminal MSYS2/MINGW64 e execute:\n"
	@printf "  $(DIM)pacman -S --needed base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-vulkan-loader mingw-w64-x86_64-shaderc mingw-w64-x86_64-luajit rust cargo$(RESET)\n\n"
else ifeq ($(LINUX_PKG_MANAGER),apt)
	@printf "\n$(CYAN)$(BOLD)  Dependencias Linux detectadas para APT$(RESET)\n\n"
	@printf "  $(LINUX_DEPS_APT)\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)apt-get update && $(SUDO)apt-get install -y $(LINUX_DEPS_APT) \
			&& printf "$(GREEN)+  Dependencias Linux instaladas.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha ao instalar dependencias Linux.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(LINUX_PKG_MANAGER),dnf)
	@printf "\n$(CYAN)$(BOLD)  Dependencias Linux detectadas para DNF$(RESET)\n\n"
	@printf "  $(LINUX_DEPS_DNF)\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)dnf install -y $(LINUX_DEPS_DNF) \
			&& printf "$(GREEN)+  Dependencias Linux instaladas.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha ao instalar dependencias Linux.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(LINUX_PKG_MANAGER),pacman)
	@printf "\n$(CYAN)$(BOLD)  Dependencias Linux detectadas para Pacman$(RESET)\n\n"
	@printf "  $(LINUX_DEPS_PACMAN)\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)pacman -S --needed $(LINUX_DEPS_PACMAN) \
			&& printf "$(GREEN)+  Dependencias Linux instaladas.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha ao instalar dependencias Linux.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(LINUX_PKG_MANAGER),zypper)
	@printf "\n$(CYAN)$(BOLD)  Dependencias Linux detectadas para Zypper$(RESET)\n\n"
	@printf "  $(LINUX_DEPS_ZYPPER)\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)zypper install -y $(LINUX_DEPS_ZYPPER) \
			&& printf "$(GREEN)+  Dependencias Linux instaladas.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha ao instalar dependencias Linux.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(LINUX_PKG_MANAGER),apk)
	@printf "\n$(CYAN)$(BOLD)  Dependencias Linux detectadas para APK$(RESET)\n\n"
	@printf "  $(LINUX_DEPS_APK)\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)apk add $(LINUX_DEPS_APK) \
			&& printf "$(GREEN)+  Dependencias Linux instaladas.$(RESET)\n\n" \
			|| printf "$(RED)x  Falha ao instalar dependencias Linux.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else
	@printf "\n$(RED)Nao foi possivel identificar o gerenciador de pacotes Linux.$(RESET)\n"
	@printf "Instale manualmente: SDL2, Vulkan, shaderc, LuaJIT, libzip, Rust, Cargo e pkg-config.\n\n"
endif

instalar-deps-win:
ifeq ($(HOST_OS),Windows)
	@printf "\n$(CYAN)$(BOLD)  Dependencias Windows detectadas para $(WIN_INSTALLER)$(RESET)\n\n"
ifeq ($(WIN_INSTALLER),winget)
	@printf "  Pacotes: Rustup, MSYS2\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		winget install -e --id Rustlang.Rustup && \
		winget install -e --id MSYS2.MSYS2 && \
		printf "$(GREEN)+  Base do ambiente Windows instalada.$(RESET)\n" && \
		printf "  Abra o terminal MSYS2 MINGW64 e rode:\n" && \
		printf "  $(DIM)pacman -S --needed base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-vulkan-loader mingw-w64-x86_64-shaderc mingw-w64-x86_64-luajit rust cargo$(RESET)\n\n" \
		|| printf "$(RED)x  Falha ao instalar dependencias Windows.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(WIN_INSTALLER),choco)
	@printf "  Pacotes: rustup.install, msys2\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		choco install -y rustup.install msys2 && \
		printf "$(GREEN)+  Base do ambiente Windows instalada.$(RESET)\n" \
		|| printf "$(RED)x  Falha ao instalar dependencias Windows.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(WIN_INSTALLER),scoop)
	@printf "  Pacotes: rustup, msys2\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		scoop install rustup msys2 && \
		printf "$(GREEN)+  Base do ambiente Windows instalada.$(RESET)\n" \
		|| printf "$(RED)x  Falha ao instalar dependencias Windows.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else
	@printf "  Nenhum instalador suportado encontrado.\n"
	@printf "  Instale manualmente Rustup e MSYS2, depois no terminal MSYS2 MINGW64:\n"
	@printf "  $(DIM)pacman -S --needed base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-vulkan-loader mingw-w64-x86_64-shaderc mingw-w64-x86_64-luajit rust cargo$(RESET)\n\n"
endif
else ifeq ($(LINUX_PKG_MANAGER),apt)
	@printf "\n$(CYAN)$(BOLD)  Dependencias de cross-compile para Windows via APT$(RESET)\n\n"
	@printf "  $(WIN_CROSS_DEPS_APT)\n  rustup target add x86_64-pc-windows-gnu\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)apt-get update && $(SUDO)apt-get install -y $(WIN_CROSS_DEPS_APT) && \
		rustup target add x86_64-pc-windows-gnu && \
		printf "$(GREEN)+  Toolchain de Windows instalada.$(RESET)\n\n" \
		|| printf "$(RED)x  Falha ao instalar toolchain de Windows.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(LINUX_PKG_MANAGER),dnf)
	@printf "\n$(CYAN)$(BOLD)  Dependencias de cross-compile para Windows via DNF$(RESET)\n\n"
	@printf "  $(WIN_CROSS_DEPS_DNF)\n  rustup target add x86_64-pc-windows-gnu\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)dnf install -y $(WIN_CROSS_DEPS_DNF) && \
		rustup target add x86_64-pc-windows-gnu && \
		printf "$(GREEN)+  Toolchain de Windows instalada.$(RESET)\n\n" \
		|| printf "$(RED)x  Falha ao instalar toolchain de Windows.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(LINUX_PKG_MANAGER),pacman)
	@printf "\n$(CYAN)$(BOLD)  Dependencias de cross-compile para Windows via Pacman$(RESET)\n\n"
	@printf "  $(WIN_CROSS_DEPS_PACMAN)\n  rustup target add x86_64-pc-windows-gnu\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)pacman -S --needed $(WIN_CROSS_DEPS_PACMAN) && \
		rustup target add x86_64-pc-windows-gnu && \
		printf "$(GREEN)+  Toolchain de Windows instalada.$(RESET)\n\n" \
		|| printf "$(RED)x  Falha ao instalar toolchain de Windows.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else ifeq ($(LINUX_PKG_MANAGER),zypper)
	@printf "\n$(CYAN)$(BOLD)  Dependencias de cross-compile para Windows via Zypper$(RESET)\n\n"
	@printf "  $(WIN_CROSS_DEPS_ZYPPER)\n  rustup target add x86_64-pc-windows-gnu\n\n"
	@printf "  > Instalar agora? [s/N]: " && read CONF; \
	if [ "$$CONF" = "s" ] || [ "$$CONF" = "S" ]; then \
		$(SUDO)zypper install -y $(WIN_CROSS_DEPS_ZYPPER) && \
		rustup target add x86_64-pc-windows-gnu && \
		printf "$(GREEN)+  Toolchain de Windows instalada.$(RESET)\n\n" \
		|| printf "$(RED)x  Falha ao instalar toolchain de Windows.$(RESET)\n\n"; \
	else \
		printf "$(DIM)  Cancelado.$(RESET)\n\n"; \
	fi
else
	@printf "\n$(RED)Nao foi possivel identificar um fluxo suportado para instalar dependencias Windows.$(RESET)\n"
	@printf "Instale manualmente um compilador MinGW-w64 e execute: rustup target add x86_64-pc-windows-gnu\n\n"
endif