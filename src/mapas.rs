// mapas.rs — TupiEngine · Validação de Mapas (tupi_seguro)
//
// Chamado pelo C via tupi_mapa_validar_rust() após o build.
// Garante que o mapa está correto antes de qualquer draw.
//
// Regras verificadas:
//   1. Grade entre 1×1 e 256×256
//   2. Atlas com dimensões positivas
//   3. Ao menos uma definição de tile
//   4. Todos os def_ids da grade apontam para defs existentes
//   5. Tiles animados têm fps > 0 e num_frames válido

use std::ffi::{c_char, c_int};
use std::fmt::Write as FmtWrite;
use std::slice;

// Espelham Mapas.h
const TUPI_MAPA_MAX_DEFS:    usize = 256;
const TUPI_MAPA_MAX_FRAMES:  usize = 32;
const TUPI_MAPA_MAX_CELULAS: usize = 256 * 256;
const TUPI_TILE_ANIMADO:     u8    = 0x08;

// ---------------------------------------------------------------------------
// Erros tipados
// ---------------------------------------------------------------------------

#[derive(Debug)]
enum ErroMapa {
    GradeInvalida      { colunas: i32, linhas: i32 },
    GrandeDemais       { total: i32 },
    AtlasInvalido      { larg: i32, alt: i32 },
    SemDefinicoes,
    DefsExcedidas      { num: i32 },
    IdForaDoIntervalo  { indice: i32, id: u8, num_defs: i32 },
    FpsInvalidoAnimado { def_id: i32 },
    FramesInvalidos    { def_id: i32, num_frames: i32 },
    TotalIncompativel  { esperado: i32, recebido: i32 },
}

impl std::fmt::Display for ErroMapa {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::GradeInvalida { colunas, linhas } =>
                write!(f, "Grade inválida: {}×{} (aceito: 1×1 até 256×256)", colunas, linhas),
            Self::GrandeDemais { total } =>
                write!(f, "Grade grande demais: {} células (máx {})", total, TUPI_MAPA_MAX_CELULAS),
            Self::AtlasInvalido { larg, alt } =>
                write!(f, "Atlas inválido: {}×{} (deve ser > 0)", larg, alt),
            Self::SemDefinicoes =>
                write!(f, "Nenhum tile definido — use Tupi.criarTile()"),
            Self::DefsExcedidas { num } =>
                write!(f, "Muitos tipos de tile: {} (máx {})", num, TUPI_MAPA_MAX_DEFS),
            Self::IdForaDoIntervalo { indice, id, num_defs } =>
                write!(f, "Célula #{}: id {} não existe (total: {})", indice, id, num_defs),
            Self::FpsInvalidoAnimado { def_id } =>
                write!(f, "Tile {} animado com fps ≤ 0", def_id),
            Self::FramesInvalidos { def_id, num_frames } =>
                write!(f, "Tile {} com num_frames={} inválido (1–{})", def_id, num_frames, TUPI_MAPA_MAX_FRAMES),
            Self::TotalIncompativel { esperado, recebido } =>
                write!(f, "Array com {} ids, esperava {}", recebido, esperado),
        }
    }
}

// ---------------------------------------------------------------------------
// Validação pura (sem FFI — testável diretamente)
// ---------------------------------------------------------------------------

struct InfoValidacao<'a> {
    colunas:    i32,
    linhas:     i32,
    num_defs:   i32,
    atlas_larg: i32,
    atlas_alt:  i32,
    ids:        &'a [u8],
    // Opcionais — passados futuramente pelo C para checar animações por def
    fps_por_def:    Option<&'a [f32]>,
    frames_por_def: Option<&'a [i32]>,
    flags_por_def:  Option<&'a [u8]>,
}

fn validar(info: &InfoValidacao<'_>) -> Vec<ErroMapa> {
    let mut erros: Vec<ErroMapa> = Vec::new();

    // 1. Dimensões
    if info.colunas < 1 || info.colunas > 256 || info.linhas < 1 || info.linhas > 256 {
        erros.push(ErroMapa::GradeInvalida { colunas: info.colunas, linhas: info.linhas });
        return erros;
    }

    // 2. Total de células
    let total = info.colunas * info.linhas;
    if total as usize > TUPI_MAPA_MAX_CELULAS {
        erros.push(ErroMapa::GrandeDemais { total });
    }

    // 3. Atlas
    if info.atlas_larg <= 0 || info.atlas_alt <= 0 {
        erros.push(ErroMapa::AtlasInvalido { larg: info.atlas_larg, alt: info.atlas_alt });
    }

    // 4. Definições
    if info.num_defs <= 0 {
        erros.push(ErroMapa::SemDefinicoes);
    } else if info.num_defs as usize > TUPI_MAPA_MAX_DEFS {
        erros.push(ErroMapa::DefsExcedidas { num: info.num_defs });
    }

    // 5. Tamanho do array
    if info.ids.len() != total as usize {
        erros.push(ErroMapa::TotalIncompativel {
            esperado: total,
            recebido: info.ids.len() as i32,
        });
        return erros;
    }

    // 6. def_ids válidos (0 = vazio, ignorado)
    if info.num_defs > 0 {
        for (i, &id) in info.ids.iter().enumerate() {
            if id == 0 { continue; }
            if (id as i32) > info.num_defs {
                erros.push(ErroMapa::IdForaDoIntervalo {
                    indice: i as i32,
                    id,
                    num_defs: info.num_defs,
                });
            }
        }
    }

    // 7. Animações (opcional — quando o C passar os arrays por def)
    if let (Some(fps_arr), Some(frames_arr), Some(flags_arr)) =
        (info.fps_por_def, info.frames_por_def, info.flags_por_def)
    {
        let n = (info.num_defs as usize)
            .min(fps_arr.len())
            .min(frames_arr.len())
            .min(flags_arr.len());

        for i in 0..n {
            if flags_arr[i] & TUPI_TILE_ANIMADO == 0 { continue; }

            let def_id = (i + 1) as i32;

            if fps_arr[i] <= 0.0 {
                erros.push(ErroMapa::FpsInvalidoAnimado { def_id });
            }
            if frames_arr[i] < 1 || frames_arr[i] as usize > TUPI_MAPA_MAX_FRAMES {
                erros.push(ErroMapa::FramesInvalidos { def_id, num_frames: frames_arr[i] });
            }
        }
    }

    erros
}

