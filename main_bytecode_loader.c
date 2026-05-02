#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define TUPI_FOOTER_MAGIC "TUPI_SLEDGE_BIN1"
#define TUPI_PAYLOAD_MAGIC "TUPI_LUA_PACK_V1"
#define TUPI_MAIN_ENTRY "__main__"
#define TUPI_SCRIPT_ARCHIVE_ENV "TUPI_SCRIPT_ARCHIVE"
#define TUPI_ASSET_DIR_ENV "TUPI_ASSET_DIR"
#define TUPI_DEFAULT_SCRIPT_ARCHIVE "../scripts/game.tupack"
#define TUPI_LOCAL_SCRIPT_ARCHIVE "./game.tupack"
#define TUPI_DEFAULT_ASSET_DIR "../assets"
#define TUPI_LOCAL_ASSET_DIR "./assets"

#define TUPI_MAGIC_SIZE 16u
#define TUPI_FOOTER_SIZE (TUPI_MAGIC_SIZE + 8u)

typedef struct {
    char* name;
    const unsigned char* data;
    size_t size;
} TupiLuaEntry;

typedef struct {
    unsigned char* payload_owner;
    size_t entry_count;
    TupiLuaEntry* entries;
    const unsigned char* main_data;
    size_t main_size;
} TupiLuaArchive;

