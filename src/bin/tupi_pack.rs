use std::env;
use std::error::Error;
use std::fs;
use std::path::{Path, PathBuf};

const FOOTER_MAGIC: &[u8; 16] = b"TUPI_SLEDGE_BIN1";
const PAYLOAD_MAGIC: &[u8; 16] = b"TUPI_LUA_PACK_V1";
const MAIN_ENTRY: &str = "__main__";
const FOOTER_SIZE: usize = FOOTER_MAGIC.len() + 8;

enum OutputMode {
    Append,
    Archive,
}

fn read_u64_le(bytes: &[u8]) -> u64 {
    let mut raw = [0u8; 8];
    raw.copy_from_slice(bytes);
    u64::from_le_bytes(raw)
}

fn is_lua_file(path: &Path) -> bool {
    matches!(path.extension().and_then(|ext| ext.to_str()), Some("lua"))
}

fn to_module_name(path: &Path) -> Result<String, Box<dyn Error>> {
    let normalized = path.to_string_lossy().replace('\\', "/");
    let without_ext = normalized
        .strip_suffix(".lua")
        .ok_or_else(|| format!("arquivo Lua invalido: {}", path.display()))?;

    Ok(without_ext.replace('/', "."))
}

fn strip_existing_payload(exe_bytes: &mut Vec<u8>) {
    if exe_bytes.len() < FOOTER_SIZE {
        return;
    }

    let footer_start = exe_bytes.len() - FOOTER_SIZE;
    if &exe_bytes[footer_start..footer_start + FOOTER_MAGIC.len()] != FOOTER_MAGIC {
        return;
    }

    let payload_size = read_u64_le(&exe_bytes[footer_start + FOOTER_MAGIC.len()..footer_start + FOOTER_SIZE]) as usize;
    if payload_size > footer_start {
        return;
    }

    let payload_start = footer_start - payload_size;
    if payload_size < PAYLOAD_MAGIC.len() + 4 {
        return;
    }

    if &exe_bytes[payload_start..payload_start + PAYLOAD_MAGIC.len()] != PAYLOAD_MAGIC {
        return;
    }

    exe_bytes.truncate(payload_start);
}

fn build_payload(main_script: &Path, modules: &[PathBuf]) -> Result<Vec<u8>, Box<dyn Error>> {
    let mut payload = Vec::new();
    let mut entries: Vec<(String, Vec<u8>)> = Vec::new();

    if !is_lua_file(main_script) {
        return Err(format!("script principal precisa ser .lua: {}", main_script.display()).into());
    }

    entries.push((MAIN_ENTRY.to_string(), fs::read(main_script)?));

    for module in modules {
        if !is_lua_file(module) {
            return Err(format!("modulo precisa ser .lua: {}", module.display()).into());
        }

        entries.push((to_module_name(module)?, fs::read(module)?));
    }

    payload.extend_from_slice(PAYLOAD_MAGIC);
    payload.extend_from_slice(&(entries.len() as u32).to_le_bytes());

    for (name, bytes) in entries {
        payload.extend_from_slice(&(name.len() as u32).to_le_bytes());
        payload.extend_from_slice(&(bytes.len() as u64).to_le_bytes());
        payload.extend_from_slice(name.as_bytes());
        payload.extend_from_slice(&bytes);
    }

    Ok(payload)
}

fn write_sledged_executable(exe_path: &Path, payload: &[u8]) -> Result<(), Box<dyn Error>> {
    let mut exe_bytes = fs::read(exe_path)?;
    let mut packed = Vec::new();
    let temp_path = exe_path.with_extension("tmp");

    strip_existing_payload(&mut exe_bytes);

    packed.reserve(exe_bytes.len() + payload.len() + FOOTER_SIZE);
    packed.extend_from_slice(&exe_bytes);
    packed.extend_from_slice(payload);
    packed.extend_from_slice(FOOTER_MAGIC);
    packed.extend_from_slice(&(payload.len() as u64).to_le_bytes());

    fs::write(&temp_path, &packed)?;

    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;

        let mode = fs::metadata(exe_path)?.permissions().mode();
        fs::set_permissions(&temp_path, fs::Permissions::from_mode(mode))?;
    }

    fs::rename(temp_path, exe_path)?;
    Ok(())
}

fn write_archive_file(output_path: &Path, payload: &[u8]) -> Result<(), Box<dyn Error>> {
    if let Some(parent) = output_path.parent() {
        fs::create_dir_all(parent)?;
    }

    fs::write(output_path, payload)?;
    Ok(())
}

fn main() -> Result<(), Box<dyn Error>> {
    let mut args = env::args_os();
    let _program = args.next();

    let mode = match args.next().and_then(|mode| mode.into_string().ok()) {
        Some(mode) if mode == "append" => OutputMode::Append,
        Some(mode) if mode == "archive" => OutputMode::Archive,
        Some(_) | None => {
            return Err("uso: tupi_pack <append|archive> <alvo> <main.lua> <modulos.lua...>".into());
        }
    };

    let target_path = args
        .next()
        .map(PathBuf::from)
        .ok_or("uso: tupi_pack <append|archive> <alvo> <main.lua> <modulos.lua...>")?;
    let main_script = args
        .next()
        .map(PathBuf::from)
        .ok_or("uso: tupi_pack <append|archive> <alvo> <main.lua> <modulos.lua...>")?;
    let modules: Vec<PathBuf> = args.map(PathBuf::from).collect();

    let payload = build_payload(&main_script, &modules)?;
    match mode {
        OutputMode::Append => {
            write_sledged_executable(&target_path, &payload)?;
            println!(
                "[Tupi] Sledging aplicado em {} com {} entradas Lua.",
                target_path.display(),
                modules.len() + 1
            );
        }
        OutputMode::Archive => {
            write_archive_file(&target_path, &payload)?;
            println!(
                "[Tupi] Arquivo Lua compactado em {} com {} entradas.",
                target_path.display(),
                modules.len() + 1
            );
        }
    }

    Ok(())
}
