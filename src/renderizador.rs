// renderizador.rs
// TupiEngine - Camada de Segurança Rust para RendererGL.c

use std::ffi::{c_float, c_int, CStr};
use std::os::raw::c_char;

// --- Asset Manager ---

#[repr(C)]
pub struct TupiAsset {
    pub ptr:     *mut u8,
    pub tamanho: usize,
}

impl TupiAsset {
    fn vazio() -> Self {
        TupiAsset { ptr: std::ptr::null_mut(), tamanho: 0 }
    }
}

/// Carrega um arquivo do disco. Retorna `tamanho == 0` em caso de falha.
#[no_mangle]
pub extern "C" fn tupi_asset_carregar(caminho: *const c_char) -> TupiAsset {
    let path_str = match unsafe { CStr::from_ptr(caminho) }.to_str() {
        Ok(s)  => s,
        Err(e) => {
            eprintln!("[TupiAsset] Caminho inválido (UTF-8): {e}");
            return TupiAsset::vazio();
        }
    };

    let bytes = match std::fs::read(path_str) {
        Ok(b) => b,
        Err(e) => {
            eprintln!("[TupiAsset] Falha ao ler '{path_str}': {e}");
            return TupiAsset::vazio();
        }
    };

    if bytes.is_empty() {
        eprintln!("[TupiAsset] Arquivo vazio: '{path_str}'");
        return TupiAsset::vazio();
    }

    if path_str.ends_with(".png") && (bytes.len() < 4 || &bytes[0..4] != b"\x89PNG") {
        eprintln!("[TupiAsset] '{path_str}' não é um PNG válido");
        return TupiAsset::vazio();
    }

    let tamanho = bytes.len();
    let mut boxed = bytes.into_boxed_slice();
    let ptr = boxed.as_mut_ptr();
    std::mem::forget(boxed);

    println!("[TupiAsset] '{path_str}' carregado ({tamanho} bytes)");
    TupiAsset { ptr, tamanho }
}

#[no_mangle]
pub extern "C" fn tupi_asset_liberar(asset: TupiAsset) {
    if asset.ptr.is_null() || asset.tamanho == 0 { return; }
    unsafe {
        let _ = Box::from_raw(std::slice::from_raw_parts_mut(asset.ptr, asset.tamanho));
    }
}

// --- Batcher com Z-Layer ---

const MAX_VERTICES: usize = 1024;

pub const TUPI_Z_MIN: i32 = -1000;
pub const TUPI_Z_MAX: i32 =  1000;

#[repr(C)]
#[derive(Clone, Copy, PartialEq)]
pub enum TupiPrimitiva {
    Retangulo = 0,
    Circulo   = 1,
    Linha     = 2,
    Triangulo = 3,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct TupiDrawCall {
    pub primitiva: TupiPrimitiva,
    pub verts:     [c_float; 8],
    pub n_verts:   c_int,
    pub cor:       [c_float; 4],
}

#[derive(Clone, Copy)]
struct DrawCallZ {
    dc:      TupiDrawCall,
    z_layer: i32,
    ordem:   u32,
}

pub struct Batcher {
    fila:         Vec<DrawCallZ>,
    verts_usados: usize,
    contador_ord: u32,
}

impl Batcher {
    pub fn novo() -> Self {
        Batcher { fila: Vec::with_capacity(256), verts_usados: 0, contador_ord: 0 }
    }

    fn tamanho_em_verts(dc: &TupiDrawCall) -> usize {
        (dc.n_verts as usize) / 2
    }

    fn clamp_z(z: i32) -> i32 {
        if z < TUPI_Z_MIN {
            eprintln!("[TupiBatcher] Z={z} < {TUPI_Z_MIN} → clampeado");
            TUPI_Z_MIN
        } else if z > TUPI_Z_MAX {
            eprintln!("[TupiBatcher] Z={z} > {TUPI_Z_MAX} → clampeado");
            TUPI_Z_MAX
        } else {
            z
        }
    }

    pub fn clamp_z_static(z: i32) -> i32 {
        z.clamp(TUPI_Z_MIN, TUPI_Z_MAX)
    }

    fn ordenar(&mut self) {
        self.fila.sort_by(|a, b| a.z_layer.cmp(&b.z_layer).then(a.ordem.cmp(&b.ordem)));
    }

