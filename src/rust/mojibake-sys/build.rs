// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

//! Compile the Mojibake C library with the `cc` crate.
//!
//! The C sources are read from `MOJIBAKE_SRC_DIR` when it is set, else from the repository's
//! `src/` directory. The source list is taken from the generated `SOURCES` block of
//! `src/CMakeLists.txt`, so it cannot drift from the CMake build.

use std::env;
use std::fs;
use std::path::{Path, PathBuf};

const FEATURES: [(&str, &str); 4] = [
    ("CARGO_FEATURE_NAMES", "MJB_FEATURE_CHARACTER_NAMES"),
    ("CARGO_FEATURE_COLLATION", "MJB_FEATURE_COLLATION"),
    ("CARGO_FEATURE_IDNA", "MJB_FEATURE_IDNA"),
    ("CARGO_FEATURE_SECURITY", "MJB_FEATURE_SECURITY"),
];

fn source_dir() -> PathBuf {
    println!("cargo:rerun-if-env-changed=MOJIBAKE_SRC_DIR");

    if let Some(dir) = env::var_os("MOJIBAKE_SRC_DIR") {
        return PathBuf::from(dir);
    }

    let manifest_dir = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    // src/rust/mojibake-sys -> src
    manifest_dir.join("../..")
}

fn c_sources(src: &Path) -> Vec<PathBuf> {
    let cmake = src.join("CMakeLists.txt");
    let contents = fs::read_to_string(&cmake)
        .unwrap_or_else(|error| panic!("cannot read {}: {error}", cmake.display()));
    let start = contents
        .find("set(SOURCES")
        .unwrap_or_else(|| panic!("no SOURCES block in {}", cmake.display()));
    let block = &contents[start + "set(SOURCES".len()..];
    let end = block.find(')').expect("unterminated SOURCES block");

    block[..end]
        .split_whitespace()
        .filter(|name| name.ends_with(".c"))
        .map(|name| src.join(name))
        .collect()
}

fn main() {
    let src = source_dir();
    let sources = c_sources(&src);
    let manifest_dir = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let shim = manifest_dir.join("shim/layout.c");

    println!(
        "cargo:rerun-if-changed={}",
        src.join("CMakeLists.txt").display()
    );
    println!("cargo:rerun-if-changed={}", shim.display());

    for entry in fs::read_dir(&src)
        .expect("cannot read the C source directory")
        .flatten()
    {
        let path = entry.path();

        if matches!(path.extension().and_then(|e| e.to_str()), Some("c" | "h")) {
            println!("cargo:rerun-if-changed={}", path.display());
        }
    }

    let mut build = cc::Build::new();
    build
        .files(&sources)
        .file(&shim)
        .include(&src)
        .warnings(false);

    if build.get_compiler().is_like_msvc() {
        build.flag("/std:c11");
    } else {
        build.flag("-std=c11");
    }

    for (cargo_feature, define) in FEATURES {
        let enabled = env::var_os(cargo_feature).is_some();
        build.define(define, if enabled { "1" } else { "0" });
    }

    build.compile("mojibake");

    // Let dependent build scripts find the headers (DEP_MOJIBAKE_INCLUDE).
    println!("cargo:include={}", src.display());
}
