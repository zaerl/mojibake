# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

import ctypes
import os
import sys
from pathlib import Path

# This is a simple Python example of how to use the Mojibake library.
# Run `make example-python` from the examples directory before starting it.
# python3 ./python/example.py to run it.

# This is an example, will not use ctypeslib2 for importing all the library symbols but declare only
# what we need.

MJB_STATUS_OK = 0
MJB_ENC_UTF_8 = 0x2
MJB_NORMALIZATION_NFC = 0


class MjbResult(ctypes.Structure):
    _fields_ = [
        ("output", ctypes.c_void_p),
        ("output_size", ctypes.c_size_t),
        ("transformed", ctypes.c_bool),
    ]


def library_path() -> Path:
    build_dir = Path(__file__).resolve().parent.parent / "build"

    if os.name == "nt":
        return build_dir / "mojibake.dll"
    if sys.platform == "darwin":
        return build_dir / "libmojibake.dylib"
    return build_dir / "libmojibake.so"


def load_mojibake() -> ctypes.CDLL:
    library = ctypes.CDLL(str(library_path()))

    library.mjb_normalize.argtypes = [
        ctypes.c_char_p,
        ctypes.c_size_t,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.POINTER(MjbResult),
    ]

    library.mjb_normalize.restype = ctypes.c_int

    library.mjb_nfkc_casefold.argtypes = [
        ctypes.c_char_p,
        ctypes.c_size_t,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.POINTER(MjbResult),
    ]

    library.mjb_nfkc_casefold.restype = ctypes.c_int

    library.mjb_count_codepoints.argtypes = [
        ctypes.c_char_p,
        ctypes.c_size_t,
        ctypes.c_int,
    ]

    library.mjb_count_codepoints.restype = ctypes.c_size_t
    library.mjb_result_free.argtypes = [ctypes.POINTER(MjbResult)]
    library.mjb_result_free.restype = ctypes.c_int

    return library


def copy_result(library: ctypes.CDLL, result: MjbResult) -> bytes:
    try:
        return ctypes.string_at(result.output, result.output_size)
    finally:
        library.mjb_result_free(ctypes.byref(result))


def normalize(library: ctypes.CDLL, input_bytes: bytes) -> bytes:
    result = MjbResult()
    status = library.mjb_normalize(
        input_bytes,
        len(input_bytes),
        MJB_ENC_UTF_8,
        MJB_NORMALIZATION_NFC,
        MJB_ENC_UTF_8,
        ctypes.byref(result),
    )

    if status != MJB_STATUS_OK:
        raise RuntimeError(f"mjb_normalize failed with status {status}")

    return copy_result(library, result)


def nfkc_casefold(library: ctypes.CDLL, input_bytes: bytes) -> bytes:
    result = MjbResult()
    status = library.mjb_nfkc_casefold(
        input_bytes,
        len(input_bytes),
        MJB_ENC_UTF_8,
        MJB_ENC_UTF_8,
        ctypes.byref(result),
    )

    if status != MJB_STATUS_OK:
        raise RuntimeError(f"mjb_nfkc_casefold failed with status {status}")

    return copy_result(library, result)


def print_string(input_bytes: bytes) -> None:
    print(
        "".join(
            chr(byte) if 0x21 <= byte <= 0x7E else f"<{byte:02X}>"
            for byte in input_bytes
        )
    )


def main() -> int:
    library = load_mojibake()

    input_bytes = "Cafe\u0301".encode()

    # Normalize example: in NFC e + ◌́ -> é (U+00E9)
    normalized = normalize(library, input_bytes)

    # Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
    print_string(input_bytes)

    # Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
    print_string(normalized)

    mojibake = "文字化け".encode()

    # Codepoint count example: mjb_count_codepoints counts Unicode codepoints, not bytes.
    codepoint_count = library.mjb_count_codepoints(
        mojibake, len(mojibake), MJB_ENC_UTF_8
    )
    print(
        f'"{mojibake.decode()}" encoded in UTF-8 is {len(mojibake)} bytes long, '
        f"and {codepoint_count} codepoints long"
    )

    case_input = "Straße".encode()

    # NFKC casefold example: in NFKC casefold, ß -> ss
    casefolded = nfkc_casefold(library, case_input)
    print(f"{case_input.decode()} -> {casefolded.decode()}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
