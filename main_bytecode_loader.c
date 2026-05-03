/*
 * main_bytecode_loader.c — Carregador ZIP do TupiEngine
 *
 * Modos de operação:
 *
 *   [A] ZIP anexado SEM libs (dist-linux / opção 3):
 *       O executável contém ELF + ZIP com scripts + assets.
 *       Roda direto — sem extração, assets e libs vêm do disco.
 *
 *   [B] ZIP anexado COM libs (bundle-linux / opção 4  ← novo):
 *       Técnica idêntica à Godot/Love2d: um único arquivo executável.
 *
 *       Na PRIMEIRA chamada (sem TUPI_LIBS_EXTRACTED):
 *         1. Detecta libs no ZIP (prefixo "lib/").
 *         2. Cria /tmp/tupi_<hash>/ e extrai as .so para lá.
 *         3. Extrai assets para /tmp/tupi_<hash>/assets/.
 *         4. Seta LD_LIBRARY_PATH e TUPI_LIBS_EXTRACTED=1.
 *         5. Re-executa SI MESMO via execv() — o dynamic linker
 *            encontra as .so no novo LD_LIBRARY_PATH e carrega
 *            SDL2, Vulkan, LuaJIT etc. sem nenhum bootstrapper externo.
 *
 *       Na SEGUNDA chamada (TUPI_LIBS_EXTRACTED=1):
 *         Já há .so carregadas. Apenas lê scripts do ZIP embutido
 *         e assets de TUPI_ASSET_DIR, e roda o jogo normalmente.
 *
 * Formato do executável empacotado:
 *   [ ELF/PE ][ ZIP ][ "TUPI_ZIP_APPENDED"(17 bytes) ][ offset_zip_u64_le(8 bytes) ]
 *
 * Estrutura interna do ZIP:
 *   __main__              → script Lua principal
 *   scripts/<mod>.lua     → módulos Lua
 *   lib/<nome>.so         → libs dinâmicas (apenas no modo bundle)
 *   assets/<caminho>      → assets do jogo
 *
 * Dependências: libzip, LuaJIT, libdl
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <zip.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/* -------------------------------------------------------------------------
 * Constantes
 * ---------------------------------------------------------------------- */

#define TUPI_APPENDED_MARKER     "TUPI_ZIP_APPENDED"
#define TUPI_MARKER_LEN          17u
#define TUPI_TRAILER_SIZE        (TUPI_MARKER_LEN + 8u)

#define TUPI_MAIN_ENTRY          "__main__"
#define TUPI_SCRIPTS_PREFIX      "scripts/"
#define TUPI_LIB_PREFIX          "lib/"
#define TUPI_ASSETS_PREFIX       "assets/"

/* Variáveis de ambiente */
#define TUPI_SCRIPT_ARCHIVE_ENV  "TUPI_SCRIPT_ARCHIVE"
#define TUPI_ASSET_DIR_ENV       "TUPI_ASSET_DIR"
#define TUPI_LIBS_EXTRACTED_ENV  "TUPI_LIBS_EXTRACTED"   /* flag de re-exec */

/* Fallbacks para modo sidecar (opção 3 / export-linux) */
#define TUPI_DEFAULT_SCRIPT_ARCHIVE "../scripts/game.tuzip"
#define TUPI_LOCAL_SCRIPT_ARCHIVE   "./game.tuzip"
#define TUPI_DEFAULT_ASSET_DIR   "../assets"
#define TUPI_LOCAL_ASSET_DIR     "./assets"

/* Prefixo do tmpdir — mantido entre sessões pelo hash do exe para
 * evitar extrações repetidas quando o usuário executa o jogo várias vezes. */
#define TUPI_TMP_PREFIX          "/tmp/tupi_"

/* -------------------------------------------------------------------------
 * Utilitários little-endian
 * ---------------------------------------------------------------------- */

static uint64_t read_u64_le(const unsigned char *p) {
    return ((uint64_t)p[0])        |
           ((uint64_t)p[1] <<  8)  |
           ((uint64_t)p[2] << 16)  |
           ((uint64_t)p[3] << 24)  |
           ((uint64_t)p[4] << 32)  |
           ((uint64_t)p[5] << 40)  |
           ((uint64_t)p[6] << 48)  |
           ((uint64_t)p[7] << 56);
}

