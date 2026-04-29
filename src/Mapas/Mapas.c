// Mapas.c — TupiEngine · Tilemap

#include "Mapas.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

extern void tupi_objeto_enviar_batch_raw(
    unsigned int textura_id,
    const float* quad,
    float r, float g, float b, float a,
    int z_index
);

// --- Ciclo de vida ---

TupiMapa* tupi_mapa_criar(int colunas, int linhas) {
    if (colunas <= 0 || linhas <= 0)              return NULL;
    if (colunas * linhas > TUPI_MAPA_MAX_CELULAS) return NULL;

    TupiMapa* m = (TupiMapa*)calloc(1, sizeof(TupiMapa));
    if (!m) return NULL;

    m->celulas = (TupiTile*)calloc(colunas * linhas, sizeof(TupiTile));
    if (!m->celulas) { free(m); return NULL; }

    m->colunas  = colunas;
    m->linhas   = linhas;
    m->num_defs = 0;
    m->valido   = 0;
    return m;
}

void tupi_mapa_destruir(TupiMapa* mapa) {
    if (!mapa) return;
    free(mapa->celulas);
    free(mapa);
}

void tupi_mapa_set_atlas(TupiMapa* mapa, unsigned int textura_id,
                          int atlas_larg, int atlas_alt) {
    if (!mapa) return;
    mapa->textura_id = textura_id;
    mapa->atlas_larg = atlas_larg;
    mapa->atlas_alt  = atlas_alt;
}

int tupi_mapa_registrar_def(TupiMapa* mapa, const TupiTileDef* def) {
    if (!mapa || !def || mapa->num_defs >= TUPI_MAPA_MAX_DEFS) return -1;
    int id = mapa->num_defs;
    mapa->defs[id] = *def;
    mapa->num_defs++;
    return id;
}

void tupi_mapa_set_grade(TupiMapa* mapa, const uint8_t* ids, int n) {
    if (!mapa || !ids) return;
    int total = mapa->colunas * mapa->linhas;
    if (n < total) total = n;
    for (int i = 0; i < total; i++) {
        mapa->celulas[i].def_id = ids[i];
        mapa->celulas[i].tempo  = 0.0f;
        mapa->celulas[i].frame  = 0;
    }
}

// --- Animação ---

void tupi_mapa_atualizar(TupiMapa* mapa, float dt) {
    if (!mapa || !mapa->valido) return;

    int total = mapa->colunas * mapa->linhas;
    for (int i = 0; i < total; i++) {
        TupiTile* cel = &mapa->celulas[i];
        if (cel->def_id == 0) continue;

        int idx = cel->def_id - 1; // def_id é 1-based
        if (idx < 0 || idx >= mapa->num_defs) continue;

        TupiTileDef* def = &mapa->defs[idx];
        if (!(def->flags & TUPI_TILE_ANIMADO) || def->num_frames <= 1) continue;
        if (def->fps <= 0.0f) continue;

        cel->tempo += dt;
        float dur = 1.0f / def->fps;
        int   fi  = (int)(cel->tempo / dur);

        if (fi >= def->num_frames) {
            if (def->loop) {
                cel->tempo -= (float)def->num_frames * dur;
                if (cel->tempo < 0.0f) cel->tempo = 0.0f;
                cel->frame = 0;
            } else {
                cel->frame = def->num_frames - 1;
            }
        } else {
            cel->frame = fi;
        }
    }
}

// --- Renderização ---

static float _quad[16]; // buffer reutilizável por frame

// Envia um tile para o batch. UV com inset de meio texel (igual ao sprites.rs)
// para evitar bleeding quando a câmera tem posição sub-pixel.
static void _enviar_tile(unsigned int tex,
                          float px, float py, float pw, float ph,
                          float u0, float v0, float u1, float v1,
                          float alpha, int z) {
    _quad[0]  = px;      _quad[1]  = py;      _quad[2]  = u0; _quad[3]  = v0;
    _quad[4]  = px + pw; _quad[5]  = py;      _quad[6]  = u1; _quad[7]  = v0;
    _quad[8]  = px;      _quad[9]  = py + ph; _quad[10] = u0; _quad[11] = v1;
    _quad[12] = px + pw; _quad[13] = py + ph; _quad[14] = u1; _quad[15] = v1;
    tupi_objeto_enviar_batch_raw(tex, _quad, 1.0f, 1.0f, 1.0f, alpha, z);
}