static uint32_t tupi_read_u32_le(const unsigned char* ptr) {
    return ((uint32_t)ptr[0]) |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

static uint64_t tupi_read_u64_le(const unsigned char* ptr) {
    return ((uint64_t)ptr[0]) |
           ((uint64_t)ptr[1] << 8) |
           ((uint64_t)ptr[2] << 16) |
           ((uint64_t)ptr[3] << 24) |
           ((uint64_t)ptr[4] << 32) |
           ((uint64_t)ptr[5] << 40) |
           ((uint64_t)ptr[6] << 48) |
           ((uint64_t)ptr[7] << 56);
}

static int tupi_read_self_path(char* out_path, size_t out_size) {
    ssize_t len = readlink("/proc/self/exe", out_path, out_size - 1u);
    if (len < 0 || (size_t)len >= out_size) {
        return -1;
    }

    out_path[len] = '\0';
    return 0;
}

static void tupi_chdir_to_executable_dir(const char* exe_path) {
    char dir_path[PATH_MAX];
    char* slash = NULL;

    strncpy(dir_path, exe_path, sizeof(dir_path) - 1u);
    dir_path[sizeof(dir_path) - 1u] = '\0';

    slash = strrchr(dir_path, '/');
    if (slash == NULL) {
        return;
    }

    *slash = '\0';
    if (dir_path[0] == '\0') {
        return;
    }

    (void)chdir(dir_path);
}

static unsigned char* tupi_read_embedded_payload(const char* exe_path, size_t* out_size) {
    FILE* file = NULL;
    unsigned char footer[TUPI_FOOTER_SIZE];
    unsigned char* payload = NULL;
    long file_size = 0;
    long payload_offset = 0;
    uint64_t payload_size = 0;

    file = fopen(exe_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "[Tupi] Nao foi possivel abrir '%s': %s\n", exe_path, strerror(errno));
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        fprintf(stderr, "[Tupi] Nao foi possivel posicionar no fim do executavel.\n");
        fclose(file);
        return NULL;
    }

    file_size = ftell(file);
    if (file_size < 0 || (unsigned long)file_size < TUPI_FOOTER_SIZE) {
        fprintf(stderr, "[Tupi] Executavel sem footer de sledging.\n");
        fclose(file);
        return NULL;
    }

    if (fseek(file, file_size - (long)TUPI_FOOTER_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "[Tupi] Nao foi possivel ler o footer do executavel.\n");
        fclose(file);
        return NULL;
    }

    if (fread(footer, 1u, sizeof(footer), file) != sizeof(footer)) {
        fprintf(stderr, "[Tupi] Falha ao ler o footer do executavel.\n");
        fclose(file);
        return NULL;
    }

    if (memcmp(footer, TUPI_FOOTER_MAGIC, TUPI_MAGIC_SIZE) != 0) {
        fprintf(stderr, "[Tupi] Payload Lua embutido nao encontrado neste executavel.\n");
        fclose(file);
        return NULL;
    }

    payload_size = tupi_read_u64_le(footer + TUPI_MAGIC_SIZE);
    if (payload_size == 0 || payload_size > (uint64_t)file_size) {
        fprintf(stderr, "[Tupi] Footer de sledging invalido.\n");
        fclose(file);
        return NULL;
    }

    payload_offset = file_size - (long)TUPI_FOOTER_SIZE - (long)payload_size;
    if (payload_offset < 0) {
        fprintf(stderr, "[Tupi] Offset do payload ficou negativo.\n");
        fclose(file);
        return NULL;
    }

    payload = (unsigned char*)malloc((size_t)payload_size);
    if (payload == NULL) {
        fprintf(stderr, "[Tupi] Memoria insuficiente para ler o payload Lua.\n");
        fclose(file);
        return NULL;
    }

    if (fseek(file, payload_offset, SEEK_SET) != 0) {
        fprintf(stderr, "[Tupi] Nao foi possivel posicionar no payload Lua.\n");
        free(payload);
        fclose(file);
        return NULL;
    }

    if (fread(payload, 1u, (size_t)payload_size, file) != (size_t)payload_size) {
        fprintf(stderr, "[Tupi] Falha ao ler o payload Lua embutido.\n");
        free(payload);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *out_size = (size_t)payload_size;
    return payload;
}

static unsigned char* tupi_read_file_payload(const char* path, size_t* out_size) {
    FILE* file = NULL;
    unsigned char* payload = NULL;
    long file_size = 0;

    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    file_size = ftell(file);
    if (file_size <= 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    payload = (unsigned char*)malloc((size_t)file_size);
    if (payload == NULL) {
        fclose(file);
        return NULL;
    }

    if (fread(payload, 1u, (size_t)file_size, file) != (size_t)file_size) {
        free(payload);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *out_size = (size_t)file_size;
    return payload;
}

static unsigned char* tupi_read_sidecar_payload(size_t* out_size) {
    const char* env_path = getenv(TUPI_SCRIPT_ARCHIVE_ENV);
    const char* candidates[3];
    size_t i = 0;

    candidates[0] = env_path;
    candidates[1] = TUPI_LOCAL_SCRIPT_ARCHIVE;
    candidates[2] = TUPI_DEFAULT_SCRIPT_ARCHIVE;

    for (i = 0; i < 3u; ++i) {
        if (candidates[i] == NULL || candidates[i][0] == '\0') {
            continue;
        }

        if (access(candidates[i], R_OK) == 0) {
            unsigned char* payload = tupi_read_file_payload(candidates[i], out_size);
            if (payload != NULL) {
                return payload;
            }
        }
    }

    return NULL;
}

static void tupi_configure_default_asset_dir(void) {
    if (getenv(TUPI_ASSET_DIR_ENV) != NULL) {
        return;
    }

    if (access(TUPI_DEFAULT_ASSET_DIR, R_OK) == 0) {
        (void)setenv(TUPI_ASSET_DIR_ENV, TUPI_DEFAULT_ASSET_DIR, 0);
        return;
    }

    if (access(TUPI_LOCAL_ASSET_DIR, R_OK) == 0) {
        (void)setenv(TUPI_ASSET_DIR_ENV, TUPI_LOCAL_ASSET_DIR, 0);
    }
}

static void tupi_free_archive(TupiLuaArchive* archive) {
    size_t i = 0;

    if (archive == NULL) {
        return;
    }

    for (i = 0; i < archive->entry_count; ++i) {
        free(archive->entries[i].name);
    }

    free(archive->entries);
    free(archive->payload_owner);

    archive->entries = NULL;
    archive->payload_owner = NULL;
    archive->entry_count = 0;
    archive->main_data = NULL;
    archive->main_size = 0;
}

static int tupi_parse_payload(unsigned char* payload, size_t payload_size, TupiLuaArchive* out_archive) {
    const unsigned char* cursor = payload;
    const unsigned char* end = payload + payload_size;
    uint32_t entry_count = 0;
    size_t i = 0;

    memset(out_archive, 0, sizeof(*out_archive));
    out_archive->payload_owner = payload;

    if (payload_size < TUPI_MAGIC_SIZE + 4u) {
        fprintf(stderr, "[Tupi] Payload Lua pequeno demais.\n");
        return -1;
    }

    if (memcmp(cursor, TUPI_PAYLOAD_MAGIC, TUPI_MAGIC_SIZE) != 0) {
        fprintf(stderr, "[Tupi] Header do payload Lua invalido.\n");
        return -1;
    }

    cursor += TUPI_MAGIC_SIZE;
    entry_count = tupi_read_u32_le(cursor);
    cursor += 4u;

    out_archive->entries = (TupiLuaEntry*)calloc((size_t)entry_count, sizeof(TupiLuaEntry));
    if (entry_count > 0 && out_archive->entries == NULL) {
        fprintf(stderr, "[Tupi] Nao foi possivel alocar a tabela do payload.\n");
        return -1;
    }

    out_archive->payload_owner = payload;
    out_archive->entry_count = (size_t)entry_count;

    for (i = 0; i < out_archive->entry_count; ++i) {
        uint32_t name_len = 0;
        uint64_t data_len = 0;
        char* name = NULL;

        if ((size_t)(end - cursor) < 12u) {
            fprintf(stderr, "[Tupi] Payload Lua truncado ao ler cabecalho da entrada %zu.\n", i);
            return -1;
        }

        name_len = tupi_read_u32_le(cursor);
        cursor += 4u;
        data_len = tupi_read_u64_le(cursor);
        cursor += 8u;

        if ((uint64_t)(end - cursor) < (uint64_t)name_len + data_len) {
            fprintf(stderr, "[Tupi] Payload Lua truncado ao ler entrada %zu.\n", i);
            return -1;
        }

        name = (char*)malloc((size_t)name_len + 1u);
        if (name == NULL) {
            fprintf(stderr, "[Tupi] Memoria insuficiente para nome do modulo %zu.\n", i);
            return -1;
        }

        memcpy(name, cursor, (size_t)name_len);
        name[name_len] = '\0';
        cursor += name_len;

        out_archive->entries[i].name = name;
        out_archive->entries[i].data = cursor;
        out_archive->entries[i].size = (size_t)data_len;

        if (strcmp(name, TUPI_MAIN_ENTRY) == 0) {
            out_archive->main_data = cursor;
            out_archive->main_size = (size_t)data_len;
        }

        cursor += (size_t)data_len;
    }

    if (out_archive->main_data == NULL) {
        fprintf(stderr, "[Tupi] Nenhum chunk principal '%s' foi encontrado.\n", TUPI_MAIN_ENTRY);
        return -1;
    }

    return 0;
}

static void tupi_module_chunk_name(const char* module_name, char** out_chunk_name) {
    size_t module_len = strlen(module_name);
    char* chunk_name = (char*)malloc(module_len + 7u);
    size_t i = 0;

    if (chunk_name == NULL) {
        *out_chunk_name = NULL;
        return;
    }

    chunk_name[0] = '@';
    for (i = 0; i < module_len; ++i) {
        chunk_name[i + 1u] = (module_name[i] == '.') ? '/' : module_name[i];
    }
    memcpy(chunk_name + module_len + 1u, ".lua", 5u);
    *out_chunk_name = chunk_name;
}

static int tupi_embedded_searcher(lua_State* L) {
    size_t i = 0;
    const char* module_name = luaL_checkstring(L, 1);
    TupiLuaArchive* archive = (TupiLuaArchive*)lua_touserdata(L, lua_upvalueindex(1));

    for (i = 0; i < archive->entry_count; ++i) {
        char* chunk_name = NULL;
        int status = 0;

        if (strcmp(archive->entries[i].name, module_name) != 0) {
            continue;
        }

        tupi_module_chunk_name(module_name, &chunk_name);
        if (chunk_name == NULL) {
            return luaL_error(L, "[Tupi] Memoria insuficiente ao preparar chunk '%s'.", module_name);
        }

        status = luaL_loadbuffer(
            L,
            (const char*)archive->entries[i].data,
            archive->entries[i].size,
            chunk_name
        );
        free(chunk_name);

        if (status != 0) {
            return lua_error(L);
        }

        return 1;
    }

    lua_pushfstring(L, "\n\tno embedded module '%s'", module_name);
    return 1;
}

static int tupi_install_embedded_searcher(lua_State* L, TupiLuaArchive* archive) {
    size_t loader_count = 0;

    lua_getglobal(L, "package");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        fprintf(stderr, "[Tupi] package nao esta disponivel no estado Lua.\n");
        return -1;
    }

    lua_getfield(L, -1, "searchers");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_getfield(L, -1, "loaders");
    }

    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        fprintf(stderr, "[Tupi] package.loaders/searchers nao foi encontrado.\n");
        return -1;
    }

    loader_count = (size_t)lua_objlen(L, -1);
    while (loader_count >= 2u) {
        lua_rawgeti(L, -1, (int)loader_count);
        lua_rawseti(L, -2, (int)loader_count + 1);
        --loader_count;
    }

    lua_pushlightuserdata(L, archive);
    lua_pushcclosure(L, tupi_embedded_searcher, 1);
    lua_rawseti(L, -2, 2);

    lua_pop(L, 2);
    return 0;
}

