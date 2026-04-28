// sprites.rs
// TupiEngine — Módulo de Sprites

use std::collections::HashMap;
use std::ffi::CStr;
use std::os::raw::{c_char, c_float, c_int, c_uint};

// --- Sistema 1: Decodificação de Imagens ---

/// Resultado da decodificação — devolvido ao C como ponteiro opaco.
/// O C deve liberar com `tupi_imagem_destruir`.
#[repr(C)]
pub struct TupiImagemRust {
    pub pixels:  *mut u8,
    pub largura: c_int,
    pub altura:  c_int,
    pub tamanho: c_int,
}

/// Carrega PNG/JPEG/BMP e retorna pixels RGBA prontos para `glTexImage2D`.
/// Retorna NULL em caso de falha.
///
/// # Safety
/// `caminho` deve ser um ponteiro válido para string C terminada em '\0'.
#[no_mangle]
pub extern "C" fn tupi_imagem_carregar_seguro(caminho: *const c_char) -> *mut TupiImagemRust {
    let caminho_str = unsafe {
        if caminho.is_null() {
            eprintln!("[TupiRust] tupi_imagem_carregar_seguro: caminho nulo");
            return std::ptr::null_mut();
        }
        match CStr::from_ptr(caminho).to_str() {
            Ok(s)  => s,
            Err(_) => {
                eprintln!("[TupiRust] tupi_imagem_carregar_seguro: caminho com caracteres inválidos");
                return std::ptr::null_mut();
            }
        }
    };

    let img = match image::open(caminho_str) {
        Ok(i)  => i,
        Err(e) => {
            eprintln!("[TupiRust] Falha ao carregar '{}': {}", caminho_str, e);
            return std::ptr::null_mut();
        }
    };

    let rgba = img.to_rgba8();
    let largura  = rgba.width()  as c_int;
    let altura   = rgba.height() as c_int;
    let tamanho  = largura * altura * 4;

    let mut pixels_vec = rgba.into_raw();
    pixels_vec.shrink_to_fit();
    let pixels = pixels_vec.as_mut_ptr();
    std::mem::forget(pixels_vec);

    Box::into_raw(Box::new(TupiImagemRust { pixels, largura, altura, tamanho }))
}

/// Libera a memória de uma `TupiImagemRust`.
///
/// # Safety
/// `img` deve ser um ponteiro válido retornado por `tupi_imagem_carregar_seguro`.
#[no_mangle]
pub extern "C" fn tupi_imagem_destruir(img: *mut TupiImagemRust) {
    if img.is_null() { return; }
    unsafe {
        let img_ref = &*img;
        if !img_ref.pixels.is_null() && img_ref.tamanho > 0 {
            Vec::from_raw_parts(img_ref.pixels, img_ref.tamanho as usize, img_ref.tamanho as usize);
        }
        drop(Box::from_raw(img));
    }
}

// --- Sistema 2: Atlas de Sprites ---

/// Coordenadas UV de uma célula no sprite sheet.
#[repr(C)]
pub struct TupiUV {
    pub u0: c_float,
    pub v0: c_float,
    pub u1: c_float,
    pub v1: c_float,
}

#[derive(Clone)]
struct EntradaAnimacao {
    linha:    u32,
    colunas:  u32,
    cel_larg: f32,
    cel_alt:  f32,
    img_larg: f32,
    img_alt:  f32,
}

pub struct TupiAtlas {
    animacoes: HashMap<String, EntradaAnimacao>,
}

impl TupiAtlas {
    fn novo() -> Self {
        TupiAtlas { animacoes: HashMap::new() }
    }

    fn registrar(&mut self, nome: &str, entrada: EntradaAnimacao) {
        self.animacoes.insert(nome.to_string(), entrada);
    }

    fn calcular_uv(&self, nome: &str, frame: u32) -> Option<TupiUV> {
        let anim = self.animacoes.get(nome)?;
        let frame_seguro = frame % anim.colunas;

        // FIX: inset de meio texel para evitar texture bleeding entre frames do sprite sheet.
        // Sem isso, o UV cai exatamente na borda do texel e a GPU pode amostrar o pixel vizinho,
        // causando a linha fina de pixel do sprite adjacente visível ao mover a câmera.
        let texel_w = 0.5 / anim.img_larg;
        let texel_h = 0.5 / anim.img_alt;

        let u0 = (frame_seguro as f32 * anim.cel_larg) / anim.img_larg + texel_w;
        let v0 = (anim.linha    as f32 * anim.cel_alt)  / anim.img_alt  + texel_h;
        let u1 = u0 + anim.cel_larg / anim.img_larg - 2.0 * texel_w;
        let v1 = v0 + anim.cel_alt  / anim.img_alt  - 2.0 * texel_h;

        Some(TupiUV { u0, v0, u1, v1 })
    }
}

#[no_mangle]
pub extern "C" fn tupi_atlas_criar() -> *mut TupiAtlas {
    Box::into_raw(Box::new(TupiAtlas::novo()))
}

/// # Safety
/// `atlas` deve ser um ponteiro válido criado por `tupi_atlas_criar`.
#[no_mangle]
pub extern "C" fn tupi_atlas_destruir(atlas: *mut TupiAtlas) {
    if atlas.is_null() { return; }
    unsafe { drop(Box::from_raw(atlas)); }
}

