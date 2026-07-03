/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

export type MojibakeArg = {
  name: string;
  type: string;
  description: string;
  wasm_generated: boolean;
};

export type MojibakeFunction = {
  comment: string;
  ret: string;
  name: string;
  attributes: string[];
  args: MojibakeArg[];
  wasm: boolean;
};

function buffer(description: string, name = 'buffer', isConst = true, wasm_generated = false): MojibakeArg {
  return {
    name,
    type: isConst ? 'const char *' : 'char *',
    description,
    wasm_generated
  };
}

function size(description = 'The size of the string, in bytes', name = 'size'): MojibakeArg {
  return {
    name,
    type: 'size_t',
    description,
    wasm_generated: true
  };
}

function encoding(description = 'The encoding of the string', name = 'encoding'): MojibakeArg {
  return {
    name,
    type: 'mjb_encoding',
    description: 'The encoding of the string',
    wasm_generated: false
  }
}

function codepoint(description = 'The codepoint to check', name = 'codepoint'): MojibakeArg {
  return {
    name,
    type: 'mjb_codepoint',
    description,
    wasm_generated: false
  }
}

export default [
  {
    comment: 'Return the codepoint character',
    ret: 'mjb_status',
    name: 'mjb_codepoint_character',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'character',
        type: 'mjb_character *',
        description: 'The character to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Normalize a string to NFC/NFKC/NFD/NFKD form',
    ret: 'mjb_status',
    name: 'mjb_normalize',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to normalize'),
      size(),
      encoding(),
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to use',
        wasm_generated: false
      },
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'result',
        type: 'mjb_result *',
        description: 'The pointer to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the next character from a string',
    ret: 'mjb_status',
    name: 'mjb_next_character',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'fn',
        type: 'mjb_next_character_fn',
        description: 'The function to call for each character',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Check if a string is normalized to NFC/NFKC/NFD/NFKD form',
    ret: 'mjb_quick_check_result',
    name: 'mjb_string_is_normalized',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to check',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Filter a string to remove invalid characters',
    ret: 'mjb_status',
    name: 'mjb_string_filter',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to filter'),
      size(),
      encoding(),
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'filters',
        type: 'mjb_filter',
        description: 'The filters to use',
        wasm_generated: false
      },
      {
        name: 'result',
        type: 'mjb_result *',
        description: 'The pointer to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return if a codepoint has a property',
    ret: 'mjb_status',
    name: 'mjb_codepoint_has_property',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'property',
        type: 'mjb_property',
        description: 'The property to check',
        wasm_generated: false
      },
      {
        name: 'value',
        type: 'uint8_t *',
        description: 'The property value, if any',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return all properties of a codepoint',
    ret: 'mjb_status',
    name: 'mjb_codepoint_properties',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'buffer',
        type: 'uint8_t *',
        description: 'The buffer to store the properties',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return a property value',
    ret: 'uint8_t',
    name: 'mjb_codepoint_property',
    attributes: [],
    args: [
      {
        name: 'properties',
        type: 'uint8_t *',
        description: 'The buffer to store the properties',
        wasm_generated: true
      },
      {
        name: 'property',
        type: 'mjb_property',
        description: 'The property to check',
        wasm_generated: false
      }
    ],
    wasm: false
  },
  {
    comment: 'Return the script of a codepoint',
    ret: 'mjb_script',
    name: 'mjb_codepoint_script',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return the string encoding (the most probable)',
    ret: 'mjb_encoding',
    name: 'mjb_string_encoding',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      size()
    ],
    wasm: true
  },
  {
    comment: 'Return true if the string is encoded in UTF-8',
    ret: 'bool',
    name: 'mjb_string_is_utf8',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      size()
    ],
    wasm: true
  },
  {
    comment: 'Return true if the string is encoded in UTF-16BE or UTF-16LE',
    ret: 'bool',
    name: 'mjb_string_is_utf16',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      size()
    ],
    wasm: true
  },
  {
    comment: 'Return true if the string is encoded in ASCII',
    ret: 'bool',
    name: 'mjb_string_is_ascii',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      size()
    ],
    wasm: true
  },
  {
    comment: 'Encode a codepoint to a string',
    ret: 'unsigned int',
    name: 'mjb_codepoint_encode',
    attributes: [],
    args: [
      codepoint('The codepoint to encode'),
      buffer('The buffer to encode the codepoint to', 'buffer', false, true),
      size('The size of the buffer, in bytes'),
      encoding('The encoding to use')
    ],
    wasm: true
  },
  {
    comment: 'Convert from an encoding to another',
    ret: 'mjb_status',
    name: 'mjb_string_convert_encoding',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to convert'),
      size(),
      encoding('The input encoding of the string'),
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'result',
        type: 'mjb_result *',
        description: 'The pointer to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the length of a string',
    ret: 'size_t',
    name: 'mjb_strnlen',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      {
        name: 'max_length',
        type: 'size_t',
        description: 'The maximum length of the string, in bytes',
        wasm_generated: true
      },
      encoding()
    ],
    wasm: true
  },
  {
    comment: 'Compare two strings using UCA',
    ret: 'int',
    name: 'mjb_string_compare',
    attributes: [],
    args: [
      buffer('The first string to compare', 's1'),
      size('The length of the first string, in bytes', 's1_length'),
      buffer('The second string to compare', 's2'),
      size('The length of the second string, in bytes', 's2_length'),
      encoding('The encoding of the strings'),
      {
        name: 'mode',
        type: 'mjb_collation_mode',
        description: 'The variable weighting strategy',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Generate a UCA sort key for a string',
    ret: 'mjb_status',
    name: 'mjb_collation_key',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to generate the sort key for'),
      size(),
      encoding(),
      {
        name: 'mode',
        type: 'mjb_collation_mode',
        description: 'The variable weighting strategy',
        wasm_generated: false
      },
      {
        name: 'result',
        type: 'mjb_result *',
        description: 'The pointer to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Change string case',
    ret: 'char *',
    name: 'mjb_case',
    attributes: [],
    args: [
      buffer('The string to change case'),
      size(),
      {
        name: 'type',
        type: 'mjb_case_type',
        description: 'The type of case change',
        wasm_generated: false
      },
      encoding()
    ],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is valid',
    ret: 'bool',
    name: 'mjb_codepoint_is_valid',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is graphic',
    ret: 'bool',
    name: 'mjb_codepoint_is_graphic',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is combining',
    ret: 'bool',
    name: 'mjb_codepoint_is_combining',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return if the codepoint is an hangul L',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_l',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: false
  },
  {
    comment: 'Return if the codepoint is an hangul V',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_v',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: false
  },
  {
    comment: 'Return if the codepoint is an hangul T',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_t',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: false
  },
  {
    comment: 'Return if the codepoint is an hangul jamo',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: false
  },
  {
    comment: 'Return if the codepoint is an hangul syllable',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_syllable',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return if the codepoint is CJK ideograph',
    ret: 'bool',
    name: 'mjb_codepoint_is_cjk_ideograph',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return if the codepoint is CJK extension',
    ret: 'bool',
    name: 'mjb_codepoint_is_cjk_ext',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the category is graphic',
    ret: 'bool',
    name: 'mjb_category_is_graphic',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'category',
        type: 'mjb_category',
        description: 'The category to check',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return true if the category is combining',
    ret: 'bool',
    name: 'mjb_category_is_combining',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'category',
        type: 'mjb_category',
        description: 'The category to check',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the numeric value of a codepoint',
    ret: 'mjb_status',
    name: 'mjb_codepoint_numeric_value',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'value',
        type: 'mjb_numeric_value *',
        description: 'The numeric value to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the character block',
    ret: 'mjb_status',
    name: 'mjb_codepoint_block',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'block',
        type: 'mjb_block_info *',
        description: 'The block to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the codepoint lowercase codepoint',
    ret: 'mjb_codepoint',
    name: 'mjb_codepoint_to_lowercase',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return the codepoint uppercase codepoint',
    ret: 'mjb_codepoint',
    name: 'mjb_codepoint_to_uppercase',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return the codepoint titlecase codepoint',
    ret: 'mjb_codepoint',
    name: 'mjb_codepoint_to_titlecase',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Unicode line break algorithm',
    ret: 'mjb_break_type',
    name: 'mjb_break_line',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_line_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Word cluster breaking',
    ret: 'mjb_break_type',
    name: 'mjb_break_word',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_word_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the number of bytes that form the first max_segments word-break segments',
    ret: 'size_t',
    name: 'mjb_truncate_word',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'max_segments',
        type: 'size_t',
        description: 'The maximum number of segments to return',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the number of bytes whose word-break segments fit within max_columns display columns',
    ret: 'size_t',
    name: 'mjb_truncate_word_width',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'context',
        type: 'mjb_width_context',
        description: 'The width context',
        wasm_generated: false
      },
      {
        name: 'max_columns',
        type: 'size_t',
        description: 'The maximum number of columns to return',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Sentence boundaries breaking',
    ret: 'mjb_break_type',
    name: 'mjb_break_sentence',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_sentence_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Grapheme cluster breaking',
    ret: 'mjb_break_type',
    name: 'mjb_segmentation',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the number of bytes that form the first max_graphemes grapheme cluster segments',
    ret: 'size_t',
    name: 'mjb_truncate',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'max_graphemes',
        type: 'size_t',
        description: 'The maximum number of graphemes to return',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the number of bytes whose grapheme clusters fit within max_columns display columns',
    ret: 'size_t',
    name: 'mjb_truncate_width',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'context',
        type: 'mjb_width_context',
        description: 'The width context',
        wasm_generated: false
      },
      {
        name: 'max_columns',
        type: 'size_t',
        description: 'The maximum number of columns to return',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Resolve bidirectional text (TR9) for a paragraph',
    ret: 'mjb_status',
    name: 'mjb_bidi_resolve',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The input string'),
      size(),
      encoding(),
      {
        name: 'direction',
        type: 'mjb_direction',
        description: 'The base paragraph direction (LTR, RTL, or AUTO for P2/P3)',
        wasm_generated: false
      },
      {
        name: 'result',
        type: 'mjb_bidi_paragraph *',
        description: 'Output paragraph; chars is library-allocated',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Free a bidi paragraph allocated by mjb_bidi_resolve',
    ret: 'void',
    name: 'mjb_bidi_free',
    attributes: [],
    args: [
      {
        name: 'paragraph',
        type: 'mjb_bidi_paragraph *',
        description: 'The paragraph to free',
        wasm_generated: false
      }
    ],
    wasm: false
  },
  {
    comment: 'Reorder a line visually (L1-L4); visual_order is caller-allocated',
    ret: 'mjb_status',
    name: 'mjb_bidi_reorder_line',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'paragraph',
        type: 'const mjb_bidi_paragraph *',
        description: 'The resolved paragraph',
        wasm_generated: false
      },
      {
        name: 'line_start',
        type: 'size_t',
        description: 'Start index into paragraph->chars',
        wasm_generated: false
      },
      {
        name: 'line_end',
        type: 'size_t',
        description: 'End index (exclusive) into paragraph->chars',
        wasm_generated: false
      },
      {
        name: 'visual_order',
        type: 'size_t *',
        description: 'Caller-allocated array of size (line_end - line_start)',
        wasm_generated: false
      }
    ],
    wasm: false
  },
  {
    comment: 'Compute visual level runs; pass runs=NULL to count first',
    ret: 'mjb_status',
    name: 'mjb_bidi_line_runs',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'paragraph',
        type: 'const mjb_bidi_paragraph *',
        description: 'The resolved paragraph',
        wasm_generated: false
      },
      {
        name: 'visual_order',
        type: 'const size_t *',
        description: 'Visual order array from mjb_bidi_reorder_line',
        wasm_generated: false
      },
      {
        name: 'count',
        type: 'size_t',
        description: 'Length of visual_order',
        wasm_generated: false
      },
      {
        name: 'runs',
        type: 'mjb_bidi_run *',
        description: 'Caller-allocated array, or NULL to only count',
        wasm_generated: false
      },
      {
        name: 'run_count',
        type: 'size_t *',
        description: 'On output: number of runs written (or total if runs=NULL)',
        wasm_generated: false
      }
    ],
    wasm: false
  },
  {
    comment: 'Return the plane of the codepoint',
    ret: 'mjb_plane',
    name: 'mjb_codepoint_plane',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the plane is valid',
    ret: 'bool',
    name: 'mjb_plane_is_valid',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'plane',
        type: 'mjb_plane',
        description: 'The plane to check',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the name of a plane, NULL if the place specified is not valid',
    ret: 'const char *',
    name: 'mjb_plane_name',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'plane',
        type: 'mjb_plane',
        description: 'The plane to check',
        wasm_generated: false
      },
      {
        name: 'abbreviation',
        type: 'bool',
        description: 'Whether to use an abbreviation',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is a valid Unicode identifier start (UAX#31 ID_Start)',
    ret: 'bool',
    name: 'mjb_codepoint_is_id_start',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is a valid Unicode identifier continuation (UAX#31 ID_Continue)',
    ret: 'bool',
    name: 'mjb_codepoint_is_id_continue',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is a valid NFKC identifier start (UAX#31 XID_Start)',
    ret: 'bool',
    name: 'mjb_codepoint_is_xid_start',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is a valid NFKC identifier continuation (UAX#31 XID_Continue)',
    ret: 'bool',
    name: 'mjb_codepoint_is_xid_continue',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is reserved for use in patterns (UAX#31 Pattern_Syntax)',
    ret: 'bool',
    name: 'mjb_codepoint_is_pattern_syntax',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint is pattern whitespace (UAX#31 Pattern_White_Space)',
    ret: 'bool',
    name: 'mjb_codepoint_is_pattern_white_space',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the string is a valid Unicode identifier (UAX#31)',
    ret: 'bool',
    name: 'mjb_string_is_identifier',
    attributes: [],
    args: [
      buffer('The string to validate'),
      size(),
      encoding(),
      {
        name: 'profile',
        type: 'mjb_identifier_profile',
        description: 'The identifier profile (DEFAULT or NFKC)',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the name of a property, NULL if the property specified is not valid',
    ret: 'const char *',
    name: 'mjb_property_name',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'property',
        type: 'mjb_property',
        description: 'The property to check',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return true if two strings are visually confusable (UTS#39 §4): skeleton(s1) == skeleton(s2)',
    ret: 'bool',
    name: 'mjb_string_is_confusable',
    attributes: [],
    args: [
      buffer('The first string', 's1'),
      size('The size of the first string, in bytes', 's1_size'),
      buffer('The second string', 's2'),
      size('The size of the second string, in bytes', 's2_size'),
      {
        name: 'encoding',
        type: 'mjb_encoding',
        description: 'The encoding of both strings',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the emoji properties',
    ret: 'mjb_status',
    name: 'mjb_codepoint_emoji',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'emoji',
        type: 'mjb_emoji_properties *',
        description: 'The emoji properties to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji property',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Presentation property',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_presentation',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Modifier property',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_modifier',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Modifier_Base property',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_modifier_base',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Component property',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_component',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return true if the codepoint has the Unicode Extended_Pictographic property',
    ret: 'bool',
    name: 'mjb_codepoint_is_extended_pictographic',
    attributes: [],
    args: [codepoint()],
    wasm: true
  },
  {
    comment: 'Return emoji sequence metadata for a complete string',
    ret: 'mjb_status',
    name: 'mjb_string_emoji_sequence',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to check'),
      size(),
      encoding(),
      {
        name: 'emoji',
        type: 'mjb_emoji_sequence *',
        description: 'The emoji sequence metadata to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return true if the complete string is an emoji sequence listed by Unicode, ' +
      'including standardized emoji variation sequences',
    ret: 'bool',
    name: 'mjb_string_is_emoji_sequence',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding()
    ],
    wasm: true
  },
  {
    comment: 'Return true if the complete string is an RGI emoji sequence, excluding plain ' +
      'standardized variation sequences',
    ret: 'bool',
    name: 'mjb_string_is_rgi_emoji',
    attributes: [],
    args: [
      buffer('The string to check'),
      size(),
      encoding()
    ],
    wasm: true
  },
  {
    comment: 'Return hangul syllable name',
    ret: 'mjb_status',
    name: 'mjb_hangul_syllable_name',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'buffer',
        type: 'char *',
        description: 'The buffer to store the result',
        wasm_generated: true
      },
      size()
    ],
    wasm: false
  },
  {
    comment: 'Hangul syllable decomposition',
    ret: 'mjb_status',
    name: 'mjb_hangul_syllable_decomposition',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'codepoints',
        type: 'mjb_codepoint *',
        description: 'The codepoints to store the result',
        wasm_generated: true
      }
    ],
    wasm: false
  },
  {
    comment: 'Hangul syllable composition',
    ret: 'size_t',
    name: 'mjb_hangul_syllable_composition',
    attributes: [],
    args: [
      {
        name: 'characters',
        type: 'mjb_buffer_character *',
        description: 'The characters to compose',
        wasm_generated: false
      },
      {
        name: 'characters_len',
        type: 'size_t',
        description: 'The length of the characters',
        wasm_generated: false
      }
    ],
    wasm: false
  },
  {
    comment: 'Return the east asian width of a codepoint',
    ret: 'mjb_status',
    name: 'mjb_codepoint_east_asian_width',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'width',
        type: 'mjb_east_asian_width *',
        description: 'The width to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Return the display width of a string',
    ret: 'mjb_status',
    name: 'mjb_display_width',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to normalize'),
      size(),
      encoding(),
      {
        name: 'context',
        type: 'mjb_width_context',
        description: 'The width context for ambiguous-width characters',
        wasm_generated: false
      },
      {
        name: 'width',
        type: 'size_t *',
        description: 'The width to store the result',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Parse a BCP 47 language tag',
    ret: 'mjb_status',
    name: 'mjb_locale_parse',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'id',
        type: 'const char *',
        description: 'The BCP 47 language tag to parse',
        wasm_generated: false
      },
      {
        name: 'size',
        type: 'size_t',
        description: 'The size of the locale identifier, in bytes',
        wasm_generated: true
      },
      {
        name: 'encoding',
        type: 'mjb_encoding',
        description: 'The encoding of the locale identifier',
        wasm_generated: false
      },
      {
        name: 'locale',
        type: 'mjb_locale_id *',
        description: 'The locale structure to store the result',
        wasm_generated: true
      },
      {
        name: 'error',
        type: 'mjb_error *',
        description: 'The error to store when parsing fails',
        wasm_generated: true
      }
    ],
    wasm: true
  },
  {
    comment: 'Set current locale',
    ret: 'mjb_status',
    name: 'mjb_locale_set',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'locale',
        type: 'unsigned int',
        description: 'The locale to set',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Output the current library version (MJB_VERSION)',
    ret: 'const char *',
    name: 'mjb_version',
    attributes: ['MJB_CONST'],
    args: [],
    wasm: true
  },
  {
    comment: 'Output the current library version number (MJB_VERSION_NUMBER)',
    ret: 'unsigned int',
    name: 'mjb_version_number',
    attributes: ['MJB_CONST'],
    args: [],
    wasm: true
  },
  {
    comment: 'Output the current supported unicode version (MJB_UNICODE_VERSION)',
    ret: 'const char *',
    name: 'mjb_unicode_version',
    attributes: ['MJB_CONST'],
    args: [],
    wasm: true
  },
  {
    comment: 'Initialize the library. Not needed to be called',
    ret: 'mjb_status',
    name: 'mjb_initialize',
    attributes: ['MJB_NODISCARD'],
    args: [],
    wasm: false
  },
  {
    comment: 'Initialize the library with custom values. Not needed to be called',
    ret: 'mjb_status',
    name: 'mjb_initialize_v2',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'alloc_fn',
        type: 'mjb_alloc_fn',
        description: 'The function to allocate memory',
        wasm_generated: false
      },
      {
        name: 'realloc_fn',
        type: 'mjb_realloc_fn',
        description: 'The function to reallocate memory',
        wasm_generated: false
      },
      {
        name: 'free_fn',
        type: 'mjb_free_fn',
        description: 'The function to free memory',
        wasm_generated: false
      }
    ],
    wasm: true
  },
  {
    comment: 'Shutdown the library. Not needed to be called',
    ret: 'void',
    name: 'mjb_shutdown',
    attributes: [],
    args: [],
    wasm: false
  },
  {
    comment: 'Allocate and zero memory',
    ret: 'void *',
    name: 'mjb_alloc',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'size',
        type: 'size_t',
        description: 'The size of the memory to allocate',
        wasm_generated: false
      }
    ],
    wasm: false
  },
  {
    comment: 'Reallocate memory',
    ret: 'void *',
    name: 'mjb_realloc',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'ptr',
        type: 'void *',
        description: 'The pointer to reallocate',
        wasm_generated: false
      },
      {
        name: 'new_size',
        type: 'size_t',
        description: 'The new size of the memory',
        wasm_generated: false
      }
    ],
    wasm: false
  },
  {
    comment: 'Free memory',
    ret: 'void',
    name: 'mjb_free',
    attributes: [],
    args: [
      {
        name: 'ptr',
        type: 'void *',
        description: 'The pointer to free',
        wasm_generated: false
      }
    ],
    wasm: false
  }
] as MojibakeFunction[];
