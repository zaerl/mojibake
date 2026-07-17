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
  Security,
  Segmentation,
  Bidirectional,
  Emoji,
  DisplayWidth,
  HangulLanguage,
  Utility
}

export type MojibakeFunction = {
  comment: string;
  ret: string;
  name: string;
  attributes: string[];
  args: MojibakeArg[];
  wasm: boolean;
  section: Section;
  // Long-form description.
  details?: string;
  // Documented return values.
  returns?: MojibakeReturnCase[];
  // Compilable C example. See utils/generate/generate-examples.ts
  example?: string;
  // Names of related public functions.
  related?: string[];
  // Unicode specifications implemented or referenced by the function.
  specs?: MojibakeSpecRef[];
};

const unicodeVersion = '17.0.0';

const uaxRevisions: Record<number, number> = {
  9: 51,
  11: 44,
  14: 55,
  15: 57,
  24: 39,
  29: 47,
  31: 43,
  44: 36
};

const utsRevisions: Record<number, number> = {
  10: 53,
  39: 32,
  51: 29
};

function buffer(description: string, name = 'buffer', isConst = true, wasm_generated = false): MojibakeArg {
  return {
    name,
    type: isConst ? 'const char *' : 'char *',
    description,
    wasm_generated
  };
}

function byte_length(description = 'The length of the string, in bytes', name = 'byte_length'): MojibakeArg {
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
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Fill `character` with the Unicode Character Database record of a codepoint: name, ' +
      'category, combining class, bidirectional category, decomposition, numeric values, ' +
      'mirrored flag, and simple case mappings. When the library is compiled with ' +
      '`MJB_FEATURE_CHARACTER_NAMES=OFF` the name field is reported as `Codepoint U+XXXX`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The character was found and filled' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`character` is NULL or the codepoint is not valid' },
      { value: 'MJB_STATUS_NOT_FOUND', description: 'The codepoint is not assigned' }
    ],
    example: `mjb_character character;

if(mjb_codepoint_character(0x022A, &character) != MJB_STATUS_OK) {
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
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to use',
        wasm_generated: false
      },
      encoding('The output encoding of the string', 'output_encoding'),
      result()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Normalize a string to the requested Unicode normalization form. If the input is ' +
      'already normalized and no encoding conversion is needed, the input buffer is returned ' +
      'as-is in `result->output` with `result->transformed` set to false, without allocating.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The string was normalized (or already normal)' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_FORM', description: '`form` is not NFC, NFD, NFKC, or NFKD' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "Cafe\\xCC\\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
mjb_result result;

if(mjb_normalize(input, strlen(input), MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// NFC: Café
printf("NFC: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}`,
    related: ['mjb_string_is_normalized', 'mjb_string_filter'],
    specs: [uax(15, 'Unicode Normalization Forms')]
  },
  {
    comment: 'Filter a string with the selected mjb_filter flags.',
    ret: 'mjb_status',
    name: 'mjb_string_filter',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to filter'),
      byte_length(),
      encoding(),
      {
        name: 'filters',
        type: 'mjb_filter',
        description: 'The filters to use',
        wasm_generated: false
      },
      encoding('The output encoding of the string', 'output_encoding'),
      result()
    ],
    details: '`MJB_FILTER_LIMIT_COMBINING` removes combining marks after the first ' +
      '`MJB_FILTER_MAX_COMBINING_MARKS` consecutive marks in an emitted run. This is useful ' +
      'for reducing Zalgo-style text while keeping ordinary accents and stacked marks.',
    wasm: true,
    section: Section.TextTransformation,
    example: `const char *mixed_whitespace = "Hello\\t\\t\\n\\nworld";
mjb_result result;

if(mjb_string_filter(mixed_whitespace, strlen(mixed_whitespace), MJB_ENC_UTF_8,
    MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: Hello world
printf("Filtered: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}

const char *controls = "\\x1\\x2\\t\\n\\v\\f\\r\\x1f";

if(mjb_string_filter(controls, strlen(controls), MJB_ENC_UTF_8, MJB_FILTER_CONTROLS,
    MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: \\t\\n\\v\\f\\r
printf("Filtered: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}`,
related: ['mjb_normalize']
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
      encoding('The output encoding of the string', 'output_encoding'),
      result()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Apply the normative `NFKC_Casefold` mapping and normalize the result to NFC. ' +
      'This transform performs compatibility folding, full default case folding, and removal ' +
      'of default-ignorable codepoints. It is intended for identifier comparison and is not ' +
      'locale-sensitive.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The transformed string was returned' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "Stra\\xC3\\x9F" "e\\xC2\\xAD";