/// # Safety
/// Todos os ponteiros devem ser válidos.
#[no_mangle]
pub extern "C" fn tupi_atlas_registrar(
    atlas:    *mut TupiAtlas,
    nome:     *const c_char,
    linha:    c_uint,
    colunas:  c_uint,
    cel_larg: c_float,
    cel_alt:  c_float,
    img_larg: c_float,
    img_alt:  c_float,
) {
    if atlas.is_null() || nome.is_null() { return; }

    let nome_str = unsafe {
        match CStr::from_ptr(nome).to_str() {
            Ok(s)  => s,
            Err(_) => return,
        }
    };

    let entrada = EntradaAnimacao { linha, colunas, cel_larg, cel_alt, img_larg, img_alt };
    unsafe { (*atlas).registrar(nome_str, entrada); }
}

/// Calcula as coordenadas UV de um frame. Retorna 1 em sucesso, 0 se a animação não existir.
///
/// # Safety
/// Todos os ponteiros devem ser válidos.
#[no_mangle]
pub extern "C" fn tupi_atlas_uv(
    atlas:  *const TupiAtlas,
    nome:   *const c_char,
    frame:  c_uint,
    uv_out: *mut TupiUV,
) -> c_int {
    if atlas.is_null() || nome.is_null() || uv_out.is_null() { return 0; }

    let nome_str = unsafe {
        match CStr::from_ptr(nome).to_str() {
            Ok(s)  => s,
            Err(_) => return 0,
        }
    };

    match unsafe { (*atlas).calcular_uv(nome_str, frame) } {
        Some(uv) => { unsafe { *uv_out = uv; }; 1 }
        None => {
            eprintln!("[TupiRust] Atlas: animacao '{}' nao encontrada", nome_str);
            0
        }
    }
}

// --- Sistema 3: Batching de Draw Calls ---

#[repr(C)]
#[derive(Clone)]
pub struct TupiItemBatch {
    pub textura_id: c_uint,
    pub quad:       [c_float; 16],
    pub cor:        [c_float; 4],  // RGB tint + alpha — multiplica sobre a textura
    pub z_index:    c_int,
}

pub struct TupiBatch {
    itens: Vec<TupiItemBatch>,
}

impl TupiBatch {
    fn novo() -> Self {
        TupiBatch { itens: Vec::with_capacity(512) }
    }

    fn adicionar(&mut self, item: TupiItemBatch) {
        self.itens.push(item);
    }

    fn ordenar(&mut self) {
        self.itens.sort_by(|a, b| {
            a.z_index.cmp(&b.z_index).then(a.textura_id.cmp(&b.textura_id))
        });
    }

    fn limpar(&mut self) {
        self.itens.clear();
    }
}

#[no_mangle]
pub extern "C" fn tupi_batch_criar() -> *mut TupiBatch {
    Box::into_raw(Box::new(TupiBatch::novo()))
}

/// # Safety
/// `batch` deve ser um ponteiro válido criado por `tupi_batch_criar`.
#[no_mangle]
pub extern "C" fn tupi_batch_destruir(batch: *mut TupiBatch) {
    if batch.is_null() { return; }
    unsafe { drop(Box::from_raw(batch)); }
}

/// # Safety
/// `batch`, `quad` e `cor` devem ser ponteiros válidos.
/// `cor` aponta para 4 floats: [r, g, b, a] — multiplica sobre os pixels da textura.
#[no_mangle]
pub extern "C" fn tupi_batch_adicionar(
    batch:      *mut TupiBatch,
    textura_id: c_uint,
    quad:       *const c_float,
    cor:        *const c_float,   // [r, g, b, a]
    z_index:    c_int,
) {
    if batch.is_null() || quad.is_null() { return; }

    let quad_arr: [c_float; 16] = unsafe {
        std::slice::from_raw_parts(quad, 16).try_into().unwrap_or([0.0; 16])
    };

    // cor padrão branco opaco se ponteiro nulo
    let cor_arr: [c_float; 4] = if cor.is_null() {
        [1.0, 1.0, 1.0, 1.0]
    } else {
        unsafe { std::slice::from_raw_parts(cor, 4).try_into().unwrap_or([1.0, 1.0, 1.0, 1.0]) }
    };

    unsafe { (*batch).adicionar(TupiItemBatch { textura_id, quad: quad_arr, cor: cor_arr, z_index }); }
}

/// Ordena por z_index + textura e retorna o ponteiro para o array.
/// Válido até a próxima chamada a `tupi_batch_adicionar` ou `tupi_batch_limpar`.
///
/// # Safety
/// `batch` e `contagem_out` devem ser ponteiros válidos.
#[no_mangle]
pub extern "C" fn tupi_batch_flush(
    batch:        *mut TupiBatch,
    contagem_out: *mut c_int,
) -> *const TupiItemBatch {
    if batch.is_null() || contagem_out.is_null() {
        if !contagem_out.is_null() { unsafe { *contagem_out = 0; } }
        return std::ptr::null();
    }

    let b = unsafe { &mut *batch };
    b.ordenar();
    unsafe { *contagem_out = b.itens.len() as c_int; }

    if b.itens.is_empty() { return std::ptr::null(); }
    b.itens.as_ptr()
}

/// Limpa todos os itens após o flush. Chamar no fim de cada frame.
///
/// # Safety
/// `batch` deve ser um ponteiro válido.
#[no_mangle]
pub extern "C" fn tupi_batch_limpar(batch: *mut TupiBatch) {
    if batch.is_null() { return; }
    unsafe { (*batch).limpar(); }
}

/// # Safety
/// `batch` deve ser um ponteiro válido.
#[no_mangle]
pub extern "C" fn tupi_batch_contagem(batch: *const TupiBatch) -> c_int {
    if batch.is_null() { return 0; }
    unsafe { (*batch).itens.len() as c_int }
}