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

export type MojibakeExamples = {
  c?: string;
  cpp?: string;
};

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
  // Examples by language.
  examples?: MojibakeExamples;
  // Preprocessor feature required to compile and run the example.
  exampleFeature?: string;
  // The C example is compiled by the tests but not run, for example because it changes
  // process-global state the tests depend on.
  exampleCompileOnly?: boolean;
  // Names of related public functions.
  related?: string[];
  // Unicode specifications implemented or referenced by the function.
  specs?: MojibakeSpecRef[];
};

const unicodeVersion = '18.0.0';

const uaxRevisions: Record<number, number> = {
  9: 52,
  11: 46,
  14: 57,
  15: 58,
  24: 41,
  29: 49,
  31: 45,
  44: 38
};

const utsRevisions: Record<number, number> = {
  10: 55,
  39: 34,
  46: 36,
  51: 31
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

function text(description: string, name = 'text'): MojibakeArg {
  return {
    name,
    type: 'const mjb_text *',
    description,
    wasm_generated: false
  };
}

function outputBuffer(description = 'The output storage, its capacity, and its encoding'):
  MojibakeArg {
  return {
    name: 'output',
    type: 'mjb_buffer *',
    description,
    wasm_generated: false,
    ownership: 'The caller retains ownership of `output->data`'
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

function diagnostic(description = 'Where to store the first malformed-input diagnostic, or NULL',
  name = 'diagnostic'): MojibakeArg {
  return {
    name,
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
    ownership: 'If `result->owned` is true, `result->output` is library-allocated and must be ' +
      'freed with `mjb_result_free(result)`. Otherwise it points into the input, which must ' +
      'outlive the result'
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
    examples: {
      c: `mjb_character character;

// U+0665 ARABIC-INDIC DIGIT FIVE
if(mjb_codepoint_info(0x0665, &character) != MJB_STATUS_OK) {
    return 1;
}

bool is_digit = character.category == MJB_CATEGORY_ND;

// Decimal digit: yes, value: 5
printf("Decimal digit: %s, value: %d", is_digit ? "yes" : "no", character.decimal);`,
      cpp: `// U+0665 ARABIC-INDIC DIGIT FIVE
auto character = mjb::Character::from(0x0665);

if(!character) {
    return 1;
}

bool is_digit = character->category() == MJB_CATEGORY_ND;

// Decimal digit: yes, value: 5
std::cout << "Decimal digit: " << (is_digit ? "yes" : "no") << ", value: "
    << character->decimal_value().value_or(-1) << std::endl;`
    },
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
      text('The text to normalize'),
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
      'already normalized and no encoding conversion is needed, `text->buffer` is returned ' +
      'as-is in `result->output` with `result->owned` set to false, without allocating. ' +
      'Malformed subsequences follow `malformed_policy`, and `diagnostic` records the first one.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The string was normalized (or already normal)' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_INVALID_FORM', description: '`form` is not NFC, NFD, NFKC, or NFKD' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent a normalized codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    examples: {
      c: `// "Cafe" + U+0301 COMBINING ACUTE ACCENT
mjb_text input = mjb_text_utf8("Cafe\\xCC\\x81", MJB_NUL_TERMINATED);
mjb_result result;

if(mjb_normalize(&input, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC, MJB_ENC_UTF_8, &result,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// NFC: Café
printf("NFC: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);`,
      cpp: `// "Cafe" + U+0301 COMBINING ACUTE ACCENT
std::string input = "Cafe\\xCC\\x81";
std::string output = mjb::normalize(input, mjb::NormalizationForm::NFC);

// NFC: Café
std::cout << "NFC: " << output << std::endl;`
    },
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
      text('The text to normalize'),
      malformedPolicy(),
      {
        name: 'form',
        type: 'mjb_normalization',
        description: 'The normalization form to use',
        wasm_generated: false,
        is_enum: true
      },
      outputBuffer(),
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Normalize a string using the same Unicode normalization forms and encoding rules ' +
      'as `mjb_normalize`. `output->encoding` selects the output encoding. Set `output->data` ' +
      'to NULL to query the required size. If `output->data` is non-NULL, `output->size` ' +
      'supplies its capacity; on return it contains the required size when the buffer is too ' +
      'small, or the written size on success. Terminators are excluded ' +
      'from the byte count and are not written. If capacity is insufficient, or another error ' +
      'occurs, the output buffer may hold a partial prefix of the result, but nothing is ' +
      'written past its capacity. NFD and NFKD write without allocation. NFC and NFKC may ' +
      'allocate temporary ' +
      'composition storage, including during a size query.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the normalized string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
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
    examples: {
      c: `const char *cafe = "Cafe\\xCC\\x81"; // "Cafe" + U+0301 COMBINING ACUTE ACCENT
mjb_text input = mjb_text_utf8(cafe, strlen(cafe));
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_8);

// Query the required size
if(mjb_normalize_into(&input, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC, &output,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

char storage[5];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);

if(mjb_normalize_into(&input, MJB_MALFORMED_STOP, MJB_NORMALIZATION_NFC, &output,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// NFC payload (no terminator): Café
printf("NFC payload (no terminator): %.*s", (int)output.size, storage);`,
      cpp: `// "Cafe" + U+0301 COMBINING ACUTE ACCENT
std::string input = "Cafe\\xCC\\x81";
std::string output;

// Replaces the content of output and reuses its capacity on later calls
mjb::normalize(input, mjb::NormalizationForm::NFC, output);

// NFC: Café
std::cout << "NFC: " << output << std::endl;`
    },
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
      text('The text to filter'),
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
        '`result` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
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
    examples: {
      c: `mjb_text mixed_whitespace = mjb_text_utf8("Hello\\t\\t\\n\\nworld", MJB_NUL_TERMINATED);
mjb_result result;

if(mjb_filter(&mixed_whitespace, MJB_MALFORMED_STOP, MJB_FILTER_COLLAPSE_SPACES, MJB_ENC_UTF_8,
    &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: Hello world
printf("Filtered: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);

mjb_text controls = mjb_text_utf8("\\x1\\x2\\t\\n\\v\\f\\r\\x1f", MJB_NUL_TERMINATED);

if(mjb_filter(&controls, MJB_MALFORMED_STOP, MJB_FILTER_CONTROLS, MJB_ENC_UTF_8, &result,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Filtered: \\t\\n\\v\\f\\r
printf("Filtered: %.*s", (int)result.output_size, result.output);

mjb_result_free(&result);`,
      cpp: `std::string mixed_whitespace = "Hello\\t\\t\\n\\nworld";

// Filtered: Hello world
std::cout << "Filtered: " << mjb::filter(mixed_whitespace, mjb::Filter::CollapseSpaces)
    << std::endl;

std::string controls = "\\x1\\x2\\t\\n\\v\\f\\r\\x1f";

// Filtered: \\t\\n\\v\\f\\r
std::cout << "Filtered: " << mjb::filter(controls, mjb::Filter::Controls) << std::endl;`
    },
    related: ['mjb_filter_into', 'mjb_normalize']
  },
  {
    comment: 'Filter a string into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_filter_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to filter'),
      malformedPolicy(),
      {
        name: 'filters',
        type: 'mjb_filter_flags',
        description: 'The filters to use',
        wasm_generated: false
      },
      outputBuffer(),
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same filters as `mjb_filter` without allocating the final output buffer. ' +
      '`output->encoding` selects the output encoding. Set `output->data` to NULL to query the ' +
      'required size. If `output->data` is non-NULL, `output->size` supplies its capacity; on ' +
      'return it contains the required size when the buffer is too small, or the written size ' +
      'on success. Terminators are excluded from the byte count and ' +
      'are not written. If capacity is insufficient, or another error occurs, the output ' +
      'buffer may hold a partial prefix of the result, but nothing is written past its ' +
      'capacity. Filtering itself ' +
      'does not allocate, but `MJB_FILTER_NORMALIZE` may allocate temporary normalization ' +
      'storage. `MJB_FILTER_LIMIT_COMBINING` keeps the first ' +
      '`MJB_FILTER_MAX_COMBINING_MARKS` consecutive marks in each emitted run. Malformed ' +
      'subsequences follow `malformed_policy`, and `diagnostic` records the first one.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the filtered string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("Hello\\t\\t\\nworld", MJB_NUL_TERMINATED);
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_8);

// Query the required size
if(mjb_filter_into(&input, MJB_MALFORMED_STOP, MJB_FILTER_COLLAPSE_SPACES, &output,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

char storage[11];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);

if(mjb_filter_into(&input, MJB_MALFORMED_STOP, MJB_FILTER_COLLAPSE_SPACES, &output,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Filtered payload (no terminator): Hello world
printf("Filtered payload (no terminator): %.*s", (int)output.size, storage);`,
      cpp: `std::string input = "Hello\\t\\t\\nworld";
std::string output;

// Replaces the content of output and reuses its capacity on later calls
mjb::filter(input, mjb::Filter::CollapseSpaces, output);

// Filtered: Hello world
std::cout << "Filtered: " << output << std::endl;`
    },
    related: ['mjb_filter', 'mjb_normalize']
  },
  {
    comment: 'Apply the Unicode NFKC_Casefold transform to a string.',
    ret: 'mjb_status',
    name: 'mjb_nfkc_casefold',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to transform'),
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
        '`result` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The transform did not stabilize, or the requested output encoding cannot represent a ' +
        'result codepoint' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("Stra\\xC3\\x9F" "e\\xC2\\xAD", MJB_NUL_TERMINATED);
mjb_result result;

if(mjb_nfkc_casefold(&input, MJB_MALFORMED_STOP, MJB_ENC_UTF_8, &result,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// strasse
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
      cpp: `// "Straße" + U+00AD SOFT HYPHEN
std::string input = "Stra\\xC3\\x9F" "e\\xC2\\xAD";

// strasse
std::cout << mjb::nfkc_casefold(input) << std::endl;`
    },
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
      text('The text to transform'),
      malformedPolicy(),
      outputBuffer(),
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same normative `NFKC_Casefold` transform as `mjb_nfkc_casefold`. ' +
      '`output->encoding` selects the output encoding. Set `output->data` to NULL to query the ' +
      'required size. If `output->data` is non-NULL, `output->size` supplies its capacity; on ' +
      'return it contains the required size when the buffer is too small, or the written size ' +
      'on success. Terminators are excluded from the byte count and ' +
      'are not written. If capacity is insufficient, or another error occurs, the output ' +
      'buffer may hold a partial prefix of the result, but nothing is written past its ' +
      'capacity. The final output ' +
      'uses caller-provided storage, but the normalization and folding passes require temporary ' +
      'allocations, including during a size query.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the transformed string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The transform did not stabilize, or the requested output encoding cannot represent a ' +
        'result codepoint' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Temporary allocation failed' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("Stra\\xC3\\x9F" "e\\xC2\\xAD", MJB_NUL_TERMINATED);
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_8);

// Query the required size
if(mjb_nfkc_casefold_into(&input, MJB_MALFORMED_STOP, &output, NULL) != MJB_STATUS_OK) {
    return 1;
}

char storage[7];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);

if(mjb_nfkc_casefold_into(&input, MJB_MALFORMED_STOP, &output, NULL) != MJB_STATUS_OK) {
    return 1;
}

// NFKC casefold payload (no terminator): strasse
printf("NFKC casefold payload (no terminator): %.*s", (int)output.size, storage);`,
      cpp: `// "Straße" + U+00AD SOFT HYPHEN
std::string input = "Stra\\xC3\\x9F" "e\\xC2\\xAD";
std::string output;

// Replaces the content of output and reuses its capacity on later calls
mjb::nfkc_casefold(input, output);

// NFKC casefold: strasse
std::cout << "NFKC casefold: " << output << std::endl;`
    },
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
      text('The domain name to process'),
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
        '`text`, `info`, or `result` is NULL, or `text->buffer` is NULL with a non-zero size' },
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
    examples: {
      c: `// "bücher.example"
mjb_text domain = mjb_text_utf8("b\\xC3\\xBC" "cher.example", MJB_NUL_TERMINATED);
mjb_idna_info info;
mjb_result result;

if(mjb_idna_to_ascii(&domain, MJB_ENC_UTF_8, &info, &result) != MJB_STATUS_OK ||
    info.errors != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// xn--bcher-kva.example
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
      cpp: `// "bücher.example"
auto domain = mjb::idna_to_ascii("b\\xC3\\xBC" "cher.example");

if(!domain.valid()) {
    return 1;
}

// xn--bcher-kva.example
std::cout << domain.text << std::endl;`
    },
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
      text('The domain name to process'),
      {
        name: 'info',
        type: 'mjb_idna_info *',
        description: 'The UTS #46 validation errors to store',
        wasm_generated: false
      },
      outputBuffer()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same strict nontransitional profile as `mjb_idna_to_ascii`. ' +
      '`output->encoding` selects the output encoding. Set `output->data` to NULL to query the ' +
      'required byte count; otherwise `output->size` supplies its capacity, and on return it ' +
      'contains the required or written byte count. If capacity is insufficient, or another ' +
      'error occurs, the output buffer may hold a partial prefix of the result, but nothing is ' +
      'written past its capacity. Processing uses temporary allocations even during a size ' +
      'query. If ' +
      '`MJB_FEATURE_IDNA=0` the function always returns `MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the ASCII domain was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`text`, `info`, or `output` is NULL, or `text->buffer` is NULL with a non-zero size' },
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
    exampleFeature: 'MJB_FEATURE_IDNA',
    examples: {
      c: `// "bücher.example"
mjb_text domain = mjb_text_utf8("b\\xC3\\xBC" "cher.example", MJB_NUL_TERMINATED);
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_8);
mjb_idna_info info;

// Query the required size
if(mjb_idna_to_ascii_into(&domain, &info, &output) != MJB_STATUS_OK) {
    return 1;
}

char storage[21];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);

if(mjb_idna_to_ascii_into(&domain, &info, &output) != MJB_STATUS_OK ||
    info.errors != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// ASCII payload (no terminator): xn--bcher-kva.example
printf("ASCII payload (no terminator): %.*s", (int)output.size, storage);`,
      cpp: `// "bücher.example"
std::string domain = "b\\xC3\\xBC" "cher.example";
std::string output;

// Returns the mjb_idna_error flags
if(mjb::idna_to_ascii(domain, output) != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// xn--bcher-kva.example
std::cout << output << std::endl;`
    },
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
      text('The domain name to process'),
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
        '`text`, `info`, or `result` is NULL, or `text->buffer` is NULL with a non-zero size' },
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
    examples: {
      c: `mjb_text domain = mjb_text_utf8("xn--bcher-kva.example", MJB_NUL_TERMINATED);
mjb_idna_info info;
mjb_result result;

if(mjb_idna_to_unicode(&domain, MJB_ENC_UTF_8, &info, &result) != MJB_STATUS_OK ||
    info.errors != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// bücher.example
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`,
      cpp: `auto domain = mjb::idna_to_unicode("xn--bcher-kva.example");

if(!domain.valid()) {
    return 1;
}

// bücher.example
std::cout << domain.text << std::endl;`
    },
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
      text('The domain name to process'),
      {
        name: 'info',
        type: 'mjb_idna_info *',
        description: 'The UTS #46 validation errors to store',
        wasm_generated: false
      },
      outputBuffer()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same strict nontransitional profile as `mjb_idna_to_unicode`. ' +
      '`output->encoding` selects the output encoding. Set `output->data` to NULL to query the ' +
      'required byte count; otherwise `output->size` supplies its capacity, and on return it ' +
      'contains the required or written byte count. If capacity is insufficient, or ' +
      'another error occurs, the output buffer may hold a partial prefix of the result, but ' +
      'nothing is written past its capacity. Processing uses temporary allocations even during ' +
      'a size query. If ' +
      '`MJB_FEATURE_IDNA=0` the function always returns `MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the Unicode domain was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`text`, `info`, or `output` is NULL, or `text->buffer` is NULL with a non-zero size' },
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
    exampleFeature: 'MJB_FEATURE_IDNA',
    examples: {
      c: `mjb_text domain = mjb_text_utf8("xn--bcher-kva.example", MJB_NUL_TERMINATED);
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_8);
mjb_idna_info info;

// Query the required size
if(mjb_idna_to_unicode_into(&domain, &info, &output) != MJB_STATUS_OK) {
    return 1;
}

char storage[15];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);

if(mjb_idna_to_unicode_into(&domain, &info, &output) != MJB_STATUS_OK ||
    info.errors != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// Unicode payload (no terminator): bücher.example
printf("Unicode payload (no terminator): %.*s", (int)output.size, storage);`,
      cpp: `std::string domain = "xn--bcher-kva.example";
std::string output;

// Returns the mjb_idna_error flags
if(mjb::idna_to_unicode(domain, output) != MJB_IDNA_ERROR_NONE) {
    return 1;
}

// bücher.example
std::cout << output << std::endl;`
    },
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
      text('The text to check'),
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
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`quick_check` or `text` is NULL, or `text->buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The text encoding is invalid or a generic UTF-16/UTF-32 encoding has no byte-order information' },
      { value: 'MJB_STATUS_INVALID_FORM', description: '`form` is not NFC, NFD, NFKC, or NFKD' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description: 'The input contains an ill-formed code-unit sequence' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("caf\\xC3\\xA9", MJB_NUL_TERMINATED);
mjb_quick_check_result check;

if(mjb_normalization_quick_check(&input, MJB_NORMALIZATION_NFC, &check) != MJB_STATUS_OK) {
    return 1;
}

// NFC normalized: yes
printf("NFC normalized: %s", check == MJB_QC_YES ? "yes" : "no");`
    },
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
    examples: {
      c: `const char utf16le[] = "\\xFF\\xFEH\\0i\\0";
mjb_encoding detected = mjb_detect_encoding(utf16le, sizeof(utf16le) - 1);
bool is_utf16le = detected == (MJB_ENC_UTF_16 | MJB_ENC_UTF_16LE);

// UTF-16LE detected: yes
printf("UTF-16LE detected: %s", is_utf16le ? "yes" : "no");`
    }
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
    examples: {
      c: `const char *input = "Plain ASCII";

// ASCII: yes
printf("ASCII: %s", mjb_is_ascii(input, strlen(input)) ? "yes" : "no");`,
      cpp: `std::string input = "Plain ASCII";

// ASCII: yes
std::cout << "ASCII: " << (mjb::is_ascii(input) ? "yes" : "no") << std::endl;`
    }
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
    examples: {
      c: `const char *input = "caf\\xC3\\xA9";

// Valid UTF-8: yes
printf("Valid UTF-8: %s", mjb_is_utf8(input, strlen(input)) ? "yes" : "no");`,
      cpp: `std::string input = "caf\\xC3\\xA9";

// Valid UTF-8: yes
std::cout << "Valid UTF-8: " << (mjb::is_utf8(input) ? "yes" : "no") << std::endl;`
    }
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
    examples: {
      c: `const char utf16be[] = "\\xFE\\xFF\\0H\\0i"; // BOM + "Hi" in UTF-16BE

// UTF-16: yes
printf("UTF-16: %s", mjb_is_utf16(utf16be, sizeof(utf16be) - 1) ? "yes" : "no");`,
      cpp: `using namespace std::string_view_literals;

// BOM + "Hi" in UTF-16BE. The sv suffix keeps the embedded NUL bytes.
auto utf16be = "\\xFE\\xFF\\0H\\0i"sv;

// UTF-16: yes
std::cout << "UTF-16: " << (mjb::is_utf16(utf16be) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Validate a complete Unicode code-unit sequence.',
    ret: 'mjb_status',
    name: 'mjb_validate_encoding',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to validate'),
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
        '`text` is NULL, or `text->buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The encoding is unsupported, or generic UTF-16/UTF-32 has no byte-order mark' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The first malformed subsequence is described by `diagnostic`' }
    ],
    examples: {
      c: `const char invalid[] = "\\xE2\\x82";
mjb_text text = mjb_text_utf8(invalid, sizeof(invalid) - 1);
mjb_diagnostic diagnostic;

if(mjb_validate_encoding(&text, &diagnostic) != MJB_STATUS_MALFORMED_INPUT ||
    diagnostic.byte_offset != 0) {
    return 1;
}`,
      cpp: `std::string invalid = "\\xE2\\x82"; // A truncated three-byte sequence
mjb_diagnostic diagnostic{};

if(mjb::validate_encoding(invalid, &diagnostic) != MJB_STATUS_MALFORMED_INPUT) {
    return 1;
}

// Malformed input at byte 0
std::cout << "Malformed input at byte " << diagnostic.byte_offset << std::endl;`
    },
    related: ['mjb_decode_next', 'mjb_decode_previous', 'mjb_is_utf8']
  },
  {
    comment: 'Return the byte length of a string that ends at its first NUL code unit.',
    ret: 'mjb_status',
    name: 'mjb_nul_terminated_length',
    attributes: ['MJB_NODISCARD'],
    args: [
      buffer('The NUL-terminated string'),
      encoding(),
      {
        name: 'byte_length',
        type: 'size_t *',
        description: 'Where to store the byte length, excluding the terminator',
        wasm_generated: false
      }
    ],
    wasm: false,
    section: Section.TextAnalysis,
    details: 'Scan for the terminator of the encoding: a zero byte for ASCII and UTF-8, a zero ' +
      '16-bit code unit for UTF-16, and a zero 32-bit code unit for UTF-32. Code units are ' +
      'aligned to the start of `buffer`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The length was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`buffer` or `byte_length` is NULL' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' }
    ],
    examples: {
      c: `const char utf16le[] = "H\\0i\\0\\0\\0"; // "Hi" in UTF-16LE
size_t byte_length;

if(mjb_nul_terminated_length(utf16le, MJB_ENC_UTF_16LE, &byte_length) != MJB_STATUS_OK) {
    return 1;
}

// Byte length: 4
printf("Byte length: %zu", byte_length);`
    },
    related: ['mjb_decode_next', 'mjb_decode_previous']
  },
  {
    comment: 'Decode the next codepoint from a string.',
    ret: 'mjb_status',
    name: 'mjb_decode_next',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to decode'),
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
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        'An argument or policy is invalid, or the text is `MJB_NUL_TERMINATED`' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding cannot be decoded' }
    ],
    examples: {
      c: `// "été"
const char *word = "\\xC3\\xA9t\\xC3\\xA9";
mjb_text input = mjb_text_utf8(word, strlen(word));
size_t offset = 0;
size_t count = 0;
mjb_codepoint codepoint;
mjb_status status;

while((status = mjb_decode_next(&input, MJB_MALFORMED_STOP, &offset, &codepoint, NULL)) ==
    MJB_STATUS_OK) {
    ++count;
}

if(status != MJB_STATUS_END_OF_INPUT) {
    return 1;
}

// 3 codepoints in 5 bytes
printf("%zu codepoints in %zu bytes", count, offset);`
    },
    related: ['mjb_decode_previous', 'mjb_validate_encoding', 'mjb_codepoint_count']
  },
  {
    comment: 'Decode the previous codepoint from a string.',
    ret: 'mjb_status',
    name: 'mjb_decode_previous',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to decode'),
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
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        'An argument or policy is invalid, or the text is `MJB_NUL_TERMINATED`' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding cannot be decoded' }
    ],
    examples: {
      c: `// "été"
const char *word = "\\xC3\\xA9t\\xC3\\xA9";
mjb_text input = mjb_text_utf8(word, strlen(word));
size_t offset = strlen(word); // Start at the end
mjb_codepoint codepoint;

if(mjb_decode_previous(&input, MJB_MALFORMED_STOP, &offset, &codepoint,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Last codepoint: U+00E9 at byte 3
printf("Last codepoint: U+%04X at byte %zu", (unsigned)codepoint, offset);`
    },
    related: ['mjb_decode_next', 'mjb_validate_encoding', 'mjb_codepoint_count']
  },
  {
    comment: 'Count the codepoints in a string.',
    ret: 'mjb_status',
    name: 'mjb_codepoint_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to count'),
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
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`count` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the malformed ' +
        'policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The text encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    examples: {
      c: `// The "Héllö" string is five Unicode characters, but has different byte lengths in different encodings.

mjb_text utf8 = mjb_text_utf8("H\\xC3\\xA9ll\\xC3\\xB6", 7); // 7 bytes
mjb_text utf16le = mjb_text_utf16le("H\\0\\xE9\\0l\\0l\\0\\xF6\\0", 10); // 10 bytes
size_t count;

if(mjb_codepoint_count(&utf8, MJB_MALFORMED_STOP, &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// 5 UTF-8 characters
printf("%zu UTF-8 characters", count);

if(mjb_codepoint_count(&utf16le, MJB_MALFORMED_STOP, &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// 5 UTF-16LE characters
printf("%zu UTF-16LE characters", count);`
    },
    related: ['mjb_grapheme_count', 'mjb_word_count', 'mjb_sentence_count']
  },
  {
    comment: 'Run a callback for each codepoint of a string.',
    ret: 'mjb_status',
    name: 'mjb_for_each_codepoint',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to check'),
      malformedPolicy(),
      {
        name: 'callback',
        type: 'mjb_for_each_codepoint_fn',
        description: 'The function to call for each codepoint',
        wasm_generated: true
      },
      {
        name: 'context',
        type: 'void *',
        description: 'A caller-owned pointer passed unchanged to every `callback` call, or NULL',
        wasm_generated: true
      },
      diagnostic()
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Decode the string according to `malformed_policy` and call the callback for every ' +
      'resulting codepoint, with its position in the string and `context`. Use `context` to ' +
      'give the callback its own state, such as a counter or an output buffer, without global ' +
      'variables. The first malformed subsequence is reported in `diagnostic` even when it is ' +
      'replaced or skipped.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'Every decoded codepoint was visited' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        'The text, callback, or malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_CALLBACK_STOPPED', description: 'The callback returned false' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("ABC", 3);
mjb_status status = mjb_for_each_codepoint(&input, MJB_MALFORMED_STOP, NULL, NULL, NULL);

// A callback is required: yes
bool callback_required = status == MJB_STATUS_INVALID_ARGUMENT;

// A callback is required: yes
printf("A callback is required: %s", callback_required ? "yes" : "no");`
    }
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
    examples: {
      c: `bool is_alphabetic;

if(mjb_codepoint_property_binary('A', MJB_PR_ALPHABETIC,
    &is_alphabetic) != MJB_STATUS_OK) {
    return 1;
}

// U+0041 is alphabetic: yes
printf("U+0041 is alphabetic: %s", is_alphabetic ? "yes" : "no");`
    },
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
    examples: {
      c: `int32_t script;

if(mjb_codepoint_property_int('A', MJB_PR_SCRIPT, &script) != MJB_STATUS_OK) {
    return 1;
}

// U+0041 uses the Latin script: yes
printf("U+0041 uses the Latin script: %s", script == MJB_SC_LATN ? "yes" : "no");`
    },
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
    examples: {
      c: `mjb_numeric_value num;

if(mjb_codepoint_numeric_value(0x0031, &num) != MJB_STATUS_OK) { // U+0031 = 1
    return 1;
}

// decimal=1, digit=1, numeric=1
printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);