    pub fn adicionar(&mut self, dc: TupiDrawCall, z: i32, flush_fn: impl Fn(&[TupiDrawCall])) -> bool {
        let verts_novos = Self::tamanho_em_verts(&dc);
        let mut flushed = false;

        if self.verts_usados + verts_novos > MAX_VERTICES {
            eprintln!("[TupiBatcher] Buffer cheio ({}/{MAX_VERTICES} verts) — flush automático", self.verts_usados);
            self.flush_interno(&flush_fn);
            flushed = true;
        }

        let z_safe = Self::clamp_z(z);
        self.contador_ord = self.contador_ord.wrapping_add(1);
        self.fila.push(DrawCallZ { dc, z_layer: z_safe, ordem: self.contador_ord });
        self.verts_usados += verts_novos;
        flushed
    }

    fn flush_interno(&mut self, flush_fn: impl Fn(&[TupiDrawCall])) {
        if self.fila.is_empty() { return; }
        self.ordenar();
        let ordenados: Vec<TupiDrawCall> = self.fila.iter().map(|e| e.dc).collect();
        flush_fn(&ordenados);
        self.fila.clear();
        self.verts_usados = 0;
        self.contador_ord = 0;
    }

    pub fn flush(&mut self, flush_fn: impl Fn(&[TupiDrawCall])) {
        self.flush_interno(flush_fn);
    }

    pub fn tamanho(&self) -> usize { self.fila.len() }
}

use std::cell::RefCell;

thread_local! {
    static BATCHER:  RefCell<Batcher>        = RefCell::new(Batcher::novo());
    static FLUSH_CB: RefCell<Option<FlushFn>> = RefCell::new(None);
}

type FlushFn = unsafe extern "C" fn(calls: *const TupiDrawCall, n: c_int);

#[no_mangle]
pub extern "C" fn tupi_batcher_registrar_flush(cb: FlushFn) {
    FLUSH_CB.with(|f| *f.borrow_mut() = Some(cb));
}

#[no_mangle]
pub extern "C" fn tupi_batcher_adicionar(dc: TupiDrawCall) {
    tupi_batcher_adicionar_z(dc, 0);
}

#[no_mangle]
pub extern "C" fn tupi_batcher_adicionar_z(dc: TupiDrawCall, z: c_int) {
    FLUSH_CB.with(|cb_cell| {
        let cb_opt = cb_cell.borrow();
        if let Some(cb) = *cb_opt {
            BATCHER.with(|b| {
                b.borrow_mut().adicionar(dc, z, |fila| unsafe {
                    cb(fila.as_ptr(), fila.len() as c_int);
                });
            });
        } else {
            eprintln!("[TupiBatcher] Flush não registrado! Chame tupi_batcher_registrar_flush primeiro.");
        }
    });
}

#[no_mangle]
pub extern "C" fn tupi_batcher_flush() {
    FLUSH_CB.with(|cb_cell| {
        let cb_opt = cb_cell.borrow();
        if let Some(cb) = *cb_opt {
            BATCHER.with(|b| {
                b.borrow_mut().flush(|fila| unsafe {
                    cb(fila.as_ptr(), fila.len() as c_int);
                });
            });
        }
    });
}

#[no_mangle]
pub extern "C" fn tupi_batcher_tamanho() -> c_int {
    BATCHER.with(|b| b.borrow().tamanho() as c_int)
}

// --- Math Core ---

#[repr(C)]
pub struct TupiMatriz {
    pub m: [c_float; 16],
}

impl TupiMatriz {
    fn identidade() -> Self {
        TupiMatriz { m: [
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0,
        ]}
    }
}

/// Projeção ortográfica com origem no canto superior esquerdo.
/// Retorna identidade se as dimensões forem inválidas.
#[no_mangle]
pub extern "C" fn tupi_projecao_ortografica(largura: c_int, altura: c_int) -> TupiMatriz {
    if largura <= 0 || altura <= 0 {
        eprintln!("[TupiMath] Dimensões inválidas: {largura}x{altura}");
        return TupiMatriz::identidade();
    }

    let l = 0.0_f32;
    let r = largura as f32;
    let t = 0.0_f32;
    let b = altura as f32;
    let n = -1.0_f32;
    let f = 1.0_f32;

    let m = [
        2.0/(r-l),     0.0,           0.0,          0.0,
        0.0,           2.0/(t-b),     0.0,          0.0,
        0.0,           0.0,          -2.0/(f-n),    0.0,
        -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n),  1.0_f32,
    ];

    for (i, v) in m.iter().enumerate() {
        if !v.is_finite() {
            eprintln!("[TupiMath] Elemento [{i}] inválido para {largura}x{altura}");
            return TupiMatriz::identidade();
        }
    }

