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
  is_enum?: boolean;
  // Memory-ownership note.
  ownership?: string;
};

// Return value.
export type MojibakeReturnCase = {
  value: string;
  description: string;
};

// Reference to a Unicode specification (UAX/UTS) or other.
export type MojibakeSpecRef = {
  name: string;
  url: string;
};

export enum Section {
  TextTransformation,
  TextAnalysis,
  SortingComparison,
  Formatting,
  Security,
  Segmentation,
  Bidirectional,
  Emoji,
  TerminalWidth,
  HangulLanguage,
  Utility
}

export type MojibakeFunction = {
  comment: string;
  ret: string;
  name: string;
  attributes: string[];
  args: MojibakeArg[];
  variadic?: boolean;
  wasm: boolean;
  wasmName?: string;
  section: Section;
  // Long-form description.
  details?: string;
  // Documented return values.
  returns?: MojibakeReturnCase[];
  // Compilable C example. See utils/generate/generate-examples.ts
  example?: string;
  // Preprocessor feature required to compile and run the example.
  exampleFeature?: string;
  // Names of related public functions.
  related?: string[];
  // Unicode specifications implemented or referenced by the function.
  specs?: MojibakeSpecRef[];
};

const unicodeVersion = '18.0.0';

const uaxRevisions: Record<number, number> = {
  9: 51,
  11: 45,
  14: 56,
  15: 57,
  24: 40,
  29: 48,
  31: 44,
  44: 36
};

const utsRevisions: Record<number, number> = {
  10: 54,
  39: 33,
  46: 35,
  51: 30
};

function buffer(description: string, name = 'buffer', isConst = true, wasm_generated = false): MojibakeArg {
  return {
    name,
    type: isConst ? 'const char *' : 'char *',
    description,
    wasm_generated
  };
}

function byte_length(description = 'The length of the string in bytes, or ' +
  '`MJB_NUL_TERMINATED` to determine it from an encoding-aware NUL code unit',
  name = 'byte_length'): MojibakeArg {
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
    description,
    wasm_generated: false,
    is_enum: true
  }
}

function malformedPolicy(description = 'How malformed code-unit sequences are handled'): MojibakeArg {
  return {
    name: 'malformed_policy',
    type: 'mjb_malformed_policy',
    description,
    wasm_generated: false,
    is_enum: true
  };
}

function diagnostic(description = 'Where to store the first malformed-input diagnostic, or NULL'): MojibakeArg {
  return {
    name: 'diagnostic',
    type: 'mjb_diagnostic *',
    description,
    wasm_generated: true
  };
}

function codepoint(description = 'The codepoint to check', name = 'codepoint'): MojibakeArg {
  return {
    name,
    type: 'mjb_codepoint',
    description,
    wasm_generated: false
  }
}

function result(description = 'The pointer to store the result'): MojibakeArg {
  return {
    name: 'result',
    type: 'mjb_result *',
    description,
    wasm_generated: true,
    ownership: 'If `result->transformed` is true, `result->output` is library-allocated and ' +
      'must be freed with `mjb_result_free(result)`'
  };
}

function uax(number: number, title: string): MojibakeSpecRef {
  const revision = uaxRevisions[number];

  if(!revision) {
    throw new Error(`Missing Unicode ${unicodeVersion} UAX #${number} revision`);
  }

  return {
    name: `UAX #${number}: ${title}, Unicode ${unicodeVersion}`,
    url: `https://www.unicode.org/reports/tr${number}/tr${number}-${revision}.html`
  };
}

function uts(number: number, title: string): MojibakeSpecRef {
  const revision = utsRevisions[number];

  if(!revision) {
    throw new Error(`Missing Unicode ${unicodeVersion} UTS #${number} revision`);
  }

  return {
    name: `UTS #${number}: ${title}, Unicode ${unicodeVersion}`,
    url: `https://www.unicode.org/reports/tr${number}/tr${number}-${revision}.html`
  };
}

function unicodeCore(section: string, title: string, anchor: string): MojibakeSpecRef {
  return {
    name: `The Unicode Standard, Version ${unicodeVersion}, ${section}: ${title}`,
    url: `https://www.unicode.org/versions/Unicode${unicodeVersion}/core-spec/chapter-3/#${anchor}`
  };
}

export default [
  {
    comment: 'Return the codepoint character.',
    ret: 'mjb_status',
    name: 'mjb_codepoint_info',
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
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Fill `character` with the Unicode Character Database record of a codepoint: name, ' +
      'category, combining class, bidirectional category, decomposition, numeric values, ' +
      'mirrored flag, and simple case mappings. When the library is compiled with ' +
      '`MJB_FEATURE_CHARACTER_NAMES=0` the name field is reported as `Codepoint U+XXXX`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The character was found and filled' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`character` is NULL or the codepoint is not valid' },
      { value: 'MJB_STATUS_NOT_FOUND', description: 'The codepoint is not assigned' }
    ],
    example: `mjb_character character;

if(mjb_codepoint_info(0x022A, &character) != MJB_STATUS_OK) {
    return 1;
}

// U+022A lowercase: U+022B
printf("U+%04X lowercase: U+%04X", character.codepoint, character.lowercase);`,
    related: ['mjb_codepoint_block', 'mjb_codepoint_script', 'mjb_codepoint_property_binary',
      'mjb_codepoint_property_int'],
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Normalize a string to NFC/NFKC/NFD/NFKD form.',
    ret: 'mjb_status',
    name: 'mjb_normalize',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to normalize'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to use',
        wasm_generated: false,
        is_enum: true
      },
      encoding('The output encoding of the string', 'output_encoding'),
      result(),
      diagnostic()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Normalize a string to the requested Unicode normalization form. If the input is ' +
      'already normalized and no encoding conversion is needed, the input buffer is returned ' +
      'as-is in `result->output` with `result->transformed` set to false, without allocating. ' +
      'Malformed subsequences follow `malformed_policy`, and `diagnostic` records the first one.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The string was normalized (or already normal)' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_INVALID_FORM', description: '`form` is not NFC, NFD, NFKC, or NFKD' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "Cafe\\xCC\\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
mjb_result result;

