// Mapas.h — TupiEngine · Tilemap
//
// Cada mapa tem: um atlas (spritesheet), uma grade de tiles e
// definições de tile (animação + colisão). A validação é feita
// em Rust antes do primeiro draw.

#ifndef TUPI_MAPAS_H
#define TUPI_MAPAS_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Limites
// ---------------------------------------------------------------------------

#define TUPI_MAPA_MAX_DEFS    256        // tipos de tile por mapa
#define TUPI_MAPA_MAX_FRAMES   32        // frames de animação por tile
#define TUPI_MAPA_MAX_CELULAS  (256*256) // 65 536 tiles por mapa

// ---------------------------------------------------------------------------
// Flags de tile (combináveis com |)
// ---------------------------------------------------------------------------

#define TUPI_TILE_VAZIO    0x00  // transparente, sem colisão
#define TUPI_TILE_SOLIDO   0x01  // bloqueia movimento
#define TUPI_TILE_PASSAGEM 0x02  // plataforma one-way
#define TUPI_TILE_TRIGGER  0x04  // zona de gatilho, sem colisão física
#define TUPI_TILE_ANIMADO  0x08  // tile tem múltiplos frames

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------

// Um frame de animação: posição (coluna, linha) no atlas
typedef struct {
    int coluna;
    int linha;
} TupiTileFrame;

// Definição de um tipo de tile
typedef struct {
    int           numero;
    uint8_t       flags;
    int           largura;
    int           altura;
    int           num_frames;
    TupiTileFrame frames[TUPI_MAPA_MAX_FRAMES];
    float         fps;
    int           loop;
    float         alpha;
    float         escala;
    int           z_index;
} TupiTileDef;

// Uma célula da grade
typedef struct {
    uint8_t def_id;   // 0 = vazio; > 0 aponta para defs[] (1-based)
    uint8_t _pad[3];
    float   tempo;    // acumulador de tempo da animação (gerenciado pelo C)
    int     frame;    // frame atual (gerenciado pelo C)
} TupiTile;

// Mapa completo
typedef struct {
    // Atlas
    unsigned int textura_id;
    int          atlas_larg;
    int          atlas_alt;

    // Grade
    int       colunas;
    int       linhas;
    TupiTile *celulas;

    // Definições de tile
    int         num_defs;
    TupiTileDef defs[TUPI_MAPA_MAX_DEFS];

    int valido;  // 1 após passar pela validação Rust
} TupiMapa;

// Resultado de validação
typedef struct {
    int  ok;
    int  num_erros;
    char mensagem[512];
} TupiMapaValidacao;

// Hitbox de um tile no espaço do mundo
typedef struct {
    float x, y, largura, altura;
    int   flags;
} TupiTileHitbox;

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------

TupiMapa *tupi_mapa_criar       (int colunas, int linhas);
void      tupi_mapa_destruir    (TupiMapa *mapa);
void      tupi_mapa_set_atlas   (TupiMapa *mapa, unsigned int textura_id, int atlas_larg, int atlas_alt);
int       tupi_mapa_registrar_def(TupiMapa *mapa, const TupiTileDef *def);
void      tupi_mapa_set_grade   (TupiMapa *mapa, const uint8_t *ids, int n);

// ---------------------------------------------------------------------------
// Renderização & atualização
// ---------------------------------------------------------------------------

void tupi_mapa_atualizar(TupiMapa *mapa, float dt);
void tupi_mapa_desenhar (TupiMapa *mapa, int z_base);

// ---------------------------------------------------------------------------
// Colisão
// ---------------------------------------------------------------------------

TupiTileHitbox tupi_mapa_hitbox_tile  (const TupiMapa *mapa, int col, int lin);
int            tupi_mapa_tile_em_ponto(const TupiMapa *mapa, float px, float py);
int            tupi_mapa_flags_tile   (const TupiMapa *mapa, int col, int lin);

// ---------------------------------------------------------------------------
// Validação (chama o Rust internamente)
// ---------------------------------------------------------------------------

int tupi_mapa_validar(TupiMapa *mapa, TupiMapaValidacao *out);

// Implementação Rust — linkada via tupi_seguro
extern int tupi_mapa_validar_rust(
    int colunas, int linhas,
    int num_defs,
    int atlas_larg, int atlas_alt,
    const uint8_t *ids, int num_ids,
    char *msg_out, int msg_cap
);

#endif // TUPI_MAPAS_H