    TupiMatriz { m }
}

#[no_mangle]
pub extern "C" fn tupi_mat4_multiplicar(a: *const TupiMatriz, b: *const TupiMatriz) -> TupiMatriz {
    if a.is_null() || b.is_null() {
        eprintln!("[TupiMath] tupi_mat4_multiplicar: ponteiro nulo");
        return TupiMatriz::identidade();
    }

    let a = unsafe { &(*a).m };
    let b = unsafe { &(*b).m };
    let mut c = [0.0_f32; 16];

    for col in 0..4 {
        for row in 0..4 {
            let mut soma = 0.0_f32;
            for k in 0..4 {
                soma += a[k * 4 + row] * b[col * 4 + k];
            }
            c[col * 4 + row] = soma;
        }
    }

    TupiMatriz { m: c }
}

// --- Sistema de Paralaxe ---

const TUPI_MAX_PARALAX: usize = 32;

#[derive(Clone, Copy)]
struct CamadaParalax {
    fator_x:      f32,
    fator_y:      f32,
    offset_x:     f32,
    offset_y:     f32,
    z_layer:      i32,
    largura_loop: f32,
    altura_loop:  f32,
    ativa:        bool,
}

impl CamadaParalax {
    fn vazia() -> Self {
        CamadaParalax {
            fator_x: 0.0, fator_y: 0.0,
            offset_x: 0.0, offset_y: 0.0,
            z_layer: 0,
            largura_loop: 0.0, altura_loop: 0.0,
            ativa: false,
        }
    }
}

struct SistemaParalax {
    camadas:        [CamadaParalax; TUPI_MAX_PARALAX],
    cam_x_ant:      f32,
    cam_y_ant:      f32,
    primeiro_frame: bool,
}

impl SistemaParalax {
    fn novo() -> Self {
        SistemaParalax {
            camadas:        [CamadaParalax::vazia(); TUPI_MAX_PARALAX],
            cam_x_ant:      0.0,
            cam_y_ant:      0.0,
            primeiro_frame: true,
        }
    }

    fn clamp_fator(v: f32, nome: &str, id: usize) -> f32 {
        if v < 0.0 {
            eprintln!("[TupiParalax] camada {id}: {nome}={v} < 0 → clampeado para 0.0");
            0.0
        } else if v > 1.0 {
            eprintln!("[TupiParalax] camada {id}: {nome}={v} > 1 → clampeado para 1.0");
            1.0
        } else {
            v
        }
    }

    fn registrar(&mut self, fator_x: f32, fator_y: f32, z_layer: i32, largura_loop: f32, altura_loop: f32) -> i32 {
        match self.camadas.iter().position(|c| !c.ativa) {
            None => {
                eprintln!("[TupiParalax] Limite de {TUPI_MAX_PARALAX} camadas atingido");
                -1
            }
            Some(id) => {
                let fx = Self::clamp_fator(fator_x, "fator_x", id);
                let fy = Self::clamp_fator(fator_y, "fator_y", id);
                let z  = Batcher::clamp_z_static(z_layer);
                self.camadas[id] = CamadaParalax {
                    fator_x: fx, fator_y: fy,
                    offset_x: 0.0, offset_y: 0.0,
                    z_layer: z,
                    largura_loop: largura_loop.max(0.0),
                    altura_loop:  altura_loop.max(0.0),
                    ativa: true,
                };
                println!("[TupiParalax] Camada {id} registrada (fator={fx:.2}×{fy:.2}, z={z})");
                id as i32
            }
        }
    }

    fn remover(&mut self, id: usize) -> bool {
        if id >= TUPI_MAX_PARALAX || !self.camadas[id].ativa {
            eprintln!("[TupiParalax] remover: ID {id} inválido ou já removido");
            return false;
        }
        self.camadas[id] = CamadaParalax::vazia();
        true
    }

    fn atualizar(&mut self, cam_x: f32, cam_y: f32) {
        if self.primeiro_frame {
            self.cam_x_ant = cam_x;
            self.cam_y_ant = cam_y;
            self.primeiro_frame = false;
            return;
        }

        let delta_x = cam_x - self.cam_x_ant;
        let delta_y = cam_y - self.cam_y_ant;
        self.cam_x_ant = cam_x;
        self.cam_y_ant = cam_y;

        for cam in self.camadas.iter_mut().filter(|c| c.ativa) {
            cam.offset_x -= delta_x * cam.fator_x;
            cam.offset_y -= delta_y * cam.fator_y;
            if cam.largura_loop > 0.0 { cam.offset_x = cam.offset_x.rem_euclid(cam.largura_loop); }
            if cam.altura_loop  > 0.0 { cam.offset_y = cam.offset_y.rem_euclid(cam.altura_loop);  }
        }
    }