/* -------------------------------------------------------------------------
 * Resolve o caminho do próprio executável via /proc/self/exe
 * ---------------------------------------------------------------------- */

static int tupi_read_self_path(char *out, size_t out_size) {
    ssize_t len = readlink("/proc/self/exe", out, out_size - 1u);
    if (len < 0 || (size_t)len >= out_size) return -1;
    out[len] = '\0';
    return 0;
}

static void tupi_chdir_to_exe_dir(const char *exe_path) {
    char  dir[PATH_MAX];
    char *slash;
    strncpy(dir, exe_path, sizeof(dir) - 1u);
    dir[sizeof(dir) - 1u] = '\0';
    slash = strrchr(dir, '/');
    if (!slash) return;
    *slash = '\0';
    if (dir[0] != '\0') (void)chdir(dir);
}

/* -------------------------------------------------------------------------
 * Leitura do offset do ZIP anexado
 * ---------------------------------------------------------------------- */

static long tupi_read_appended_zip_offset(const char *exe_path) {
    FILE          *f;
    unsigned char  trailer[TUPI_TRAILER_SIZE];
    long           file_size;
    uint64_t       zip_offset;

    f = fopen(exe_path, "rb");
    if (!f) return -1L;

    if (fseek(f, 0L, SEEK_END) != 0) { fclose(f); return -1L; }
    file_size = ftell(f);
    if (file_size < 0 || (unsigned long)file_size < TUPI_TRAILER_SIZE) {
        fclose(f);
        return -1L;
    }
    if (fseek(f, file_size - (long)TUPI_TRAILER_SIZE, SEEK_SET) != 0) {
        fclose(f);
        return -1L;
    }
    if (fread(trailer, 1u, TUPI_TRAILER_SIZE, f) != TUPI_TRAILER_SIZE) {
        fclose(f);
        return -1L;
    }
    fclose(f);

    if (memcmp(trailer, TUPI_APPENDED_MARKER, TUPI_MARKER_LEN) != 0)
        return -1L;

    zip_offset = read_u64_le(trailer + TUPI_MARKER_LEN);
    if (zip_offset >= (uint64_t)file_size) return -1L;
    return (long)zip_offset;
}

/* -------------------------------------------------------------------------
 * Abre o ZIP anexado ao executável via libzip com fonte em offset
 * ---------------------------------------------------------------------- */

static zip_t *tupi_open_appended_zip(const char *exe_path, long zip_offset) {
    zip_source_t *src;
    zip_t        *za;
    zip_error_t   ze;
    long          file_size;
    zip_uint64_t  zip_len;
    FILE         *f;

    f = fopen(exe_path, "rb");
    if (!f) return NULL;
    fseek(f, 0L, SEEK_END);
    file_size = ftell(f);
    fclose(f);
    if (file_size < 0) return NULL;

    zip_len = (zip_uint64_t)(file_size - (long)TUPI_TRAILER_SIZE - zip_offset);

    zip_error_init(&ze);
    src = zip_source_file_create(exe_path, (zip_uint64_t)zip_offset,
                                 (zip_int64_t)zip_len, &ze);
    if (!src) {
        fprintf(stderr, "[Tupi] Não foi possível criar fonte ZIP: %s\n",
                zip_error_strerror(&ze));
        zip_error_fini(&ze);
        return NULL;
    }

    za = zip_open_from_source(src, ZIP_RDONLY, &ze);
    if (!za) {
        fprintf(stderr, "[Tupi] Falha ao abrir ZIP embutido: %s\n",
                zip_error_strerror(&ze));
        zip_source_free(src);
        zip_error_fini(&ze);
        return NULL;
    }
    zip_error_fini(&ze);
    return za;
}

