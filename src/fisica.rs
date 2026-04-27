// fisica.rs
// TupiEngine — Bindings FFI para o sistema de física C

use std::ffi::c_float;

// --- Tipos espelhados do C ---

#[repr(C)]
pub struct TupiRetCol {
    pub x: c_float, pub y: c_float,
    pub largura: c_float, pub altura: c_float,
}

#[repr(C)]
pub struct TupiCircCol {
    pub x: c_float, pub y: c_float,
    pub raio: c_float,
}

#[repr(C)]
pub struct TupiColisao {
    pub colidindo: i32,
    pub dx: c_float,
    pub dy: c_float,
}

#[repr(C)]
pub struct TupiCorpo {
    pub x: c_float,
    pub y: c_float,
    pub vel_x: c_float,
    pub vel_y: c_float,
    pub acel_x: c_float,
    pub acel_y: c_float,
    pub massa: c_float,
    pub elasticidade: c_float,
    pub atrito: c_float,
}

// --- FFI ---
// Símbolos resolvidos pelo GCC no link final (libtupi_seguro.a + .o do C → .so).

unsafe extern "C" {
    pub fn tupi_fisica_atualizar(c: *mut TupiCorpo, dt: c_float, gravidade: c_float);
    pub fn tupi_fisica_impulso(c: *mut TupiCorpo, fx: c_float, fy: c_float);
    pub fn tupi_corpo_ret(c: *mut TupiCorpo, largura: c_float, altura: c_float) -> TupiRetCol;
    pub fn tupi_corpo_cir(c: *mut TupiCorpo, raio: c_float) -> TupiCircCol;
    pub fn tupi_resolver_colisao(a: *mut TupiCorpo, b: *mut TupiCorpo, info: TupiColisao);
    pub fn tupi_resolver_estatico(a: *mut TupiCorpo, info: TupiColisao);
    pub fn tupi_aplicar_atrito(c: *mut TupiCorpo, dt: c_float);
    pub fn tupi_limitar_velocidade(c: *mut TupiCorpo, max_vel: c_float);
}

// --- Wrapper seguro ---

pub struct Corpo(pub TupiCorpo);

impl Corpo {
    pub fn novo(x: f32, y: f32, massa: f32, elasticidade: f32, atrito: f32) -> Self {
        Corpo(TupiCorpo {
            x, y,
            vel_x: 0.0, vel_y: 0.0,
            acel_x: 0.0, acel_y: 0.0,
            massa, elasticidade, atrito,
        })
    }

    /// Corpo estático (paredes, chão) — massa 0, nunca se move.
    pub fn estatico(x: f32, y: f32) -> Self {
        Self::novo(x, y, 0.0, 0.0, 1.0)
    }

    pub fn atualizar(&mut self, dt: f32, gravidade: f32) {
        unsafe { tupi_fisica_atualizar(&mut self.0, dt, gravidade); }
    }

    pub fn impulso(&mut self, fx: f32, fy: f32) {
        unsafe { tupi_fisica_impulso(&mut self.0, fx, fy); }
    }

    pub fn resolver_estatico(&mut self, info: TupiColisao) {
        unsafe { tupi_resolver_estatico(&mut self.0, info); }
    }

    pub fn aplicar_atrito(&mut self, dt: f32) {
        unsafe { tupi_aplicar_atrito(&mut self.0, dt); }
    }

    pub fn limitar_velocidade(&mut self, max: f32) {
        unsafe { tupi_limitar_velocidade(&mut self.0, max); }
    }

    pub fn x(&self)     -> f32 { self.0.x }
    pub fn y(&self)     -> f32 { self.0.y }
    pub fn vel_x(&self) -> f32 { self.0.vel_x }
    pub fn vel_y(&self) -> f32 { self.0.vel_y }
}