if(mjb_codepoint_numeric_value(0x00BD, &num) != MJB_STATUS_OK) { // U+00BD = '½'
    return 1;
}

// decimal=-1, digit=-1, numeric=1/2
printf("decimal=%d, digit=%d, numeric=%s", num.decimal, num.digit, num.numeric);`
    },
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
    examples: {
      c: `mjb_block_info block;

if(mjb_codepoint_block('A', &block) != MJB_STATUS_OK) {
    return 1;
}

// Block: Basic Latin
printf("Block: %s", block.name);`,
      cpp: `auto block = mjb::block_info('A');

if(!block) {
    return 1;
}

// Block: Basic Latin
std::cout << "Block: " << block->name() << std::endl;`
    },
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
    examples: {
      c: `mjb_script script = mjb_codepoint_script(0x03A9); // Greek capital omega

// Greek script: yes
printf("Greek script: %s", script == MJB_SC_GREK ? "yes" : "no");`,
      cpp: `mjb_script script = mjb::script(0x03A9); // Greek capital omega

// Greek script: yes
std::cout << "Greek script: " << (script == MJB_SC_GREK ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `size_t count = 0;

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
      cpp: `// Throws mjb::LibraryError on failure
std::vector<mjb_script> scripts = mjb::script_extensions(0x30FC);

// U+30FC has 2 Script_Extensions
std::cout << "U+30FC has " << scripts.size() << " Script_Extensions" << std::endl;`
    },
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
    examples: {
      c: `char encoded[4];
unsigned int size = mjb_codepoint_encode(0x20AC, encoded, sizeof(encoded), MJB_ENC_UTF_8);

// € sign uses 3 UTF-8 bytes
printf("%.*s sign uses %u UTF-8 bytes", (int)size, encoded, size);`
    }
  },
  {
    comment: 'Convert from one encoding to another.',
    ret: 'mjb_status',
    name: 'mjb_convert_encoding',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to convert'),
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
        '`result` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent an input codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("caf\\xC3\\xA9", MJB_NUL_TERMINATED);
mjb_result result;

if(mjb_convert_encoding(&input, MJB_MALFORMED_STOP, MJB_ENC_UTF_16LE, &result,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE bytes: 8
printf("UTF-16LE bytes: %zu", result.output_size);
mjb_result_free(&result);`
    },
    related: ['mjb_convert_encoding_into', 'mjb_detect_encoding', 'mjb_codepoint_encode']
  },
  {
    comment: 'Convert from one encoding to another into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_convert_encoding_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to convert'),
      malformedPolicy(),
      outputBuffer(),
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Convert a string using the same encoding and BOM rules as ' +
      '`mjb_convert_encoding`, without allocating memory. `output->encoding` selects the ' +
      'output encoding. Set `output->data` to NULL to query the required size. If ' +
      '`output->data` is non-NULL, `output->size` supplies its capacity; on return it contains ' +
      'the required size when the buffer is too small, or the written size on success. The size is the encoded payload byte count: terminators are excluded, and ' +
      'this function does not write a terminator. If capacity is insufficient, or another ' +
      'error occurs, the output buffer may hold a partial prefix of the result, but nothing is ' +
      'written past its capacity.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the string was converted' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output` or `text` is NULL, `text->buffer` is NULL with a non-zero size, or the ' +
        'malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent an input codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The required output size would overflow' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The output capacity is smaller than the required byte count' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("caf\\xC3\\xA9", MJB_NUL_TERMINATED);
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_16LE);

// Query the required size
if(mjb_convert_encoding_into(&input, MJB_MALFORMED_STOP, &output, NULL) != MJB_STATUS_OK) {
    return 1;
}

unsigned char storage[8];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_16LE);

if(mjb_convert_encoding_into(&input, MJB_MALFORMED_STOP, &output, NULL) != MJB_STATUS_OK) {
    return 1;
}

// UTF-16LE payload bytes (no terminator): 8
printf("UTF-16LE payload bytes (no terminator): %zu", output.size);`
    },
    related: ['mjb_convert_encoding', 'mjb_detect_encoding', 'mjb_codepoint_encode']
  },
  {
    comment: 'Compare two strings using a Unicode caseless matching relation.',
    ret: 'mjb_status',
    name: 'mjb_caseless_match',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The first text to compare', 'first'),
      text('The second text to compare', 'second'),
      malformedPolicy(),
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
      },
      diagnostic('Where to store the first malformed-input diagnostic of `first`, or NULL',
        'first_diagnostic'),
      diagnostic('Where to store the first malformed-input diagnostic of `second`, or NULL',
        'second_diagnostic')
    ],
    wasm: true,
    section: Section.SortingComparison,
    details: 'Compare two strings for case-insensitive equality using Unicode full default case ' +
      'folding. Canonical mode implements D145 and is recommended for ordinary text. ' +
      'Unnormalized mode implements D144 without normalization. Compatibility mode implements ' +
      'the iterated folding and compatibility normalization required by D146. Identifier mode ' +
      'implements D147 by applying NFD before `mjb_nfkc_casefold`; it removes default-ignorable ' +
      'codepoints but does not validate identifier syntax.' +
      ' Malformed subsequences follow `malformed_policy`. `first_diagnostic` and ' +
      '`second_diagnostic` each record the first malformed subsequence of their own text, with ' +
      'offsets into that text; when `first` stops on malformed input, `second` is not examined ' +
      'and `second_diagnostic` stays clear.',
    returns: [
      { value: 'MJB_STATUS_OK', description: '`matches` contains the comparison result' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`matches`, `first`, or `second` is NULL, a text buffer is NULL with a non-zero size, ' +
        '`mode` is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An intermediate size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The identifier case-folding transform did not converge' }
    ],
    examples: {
      c: `mjb_text left = mjb_text_utf8("Stra\\xC3\\x9F" "e", MJB_NUL_TERMINATED);
mjb_text right = mjb_text_utf8("STRASSE", MJB_NUL_TERMINATED);
bool matches;

if(mjb_caseless_match(&left, &right, MJB_MALFORMED_STOP, MJB_CASELESS_CANONICAL, &matches, NULL,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Canonical caseless match: yes
printf("Canonical caseless match: %s", matches ? "yes" : "no");`
    },
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
      text('The first text to compare', 'first'),
      text('The second text to compare', 'second'),
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
        description: 'The maximum collation level to compare',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'order',
        type: 'int *',
        description: 'The strcmp-style comparison result to store',
        wasm_generated: true
      },
      diagnostic('Where to store the first malformed-input diagnostic of `first`, or NULL',
        'first_diagnostic'),
      diagnostic('Where to store the first malformed-input diagnostic of `second`, or NULL',
        'second_diagnostic')
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
      'case-insensitive equality is the intended operation. Malformed subsequences follow ' +
      '`malformed_policy`. `first_diagnostic` and `second_diagnostic` each record the first ' +
      'malformed subsequence of their own text, with offsets into that text; when `first` stops ' +
      'on malformed input, `second` is not read and `second_diagnostic` stays clear. See ' +
      '[Unicode collation](#unicode-collation) for the comparison process and configuration ' +
      'guidance. If `MJB_FEATURE_COLLATION=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: '`order` is negative, zero, or positive according to the collation order' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`order`, `first`, or `second` is NULL, a text buffer is NULL with a non-zero size, the ' +
        'malformed policy is invalid, or an option is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An intermediate size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_COLLATION=0`' }
    ],
    examples: {
      c: `mjb_text apple = mjb_text_utf8("apple", 5);
mjb_text banana = mjb_text_utf8("banana", 6);
int order;

if(mjb_collation_compare(&apple, &banana, MJB_MALFORMED_STOP, MJB_COLLATION_NON_IGNORABLE,
    MJB_COLLATION_TERTIARY, &order, NULL, NULL) != MJB_STATUS_OK) {
    return 1;
}

// apple sorts before banana: yes
printf("apple sorts before banana: %s", order < 0 ? "yes" : "no");`
    },
    related: ['mjb_collation_key', 'mjb_collation_key_into', 'mjb_caseless_match'],
    specs: [uts(10, 'Unicode Collation Algorithm')]
  },
  {
    comment: 'Generate a UCA sort key for a string.',
    ret: 'mjb_status',
    name: 'mjb_collation_key',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to generate the sort key for'),
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
          '`result` or `text` is NULL, `text->buffer` is NULL with a non-zero size, the ' +
          'malformed policy is invalid, or a collation option is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The sort key size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_COLLATION=0`' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("r\\xC3\\xA9sum\\xC3\\xA9", 8);
mjb_result key;

if(mjb_collation_key(&input, MJB_MALFORMED_STOP, MJB_COLLATION_NON_IGNORABLE,
    MJB_COLLATION_TERTIARY, &key, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Sort key is non-empty: yes
printf("Sort key is non-empty: %s", key.output_size > 0 ? "yes" : "no");
mjb_result_free(&key);`
    },
    related: ['mjb_collation_key_into', 'mjb_collation_compare'],
    specs: [uts(10, 'Unicode Collation Algorithm')]
  },
  {
    comment: 'Generate a binary collation key into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_collation_key_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to generate the sort key for'),
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
      outputBuffer('The output storage and its capacity. The key is binary: `output->encoding` ' +
        'is ignored'),
      diagnostic()
    ],
    wasm: false,
    section: Section.SortingComparison,
    exampleFeature: 'MJB_FEATURE_COLLATION',
    details: 'Generate the same binary sort key as `mjb_collation_key` without allocating the ' +
      'final key buffer. Set `output->data` to NULL to query the required byte count. If ' +
      '`output->data` is non-NULL, `output->size` supplies its capacity; on return it contains ' +
      'the required size when the buffer is too small, or the written size on success. A ' +
      'collation key is binary: `output->encoding` is ignored, and no terminator is included ' +
      'or written. If capacity is insufficient, or another error ' +
      'occurs, the output buffer may hold a partial prefix of the key, but nothing is written ' +
      'past its capacity. Collation processing still uses temporary allocations, including ' +
      'during a ' +
      'size query. Empty input and non-empty input with no effective weights at the selected ' +
      'strength both require zero bytes. If `MJB_FEATURE_COLLATION=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the binary sort key was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output` or `text` is NULL, `text->buffer` is NULL with a non-zero size, the malformed ' +
        'policy is invalid, or a collation option is invalid' },
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("r\\xC3\\xA9sum\\xC3\\xA9", 8);
// A collation key is binary, so the buffer encoding is ignored
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UNKNOWN);

// Query the required size
if(mjb_collation_key_into(&input, MJB_MALFORMED_STOP, MJB_COLLATION_NON_IGNORABLE,
    MJB_COLLATION_TERTIARY, &output, NULL) != MJB_STATUS_OK) {
    return 1;
}

unsigned char storage[64];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UNKNOWN);

if(mjb_collation_key_into(&input, MJB_MALFORMED_STOP, MJB_COLLATION_NON_IGNORABLE,
    MJB_COLLATION_TERTIARY, &output, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Sort key is non-empty: yes
printf("Sort key is non-empty: %s", output.size > 0 ? "yes" : "no");`
    },
    related: ['mjb_collation_key', 'mjb_collation_compare'],
    specs: [uts(10, 'Unicode Collation Algorithm')]
  },
  {
    comment: 'Change string case.',
    ret: 'mjb_status',
    name: 'mjb_map_case',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to change case'),
      malformedPolicy(),
      {
        name: 'type',
        type: 'mjb_map_case_type',
        description: 'The type of case change',
        wasm_generated: false,
        is_enum: true
      },
      {
        name: 'locale',
        type: 'mjb_locale',
        description: 'The language whose casing rules apply, or `MJB_LOCALE_NOT_SET` for the ' +
          'default mappings',
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
      'subsequent characters in that segment are lowercased. `locale` selects the ' +
      'language-sensitive rules of SpecialCasing.txt for this call only; there is no global ' +
      'locale, so concurrent calls with different locales do not interfere. `MJB_LOCALE_TR` and ' +
      '`MJB_LOCALE_AZ` apply Turkish/Azerbaijani dotted-I casing and Turkic `T` case-folding ' +
      'mappings. `MJB_LOCALE_LT` applies Lithuanian dot-above casing rules, while case folding ' +
      'remains the default non-Turkic mapping. `MJB_LOCALE_NOT_SET` and every other valid ' +
      'locale use the default mappings. Malformed subsequences follow `malformed_policy`, and ' +
      '`diagnostic` records the first one.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The case conversion succeeded' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` or `text` is NULL, `text->buffer` is NULL with a non-zero size, the malformed ' +
        'policy is invalid, `type` is not a valid case type, or `locale` is not a valid ' +
        '`mjb_locale` value' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'An encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The requested output encoding cannot represent a mapped codepoint' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("Stra\\xC3\\x9F""e", MJB_NUL_TERMINATED); // "Straße"
mjb_result result;

if(mjb_map_case(&input, MJB_MALFORMED_STOP, MJB_CASE_UPPER, MJB_LOCALE_NOT_SET, MJB_ENC_UTF_8,
    &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Upper: STRASSE
printf("Upper: %.*s", (int)result.output_size, result.output);
mjb_result_free(&result);

mjb_text istanbul = mjb_text_utf8("istanbul", MJB_NUL_TERMINATED);

if(mjb_map_case(&istanbul, MJB_MALFORMED_STOP, MJB_CASE_UPPER, MJB_LOCALE_TR, MJB_ENC_UTF_8,
    &result, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Turkish upper: \\xC4\\xB0STANBUL
printf("Turkish upper: %.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`
    },
    related: ['mjb_map_case_into', 'mjb_locale_parse', 'mjb_caseless_match'],
    specs: [unicodeCore('Section 3.13', 'Default Case Algorithms', 'G33992')]
  },
  {
    comment: 'Change string case into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_map_case_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to change case'),
      malformedPolicy(),
      {
        name: 'type',
        type: 'mjb_map_case_type',
        description: 'The type of case change',
        wasm_generated: false
      },
      {
        name: 'locale',
        type: 'mjb_locale',
        description: 'The language whose casing rules apply, or `MJB_LOCALE_NOT_SET` for the ' +
          'default mappings',
        wasm_generated: false,
        is_enum: true
      },
      outputBuffer(),
      diagnostic()
    ],
    wasm: false,
    section: Section.TextTransformation,
    details: 'Apply the same full, special, conditional, titlecase, locale-sensitive, and case ' +
      'folding mappings as `mjb_map_case` without allocating memory. `output->encoding` selects ' +
      'the output encoding. Set `output->data` to NULL to query the required size. If ' +
      '`output->data` is non-NULL, `output->size` supplies its capacity; on return it contains ' +
      'the required size when the buffer is too small, or the written size on success. Terminators are excluded from the byte count and are not written. If ' +
      'capacity is insufficient, or another error occurs, the output buffer may hold a partial ' +
      'prefix of the result, but nothing is written past its capacity.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the case-mapped string was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output` or `text` is NULL, `text->buffer` is NULL with a non-zero size, the malformed ' +
        'policy is invalid, `type` is invalid, or `locale` is not a valid `mjb_locale` value' },
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("Stra\\xC3\\x9F""e", MJB_NUL_TERMINATED); // "Straße"
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_8);

