// ColisoesAABB.c — TupiEngine

#include "ColisoesAABB.h"
#include <math.h>

int tupi_ret_ret(TupiRetCol a, TupiRetCol b) {
    if (a.x + a.largura <= b.x) return 0;
    if (b.x + b.largura <= a.x) return 0;
    if (a.y + a.altura  <= b.y) return 0;
    if (b.y + b.altura  <= a.y) return 0;
    return 1;
}

TupiColisao tupi_ret_ret_info(TupiRetCol a, TupiRetCol b) {
    TupiColisao r = { 0, 0.0f, 0.0f };
    if (!tupi_ret_ret(a, b)) return r;

    float esq   = (b.x + b.largura) - a.x;
    float dir   = (a.x + a.largura) - b.x;
    float cima  = (b.y + b.altura)  - a.y;
    float baixo = (a.y + a.altura)  - b.y;
    float mx    = (esq < dir)    ? esq  : dir;
    float my    = (cima < baixo) ? cima : baixo;

    r.colidindo = 1;
    if (mx < my) r.dx = (esq < dir)    ? -esq  : dir;
    else         r.dy = (cima < baixo) ? -cima : baixo;
    return r;
}

int tupi_cir_cir(TupiCircCol a, TupiCircCol b) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float soma = a.raio + b.raio;
    return dx*dx + dy*dy <= soma * soma;
}

TupiColisao tupi_cir_cir_info(TupiCircCol a, TupiCircCol b) {
    TupiColisao r = { 0, 0.0f, 0.0f };
    float dx = b.x - a.x, dy = b.y - a.y;
    float dist = sqrtf(dx*dx + dy*dy);
    float soma = a.raio + b.raio;
    if (dist >= soma) return r;

    r.colidindo = 1;
    if (dist == 0.0f) { r.dx = soma; return r; }

    float pen = soma - dist;
    r.dx = -(dx / dist) * pen;
    r.dy = -(dy / dist) * pen;
    return r;
}

int tupi_ret_cir(TupiRetCol r, TupiCircCol c) {
    float px = c.x, py = c.y;
    if (px < r.x)             px = r.x;
    if (px > r.x + r.largura) px = r.x + r.largura;
    if (py < r.y)             py = r.y;
    if (py > r.y + r.altura)  py = r.y + r.altura;
    float dx = c.x - px, dy = c.y - py;
    return dx*dx + dy*dy <= c.raio * c.raio;
}

int tupi_ponto_ret(float px, float py, TupiRetCol r) {
    return px >= r.x && px <= r.x + r.largura &&
           py >= r.y && py <= r.y + r.altura;
}

int tupi_ponto_cir(float px, float py, TupiCircCol c) {
    float dx = px - c.x, dy = py - c.y;
    return dx*dx + dy*dy <= c.raio * c.raio;
}