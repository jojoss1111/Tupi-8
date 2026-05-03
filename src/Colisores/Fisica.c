// Fisica.c — TupiEngine

#include "Fisica.h"
#include <math.h>

void tupi_fisica_atualizar(TupiCorpo* c, float dt, float gravidade) {
    if (c->massa == 0.0f) return;
    c->aceleracaoY += gravidade;
    c->velX += c->aceleracaoX * dt;
    c->velY += c->aceleracaoY * dt;
    c->x += c->velX * dt;
    c->y += c->velY * dt;
    c->aceleracaoX = 0.0f;
    c->aceleracaoY = 0.0f;
}

void tupi_fisica_impulso(TupiCorpo* c, float fx, float fy) {
    if (c->massa == 0.0f) return;
    c->velX += fx / c->massa;
    c->velY += fy / c->massa;
}

TupiRetCol tupi_corpo_ret(TupiCorpo* c, float largura, float altura) {
    return tupi_ret(c->x, c->y, largura, altura);
}

TupiCircCol tupi_corpo_cir(TupiCorpo* c, float raio) {
    return tupi_cir(c->x, c->y, raio);
}

void tupi_resolver_colisao(TupiCorpo* a, TupiCorpo* b, TupiColisao info) {
    if (!info.colidindo) return;

    float total  = a->massa + b->massa;
    float ratioA = (b->massa > 0.0f) ? b->massa / total : 1.0f;
    float ratioB = (a->massa > 0.0f) ? a->massa / total : 1.0f;

    if (a->massa > 0.0f) { a->x += info.dx * ratioA; a->y += info.dy * ratioA; }
    if (b->massa > 0.0f) { b->x -= info.dx * ratioB; b->y -= info.dy * ratioB; }

    float e = (a->elasticidade + b->elasticidade) * 0.5f;

    if (info.dy != 0.0f) {
        float imp = -(1.0f + e) * (a->velY - b->velY) / (1.0f/a->massa + 1.0f/b->massa);
        if (a->massa > 0.0f) a->velY += imp / a->massa;
        if (b->massa > 0.0f) b->velY -= imp / b->massa;
    }
    if (info.dx != 0.0f) {
        float imp = -(1.0f + e) * (a->velX - b->velX) / (1.0f/a->massa + 1.0f/b->massa);
        if (a->massa > 0.0f) a->velX += imp / a->massa;
        if (b->massa > 0.0f) b->velX -= imp / b->massa;
    }
}

void tupi_resolver_estatico(TupiCorpo* a, TupiColisao info) {
    if (!info.colidindo || a->massa == 0.0f) return;
    a->x += info.dx;
    a->y += info.dy;
    if (info.dy != 0.0f) a->velY = -a->velY * a->elasticidade;
    if (info.dx != 0.0f) a->velX = -a->velX * a->elasticidade;
}

void tupi_aplicar_atrito(TupiCorpo* c, float dt) {
    if (c->massa == 0.0f) return;
    float f = 1.0f - c->atrito * dt;
    if (f < 0.0f) f = 0.0f;
    c->velX *= f;
    c->velY *= f;
}

void tupi_limitar_velocidade(TupiCorpo* c, float maxVel) {
    float mag2 = c->velX * c->velX + c->velY * c->velY;
    if (mag2 > maxVel * maxVel) {
        float e = maxVel / sqrtf(mag2);
        c->velX *= e;
        c->velY *= e;
    }
}