// Query the required size
if(mjb_map_case_into(&input, MJB_MALFORMED_STOP, MJB_CASE_UPPER, MJB_LOCALE_NOT_SET, &output,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

char storage[7];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);

if(mjb_map_case_into(&input, MJB_MALFORMED_STOP, MJB_CASE_UPPER, MJB_LOCALE_NOT_SET, &output,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Upper payload (no terminator): STRASSE
printf("Upper payload (no terminator): %.*s", (int)output.size, storage);`
    },
    related: ['mjb_map_case', 'mjb_locale_parse'],
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
    examples: {
      c: `// U+10FFFD valid: yes
printf("U+10FFFD valid: %s", mjb_codepoint_is_valid(0x10FFFD) ? "yes" : "no");`,
      cpp: `// U+10FFFD valid: yes
std::cout << "U+10FFFD valid: " << (mjb::is_valid(0x10FFFD) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return true if the codepoint is graphic.',
    ret: 'bool',
    name: 'mjb_codepoint_is_graphic',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    examples: {
      c: `// Letter A is graphic: yes
printf("Letter A is graphic: %s", mjb_codepoint_is_graphic('A') ? "yes" : "no");`,
      cpp: `// Letter A is graphic: yes
std::cout << "Letter A is graphic: " << (mjb::is_graphic('A') ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return true if the codepoint is combining.',
    ret: 'bool',
    name: 'mjb_codepoint_is_combining',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    examples: {
      c: `// U+0301 is combining: yes
printf("U+0301 is combining: %s", mjb_codepoint_is_combining(0x0301) ? "yes" : "no");`,
      cpp: `// U+0301 is combining: yes
std::cout << "U+0301 is combining: " << (mjb::is_combining(0x0301) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return if the codepoint is a hangul L.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_leading_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    examples: {
      c: `// U+1100 is a leading Jamo: yes
printf("U+1100 is a leading Jamo: %s", mjb_codepoint_is_hangul_leading_jamo(0x1100) ? "yes" : "no");`,
      cpp: `// U+1100 is a leading Jamo: yes
std::cout << "U+1100 is a leading Jamo: "
    << (mjb::is_hangul_leading_jamo(0x1100) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return if the codepoint is a hangul V.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_vowel_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    examples: {
      c: `// U+1161 is a vowel Jamo: yes
printf("U+1161 is a vowel Jamo: %s", mjb_codepoint_is_hangul_vowel_jamo(0x1161) ? "yes" : "no");`,
      cpp: `// U+1161 is a vowel Jamo: yes
std::cout << "U+1161 is a vowel Jamo: "
    << (mjb::is_hangul_vowel_jamo(0x1161) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return if the codepoint is a hangul T.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_trailing_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    examples: {
      c: `// U+11A8 is a trailing Jamo: yes
printf("U+11A8 is a trailing Jamo: %s", mjb_codepoint_is_hangul_trailing_jamo(0x11A8) ? "yes" : "no");`,
      cpp: `// U+11A8 is a trailing Jamo: yes
std::cout << "U+11A8 is a trailing Jamo: "
    << (mjb::is_hangul_trailing_jamo(0x11A8) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return if the codepoint is a hangul jamo.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_jamo',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    examples: {
      c: `// U+1100 is Hangul Jamo: yes
printf("U+1100 is Hangul Jamo: %s", mjb_codepoint_is_hangul_jamo(0x1100) ? "yes" : "no");`,
      cpp: `// U+1100 is Hangul Jamo: yes
std::cout << "U+1100 is Hangul Jamo: " << (mjb::is_hangul_jamo(0x1100) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return if the codepoint is a hangul syllable.',
    ret: 'bool',
    name: 'mjb_codepoint_is_hangul_syllable',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.HangulLanguage,
    examples: {
      c: `// U+AC00 is a Hangul syllable: yes
printf("U+AC00 is a Hangul syllable: %s", mjb_codepoint_is_hangul_syllable(0xAC00) ? "yes" : "no");`,
      cpp: `// U+AC00 is a Hangul syllable: yes
std::cout << "U+AC00 is a Hangul syllable: "
    << (mjb::is_hangul_syllable(0xAC00) ? "yes" : "no") << std::endl;`
    }
  },
  {
    comment: 'Return if the codepoint is CJK ideograph.',
    ret: 'bool',
    name: 'mjb_codepoint_is_cjk_ideograph',
    attributes: ['MJB_CONST'],
    args: [codepoint()],
    wasm: true,
    section: Section.TextAnalysis,
    examples: {
      c: `// U+4E00 is a CJK ideograph: yes
printf("U+4E00 is a CJK ideograph: %s", mjb_codepoint_is_cjk_ideograph(0x4E00) ? "yes" : "no");`,
      cpp: `// U+4E00 is a CJK ideograph: yes
std::cout << "U+4E00 is a CJK ideograph: "
    << (mjb::is_cjk_ideograph(0x4E00) ? "yes" : "no") << std::endl;`
    }
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
    examples: {
      c: `// U+20000 is a CJK extension ideograph: yes
printf("U+20000 is a CJK extension ideograph: %s", mjb_codepoint_is_cjk_extension_ideograph(0x20000) ? "yes" : "no");`,
      cpp: `// U+20000 is a CJK extension ideograph: yes
std::cout << "U+20000 is a CJK extension ideograph: "
    << (mjb::is_cjk_extension_ideograph(0x20000) ? "yes" : "no") << std::endl;`
    }
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
    examples: {
      c: `// Uppercase letters are graphic: yes
bool graphic = mjb_category_is_graphic(MJB_CATEGORY_LU);

// Uppercase letters are graphic: yes
printf("Uppercase letters are graphic: %s", graphic ? "yes" : "no");`
    }
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
    examples: {
      c: `// Nonspacing marks are combining: yes
bool combining = mjb_category_is_combining(MJB_CATEGORY_MN);

// Nonspacing marks are combining: yes
printf("Nonspacing marks are combining: %s", combining ? "yes" : "no");`
    }
  },
  {
    comment: 'Unicode line break algorithm.',
    ret: 'mjb_break_type',
    name: 'mjb_line_break_next',
    attributes: [],
    args: [
      text('The text to check'),
      {
        name: 'state',
        type: 'mjb_line_break_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Iterate the line break opportunities of a string. Start with `state.index` set to 0 and call repeatedly with the same ' +
      'state until it returns `MJB_BT_NOT_SET`. Each call returns the break type of one ' +
      'position, from the boundary after the first codepoint to the end of the text, and stores ' +
      'its byte offset in `state.position`, so each segment runs from one break position to the ' +
      'next. A malformed subsequence counts as one position, like U+FFFD, but an incomplete ' +
      'subsequence at the end of the text ends the iteration at the position before it.',
    examples: {
      c: `mjb_text input = mjb_text_utf8("Hello world", 11);
mjb_line_break_state state;
state.index = 0;
mjb_break_type type = mjb_line_break_next(&input, &state);

// First line-break result is set: yes
printf("First line-break result is set: %s", type != MJB_BT_NOT_SET ? "yes" : "no");`
    },
    related: ['mjb_grapheme_break_next', 'mjb_word_break_next', 'mjb_sentence_break_next'],
    specs: [uax(14, 'Unicode Line Breaking Algorithm')]
  },
  {
    comment: 'Word cluster breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_word_break_next',
    attributes: [],
    args: [
      text('The text to check'),
      {
        name: 'state',
        type: 'mjb_word_break_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Iterate the word boundaries of a string. Start with `state.index` set to 0 and call repeatedly with the same ' +
      'state until it returns `MJB_BT_NOT_SET`. Each call returns the break type of one ' +
      'position, from the boundary after the first codepoint to the end of the text, and stores ' +
      'its byte offset in `state.position`, so each segment runs from one break position to the ' +
      'next. A malformed subsequence counts as one position, like U+FFFD, but an incomplete ' +
      'subsequence at the end of the text ends the iteration at the position before it.',
    examples: {
      c: `mjb_text input = mjb_text_utf8("Hello world", 11);
mjb_word_break_state state;
state.index = 0;
size_t boundaries = 0;

while(mjb_word_break_next(&input, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Word-break positions: 11
printf("Word-break positions: %zu", boundaries);`
    },
    related: ['mjb_grapheme_break_next', 'mjb_sentence_break_next', 'mjb_word_prefix_length'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Sentence boundaries breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_sentence_break_next',
    attributes: [],
    args: [
      text('The text to check'),
      {
        name: 'state',
        type: 'mjb_sentence_break_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Iterate the sentence boundaries of a string. Start with `state.index` set to 0 and call repeatedly with the same ' +
      'state until it returns `MJB_BT_NOT_SET`. Each call returns the break type of one ' +
      'position, from the boundary after the first codepoint to the end of the text, and stores ' +
      'its byte offset in `state.position`, so each segment runs from one break position to the ' +
      'next. A malformed subsequence counts as one position, like U+FFFD, but an incomplete ' +
      'subsequence at the end of the text ends the iteration at the position before it.',
    examples: {
      c: `mjb_sentence_break_state state;
state.index = 0;
size_t boundaries = 0;
mjb_text input = mjb_text_utf8("Hello. Goodbye.", 15);

while(mjb_sentence_break_next(&input, &state) != MJB_BT_NOT_SET) {
    ++boundaries;
}

// Sentence-break positions: 15
printf("Sentence-break positions: %zu", boundaries);`
    },
    related: ['mjb_grapheme_break_next', 'mjb_word_break_next'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Count the sentence segments in a string.',
    ret: 'mjb_status',
    name: 'mjb_sentence_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to count'),
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
        '`count` is NULL, the text is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("Hello. How are you? Fine!", MJB_NUL_TERMINATED);
size_t count;

if(mjb_sentence_count(&input, MJB_MALFORMED_STOP, &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Sentences: 3
printf("Sentences: %zu", count);`
    },
    related: ['mjb_sentence_break_next', 'mjb_grapheme_count', 'mjb_word_count'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Grapheme cluster breaking.',
    ret: 'mjb_break_type',
    name: 'mjb_grapheme_break_next',
    attributes: [],
    args: [
      text('The text to check'),
      {
        name: 'state',
        type: 'mjb_grapheme_break_state *',
        description: 'The state to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    details: 'Iterate the grapheme cluster (user-perceived character) boundaries of a string. Start with `state.index` set to 0 and call repeatedly with the same ' +
      'state until it returns `MJB_BT_NOT_SET`. Each call returns the break type of one ' +
      'position, from the boundary after the first codepoint to the end of the text, and stores ' +
      'its byte offset in `state.position`, so each segment runs from one break position to the ' +
      'next. A malformed subsequence counts as one position, like U+FFFD, but an incomplete ' +
      'subsequence at the end of the text ends the iteration at the position before it.',
    examples: {
      c: `mjb_text input = mjb_text_utf8("e\\xCC\\x81", 3); // e + combining acute accent
mjb_grapheme_break_state state;
state.index = 0;
size_t codepoints = 0;

while(mjb_grapheme_break_next(&input, &state) != MJB_BT_NOT_SET) {
    ++codepoints;
}

// Codepoints examined: 2
printf("Codepoints examined: %zu", codepoints);`
    },
    related: ['mjb_word_break_next', 'mjb_sentence_break_next', 'mjb_line_break_next', 'mjb_grapheme_prefix_length'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return the number of bytes that form the first `max_graphemes` grapheme cluster segments.',
    ret: 'size_t',
    name: 'mjb_grapheme_prefix_length',
    attributes: [],
    args: [
      text('The text to check'),
      {
        name: 'max_graphemes',
        type: 'size_t',
        description: 'The maximum number of graphemes to return',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    examples: {
      c: `mjb_text input = mjb_text_utf8("A\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9Z", MJB_NUL_TERMINATED); // A🇮🇹Z
size_t bytes = mjb_grapheme_prefix_length(&input, 2);

// First two graphemes use 9 bytes
printf("First two graphemes use %zu bytes", bytes);`
    }
  },
  {
    comment: 'Count the extended grapheme clusters in a string.',
    ret: 'mjb_status',
    name: 'mjb_grapheme_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to count'),
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
        '`count` is NULL, the text is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("A\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9", MJB_NUL_TERMINATED); // A🇮🇹
size_t count;

if(mjb_grapheme_count(&input, MJB_MALFORMED_STOP, &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Grapheme clusters: 2
printf("Grapheme clusters: %zu", count);`
    },
    related: ['mjb_grapheme_prefix_length', 'mjb_grapheme_break_next', 'mjb_terminal_width'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return the number of bytes whose grapheme clusters fit within max_columns terminal cells.',
    ret: 'size_t',
    name: 'mjb_grapheme_prefix_length_for_width',
    attributes: [],
    args: [
      text('The text to check'),
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("A\\xE7\\x95\\x8C", MJB_NUL_TERMINATED); // A界
size_t bytes = mjb_grapheme_prefix_length_for_width(&input, MJB_TERMINAL_WIDTH_NARROW, 2);

// Two columns include 1 byte
printf("Two columns include %zu byte", bytes);`
    }
  },
  {
    comment: 'Return the number of bytes that form the first max_segments word-break segments.',
    ret: 'size_t',
    name: 'mjb_word_prefix_length',
    attributes: [],
    args: [
      text('The text to check'),
      {
        name: 'max_segments',
        type: 'size_t',
        description: 'The maximum number of segments to return',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Segmentation,
    examples: {
      c: `mjb_text input = mjb_text_utf8("Hello world", MJB_NUL_TERMINATED);
size_t bytes = mjb_word_prefix_length(&input, 1);

// First word segment uses 5 bytes
printf("First word segment uses %zu bytes", bytes);`
    }
  },
  {
    comment: 'Count the word-like segments in a string.',
    ret: 'mjb_status',
    name: 'mjb_word_count',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to count'),
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
      'matching the default Unicode word-boundary rules. Unlike `mjb_word_prefix_length`, whose ' +
      '`max_segments` counts every raw segment, this function skips non-word segments. For ' +
      'scripts segmented by dictionary lookup in other implementations, such as Chinese, ' +
      'Japanese, Thai, Lao, Khmer, and Burmese, the count approximates one word per character: ' +
      'Mojibake does not use frequency dictionaries to keep the size of the library small. ' +
      'Malformed subsequences follow `malformed_policy`, and `diagnostic` records the first one. ' +
      'On failure, `count` is set to zero.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The count was computed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`count` is NULL, the text is invalid, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is not a supported input encoding' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("Hello, world! It works.", MJB_NUL_TERMINATED);
size_t count;

if(mjb_word_count(&input, MJB_MALFORMED_STOP, &count, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Words: 4
printf("Words: %zu", count);`
    },
    related: ['mjb_word_break_next', 'mjb_word_prefix_length', 'mjb_grapheme_count', 'mjb_sentence_count'],
    specs: [uax(29, 'Unicode Text Segmentation')]
  },
  {
    comment: 'Return the number of bytes whose word-break segments fit within max_columns terminal cells.',
    ret: 'size_t',
    name: 'mjb_word_prefix_length_for_width',
    attributes: [],
    args: [
      text('The text to check'),
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("Hello world", MJB_NUL_TERMINATED);
size_t bytes = mjb_word_prefix_length_for_width(&input, MJB_TERMINAL_WIDTH_NARROW, 6);

// Six columns include 6 bytes
printf("Six columns include %zu bytes", bytes);`
    }
  },
  {
    comment: 'Resolve bidirectional text (TR9) for a paragraph.',
    ret: 'mjb_status',
    name: 'mjb_bidi_resolve',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to resolve'),
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
        description: '`result` or `text` is NULL, or `text->buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The paragraph size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("abc \\xD7\\x90\\xD7\\x91\\xD7\\x92", MJB_NUL_TERMINATED); // abc אבג
mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve(&input, MJB_DIRECTION_AUTO, &paragraph) != MJB_STATUS_OK) {
    return 1;
}

// Paragraph codepoints: 7
printf("Paragraph codepoints: %zu", paragraph.count);
mjb_bidi_paragraph_free(&paragraph);`
    },
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("\\xD7\\x90\\xD7\\x91\\xD7\\x92", MJB_NUL_TERMINATED); // אבג
mjb_bidi_paragraph paragraph;
size_t visual_order[3];

if(mjb_bidi_resolve(&input, MJB_DIRECTION_AUTO, &paragraph) != MJB_STATUS_OK ||
    mjb_bidi_reorder_line(&paragraph, 0, paragraph.count,
        visual_order) != MJB_STATUS_OK) {
    return 1;
}

// First visual index: 2
printf("First visual index: %zu", visual_order[0]);
mjb_bidi_paragraph_free(&paragraph);`
    },
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("abc", 3);
mjb_bidi_paragraph paragraph;
size_t visual_order[3];
size_t run_count = 0;

if(mjb_bidi_resolve(&input, MJB_DIRECTION_LTR, &paragraph) != MJB_STATUS_OK ||
    mjb_bidi_reorder_line(&paragraph, 0, 3, visual_order) != MJB_STATUS_OK ||
    mjb_bidi_line_runs(&paragraph, visual_order, 3, NULL,
        &run_count) != MJB_STATUS_OK) {
    return 1;
}

// Visual runs: 1
printf("Visual runs: %zu", run_count);
mjb_bidi_paragraph_free(&paragraph);`
    },
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
    examples: {
      c: `mjb_text input = mjb_text_utf8("abc", 3);
mjb_bidi_paragraph paragraph;

if(mjb_bidi_resolve(&input, MJB_DIRECTION_LTR, &paragraph) != MJB_STATUS_OK) {
    return 1;
}

mjb_bidi_paragraph_free(&paragraph);

// Paragraph released: yes
printf("Paragraph released: %s", paragraph.chars == NULL ? "yes" : "no");`
    },
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
    examples: {
      c: `// Greek alpha starts an identifier: yes
bool starts = mjb_codepoint_is_id_start(0x03B1);

// Greek alpha starts an identifier: yes
printf("Greek alpha starts an identifier: %s", starts ? "yes" : "no");`,
      cpp: `// Greek alpha starts an identifier: yes
std::cout << "Greek alpha starts an identifier: "
    << (mjb::is_id_start(0x03B1) ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Digit 7 continues an identifier: yes
bool continues = mjb_codepoint_is_id_continue('7');

// Digit 7 continues an identifier: yes
printf("Digit 7 continues an identifier: %s", continues ? "yes" : "no");`,
      cpp: `// Digit 7 continues an identifier: yes
std::cout << "Digit 7 continues an identifier: "
    << (mjb::is_id_continue('7') ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Letter A is XID_Start: yes
printf("Letter A is XID_Start: %s", mjb_codepoint_is_xid_start('A') ? "yes" : "no");`,
      cpp: `// Letter A is XID_Start: yes
std::cout << "Letter A is XID_Start: " << (mjb::is_xid_start('A') ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Underscore is XID_Continue: yes
bool continues = mjb_codepoint_is_xid_continue('_');

// Underscore is XID_Continue: yes
printf("Underscore is XID_Continue: %s", continues ? "yes" : "no");`,
      cpp: `// Underscore is XID_Continue: yes
std::cout << "Underscore is XID_Continue: "
    << (mjb::is_xid_continue('_') ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Plus sign is Pattern_Syntax: yes
bool syntax = mjb_codepoint_is_pattern_syntax('+');

// Plus sign is Pattern_Syntax: yes
printf("Plus sign is Pattern_Syntax: %s", syntax ? "yes" : "no");`,
      cpp: `// Plus sign is Pattern_Syntax: yes
std::cout << "Plus sign is Pattern_Syntax: "
    << (mjb::is_pattern_syntax('+') ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Space is Pattern_White_Space: yes
bool whitespace = mjb_codepoint_is_pattern_white_space(' ');

// Space is Pattern_White_Space: yes
printf("Space is Pattern_White_Space: %s", whitespace ? "yes" : "no");`,
      cpp: `// Space is Pattern_White_Space: yes
std::cout << "Space is Pattern_White_Space: "
    << (mjb::is_pattern_white_space(' ') ? "yes" : "no") << std::endl;`
    },
    specs: [uax(31, 'Unicode Identifiers and Syntax')]
  },
  {
    comment: 'Return true if the string is a valid Unicode identifier (Unicode 18.0.0 UAX #31).',
    ret: 'bool',
    name: 'mjb_is_identifier',
    attributes: [],
    args: [
      text('The text to validate'),
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
    examples: {
      c: `mjb_text identifier = mjb_text_utf8("delta_2", MJB_NUL_TERMINATED);

bool valid = mjb_is_identifier(&identifier, MJB_IDENTIFIER_NFKC);

// Valid identifier: yes
printf("Valid identifier: %s", valid ? "yes" : "no");`
    },
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
      text('The text to analyze'),
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
        '`count`, `kind`, or `text` is NULL, or `text->buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description:
        'The input encoding is invalid or lacks required byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'The input contains an ill-formed code-unit sequence' },
      { value: 'MJB_STATUS_OUTPUT_TOO_SMALL', description:
        'The script buffer capacity is smaller than the required count' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_SECURITY=0`' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("\\xE3\\x81\\xAD\\xE3\\x82\\xAC", MJB_NUL_TERMINATED); // Hiragana + Katakana
mjb_script scripts[1];
size_t count = 1;
mjb_script_set_kind kind;

if(mjb_resolved_script_set(&input, scripts, &count, &kind) != MJB_STATUS_OK) {
    return 1;
}

bool japanese = kind == MJB_SCRIPT_SET_RESOLVED && count == 1 && scripts[0] == MJB_SC_JPAN;

// Japanese writing system: yes
printf("Japanese writing system: %s", japanese ? "yes" : "no");`
    },
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
    examples: {
      c: `const char *name = mjb_property_name(MJB_PR_ALPHABETIC);

// Property: Alphabetic
printf("Property: %s", name);`
    },
    specs: [uax(44, 'Unicode Character Database')]
  },
  {
    comment: 'Compute a Unicode confusable skeleton (Unicode 18.0.0 UTS #39 Section 4).',
    ret: 'mjb_status',
    name: 'mjb_confusable_skeleton',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to transform'),
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
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`result` or `text` is NULL, or `text->buffer` is NULL with a non-zero size' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An encoding is invalid' },
      { value: 'MJB_STATUS_UNSUPPORTED', description:
        'The output encoding cannot represent a skeleton codepoint' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The output size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_SECURITY=0`' }
    ],
    examples: {
      c: `// Cyrillic U+0435 in place of e
mjb_text input = mjb_text_utf8("h\\xD0\\xB5llo", MJB_NUL_TERMINATED);
mjb_result result;

if(mjb_confusable_skeleton(&input, MJB_ENC_UTF_8, &result) != MJB_STATUS_OK) {
    return 1;
}

// hello
printf("%.*s", (int)result.output_size, result.output);
mjb_result_free(&result);`
    },
    related: ['mjb_confusable_skeleton_into', 'mjb_confusable_match', 'mjb_is_identifier'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Compute a Unicode confusable skeleton into a caller-provided buffer.',
    ret: 'mjb_status',
    name: 'mjb_confusable_skeleton_into',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to transform'),
      outputBuffer()
    ],
    wasm: false,
    section: Section.Security,
    exampleFeature: 'MJB_FEATURE_SECURITY',
    details: 'Compute the same UTS #39 `bidiSkeleton(LTR, input)` as ' +
      '`mjb_confusable_skeleton` without allocating the final output buffer. ' +
      '`output->encoding` selects the output encoding. Set `output->data` to NULL to query the ' +
      'required size. If `output->data` is non-NULL, `output->size` supplies its capacity; on ' +
      'return it contains the required size when the buffer is too small, or the written size ' +
      'on success. Terminators are excluded and are not written. If capacity is ' +
      'insufficient, or another error occurs, the output buffer may hold a partial prefix of ' +
      'the result, but nothing is written past its capacity. Bidirectional resolution, ' +
      'normalization, and ' +
      'skeleton mapping still require temporary allocations, including during a size query. If ' +
      '`MJB_FEATURE_SECURITY=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.',
    returns: [
      { value: 'MJB_STATUS_OK', description:
        'The required size was returned or the skeleton was written' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`output` or `text` is NULL, or `text->buffer` is NULL with a non-zero size' },
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
    examples: {
      c: `// Cyrillic U+0435 in place of e
mjb_text input = mjb_text_utf8("h\\xD0\\xB5llo", MJB_NUL_TERMINATED);
mjb_buffer output = mjb_buffer_make(NULL, 0, MJB_ENC_UTF_8);

// Query the required size
if(mjb_confusable_skeleton_into(&input, &output) != MJB_STATUS_OK) {
    return 1;
}

char storage[5];

if(output.size > sizeof(storage)) {
    return 1;
}

output = mjb_buffer_make(storage, sizeof(storage), MJB_ENC_UTF_8);

if(mjb_confusable_skeleton_into(&input, &output) != MJB_STATUS_OK) {
    return 1;
}

// Skeleton payload (no terminator): hello
printf("Skeleton payload (no terminator): %.*s", (int)output.size, storage);`
    },
    related: ['mjb_confusable_skeleton', 'mjb_confusable_match', 'mjb_is_identifier'],
    specs: [uts(39, 'Unicode Security Mechanisms')]
  },
  {
    comment: 'Determine whether two strings are visually confusable (Unicode 18.0.0 UTS #39 Section 4): skeleton(s1) == skeleton(s2).',
    ret: 'mjb_status',
    name: 'mjb_confusable_match',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The first text', 'first'),
      text('The second text', 'second'),
      malformedPolicy(),
      {
        name: 'confusable',
        type: 'bool *',
        description: 'Whether the strings are visually confusable',
        wasm_generated: true
      },
      diagnostic('Where to store the first malformed-input diagnostic of `first`, or NULL',
        'first_diagnostic'),
      diagnostic('Where to store the first malformed-input diagnostic of `second`, or NULL',
        'second_diagnostic')
    ],
    wasm: true,
    section: Section.Security,
    exampleFeature: 'MJB_FEATURE_SECURITY',
    details: 'Compute the confusable skeleton of both strings and store true when the ' +
      'skeletons are equal, meaning the two strings are visually confusable, such as ' +
      '"good" and "gооd" with Cyrillic о. If `MJB_FEATURE_SECURITY=0` the function always returns ' +
      '`MJB_STATUS_FEATURE_NOT_ENABLED`.' +
      ' Malformed subsequences follow `malformed_policy`. `first_diagnostic` and ' +
      '`second_diagnostic` each record the first malformed subsequence of their own text, with ' +
      'offsets into that text; when `first` stops on malformed input, `second` is not examined ' +
      'and `second_diagnostic` stays clear.',
    returns: [
      { value: 'MJB_STATUS_OK', description: '`confusable` contains the comparison result' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description:
        '`confusable`, `first`, or `second` is NULL, a text buffer is NULL with a non-zero ' +
        'size, or the malformed policy is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'An input encoding is invalid or lacks byte-order information' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'An intermediate size would overflow' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'Allocation failed' },
      { value: 'MJB_STATUS_FEATURE_NOT_ENABLED', description:
        'The library was compiled with `MJB_FEATURE_SECURITY=0`' }
    ],
    examples: {
      c: `mjb_text latin = mjb_text_utf8("hello", MJB_NUL_TERMINATED);
mjb_text mixed = mjb_text_utf8("h\\xD0\\xB5llo", MJB_NUL_TERMINATED); // Cyrillic е
bool confusable;

if(mjb_confusable_match(&latin, &mixed, MJB_MALFORMED_STOP, &confusable, NULL,
    NULL) != MJB_STATUS_OK) {
    return 1;
}

// Visually confusable: yes
printf("Visually confusable: %s", confusable ? "yes" : "no");`
    },
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
    examples: {
      c: `mjb_emoji_properties emoji;

if(mjb_codepoint_emoji_properties(0x1F600, &emoji) != MJB_STATUS_OK) {
    return 1;
}

// U+1F600 has Emoji_Presentation: yes
printf("U+1F600 has Emoji_Presentation: %s", emoji.presentation ? "yes" : "no");`
    },
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
    examples: {
      c: `// Number sign has the Emoji property: yes
bool emoji = mjb_codepoint_is_emoji('#');

// Number sign has the Emoji property: yes
printf("Number sign has the Emoji property: %s", emoji ? "yes" : "no");`,
      cpp: `// Number sign has the Emoji property: yes
std::cout << "Number sign has the Emoji property: "
    << (mjb::is_emoji('#') ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Grinning face defaults to emoji presentation: yes
bool presentation = mjb_codepoint_is_emoji_presentation(0x1F600);

// Grinning face defaults to emoji presentation: yes
printf("Grinning face defaults to emoji presentation: %s", presentation ? "yes" : "no");`,
      cpp: `// Grinning face defaults to emoji presentation: yes
std::cout << "Grinning face defaults to emoji presentation: "
    << (mjb::is_emoji_presentation(0x1F600) ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Medium skin tone is an emoji modifier: yes
bool modifier = mjb_codepoint_is_emoji_modifier(0x1F3FD);

// Medium skin tone is an emoji modifier: yes
printf("Medium skin tone is an emoji modifier: %s", modifier ? "yes" : "no");`,
      cpp: `// Medium skin tone is an emoji modifier: yes
std::cout << "Medium skin tone is an emoji modifier: "
    << (mjb::is_emoji_modifier(0x1F3FD) ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Waving hand accepts an emoji modifier: yes
bool modifier_base = mjb_codepoint_is_emoji_modifier_base(0x1F44B);

// Waving hand accepts an emoji modifier: yes
printf("Waving hand accepts an emoji modifier: %s", modifier_base ? "yes" : "no");`,
      cpp: `// Waving hand accepts an emoji modifier: yes
std::cout << "Waving hand accepts an emoji modifier: "
    << (mjb::is_emoji_modifier_base(0x1F44B) ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Zero-width joiner is an emoji component: yes
bool component = mjb_codepoint_is_emoji_component(0x200D);

// Zero-width joiner is an emoji component: yes
printf("Zero-width joiner is an emoji component: %s", component ? "yes" : "no");`,
      cpp: `// Zero-width joiner is an emoji component: yes
std::cout << "Zero-width joiner is an emoji component: "
    << (mjb::is_emoji_component(0x200D) ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `// Red heart is Extended_Pictographic: yes
bool pictographic = mjb_codepoint_is_extended_pictographic(0x2764);

// Red heart is Extended_Pictographic: yes
printf("Red heart is Extended_Pictographic: %s", pictographic ? "yes" : "no");`,
      cpp: `// Red heart is Extended_Pictographic: yes
std::cout << "Red heart is Extended_Pictographic: "
    << (mjb::is_extended_pictographic(0x2764) ? "yes" : "no") << std::endl;`
    },
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
    examples: {
      c: `mjb_plane plane = mjb_codepoint_plane(0x1F600);

// U+1F600 is in the SMP: yes
printf("U+1F600 is in the SMP: %s", plane == MJB_PLANE_SMP ? "yes" : "no");`
    }
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
    examples: {
      c: `// Plane 16 is valid: yes
printf("Plane 16 is valid: %s", mjb_plane_is_valid(MJB_PLANE_PUA_B) ? "yes" : "no");`
    }
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
    examples: {
      c: `// Plane: Basic Multilingual Plane
printf("Plane: %s", mjb_plane_name(MJB_PLANE_BMP, false));`
    }
  },
  {
    comment: 'Return emoji sequence metadata for a complete string.',
    ret: 'mjb_status',
    name: 'mjb_emoji_sequence_info',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The text to check'),
      {
        name: 'emoji',
        type: 'mjb_emoji_sequence *',
        description: 'The emoji sequence metadata to store the result',
        wasm_generated: true
      }
    ],
    wasm: true,
    section: Section.Emoji,
    examples: {
      c: `mjb_text flag = mjb_text_utf8("\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9", MJB_NUL_TERMINATED); // 🇮🇹
mjb_emoji_sequence emoji;

if(mjb_emoji_sequence_info(&flag, &emoji) != MJB_STATUS_OK) {
    return 1;
}

// Sequence codepoints: 2
printf("Sequence codepoints: %zu", emoji.codepoint_count);`
    },
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
      text('The text to check')
    ],
    section: Section.Emoji,
    wasm: true,
    examples: {
      c: `mjb_text keycap = mjb_text_utf8("1\\xEF\\xB8\\x8F\\xE2\\x83\\xA3", MJB_NUL_TERMINATED); // 1️⃣

bool listed = mjb_is_emoji_sequence(&keycap);

// Listed emoji sequence: yes
printf("Listed emoji sequence: %s", listed ? "yes" : "no");`
    },
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
      text('The text to check')
    ],
    wasm: true,
    wasmName: 'isRGIEmoji',
    section: Section.Emoji,
    examples: {
      c: `mjb_text flag = mjb_text_utf8("\\xF0\\x9F\\x87\\xAE\\xF0\\x9F\\x87\\xB9", MJB_NUL_TERMINATED); // 🇮🇹

bool rgi = mjb_is_rgi_emoji(&flag);

// RGI emoji: yes
printf("RGI emoji: %s", rgi ? "yes" : "no");`
    },
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
    examples: {
      c: `char name[32];

if(mjb_hangul_syllable_name(0xAC01, name, sizeof(name)) != MJB_STATUS_OK) {
    return 1;
}

// Name: HANGUL SYLLABLE GAG
printf("Name: %s", name);`
    }
  },
  {
    comment: 'Decompose a Hangul syllable into its jamo.',
    ret: 'mjb_status',
    name: 'mjb_hangul_syllable_decompose',
    attributes: ['MJB_NODISCARD'],
    args: [
      codepoint(),
      {
        name: 'codepoints',
        type: 'mjb_codepoint *',
        description: 'Three codepoints to store the leading, vowel, and trailing jamo; the ' +
          'trailing jamo is 0 when the syllable has none',
        wasm_generated: true
      }
    ],
    wasm: false,
    section: Section.HangulLanguage,
    details: 'Apply the arithmetic Hangul syllable decomposition of Unicode Section 3.12, the ' +
      'inverse of `mjb_hangul_syllable_compose`.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The syllable was decomposed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`codepoints` is NULL' },
      { value: 'MJB_STATUS_NOT_FOUND', description: '`codepoint` is not a Hangul syllable' }
    ],
    examples: {
      c: `mjb_codepoint decomposition[3];

if(mjb_hangul_syllable_decompose(0xAC01, decomposition) != MJB_STATUS_OK) {
    return 1;
}

// Decomposition starts with: U+1100
printf("Decomposition starts with: U+%04X", decomposition[0]);`
    },
    related: ['mjb_hangul_syllable_compose', 'mjb_hangul_syllable_name']
  },
  {
    comment: 'Compose Hangul jamo into a syllable.',
    ret: 'mjb_status',
    name: 'mjb_hangul_syllable_compose',
    attributes: ['MJB_NODISCARD'],
    args: [
      {
        name: 'leading',
        type: 'mjb_codepoint',
        description: 'The leading consonant jamo (L), U+1100 to U+1112',
        wasm_generated: false
      },
      {
        name: 'vowel',
        type: 'mjb_codepoint',
        description: 'The vowel jamo (V), U+1161 to U+1175',
        wasm_generated: false
      },
      {
        name: 'trailing',
        type: 'mjb_codepoint',
        description: 'The trailing consonant jamo (T), U+11A8 to U+11C2, or 0 for none',
        wasm_generated: false
      },
      {
        name: 'syllable',
        type: 'mjb_codepoint *',
        description: 'The composed syllable; set to `MJB_CODEPOINT_NOT_VALID` on failure',
        wasm_generated: true
      }
    ],
    wasm: false,
    section: Section.HangulLanguage,
    details: 'Apply the arithmetic Hangul syllable composition of Unicode Section 3.12, the ' +
      'inverse of `mjb_hangul_syllable_decompose`. Normalization composes Hangul automatically; ' +
      'use this function to build a syllable from individual jamo.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The syllable was composed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT', description: '`syllable` is NULL' },
      { value: 'MJB_STATUS_NOT_FOUND', description:
        'A jamo is outside its range, so the jamo do not form a syllable' }
    ],
    examples: {
      c: `mjb_codepoint syllable;

// Choseong kiyeok, jungseong a, jongseong kiyeok
if(mjb_hangul_syllable_compose(0x1100, 0x1161, 0x11A8, &syllable) != MJB_STATUS_OK) {
    return 1;
}

// Syllable: U+AC01
printf("Syllable: U+%04X", syllable);`
    },
    related: ['mjb_hangul_syllable_decompose', 'mjb_normalize']
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
    examples: {
      c: `mjb_east_asian_width width;

if(mjb_codepoint_east_asian_width(0x754C, &width) != MJB_STATUS_OK) { // 界
    return 1;
}

// U+754C is wide: yes
printf("U+754C is wide: %s", width == MJB_EAW_WIDE ? "yes" : "no");`
    },
    related: ['mjb_terminal_width'],
    specs: [uax(11, 'East Asian Width')]
  },
  {
    comment: 'Return the estimated terminal-cell width of printable, single-line text.',
    ret: 'mjb_status',
    name: 'mjb_terminal_width',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The printable, single-line text to measure'),
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
        '`width` is NULL, the profile or malformed policy is invalid, or the text is invalid' },
      { value: 'MJB_STATUS_INVALID_ENCODING', description: 'The encoding is invalid or lacks required byte-order information' },
      { value: 'MJB_STATUS_UNSUPPORTED', description: 'The input contains a control, line separator, or paragraph separator' },
      { value: 'MJB_STATUS_NO_MEMORY', description: 'NFC normalization could not allocate memory' },
      { value: 'MJB_STATUS_OVERFLOW', description: 'The width would overflow' },
      { value: 'MJB_STATUS_MALFORMED_INPUT', description:
        'Malformed input was encountered with `MJB_MALFORMED_STOP`' }
    ],
    examples: {
      c: `mjb_text input = mjb_text_utf8("A\\xE7\\x95\\x8C", MJB_NUL_TERMINATED); // A界
size_t width;

if(mjb_terminal_width(&input, MJB_MALFORMED_STOP,
    MJB_TERMINAL_WIDTH_NARROW, &width, NULL) != MJB_STATUS_OK) {
    return 1;
}

// Terminal cells: 3
printf("Terminal cells: %zu", width);`
    },
    related: ['mjb_codepoint_east_asian_width', 'mjb_grapheme_prefix_length_for_width',
      'mjb_grapheme_break_next', 'mjb_is_rgi_emoji'],
    specs: [uax(11, 'East Asian Width'), uax(29, 'Unicode Text Segmentation'),
      uts(51, 'Unicode Emoji')]
  },
  {
    comment: 'Parse a BCP 47 language tag.',
    ret: 'mjb_status',
    name: 'mjb_locale_parse',
    attributes: ['MJB_NODISCARD'],
    args: [
      text('The BCP 47 language tag to parse', 'id'),
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
    examples: {
      c: `mjb_text id = mjb_text_utf8("sr-Latn-RS", MJB_NUL_TERMINATED);
mjb_locale_id locale;

if(mjb_locale_parse(&id, &locale) != MJB_STATUS_OK) {
    return 1;
}

// Locale: sr Latn RS
printf("Locale: %s %s %s", locale.language, locale.script, locale.region);`
    },
    related: ['mjb_locale_code', 'mjb_map_case'],
    specs: [{ name: 'BCP 47: Tags for Identifying Languages', url: 'https://www.rfc-editor.org/rfc/rfc5646' }]
  },
  {
    comment: 'Return the ISO 639-1 code of a locale, NULL if the locale is not valid.',
    ret: 'const char *',
    name: 'mjb_locale_code',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'locale',
        type: 'mjb_locale',
        description: 'The locale',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.Utility,
    details: 'Return the lowercase two-letter ISO 639-1 code that names `locale`, such as `tr` ' +
      'for `MJB_LOCALE_TR`. `MJB_LOCALE_NOT_SET` yields an empty string.',
    returns: [
      { value: 'const char *', description: 'The ISO 639-1 code, or NULL if `locale` is not a valid `mjb_locale` value' }
    ],
    examples: {
      c: `// Locale code: tr
printf("Locale code: %s", mjb_locale_code(MJB_LOCALE_TR));`
    },
    related: ['mjb_locale_parse', 'mjb_map_case']
  },
  {
    comment: 'Return the ISO 15924 code of a script, NULL if the script is not valid.',
    ret: 'const char *',
    name: 'mjb_script_code',
    attributes: ['MJB_CONST'],
    args: [
      {
        name: 'script',
        type: 'mjb_script',
        description: 'The script',
        wasm_generated: false
      }
    ],
    wasm: true,
    section: Section.TextAnalysis,
    details: 'Return the four-letter ISO 15924 code that names `script`, such as `Latn` for ' +
      '`MJB_SC_LATN`. `MJB_SC_NOT_SET` yields an empty string.',
    returns: [
      { value: 'const char *', description: 'The ISO 15924 code, or NULL if `script` is not a valid `mjb_script` value' }
    ],
    examples: {
      c: `// Script code: Latn
printf("Script code: %s", mjb_script_code(MJB_SC_LATN));`
    },
    related: ['mjb_codepoint_script', 'mjb_codepoint_script_extensions']
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
    examples: {
      c: `mjb_result result;

mjb_text input = mjb_text_utf8("A", 1);

if(mjb_convert_encoding(&input, MJB_MALFORMED_STOP, MJB_ENC_UTF_16LE, &result,
    NULL) != MJB_STATUS_OK ||
    mjb_result_free(&result) != MJB_STATUS_OK) {
    return 1;
}

// Result released: yes
printf("Result released: %s", result.output == NULL ? "yes" : "no");`
    }
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
    examples: {
      c: `char buffer[4];
int required = mjb_utf8_snprintf(buffer, sizeof(buffer), "%s",
    "\\xC3\\xA9\\xC3\\xA9"); // éé

// 4: é
printf("%d: %s", required, buffer);`
    },
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
    examples: {
      c: `// A printf-style function of your own that never ends with a partial codepoint
static int format_utf8(char *buffer, size_t buffer_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int required = mjb_utf8_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return required;
}

char buffer[4];
int required = format_utf8(buffer, sizeof(buffer), "%s", "\\xC3\\xA9\\xC3\\xA9"); // éé

// 4: é
printf("%d: %s", required, buffer);`
    },
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
    examples: {
      c: `char buffer[4];
int required = mjb_utf8_grapheme_snprintf(buffer, sizeof(buffer), "%s",
    "Ae\\xCC\\x81" "B"); // A, e + combining acute accent, B

// 5: A
printf("%d: %s", required, buffer);`
    },
    related: ['mjb_utf8_grapheme_vsnprintf', 'mjb_utf8_snprintf',
      'mjb_grapheme_prefix_length'],
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
    examples: {
      c: `// A printf-style function of your own that never splits a grapheme cluster
static int format_graphemes(char *buffer, size_t buffer_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int required = mjb_utf8_grapheme_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return required;
}

char buffer[4];
int required = format_graphemes(buffer, sizeof(buffer), "%s",
    "Ae\\xCC\\x81" "B"); // A, e + combining acute accent, B

// 5: A
printf("%d: %s", required, buffer);`
    },
    related: ['mjb_utf8_grapheme_snprintf', 'mjb_utf8_vsnprintf',
      'mjb_grapheme_prefix_length'],
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
    examples: {
      c: `const char *message = mjb_status_message(MJB_STATUS_INVALID_ARGUMENT);

// Error message: One or more arguments are invalid or inconsistent with the requested operation
printf("Error message: %s", message);`
    },
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
    examples: {
      c: `const char *version = mjb_version();

// Version is available: yes
printf("Version is available: %s", version[0] != '\\0' ? "yes" : "no");`
    },
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
    examples: {
      c: `unsigned int version = mjb_version_number();

// Version number is positive: yes
printf("Version number is positive: %s", version > 0 ? "yes" : "no");`
    },
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
    examples: {
      c: `const char *version = mjb_unicode_version();

// Unicode version: 18.0.0
printf("Unicode version: %s", version);`
    },
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
      'pointer. The library passes the size of every block it reallocates or frees, so arena, ' +
      'pool, and accounting allocators do not need to record sizes themselves: `old_size` and ' +
      '`size` are the size most recently requested for that block, requested sizes are never 0, ' +
      '`realloc` receives NULL with an `old_size` of 0 for a new block and leaves the old block ' +
      'valid on failure, and `free` is never called with NULL. The context remains caller-owned ' +
      'and must stay valid for the lifetime of all Mojibake allocations. Pass NULL to explicitly ' +
      'select the default allocator. This process-global configuration may be set only once and ' +
      'must happen before any other library call or concurrent library use.',
    returns: [
      { value: 'MJB_STATUS_OK', description: 'The allocator was installed' },
      { value: 'MJB_STATUS_INVALID_ARGUMENT',
        description: 'A callback is NULL, or the allocator was already configured' }
    ],
    exampleCompileOnly: true,
    examples: {
      c: `// An allocator that counts the bytes in use. The context points to the counter.
static void *counting_alloc(void *context, size_t size) {
    void *ptr = malloc(size);

    if(ptr) {
        *(size_t *)context += size;
    }

    return ptr;
}

static void *counting_realloc(void *context, void *ptr, size_t old_size, size_t new_size) {
    void *new_ptr = realloc(ptr, new_size);

    if(new_ptr) {
        *(size_t *)context = *(size_t *)context - old_size + new_size;
    }

    return new_ptr;
}

static void counting_free(void *context, void *ptr, size_t size) {
    *(size_t *)context -= size;
    free(ptr);
}

static size_t bytes_in_use = 0;
mjb_allocator allocator = { &bytes_in_use, counting_alloc, counting_realloc, counting_free };

// Install it once, before any other library call
if(mjb_set_allocator(&allocator) != MJB_STATUS_OK) {
    return 1;
}`
    },
    related: ['mjb_result_free', 'mjb_bidi_paragraph_free']
  }
] as MojibakeFunction[];