    fn offset(&self, id: usize) -> Option<(f32, f32, i32)> {
        if id >= TUPI_MAX_PARALAX || !self.camadas[id].ativa { return None; }
        let c = &self.camadas[id];
        Some((c.offset_x, c.offset_y, c.z_layer))
    }

    fn resetar_camera(&mut self) {
        self.cam_x_ant = 0.0;
        self.cam_y_ant = 0.0;
        self.primeiro_frame = true;
    }

    fn resetar_camada(&mut self, id: usize) -> bool {
        if id >= TUPI_MAX_PARALAX || !self.camadas[id].ativa {
            eprintln!("[TupiParalax] resetar_camada: ID {id} inválido");
            return false;
        }
        self.camadas[id].offset_x = 0.0;
        self.camadas[id].offset_y = 0.0;
        true
    }
}

thread_local! {
    static PARALAX: RefCell<SistemaParalax> = RefCell::new(SistemaParalax::novo());
}

#[repr(C)]
pub struct TupiParalaxOffset {
    pub offset_x: c_float,
    pub offset_y: c_float,
    pub z_layer:  c_int,
    pub valido:   c_int,
}

#[no_mangle]
pub extern "C" fn tupi_paralax_registrar(
    fator_x: c_float, fator_y: c_float,
    z_layer: c_int,
    largura_loop: c_float, altura_loop: c_float,
) -> c_int {
    PARALAX.with(|p| p.borrow_mut().registrar(fator_x, fator_y, z_layer, largura_loop, altura_loop))
}

#[no_mangle]
pub extern "C" fn tupi_paralax_remover(id: c_int) -> c_int {
    if id < 0 { return 0; }
    PARALAX.with(|p| p.borrow_mut().remover(id as usize) as c_int)
}

/// Deve ser chamado uma vez por frame com a posição da câmera.
#[no_mangle]
pub extern "C" fn tupi_paralax_atualizar(cam_x: c_float, cam_y: c_float) {
    PARALAX.with(|p| p.borrow_mut().atualizar(cam_x, cam_y));
}

#[no_mangle]
pub extern "C" fn tupi_paralax_offset(id: c_int) -> TupiParalaxOffset {
    if id < 0 {
        return TupiParalaxOffset { offset_x: 0.0, offset_y: 0.0, z_layer: 0, valido: 0 };
    }
    PARALAX.with(|p| match p.borrow().offset(id as usize) {
        Some((ox, oy, z)) => TupiParalaxOffset { offset_x: ox, offset_y: oy, z_layer: z, valido: 1 },
        None              => TupiParalaxOffset { offset_x: 0.0, offset_y: 0.0, z_layer: 0, valido: 0 },
    })
}

#[no_mangle]
pub extern "C" fn tupi_paralax_resetar() {
    PARALAX.with(|p| p.borrow_mut().resetar_camera());
}

#[no_mangle]
pub extern "C" fn tupi_paralax_resetar_camada(id: c_int) -> c_int {
    if id < 0 { return 0; }
    PARALAX.with(|p| p.borrow_mut().resetar_camada(id as usize) as c_int)
}

#[no_mangle]
pub extern "C" fn tupi_paralax_set_fator(id: c_int, fator_x: c_float, fator_y: c_float) -> c_int {
    if id < 0 { return 0; }
    let uid = id as usize;
    PARALAX.with(|p| -> c_int {
        let mut sys = p.borrow_mut();
        if uid >= TUPI_MAX_PARALAX || !sys.camadas[uid].ativa {
            eprintln!("[TupiParalax] set_fator: ID {uid} inválido");
            return 0;
        }
        sys.camadas[uid].fator_x = SistemaParalax::clamp_fator(fator_x, "fator_x", uid);
        sys.camadas[uid].fator_y = SistemaParalax::clamp_fator(fator_y, "fator_y", uid);
        1
    })
}

#[no_mangle]
pub extern "C" fn tupi_paralax_total_ativas() -> c_int {
    PARALAX.with(|p| p.borrow().camadas.iter().filter(|c| c.ativa).count() as c_int)
}