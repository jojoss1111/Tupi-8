// tupi_pack.rs — Empacotador ZIP do TupiEngine

use std::env;
use std::error::Error;
use std::fs;
use std::io::Write;
use std::path::{Path, PathBuf};

use zip::write::SimpleFileOptions;
use zip::{CompressionMethod, ZipWriter};
// ---------------------------------------------------------------------------
// Constantes
// ---------------------------------------------------------------------------

/// Marcador gravado no comentário do ZIP para identificar um ZIP anexado.
/// Formato do comentário: b"TUPI_ZIP_APPENDED" + u64_le(offset_do_zip)
const APPENDED_MARKER: &[u8] = b"TUPI_ZIP_APPENDED";
const MARKER_TOTAL: usize = APPENDED_MARKER.len() + 8; // marcador + u64

const MAIN_ENTRY: &str = "__main__";
const ENGINE_ENTRY: &str = "__engine__";
const SCRIPTS_PREFIX: &str = "scripts/";
const LIB_PREFIX: &str = "lib/";
const ASSETS_PREFIX: &str = "assets/";

const ASSET_EXTENSIONS: &[&str] = &[
    "png", "jpg", "jpeg", "bmp", "gif", "webp",
    "wav", "ogg", "mp3", "flac",
    "ttf", "otf",
    "glsl", "vert", "frag", "spv",
    "json", "toml", "csv", "txt",
    "tmx", "tsx",
];

// ---------------------------------------------------------------------------
// Modos
// ---------------------------------------------------------------------------

enum OutputMode {
    Append,   // anexa ZIP ao executável existente
    Archive,  // gera um .tuzip standalone
    Bundle,   // copia bootstrapper + anex ZIP com engine + libs + assets + scripts
}

// ---------------------------------------------------------------------------
// Helpers de arquivo
// ---------------------------------------------------------------------------

fn is_lua_file(path: &Path) -> bool {
    matches!(path.extension().and_then(|e| e.to_str()), Some("lua"))
}

fn is_asset_file(path: &Path) -> bool {
    match path.extension().and_then(|e| e.to_str()) {
        Some(ext) => ASSET_EXTENSIONS.contains(&ext.to_lowercase().as_str()),
        None => false,
    }
}

/// Converte `scripts/utils/camera.lua` → `scripts/utils.camera.lua`
/// (preserva a estrutura com `.` como separador de módulo dentro de `scripts/`)
fn to_zip_script_path(path: &Path) -> Result<String, Box<dyn Error>> {
    let normalized = path.to_string_lossy().replace('\\', "/");
    let without_ext = normalized
        .strip_suffix(".lua")
        .ok_or_else(|| format!("arquivo Lua inválido: {}", path.display()))?;
    // Converte separadores de dir em pontos para o nome do módulo
    let module_name = without_ext.replace('/', ".");
    Ok(format!("{}{}.lua", SCRIPTS_PREFIX, module_name))
}

fn to_zip_asset_path(path: &Path) -> String {
    let normalized = path.to_string_lossy().replace('\\', "/");

    // 1. Já está limpo com prefixo "assets/" — usa direto
    if normalized.starts_with("assets/") {
        return normalized.to_owned();
    }

    // 2. Vem de .build/bundle_assets/ (caminho temporário antigo) — remove o prefixo
    if let Some(pos) = normalized.find("bundle_assets/") {
        let rel = &normalized[pos + "bundle_assets/".len()..];
        return format!("{}{}", ASSETS_PREFIX, rel);
    }

    // 3. Caminho relativo do projeto (ex: "./ascii.png", "./tilesets/grama.png")
    //    Remove "./" e usa o path relativo como está — preserva subpastas.
    let rel = normalized
        .strip_prefix("./")
        .unwrap_or(&normalized);

    format!("{}{}", ASSETS_PREFIX, rel)
}



fn to_zip_lib_path(path: &Path) -> String {
    let filename = path
        .file_name()
        .unwrap_or(path.as_os_str())
        .to_string_lossy();
    format!("{}{}", LIB_PREFIX, filename)
}

// ---------------------------------------------------------------------------
// Leitura do offset do ZIP anexado
// ---------------------------------------------------------------------------