void tupi_mapa_desenhar(TupiMapa* mapa, int z_base) {
    if (!mapa || !mapa->valido) return;

    float        aw  = (float)mapa->atlas_larg;
    float        ah  = (float)mapa->atlas_alt;
    unsigned int tex = mapa->textura_id;

    for (int lin = 0; lin < mapa->linhas; lin++) {
        for (int col = 0; col < mapa->colunas; col++) {
            TupiTile* cel = &mapa->celulas[lin * mapa->colunas + col];
            if (cel->def_id == 0) continue;

            int idx = cel->def_id - 1;
            if (idx < 0 || idx >= mapa->num_defs) continue;

            TupiTileDef*  def = &mapa->defs[idx];
            int           fi  = (cel->frame < def->num_frames) ? cel->frame : 0;
            TupiTileFrame* fr = &def->frames[fi];
            float escala = (def->escala > 0.0f) ? def->escala : 1.0f;

            // Pixel-snap: evita bleeding causado por posições sub-pixel da câmera
            float px = roundf((float)(col * def->largura));
            float py = roundf((float)(lin * def->altura));
            float cw = (float)def->largura * escala;
            float ch = (float)def->altura  * escala;
            float sw = (float)def->largura;
            float sh = (float)def->altura;

            // Inset de meio texel — igual ao sprites.rs — previne
            // que o sampler leia o pixel do tile vizinho na borda do UV.
            float tw = 0.5f / aw;
            float th = 0.5f / ah;
            float u0 = ((float)fr->coluna * sw) / aw + tw;
            float v0 = ((float)fr->linha  * sh) / ah + th;
            float u1 = u0 + sw / aw - 2.0f * tw;
            float v1 = v0 + sh / ah - 2.0f * th;

            _enviar_tile(tex, px, py, cw, ch, u0, v0, u1, v1,
                         def->alpha, z_base + def->z_index);
        }
    }
}

// --- Colisão ---

TupiTileHitbox tupi_mapa_hitbox_tile(const TupiMapa* mapa, int col, int lin) {
    TupiTileHitbox hb = {0, 0, 0, 0, TUPI_TILE_VAZIO};
    if (!mapa || col < 0 || lin < 0 || col >= mapa->colunas || lin >= mapa->linhas)
        return hb;

    TupiTile* cel = &mapa->celulas[lin * mapa->colunas + col];
    if (cel->def_id == 0) return hb;

    int idx = cel->def_id - 1;
    if (idx < 0 || idx >= mapa->num_defs) return hb;

    const TupiTileDef* def = &mapa->defs[idx];
    hb.x       = (float)(col * def->largura);
    hb.y       = (float)(lin * def->altura);
    hb.largura = (float)def->largura;
    hb.altura  = (float)def->altura;
    hb.flags   = def->flags;
    return hb;
}

int tupi_mapa_tile_em_ponto(const TupiMapa* mapa, float px, float py) {
    if (!mapa || mapa->num_defs == 0) return 0;

    // Usa o primeiro tile definido como referência de tamanho (grade uniforme)
    int larg = 0, alt = 0;
    for (int i = 0; i < mapa->num_defs; i++) {
        if (mapa->defs[i].largura > 0) {
            larg = mapa->defs[i].largura;
            alt  = mapa->defs[i].altura;
            break;
        }
    }
    if (larg == 0 || alt == 0) return 0;

    int col = (int)(px / (float)larg);
    int lin = (int)(py / (float)alt);
    if (col < 0 || lin < 0 || col >= mapa->colunas || lin >= mapa->linhas) return 0;

    TupiTile* cel = &mapa->celulas[lin * mapa->colunas + col];
    if (cel->def_id == 0) return 0;

    int idx = cel->def_id - 1;
    if (idx < 0 || idx >= mapa->num_defs) return 0;

    return mapa->defs[idx].numero;
}

int tupi_mapa_flags_tile(const TupiMapa* mapa, int col, int lin) {
    if (!mapa || col < 0 || lin < 0 || col >= mapa->colunas || lin >= mapa->linhas)
        return TUPI_TILE_VAZIO;

    TupiTile* cel = &mapa->celulas[lin * mapa->colunas + col];
    if (cel->def_id == 0) return TUPI_TILE_VAZIO;

    int idx = cel->def_id - 1;
    if (idx < 0 || idx >= mapa->num_defs) return TUPI_TILE_VAZIO;

    return (int)mapa->defs[idx].flags;
}

// --- Validação (delega ao Rust) ---

int tupi_mapa_validar(TupiMapa* mapa, TupiMapaValidacao* out) {
    if (!mapa || !out) {
        if (out) {
            out->ok = 0;
            out->num_erros = 1;
            snprintf(out->mensagem, sizeof(out->mensagem), "[Mapa] Ponteiro nulo.");
        }
        return 0;
    }

    int total = mapa->colunas * mapa->linhas;
    uint8_t* ids = (uint8_t*)malloc(total);
    if (!ids) {
        out->ok = 0;
        out->num_erros = 1;
        snprintf(out->mensagem, sizeof(out->mensagem), "[Mapa] Sem memoria para validacao.");
        return 0;
    }
    for (int i = 0; i < total; i++) ids[i] = mapa->celulas[i].def_id;

    int ok = tupi_mapa_validar_rust(
        mapa->colunas, mapa->linhas,
        mapa->num_defs,
        mapa->atlas_larg, mapa->atlas_alt,
        ids, total,
        out->mensagem, (int)sizeof(out->mensagem)
    );
    free(ids);

    out->ok        = ok;
    out->num_erros = ok ? 0 : 1;
    mapa->valido   = ok;
    return ok;
}