mjb_result result;

if(mjb_nfkc_casefold(input, strlen(input), MJB_ENC_UTF_8, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// strasse
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
    related: ['mjb_normalize', 'mjb_case', 'mjb_string_is_identifier'],
    specs: [
      unicodeCore('Section 3.13', 'Default Case Algorithms', 'G33992'),
      uax(31, 'Unicode Identifiers and Syntax'),
      uax(44, 'Unicode Character Database')
    ]
  },
  {
    comment: 'Check if a string is normalized to NFC/NFKC/NFD/NFKD form.',
    ret: 'mjb_quick_check_result',
    name: 'mjb_string_is_normalized',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to check',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Run the normalization quick-check on a string without allocating. `MJB_QC_MAYBE` ' +
      'means the string may still be normalized, and only a full normalization pass with ' +
      '`mjb_normalize` can decide.',
    returns: [
      { value: 'MJB_QC_YES', description: 'The string is normalized to the requested form' },
      { value: 'MJB_QC_NO', description: 'The string is not normalized' },
      { value: 'MJB_QC_MAYBE', description: 'Inconclusive: a full normalization is needed to decide' }
    ],
    example: `const char *input = "caf\\xC3\\xA9";
mjb_quick_check_result check = mjb_string_is_normalized(input, strlen(input),
    MJB_ENC_UTF_8, MJB_NORMALIZATION_NFC);

// NFC normalized: yes
printf("NFC normalized: %s", check == MJB_QC_YES ? "yes" : "no");`,
    related: ['mjb_normalize'],
    specs: [uax(15, 'Unicode Normalization Forms')]
  },
  {
    comment: 'Return the string encoding (the most probable).',
    ret: 'mjb_encoding',
    name: 'mjb_string_encoding',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length()
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: '`mjb_string_encoding` reports BOM-derived UTF-16/UTF-32 schemes with the generic ' +
      'family bit plus the resolved endian bit. Passing that detected value consumes the leading ' +
      'BOM as a signature. Passing an explicit-endian encoding such as `MJB_ENC_UTF_16BE` preserves ' +
      'an initial U+FEFF as text. When flags overlap, as with a UTF-32LE BOM that also has the ' +
      'UTF-16LE BOM prefix, decoding gives UTF-32 precedence.',
    example: `const char utf16le[] = "\\xFF\\xFEH\\0i\\0";
mjb_encoding detected = mjb_string_encoding(utf16le, sizeof(utf16le) - 1);
bool is_utf16le = detected == (MJB_ENC_UTF_16 | MJB_ENC_UTF_16LE);

// UTF-16LE detected: yes
printf("UTF-16LE detected: %s", is_utf16le ? "yes" : "no");`
  },
  {
    comment: 'Return true if the string is encoded in ASCII.',
    ret: 'bool',
    name: 'mjb_string_is_ascii',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length()
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `const char *input = "Plain ASCII";

// ASCII: yes
printf("ASCII: %s", mjb_string_is_ascii(input, strlen(input)) ? "yes" : "no");`
  },
  {
    comment: 'Return true if the string is encoded in UTF-8.',
    ret: 'bool',
    name: 'mjb_string_is_utf8',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length()
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `const char *input = "caf\\xC3\\xA9";

// Valid UTF-8: yes
printf("Valid UTF-8: %s", mjb_string_is_utf8(input, strlen(input)) ? "yes" : "no");`
  },
  {
    comment: 'Return true if the string is encoded in UTF-16BE or UTF-16LE.',
    ret: 'bool',
    name: 'mjb_string_is_utf16',
    attributes: [
      'MJB_PURE'
    ],
    args: [
      buffer('The string to check'),
      byte_length()
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `const char utf16be[] = "\\xFE\\xFF\\0H\\0i"; // BOM + "Hi" in UTF-16BE

// UTF-16: yes
printf("UTF-16: %s", mjb_string_is_utf16(utf16be, sizeof(utf16be) - 1) ? "yes" : "no");`
  },
  {
    comment: 'Return the length of a string.',
    ret: 'size_t',
    name: 'mjb_string_length',
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
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Return the number of Unicode codepoints in a string, up to `max_length` bytes.',
    example: `// The "Héllö" string is five Unicode characters, but has different byte lengths in different encodings.

const char *utf8 = "H\\xC3\\xA9ll\\xC3\\xB6"; // 7 bytes
const char utf16le[] = "H\\0\\xE9\\0l\\0l\\0\\xF6\\0"; // 10 bytes
const char utf16be[] = "\\0H\\0\\xE9\\0l\\0l\\0\\xF6"; // 10 bytes
const char utf32le[] = "H\\0\\0\\0\\xE9\\0\\0\\0l\\0\\0\\0l\\0\\0\\0\\xF6\\0\\0\\0"; // 20 bytes
const char utf32be[] = "\\0\\0\\0H\\0\\0\\0\\xE9\\0\\0\\0l\\0\\0\\0l\\0\\0\\0\\xF6"; // 20 bytes

// 5 UTF-8 characters
printf("%zu UTF-8 characters", mjb_string_length(utf8, 7, MJB_ENC_UTF_8));
// 5 UTF-16LE characters
printf("%zu UTF-16LE characters", mjb_string_length(utf16le, 10, MJB_ENC_UTF_16LE));
// 5 UTF-16BE characters
printf("%zu UTF-16BE characters", mjb_string_length(utf16be, 10, MJB_ENC_UTF_16BE));
// 5 UTF-32LE characters
printf("%zu UTF-32LE characters", mjb_string_length(utf32le, 20, MJB_ENC_UTF_32LE));
// 5 UTF-32BE characters
printf("%zu UTF-32BE characters", mjb_string_length(utf32be, 20, MJB_ENC_UTF_32BE));`
  },
  {
    comment: 'Run a callback for each character of a string.',
    ret: 'mjb_status',
    name: 'mjb_string_each_character',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding(),
      {
        name: 'callback',
        type: 'mjb_string_each_character_fn',
        description: 'The function to call for each character',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    example: `mjb_status status = mjb_string_each_character("ABC", 3, MJB_ENC_UTF_8, NULL);

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
    name: 'mjb_string_convert_encoding',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to convert'),
      byte_length(),
      encoding('The input encoding of the string'),
      encoding('The output encoding of the string', 'output_encoding'),
      result()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Convert a string between the supported encodings (UTF-8, UTF-16LE/BE, ' +
      'UTF-32LE/BE). Generic UTF-16/UTF-32 input consumes a leading BOM as the encoding scheme ' +
      'signature and uses it to resolve byte order. Explicit-endian input preserves an initial ' +
      'U+FEFF as text. Generic UTF-16/UTF-32 without a BOM, and generic UTF-16/UTF-32 output, are ' +
      'rejected because the byte order is not specified.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The string was converted' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` is NULL, `buffer` is NULL with a non-zero size, or the input is not valid in the source encoding' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'A generic UTF-16/UTF-32 encoding did not provide enough byte order information' },
      { value: 'MJB_STATUS_UNSUPPORTED', description: 'The requested encoding conversion is not supported' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "caf\\xC3\\xA9";
mjb_result result;

if(mjb_string_convert_encoding(input, strlen(input), MJB_ENC_UTF_8,
    MJB_ENC_UTF_16LE, &result) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE bytes: 8
printf("UTF-16LE bytes: %zu", result.output_size);
mjb_result_free(&result);`,
    related: ['mjb_string_encoding', 'mjb_codepoint_encode']
  },
  {
    comment: 'Compare two strings using UCA.',
    ret: 'int',
    name: 'mjb_string_compare',
    attributes: [],
    args: [
      buffer('The first string to compare', 's1'),
      byte_length('The length of the first string, in bytes', 's1_byte_length'),
      encoding('The encoding of the first string', 's1_encoding'),
      buffer('The second string to compare', 's2'),
      byte_length('The length of the second string, in bytes', 's2_byte_length'),
      encoding('The encoding of the second string', 's2_encoding'),
      {
        name: 'mode',
        type: 'mjb_collation_mode',
        description: 'The variable weighting strategy',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.SortingComparison,
    details: 'Compare two strings using the Unicode Collation Algorithm and the default ' +
      'collation element table (DUCET), with `strcmp`-style semantics.',
    returns: [
      { value: '< 0', description: 'The first string collates before the second' },
      { value: '0', description: 'The strings are equal under UCA' },
      { value: '> 0', description: 'The first string collates after the second' }
    ],
    example: `int order = mjb_string_compare("apple", 5, MJB_ENC_UTF_8,
    "banana", 6, MJB_ENC_UTF_8, MJB_COLLATION_NON_IGNORABLE);

// apple sorts before banana: yes
printf("apple sorts before banana: %s", order < 0 ? "yes" : "no");`,
    related: ['mjb_collation_key'],
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
      {
        name: 'mode',
        type: 'mjb_collation_mode',
        description: 'The variable weighting strategy',
        wasm_generated: false
      },
      result('The pointer to store the binary sort key')
    ],
    wasm: true,
    section: Section.SortingComparison,
    details: 'Generate a binary sort key for a string. Sort keys of different strings can be ' +
      'compared with `memcmp` and yield the same order as `mjb_string_compare`. Useful when ' +
      'the same strings are compared many times, such as sorting or database indexing.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The sort key was generated' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT',
        description: '`result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The sort key size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `mjb_result key;

if(mjb_collation_key("r\\xC3\\xA9sum\\xC3\\xA9", 8, MJB_ENC_UTF_8,
    MJB_COLLATION_NON_IGNORABLE, &key) != MJB_STATUS_OK) {
    return 1;
}

// Sort key is non-empty: yes
printf("Sort key is non-empty: %s", key.output_size > 0 ? "yes" : "no");
mjb_result_free(&key);`,
    related: ['mjb_string_compare'],
    specs: [uts(10, 'Unicode Collation Algorithm')]
  },
  {
    comment: 'Change string case.',
    ret: 'mjb_status',
    name: 'mjb_case',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to change case'),
      byte_length(),
      encoding(),
      {
        name: 'type',
        type: 'mjb_case_type',
        description: 'The type of case change',
        wasm_generated: false
      },
      encoding('The output encoding of the string', 'output_encoding'),
      result()
    ],
    wasm: true,
    section: Section.TextTransformation,
    details: 'Convert a string to uppercase, lowercase, titlecase, or its case-folded form. ' +
      'Full case mappings are applied, including special casing and conditional mappings, so ' +
      'the output may have a different length than the input. Titlecase uses UAX #29 word ' +
      'boundaries: the first cased character in each word segment is titlecased, and ' +
      'subsequent characters in that segment are lowercased. Casing is tailored by the ' +
      'process-global locale set with `mjb_locale_set`: the default `MJB_LOCALE_EN` uses ' +
      'default non-Turkic mappings. `MJB_LOCALE_TR` and `MJB_LOCALE_AZ` apply ' +
      'Turkish/Azerbaijani dotted-I casing and Turkic `T` case-folding mappings. ' +
      '`MJB_LOCALE_LT` applies Lithuanian dot-above casing rules, while case folding remains ' +
      'the default non-Turkic mapping.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The case conversion succeeded' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` is NULL, `buffer` is NULL with a non-zero size, or `type` is not a valid case type' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `const char *input = "Stra\\xC3\\x9F""e"; // "Straße"
mjb_result result;

if(mjb_case(input, strlen(input), MJB_ENC_UTF_8, MJB_CASE_UPPER, MJB_ENC_UTF_8,
    &result) != MJB_STATUS_OK) {
    return 1;
}

// Upper: STRASSE
printf("Upper: %.*s", (int)result.output_size, result.output);

if(result.transformed) {
    mjb_free(result.output);
}`,
    related: ['mjb_locale_set'],
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
    name: 'mjb_codepoint_is_hangul_l',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+1100 is a leading Jamo: yes
printf("U+1100 is a leading Jamo: %s", mjb_codepoint_is_hangul_l(0x1100) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is a hangul V.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_v',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+1161 is a vowel Jamo: yes
printf("U+1161 is a vowel Jamo: %s", mjb_codepoint_is_hangul_v(0x1161) ? "yes" : "no");`
  },
  {
    comment: 'Return if the codepoint is a hangul T.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_t',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    example: `// U+11A8 is a trailing Jamo: yes
printf("U+11A8 is a trailing Jamo: %s", mjb_codepoint_is_hangul_t(0x11A8) ? "yes" : "no");`
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
    name: 'mjb_codepoint_is_cjk_ext',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    example: `// U+20000 is a CJK extension ideograph: yes
printf("U+20000 is a CJK extension ideograph: %s", mjb_codepoint_is_cjk_ext(0x20000) ? "yes" : "no");`
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
        wasm_generated: false
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
        wasm_generated: false
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
    name: 'mjb_break_line',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
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
mjb_break_type type = mjb_break_line("Hello world", 11, MJB_ENC_UTF_8, &state);

// First line-break result is set: yes
printf("First line-break result is set: %s", type != MJB_BT_NOT_SET ? "yes" : "no");`,
    related: ['mjb_break_grapheme_cluster', 'mjb_break_word', 'mjb_break_sentence'],
    specs: [uax(14, 'Unicode Line Breaking Algorithm')]
  },
  {
    comment: 'Word cluster breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_break_word',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
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

while(mjb_break_word("Hello world", 11, MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Word-break positions: 11
printf("Word-break positions: %zu", boundaries);`,
    related: ['mjb_break_grapheme_cluster', 'mjb_break_sentence', 'mjb_truncate_word'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Sentence boundaries breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_break_sentence',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
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

while(mjb_break_sentence(input, strlen(input), MJB_ENC_UTF_8, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Sentence-break positions: 15
printf("Sentence-break positions: %zu", boundaries);`,
    related: ['mjb_break_grapheme_cluster', 'mjb_break_word'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Grapheme cluster breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_break_grapheme_cluster',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
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
      'Call repeatedly with the same state until it reports the end of the string.',
    example: `const char *input = "e\\xCC\\x81"; // e + combining acute accent
mjb_next_state state;
state.index = 0;
size_t codepoints = 0;

while(mjb_break_grapheme_cluster(input, strlen(input), MJB_ENC_UTF_8,
    &state) != MJB_BT_NOT_SET) {
    ++codepoints;
}

// Codepoints examined: 2
printf("Codepoints examined: %zu", codepoints);`,
    related: ['mjb_break_word', 'mjb_break_sentence', 'mjb_break_line', 'mjb_truncate'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return the number of bytes that form the first `max_graphemes` grapheme cluster segments.',
    ret: 'size_t',
    name: 'mjb_truncate',
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
size_t bytes = mjb_truncate(input, strlen(input), MJB_ENC_UTF_8, 2);

// First two graphemes use 9 bytes
printf("First two graphemes use %zu bytes", bytes);`
  },
  {
    comment: 'Return the number of bytes whose grapheme clusters fit within max_columns display columns.',
    ret: 'size_t',
    name: 'mjb_truncate_width',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
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
    wasm: true,
    section: Section.DisplayWidth,
    example: `const char *input = "A\\xE7\\x95\\x8C"; // A界
size_t bytes = mjb_truncate_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_WIDTH_CONTEXT_WESTERN, 2);

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
    comment: 'Return the number of bytes whose word-break segments fit within max_columns display columns.',
    ret: 'size_t',
    name: 'mjb_truncate_word_width',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
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
    wasm: true,
    section: Section.Segmentation,
    example: `const char *input = "Hello world";
size_t bytes = mjb_truncate_word_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_WIDTH_CONTEXT_WESTERN, 6);

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
        wasm_generated: false
      },
      {
        name: 'result',
        type: 'mjb_bidi_paragraph *',
        description: 'Output paragraph; chars is library-allocated',
        wasm_generated: false,
        ownership: '`result->chars` is library-allocated and must be freed with `mjb_bidi_free()`'
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
mjb_bidi_free(&paragraph);`,
    related: ['mjb_bidi_free', 'mjb_bidi_reorder_line', 'mjb_bidi_line_runs'],
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
mjb_bidi_free(&paragraph);`,
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
mjb_bidi_free(&paragraph);`,
    related: ['mjb_bidi_resolve', 'mjb_bidi_reorder_line'],
    specs: [uax(9, 'Unicode Bidirectional Algorithm')]
  },
  {
    comment: 'Free a bidi paragraph allocated by mjb_bidi_resolve.',
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
    wasm: false,
    section: Section.Bidirectional,
    example: `mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve("abc", 3, MJB_ENC_UTF_8, MJB_DIRECTION_LTR,
    &paragraph) != MJB_STATUS_OK) {
    return 1;
}

mjb_bidi_free(&paragraph);

// Paragraph released: yes
printf("Paragraph released: %s", paragraph.chars == NULL ? "yes" : "no");`,
    related: ['mjb_bidi_resolve']
  },
  {
    comment: 'Return true if the codepoint is a valid Unicode identifier start (Unicode 17.0.0 UAX #31 ID_Start).',
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
    comment: 'Return true if the codepoint is a valid Unicode identifier continuation (Unicode 17.0.0 UAX #31 ID_Continue).',
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
    comment: 'Return true if the codepoint is a valid NFKC identifier start (Unicode 17.0.0 UAX #31 XID_Start).',
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
    comment: 'Return true if the codepoint is a valid NFKC identifier continuation (Unicode 17.0.0 UAX #31 XID_Continue).',
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
    comment: 'Return true if the codepoint is reserved for use in patterns (Unicode 17.0.0 UAX #31 Pattern_Syntax).',
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
    comment: 'Return true if the codepoint is pattern whitespace (Unicode 17.0.0 UAX #31 Pattern_White_Space).',
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
    comment: 'Return true if the string is a valid Unicode identifier (Unicode 17.0.0 UAX #31).',
    ret: 'bool',
    name: 'mjb_string_is_identifier',
    attributes: [],
    args: [
      buffer('The string to validate'),
      byte_length(),
      encoding(),
      {
        name: 'profile',
        type: 'mjb_identifier_profile',
        description: 'The identifier profile (DEFAULT or NFKC)',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Security,
    details: 'Validate a string as a Unicode identifier: the first character must be a valid ' +
      'identifier start and the following ones valid identifier continuations, using ID_Start/' +
      'ID_Continue for the DEFAULT profile or XID_Start/XID_Continue for the NFKC profile.',
    example: `const char *identifier = "delta_2";

bool valid = mjb_string_is_identifier(identifier, strlen(identifier), MJB_ENC_UTF_8,
    MJB_IDENTIFIER_NFKC);

// Valid identifier: yes
printf("Valid identifier: %s", valid ? "yes" : "no");`,
    related: ['mjb_codepoint_is_id_start', 'mjb_codepoint_is_id_continue',
      'mjb_codepoint_is_xid_start', 'mjb_codepoint_is_xid_continue'],
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
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
    comment: 'Compute a Unicode confusable skeleton (Unicode 17.0.0 UTS #39 Section 4).',
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
    details: 'Compute the UTS #39 `bidiSkeleton(LTR, input)`: apply the Unicode Bidirectional ' +
      'Algorithm through L4, then NFD, remove default-ignorables, substitute prototypes from ' +
      '`confusables.txt`, and reapply NFD. Skeletons can be stored or indexed so future ' +
      'confusable checks can compare them directly.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The confusable skeleton was returned' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`result` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
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
    related: ['mjb_string_is_confusable', 'mjb_string_is_identifier'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Return true if two strings are visually confusable (Unicode 17.0.0 UTS #39 Section 4): skeleton(s1) == skeleton(s2).',
    ret: 'bool',
    name: 'mjb_string_is_confusable',
    attributes: [],
    args: [
      buffer('The first string', 's1'),
      byte_length('The length of the first string, in bytes', 's1_byte_length'),
      encoding('The encoding of the first string', 's1_encoding'),
      buffer('The second string', 's2'),
      byte_length('The length of the second string, in bytes', 's2_byte_length'),
      encoding('The encoding of the second string', 's2_encoding')
    ],
    wasm: true,
    section: Section.Security,
    details: 'Compute the confusable skeleton of both strings and return true when the ' +
      'skeletons are equal, meaning the two strings are visually confusable, such as ' +
      '"good" and "gооd" with Cyrillic о.',
    example: `const char *latin = "hello";
const char *mixed = "h\\xD0\\xB5llo"; // Cyrillic е

bool confusable = mjb_string_is_confusable(latin, strlen(latin), MJB_ENC_UTF_8,
    mixed, strlen(mixed), MJB_ENC_UTF_8);

// Visually confusable: yes
printf("Visually confusable: %s", confusable ? "yes" : "no");`,
    related: ['mjb_confusable_skeleton', 'mjb_string_is_identifier'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Return the emoji properties.',
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
    wasm: true,
    section: Section.Emoji,
    example: `mjb_emoji_properties emoji;

if(mjb_codepoint_emoji(0x1F600, &emoji) != MJB_STATUS_OK) {
    return 1;
}

// U+1F600 has Emoji_Presentation: yes
printf("U+1F600 has Emoji_Presentation: %s", emoji.presentation ? "yes" : "no");`,
    related: ['mjb_string_emoji_sequence', 'mjb_codepoint_is_emoji'],
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
        wasm_generated: false
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
        wasm_generated: false
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
    name: 'mjb_string_emoji_sequence',
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

if(mjb_string_emoji_sequence(flag, strlen(flag), MJB_ENC_UTF_8,
    &emoji) != MJB_STATUS_OK) {
    return 1;
}

// Sequence codepoints: 2
printf("Sequence codepoints: %zu", emoji.codepoint_count);`,
    related: ['mjb_string_is_emoji_sequence', 'mjb_string_is_rgi_emoji'],
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the complete string is an emoji sequence listed by Unicode, ' +
      'including standardized emoji variation sequences.',
    ret: 'bool',
    name: 'mjb_string_is_emoji_sequence',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding()
    ],
    section: Section.Emoji,
    wasm: true,
    example: `const char *keycap = "1\\xEF\\xB8\\x8F\\xE2\\x83\\xA3"; // 1️⃣

bool listed = mjb_string_is_emoji_sequence(keycap, strlen(keycap), MJB_ENC_UTF_8);

// Listed emoji sequence: yes
printf("Listed emoji sequence: %s", listed ? "yes" : "no");`,
    related: ['mjb_string_is_rgi_emoji', 'mjb_string_emoji_sequence'],
    specs: [uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Return true if the complete string is an RGI emoji sequence, excluding plain ' +
      'standardized variation sequences.',
    ret: 'bool',
    name: 'mjb_string_is_rgi_emoji',
    attributes: [],
    args: [
      buffer('The string to check'),
      byte_length(),
      encoding()
    ],
    wasm: true,
    section: Section.Emoji,
    example: `const char *flag = "\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9"; // 🇮🇹

bool rgi = mjb_string_is_rgi_emoji(flag, strlen(flag), MJB_ENC_UTF_8);

// RGI emoji: yes
printf("RGI emoji: %s", rgi ? "yes" : "no");`,
    related: ['mjb_string_is_emoji_sequence', 'mjb_string_emoji_sequence'],
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
      byte_length()
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
    section: Section.DisplayWidth,
    example: `mjb_east_asian_width width;

if(mjb_codepoint_east_asian_width(0x754C, &width) != MJB_STATUS_OK) { // 界
    return 1;
}

// U+754C is wide: yes
printf("U+754C is wide: %s", width == MJB_EAW_WIDE ? "yes" : "no");`,
    related: ['mjb_display_width'],
    specs: [uax(11, 'East Asian Width')]
  },
  {
    comment: 'Return the display width of a string.',
    ret: 'mjb_status',
    name: 'mjb_display_width',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The string to normalize'),
      byte_length(),
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
    wasm: true,
    section: Section.DisplayWidth,
    details: 'Compute the number of display columns a string occupies in a terminal, ' +
      'accounting for wide and ambiguous East Asian characters, combining marks, and emoji ' +
      'sequences.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The width was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`width` is NULL, or `buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The width would overflow' }
    ],
    example: `const char *input = "A\\xE7\\x95\\x8C"; // A界
size_t width;

if(mjb_display_width(input, strlen(input), MJB_ENC_UTF_8,
    MJB_WIDTH_CONTEXT_WESTERN, &width) != MJB_STATUS_OK) {
    return 1;
}

// Display columns: 3
printf("Display columns: %zu", width);`,
    related: ['mjb_codepoint_east_asian_width', 'mjb_truncate_width'],
    specs: [uax(11, 'East Asian Width')]
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
        description: 'The length of the locale identifier, in bytes',
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
    wasm: true,
    section: Section.Utility,
    details: 'Parse a BCP 47 language tag, such as `sr-Latn-RS`, into its components: ' +
      'language, extended language, script, region, variant, extensions, private use, and ' +
      'grandfathered tags. Parsing is strict: malformed tags are rejected and `error` is ' +
      'filled with the failure reason.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The tag was parsed and `locale` filled' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: 'An argument is NULL or the tag is not a valid BCP 47 language tag' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    example: `mjb_locale_id locale;
mjb_error error;

if(mjb_locale_parse("sr-Latn-RS", 10, MJB_ENC_UTF_8, &locale,
    &error) != MJB_STATUS_OK) {
    return 1;
}

// Locale: sr Latn RS
printf("Locale: %s %s %s", locale.language, locale.script, locale.region);`,
    related: ['mjb_locale_set'],
    specs: [{ name: 'BCP 47: Tags for Identifying Languages', url: 'https://www.rfc-editor.org/rfc/rfc5646' }]
  },
  {
    comment: 'Set current locale used by locale-sensitive casing.',
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
    wasm: true,
    section: Section.Utility,
    details: 'Set the process-global locale used by `mjb_case`. The default locale is ' +
      '`MJB_LOCALE_EN`, and `mjb_shutdown` resets it to `MJB_LOCALE_EN`. Only ' +
      '`MJB_LOCALE_TR`, `MJB_LOCALE_AZ`, and `MJB_LOCALE_LT` currently tailor casing. Other ' +
      'valid locale values are accepted but do not change Unicode algorithm behavior.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The locale was set' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`locale` is not a valid `mjb_locale` value' }
    ],
    example: `if(mjb_locale_set(MJB_LOCALE_TR) != MJB_STATUS_OK) {
    return 1;
}

// Turkish locale selected: yes
printf("Turkish locale selected: yes");
if(mjb_locale_set(MJB_LOCALE_EN) != MJB_STATUS_OK) {
    return 1;
}`,
    related: ['mjb_case']
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

if(mjb_string_convert_encoding("A", 1, MJB_ENC_UTF_8, MJB_ENC_UTF_16LE,
    &result) != MJB_STATUS_OK || mjb_result_free(&result) != MJB_STATUS_OK) {
    return 1;
}

// Result released: yes
printf("Result released: %s", result.output == NULL ? "yes" : "no");`
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

// Unicode version: 17.0.0
printf("Unicode version: %s", version);`,
    related: ['mjb_version', 'mjb_version_number']
  },
  {
    comment: 'Set the library memory functions.',
    ret: 'mjb_status',
    name: 'mjb_set_memory_functions',
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
    wasm: false,
    section: Section.Utility,
    details: 'Replace the allocator used by the library for all internal allocations and for ' +
      'the buffers returned in `mjb_result`. Must be called before any other library call.',
    example: `mjb_shutdown(); // Ensure no allocator is currently locked in.

if(mjb_set_memory_functions(malloc, realloc, free) != MJB_STATUS_OK) {
    return 1;
}

// Standard allocator installed: yes
printf("Standard allocator installed: yes");
mjb_shutdown();`,
    related: ['mjb_alloc', 'mjb_realloc', 'mjb_free']
  },
  {
    comment: 'Shutdown the library. Not needed to be called.',
    ret: 'void',
    name: 'mjb_shutdown',
    attributes: [],
    args: [],
    wasm: false,
    section: Section.Utility,
    example: `mjb_shutdown();

// Library state reset: yes
printf("Library state reset: yes");`
  },
  {
    comment: 'Allocate memory.',
    ret: 'void *',
    name: 'mjb_alloc',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'byte_length',
        type: 'size_t',
        description: 'The length of the memory to allocate',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.Utility,
    details: 'Allocate memory using the allocator set by `mjb_set_memory_functions`. If no ' +
      'allocator is set, the default allocator is used.',
    example: `char *buffer = (char*)mjb_alloc(sizeof("allocated"));

if(buffer == NULL) {
    return 1;
}

memcpy(buffer, "allocated", sizeof("allocated"));

// Buffer: allocated
printf("Buffer: %s", buffer);
mjb_free(buffer);`,
    related: ['mjb_realloc', 'mjb_free']
  },
  {
    comment: 'Reallocate memory.',
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
    wasm: false,
    section: Section.Utility,
    details: 'Reallocate memory using the allocator set by `mjb_set_memory_functions`. If no ' +
      'allocator is set, the default allocator is used.',
    example: `char *buffer = (char*)mjb_alloc(8);

if(buffer == NULL) {
    return 1;
}

char *larger = (char*)mjb_realloc(buffer, 32);

if(larger == NULL) {
    mjb_free(buffer);
    return 1;
}

// Reallocation succeeded: yes
printf("Reallocation succeeded: yes");
mjb_free(larger);`,
    related: ['mjb_alloc', 'mjb_free']
  },
  {
    comment: 'Free memory.',
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
    wasm: false,
    section: Section.Utility,
    details: 'Free memory using the allocator set by `mjb_set_memory_functions`. If no ' +
      'allocator is set, the default allocator is used.',
    example: `void *memory = mjb_alloc(16);

if(memory == NULL) {
    return 1;
}

mjb_free(memory);

// Memory freed: yes
printf("Memory freed: yes");`,
    related: ['mjb_alloc', 'mjb_realloc']
  }
] as MojibakeFunction[];