/// Lê o offset do ZIP anexado a partir do comentário gravado no final do arquivo.
/// Retorna `None` se não houver marcador.
fn read_appended_zip_offset(exe_bytes: &[u8]) -> Option<u64> {
    if exe_bytes.len() < MARKER_TOTAL {
        return None;
    }
    let tail = &exe_bytes[exe_bytes.len() - MARKER_TOTAL..];
    if &tail[..APPENDED_MARKER.len()] != APPENDED_MARKER {
        return None;
    }
    let mut raw = [0u8; 8];
    raw.copy_from_slice(&tail[APPENDED_MARKER.len()..]);
    Some(u64::from_le_bytes(raw))
}

/// Remove um ZIP previamente anexado (se existir), devolvendo apenas o ELF/PE.
fn strip_appended_zip(exe_bytes: &mut Vec<u8>) {
    if let Some(offset) = read_appended_zip_offset(exe_bytes) {
        if (offset as usize) < exe_bytes.len() {
            exe_bytes.truncate(offset as usize);
        }
    }
}

// ---------------------------------------------------------------------------
// Opções de compressão por tipo de arquivo
// ---------------------------------------------------------------------------

fn compression_for(zip_path: &str) -> CompressionMethod {
    // Arquivos já comprimidos não se beneficiam de deflate
    let already_compressed = ["png", "jpg", "jpeg", "gif", "webp",
                               "ogg", "mp3", "flac", "spv"];
    let ext = Path::new(zip_path)
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("")
        .to_lowercase();
    if already_compressed.contains(&ext.as_str()) {
        CompressionMethod::Stored
    } else {
        CompressionMethod::Deflated
    }
}

fn file_options(zip_path: &str) -> SimpleFileOptions {
    let method = compression_for(zip_path);
    let opts = SimpleFileOptions::default().compression_method(method);
    
    // Aplica o nível de compressão apenas se o método for Deflated
    if method == CompressionMethod::Deflated {
        opts.compression_level(Some(6))
    } else {
        opts
    }
}

// ---------------------------------------------------------------------------
// Construção do ZIP
// ---------------------------------------------------------------------------

fn build_zip(
    main_script: &Path,
    modules: &[PathBuf],
    assets: &[PathBuf],
    libs: &[PathBuf],
    engine_bin: Option<&Path>,
) -> Result<Vec<u8>, Box<dyn Error>> {
    let buf: Vec<u8> = Vec::new();
    let cursor = std::io::Cursor::new(buf);
    let mut zip = ZipWriter::new(cursor);

    // --- Engine (modo bundle) ---
    if let Some(bin) = engine_bin {
        let data = fs::read(bin)?;
        println!("[Tupi] Engine: {} ({} bytes)", bin.display(), data.len());
        let opts = file_options(ENGINE_ENTRY);
        zip.start_file(ENGINE_ENTRY, opts)?;
        zip.write_all(&data)?;
    }

    // --- Script principal ---
    if !is_lua_file(main_script) {
        return Err(format!("script principal precisa ser .lua: {}", main_script.display()).into());
    }
    {
        let data = fs::read(main_script)?;
        println!("[Tupi] Main:   {} ({} bytes)", main_script.display(), data.len());
        let opts = file_options(MAIN_ENTRY);
        zip.start_file(MAIN_ENTRY, opts)?;
        zip.write_all(&data)?;
    }

    // --- Módulos Lua ---
    for module in modules {
        if !is_lua_file(module) {
            return Err(format!("módulo precisa ser .lua: {}", module.display()).into());
        }
        let zip_path = to_zip_script_path(module)?;
        let data = fs::read(module)?;
        println!("[Tupi] Script: {} → {} ({} bytes)", module.display(), zip_path, data.len());
        let opts = file_options(&zip_path);
        zip.start_file(&zip_path, opts)?;
        zip.write_all(&data)?;
    }

    // --- Bibliotecas dinâmicas ---
    for lib in libs {
        let zip_path = to_zip_lib_path(lib);
        let data = fs::read(lib)?;
        println!("[Tupi] Lib:    {} → {} ({} bytes)", lib.display(), zip_path, data.len());
        let opts = file_options(&zip_path);
        zip.start_file(&zip_path, opts)?;
        zip.write_all(&data)?;
    }

    // --- Assets ---
    for asset in assets {
        if !asset.exists() {
            return Err(format!("asset não encontrado: {}", asset.display()).into());
        }
        let zip_path = to_zip_asset_path(asset);
        let data = fs::read(asset)?;
        println!("[Tupi] Asset:  {} → {} ({} bytes)", asset.display(), zip_path, data.len());
        let opts = file_options(&zip_path);
        zip.start_file(&zip_path, opts)?;
        zip.write_all(&data)?;
    }

    let cursor = zip.finish()?;
    Ok(cursor.into_inner())
}