/* Abre um arquivo ZIP sidecar (.tuzip) */
static zip_t *tupi_open_zip_file(const char *path) {
    int    err = 0;
    zip_t *za  = zip_open(path, ZIP_RDONLY, &err);
    if (!za) {
        zip_error_t ze;
        zip_error_init_with_code(&ze, err);
        fprintf(stderr, "[Tupi] Falha ao abrir '%s': %s\n",
                path, zip_error_strerror(&ze));
        zip_error_fini(&ze);
    }
    return za;
}

/* -------------------------------------------------------------------------
 * Lê uma entrada do ZIP para um buffer heap.
 * O chamador é responsável por free().
 * ---------------------------------------------------------------------- */

static unsigned char *tupi_zip_read_entry(zip_t *za, zip_int64_t idx,
                                           size_t *out_size) {
    zip_stat_t     st;
    zip_file_t    *zf;
    unsigned char *buf;
    zip_int64_t    n;

    if (zip_stat_index(za, (zip_uint64_t)idx, 0, &st) != 0) return NULL;
    if (!(st.valid & ZIP_STAT_SIZE))                         return NULL;

    buf = (unsigned char *)malloc((size_t)st.size + 1u);
    if (!buf) return NULL;

    zf = zip_fopen_index(za, (zip_uint64_t)idx, 0);
    if (!zf) { free(buf); return NULL; }

    n = zip_fread(zf, buf, st.size);
    zip_fclose(zf);

    if (n < 0 || (zip_uint64_t)n != st.size) { free(buf); return NULL; }
    buf[st.size] = '\0';
    *out_size = (size_t)st.size;
    return buf;
}

/* -------------------------------------------------------------------------
 * mkdir -p para um caminho de arquivo
 * ---------------------------------------------------------------------- */

