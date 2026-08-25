mod policy;
mod protocol;
mod receipt;
mod relay;
mod transport;

use std::env;
use std::fs;
use std::io::{Read, Write};
use std::os::unix::fs::PermissionsExt;
use std::os::unix::net::UnixListener;
use std::path::{Path, PathBuf};

fn default_socket_path() -> PathBuf {
    if let Ok(runtime) = env::var("XDG_RUNTIME_DIR") {
        if !runtime.is_empty() {
            return Path::new(&runtime).join("nougat/privacy-broker-v1.sock");
        }
    }
    PathBuf::from("/tmp/nougat-privacy-broker-v1.sock")
}

fn main() -> std::io::Result<()> {
    let policy = policy::PrivacyPolicy::secure_search_baseline();
    if !policy.valid() {
        return Err(std::io::Error::new(std::io::ErrorKind::PermissionDenied, "privacy policy invalid"));
    }

    let path = default_socket_path();
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
        fs::set_permissions(parent, fs::Permissions::from_mode(0o700))?;
    }
    if path.exists() {
        fs::remove_file(&path)?;
    }
    let listener = UnixListener::bind(&path)?;
    fs::set_permissions(&path, fs::Permissions::from_mode(0o600))?;

    for connection in listener.incoming() {
        let mut stream = match connection {
            Ok(value) => value,
            Err(_) => continue,
        };
        let mut data = [0_u8; 1024];
        let count = match stream.read(&mut data) {
            Ok(value) => value,
            Err(_) => continue,
        };
        let line = String::from_utf8_lossy(&data[..count]);
        // Never print or persist client input. The v1 broker only handles
        // metadata/status requests and refuses remote query transport.
        let reply = protocol::handle_line(&line);
        let _ = stream.write_all(reply.as_bytes());
    }
    Ok(())
}