// ---------------------------------------------------------------------------
// Escrita do executável com ZIP anexado
// ---------------------------------------------------------------------------

fn write_appended_executable(exe_path: &Path, zip_bytes: &[u8]) -> Result<(), Box<dyn Error>> {
    let mut exe_bytes = fs::read(exe_path)?;
    strip_appended_zip(&mut exe_bytes);

    let zip_offset = exe_bytes.len() as u64;

    // Monta: ELF + ZIP + marcador(16) + offset_u64(8)
    let mut out = Vec::with_capacity(exe_bytes.len() + zip_bytes.len() + MARKER_TOTAL);
    out.extend_from_slice(&exe_bytes);
    out.extend_from_slice(zip_bytes);
    out.extend_from_slice(APPENDED_MARKER);
    out.extend_from_slice(&zip_offset.to_le_bytes());

    let tmp = exe_path.with_extension("tmp");
    fs::write(&tmp, &out)?;

    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let mode = fs::metadata(exe_path)?.permissions().mode();
        fs::set_permissions(&tmp, fs::Permissions::from_mode(mode))?;
    }

    fs::rename(&tmp, exe_path)?;
    println!(
        "[Tupi] ZIP ({} bytes) anexado em offset {} de '{}'.",
        zip_bytes.len(),
        zip_offset,
        exe_path.display()
    );
    Ok(())
}

fn write_archive_file(output_path: &Path, zip_bytes: &[u8]) -> Result<(), Box<dyn Error>> {
    if let Some(parent) = output_path.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::write(output_path, zip_bytes)?;
    println!(
        "[Tupi] Arquivo ZIP standalone criado: '{}' ({} bytes).",
        output_path.display(),
        zip_bytes.len()
    );
    Ok(())
}

// ---------------------------------------------------------------------------
// Coleta recursiva de arquivos
// ---------------------------------------------------------------------------

fn collect_assets(dir: &Path, out: &mut Vec<PathBuf>) -> Result<(), Box<dyn Error>> {
    for entry in fs::read_dir(dir)? {
        let path = entry?.path();
        if path.is_dir() {
            collect_assets(&path, out)?;
        } else if is_asset_file(&path) {
            out.push(path);
        }
    }
    Ok(())
}

fn collect_libs(dir: &Path, out: &mut Vec<PathBuf>) -> Result<(), Box<dyn Error>> {
    for entry in fs::read_dir(dir)? {
        let path = entry?.path();
        if path.is_file() && path.to_string_lossy().contains(".so") {
            out.push(path);
        }
    }
    Ok(())
}

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------