if(mjb_normalize(input, MJB_NUL_TERMINATED, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// NFC: Café
printf("NFC: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);`,
    related: ['mjb_normalize_into', 'mjb_normalization_quick_check', 'mjb_filter',
      'mjb_filter_into'],
    specs: [uax(15, 'Unicode Normalization Forms')]
  },
  {
    comment: 'Normalize a string into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_normalize_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to normalize'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to use',
        wasm_generated: false,
        is_enum: true
      },
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Normalize a string using the same Unicode normalization forms and encoding rules ' +
      'as `mjb_normalize`. Set `output` to NULL to query the required size. If `output` is ' +
      'non-NULL, `*output_size` supplies its capacity; on return it contains the required size ' +
      'when the buffer is too small, or the written size on success. Terminators are excluded ' +
      'from the byte count and are not written. No bytes are written when capacity is ' +
      'insufficient. NFD and NFKD write without allocation. NFC and NFKC may allocate temporary ' +
      'composition storage, including during a size query.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the normalized string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output_size` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_INVALID_FORM', description:
        '`form` is not NFC, NFD, NFKC, or NFKD' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent a normalized codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Temporary composition allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' }
    ],
    example: `const char *input = "Cafe\\xCC\\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
size_t output_size = 0;

if(mjb_normalize_into(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, NULL, &output_size, NULL) != MJB_STATUS_OK) {
    return 1;
}

char output[5];

if(output_size > sizeof(output) || mjb_normalize_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, output, &output_size,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// NFC payload (no terminator): Café
printf("NFC payload (no terminator): %.*s", (int)output_size, output);`,
    related: ['mjb_normalize', 'mjb_normalization_quick_check', 'mjb_filter_into',
      'mjb_nfkc_casefold_into'],
    specs: [uax(15, 'Unicode Normalization Forms')]
  },
  {
    comment: 'Filter a string with the selected mjb_filter_flags.',
    ret: 'mjb_status',
    name: 'mjb_filter',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to filter'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'filters',
        type: 'mjb_filter_flags',
        description: 'The filters to use',
        wasm_generated: false
      },
      encoding('The output encoding of the string', 'output_encoding'),
      result(),
      diagnostic()
    ],
    details: '`MJB_FILTER_LIMIT_COMBINING` removes combining marks after the first ' +
      '`MJB_FILTER_MAX_COMBINING_MARKS` consecutive marks in an emitted run. This is useful ' +
      'for reducing Zalgo-style text while keeping ordinary accents and stacked marks. ' +
      'Malformed subsequences are stopped, replaced, or skipped according to ' +
      '`malformed_policy`; `diagnostic` records the first one encountered.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The filtered string was returned' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent a filtered codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    wasm: true,
    section: Section.TextTransformation,
    example: `const char *mixed_whitespace = "Hello\\t\\t\\n\\nworld";
mjb_result result;

if(mjb_filter(mixed_whitespace, strlen(mixed_whitespace), MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP, MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, &result,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: Hello world
printf("Filtered: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);

const char *controls = "\\x1\\x2\\t\\n\\v\\f\\r\\x1f";

if(mjb_filter(controls, strlen(controls), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_FILTER_CONTROLS, MJB_ENC_UTF_8, &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: \\t\\n\\v\\f\\r
printf("Filtered: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);`,
    related: ['mjb_filter_into', 'mjb_normalize']
  },
  {
    comment: 'Filter a string into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_filter_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to filter'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'filters',
        type: 'mjb_filter_flags',
        description: 'The filters to use',
        wasm_generated: false
      },
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same filters as `mjb_filter` without allocating the final output buffer. ' +
      'Set `output` to NULL to query the required size. If `output` is non-NULL, `*output_size` ' +
      'supplies its capacity; on return it contains the required size when the buffer is too ' +
      'small, or the written size on success. Terminators are excluded from the byte count and ' +
      'are not written. No bytes are written when capacity is insufficient. Filtering itself ' +
      'does not allocate, but `MJB_FILTER_NORMALIZE` may allocate temporary normalization ' +
      'storage. `MJB_FILTER_LIMIT_COMBINING` keeps the first ' +
      '`MJB_FILTER_MAX_COMBINING_MARKS` consecutive marks in each emitted run. Malformed ' +
      'subsequences follow `malformed_policy`, and `diagnostic` records the first one.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the filtered string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output_size` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent a filtered codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description:
        'Temporary normalization allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' }
    ],
    example: `const char *input = "Hello\\t\\t\\nworld";
size_t output_size = 0;

if(mjb_filter_into(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, NULL, &output_size,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

char output[11];

if(output_size > sizeof(output) || mjb_filter_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP, MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, output,
    &output_size, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Filtered payload (no terminator): Hello world
printf("Filtered payload (no terminator): %.*s", (int)output_size, output);`,
    related: ['mjb_filter', 'mjb_normalize']
  },
  {
    comment: 'Apply the Unicode NFKC_Casefold transform to a string.',
    ret: 'mjb_status',
    name: 'mjb_nfkc_casefold',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to transform'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      encoding('The output encoding of the string', 'output_encoding'),
      result(),
      diagnostic()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Apply the normative `NFKC_Casefold` mapping and normalize the result to NFC. ' +
      'This transform performs compatibility folding, full default case folding, and removal ' +
      'of default-ignorable codepoints. It is intended for identifier comparison and is not ' +
      'locale-sensitive. Malformed subsequences follow `malformed_policy`, and `diagnostic` ' +
      'records the first one.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The transformed string was returned' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_UNSUPPORTED', description: 'The transform did not stabilize' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "Stra\\xC3\\x9F" "e\\xC2\\xAD";
mjb_result result;

if(mjb_nfkc_casefold(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_ENC_UTF_8, &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// strasse
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
    related: ['mjb_nfkc_casefold_into', 'mjb_normalize', 'mjb_map_case', 'mjb_is_identifier'],
    specs: [
      unicodeCore('Section 3.13', 'Default Case Algorithms', 'G33992'),
      uax(31, 'Unicode Identifiers and Syntax'),
      uax(44, 'Unicode Character Database')
    ]
  },
  {
    comment: 'Apply the Unicode NFKC_Casefold transform into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_nfkc_casefold_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to transform'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same normative `NFKC_Casefold` transform as `mjb_nfkc_casefold`. Set ' +
      '`output` to NULL to query the required size. If `output` is non-NULL, `*output_size` ' +
      'supplies its capacity; on return it contains the required size when the buffer is too ' +
      'small, or the written size on success. Terminators are excluded from the byte count and ' +
      'are not written. No bytes are written when capacity is insufficient. The final output ' +
      'uses caller-provided storage, but the normalization and folding passes require temporary ' +
      'allocations, including during a size query.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the transformed string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output_size` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_UNSUPPORTED', description: 'The transform did not stabilize' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Temporary allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' }
    ],
    example: `const char *input = "Stra\\xC3\\x9F" "e\\xC2\\xAD";
size_t output_size = 0;

if(mjb_nfkc_casefold_into(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_ENC_UTF_8, NULL, &output_size, NULL) != MJB_STATUS_OK) {
    return 1;
}

char output[7];

if(output_size > sizeof(output) || mjb_nfkc_casefold_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP, MJB_ENC_UTF_8, output, &output_size, NULL) != MJB_STATUS_OK) {
    return 1;
}

// NFKC casefold payload (no terminator): strasse
printf("NFKC casefold payload (no terminator): %.*s", (int)output_size, output);`,
    related: ['mjb_nfkc_casefold', 'mjb_normalize', 'mjb_map_case', 'mjb_is_identifier'],
    specs: [
      unicodeCore('Section 3.13', 'Default Case Algorithms', 'G33992'),
      uax(31, 'Unicode Identifiers and Syntax'),
      uax(44, 'Unicode Character Database')
    ]
  },
  {
    comment: 'Convert a domain name to its UTS #46 nontransitional ASCII form.',
    ret: 'mjb_status',
    name: 'mjb_idna_to_ascii',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The domain name to process'),
      byte_length(),
      encoding(),
      encoding('The output encoding of the ASCII domain name', 'output_encoding'),
      {
        name: 'info',
        type: 'mjb_idna_info *',
        description: 'The UTS #46 validation errors to store',
        wasm_generated: true
      },
      result()
    ],
    wasm: true,
    section: Section.TextTransformation,
    exampleFeature: 'MJB_FEATURE_IDNA',
    details: 'Apply UTS #46 nontransitional processing with STD3 ASCII, hyphen, joiner, bidi, ' +
      'label-length, and domain-length checks enabled. Non-ASCII labels are encoded with ' +
      'Punycode. A successful operational status can still set `info->errors`; an errored ' +
      'ToASCII result must not be used for DNS lookup. If `MJB_FEATURE_IDNA=0` the function ' +
      'always returns `MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'Processing completed; inspect `info->errors` for validation failures' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`info` or `result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An encoding is invalid' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent the result' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An output or Punycode size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_IDNA=0`' }
    ],
    example: `const char *domain = "b\\xC3\\xBC" "cher.example"; // "bücher.example"
mjb_idna_info info;
mjb_result result;

if(mjb_idna_to_ascii(domain, strlen(domain), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    &info, &result) != MJB_STATUS_OK || info.errors != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// xn--bcher-kva.example
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
    related: ['mjb_idna_to_ascii_into', 'mjb_idna_to_unicode',
      'mjb_idna_to_unicode_into'],
    specs: [uts(46, 'Unicode IDNA Compatibility Processing')]
  },
  {
    comment: 'Convert a domain name to its UTS #46 nontransitional ASCII form into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_idna_to_ascii_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The domain name to process'),
      byte_length(),
      encoding(),
      encoding('The output encoding of the ASCII domain name', 'output_encoding'),
      {
        name: 'info',
        type: 'mjb_idna_info *',
        description: 'The UTS #46 validation errors to store',
        wasm_generated: false
      },
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same strict nontransitional profile as `mjb_idna_to_ascii`. Set `output` ' +
      'to NULL to query the required byte count. No bytes are written if capacity is ' +
      'insufficient. Processing uses temporary allocations even during a size query. If ' +
      '`MJB_FEATURE_IDNA=0` the function always returns `MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the ASCII domain was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`info` or `output_size` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An encoding is invalid' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent the result' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An output or Punycode size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'A temporary allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_IDNA=0`' }
    ],
    related: ['mjb_idna_to_ascii', 'mjb_idna_to_unicode',
      'mjb_idna_to_unicode_into'],
    specs: [uts(46, 'Unicode IDNA Compatibility Processing')]
  },
  {
    comment: 'Convert a domain name to its UTS #46 nontransitional Unicode form.',
    ret: 'mjb_status',
    name: 'mjb_idna_to_unicode',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The domain name to process'),
      byte_length(),
      encoding(),
      encoding('The output encoding of the Unicode domain name', 'output_encoding'),
      {
        name: 'info',
        type: 'mjb_idna_info *',
        description: 'The UTS #46 validation errors to store',
        wasm_generated: true
      },
      result()
    ],
    wasm: true,
    section: Section.TextTransformation,
    exampleFeature: 'MJB_FEATURE_IDNA',
    details: 'Apply UTS #46 nontransitional processing and decode valid `xn--` labels. The ' +
      'function returns its best-effort converted string even when `info->errors` records a ' +
      'validation problem. If `MJB_FEATURE_IDNA=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'Processing completed; inspect `info->errors` for validation failures' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`info` or `result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An encoding is invalid' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent the result' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An output or Punycode size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_IDNA=0`' }
    ],
    example: `const char *domain = "xn--bcher-kva.example";
mjb_idna_info info;
mjb_result result;

if(mjb_idna_to_unicode(domain, strlen(domain), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    &info, &result) != MJB_STATUS_OK || info.errors != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// bücher.example
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
    related: ['mjb_idna_to_unicode_into', 'mjb_idna_to_ascii',
      'mjb_idna_to_ascii_into'],
    specs: [uts(46, 'Unicode IDNA Compatibility Processing')]
  },
  {
    comment: 'Convert a domain name to its UTS #46 nontransitional Unicode form into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_idna_to_unicode_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The domain name to process'),
      byte_length(),
      encoding(),
      encoding('The output encoding of the Unicode domain name', 'output_encoding'),
      {
        name: 'info',
        type: 'mjb_idna_info *',
        description: 'The UTS #46 validation errors to store',
        wasm_generated: false
      },
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same strict nontransitional profile as `mjb_idna_to_unicode`. Set ' +
      '`output` to NULL to query the required byte count. No bytes are written if capacity is ' +
      'insufficient. Processing uses temporary allocations even during a size query. If ' +
      '`MJB_FEATURE_IDNA=0` the function always returns `MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the Unicode domain was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`info` or `output_size` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An encoding is invalid' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent the result' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An output or Punycode size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'A temporary allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_IDNA=0`' }
    ],
    related: ['mjb_idna_to_unicode', 'mjb_idna_to_ascii',
      'mjb_idna_to_ascii_into'],
    specs: [uts(46, 'Unicode IDNA Compatibility Processing')]
  },
  {
    comment: 'Check if a string is normalized to NFC/NFKC/NFD/NFKD form.',
    ret: 'mjb_status',
    name: 'mjb_normalization_quick_check',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to check',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'quick_check',
        type: 'mjb_quick_check_result *',
        description: 'The quick-check result to store',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Run the normalization quick-check on a string without allocating. `MJB_QC_MAYBE` ' +
      'means the string may still be normalized, and only a full normalization pass with ' +
      '`mjb_normalize` can decide.',
    returns: [
      { value: 'MJB_STATUS_OK', description: '`quick_check` contains `MJB_QC_YES`, `MJB_QC_NO`, or `MJB_QC_MAYBE`' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`quick_check` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: '`encoding` is invalid or a generic UTF-16/UTF-32 encoding has no byte-order information' },
      { value: 'MJB_STATUS_INVALID_FORM', description: '`form` is not NFC, NFD, NFKC, or NFKD' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description: 'The input contains an ill-formed code-unit sequence' }
    ],
    example: `const char *input = "caf\\xC3\\xA9";
mjb_quick_check_result check;

if(mjb_normalization_quick_check(input, strlen(input), MJB_ENC_UTF_8,
    MJB_NORMALIZATION_NFC, &check) != MJB_STATUS_OK) {
    return 1;
}

// NFC normalized: yes
printf("NFC normalized: %s", check == MJB_QC_YES ? "yes" : "no");`,
    related: ['mjb_normalize'],
    specs: [uax(15, 'Unicode Normalization Forms')]
  },
  {
    comment: 'Return the string encoding (the most probable).',
    ret: 'mjb_encoding',
    name: 'mjb_detect_encoding',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length('The explicit length of the string, in bytes')
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: '`mjb_detect_encoding` reports BOM-derived UTF-16/UTF-32 schemes with the generic ' +
      'family bit plus the resolved endian bit. Passing that detected value consumes the leading ' +
      'BOM as a signature. Passing an explicit-endian encoding such as `MJB_ENC_UTF_16BE` preserves ' +
      'an initial U+FEFF as text. When flags overlap, as with a UTF-32LE BOM that also has the ' +
      'UTF-16LE BOM prefix, decoding gives UTF-32 precedence. `MJB_NUL_TERMINATED` is not ' +
      'accepted because the encoding, and therefore the NUL code-unit width, is unknown.',
    example: `const char utf16le[] = "\\xFF\\xFEH\\0i\\0";
mjb_encoding detected = mjb_detect_encoding(utf16le, sizeof(utf16le) - 1);
bool is_utf16le = detected == (MJB_ENC_UTF_16 | MJB_ENC_UTF_16LE);

// UTF-16LE detected: yes
printf("UTF-16LE detected: %s", is_utf16le ? "yes" : "no");`
  },
  {
    comment: 'Return true if the string is encoded in ASCII.',
    ret: 'bool',
    name: 'mjb_is_ascii',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length()
    ],
    wasm: true,
    wasmName: 'isASCII',
    section: Section.TextAnalysis,
    example: `const char *input = "Plain ASCII";

// ASCII: yes
printf("ASCII: %s", mjb_is_ascii(input, strlen(input)) ? "yes" : "no");`
  },
  {
    comment: 'Return true if the string is encoded in UTF-8.',
    ret: 'bool',
    name: 'mjb_is_utf8',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length()
    ],
    wasm: true,
    wasmName: 'isUTF8',
    section: Section.TextAnalysis,
    example: `const char *input = "caf\\xC3\\xA9";

// Valid UTF-8: yes
printf("Valid UTF-8: %s", mjb_is_utf8(input, strlen(input)) ? "yes" : "no");`
  },
  {
    comment: 'Return true if the string is encoded in UTF-16BE or UTF-16LE.',
    ret: 'bool',
    name: 'mjb_is_utf16',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length()
    ],
    wasm: true,
    wasmName: 'isUTF16',
    section: Section.TextAnalysis,
    example: `const char utf16be[] = "\\xFE\\xFF\\0H\\0i"; // BOM + "Hi" in UTF-16BE

// UTF-16: yes
printf("UTF-16: %s", mjb_is_utf16(utf16be, sizeof(utf16be) - 1) ? "yes" : "no");`
  },
  {
    comment: 'Validate a complete Unicode code-unit sequence.',
    ret: 'mjb_status',
    name: 'mjb_string_validate',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to validate'),
      byte_length(),
      encoding(),
      diagnostic()
    ],
    wasm: false,
    section: Section.TextAnalysis,
    details: 'Validate the complete input without producing output. Empty input is well-formed. ' +
      'On malformed input, `diagnostic` identifies the first maximal ill-formed subsequence. ' +
      'Generic UTF-16 and UTF-32 require a byte-order mark.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The complete input is well-formed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The encoding is unsupported, or generic UTF-16/UTF-32 has no byte-order mark' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The first malformed subsequence is described by `diagnostic`' }
    ],
    example: `const char invalid[] = "\\xE2\\x82";
mjb_diagnostic diagnostic;

if(mjb_string_validate(invalid, sizeof(invalid) - 1, MJB_ENC_UTF_8,
    &diagnostic) != MJB_STATUS_MALFORMED_INPUT || diagnostic.byte_offset != 0) {
    return 1;
}`,
    related: ['mjb_decode_next', 'mjb_decode_previous', 'mjb_is_utf8']
  },
  {
    comment: 'Decode the next codepoint from a string.',
    ret: 'mjb_status',
    name: 'mjb_decode_next',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to decode'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'offset',
        type: 'size_t *',
        description: 'The input byte offset and the byte offset following the decoded subsequence',
        wasm_generated: false
      },
      {
        name: 'codepoint',
        type: 'mjb_codepoint *',
        description: 'Where to store the decoded codepoint',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.TextAnalysis,
    details: 'Decode one codepoint and advance `offset`. With `MJB_MALFORMED_STOP`, malformed ' +
      'input returns `MJB_STATUS_MALFORMED_INPUT`; `offset` still advances over the malformed ' +
      'subsequence so decoding can resume. `MJB_MALFORMED_REPLACE` returns U+FFFD, while ' +
      '`MJB_MALFORMED_SKIP` advances until a valid codepoint or end of input. A diagnostic is ' +
      'reported for replacement and skipping even when the function returns success.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'A codepoint was decoded' },
      { value: 'MJB_STATUS_END_OF_INPUT', description: 'No codepoint remains' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: 'An argument or policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding cannot be decoded' }
    ],
    related: ['mjb_decode_previous', 'mjb_string_validate', 'mjb_codepoint_count']
  },
  {
    comment: 'Decode the previous codepoint from a string.',
    ret: 'mjb_status',
    name: 'mjb_decode_previous',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to decode'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'offset',
        type: 'size_t *',
        description: 'The input byte offset and the start of the decoded subsequence',
        wasm_generated: false
      },
      {
        name: 'codepoint',
        type: 'mjb_codepoint *',
        description: 'Where to store the decoded codepoint',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.TextAnalysis,
    details: 'Decode backward from `offset`, using the same malformed-input policies and ' +
      'diagnostic contract as `mjb_decode_next`. On success, `offset` is the first byte of the ' +
      'decoded codepoint. Start with the input byte length to iterate from the end.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'A codepoint was decoded' },
      { value: 'MJB_STATUS_END_OF_INPUT', description: 'No codepoint precedes `offset`' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: 'An argument or policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding cannot be decoded' }
    ],
    related: ['mjb_decode_next', 'mjb_string_validate', 'mjb_codepoint_count']
  },
  {
    comment: 'Count the codepoints in a string.',
    ret: 'mjb_status',
    name: 'mjb_codepoint_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to count'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'count',
        type: 'size_t *',
        description: 'The number of codepoints to store; set to zero on failure',
        wasm_generated: true
      },
      diagnostic()
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Count the number of decoded Unicode codepoints in a string. Malformed subsequences ' +
      'are stopped, replaced, or skipped according to `malformed_policy`. A replacement counts ' +
      'as one codepoint. On failure, `count` is set to zero.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The count was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`count` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    example: `// The "Héllö" string is five Unicode characters, but has different byte lengths in different encodings.

