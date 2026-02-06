/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

export default [
  {
    "comment": "Return the codepoint character",
    "ret": "bool",
    "name": "codepoint_character",
    "attributes": [
      "MJB_NONNULL(2)"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      },
      {
        "name": "character",
        "type": "mjb_character *",
        "description": "The character to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Normalize a string to NFC/NFKC/NFD/NFKD form",
    "ret": "bool",
    "name": "normalize",
    "attributes": [
      "MJB_NONNULL(1, 5)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to normalize",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "form",
        "type": "mjb_normalization",
        "description": "The normalization form to use",
        "wasm_generated": false
      },
      {
        "name": "result",
        "type": "mjb_result *",
        "description": "The pointer to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the next character from a string",
    "ret": "bool",
    "name": "next_character",
    "attributes": [
      "MJB_NONNULL(1, 4)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "fn",
        "type": "mjb_next_character_fn",
        "description": "The function to call for each character",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Check if a string is normalized to NFC/NFKC/NFD/NFKD form",
    "ret": "mjb_quick_check_result",
    "name": "string_is_normalized",
    "attributes": [
      "MJB_NONNULL(1)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "form",
        "type": "mjb_normalization",
        "description": "The normalization form to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Filter a string to remove invalid characters",
    "ret": "bool",
    "name": "string_filter",
    "attributes": [
      "MJB_NONNULL(1, 6)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to filter",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "output_encoding",
        "type": "mjb_encoding",
        "description": "The output encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "filters",
        "type": "mjb_filter",
        "description": "The filters to use",
        "wasm_generated": false
      },
      {
        "name": "result",
        "type": "mjb_result *",
        "description": "The pointer to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return if a codepoint has a property",
    "ret": "bool",
    "name": "codepoint_has_property",
    "attributes": [],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      },
      {
        "name": "property",
        "type": "mjb_property",
        "description": "The property to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return all properties of a codepoint",
    "ret": "bool",
    "name": "codepoint_properties",
    "attributes": [],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      },
      {
        "name": "buffer",
        "type": "char *",
        "description": "The buffer to store the properties",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return a property value",
    "ret": "char",
    "name": "codepoint_property",
    "attributes": [],
    "args": [
      {
        "name": "properties",
        "type": "char *",
        "description": "The buffer to store the properties",
        "wasm_generated": true
      },
      {
        "name": "property",
        "type": "mjb_property",
        "description": "The property to check",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Return the string encoding (the most probable)",
    "ret": "mjb_encoding",
    "name": "string_encoding",
    "attributes": [
      "MJB_PURE"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the string is encoded in UTF-8",
    "ret": "bool",
    "name": "string_is_utf8",
    "attributes": [
      "MJB_PURE"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the string is encoded in UTF-16BE or UTF-16LE",
    "ret": "bool",
    "name": "string_is_utf16",
    "attributes": [
      "MJB_PURE"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the string is encoded in ASCII",
    "ret": "bool",
    "name": "string_is_ascii",
    "attributes": [
      "MJB_PURE"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Encode a codepoint to a string",
    "ret": "unsigned int",
    "name": "codepoint_encode",
    "attributes": [],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to encode",
        "wasm_generated": false
      },
      {
        "name": "buffer",
        "type": "char *",
        "description": "The buffer to encode the codepoint to",
        "wasm_generated": true
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the buffer, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding to use",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Convert from an encoding to another",
    "ret": "bool",
    "name": "string_convert_encoding",
    "attributes": [
      "MJB_NONNULL(1, 5)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to convert",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The input encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "output_encoding",
        "type": "mjb_encoding",
        "description": "The output encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "result",
        "type": "mjb_result *",
        "description": "The pointer to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the length of a string",
    "ret": "size_t",
    "name": "strnlen",
    "attributes": [
      "MJB_PURE"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "max_length",
        "type": "size_t",
        "description": "The maximum length of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Compare two strings",
    "ret": "int",
    "name": "string_compare",
    "attributes": [
      "MJB_PURE",
      "MJB_NONNULL(1, 4)"
    ],
    "args": [
      {
        "name": "s1",
        "type": "const char *",
        "description": "The first string to compare",
        "wasm_generated": false
      },
      {
        "name": "s1_length",
        "type": "size_t",
        "description": "The length of the first string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "s1_encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the first string",
        "wasm_generated": false
      },
      {
        "name": "s2",
        "type": "const char *",
        "description": "The second string to compare",
        "wasm_generated": false
      },
      {
        "name": "s2_length",
        "type": "size_t",
        "description": "The length of the second string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "s2_encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the second string",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Change string case",
    "ret": "char *",
    "name": "case",
    "attributes": [
      "MJB_NONNULL(1)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to change case",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The length of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "type",
        "type": "mjb_case_type",
        "description": "The type of case change",
        "wasm_generated": false
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the codepoint is valid",
    "ret": "bool",
    "name": "codepoint_is_valid",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the codepoint is graphic",
    "ret": "bool",
    "name": "codepoint_is_graphic",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the codepoint is combining",
    "ret": "bool",
    "name": "codepoint_is_combining",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return if the codepoint is an hangul L",
    "ret": "bool",
    "name": "codepoint_is_hangul_l",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Return if the codepoint is an hangul V",
    "ret": "bool",
    "name": "codepoint_is_hangul_v",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Return if the codepoint is an hangul T",
    "ret": "bool",
    "name": "codepoint_is_hangul_t",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Return if the codepoint is an hangul jamo",
    "ret": "bool",
    "name": "codepoint_is_hangul_jamo",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Return if the codepoint is an hangul syllable",
    "ret": "bool",
    "name": "codepoint_is_hangul_syllable",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return if the codepoint is CJK ideograph",
    "ret": "bool",
    "name": "codepoint_is_cjk_ideograph",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the category is graphic",
    "ret": "bool",
    "name": "category_is_graphic",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "category",
        "type": "mjb_category",
        "description": "The category to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the category is combining",
    "ret": "bool",
    "name": "category_is_combining",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "category",
        "type": "mjb_category",
        "description": "The category to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the character block",
    "ret": "bool",
    "name": "codepoint_block",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      },
      {
        "name": "block",
        "type": "mjb_block_info *",
        "description": "The block to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the codepoint lowercase codepoint",
    "ret": "mjb_codepoint",
    "name": "codepoint_to_lowercase",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the codepoint uppercase codepoint",
    "ret": "mjb_codepoint",
    "name": "codepoint_to_uppercase",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the codepoint titlecase codepoint",
    "ret": "mjb_codepoint",
    "name": "codepoint_to_titlecase",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Unicode line break algorithm",
    "ret": "mjb_line_break *",
    "name": "break_line",
    "attributes": [
      "MJB_NONNULL(1, 4)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "output_size",
        "type": "size_t *",
        "description": "The size of the output",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Word and grapheme cluster breaking",
    "ret": "bool",
    "name": "segmentation",
    "attributes": [
      "MJB_NONNULL(1, 4)"
    ],
    "args": [
      {
        "name": "buffer",
        "type": "const char *",
        "description": "The string to check",
        "wasm_generated": false
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the string, in bytes",
        "wasm_generated": true
      },
      {
        "name": "encoding",
        "type": "mjb_encoding",
        "description": "The encoding of the string",
        "wasm_generated": false
      },
      {
        "name": "state",
        "type": "mjb_next_state *",
        "description": "The state to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the plane of the codepoint",
    "ret": "mjb_plane",
    "name": "codepoint_plane",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return true if the plane is valid",
    "ret": "bool",
    "name": "plane_is_valid",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "plane",
        "type": "mjb_plane",
        "description": "The plane to check",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the name of a plane, NULL if the place specified is not valid",
    "ret": "const char *",
    "name": "plane_name",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [
      {
        "name": "plane",
        "type": "mjb_plane",
        "description": "The plane to check",
        "wasm_generated": false
      },
      {
        "name": "abbreviation",
        "type": "bool",
        "description": "Whether to use an abbreviation",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return the emoji properties",
    "ret": "bool",
    "name": "codepoint_emoji",
    "attributes": [
      "MJB_NONNULL(2)"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint to check",
        "wasm_generated": false
      },
      {
        "name": "emoji",
        "type": "mjb_emoji_properties *",
        "description": "The emoji properties to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": true
  },
  {
    "comment": "Return hangul syllable name",
    "ret": "bool",
    "name": "hangul_syllable_name",
    "attributes": [
      "MJB_NONNULL(2)"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      },
      {
        "name": "buffer",
        "type": "char *",
        "description": "The buffer to store the result",
        "wasm_generated": true
      },
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the buffer",
        "wasm_generated": true
      }
    ],
    "wasm": false
  },
  {
    "comment": "Hangul syllable decomposition",
    "ret": "bool",
    "name": "hangul_syllable_decomposition",
    "attributes": [
      "MJB_NODISCARD",
      "MJB_NONNULL(2)"
    ],
    "args": [
      {
        "name": "codepoint",
        "type": "mjb_codepoint",
        "description": "The codepoint",
        "wasm_generated": false
      },
      {
        "name": "codepoints",
        "type": "mjb_codepoint *",
        "description": "The codepoints to store the result",
        "wasm_generated": true
      }
    ],
    "wasm": false
  },
  {
    "comment": "Hangul syllable composition",
    "ret": "size_t",
    "name": "hangul_syllable_composition",
    "attributes": [
      "MJB_NONNULL(1)"
    ],
    "args": [
      {
        "name": "characters",
        "type": "mjb_buffer_character *",
        "description": "The characters to compose",
        "wasm_generated": false
      },
      {
        "name": "characters_len",
        "type": "size_t",
        "description": "The length of the characters",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Output the current library version (MJB_VERSION)",
    "ret": "const char *",
    "name": "version",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [],
    "wasm": true
  },
  {
    "comment": "Output the current library version number (MJB_VERSION_NUMBER)",
    "ret": "unsigned int",
    "name": "version_number",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [],
    "wasm": true
  },
  {
    "comment": "Output the current supported unicode version (MJB_UNICODE_VERSION)",
    "ret": "const char *",
    "name": "unicode_version",
    "attributes": [
      "MJB_CONST"
    ],
    "args": [],
    "wasm": true
  },
  {
    "comment": "Initialize the library. Not needed to be called",
    "ret": "bool",
    "name": "initialize",
    "attributes": [
      "MJB_NODISCARD"
    ],
    "args": [],
    "wasm": false
  },
  {
    "comment": "Initialize the library with custom values. Not needed to be called",
    "ret": "bool",
    "name": "initialize_v2",
    "attributes": [
      "MJB_NODISCARD"
    ],
    "args": [
      {
        "name": "alloc_fn",
        "type": "mjb_alloc_fn",
        "description": "The function to allocate memory",
        "wasm_generated": false
      },
      {
        "name": "realloc_fn",
        "type": "mjb_realloc_fn",
        "description": "The function to reallocate memory",
        "wasm_generated": false
      },
      {
        "name": "free_fn",
        "type": "mjb_free_fn",
        "description": "The function to free memory",
        "wasm_generated": false
      },
      {
        "name": "db",
        "type": "const char *",
        "description": "The database content of path to use",
        "wasm_generated": false
      },
      {
        "name": "db_size",
        "type": "size_t",
        "description": "The size of the database content",
        "wasm_generated": false
      }
    ],
    "wasm": true
  },
  {
    "comment": "Shutdown the library. Not needed to be called",
    "ret": "void",
    "name": "shutdown",
    "attributes": [],
    "args": [],
    "wasm": false
  },
  {
    "comment": "Allocate and zero memory",
    "ret": "void *",
    "name": "alloc",
    "attributes": [
      "MJB_NODISCARD"
    ],
    "args": [
      {
        "name": "size",
        "type": "size_t",
        "description": "The size of the memory to allocate",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Reallocate memory",
    "ret": "void *",
    "name": "realloc",
    "attributes": [
      "MJB_NODISCARD",
      "MJB_NONNULL(1)"
    ],
    "args": [
      {
        "name": "ptr",
        "type": "void *",
        "description": "The pointer to reallocate",
        "wasm_generated": false
      },
      {
        "name": "new_size",
        "type": "size_t",
        "description": "The new size of the memory",
        "wasm_generated": false
      }
    ],
    "wasm": false
  },
  {
    "comment": "Free memory",
    "ret": "void",
    "name": "free",
    "attributes": [
      "MJB_NONNULL(1)"
    ],
    "args": [
      {
        "name": "ptr",
        "type": "void *",
        "description": "The pointer to free",
        "wasm_generated": false
      }
    ],
    "wasm": false
  }
]