fn main() -> Result<(), Box<dyn Error>> {
    let mut args = env::args_os();
    let _prog = args.next();

    let mode = match args.next().and_then(|m| m.into_string().ok()).as_deref() {
        Some("append")  => OutputMode::Append,
        Some("archive") => OutputMode::Archive,
        Some("bundle")  => OutputMode::Bundle,
        _ => return Err("uso: tupi_pack <append|archive|bundle> ...".into()),
    };

    let target_path = args.next().map(PathBuf::from).ok_or("faltando <alvo>")?;

    match mode {
        // append <exe> <main.lua> [mods...] [--libs <dir>] [--assets <dir|arq...>]
        // archive <zip> <main.lua> [mods...] [--libs <dir>] [--assets <dir|arq...>]
        OutputMode::Append | OutputMode::Archive => {
            let main_script = args.next().map(PathBuf::from).ok_or("faltando <main.lua>")?;
            let mut modules: Vec<PathBuf> = Vec::new();
            let mut raw_assets: Vec<PathBuf> = Vec::new();
            let mut raw_libs: Vec<PathBuf> = Vec::new();
            let mut cur_flag = "";

            for arg in args {
                let p = PathBuf::from(&arg);
                match p.to_string_lossy().as_ref() {
                    "--libs"   => { cur_flag = "libs";   continue; }
                    "--assets" => { cur_flag = "assets"; continue; }
                    _          => {}
                }
                match cur_flag {
                    "libs"   => raw_libs.push(p),
                    "assets" => raw_assets.push(p),
                    _        => modules.push(p),
                }
            }

            let mut libs: Vec<PathBuf> = Vec::new();
            for item in &raw_libs {
                if item.is_dir() {
                    collect_libs(item, &mut libs)?;
                } else if item.is_file() {
                    libs.push(item.clone());
                } else {
                    return Err(format!("lib não encontrada: {}", item.display()).into());
                }
            }
            libs.sort();

            let mut assets: Vec<PathBuf> = Vec::new();
            for item in &raw_assets {
                if item.is_dir() {
                    collect_assets(item, &mut assets)?;
                } else if item.is_file() && is_asset_file(item) {
                    assets.push(item.clone());
                } else {
                    return Err(format!("asset não encontrado: {}", item.display()).into());
                }
            }
            assets.sort();

            let zip_bytes = build_zip(&main_script, &modules, &assets, &libs, None)?;

            match mode {
                OutputMode::Append => {
                    write_appended_executable(&target_path, &zip_bytes)?;
                    println!(
                        "[Tupi] append '{}' — {} scripts + {} libs + {} assets.",
                        target_path.display(),
                        modules.len() + 1,
                        libs.len(),
                        assets.len()
                    );
                }
                OutputMode::Archive => {
                    write_archive_file(&target_path, &zip_bytes)?;
                    println!(
                        "[Tupi] archive '{}' — {} scripts + {} libs + {} assets.",
                        target_path.display(),
                        modules.len() + 1,
                        libs.len(),
                        assets.len()
                    );
                }
                _ => unreachable!(),
            }
        }

        // bundle <saida> <bootstrapper> <engine_exe> <main.lua> [mods...]
        //        [--libs <lib_dir>] [--assets <dir|arq...>]
        OutputMode::Bundle => {
            let bootstrapper =
                args.next().map(PathBuf::from).ok_or("bundle: faltando <bootstrapper>")?;
            let engine_exe =
                args.next().map(PathBuf::from).ok_or("bundle: faltando <engine_exe>")?;
            let main_script =
                args.next().map(PathBuf::from).ok_or("bundle: faltando <main.lua>")?;

            let mut modules: Vec<PathBuf> = Vec::new();
            let mut raw_libs: Vec<PathBuf> = Vec::new();
            let mut raw_assets: Vec<PathBuf> = Vec::new();
            let mut cur_flag = "";

            for arg in args {
                let p = PathBuf::from(&arg);
                match p.to_string_lossy().as_ref() {
                    "--libs"   => { cur_flag = "libs";   continue; }
                    "--assets" => { cur_flag = "assets"; continue; }
                    _          => {}
                }
                match cur_flag {
                    "libs"   => raw_libs.push(p),
                    "assets" => raw_assets.push(p),
                    _        => modules.push(p),
                }
            }

            let mut libs: Vec<PathBuf> = Vec::new();
            for item in &raw_libs {
                if item.is_dir() {
                    collect_libs(item, &mut libs)?;
                } else if item.is_file() {
                    libs.push(item.clone());
                } else {
                    return Err(format!("lib não encontrada: {}", item.display()).into());
                }
            }
            libs.sort();

            let mut assets: Vec<PathBuf> = Vec::new();
            for item in &raw_assets {
                if item.is_dir() {
                    collect_assets(item, &mut assets)?;
                } else if item.is_file() && is_asset_file(item) {
                    assets.push(item.clone());
                } else {
                    return Err(format!("asset não encontrado: {}", item.display()).into());
                }
            }
            assets.sort();

            // Destino começa como cópia do bootstrapper
            fs::copy(&bootstrapper, &target_path)?;
            #[cfg(unix)]
            {
                use std::os::unix::fs::PermissionsExt;
                let mode = fs::metadata(&bootstrapper)?.permissions().mode();
                fs::set_permissions(&target_path, fs::Permissions::from_mode(mode | 0o111))?;
            }

            let zip_bytes =
                build_zip(&main_script, &modules, &assets, &libs, Some(&engine_exe))?;
            write_appended_executable(&target_path, &zip_bytes)?;

            println!(
                "[Tupi] bundle '{}'\n  engine={} libs={} scripts={} assets={}",
                target_path.display(),
                engine_exe.display(),
                libs.len(),
                modules.len() + 1,
                assets.len()
            );
        }
    }

    Ok(())
}