// ---------------------------------------------------------------------------
// FFI — chamada pelo C em Mapas.c
// ---------------------------------------------------------------------------

/// # Safety
/// `ids` deve ser válido com `num_ids` bytes. `msg_out` deve ter `msg_cap` bytes.
#[no_mangle]
pub unsafe extern "C" fn tupi_mapa_validar_rust(
    colunas:    c_int,
    linhas:     c_int,
    num_defs:   c_int,
    atlas_larg: c_int,
    atlas_alt:  c_int,
    ids:        *const u8,
    num_ids:    c_int,
    msg_out:    *mut c_char,
    msg_cap:    c_int,
) -> c_int {
    let ids_slice: &[u8] = if ids.is_null() || num_ids <= 0 {
        &[]
    } else {
        slice::from_raw_parts(ids, num_ids as usize)
    };

    let info = InfoValidacao {
        colunas, linhas, num_defs,
        atlas_larg, atlas_alt,
        ids: ids_slice,
        fps_por_def: None, frames_por_def: None, flags_por_def: None,
    };

    let erros = validar(&info);

    // Escreve erros no buffer C
    if !msg_out.is_null() && msg_cap > 0 {
        let cap = (msg_cap as usize).saturating_sub(1);
        let mut msg = String::new();
        for e in &erros {
            let _ = writeln!(msg, "[TupiMapa] {}", e);
        }
        if msg.is_empty() {
            msg.push_str("[TupiMapa] OK\0");
        }
        let bytes = msg.as_bytes();
        let n = bytes.len().min(cap);
        std::ptr::copy_nonoverlapping(bytes.as_ptr(), msg_out as *mut u8, n);
        *msg_out.add(n) = 0;
    }

    if erros.is_empty() { 1 } else { 0 }
}

// ---------------------------------------------------------------------------
// Testes
// ---------------------------------------------------------------------------

#[cfg(test)]
mod testes {
    use super::*;

    fn ids_validos(cols: i32, lins: i32, num_defs: i32) -> Vec<u8> {
        let n = (cols * lins) as usize;
        (0..n).map(|_| if num_defs > 0 { 1u8 } else { 0u8 }).collect()
    }

    #[test]
    fn mapa_valido_simples() {
        let ids = ids_validos(10, 10, 3);
        let info = InfoValidacao {
            colunas: 10, linhas: 10, num_defs: 3,
            atlas_larg: 256, atlas_alt: 256, ids: &ids,
            fps_por_def: None, frames_por_def: None, flags_por_def: None,
        };
        assert!(validar(&info).is_empty());
    }

    #[test]
    fn grade_invalida_zero() {
        let info = InfoValidacao {
            colunas: 0, linhas: 0, num_defs: 1,
            atlas_larg: 64, atlas_alt: 64, ids: &[],
            fps_por_def: None, frames_por_def: None, flags_por_def: None,
        };
        let erros = validar(&info);
        assert!(erros.iter().any(|e| matches!(e, ErroMapa::GradeInvalida { .. })));
    }

    #[test]
    fn sem_definicoes() {
        let ids = vec![0u8; 4];
        let info = InfoValidacao {
            colunas: 2, linhas: 2, num_defs: 0,
            atlas_larg: 64, atlas_alt: 64, ids: &ids,
            fps_por_def: None, frames_por_def: None, flags_por_def: None,
        };
        assert!(validar(&info).iter().any(|e| matches!(e, ErroMapa::SemDefinicoes)));
    }

    #[test]
    fn id_fora_do_intervalo() {
        let ids = vec![5u8, 1, 1, 1];
        let info = InfoValidacao {
            colunas: 2, linhas: 2, num_defs: 2,
            atlas_larg: 128, atlas_alt: 128, ids: &ids,
            fps_por_def: None, frames_por_def: None, flags_por_def: None,
        };
        assert!(validar(&info).iter().any(|e| matches!(e, ErroMapa::IdForaDoIntervalo { .. })));
    }

    #[test]
    fn atlas_invalido() {
        let ids = ids_validos(4, 4, 1);
        let info = InfoValidacao {
            colunas: 4, linhas: 4, num_defs: 1,
            atlas_larg: 0, atlas_alt: -1, ids: &ids,
            fps_por_def: None, frames_por_def: None, flags_por_def: None,
        };
        assert!(validar(&info).iter().any(|e| matches!(e, ErroMapa::AtlasInvalido { .. })));
    }

    #[test]
    fn total_incompativel() {
        let ids = vec![1u8; 6]; // grade 3×3 = 9, mas só 6 ids
        let info = InfoValidacao {
            colunas: 3, linhas: 3, num_defs: 1,
            atlas_larg: 64, atlas_alt: 64, ids: &ids,
            fps_por_def: None, frames_por_def: None, flags_por_def: None,
        };
        assert!(validar(&info).iter().any(|e| matches!(e, ErroMapa::TotalIncompativel { .. })));
    }
}