static int tupi_mkdirs_for_file(const char *filepath) {
    char  tmp[PATH_MAX];
    char *p;
    size_t len = strlen(filepath);
    if (len == 0 || len >= PATH_MAX) return -1;
    memcpy(tmp, filepath, len + 1u);
    p = strrchr(tmp, '/');
    if (!p) return 0;
    *p = '\0';
    for (p = tmp + 1u; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
    return 0;
}

static int tupi_write_file(const char *path, const unsigned char *data,
                            size_t size, int executable) {
    FILE *f;
    if (tupi_mkdirs_for_file(path) != 0) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    if (fwrite(data, 1u, size, f) != size) { fclose(f); return -1; }
    fclose(f);
    if (executable) chmod(path, 0755);
    return 0;
}

/* -------------------------------------------------------------------------
 * Hash simples do executável (primeiros + últimos 512 bytes) para
 * gerar um tmpdir determinístico. Isso evita re-extração a cada run.
 * ---------------------------------------------------------------------- */

static uint32_t tupi_exe_hash(const char *exe_path) {
    FILE         *f;
    unsigned char buf[512];
    size_t        n;
    uint32_t      h = 2166136261u; /* FNV-1a offset basis */
    size_t        i;

    f = fopen(exe_path, "rb");
    if (!f) return 0;

    /* Lê os primeiros 512 bytes */
    n = fread(buf, 1u, sizeof(buf), f);
    for (i = 0; i < n; i++) {
        h ^= buf[i];
        h *= 16777619u;
    }

    /* Lê os últimos 512 bytes */
    if (fseek(f, -512L, SEEK_END) == 0) {
        n = fread(buf, 1u, sizeof(buf), f);
        for (i = 0; i < n; i++) {
            h ^= buf[i];
            h *= 16777619u;
        }
    }

    fclose(f);
    return h;
}

/* -------------------------------------------------------------------------
 * Verifica se o ZIP contém entradas com um dado prefixo
 * ---------------------------------------------------------------------- */

static int tupi_zip_has_prefix(zip_t *za, const char *prefix) {
    zip_int64_t n = zip_get_num_entries(za, 0);
    zip_int64_t i;
    size_t      plen = strlen(prefix);
    for (i = 0; i < n; i++) {
        const char *name = zip_get_name(za, (zip_uint64_t)i, 0);
        if (name && strncmp(name, prefix, plen) == 0) return 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * ESTÁGIO 1 (bundle): extrai libs + assets, seta env, re-executa si mesmo.
 *
 * Chamado quando o ZIP contém "lib/" E TUPI_LIBS_EXTRACTED não está setado.
 * Retorna 0 em sucesso (mas nunca retorna de verdade — termina em execv).
 * ---------------------------------------------------------------------- */

static int tupi_self_extract_and_reexec(zip_t *za,
                                         const char *exe_path,
                                         char **argv) {
    char        tmpdir[PATH_MAX];
    char        lib_dir[PATH_MAX + 8];    /* +"/lib"    = 4 bytes extra */
    char        asset_dir[PATH_MAX + 8];  /* +"/assets" = 7 bytes extra */
    zip_int64_t num_entries;
    zip_int64_t i;
    int         has_libs   = 0;
    int         has_assets = 0;
    uint32_t    hash;

    /* Gera tmpdir determinístico baseado no hash do exe */
    hash = tupi_exe_hash(exe_path);
    snprintf(tmpdir,    sizeof(tmpdir),    "%s%08x", TUPI_TMP_PREFIX, hash);
    snprintf(lib_dir,   sizeof(lib_dir),   "%s/lib",    tmpdir);
    snprintf(asset_dir, sizeof(asset_dir), "%s/assets", tmpdir);

    /* Cria o tmpdir (pode já existir de uma execução anterior) */
    if (mkdir(tmpdir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "[Tupi] Não foi possível criar tmpdir '%s': %s\n",
                tmpdir, strerror(errno));
        return -1;
    }

    num_entries = zip_get_num_entries(za, 0);

    /* Extrai apenas libs e assets — scripts ficam no ZIP (lidos sob demanda) */
    for (i = 0; i < num_entries; i++) {
        const char    *name = zip_get_name(za, (zip_uint64_t)i, 0);
        unsigned char *data;
        size_t         data_size;
        char           dest[PATH_MAX * 2]; /* espaço para base + nome de entry */

        if (!name) continue;

        if (strncmp(name, TUPI_LIB_PREFIX, strlen(TUPI_LIB_PREFIX)) == 0) {
            const char *libname = name + strlen(TUPI_LIB_PREFIX);
            if (libname[0] == '\0') continue; /* entrada de diretório */
            snprintf(dest, sizeof(dest), "%s/%s", lib_dir, libname);

            /* Pula se já extraída (tmpdir reutilizado) */
            if (access(dest, F_OK) == 0) {
                has_libs = 1;
                continue;
            }

            data = tupi_zip_read_entry(za, i, &data_size);
            if (!data) {
                fprintf(stderr, "[Tupi] Falha ao ler lib '%s' do ZIP.\n", name);
                continue;
            }
            if (tupi_write_file(dest, data, data_size, 0) == 0) {
                has_libs = 1;
                fprintf(stderr, "[Tupi] Lib extraída: %s (%zu bytes)\n",
                        libname, data_size);
            } else {
                fprintf(stderr, "[Tupi] Falha ao escrever lib '%s'.\n", dest);
            }
            free(data);

        } else if (strncmp(name, TUPI_ASSETS_PREFIX,
                           strlen(TUPI_ASSETS_PREFIX)) == 0) {
            const char *rel = name + strlen(TUPI_ASSETS_PREFIX);
            if (rel[0] == '\0') continue;
            snprintf(dest, sizeof(dest), "%s/%s", asset_dir, rel);

            if (access(dest, F_OK) == 0) {
                has_assets = 1;
                continue;
            }

            data = tupi_zip_read_entry(za, i, &data_size);
            if (!data) {
                fprintf(stderr, "[Tupi] Falha ao ler asset '%s' do ZIP.\n", name);
                continue;
            }
            if (tupi_write_file(dest, data, data_size, 0) == 0) {
                has_assets = 1;
            }
            free(data);
        }
    }

    /* Configura LD_LIBRARY_PATH */
    if (has_libs) {
        const char *old_ld = getenv("LD_LIBRARY_PATH");
        char        new_ld[PATH_MAX * 2];
        if (old_ld && old_ld[0] != '\0')
            snprintf(new_ld, sizeof(new_ld), "%s:%s", lib_dir, old_ld);
        else
            snprintf(new_ld, sizeof(new_ld), "%s", lib_dir);
        setenv("LD_LIBRARY_PATH", new_ld, 1);
        fprintf(stderr, "[Tupi] Libs em: %s\n", lib_dir);
    }

    /* Configura TUPI_ASSET_DIR */
    if (has_assets) {
        setenv(TUPI_ASSET_DIR_ENV, asset_dir, 1);
        fprintf(stderr, "[Tupi] Assets em: %s\n", asset_dir);
    }

    /* Marca que a extração já foi feita — evita loop infinito */
    setenv(TUPI_LIBS_EXTRACTED_ENV, "1", 1);

    /* Re-executa O PRÓPRIO EXECUTÁVEL.
     * O dynamic linker vai achar as .so em LD_LIBRARY_PATH e
     * carregar SDL2/Vulkan/LuaJIT antes de chegar no main().
     * Na próxima entrada, TUPI_LIBS_EXTRACTED=1 e seguimos em frente. */
    fprintf(stderr, "[Tupi] Re-executando com libs carregadas...\n");
    execv(exe_path, argv);

    /* execv só retorna em erro */
    fprintf(stderr, "[Tupi] Falha ao re-executar '%s': %s\n",
            exe_path, strerror(errno));
    return -1;
}

/* -------------------------------------------------------------------------
 * Sidecar: procura game.tuzip ao lado do executável ou via env
 * ---------------------------------------------------------------------- */

static zip_t *tupi_open_sidecar_zip(void) {
    const char *candidates[3];
    const char *env_path = getenv(TUPI_SCRIPT_ARCHIVE_ENV);
    size_t      i;

    candidates[0] = env_path;
    candidates[1] = TUPI_LOCAL_SCRIPT_ARCHIVE;
    candidates[2] = TUPI_DEFAULT_SCRIPT_ARCHIVE;

    for (i = 0; i < 3u; i++) {
        if (!candidates[i] || candidates[i][0] == '\0') continue;
        if (access(candidates[i], R_OK) == 0)
            return tupi_open_zip_file(candidates[i]);
    }
    return NULL;
}

/* -------------------------------------------------------------------------
 * Fallback de assets do disco (modo export-linux / desenvolvimento)
 * ---------------------------------------------------------------------- */

static void tupi_configure_default_asset_dir(void) {
    if (getenv(TUPI_ASSET_DIR_ENV)) return;
    if (access(TUPI_DEFAULT_ASSET_DIR, R_OK) == 0) {
        setenv(TUPI_ASSET_DIR_ENV, TUPI_DEFAULT_ASSET_DIR, 0);
        return;
    }
    if (access(TUPI_LOCAL_ASSET_DIR, R_OK) == 0)
        setenv(TUPI_ASSET_DIR_ENV, TUPI_LOCAL_ASSET_DIR, 0);
}

/* -------------------------------------------------------------------------
 * Estrutura de archive em memória
 * ---------------------------------------------------------------------- */

typedef struct {
    zip_t *za;           /* handle libzip — manter aberto enquanto Lua roda */
} TupiArchive;

static void tupi_free_archive(TupiArchive *ar) {
    if (!ar) return;
    if (ar->za) { zip_close(ar->za); ar->za = NULL; }
}

/* -------------------------------------------------------------------------
 * Integração Lua: searcher que carrega módulos direto do ZIP (zero-copy)
 * ---------------------------------------------------------------------- */

static int tupi_embedded_searcher(lua_State *L) {
    const char   *module_name = luaL_checkstring(L, 1);
    TupiArchive  *ar          = (TupiArchive *)lua_touserdata(L, lua_upvalueindex(1));
    char          zip_path[512];
    zip_int64_t   idx;
    unsigned char *data;
    size_t         data_size;
    char          *chunk_name;
    int            status;

    /* "utils.camera" → "scripts/utils.camera.lua" */
    snprintf(zip_path, sizeof(zip_path), "%s%s.lua", TUPI_SCRIPTS_PREFIX,
             module_name);

    idx = zip_name_locate(ar->za, zip_path, 0);
    if (idx < 0) {
        snprintf(zip_path, sizeof(zip_path), "%s.lua", module_name);
        idx = zip_name_locate(ar->za, zip_path, 0);
    }
    if (idx < 0) {
        lua_pushfstring(L, "\n\tno embedded module '%s'", module_name);
        return 1;
    }

    data = tupi_zip_read_entry(ar->za, idx, &data_size);
    if (!data)
        return luaL_error(L, "[Tupi] Falha ao ler módulo '%s' do ZIP.", module_name);

    chunk_name = (char *)malloc(strlen(zip_path) + 2u);
    if (!chunk_name) { free(data); return luaL_error(L, "sem memória"); }
    chunk_name[0] = '@';
    memcpy(chunk_name + 1u, zip_path, strlen(zip_path) + 1u);

    status = luaL_loadbuffer(L, (const char *)data, data_size, chunk_name);
    free(chunk_name);
    free(data);

    if (status != 0) return lua_error(L);
    return 1;
}

static int tupi_install_searcher(lua_State *L, TupiArchive *ar) {
    size_t loader_count;

    lua_getglobal(L, "package");
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return -1; }

    lua_getfield(L, -1, "searchers");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_getfield(L, -1, "loaders"); /* LuaJIT 5.1 compat */
    }
    if (!lua_istable(L, -1)) { lua_pop(L, 2); return -1; }

    loader_count = (size_t)lua_objlen(L, -1);
    while (loader_count >= 2u) {
        lua_rawgeti(L, -1, (int)loader_count);
        lua_rawseti(L, -2, (int)loader_count + 1);
        --loader_count;
    }

    lua_pushlightuserdata(L, ar);
    lua_pushcclosure(L, tupi_embedded_searcher, 1);
    lua_rawseti(L, -2, 2);
    lua_pop(L, 2);
    return 0;
}

/* -------------------------------------------------------------------------
 * Popula tabela arg
 * ---------------------------------------------------------------------- */

static void tupi_push_arg_table(lua_State *L, int argc, char **argv) {
    int i;
    lua_newtable(L);
    for (i = 0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i);
    }
    lua_setglobal(L, "arg");
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(int argc, char **argv) {
    char        exe_path[PATH_MAX];
    TupiArchive archive;
    lua_State  *L;
    long        zip_offset;
    zip_int64_t main_idx;
    unsigned char *main_data = NULL;
    size_t         main_size = 0;
    int            exit_code = EXIT_FAILURE;

    memset(&archive, 0, sizeof(archive));

    /* --- Resolve o próprio executável --- */
    if (tupi_read_self_path(exe_path, sizeof(exe_path)) != 0) {
        fprintf(stderr, "[Tupi] Não foi possível resolver /proc/self/exe.\n");
        return EXIT_FAILURE;
    }
    tupi_chdir_to_exe_dir(exe_path);

    /* -----------------------------------------------------------------------
     * Abertura do ZIP
     *
     * Prioridade:
     *   1. Sidecar game.tuzip (modo export-linux / desenvolvimento)
     *   2. ZIP anexado ao próprio executável (dist-linux e bundle-linux)
     * -------------------------------------------------------------------- */

    archive.za = tupi_open_sidecar_zip();

    if (!archive.za) {
        zip_offset = tupi_read_appended_zip_offset(exe_path);
        if (zip_offset >= 0)
            archive.za = tupi_open_appended_zip(exe_path, zip_offset);
    }

    if (!archive.za) {
        fprintf(stderr, "[Tupi] Nenhum pacote ZIP encontrado "
                        "(embutido ou sidecar game.tuzip).\n");
        return EXIT_FAILURE;
    }

    /* -----------------------------------------------------------------------
     * Modo bundle (opção 4): ZIP contém "lib/" E ainda não extraímos.
     *
     * Nota: TUPI_LIBS_EXTRACTED é setado por NÓS MESMOS antes do execv(),
     * portanto na re-execução já encontramos as libs linkadas corretamente
     * e pulamos este bloco.
     * -------------------------------------------------------------------- */

    if (!getenv(TUPI_LIBS_EXTRACTED_ENV) &&
        tupi_zip_has_prefix(archive.za, TUPI_LIB_PREFIX))
    {
        /* tupi_self_extract_and_reexec() chama execv() e não retorna em
         * caso de sucesso. Retorna -1 apenas em falha. */
        if (tupi_self_extract_and_reexec(archive.za, exe_path, argv) != 0) {
            fprintf(stderr, "[Tupi] Falha na auto-extração de libs.\n");
            tupi_free_archive(&archive);
            return EXIT_FAILURE;
        }
        /* Nunca chegamos aqui em caso de sucesso */
    }

    /* -----------------------------------------------------------------------
     * Execução normal (modo dist-linux, export-linux, ou 2ª passagem bundle)
     * -------------------------------------------------------------------- */

    /* Configura diretório de assets (se não setado pelo estágio anterior) */
    if (!getenv(TUPI_ASSET_DIR_ENV))
        tupi_configure_default_asset_dir();

    /* Modo bundle (2ª passagem): muda CWD para o asset_dir extraído.
     * O engine Rust abre assets pelo path relativo que o Lua passa ("ascii.png")
     * sem consultar TUPI_ASSET_DIR. Ao mudar o CWD para o tmpdir de assets,
     * qualquer open("ascii.png") resolve para /tmp/tupi_<hash>/assets/ascii.png. */
    {
        const char *tupi_asset_dir = getenv(TUPI_ASSET_DIR_ENV);
        if (tupi_asset_dir && getenv(TUPI_LIBS_EXTRACTED_ENV)) {
            if (chdir(tupi_asset_dir) != 0) {
                fprintf(stderr, "[Tupi] Aviso: nao foi possivel mudar CWD para '%s': %s\n",
                        tupi_asset_dir, strerror(errno));
            }
        }
    }

    /* Localiza __main__ */
    main_idx = zip_name_locate(archive.za, TUPI_MAIN_ENTRY, 0);
    if (main_idx < 0) {
        fprintf(stderr, "[Tupi] Entrada '__main__' não encontrada no ZIP.\n");
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    main_data = tupi_zip_read_entry(archive.za, main_idx, &main_size);
    if (!main_data) {
        fprintf(stderr, "[Tupi] Falha ao ler '__main__' do ZIP.\n");
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    /* Inicializa Lua */
    L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "[Tupi] Não foi possível criar o estado Lua.\n");
        free(main_data);
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    luaL_openlibs(L);
    tupi_push_arg_table(L, argc, argv);

    lua_pushboolean(L, 1);
    lua_setglobal(L, "TUPI_STANDALONE");

    lua_pushstring(L, exe_path);
    lua_setglobal(L, "TUPI_EXECUTABLE_PATH");

    /* Informa ao Lua se estamos no modo bundle (libs extraídas) */
    lua_pushboolean(L, getenv(TUPI_LIBS_EXTRACTED_ENV) != NULL);
    lua_setglobal(L, "TUPI_BUNDLE_MODE");

    /* TUPI_ASSET_DIR para Lua */
    {
        const char *asset_dir = getenv(TUPI_ASSET_DIR_ENV);
        if (asset_dir)
            lua_pushstring(L, asset_dir);
        else
            lua_pushnil(L);
        lua_setglobal(L, "TUPI_ASSET_DIR");
    }

    /* Instala searcher para módulos embutidos no ZIP */
    if (tupi_install_searcher(L, &archive) != 0) {
        fprintf(stderr, "[Tupi] Falha ao instalar searcher Lua.\n");
        goto cleanup;
    }

    /* Carrega e executa __main__ */
    if (luaL_loadbuffer(L, (const char *)main_data, main_size, "@main.lua") != 0) {
        fprintf(stderr, "[Tupi] Falha ao carregar main.lua: %s\n",
                lua_tostring(L, -1));
        goto cleanup;
    }
    if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
        fprintf(stderr, "[Tupi] Erro ao executar main.lua: %s\n",
                lua_tostring(L, -1));
        goto cleanup;
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    lua_close(L);
    free(main_data);
    tupi_free_archive(&archive);
    return exit_code;
}