const char *utf8 = "H\\xC3\\xA9ll\\xC3\\xB6"; // 7 bytes
const char utf16le[] = "H\\0\\xE9\\0l\\0l\\0\\xF6\\0"; // 10 bytes
size_t count;

if(mjb_codepoint_count(utf8, 7, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// 5 UTF-8 characters
printf("%zu UTF-8 characters", count);

if(mjb_codepoint_count(utf16le, 10, MJB_ENC_UTF_16LE, MJB_MALFORMED_STOP,
    &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// 5 UTF-16LE characters
printf("%zu UTF-16LE characters", count);`,
    related: ['mjb_grapheme_count', 'mjb_word_count', 'mjb_sentence_count']
  },
  {
    comment: 'Run a callback for each codepoint of a string.',
    ret: 'mjb_status',
    name: 'mjb_for_each_codepoint',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'callback',
        type: 'mjb_for_each_codepoint_fn',
        description: 'The function to call for each codepoint',
        wasm_generated: true
      },
      diagnostic()
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Decode the string according to `malformed_policy` and call the callback for every ' +
      'resulting codepoint. The first malformed subsequence is reported in `diagnostic` even ' +
      'when it is replaced or skipped.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'Every decoded codepoint was visited' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        'The buffer, callback, or malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_CALLBACK_STOPPED', description: 'The callback returned false' }
    ],
    example: `mjb_status status = mjb_for_each_codepoint("ABC", 3, MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP, NULL, NULL);

// A callback is required: yes
bool callback_required = status == MJB_STATUS_INVALID_ARGUMENT;

// A callback is required: yes
printf("A callback is required: %s", callback_required ? "yes" : "no");`
  },
  {
    comment: 'Return the value of a binary Unicode property.',
    ret: 'mjb_status',
    name: 'mjb_codepoint_property_binary',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'property',
        type: 'mjb_property',
        description: 'The binary property to query',
        wasm_generated: false
      },
      {
        name: 'value',
        type: 'bool *',
        description: 'Where to store the binary property value',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Return `true` when the codepoint has the binary property and `false` when it does ' +
      'not. Passing an enumerated property is a type mismatch and returns ' +
      '`MJB_STATUS_INVALID_ARGUMENT`.',
    example: `bool is_alphabetic;

if(mjb_codepoint_property_binary('A', MJB_PR_ALPHABETIC,
    &is_alphabetic) != MJB_STATUS_OK) {
    return 1;
}

// U+0041 is alphabetic: yes
printf("U+0041 is alphabetic: %s", is_alphabetic ? "yes" : "no");`,
    related: ['mjb_codepoint_property_int'],
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Return the value of an enumerated or integer Unicode property.',
    ret: 'mjb_status',
    name: 'mjb_codepoint_property_int',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'property',
        type: 'mjb_property',
        description: 'The enumerated or integer property to query',
        wasm_generated: false
      },
      {
        name: 'value',
        type: 'int32_t *',
        description: 'Where to store the property value',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Passing a binary property is a type mismatch and returns ' +
      '`MJB_STATUS_INVALID_ARGUMENT`. `MJB_STATUS_NOT_FOUND` means that the codepoint has no ' +
      'stored value for the requested property.',
    example: `int32_t script;

if(mjb_codepoint_property_int('A', MJB_PR_SCRIPT, &script) != MJB_STATUS_OK) {
    return 1;
}

// U+0041 uses the Latin script: yes
printf("U+0041 uses the Latin script: %s", script == MJB_SC_LATN ? "yes" : "no");`,
    related: ['mjb_codepoint_property_binary'],
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Return the numeric value of a codepoint.',
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
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Return the numeric value of a codepoint, if any. If the codepoint has no numeric ' +
      'value, `value->decimal` and `value->digit` are set to `MJB_NUMBER_NOT_VALID` (-1).',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The character was found and filled' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`value` is NULL or the codepoint is not valid' },
    ],
    example: `mjb_numeric_value num;

if(mjb_codepoint_numeric_value(0x0031, &num) != MJB_STATUS_OK) { // U+0031 = 1
    return 1;
}

// decimal=1, digit=1, numeric=1
printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);

if(mjb_codepoint_numeric_value(0x00BD, &num) != MJB_STATUS_OK) { // U+00BD = '½'
    return 1;
}

// decimal=-1, digit=-1, numeric=1/2
printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);`,
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Return the character block.',
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
    wasm: true,
    section: Section.TextAnalysis,
    example: `mjb_block_info block;

if(mjb_codepoint_block('A', &block) != MJB_STATUS_OK) {
    return 1;
}

// Block: Basic Latin
printf("Block: %s", block.name);`,
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Return the script of a codepoint.',
    ret: 'mjb_script',
    name: 'mjb_codepoint_script',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `mjb_script script = mjb_codepoint_script(0x03A9); // Greek capital omega

// Greek script: yes
printf("Greek script: %s", script == MJB_SC_GREK ? "yes" : "no");`,
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Return the Script_Extensions set of a codepoint.',
    ret: 'mjb_status',
    name: 'mjb_codepoint_script_extensions',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'scripts',
        type: 'mjb_script *',
        description: 'The caller-provided script buffer, or NULL to query the required count',
        wasm_generated: true
      },
      {
        name: 'count',
        type: 'size_t *',
        description: 'The input capacity and output script count',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Return the explicit Script_Extensions set, or the ordinary Script value when the ' +
      'codepoint has no explicit Script_Extensions entry. Call first with `scripts` set to NULL ' +
      'to obtain the required count.',
    example: `size_t count = 0;

if(mjb_codepoint_script_extensions(0x30FC, NULL, &count) != MJB_STATUS_OK) {
    return 1;
}

mjb_script scripts[3];

if(count > 3 || mjb_codepoint_script_extensions(0x30FC, scripts,
    &count) != MJB_STATUS_OK) {
    return 1;
}

// U+30FC has 2 Script_Extensions
printf("U+30FC has %zu Script_Extensions", count);`,
    related: ['mjb_codepoint_script'],
    specs: [uax(24, 'Unicode Script Property')]
  },
  {
    comment: 'Encode a codepoint to a string.',
    ret: 'unsigned int',
    name: 'mjb_codepoint_encode',
    attributes: [],
    args: [
      codepoint('The codepoint to encode'),
      buffer('The buffer to encode the codepoint to', 'buffer', false, true),
      byte_length('The length of the buffer, in bytes'),
      encoding('The encoding to use')
    ],
    wasm: true,
    section: Section.TextTransformation,
    example: `char encoded[4];
unsigned int size = mjb_codepoint_encode(0x20AC, encoded, sizeof(encoded), MJB_ENC_UTF_8);

// € sign uses 3 UTF-8 bytes
printf("%.*s sign uses %u UTF-8 bytes", (int)size, encoded, size);`
  },
  {
    comment: 'Convert from one encoding to another.',
    ret: 'mjb_status',
    name: 'mjb_convert_encoding',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to convert'),
      byte_length(),
      encoding('The input encoding of the string'),
      malformedPolicy(),
      encoding('The output encoding of the string', 'output_encoding'),
      result(),
      diagnostic()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Convert a string between the supported encodings (UTF-8, UTF-16LE/BE, ' +
      'UTF-32LE/BE). Generic UTF-16/UTF-32 input consumes a leading BOM as the encoding scheme ' +
      'signature and uses it to resolve byte order. Explicit-endian input preserves an initial ' +
      'U+FEFF as text. Generic UTF-16/UTF-32 without a BOM, and generic UTF-16/UTF-32 output, are ' +
      'rejected because the byte order is not specified. Malformed source subsequences are ' +
      'stopped, replaced, or skipped according to `malformed_policy`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The string was converted' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` is NULL, `buffer` is NULL with a non-zero size, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description: 'The requested encoding conversion is not supported' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "caf\\xC3\\xA9";
mjb_result result;

if(mjb_convert_encoding(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_ENC_UTF_16LE, &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE bytes: 8
printf("UTF-16LE bytes: %zu", result.output_size);
mjb_result_free(&result);`,
    related: ['mjb_convert_encoding_into', 'mjb_detect_encoding', 'mjb_codepoint_encode']
  },
  {
    comment: 'Convert from one encoding to another into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_convert_encoding_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to convert'),
      byte_length(),
      encoding('The input encoding of the string'),
      malformedPolicy(),
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Convert a string using the same encoding and BOM rules as ' +
      '`mjb_convert_encoding`, without allocating memory. Set `output` to NULL to query the ' +
      'required size. If `output` is non-NULL, `*output_size` supplies its capacity; on return ' +
      'it contains the required size when the buffer is too small, or the written size on ' +
      'success. The size is the encoded payload byte count: terminators are excluded, and ' +
      'this function does not write a terminator. No bytes are written when capacity is ' +
      'insufficient.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the string was converted' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output_size` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested encoding conversion is not supported' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' }
    ],
    example: `const char *input = "caf\\xC3\\xA9";
size_t output_size = 0;

if(mjb_convert_encoding_into(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_ENC_UTF_16LE, NULL, &output_size, NULL) != MJB_STATUS_OK) {
    return 1;
}

unsigned char output[8];

if(output_size > sizeof(output) || mjb_convert_encoding_into(input, strlen(input),
    MJB_ENC_UTF_8, MJB_MALFORMED_STOP, MJB_ENC_UTF_16LE, output, &output_size,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE payload bytes (no terminator): 8
printf("UTF-16LE payload bytes (no terminator): %zu", output_size);`,
    related: ['mjb_convert_encoding', 'mjb_detect_encoding', 'mjb_codepoint_encode']
  },
  {
    comment: 'Compare two strings using a Unicode caseless matching relation.',
    ret: 'mjb_status',
    name: 'mjb_caseless_match',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The first string to compare', 's1'),
      byte_length('The length of the first string in bytes, or `MJB_NUL_TERMINATED`',
        's1_byte_length'),
      encoding('The encoding of the first string', 's1_encoding'),
      buffer('The second string to compare', 's2'),
      byte_length('The length of the second string in bytes, or `MJB_NUL_TERMINATED`',
        's2_byte_length'),
      encoding('The encoding of the second string', 's2_encoding'),
      {
        name: 'mode',
        type: 'mjb_caseless_mode',
        description: 'The Unicode caseless matching relation',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'matches',
        type: 'bool *',
        description: 'Whether the strings are a caseless match',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.SortingComparison,
    details: 'Compare two strings for case-insensitive equality using Unicode full default case ' +
      'folding. Canonical mode implements D145 and is recommended for ordinary text. ' +
      'Unnormalized mode implements D144 without normalization. Compatibility mode implements ' +
      'the iterated folding and compatibility normalization required by D146. Identifier mode ' +
      'implements D147 by applying NFD before `mjb_nfkc_casefold`; it removes default-ignorable ' +
      'codepoints but does not validate identifier syntax.',
    returns: [
      { value: 'MJB_STATUS_OK', description: '`matches` contains the comparison result' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`matches` is NULL, an input buffer is NULL with a non-zero size, or `mode` is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'An input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An intermediate size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The identifier case-folding transform did not converge' }
    ],
    example: `const char *left = "Stra\\xC3\\x9F" "e";
const char *right = "STRASSE";
bool matches;

if(mjb_caseless_match(left, strlen(left), MJB_ENC_UTF_8,
    right, strlen(right), MJB_ENC_UTF_8, MJB_CASELESS_CANONICAL, &matches) != MJB_STATUS_OK) {
    return 1;
}

// Canonical caseless match: yes
printf("Canonical caseless match: %s", matches ? "yes" : "no");`,
    related: ['mjb_map_case', 'mjb_nfkc_casefold', 'mjb_normalize',
      'mjb_collation_compare', 'mjb_is_identifier'],
    specs: [
      unicodeCore('Section 3.13.5', 'Default Caseless Matching', 'G33992'),
      uax(31, 'Unicode Identifiers and Syntax')
    ]
  },
  {
    comment: 'Compare two strings using UCA.',
    ret: 'mjb_status',
    name: 'mjb_collation_compare',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The first string to compare', 's1'),
      byte_length('The length of the first string in bytes, or `MJB_NUL_TERMINATED`',
        's1_byte_length'),
      encoding('The encoding of the first string', 's1_encoding'),
      buffer('The second string to compare', 's2'),
      byte_length('The length of the second string in bytes, or `MJB_NUL_TERMINATED`',
        's2_byte_length'),
      encoding('The encoding of the second string', 's2_encoding'),
      {
        name: 'variable_weighting',
        type: 'mjb_collation_variable_weighting',
        description: 'The variable weighting strategy',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'strength',
        type: 'mjb_collation_strength',
        description: 'The maximum collation level to compare',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'order',
        type: 'int *',
        description: 'The strcmp-style comparison result to store',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.SortingComparison,
    exampleFeature: 'MJB_FEATURE_COLLATION',
    details: 'Compare two strings using the Unicode Collation Algorithm and the default ' +
      'collation element table (DUCET), with `strcmp`-style semantics. Primary strength compares ' +
      'base-character weights. Secondary strength also compares accents while ignoring tertiary ' +
      'case differences. Tertiary strength adds case and variant differences. Quaternary strength ' +
      'also compares variable elements moved to level 4 by `MJB_COLLATION_SHIFTED`; with ' +
      '`MJB_COLLATION_NON_IGNORABLE`, it is equivalent to tertiary strength. Collation equality ' +
      'at a selected strength is not Unicode caseless matching; use `mjb_caseless_match` when ' +
      'case-insensitive equality is the intended operation. See ' +
      '[Unicode collation](#unicode-collation) for the comparison process and configuration ' +
      'guidance. If `MJB_FEATURE_COLLATION=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: '`order` is negative, zero, or positive according to the collation order' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`order` is NULL, an input buffer is NULL with a non-zero size, or an option is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description: 'An input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An intermediate size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_COLLATION=0`' }
    ],
    example: `int order;

if(mjb_collation_compare("apple", 5, MJB_ENC_UTF_8,
    "banana", 6, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE,
    MJB_COLLATION_TERTIARY, &order) != MJB_STATUS_OK) {
    return 1;
}

// apple sorts before banana: yes
printf("apple sorts before banana: %s", order < 0 ? "yes" : "no");`,
    related: ['mjb_collation_key', 'mjb_collation_key_into', 'mjb_caseless_match'],
    specs: [uts(10, 'Unicode Collation Algorithm')]
  },
  {
    comment: 'Generate a UCA sort key for a string.',
    ret: 'mjb_status',
    name: 'mjb_collation_key',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to generate the sort key for'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'variable_weighting',
        type: 'mjb_collation_variable_weighting',
        description: 'The variable weighting strategy',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'strength',
        type: 'mjb_collation_strength',
        description: 'The maximum collation level to include',
        wasm_generated: false,
        is_enum: true
      },
      result('The pointer to store the binary sort key'),
      diagnostic()
    ],
    wasm: true,
    section: Section.SortingComparison,
    exampleFeature: 'MJB_FEATURE_COLLATION',
    details: 'Generate a binary sort key for a string. Sort keys of different strings can be ' +
      'compared with `memcmp` and yield the same order as `mjb_collation_compare` when both use ' +
      'the same variable weighting and strength. Useful when the same strings are compared many ' +
      'times, such as sorting or database indexing. Empty input and non-empty input with no ' +
      'effective weights at the selected strength both produce a zero-length key. Malformed ' +
      'subsequences follow `malformed_policy`, and `diagnostic` records the first one. If ' +
      '`MJB_FEATURE_COLLATION=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The sort key was generated' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT',
        description:
          '`result` is NULL, the buffer is invalid, the malformed policy is invalid, or a ' +
          'collation option is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The sort key size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_COLLATION=0`' }
    ],
    example: `mjb_result key;

if(mjb_collation_key("r\\xC3\\xA9sum\\xC3\\xA9", 8, MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP, MJB_COLLATION_NON_IGNORABLE, MJB_COLLATION_TERTIARY, &key,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Sort key is non-empty: yes
printf("Sort key is non-empty: %s", key.output_size > 0 ? "yes" : "no");
mjb_result_free(&key);`,
    related: ['mjb_collation_key_into', 'mjb_collation_compare'],
    specs: [uts(10, 'Unicode Collation Algorithm')]
  },
  {
    comment: 'Generate a binary collation key into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_collation_key_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to generate the sort key for'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'variable_weighting',
        type: 'mjb_collation_variable_weighting',
        description: 'The variable weighting strategy',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'strength',
        type: 'mjb_collation_strength',
        description: 'The maximum collation level to include',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided binary output buffer, or NULL to query its size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.SortingComparison,
    exampleFeature: 'MJB_FEATURE_COLLATION',
    details: 'Generate the same binary sort key as `mjb_collation_key` without allocating the ' +
      'final key buffer. Set `output` to NULL to query the required byte count. If `output` is ' +
      'non-NULL, `*output_size` supplies its capacity; on return it contains the required size ' +
      'when the buffer is too small, or the written size on success. A collation key is binary: ' +
      'no terminator is included or written, and no bytes are written when capacity is ' +
      'insufficient. Collation processing still uses temporary allocations, including during a ' +
      'size query. Empty input and non-empty input with no effective weights at the selected ' +
      'strength both require zero bytes. If `MJB_FEATURE_COLLATION=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the binary sort key was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output_size` is NULL, the buffer is invalid, the malformed policy is invalid, or a ' +
        'collation option is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The input encoding is invalid' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required key size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Temporary allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_COLLATION=0`' }
    ],
    example: `const char *input = "r\\xC3\\xA9sum\\xC3\\xA9";
size_t output_size = 0;

if(mjb_collation_key_into(input, 8, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_COLLATION_NON_IGNORABLE, MJB_COLLATION_TERTIARY, NULL, &output_size,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

unsigned char output[64];

if(output_size > sizeof(output) || mjb_collation_key_into(input, 8, MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP,
    MJB_COLLATION_NON_IGNORABLE, MJB_COLLATION_TERTIARY, output,
    &output_size, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Sort key is non-empty: yes
printf("Sort key is non-empty: %s", output_size > 0 ? "yes" : "no");`,
    related: ['mjb_collation_key', 'mjb_collation_compare'],
    specs: [uts(10, 'Unicode Collation Algorithm')]
  },
  {
    comment: 'Change string case.',
    ret: 'mjb_status',
    name: 'mjb_map_case',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to change case'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'type',
        type: 'mjb_map_case_type',
        description: 'The type of case change',
        wasm_generated: false,
        is_enum: true
      },
      encoding('The output encoding of the string', 'output_encoding'),
      result(),
      diagnostic()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Convert a string to uppercase, lowercase, titlecase, or its case-folded form. ' +
      'Full case mappings are applied, including special casing and conditional mappings, so ' +
      'the output may have a different length than the input. Titlecase uses UAX #29 word ' +
      'boundaries: the first cased character in each word segment is titlecased, and ' +
      'subsequent characters in that segment are lowercased. Casing is tailored by the ' +
      'process-global locale set with `mjb_set_locale`: the default `MJB_LOCALE_EN` uses ' +
      'default non-Turkic mappings. `MJB_LOCALE_TR` and `MJB_LOCALE_AZ` apply ' +
      'Turkish/Azerbaijani dotted-I casing and Turkic `T` case-folding mappings. ' +
      '`MJB_LOCALE_LT` applies Lithuanian dot-above casing rules, while case folding remains ' +
      'the default non-Turkic mapping. Malformed subsequences follow `malformed_policy`, and ' +
      '`diagnostic` records the first one.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The case conversion succeeded' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` is NULL, the buffer is invalid, the malformed policy is invalid, or `type` is ' +
        'not a valid case type' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "Stra\\xC3\\x9F""e"; // "Straße"
mjb_result result;

if(mjb_map_case(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP, MJB_CASE_UPPER,
    MJB_ENC_UTF_8, &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Upper: STRASSE
printf("Upper: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);`,
    related: ['mjb_map_case_into', 'mjb_set_locale', 'mjb_get_locale'],
    specs: [unicodeCore('Section 3.13', 'Default Case Algorithms', 'G33992')]
  },
  {
    comment: 'Change string case into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_map_case_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to change case'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'type',
        type: 'mjb_map_case_type',
        description: 'The type of case change',
        wasm_generated: false
      },
      encoding('The output encoding of the string', 'output_encoding'),
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      },
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same full, special, conditional, titlecase, locale-sensitive, and case ' +
      'folding mappings as `mjb_map_case` without allocating memory. Set `output` to NULL to ' +
      'query the required size. If `output` is non-NULL, `*output_size` supplies its capacity; ' +
      'on return it contains the required size when the buffer is too small, or the written ' +
      'size on success. Terminators are excluded from the byte count and are not written. No ' +
      'bytes are written when capacity is insufficient.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the case-mapped string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output_size` is NULL, the buffer is invalid, the malformed policy is invalid, or ' +
        '`type` is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent a mapped codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' }
    ],
    example: `const char *input = "Stra\\xC3\\x9F""e"; // "Straße"
size_t output_size = 0;

if(mjb_map_case_into(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_CASE_UPPER, MJB_ENC_UTF_8, NULL, &output_size, NULL) != MJB_STATUS_OK) {
    return 1;
}

char output[7];

if(output_size > sizeof(output) || mjb_map_case_into(input, strlen(input), MJB_ENC_UTF_8,
    MJB_MALFORMED_STOP, MJB_CASE_UPPER, MJB_ENC_UTF_8, output, &output_size,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Upper payload (no terminator): STRASSE
printf("Upper payload (no terminator): %.*s", (int)output_size, output);`,
    related: ['mjb_map_case', 'mjb_set_locale', 'mjb_get_locale'],
    specs: [unicodeCore('Section 3.13', 'Default Case Algorithms', 'G33992')]
  },
  {
    comment: 'Return true if the codepoint is valid.',
    ret: 'bool',
    name: 'mjb_codepoint_is_valid',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// U+10FFFD valid: yes
printf("U+10FFFD valid: %s", mjb_codepoint_is_valid(0x10FFFD) ? "yes" : "no");`
  },
  {
    comment: 'Return true if the codepoint is graphic.',
    ret: 'bool',
    name: 'mjb_codepoint_is_graphic',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Letter A is graphic: yes
printf("Letter A is graphic: %s", mjb_codepoint_is_graphic('A') ? "yes" : "no");`
  },
  {
    comment: 'Return true if the codepoint is combining.',
    ret: 'bool',
    name: 'mjb_codepoint_is_combining',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// U+0301 is combining: yes
printf("U+0301 is combining: %s", mjb_codepoint_is_combining(0x0301) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is a hangul L.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_leading_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+1100 is a leading Jamo: yes
printf("U+1100 is a leading Jamo: %s", mjb_codepoint_is_hangul_leading_jamo(0x1100) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is a hangul V.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_vowel_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+1161 is a vowel Jamo: yes
printf("U+1161 is a vowel Jamo: %s", mjb_codepoint_is_hangul_vowel_jamo(0x1161) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is a hangul T.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_trailing_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+11A8 is a trailing Jamo: yes
printf("U+11A8 is a trailing Jamo: %s", mjb_codepoint_is_hangul_trailing_jamo(0x11A8) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is a hangul jamo.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+1100 is Hangul Jamo: yes
printf("U+1100 is Hangul Jamo: %s", mjb_codepoint_is_hangul_jamo(0x1100) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is a hangul syllable.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_syllable',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+AC00 is a Hangul syllable: yes
printf("U+AC00 is a Hangul syllable: %s", mjb_codepoint_is_hangul_syllable(0xAC00) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is CJK ideograph.',
    ret: 'bool',
    name: 'mjb_codepoint_is_cjk_ideograph',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// U+4E00 is a CJK ideograph: yes
printf("U+4E00 is a CJK ideograph: %s", mjb_codepoint_is_cjk_ideograph(0x4E00) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is CJK extension.',
    ret: 'bool',
    name: 'mjb_codepoint_is_cjk_extension_ideograph',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    wasmName: 'codepointIsCJKExtensionIdeograph',
    section: Section.TextAnalysis,
    example: `// U+20000 is a CJK extension ideograph: yes
printf("U+20000 is a CJK extension ideograph: %s", mjb_codepoint_is_cjk_extension_ideograph(0x20000) ? "yes" : "no");`
  },
  {
    comment: 'Return true if the category is graphic.',
    ret: 'bool',
    name: 'mjb_category_is_graphic',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'category',
        type: 'mjb_category',
        description: 'The category to check',
        wasm_generated: false,
        is_enum: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Uppercase letters are graphic: yes
bool graphic = mjb_category_is_graphic(MJB_CATEGORY_LU);

// Uppercase letters are graphic: yes
printf("Uppercase letters are graphic: %s", graphic ? "yes" : "no");`
  },
  {
    comment: 'Return true if the category is combining.',
    ret: 'bool',
    name: 'mjb_category_is_combining',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'category',
        type: 'mjb_category',
        description: 'The category to check',
        wasm_generated: false,
        is_enum: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Nonspacing marks are combining: yes
bool combining = mjb_category_is_combining(MJB_CATEGORY_MN);

// Nonspacing marks are combining: yes
printf("Nonspacing marks are combining: %s", combining ? "yes" : "no");`
  },
  {
    comment: 'Unicode line break algorithm.',
    ret: 'mjb_break_type',
    name: 'mjb_next_line_break',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length('The explicit length of the string, in bytes'),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_line_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    example: `mjb_next_line_state state;
state.index = 0;
mjb_break_type type = mjb_next_line_break("Hello world", 11, MJB_ENC_UTF_8, &state);

// First line-break result is set: yes
printf("First line-break result is set: %s", type != MJB_BT_NOT_SET ? "yes" : "no");`,
    related: ['mjb_next_grapheme_break', 'mjb_next_word_break', 'mjb_next_sentence_break'],
    specs: [uax(14, 'Unicode Line Breaking Algorithm')]
  },
  {
    comment: 'Word cluster breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_next_word_break',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length('The explicit length of the string, in bytes'),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_word_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    example: `mjb_next_word_state state;
state.index = 0;
size_t boundaries = 0;

while(mjb_next_word_break("Hello world", 11, MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Word-break positions: 11
printf("Word-break positions: %zu", boundaries);`,
    related: ['mjb_next_grapheme_break', 'mjb_next_sentence_break', 'mjb_truncate_word'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Sentence boundaries breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_next_sentence_break',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length('The explicit length of the string, in bytes'),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_sentence_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    example: `mjb_next_sentence_state state;
state.index = 0;
size_t boundaries = 0;
const char *input = "Hello. Goodbye.";

while(mjb_next_sentence_break(input, strlen(input), MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Sentence-break positions: 15
printf("Sentence-break positions: %zu", boundaries);`,
    related: ['mjb_next_grapheme_break', 'mjb_next_word_break'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Count the sentence segments in a string.',
    ret: 'mjb_status',
    name: 'mjb_sentence_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to count'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'count',
        type: 'size_t *',
        description: 'The number of sentence segments to store; set to zero on failure',
        wasm_generated: true
      },
      diagnostic()
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Count the sentence segments produced by the default Unicode sentence-boundary ' +
      'rules. The default rules carry no abbreviation list, so text such as `Dr. Smith` counts ' +
      'as two sentences. Malformed subsequences follow `malformed_policy`, and `diagnostic` ' +
      'records the first one. On failure, `count` is set to zero.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The count was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`count` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    example: `const char *input = "Hello. How are you? Fine!";
size_t count;

if(mjb_sentence_count(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Sentences: 3
printf("Sentences: %zu", count);`,
    related: ['mjb_next_sentence_break', 'mjb_grapheme_count', 'mjb_word_count'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Grapheme cluster breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_next_grapheme_break',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length('The explicit length of the string, in bytes'),
      encoding(),
      {
        name: 'state',
        type: 'mjb_next_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Iterate the grapheme cluster (user-perceived character) boundaries of a string. ' +
      'Call repeatedly with the same state until it reports the end of the string. Stateful ' +
      'break functions require an explicit length and do not accept `MJB_NUL_TERMINATED`; ' +
      'determine the length once before iteration.',
    example: `const char *input = "e\\xCC\\x81"; // e + combining acute accent
mjb_next_state state;
state.index = 0;
size_t codepoints = 0;

while(mjb_next_grapheme_break(input, strlen(input), MJB_ENC_UTF_8,
    &state) != MJB_BT_NOT_SET) {
    ++codepoints;
}

// Codepoints examined: 2
printf("Codepoints examined: %zu", codepoints);`,
    related: ['mjb_next_word_break', 'mjb_next_sentence_break', 'mjb_next_line_break', 'mjb_truncate_grapheme'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return the number of bytes that form the first `max_graphemes` grapheme cluster segments.',
    ret: 'size_t',
    name: 'mjb_truncate_grapheme',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'max_graphemes',
        type: 'size_t',
        description: 'The maximum number of graphemes to return',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    example: `const char *input = "A\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9Z"; // A🇮🇹Z
size_t bytes = mjb_truncate_grapheme(input, strlen(input), MJB_ENC_UTF_8, 2);

// First two graphemes use 9 bytes
printf("First two graphemes use %zu bytes", bytes);`
  },
  {
    comment: 'Count the extended grapheme clusters in a string.',
    ret: 'mjb_status',
    name: 'mjb_grapheme_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to count'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'count',
        type: 'size_t *',
        description: 'The number of grapheme clusters to store; set to zero on failure',
        wasm_generated: true
      },
      diagnostic()
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Count user-perceived characters: the number of extended grapheme cluster segments ' +
      'in the string. Malformed subsequences follow `malformed_policy`, and `diagnostic` records ' +
      'the first one. On failure, `count` is set to zero.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The count was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`count` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    example: `const char *input = "A\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9"; // A🇮🇹
size_t count;

if(mjb_grapheme_count(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Grapheme clusters: 2
printf("Grapheme clusters: %zu", count);`,
    related: ['mjb_truncate_grapheme', 'mjb_next_grapheme_break', 'mjb_terminal_width'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return the number of bytes whose grapheme clusters fit within max_columns terminal cells.',
    ret: 'size_t',
    name: 'mjb_truncate_grapheme_width',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'profile',
        type: 'mjb_terminal_width_profile',
        description: 'The terminal-width profile',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'max_columns',
        type: 'size_t',
        description: 'The maximum number of columns to return',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.TerminalWidth,
    details: 'Truncate only at extended grapheme-cluster boundaries, using the same terminal-cell ' +
      'policy as `mjb_terminal_width`. The returned prefix stops before a cluster that would exceed ' +
      'the budget or cannot be measured as printable, single-line terminal text.',
    example: `const char *input = "A\\xE7\\x95\\x8C"; // A界
size_t bytes = mjb_truncate_grapheme_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_TERMINAL_WIDTH_NARROW, 2);

// Two columns include 1 byte
printf("Two columns include %zu byte", bytes);`
  },
  {
    comment: 'Return the number of bytes that form the first max_segments word-break segments.',
    ret: 'size_t',
    name: 'mjb_truncate_word',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'max_segments',
        type: 'size_t',
        description: 'The maximum number of segments to return',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    example: `const char *input = "Hello world";
size_t bytes = mjb_truncate_word(input, strlen(input), MJB_ENC_UTF_8, 1);

// First word segment uses 5 bytes
printf("First word segment uses %zu bytes", bytes);`
  },
  {
    comment: 'Count the word-like segments in a string.',
    ret: 'mjb_status',
    name: 'mjb_word_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to count'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'count',
        type: 'size_t *',
        description: 'The number of word-like segments to store; set to zero on failure',
        wasm_generated: true
      },
      diagnostic()
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Count the words in a string: the word-break segments that contain at least one ' +
      'alphabetic or numeric character. Punctuation, whitespace, and symbol segments are not ' +
      'counted, so `Hello, world!` counts as two words. Hyphenated compounds count each part, ' +
      'matching the default Unicode word-boundary rules. Unlike `mjb_truncate_word`, whose ' +
      '`max_segments` counts every raw segment, this function skips non-word segments. For ' +
      'scripts segmented by dictionary lookup in other implementations, such as Chinese, ' +
      'Japanese, Thai, Lao, Khmer, and Burmese, the count approximates one word per character: ' +
      'Mojibake does not use frequency dictionaries to keep the size of the library small. ' +
      'Malformed subsequences follow `malformed_policy`, and `diagnostic` records the first one. ' +
      'On failure, `count` is set to zero.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The count was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`count` is NULL, the buffer is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    example: `const char *input = "Hello, world! It works.";
size_t count;

if(mjb_word_count(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Words: 4
printf("Words: %zu", count);`,
    related: ['mjb_next_word_break', 'mjb_truncate_word', 'mjb_grapheme_count', 'mjb_sentence_count'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return the number of bytes whose word-break segments fit within max_columns terminal cells.',
    ret: 'size_t',
    name: 'mjb_truncate_word_width',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'profile',
        type: 'mjb_terminal_width_profile',
        description: 'The terminal-width profile',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'max_columns',
        type: 'size_t',
        description: 'The maximum number of columns to return',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.TerminalWidth,
    details: 'Truncate only at word-break boundaries, using the same terminal-cell policy as ' +
      '`mjb_terminal_width`. The returned prefix stops before a segment that would exceed the ' +
      'budget or cannot be measured as printable, single-line terminal text.',
    example: `const char *input = "Hello world";
size_t bytes = mjb_truncate_word_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_TERMINAL_WIDTH_NARROW, 6);

// Six columns include 6 bytes
printf("Six columns include %zu bytes", bytes);`
  },
  {
    comment: 'Resolve bidirectional text (TR9) for a paragraph.',
    ret: 'mjb_status',
    name: 'mjb_bidi_resolve',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The input string'),
      byte_length(),
      encoding(),
      {
        name: 'direction',
        type: 'mjb_direction',
        description: 'The base paragraph direction (LTR, RTL, or AUTO for P2/P3)',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'result',
        type: 'mjb_bidi_paragraph *',
        description: 'Output paragraph; chars is library-allocated',
        wasm_generated: false,
        ownership: '`result->chars` is library-allocated and must be freed with `mjb_bidi_paragraph_free()`'
      }
    ],
    wasm: true,
    section: Section.Bidirectional,
    details: 'Resolve the embedding levels of a paragraph following the Unicode Bidirectional ' +
      'Algorithm. The resolved paragraph can then be split into lines and reordered visually ' +
      'with `mjb_bidi_reorder_line` and `mjb_bidi_line_runs`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The paragraph was resolved' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT',
        description: '`result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The paragraph size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "abc \\xD7\\x90\\xD7\\x91\\xD7\\x92"; // abc אבג
mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve(input, strlen(input), MJB_ENC_UTF_8, MJB_DIRECTION_AUTO,
    &paragraph) != MJB_STATUS_OK) {
    return 1;
}

// Paragraph codepoints: 7
printf("Paragraph codepoints: %zu", paragraph.count);
mjb_bidi_paragraph_free(&paragraph);`,
    related: ['mjb_bidi_paragraph_free', 'mjb_bidi_reorder_line', 'mjb_bidi_line_runs'],
    specs: [uax(9, 'Unicode Bidirectional Algorithm')]
  },
  {
    comment: 'Reorder a line visually (L1-L4); visual_order is caller-allocated.',
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
        description: 'Caller-allocated array of size (`line_end` - `line_start`)',
        wasm_generated: false,
        ownership: 'Caller-allocated; the library does not retain or free it'
      }
    ],
    wasm: false,
    section: Section.Bidirectional,
    example: `const char *input = "\\xD7\\x90\\xD7\\x91\\xD7\\x92"; // אבג
mjb_bidi_paragraph paragraph;
size_t visual_order[3];

if(mjb_bidi_resolve(input, strlen(input), MJB_ENC_UTF_8, MJB_DIRECTION_AUTO,
    &paragraph) != MJB_STATUS_OK ||
    mjb_bidi_reorder_line(&paragraph, 0, paragraph.count,
        visual_order) != MJB_STATUS_OK) {
    return 1;
}

// First visual index: 2
printf("First visual index: %zu", visual_order[0]);
mjb_bidi_paragraph_free(&paragraph);`,
    related: ['mjb_bidi_resolve', 'mjb_bidi_line_runs'],
    specs: [uax(9, 'Unicode Bidirectional Algorithm')]
  },
  {
    comment: 'Compute visual level runs; pass runs=NULL to count first.',
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
        description: 'Visual order array from `mjb_bidi_reorder_line`',
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
        description: 'On output: number of runs written (or total if `runs` = `NULL`)',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Bidirectional,
    example: `mjb_bidi_paragraph paragraph;
size_t visual_order[3];
size_t run_count = 0;

if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
    &paragraph) != MJB_STATUS_OK ||
    mjb_bidi_reorder_line(&paragraph, 0, 3, visual_order) != MJB_STATUS_OK ||
    mjb_bidi_line_runs(&paragraph, visual_order, 3, NULL,
        &run_count) != MJB_STATUS_OK) {
    return 1;
}

// Visual runs: 1
printf("Visual runs: %zu", run_count);
mjb_bidi_paragraph_free(&paragraph);`,
    related: ['mjb_bidi_resolve', 'mjb_bidi_reorder_line'],
    specs: [uax(9, 'Unicode Bidirectional Algorithm')]
  },
  {
    comment: 'Free a bidi paragraph allocated by mjb_bidi_resolve.',
    ret: 'void',
    name: 'mjb_bidi_paragraph_free',
    attributes: [],
    args: [
      {
        name: 'paragraph',
        type: 'mjb_bidi_paragraph *',
        description: 'The paragraph to free',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Bidirectional,
    example: `mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
    &paragraph) != MJB_STATUS_OK) {
    return 1;
}

mjb_bidi_paragraph_free(&paragraph);

// Paragraph released: yes
printf("Paragraph released: %s", paragraph.chars == NULL ? "yes" : "no");`,
    related: ['mjb_bidi_resolve']
  },
  {
    comment: 'Return true if the codepoint is a valid Unicode identifier start (Unicode 18.0.0 UAX #31 ID_Start).',
    ret: 'bool',
    name: 'mjb_codepoint_is_id_start',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Greek alpha starts an identifier: yes
bool starts = mjb_codepoint_is_id_start(0x03B1);

// Greek alpha starts an identifier: yes
printf("Greek alpha starts an identifier: %s", starts ? "yes" : "no");`,
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return true if the codepoint is a valid Unicode identifier continuation (Unicode 18.0.0 UAX #31 ID_Continue).',
    ret: 'bool',
    name: 'mjb_codepoint_is_id_continue',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Digit 7 continues an identifier: yes
bool continues = mjb_codepoint_is_id_continue('7');

// Digit 7 continues an identifier: yes
printf("Digit 7 continues an identifier: %s", continues ? "yes" : "no");`,
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return true if the codepoint is a valid NFKC identifier start (Unicode 18.0.0 UAX #31 XID_Start).',
    ret: 'bool',
    name: 'mjb_codepoint_is_xid_start',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Letter A is XID_Start: yes
printf("Letter A is XID_Start: %s", mjb_codepoint_is_xid_start('A') ? "yes" : "no");`,
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return true if the codepoint is a valid NFKC identifier continuation (Unicode 18.0.0 UAX #31 XID_Continue).',
    ret: 'bool',
    name: 'mjb_codepoint_is_xid_continue',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Underscore is XID_Continue: yes
bool continues = mjb_codepoint_is_xid_continue('_');

// Underscore is XID_Continue: yes
printf("Underscore is XID_Continue: %s", continues ? "yes" : "no");`,
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return true if the codepoint is reserved for use in patterns (Unicode 18.0.0 UAX #31 Pattern_Syntax).',
    ret: 'bool',
    name: 'mjb_codepoint_is_pattern_syntax',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Plus sign is Pattern_Syntax: yes
bool syntax = mjb_codepoint_is_pattern_syntax('+');

// Plus sign is Pattern_Syntax: yes
printf("Plus sign is Pattern_Syntax: %s", syntax ? "yes" : "no");`,
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return true if the codepoint is pattern whitespace (Unicode 18.0.0 UAX #31 Pattern_White_Space).',
    ret: 'bool',
    name: 'mjb_codepoint_is_pattern_white_space',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Space is Pattern_White_Space: yes
bool whitespace = mjb_codepoint_is_pattern_white_space(' ');

// Space is Pattern_White_Space: yes
printf("Space is Pattern_White_Space: %s", whitespace ? "yes" : "no");`,
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return true if the string is a valid Unicode identifier (Unicode 18.0.0 UAX #31).',
    ret: 'bool',
    name: 'mjb_is_identifier',
    attributes: [],
    args: [
      buffer('The string to validate'),
      byte_length(),
      encoding(),
      {
        name: 'profile',
        type: 'mjb_identifier_profile',
        description: 'The identifier profile (DEFAULT or NFKC)',
        wasm_generated: false,
        is_enum: true
      }
    ],
    wasm: true,
    section: Section.Security,
    details: 'Validate a string as a Unicode identifier: the first character must be a valid ' +
      'identifier start and the following ones valid identifier continuations, using ID_Start/' +
      'ID_Continue for the DEFAULT profile or XID_Start/XID_Continue for the NFKC profile.',
    example: `const char *identifier = "delta_2";

bool valid = mjb_is_identifier(identifier, strlen(identifier), MJB_ENC_UTF_8,
    MJB_IDENTIFIER_NFKC);

// Valid identifier: yes
printf("Valid identifier: %s", valid ? "yes" : "no");`,
    related: ['mjb_codepoint_is_id_start', 'mjb_codepoint_is_id_continue',
      'mjb_codepoint_is_xid_start', 'mjb_codepoint_is_xid_continue'],
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return the UTS #39 resolved script set of a string.',
    ret: 'mjb_status',
    name: 'mjb_resolved_script_set',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to analyze'),
      byte_length(),
      encoding(),
      {
        name: 'scripts',
        type: 'mjb_script *',
        description: 'The caller-provided script buffer, or NULL to query the required count',
        wasm_generated: true
      },
      {
        name: 'count',
        type: 'size_t *',
        description: 'The input capacity and output script count',
        wasm_generated: true
      },
      {
        name: 'kind',
        type: 'mjb_script_set_kind *',
        description: 'Whether the resolved set is empty, concrete, or ALL',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Security,
    exampleFeature: 'MJB_FEATURE_SECURITY',
    details: 'Intersect the augmented Script_Extensions sets of every codepoint as specified by ' +
      'UTS #39. Common and Inherited resolve to ALL. Han, Hiragana, Katakana, Hangul, and ' +
      'Bopomofo are augmented with the Hanb, Jpan, and Kore writing-system values. Set `scripts` ' +
      'to NULL to query the required count. A mixed-script string returns EMPTY with a zero ' +
      'count; an empty string or a string containing only Common or Inherited characters returns ' +
      'ALL with a zero count. If `MJB_FEATURE_SECURITY=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required count and set kind were returned, or the resolved scripts were written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`count` or `kind` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The input encoding is invalid or lacks required byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The script buffer capacity is smaller than the required count' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_SECURITY=0`' }
    ],
    example: `const char *input = "\\xE3\\x81\\xAD\\xE3\\x82\\xAC"; // Hiragana + Katakana
mjb_script scripts[1];
size_t count = 1;
mjb_script_set_kind kind;

if(mjb_resolved_script_set(input, strlen(input), MJB_ENC_UTF_8, scripts, &count,
    &kind) != MJB_STATUS_OK) {
    return 1;
}

bool japanese = kind == MJB_SCRIPT_SET_RESOLVED && count == 1 && scripts[0] == MJB_SC_JPAN;

// Japanese writing system: yes
printf("Japanese writing system: %s", japanese ? "yes" : "no");`,
    related: ['mjb_codepoint_script_extensions', 'mjb_is_identifier',
      'mjb_confusable_skeleton', 'mjb_confusable_match'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Return the name of a property, NULL if the property specified is not valid.',
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
    wasm: true,
    section: Section.Utility,
    example: `const char *name = mjb_property_name(MJB_PR_ALPHABETIC);

// Property: Alphabetic
printf("Property: %s", name);`,
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Compute a Unicode confusable skeleton (Unicode 18.0.0 UTS #39 Section 4).',
    ret: 'mjb_status',
    name: 'mjb_confusable_skeleton',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to transform'),
      byte_length(),
      encoding(),
      encoding('The output encoding of the skeleton', 'output_encoding'),
      result()
    ],
    wasm: true,
    section: Section.Security,
    exampleFeature: 'MJB_FEATURE_SECURITY',
    details: 'Compute the UTS #39 `bidiSkeleton(LTR, input)`: apply the Unicode Bidirectional ' +
      'Algorithm through L4, then NFD, remove default-ignorables, substitute prototypes from ' +
      '`confusables.txt`, and reapply NFD. Skeletons can be stored or indexed so future ' +
      'confusable checks can compare them directly. If `MJB_FEATURE_SECURITY=0` the function ' +
      'always returns `MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The confusable skeleton was returned' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_SECURITY=0`' }
    ],
    example: `const char *input = "h\\xD0\\xB5llo"; // Cyrillic U+0435 in place of e
mjb_result result;

if(mjb_confusable_skeleton(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// hello
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
    related: ['mjb_confusable_skeleton_into', 'mjb_confusable_match', 'mjb_is_identifier'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Compute a Unicode confusable skeleton into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_confusable_skeleton_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to transform'),
      byte_length(),
      encoding(),
      encoding('The output encoding of the skeleton', 'output_encoding'),
      {
        name: 'output',
        type: 'void *',
        description: 'The caller-provided output buffer, or NULL to query the required size',
        wasm_generated: false,
        ownership: 'The caller retains ownership'
      },
      {
        name: 'output_size',
        type: 'size_t *',
        description: 'The input capacity and output required or written byte count',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Security,
    exampleFeature: 'MJB_FEATURE_SECURITY',
    details: 'Compute the same UTS #39 `bidiSkeleton(LTR, input)` as ' +
      '`mjb_confusable_skeleton` without allocating the final output buffer. Set `output` to ' +
      'NULL to query the required size. If `output` is non-NULL, `*output_size` supplies its ' +
      'capacity; on return it contains the required size when the buffer is too small, or the ' +
      'written size on success. Terminators are excluded and are not written. No bytes are ' +
      'written when capacity is insufficient. Bidirectional resolution, normalization, and ' +
      'skeleton mapping still require temporary allocations, including during a size query. If ' +
      '`MJB_FEATURE_SECURITY=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the skeleton was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output_size` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An encoding is invalid' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The output encoding cannot represent a skeleton codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Temporary allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_SECURITY=0`' }
    ],
    example: `const char *input = "h\\xD0\\xB5llo"; // Cyrillic U+0435 in place of e
size_t output_size = 0;

if(mjb_confusable_skeleton_into(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    NULL, &output_size) != MJB_STATUS_OK) {
    return 1;
}

char output[5];

if(output_size > sizeof(output) || mjb_confusable_skeleton_into(input, strlen(input),
    MJB_ENC_UTF_8, MJB_ENC_UTF_8, output, &output_size) != MJB_STATUS_OK) {
    return 1;
}

// Skeleton payload (no terminator): hello
printf("Skeleton payload (no terminator): %.*s", (int)output_size, output);`,
    related: ['mjb_confusable_skeleton', 'mjb_confusable_match', 'mjb_is_identifier'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Determine whether two strings are visually confusable (Unicode 18.0.0 UTS #39 Section 4): skeleton(s1) == skeleton(s2).',
    ret: 'mjb_status',
    name: 'mjb_confusable_match',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The first string', 's1'),
      byte_length('The length of the first string in bytes, or `MJB_NUL_TERMINATED`',
        's1_byte_length'),
      encoding('The encoding of the first string', 's1_encoding'),
      buffer('The second string', 's2'),
      byte_length('The length of the second string in bytes, or `MJB_NUL_TERMINATED`',
        's2_byte_length'),
      encoding('The encoding of the second string', 's2_encoding'),
      {
        name: 'confusable',
        type: 'bool *',
        description: 'Whether the strings are visually confusable',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Security,
    exampleFeature: 'MJB_FEATURE_SECURITY',
    details: 'Compute the confusable skeleton of both strings and store true when the ' +
      'skeletons are equal, meaning the two strings are visually confusable, such as ' +
      '"good" and "gооd" with Cyrillic о. If `MJB_FEATURE_SECURITY=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: '`confusable` contains the comparison result' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`confusable` is NULL, or an input buffer is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description: 'An input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An intermediate size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_SECURITY=0`' }
    ],
    example: `const char *latin = "hello";
const char *mixed = "h\\xD0\\xB5llo"; // Cyrillic е
bool confusable;

if(mjb_confusable_match(latin, strlen(latin), MJB_ENC_UTF_8,
    mixed, strlen(mixed), MJB_ENC_UTF_8, &confusable) != MJB_STATUS_OK) {
    return 1;
}

// Visually confusable: yes
printf("Visually confusable: %s", confusable ? "yes" : "no");`,
    related: ['mjb_confusable_skeleton', 'mjb_confusable_skeleton_into', 'mjb_is_identifier'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Return the emoji properties.',
    ret: 'mjb_status',
    name: 'mjb_codepoint_emoji_properties',
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
    wasm: true,
    section: Section.Emoji,
    example: `mjb_emoji_properties emoji;

if(mjb_codepoint_emoji_properties(0x1F600, &emoji) != MJB_STATUS_OK) {
    return 1;
}

// U+1F600 has Emoji_Presentation: yes
printf("U+1F600 has Emoji_Presentation: %s", emoji.presentation ? "yes" : "no");`,
    related: ['mjb_emoji_sequence_info', 'mjb_codepoint_is_emoji'],
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji property.',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.Emoji,
    example: `// Number sign has the Emoji property: yes
bool emoji = mjb_codepoint_is_emoji('#');

// Number sign has the Emoji property: yes
printf("Number sign has the Emoji property: %s", emoji ? "yes" : "no");`,
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Presentation property.',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_presentation',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.Emoji,
    example: `// Grinning face defaults to emoji presentation: yes
bool presentation = mjb_codepoint_is_emoji_presentation(0x1F600);

// Grinning face defaults to emoji presentation: yes
printf("Grinning face defaults to emoji presentation: %s", presentation ? "yes" : "no");`,
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Modifier property.',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_modifier',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.Emoji,
    example: `// Medium skin tone is an emoji modifier: yes
bool modifier = mjb_codepoint_is_emoji_modifier(0x1F3FD);

// Medium skin tone is an emoji modifier: yes
printf("Medium skin tone is an emoji modifier: %s", modifier ? "yes" : "no");`,
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Modifier_Base property.',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_modifier_base',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.Emoji,
    example: `// Waving hand accepts an emoji modifier: yes
bool modifier_base = mjb_codepoint_is_emoji_modifier_base(0x1F44B);

// Waving hand accepts an emoji modifier: yes
printf("Waving hand accepts an emoji modifier: %s", modifier_base ? "yes" : "no");`,
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the codepoint has the Unicode Emoji_Component property.',
    ret: 'bool',
    name: 'mjb_codepoint_is_emoji_component',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.Emoji,
    example: `// Zero-width joiner is an emoji component: yes
bool component = mjb_codepoint_is_emoji_component(0x200D);

// Zero-width joiner is an emoji component: yes
printf("Zero-width joiner is an emoji component: %s", component ? "yes" : "no");`,
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the codepoint has the Unicode Extended_Pictographic property.',
    ret: 'bool',
    name: 'mjb_codepoint_is_extended_pictographic',
    attributes: [],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Red heart is Extended_Pictographic: yes
bool pictographic = mjb_codepoint_is_extended_pictographic(0x2764);

// Red heart is Extended_Pictographic: yes
printf("Red heart is Extended_Pictographic: %s", pictographic ? "yes" : "no");`,
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return the plane of the codepoint.',
    ret: 'mjb_plane',
    name: 'mjb_codepoint_plane',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `mjb_plane plane = mjb_codepoint_plane(0x1F600);

// U+1F600 is in the SMP: yes
printf("U+1F600 is in the SMP: %s", plane == MJB_PLANE_SMP ? "yes" : "no");`
  },
  {
    comment: 'Return true if the plane is valid.',
    ret: 'bool',
    name: 'mjb_plane_is_valid',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'plane',
        type: 'mjb_plane',
        description: 'The plane to check',
        wasm_generated: false,
        is_enum: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Plane 16 is valid: yes
printf("Plane 16 is valid: %s", mjb_plane_is_valid(MJB_PLANE_PUA_B) ? "yes" : "no");`
  },
  {
    comment: 'Return the name of a plane, NULL if the plane specified is not valid.',
    ret: 'const char *',
    name: 'mjb_plane_name',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'plane',
        type: 'mjb_plane',
        description: 'The plane to check',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'abbreviation',
        type: 'bool',
        description: 'Whether to use an abbreviation',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// Plane: Basic Multilingual Plane
printf("Plane: %s", mjb_plane_name(MJB_PLANE_BMP, false));`
  },
  {
    comment: 'Return emoji sequence metadata for a complete string.',
    ret: 'mjb_status',
    name: 'mjb_emoji_sequence_info',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'emoji',
        type: 'mjb_emoji_sequence *',
        description: 'The emoji sequence metadata to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Emoji,
    example: `const char *flag = "\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9"; // 🇮🇹
mjb_emoji_sequence emoji;

if(mjb_emoji_sequence_info(flag, strlen(flag), MJB_ENC_UTF_8,
    &emoji) != MJB_STATUS_OK) {
    return 1;
}

// Sequence codepoints: 2
printf("Sequence codepoints: %zu", emoji.codepoint_count);`,
    related: ['mjb_is_emoji_sequence', 'mjb_is_rgi_emoji'],
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the complete string is an emoji sequence listed by Unicode, ' +
      'including standardized emoji variation sequences.',
    ret: 'bool',
    name: 'mjb_is_emoji_sequence',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding()
    ],
    section: Section.Emoji,
    wasm: true,
    example: `const char *keycap = "1\\xEF\\xB8\\x8F\\xE2\\x83\\xA3"; // 1️⃣

bool listed = mjb_is_emoji_sequence(keycap, strlen(keycap), MJB_ENC_UTF_8);

// Listed emoji sequence: yes
printf("Listed emoji sequence: %s", listed ? "yes" : "no");`,
    related: ['mjb_is_rgi_emoji', 'mjb_emoji_sequence_info'],
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the complete string is an RGI emoji sequence, excluding plain ' +
      'standardized variation sequences.',
    ret: 'bool',
    name: 'mjb_is_rgi_emoji',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding()
    ],
    wasm: true,
    wasmName: 'isRGIEmoji',
    section: Section.Emoji,
    example: `const char *flag = "\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9"; // 🇮🇹

bool rgi = mjb_is_rgi_emoji(flag, strlen(flag), MJB_ENC_UTF_8);

// RGI emoji: yes
printf("RGI emoji: %s", rgi ? "yes" : "no");`,
    related: ['mjb_is_emoji_sequence', 'mjb_emoji_sequence_info'],
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return hangul syllable name.',
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
      byte_length('The capacity of the output buffer, in bytes')
    ],
    wasm: true,
    section: Section.HangulLanguage,
    example: `char name[32];

if(mjb_hangul_syllable_name(0xAC01, name, sizeof(name)) != MJB_STATUS_OK) {
    return 1;
}

// Name: HANGUL SYLLABLE GAG
printf("Name: %s", name);`
  },
  {
    comment: 'Hangul syllable decomposition.',
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
    wasm: false,
    section: Section.HangulLanguage,
    example: `mjb_codepoint decomposition[3];

if(mjb_hangul_syllable_decomposition(0xAC01,
    decomposition) != MJB_STATUS_OK) {
    return 1;
}

// Decomposition starts with: U+1100
printf("Decomposition starts with: U+%04X", decomposition[0]);`
  },
  {
    comment: 'Hangul syllable composition.',
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
    wasm: false,
    section: Section.HangulLanguage,
    example: `mjb_buffer_character characters[] = {
    { 0x1100, 0 }, // choseong kiyeok
    { 0x1161, 0 }, // jungseong a
    { 0x11A8, 0 }  // jongseong kiyeok
};
size_t length = mjb_hangul_syllable_composition(characters, 3);

// Composition: U+AC01
printf("Composition: U+%04X", length == 1 ? characters[0].codepoint : 0);`
  },
  {
    comment: 'Return the east asian width of a codepoint.',
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
    wasm: true,
    section: Section.TerminalWidth,
    example: `mjb_east_asian_width width;

if(mjb_codepoint_east_asian_width(0x754C, &width) != MJB_STATUS_OK) { // 界
    return 1;
}

// U+754C is wide: yes
printf("U+754C is wide: %s", width == MJB_EAW_WIDE ? "yes" : "no");`,
    related: ['mjb_terminal_width'],
    specs: [uax(11, 'East Asian Width')]
  },
  {
    comment: 'Return the estimated terminal-cell width of printable, single-line text.',
    ret: 'mjb_status',
    name: 'mjb_terminal_width',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The printable, single-line string to measure'),
      byte_length(),
      encoding(),
      malformedPolicy(),
      {
        name: 'profile',
        type: 'mjb_terminal_width_profile',
        description: 'The terminal-width profile for ambiguous-width characters',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'width',
        type: 'size_t *',
        description: 'The number of terminal cells to store; set to zero on failure',
        wasm_generated: true
      },
      diagnostic()
    ],
    wasm: true,
    section: Section.TerminalWidth,
    details: 'Estimate the number of fixed terminal cells occupied by printable, single-line ' +
      'text. The input is normalized to NFC so canonically equivalent text has the same width. ' +
      'Combining and format characters occupy no additional cells, listed emoji-presentation ' +
      'sequences occupy two cells, and text-presentation sequences retain their East Asian Width. ' +
      'This is a deterministic terminal policy, not a measurement of proportional glyph advances. ' +
      'Use grapheme boundaries for cursor movement, selection, deletion, and user-perceived ' +
      'character counts. Controls, line separators, and paragraph separators are rejected because ' +
      'their effect depends on terminal state. Malformed subsequences follow `malformed_policy`, ' +
      'and `diagnostic` records the first one. On failure, `width` is set to zero.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The width was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`width` is NULL, the profile or malformed policy is invalid, or the buffer is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is invalid or lacks required byte-order information' },
      { value: 'MJB_STATUS_UNSUPPORTED', description: 'The input contains a control, line separator, or paragraph separator' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'NFC normalization could not allocate memory' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The width would overflow' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    example: `const char *input = "A\\xE7\\x95\\x8C"; // A界
size_t width;

if(mjb_terminal_width(input, strlen(input), MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_TERMINAL_WIDTH_NARROW, &width, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Terminal cells: 3
printf("Terminal cells: %zu", width);`,
    related: ['mjb_codepoint_east_asian_width', 'mjb_truncate_grapheme_width',
      'mjb_next_grapheme_break', 'mjb_is_rgi_emoji'],
    specs: [uax(11, 'East Asian Width'), uax(29, 'Unicode Text Segmentation'),
      uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Parse a BCP 47 language tag.',
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
        name: 'byte_length',
        type: 'size_t',
        description: 'The length of the locale identifier in bytes, or `MJB_NUL_TERMINATED`',
        wasm_generated: true
      },
      {
        name: 'encoding',
        type: 'mjb_encoding',
        description: 'The encoding of the locale identifier',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'locale',
        type: 'mjb_locale_id *',
        description: 'The locale structure to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Utility,
    details: 'Parse a BCP 47 language tag, such as `sr-Latn-RS`, into its components: ' +
      'language, extended language, script, region, variant, extensions, private use, and ' +
      'grandfathered tags. Parsing is strict: malformed tags are rejected.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The tag was parsed and `locale` filled' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: 'An argument is NULL or the tag is not a valid BCP 47 language tag' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `mjb_locale_id locale;

if(mjb_locale_parse("sr-Latn-RS", 10, MJB_ENC_UTF_8, &locale) != MJB_STATUS_OK) {
    return 1;
}

// Locale: sr Latn RS
printf("Locale: %s %s %s", locale.language, locale.script, locale.region);`,
    related: ['mjb_set_locale'],
    specs: [{ name: 'BCP 47: Tags for Identifying Languages', url: 'https://www.rfc-editor.org/rfc/rfc5646' }]
  },
  {
    comment: 'Set the current process-global locale.',
    ret: 'mjb_status',
    name: 'mjb_set_locale',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'locale',
        type: 'mjb_locale',
        description: 'The locale to set',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Utility,
    details: 'Set the process-global locale used by `mjb_map_case`. The default locale is ' +
      '`MJB_LOCALE_EN`, and `mjb_reset_locale` resets it to `MJB_LOCALE_EN`. Only ' +
      '`MJB_LOCALE_TR`, `MJB_LOCALE_AZ`, and `MJB_LOCALE_LT` currently tailor casing. Other ' +
      'valid locale values are accepted but do not change Unicode algorithm behavior.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The locale was set' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`locale` is not a valid `mjb_locale` value' }
    ],
    example: `if(mjb_set_locale(MJB_LOCALE_TR) != MJB_STATUS_OK) {
    return 1;
}

// Turkish locale selected: yes
printf("Turkish locale selected: yes");
if(mjb_set_locale(MJB_LOCALE_EN) != MJB_STATUS_OK) {
    return 1;
}`,
    related: ['mjb_get_locale', 'mjb_reset_locale', 'mjb_map_case', 'mjb_map_case_into']
  },
  {
    comment: 'Return the current process-global locale.',
    ret: 'mjb_locale',
    name: 'mjb_get_locale',
    attributes: ['MJB_PURE'],
    args: [],
    wasm: true,
    section: Section.Utility,
    details: 'Return the process-global locale selected with `mjb_set_locale`. The default is ' +
      '`MJB_LOCALE_EN`, and `mjb_reset_locale` restores that default.',
    returns: [
      { value: 'mjb_locale', description: 'The currently selected locale' }
    ],
    example: `mjb_locale locale = mjb_get_locale();

// Current locale is English: yes
printf("Current locale is English: %s", locale == MJB_LOCALE_EN ? "yes" : "no");`,
    related: ['mjb_set_locale', 'mjb_reset_locale', 'mjb_map_case', 'mjb_map_case_into']
  },
  {
    comment: 'Reset the process-global locale.',
    ret: 'void',
    name: 'mjb_reset_locale',
    attributes: [],
    args: [],
    wasm: false,
    section: Section.Utility,
    details: 'Restore the process-global locale to `MJB_LOCALE_EN`.',
    example: `if(mjb_set_locale(MJB_LOCALE_TR) != MJB_STATUS_OK) {
    return 1;
}

mjb_reset_locale();
mjb_locale locale = mjb_get_locale();

// Current locale reset to English: yes
printf("Current locale reset to English: %s", locale == MJB_LOCALE_EN ? "yes" : "no");`,
    related: ['mjb_set_locale', 'mjb_get_locale']
  },
  {
    comment: 'Free a mjb_result.',
    ret: 'mjb_status',
    name: 'mjb_result_free',
    attributes: [],
    args: [
      {
        name: 'result',
        type: 'mjb_result *',
        description: 'The result to free',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Utility,
    details: 'Free the memory allocated for a `mjb_result`. The `result` pointer is set to NULL.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The result was freed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`result` is NULL' }
    ],
    example: `mjb_result result;

if(mjb_convert_encoding("A", 1, MJB_ENC_UTF_8, MJB_MALFORMED_STOP,
    MJB_ENC_UTF_16LE, &result, NULL) != MJB_STATUS_OK ||
    mjb_result_free(&result) != MJB_STATUS_OK) {
    return 1;
}

// Result released: yes
printf("Result released: %s", result.output == NULL ? "yes" : "no");`
  },
  {
    comment: 'Format a UTF-8 string without leaving an incomplete trailing codepoint.',
    ret: 'int',
    name: 'mjb_utf8_snprintf',
    attributes: ['MJB_NODISCARD', 'MJB_PRINTF_FORMAT(3, 4)'],
    args: [
      {
        name: 'buffer',
        type: 'char *',
        description: 'The destination buffer, or NULL when buffer_size is zero',
        wasm_generated: false
      },
      {
        name: 'buffer_size',
        type: 'size_t',
        description: 'The destination buffer capacity in bytes, including the terminating NULL',
        wasm_generated: false
      },
      {
        name: 'format',
        type: 'const char *',
        description: 'The printf format string',
        wasm_generated: false
      }
    ],
    variadic: true,
    wasm: false,
    section: Section.Formatting,
    details: 'Use the C library formatting rules and return semantics of `snprintf`. If the ' +
      'destination buffer truncates a well-formed UTF-8 result, any incomplete trailing codepoint' +
      ' is removed before the terminating NULL. Truncation is at a codepoint boundary, not a ' +
      'grapheme-cluster boundary.',
    returns: [
      {
        value: 'A nonnegative value',
        description:
          'The number of bytes the complete result requires, excluding the terminating NULL'
      },
      {
        value: 'A negative value',
        description: 'The underlying `vsnprintf` reported an encoding error'
      }
    ],
    example: `char buffer[4];
int required = mjb_utf8_snprintf(buffer, sizeof(buffer), "%s",
    "\\xC3\\xA9\\xC3\\xA9"); // éé

// 4: é
printf("%d: %s", required, buffer);`,
    related: ['mjb_utf8_vsnprintf', 'mjb_utf8_grapheme_snprintf', 'mjb_is_utf8']
  },
  {
    comment:
      'Format a UTF-8 string from a va_list without leaving an incomplete trailing codepoint.',
    ret: 'int',
    name: 'mjb_utf8_vsnprintf',
    attributes: ['MJB_NODISCARD', 'MJB_PRINTF_FORMAT(3, 0)'],
    args: [
      {
        name: 'buffer',
        type: 'char *',
        description: 'The destination buffer, or NULL when buffer_size is zero',
        wasm_generated: false
      },
      {
        name: 'buffer_size',
        type: 'size_t',
        description: 'The destination buffer capacity in bytes, including the terminating NULL',
        wasm_generated: false
      },
      {
        name: 'format',
        type: 'const char *',
        description: 'The printf format string',
        wasm_generated: false
      },
      {
        name: 'args',
        type: 'va_list',
        description: 'The formatting arguments',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Formatting,
    details: 'The `va_list` counterpart of `mjb_utf8_snprintf`. It has the same UTF-8 input ' +
      'requirements, clipping behavior, and return semantics as `mjb_utf8_snprintf`.',
    returns: [
      {
        value: 'A nonnegative value',
        description:
          'The number of bytes the complete result requires, excluding the terminating NULL'
      },
      {
        value: 'A negative value',
        description: 'The underlying `vsnprintf` reported an encoding error'
      }
    ],
    related: ['mjb_utf8_snprintf', 'mjb_utf8_grapheme_vsnprintf', 'mjb_is_utf8']
  },
  {
    comment: 'Format UTF-8 without truncating an extended grapheme cluster.',
    ret: 'int',
    name: 'mjb_utf8_grapheme_snprintf',
    attributes: ['MJB_NODISCARD', 'MJB_PRINTF_FORMAT(3, 4)'],
    args: [
      {
        name: 'buffer',
        type: 'char *',
        description: 'The destination buffer, or NULL when buffer_size is zero',
        wasm_generated: false
      },
      {
        name: 'buffer_size',
        type: 'size_t',
        description: 'The destination buffer capacity in bytes, including the terminating NULL',
        wasm_generated: false
      },
      {
        name: 'format',
        type: 'const char *',
        description: 'The printf format string',
        wasm_generated: false
      }
    ],
    variadic: true,
    wasm: false,
    section: Section.Formatting,
    details: 'Use the C library formatting rules and return semantics of `snprintf`. If the ' +
      'destination buffer truncates a well-formed UTF-8 result, the output is shortened to the ' +
      'largest prefix that ends at an extended grapheme-cluster boundary in the complete result. ' +
      'Unlike `mjb_utf8_snprintf`, truncation can require temporary allocation and a second ' +
      'evaluation of the format. Do not use `%n` or arguments whose values can change as a ' +
      'formatting side effect. On allocation failure the destination is set to an empty string, ' +
      'the function returns a negative value, and `errno` is set to `ENOMEM`.',
    returns: [
      {
        value: 'A nonnegative value',
        description:
          'The number of bytes the complete result requires, excluding the terminating NULL'
      },
      {
        value: 'A negative value',
        description: 'Formatting failed or the complete result could not be obtained'
      }
    ],
    example: `char buffer[4];
int required = mjb_utf8_grapheme_snprintf(buffer, sizeof(buffer), "%s",
    "Ae\\xCC\\x81" "B"); // A, e + combining acute accent, B

// 5: A
printf("%d: %s", required, buffer);`,
    related: ['mjb_utf8_grapheme_vsnprintf', 'mjb_utf8_snprintf',
      'mjb_truncate_grapheme'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Format UTF-8 from a va_list without truncating an extended grapheme cluster.',
    ret: 'int',
    name: 'mjb_utf8_grapheme_vsnprintf',
    attributes: ['MJB_NODISCARD', 'MJB_PRINTF_FORMAT(3, 0)'],
    args: [
      {
        name: 'buffer',
        type: 'char *',
        description: 'The destination buffer, or NULL when buffer_size is zero',
        wasm_generated: false
      },
      {
        name: 'buffer_size',
        type: 'size_t',
        description: 'The destination buffer capacity in bytes, including the terminating NULL',
        wasm_generated: false
      },
      {
        name: 'format',
        type: 'const char *',
        description: 'The printf format string',
        wasm_generated: false
      },
      {
        name: 'args',
        type: 'va_list',
        description: 'The formatting arguments',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Formatting,
    details: 'The `va_list` counterpart of `mjb_utf8_grapheme_snprintf`. It has the same ' +
      'grapheme-safe clipping behavior, allocation requirements, and return semantics as ' +
      '`mjb_utf8_grapheme_snprintf`.',
    returns: [
      {
        value: 'A nonnegative value',
        description:
          'The number of bytes the complete result requires, excluding the terminating NULL'
      },
      {
        value: 'A negative value',
        description: 'Formatting failed or the complete result could not be obtained'
      }
    ],
    related: ['mjb_utf8_grapheme_snprintf', 'mjb_utf8_vsnprintf',
      'mjb_truncate_grapheme'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return a UTF-8 English-language message that describes a status code.',
    ret: 'const char *',
    name: 'mjb_status_message',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'status',
        type: 'mjb_status',
        description: 'The status code to describe',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Utility,
    details: 'Return a static English-language message of at most 100 characters.',
    example: `const char *message = mjb_status_message(MJB_STATUS_INVALID_ARGUMENT);

// Error message: One or more arguments are invalid or inconsistent with the requested operation
printf("Error message: %s", message);`,
    related: []
  },
  {
    comment: 'Output the current library version (MJB_VERSION).',
    ret: 'const char *',
    name: 'mjb_version',
    attributes: ['MJB_CONST'],
    args: [],
    wasm: true,
    section: Section.Utility,
    details: 'Output the current library version as a string, such as "1.0.0".',
    example: `const char *version = mjb_version();

// Version is available: yes
printf("Version is available: %s", version[0] != '\\0' ? "yes" : "no");`,
    related: ['mjb_version_number', 'mjb_unicode_version']
  },
  {
    comment: 'Output the current library version number (MJB_VERSION_NUMBER).',
    ret: 'unsigned int',
    name: 'mjb_version_number',
    attributes: ['MJB_CONST'],
    args: [],
    wasm: true,
    section: Section.Utility,
    details: 'Output the current library version number as an unsigned integer.',
    example: `unsigned int version = mjb_version_number();

// Version number is positive: yes
printf("Version number is positive: %s", version > 0 ? "yes" : "no");`,
    related: ['mjb_version', 'mjb_unicode_version']
  },
  {
    comment: 'Output the current supported Unicode version (MJB_UNICODE_VERSION).',
    ret: 'const char *',
    name: 'mjb_unicode_version',
    attributes: ['MJB_CONST'],
    args: [],
    wasm: true,
    section: Section.Utility,
    details: 'Output the current supported Unicode version as a string, such as "15.0.0".',
    example: `const char *version = mjb_unicode_version();

// Unicode version: 18.0.0
printf("Unicode version: %s", version);`,
    related: ['mjb_version', 'mjb_version_number']
  },
  {
    comment: 'Set the process-global memory allocator.',
    ret: 'mjb_status',
    name: 'mjb_set_allocator',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'allocator',
        type: 'const mjb_allocator *',
        description: 'The complete allocator to copy, or NULL to select the default allocator',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Utility,
    details: 'Install the allocator used for internal allocations and owned result buffers. ' +
      'The allocator is copied and must provide all three callbacks, which receive its context ' +
      'pointer and follow the corresponding C standard library semantics. The context remains ' +
      'caller-owned and must stay valid for the lifetime of all Mojibake allocations. Pass NULL ' +
      'to explicitly select the default allocator. This process-global configuration may be ' +
      'set only once and must happen before any other library call or concurrent library use.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The allocator was installed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT',
        description: 'A callback is NULL, or the allocator was already configured' }
    ],
    related: ['mjb_result_free', 'mjb_bidi_paragraph_free']
  }
] as MojibakeFunction[];
