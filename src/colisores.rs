// colisores.rs
// TupiEngine — Guardião de Colisões

// --- Tipos ---

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Layer(pub u32);

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Tag(pub u32);

#[derive(Clone, Copy)]
pub struct Colisor {
    pub layer: Layer,
    pub tag:   Tag,
}

#[derive(Clone, Copy)]
pub struct MascaraColisao(pub u32);

// --- Layers predefinidas ---

pub const LAYER_PADRAO:  Layer = Layer(0b0001);
pub const LAYER_JOGADOR: Layer = Layer(0b0010);
pub const LAYER_INIMIGO: Layer = Layer(0b0100);
pub const LAYER_CENARIO: Layer = Layer(0b1000);

// --- Tags predefinidas ---

pub const TAG_NENHUMA:  Tag = Tag(0);
pub const TAG_JOGADOR:  Tag = Tag(1);
pub const TAG_INIMIGO:  Tag = Tag(2);
pub const TAG_PROJETIL: Tag = Tag(3);
pub const TAG_ITEM:     Tag = Tag(4);
pub const TAG_CENARIO:  Tag = Tag(5);

// --- Guardião ---

pub struct GuardiaoColisao {
    mascaras: [u32; 32],
}

impl GuardiaoColisao {
    pub fn novo() -> Self {
        GuardiaoColisao { mascaras: [u32::MAX; 32] }
    }

    pub fn definir_mascara(&mut self, layer: Layer, mascara: u32) {
        let i = layer.0.trailing_zeros() as usize;
        if i < 32 { self.mascaras[i] = mascara; }
    }

    pub fn deve_colidir(&self, a: Colisor, b: Colisor) -> bool {
        let i = a.layer.0.trailing_zeros() as usize;
        let j = b.layer.0.trailing_zeros() as usize;
        if i >= 32 || j >= 32 { return false; }
        (self.mascaras[i] & b.layer.0 != 0) && (self.mascaras[j] & a.layer.0 != 0)
    }

    pub fn tem_tag(colisor: Colisor, tag: Tag) -> bool {
        colisor.tag == tag
    }
}

// --- Evento de Colisão ---

pub struct EventoColisao {
    pub colidiu: bool,
    pub tag_a:   Tag,
    pub tag_b:   Tag,
    pub dx:      f32,
    pub dy:      f32,
}

impl EventoColisao {
    pub fn vazio() -> Self {
        EventoColisao { colidiu: false, tag_a: TAG_NENHUMA, tag_b: TAG_NENHUMA, dx: 0.0, dy: 0.0 }
    }

    pub fn novo(tag_a: Tag, tag_b: Tag, dx: f32, dy: f32) -> Self {
        EventoColisao { colidiu: true, tag_a, tag_b, dx, dy }
    }

    pub fn entre(&self, ta: Tag, tb: Tag) -> bool {
        (self.tag_a == ta && self.tag_b == tb) || (self.tag_a == tb && self.tag_b == ta)
    }
}

// --- Testes ---

#[cfg(test)]
mod testes {
    use super::*;

    #[test]
    fn jogador_colide_com_inimigo() {
        let mut g = GuardiaoColisao::novo();
        g.definir_mascara(LAYER_JOGADOR, LAYER_INIMIGO.0 | LAYER_CENARIO.0);
        g.definir_mascara(LAYER_INIMIGO, LAYER_JOGADOR.0);

        let jogador = Colisor { layer: LAYER_JOGADOR, tag: TAG_JOGADOR };
        let inimigo = Colisor { layer: LAYER_INIMIGO, tag: TAG_INIMIGO };
        let outro   = Colisor { layer: LAYER_PADRAO,  tag: TAG_NENHUMA  };

        assert!(g.deve_colidir(jogador, inimigo));
        assert!(!g.deve_colidir(jogador, outro));
    }

    #[test]
    fn evento_entre_tags() {
        let ev = EventoColisao::novo(TAG_JOGADOR, TAG_PROJETIL, -5.0, 0.0);
        assert!(ev.entre(TAG_JOGADOR, TAG_PROJETIL));
        assert!(ev.entre(TAG_PROJETIL, TAG_JOGADOR));
        assert!(!ev.entre(TAG_JOGADOR, TAG_INIMIGO));
    }
}