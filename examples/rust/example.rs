// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::os::raw::{c_char, c_int};
use std::process::ExitCode;
use std::slice;

const MJB_STATUS_OK: c_int = 0;
const MJB_ENC_UTF_8: c_int = 0x2;
const MJB_NORMALIZATION_NFC: c_int = 0;

#[repr(C)]
struct MjbResult {
    output: *mut c_char,
    output_size: usize,
    transformed: bool,
}

impl MjbResult {
    fn new() -> Self {
        Self {
            output: std::ptr::null_mut(),
            output_size: 0,
            transformed: false,
        }
    }

    fn as_bytes(&self) -> &[u8] {
        if self.output_size == 0 {
            return &[];
        }

        unsafe { slice::from_raw_parts(self.output.cast(), self.output_size) }
    }
}

impl Drop for MjbResult {
    fn drop(&mut self) {
        unsafe {
            mjb_result_free(self);
        }
    }
}

extern "C" {
    fn mjb_normalize(
        buffer: *const c_char,
        byte_length: usize,
        encoding: c_int,
        form: c_int,
        output_encoding: c_int,
        result: *mut MjbResult,
    ) -> c_int;

    fn mjb_nfkc_casefold(
        buffer: *const c_char,
        byte_length: usize,
        encoding: c_int,
        output_encoding: c_int,
        result: *mut MjbResult,
    ) -> c_int;

    fn mjb_codepoint_count(
        buffer: *const c_char,
        byte_length: usize,
        encoding: c_int,
        count: *mut usize,
    ) -> c_int;

    fn mjb_result_free(result: *mut MjbResult) -> c_int;
}

// This is a simple Rust example of how to use the Mojibake library.
// Run `make example-rust` from the examples directory to compile it.
// ./build/example-rust to run it.
fn main() -> ExitCode {
    if run() {
        ExitCode::SUCCESS
    } else {
        ExitCode::FAILURE
    }
}

fn run() -> bool {
    let input = "Cafe\u{0301}".as_bytes();
    let mut normalized = MjbResult::new();

    // Normalize example: in NFC e + ◌́ -> é (U+00E9)
    if unsafe {
        mjb_normalize(
            input.as_ptr().cast(),
            input.len(),
            MJB_ENC_UTF_8,
            MJB_NORMALIZATION_NFC,
            MJB_ENC_UTF_8,
            &mut normalized,
        )
    } != MJB_STATUS_OK
    {
        return false;
    }

    // Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
    print_string(input);

    // Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
    print_string(normalized.as_bytes());

    let mojibake = "文字化け";

    // Codepoint count example: mjb_codepoint_count counts Unicode codepoints, not bytes.
    let mut codepoint_count: usize = 0;
    let status = unsafe {
        mjb_codepoint_count(
            mojibake.as_ptr().cast(),
            mojibake.len(),
            MJB_ENC_UTF_8,
            &mut codepoint_count,
        )
    };
    assert_eq!(status, MJB_STATUS_OK, "mjb_codepoint_count failed");
    println!(
        "\"{mojibake}\" encoded in UTF-8 is {} bytes long, and {codepoint_count} codepoints long",
        mojibake.len()
    );

    let case_input = "Straße";
    let mut casefolded = MjbResult::new();

    // NFKC casefold example: in NFKC casefold, ß -> ss
    if unsafe {
        mjb_nfkc_casefold(
            case_input.as_ptr().cast(),
            case_input.len(),
            MJB_ENC_UTF_8,
            MJB_ENC_UTF_8,
            &mut casefolded,
        )
    } != MJB_STATUS_OK
    {
        return false;
    }

    println!(
        "{case_input} -> {}",
        String::from_utf8_lossy(casefolded.as_bytes())
    );

    true
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
