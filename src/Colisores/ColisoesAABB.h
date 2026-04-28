// ColisoesAABB.h — TupiEngine

#ifndef COLISOES_AABB_H
#define COLISOES_AABB_H

typedef struct { float x, y, largura, altura; } TupiRetCol;
typedef struct { float x, y, raio; }             TupiCircCol;
typedef struct { int colidindo; float dx, dy; }  TupiColisao;

// Retângulo vs Retângulo
int         tupi_ret_ret(TupiRetCol a, TupiRetCol b);
TupiColisao tupi_ret_ret_info(TupiRetCol a, TupiRetCol b);

// Círculo vs Círculo
int         tupi_cir_cir(TupiCircCol a, TupiCircCol b);
TupiColisao tupi_cir_cir_info(TupiCircCol a, TupiCircCol b);

// Retângulo vs Círculo
int tupi_ret_cir(TupiRetCol r, TupiCircCol c);

// Ponto vs Forma
int tupi_ponto_ret(float px, float py, TupiRetCol r);
int tupi_ponto_cir(float px, float py, TupiCircCol c);

// Construtores rápidos
static inline TupiRetCol  tupi_ret(float x, float y, float l, float a) { TupiRetCol  r = { x, y, l, a }; return r; }
static inline TupiCircCol tupi_cir(float x, float y, float raio)       { TupiCircCol c = { x, y, raio }; return c; }

#endif // COLISOES_AABB_H