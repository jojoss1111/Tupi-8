/*
 * tupi_bootstrapper.c — Estágio 1 do bundle ZIP do TupiEngine
 *
 * Responsabilidades (idênticas ao anterior, formato diferente):
 *   1. Lê o ZIP anexado em si mesmo (marcador "TUPI_ZIP_APPENDED").
 *   2. Extrai para /tmp/tupi_XXXXXX/:
 *        engine/tupi_engine     ← binário real (__engine__)
 *        lib/<nome>.so          ← libs dinâmicas (prefixo "lib/")
 *        assets/...             ← assets do jogo (prefixo "assets/")
 *        game.tuzip             ← re-empacota scripts como ZIP standalone
 *   3. Define LD_LIBRARY_PATH, TUPI_ASSET_DIR e TUPI_SCRIPT_ARCHIVE.
 *   4. Re-executa o engine via execv().
 *   5. Limpa /tmp/tupi_XXXXXX em atexit/sinal.
 *
 * Compilação (depende apenas de libzip e libc):
 *   gcc -O2 -Wall -o .build/tupi_bootstrapper tupi_bootstrapper.c \
 *       $(pkg-config --cflags --libs libzip)
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <zip.h>

/* -------------------------------------------------------------------------
 * Constantes (espelham tupi_pack.rs e main_bytecode_loader.c)
 * ---------------------------------------------------------------------- */

#define TUPI_APPENDED_MARKER  "TUPI_ZIP_APPENDED"
#define TUPI_MARKER_LEN       17u
#define TUPI_TRAILER_SIZE     (TUPI_MARKER_LEN + 8u)

#define TUPI_MAIN_ENTRY       "__main__"
#define TUPI_ENGINE_ENTRY     "__engine__"
#define TUPI_SCRIPTS_PREFIX   "scripts/"
#define TUPI_LIB_PREFIX       "lib/"
#define TUPI_ASSETS_PREFIX    "assets/"

#define ENGINE_SUBDIR         "engine"
#define ENGINE_BIN_NAME       "tupi_engine"
#define SCRIPTS_SIDECAR_NAME  "game.tuzip"

/* -------------------------------------------------------------------------
 * Global: tmpdir para limpeza em atexit/sinal
 * ---------------------------------------------------------------------- */

static char g_tmpdir[64] = {0};

/* -------------------------------------------------------------------------
 * Utilitários
 * ---------------------------------------------------------------------- */

static uint64_t read_u64_le(const unsigned char *p) {
    return ((uint64_t)p[0])       |
           ((uint64_t)p[1] <<  8) |
           ((uint64_t)p[2] << 16) |
           ((uint64_t)p[3] << 24) |
           ((uint64_t)p[4] << 32) |
           ((uint64_t)p[5] << 40) |
           ((uint64_t)p[6] << 48) |
           ((uint64_t)p[7] << 56);
}

static int mkdirs_for_file(const char *filepath) {
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

static int write_file(const char *path, const unsigned char *data,
                      size_t size, int executable) {
    FILE *f;
    if (mkdirs_for_file(path) != 0) {
        fprintf(stderr, "[Bootstrap] Não foi possível criar diretórios para: %s\n", path);
        return -1;
    }
    f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[Bootstrap] Não foi possível abrir para escrita: %s (%s)\n",
                path, strerror(errno));
        return -1;
    }
    if (fwrite(data, 1u, size, f) != size) {
        fclose(f);
        fprintf(stderr, "[Bootstrap] Erro de escrita em: %s\n", path);
        return -1;
    }
    fclose(f);
    if (executable) chmod(path, 0755);
    return 0;
}

/* -------------------------------------------------------------------------
 * Leitura do offset do ZIP anexado
 * ---------------------------------------------------------------------- */

