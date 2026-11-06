// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::process::ExitCode;

use mojibake::encoding::{self, Encoding, Malformed};
use mojibake::{Form, StrExt};

// This is a simple Rust example of how to use the Mojibake library through the safe `mojibake`
// crate in src/rust. Run `make example-rust` from the examples directory to compile it.
// ./build/example-rust to run it.
fn main() -> ExitCode {
    match run() {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("error: {error}");
            ExitCode::FAILURE
        }
    }
}

fn run() -> mojibake::Result<()> {
    let input = "Cafe\u{0301}";

    // Normalize example: in NFC e + ◌́ -> é (U+00E9)
    let normalized = input.normalize(Form::Nfc)?;

    // Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
    print_string(input.as_bytes());

    // Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
    print_string(normalized.as_bytes());

    let mojibake = "文字化け";

    // Codepoint count example: codepoint_count counts Unicode codepoints, not bytes.
    let codepoint_count =
        encoding::codepoint_count(mojibake.as_bytes(), Encoding::Utf8, Malformed::Stop)?;
    println!(
        "\"{mojibake}\" encoded in UTF-8 is {} bytes long, and {codepoint_count} codepoints long",
        mojibake.len()
    );

    let case_input = "Straße";

    // NFKC casefold example: in NFKC casefold, ß -> ss
    let casefolded = mojibake::nfkc_casefold(case_input)?;
    println!("{case_input} -> {casefolded}");

    Ok(())
}

fn print_string(input: &[u8]) {
    for byte in input {
        if (0x21..=0x7E).contains(byte) {
            print!("{}", char::from(*byte));
        } else {
            print!("<{byte:02X}>");
        }
    }

    println!();
}
