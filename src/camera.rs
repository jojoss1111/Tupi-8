// src/camera.rs
// TupiEngine — Validação de parâmetros da Câmera 2D

const POSICAO_MAX: f32 = 1_000_000.0;
const ZOOM_MIN:    f32 = 0.001;
const ZOOM_MAX:    f32 = 1_000.0;
const ROT_MAX:     f32 = std::f32::consts::TAU * 100.0;

/// Valida os parâmetros antes de qualquer operação de câmera.
/// Retorna 1 (válido) ou 0 (inválido).
///
/// Nota: os parâmetros validados são os do ALVO (x, y), não de _cam_x/_cam_y.
#[no_mangle]
pub extern "C" fn tupi_camera_validar(x: f32, y: f32, zoom: f32, rotacao: f32) -> i32 {
    if !x.is_finite() || !y.is_finite() || !zoom.is_finite() || !rotacao.is_finite() {
        return 0;
    }
    if x.abs() > POSICAO_MAX || y.abs() > POSICAO_MAX { return 0; }
    if zoom < ZOOM_MIN || zoom > ZOOM_MAX { return 0; }
    if rotacao.abs() > ROT_MAX { return 0; }
    1
}