static void tupi_push_arg_table(lua_State* L, int argc, char** argv) {
    int i = 0;

    lua_newtable(L);
    for (i = 0; i < argc; ++i) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i);
    }
    lua_setglobal(L, "arg");
}

int main(int argc, char** argv) {
    char exe_path[PATH_MAX];
    size_t payload_size = 0;
    unsigned char* payload = NULL;
    TupiLuaArchive archive;
    lua_State* L = NULL;
    int exit_code = EXIT_FAILURE;

    memset(&archive, 0, sizeof(archive));

    if (tupi_read_self_path(exe_path, sizeof(exe_path)) != 0) {
        fprintf(stderr, "[Tupi] Nao foi possivel resolver /proc/self/exe.\n");
        return EXIT_FAILURE;
    }

    tupi_chdir_to_executable_dir(exe_path);
    tupi_configure_default_asset_dir();

    payload = tupi_read_sidecar_payload(&payload_size);
    if (payload == NULL) {
        payload = tupi_read_embedded_payload(exe_path, &payload_size);
    }

    if (payload == NULL) {
        fprintf(stderr, "[Tupi] Nenhum pacote Lua foi encontrado nem ao lado do executavel nem embutido.\n");
        return EXIT_FAILURE;
    }

    if (tupi_parse_payload(payload, payload_size, &archive) != 0) {
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    L = luaL_newstate();
    if (L == NULL) {
        fprintf(stderr, "[Tupi] Nao foi possivel criar o estado Lua.\n");
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    luaL_openlibs(L);
    tupi_push_arg_table(L, argc, argv);

    lua_pushboolean(L, 1);
    lua_setglobal(L, "TUPI_STANDALONE");

    lua_pushstring(L, exe_path);
    lua_setglobal(L, "TUPI_EXECUTABLE_PATH");

    if (tupi_install_embedded_searcher(L, &archive) != 0) {
        lua_close(L);
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    if (luaL_loadbuffer(L, (const char*)archive.main_data, archive.main_size, "@main.lua") != 0) {
        fprintf(stderr, "[Tupi] Falha ao carregar main.lua do pacote: %s\n", lua_tostring(L, -1));
        lua_close(L);
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
        fprintf(stderr, "[Tupi] Erro ao executar main.lua do pacote: %s\n", lua_tostring(L, -1));
        lua_close(L);
        tupi_free_archive(&archive);
        return EXIT_FAILURE;
    }

    exit_code = EXIT_SUCCESS;
    lua_close(L);
    tupi_free_archive(&archive);
    return exit_code;
}