static long read_appended_zip_offset(const char *exe_path) {
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
 * Abre o ZIP anexado via libzip (fonte a partir de offset dentro do ELF)
 * ---------------------------------------------------------------------- */

static zip_t *open_appended_zip(const char *exe_path, long zip_offset) {
    zip_source_t *src;
    zip_t        *za;
    zip_error_t   ze;
    long           file_size;
    zip_uint64_t   zip_len;
    FILE          *f;

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
        fprintf(stderr, "[Bootstrap] Erro ao criar fonte ZIP: %s\n",
                zip_error_strerror(&ze));
        zip_error_fini(&ze);
        return NULL;
    }
    za = zip_open_from_source(src, ZIP_RDONLY, &ze);
    if (!za) {
        fprintf(stderr, "[Bootstrap] Erro ao abrir ZIP embutido: %s\n",
                zip_error_strerror(&ze));
        zip_source_free(src);
        zip_error_fini(&ze);
        return NULL;
    }
    zip_error_fini(&ze);
    return za;
}

/* -------------------------------------------------------------------------
 * Lê uma entrada do ZIP para um buffer heap
 * ---------------------------------------------------------------------- */

static unsigned char *zip_read_entry(zip_t *za, zip_int64_t idx, size_t *out_size) {
    zip_stat_t     st;
    zip_file_t    *zf;
    unsigned char *buf;
    zip_int64_t    n;

    if (zip_stat_index(za, (zip_uint64_t)idx, 0, &st) != 0) return NULL;
    if (!(st.valid & ZIP_STAT_SIZE)) return NULL;

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
 * Re-empacota apenas os scripts Lua como ZIP standalone (game.tuzip)
 *
 * O engine filho vai ler este arquivo via TUPI_SCRIPT_ARCHIVE.
 * Inclui: __main__ e qualquer entrada em scripts/
 * Exclui: __engine__, lib/, assets/
 * ---------------------------------------------------------------------- */

static int write_scripts_zip(const char *dest_path, zip_t *za) {
    zip_int64_t   num_entries;
    zip_int64_t   i;
    zip_source_t *src_out;
    zip_t        *zout;
    zip_error_t   ze;
    int           ret = 0;

    if (mkdirs_for_file(dest_path) != 0) return -1;

    zip_error_init(&ze);
    src_out = zip_source_file_create(dest_path, 0, 0, &ze);
    if (!src_out) {
        fprintf(stderr, "[Bootstrap] Não foi possível criar fonte para %s: %s\n",
                dest_path, zip_error_strerror(&ze));
        zip_error_fini(&ze);
        return -1;
    }

    /* Cria o ZIP de saída diretamente no arquivo */
    zout = zip_open(dest_path, ZIP_CREATE | ZIP_TRUNCATE, NULL);
    zip_source_free(src_out); /* não mais necessário após zip_open */
    if (!zout) {
        zout = zip_open(dest_path, ZIP_CREATE | ZIP_TRUNCATE, NULL);
    }
    /* Se ainda falhar, tenta via open direto */
    if (!zout) {
        int zerr = 0;
        zout = zip_open(dest_path, ZIP_CREATE | ZIP_TRUNCATE, &zerr);
        if (!zout) {
            fprintf(stderr, "[Bootstrap] Não foi possível criar %s\n", dest_path);
            zip_error_fini(&ze);
            return -1;
        }
    }
    zip_error_fini(&ze);

    num_entries = zip_get_num_entries(za, 0);
    for (i = 0; i < num_entries; ++i) {
        const char   *name = zip_get_name(za, (zip_uint64_t)i, 0);
        unsigned char *data;
        size_t         data_size;
        zip_source_t  *buf_src;

        if (!name) continue;
        /* Filtra: só __main__ e scripts/ */
        if (strcmp(name, TUPI_ENGINE_ENTRY) == 0) continue;
        if (strncmp(name, TUPI_LIB_PREFIX,    strlen(TUPI_LIB_PREFIX))    == 0) continue;
        if (strncmp(name, TUPI_ASSETS_PREFIX,  strlen(TUPI_ASSETS_PREFIX)) == 0) continue;

        data = zip_read_entry(za, i, &data_size);
        if (!data) {
            fprintf(stderr, "[Bootstrap] Falha ao ler script '%s'.\n", name);
            continue;
        }

        buf_src = zip_source_buffer(zout, data, data_size, 1 /* libzip free */);
        if (!buf_src) { free(data); continue; }

        if (zip_file_add(zout, name, buf_src, ZIP_FL_OVERWRITE) < 0) {
            fprintf(stderr, "[Bootstrap] Falha ao adicionar '%s' ao ZIP de scripts.\n", name);
            zip_source_free(buf_src);
        }
    }

    if (zip_close(zout) != 0) {
        fprintf(stderr, "[Bootstrap] Erro ao fechar ZIP de scripts.\n");
        ret = -1;
    }
    return ret;
}

/* -------------------------------------------------------------------------
 * Limpeza do tmpdir
 * ---------------------------------------------------------------------- */

static void cleanup_tmpdir(void) {
    char cmd[sizeof(g_tmpdir) + 16];
    if (g_tmpdir[0] == '\0') return;
    if (strncmp(g_tmpdir, "/tmp/tupi_", 10u) != 0) return;
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", g_tmpdir);
    (void)system(cmd);
    g_tmpdir[0] = '\0';
}

static void signal_handler(int sig) {
    cleanup_tmpdir();
    signal(sig, SIG_DFL);
    raise(sig);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(int argc, char **argv) {
    char        exe_path[PATH_MAX];
    ssize_t     exe_len;
    long        zip_offset;
    zip_t      *za = NULL;
    zip_int64_t num_entries;
    zip_int64_t i;
    char        tmp_template[64];

    char engine_path[PATH_MAX];
    char lib_dir[PATH_MAX];
    char assets_dir[PATH_MAX];
    char scripts_path[PATH_MAX];

    int found_engine = 0;
    int found_assets = 0;
    int found_libs   = 0;

    /* --- Resolve o próprio executável --- */
    exe_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1u);
    if (exe_len < 0 || (size_t)exe_len >= sizeof(exe_path)) {
        fprintf(stderr, "[Bootstrap] Não foi possível resolver /proc/self/exe.\n");
        return EXIT_FAILURE;
    }
    exe_path[exe_len] = '\0';

    /* --- Lê offset do ZIP anexado --- */
    zip_offset = read_appended_zip_offset(exe_path);
    if (zip_offset < 0) {
        fprintf(stderr, "[Bootstrap] Nenhum ZIP encontrado neste executável.\n");
        return EXIT_FAILURE;
    }

    /* --- Abre o ZIP --- */
    za = open_appended_zip(exe_path, zip_offset);
    if (!za) return EXIT_FAILURE;

    /* --- Verifica se há __engine__ --- */
    if (zip_name_locate(za, TUPI_ENGINE_ENTRY, 0) < 0) {
        fprintf(stderr, "[Bootstrap] Entrada '__engine__' não encontrada no ZIP.\n");
        zip_close(za);
        return EXIT_FAILURE;
    }
    found_engine = 1;

    /* Verifica se há libs e assets */
    num_entries = zip_get_num_entries(za, 0);
    for (i = 0; i < num_entries; ++i) {
        const char *name = zip_get_name(za, (zip_uint64_t)i, 0);
        if (!name) continue;
        if (strncmp(name, TUPI_ASSETS_PREFIX, strlen(TUPI_ASSETS_PREFIX)) == 0) found_assets = 1;
        if (strncmp(name, TUPI_LIB_PREFIX,    strlen(TUPI_LIB_PREFIX))    == 0) found_libs   = 1;
    }

    /* --- Cria tmpdir --- */
    strncpy(tmp_template, "/tmp/tupi_XXXXXX", sizeof(tmp_template) - 1u);
    if (!mkdtemp(tmp_template)) {
        fprintf(stderr, "[Bootstrap] Não foi possível criar tmpdir: %s\n", strerror(errno));
        zip_close(za);
        return EXIT_FAILURE;
    }
    strncpy(g_tmpdir, tmp_template, sizeof(g_tmpdir) - 1u);

    atexit(cleanup_tmpdir);
    signal(SIGTERM, signal_handler);
    signal(SIGINT,  signal_handler);
    signal(SIGHUP,  signal_handler);

    /* Monta caminhos */
    snprintf(engine_path,  sizeof(engine_path),  "%s/%s/%s",
             g_tmpdir, ENGINE_SUBDIR, ENGINE_BIN_NAME);
    snprintf(lib_dir,      sizeof(lib_dir),       "%s/%s", g_tmpdir, "lib");
    snprintf(assets_dir,   sizeof(assets_dir),    "%s/%s", g_tmpdir, "assets");
    snprintf(scripts_path, sizeof(scripts_path),  "%s/%s", g_tmpdir, SCRIPTS_SIDECAR_NAME);

    /* --- Extrai entradas do ZIP --- */
    for (i = 0; i < num_entries; ++i) {
        const char    *name = zip_get_name(za, (zip_uint64_t)i, 0);
        unsigned char *data;
        size_t         data_size;
        char           dest[PATH_MAX];

        if (!name) continue;

        if (strcmp(name, TUPI_ENGINE_ENTRY) == 0) {
            /* __engine__ → engine/tupi_engine */
            snprintf(dest, sizeof(dest), "%s", engine_path);
            data = zip_read_entry(za, i, &data_size);
            if (!data || write_file(dest, data, data_size, 1) != 0) {
                fprintf(stderr, "[Bootstrap] Falha ao extrair engine.\n");
                free(data);
                zip_close(za);
                return EXIT_FAILURE;
            }
            fprintf(stderr, "[Bootstrap] Engine extraído: %s (%zu bytes)\n",
                    dest, data_size);
            free(data);
            continue;
        }

        if (strncmp(name, TUPI_LIB_PREFIX, strlen(TUPI_LIB_PREFIX)) == 0) {
            /* lib/<nome>.so → lib/<nome>.so */
            const char *libname = name + strlen(TUPI_LIB_PREFIX);
            snprintf(dest, sizeof(dest), "%s/%s", lib_dir, libname);
            data = zip_read_entry(za, i, &data_size);
            if (data) {
                if (write_file(dest, data, data_size, 0) == 0)
                    fprintf(stderr, "[Bootstrap] Lib extraída: %s (%zu bytes)\n",
                            libname, data_size);
                free(data);
            }
            continue;
        }

        if (strncmp(name, TUPI_ASSETS_PREFIX, strlen(TUPI_ASSETS_PREFIX)) == 0) {
            /* assets/<rel> → <tmpdir>/assets/<rel> */
            snprintf(dest, sizeof(dest), "%s/%s", g_tmpdir, name);
            data = zip_read_entry(za, i, &data_size);
            if (data) {
                if (write_file(dest, data, data_size, 0) == 0)
                    found_assets = 1;
                free(data);
            }
            continue;
        }

        /* Scripts (__main__ e scripts/*) → tratados por write_scripts_zip */
    }

    /* --- Gera game.tuzip com os scripts --- */
    if (write_scripts_zip(scripts_path, za) != 0) {
        fprintf(stderr, "[Bootstrap] Falha ao gerar %s.\n", scripts_path);
        zip_close(za);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[Bootstrap] Scripts em: %s\n", scripts_path);

    zip_close(za);
    za = NULL;

    /* --- Variáveis de ambiente para o engine --- */

    /* LD_LIBRARY_PATH */
    if (found_libs) {
        const char *old_ld = getenv("LD_LIBRARY_PATH");
        char        new_ld[PATH_MAX * 2];
        if (old_ld && old_ld[0] != '\0')
            snprintf(new_ld, sizeof(new_ld), "%s:%s", lib_dir, old_ld);
        else
            snprintf(new_ld, sizeof(new_ld), "%s", lib_dir);
        setenv("LD_LIBRARY_PATH", new_ld, 1);
    }

    /* TUPI_SCRIPT_ARCHIVE */
    setenv("TUPI_SCRIPT_ARCHIVE", scripts_path, 1);

    /* TUPI_ASSET_DIR */
    if (found_assets) {
        char candidate[PATH_MAX];
        snprintf(candidate, sizeof(candidate), "%s/assets", g_tmpdir);
        if (access(candidate, R_OK) == 0)
            setenv("TUPI_ASSET_DIR", candidate, 1);
        else
            setenv("TUPI_ASSET_DIR", assets_dir, 1);
        fprintf(stderr, "[Bootstrap] Assets em: %s\n", getenv("TUPI_ASSET_DIR"));
    }

    fprintf(stderr, "[Bootstrap] Iniciando engine: %s\n", engine_path);
    execv(engine_path, argv);

    fprintf(stderr, "[Bootstrap] Falha ao executar engine '%s': %s\n",
            engine_path, strerror(errno));
    return EXIT_FAILURE;
}