/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import mojibakeModule from './mojibake.js';
import type { Codepoint, MojibakeWasmModule } from './mojibake.js';

export * from './locales.js';
export * from './unicode.js';

// mjb_encoding
export enum Encoding {
  UNKNOWN  = 0x0,
  ASCII    = 0x1,
  UTF_8    = 0x2,
  UTF_16   = 0x4,
  UTF_16BE = 0x8,
  UTF_16LE = 0x10,
  UTF_32   = 0x20,
  UTF_32BE = 0x40,
  UTF_32LE = 0x80,
};

// mjb_normalization
export enum Normalization {
  NFC,
  NFD,
  NFKC,
  NFKD,
};

// mjb_quick_check_result
export enum QuickCheckResult {
  YES        = 0x0,
  NO         = 0x1,
  MAYBE      = 0x2,
  // See: DerivedNormalizationProps.txt
  NFD_NO     = 0x4,
  NFD_MAYBE  = 0x8, // Impossible to happen
  NFC_NO     = 0x10,
  NFC_MAYBE  = 0x20,
  NFKC_NO    = 0x40,
  NFKC_MAYBE = 0x80,
  NFKD_NO    = 0x100,
  NFKD_MAYBE = 0x200 // Impossible to happen
};

// mjb_filter
export enum FilterType {
  NONE            = 0x0,
  NORMALIZE       = 0x1,
  SPACES          = 0x2,
  COLLAPSE_SPACES = 0x4,
  CONTROLS        = 0x8,
  NUMERIC         = 0x10,
  LIMIT_COMBINING = 0x20
};

// mjb_case_type
export enum CaseType {
  NONE,
  UPPER,
  LOWER,
  TITLE,
  CASEFOLD
};

// mjb_error
export enum ErrorType {
  NONE,
  INVALID_ARGUMENT,
  UNSUPPORTED,
};

// mjb_status
export enum Status {
  OK = 0,
  INVALID_ARGUMENT,
  INVALID_ENCODING,
  INVALID_CODEPOINT,
  INVALID_FORM,
  UNSUPPORTED,
  NO_MEMORY,
  OVERFLOW,
  MALFORMED_INPUT,
  OUTPUT_TOO_SMALL,
  CALLBACK_STOPPED,
  NOT_FOUND,
};

// mjb_locale_id
export type LocaleID = {
  language: string;      // 9
  extlang: string;       // 12
  script: string;        // 5
  region: string;        // 4
  variant: string;       // 32
  extensions: string;    // 128
  private_use: string;   // 128
  grandfathered: string; // 32
}

type ComposedString = {
  bytes: Uint8Array | null;
  output: string;
}

// mjb_result pointer
type RawResult = {
  output: Pointer;
  output_size: number;
  transformed: boolean;
};

// mjb_result
type Result = ComposedString & {
  outputSize: number;
  transformed: boolean;
};

// mjb_block_info
export type BlockInfo = {
  id: number;
  name: string; // 128
  start: number;
  end: number;
}

// mjb_character
export type Character = {
  codepoint: number | null;
  name: string; // 128
  category: number;
  combining: number;
  bidirectional: number;
  decomposition: number;
  decimal: number | null;
  digit: number | null;
  numeric: string; // 16
  mirrored: boolean;
  uppercase: number | null;
  lowercase: number | null;
  titlecase: number | null;
}

// mjb_numeric_value
export type NumericValue = {
  decimal: number | null;
  digit: number | null;
  numeric: string; // 16
}

// mjb_emoji_properties
export type EmojiProperties = {
  codepoint: number | null;
  emoji: boolean;
  presentation: boolean;
  modifier: boolean;
  modifier_base: boolean;
  component: boolean;
  extended_pictographic: boolean;
}

// mjb_emoji_sequence_type
export enum EmojiSequenceType {
  NONE,
  BASIC,
  KEYCAP,
  FLAG,
  TAG,
  MODIFIER,
  ZWJ,
  TEXT_VARIATION,
  EMOJI_VARIATION,
};

// mjb_emoji_qualification
export enum EmojiQualification {
  NONE,
  COMPONENT,
  FULLY_QUALIFIED,
  MINIMALLY_QUALIFIED,
  UNQUALIFIED,
};

// mjb_emoji_sequence
export type EmojiSequence = {
  type: EmojiSequenceType;
  qualification: EmojiQualification;
  codepoint_count: number;
}

// mjb_break_type
export enum BreakType {
  NOT_SET,
  MANDATORY,
  NO_BREAK,
  ALLOWED,
};

export type BufferCharacter = {
  codepoint: Codepoint;
  combining: number;
}

// mjb_character_position
export enum NextCharacterType {
  NONE,
  FIRST,
  LAST,
};

// mjb_width_context
export enum WidthContext {
  WESTERN,
  EAST_ASIAN,
  AUTO
};

// mjb_next_state
// mjb_next_line_state
// mjb_next_word_state
// mjb_next_sentence_state
// mjb_string_each_character_fn

// mjb_direction
export enum Direction {
  LTR,
  RTL,
  AUTO
};

// mjb_bidi_char
export type BidiChar = {
  codepoint: number;
  byte_offset: number;
  level: number;
  resolved_class: number;
  mirroring_glyph: number | null;
};

// mjb_bidi_paragraph
export type BidiParagraph = {
  chars: BidiChar[];
  count: number;
  paragraph_level: number;
  direction: Direction;
}

// mjb_bidi_run

// mjb_collation_mode
export enum CollationMode {
  NON_IGNORABLE,
  SHIFTED,
};

// mjb_identifier_profile
export enum IdentifierProfile {
  DEFAULT,
  NFKC,
};

// Result of mjb_next_character function
export type NextCharacter = {
  character: Character;
  type: NextCharacterType; // mjb_character_position
}

// Generic pointer type for memory management
export type Pointer = number;

// Mojibake accept UTF_8, UTF_16, UTF_32
export type MojibakeInput = string | ArrayBuffer | ArrayBufferView;

// Used for `buffer, encoding, output_encoding` type of functions
export type TextInputOptions = {
  encoding?: Encoding;
  additionalEncoding?: Encoding;
  outputEncoding?: Encoding;
};

// Used for preRun and postRun callbacks
export type MojibakeRuntimeCallback = (module: MojibakeWasmModule) => void;

// Seem Module object: https://emscripten.org/docs/api_reference/module.html
export type MojibakeModuleOptions = {
  arguments?: string[];
  // buffer
  // wasmMemory: ArrayBuffer | SharedArrayBuffer
  locateFile?: (path: string, prefix: string) => string;
  // logReadFiles: boolean
  // printWithColors: boolean
  onAbort?: (reason: unknown) => void;
  onRuntimeInitialized?: () => void;
  noExitRuntime?: boolean;
  // noItialRun: boolean
  preInit?: (() => void) | (() => void)[];
  // preinitializedWebGLContext: WebGLRenderingContext
  preRun?: MojibakeRuntimeCallback | MojibakeRuntimeCallback[];
  postRun?: MojibakeRuntimeCallback | MojibakeRuntimeCallback[];
  print?: (text: string) => void;
  printErr?: (text: string) => void;
  // mainScriptUrlOrBlob: any
};

// String data structure for handling string data in WebAssembly heap memory
type WasmInput = {
  ptr: Pointer;
  size: number;
  encoding: Encoding;
};

// Used by collectBreaks callbacks
type BreakFunction = (buffer: Pointer, size: number, encoding: number, state: Pointer) => number;

// Emscripten exported function type
type MojibakeModuleFactory = (options?: MojibakeModuleOptions) => Promise<MojibakeWasmModule>;

// Read memory to be collected as a structure
class StructReader {
  private offset = 0;

  constructor(private readonly ptr: Pointer, private readonly heap: Uint8Array,
    private readonly view: DataView, private readonly decoder: TextDecoder) {}

  u8(): number {
    return this.heap[this.ptr + this.offset++];
  }

  u16(): number {
    this.align(2);
    const value = this.view.getUint16(this.ptr + this.offset, true);
    this.offset += 2;

    return value;
  }

  u32(): number {
    this.align(4);
    const value = this.view.getUint32(this.ptr + this.offset, true);
    this.offset += 4;

    return value;
  }

  i32(): number {
    this.align(4);
    const value = this.view.getInt32(this.ptr + this.offset, true);
    this.offset += 4;

    return value;
  }

  str(maxLength: number): string {
    const start = this.ptr + this.offset;
    const bytes = this.heap.subarray(start, start + maxLength);
    const end = bytes.indexOf(0);
    this.offset += maxLength;

    return this.decoder.decode(end === -1 ? bytes : bytes.subarray(0, end));
  }

  private align(bytes: number): void {
    this.offset = (this.offset + bytes - 1) & ~(bytes - 1);
  }
}

export class Mojibake {
  utf8Encoder = new TextEncoder();
  utf8Decoder = new TextDecoder('utf-8');
  utf16beDecoder = new TextDecoder('utf-16be');
  utf16leDecoder = new TextDecoder('utf-16le');

  private constructor(private readonly module: MojibakeWasmModule) {}

  static async create(options?: MojibakeModuleOptions): Promise<Mojibake> {
    const module = await (mojibakeModule as MojibakeModuleFactory)(options);

    return new Mojibake(module);
  }

  // mjb_status mjb_codepoint_character(mjb_codepoint codepoint, mjb_character *character)
  codepointCharacter(codepoint: number): Character | null {
    // Allocate memory for mjb_character structure
    const structSize = 512;
    const ptr = this.malloc(structSize);

    try {
      const status = this.module._mjb_codepoint_character(codepoint, ptr);

      if(status === Status.OK) {
        return this.pointerToCharacter(ptr);
      }

      throw new Error('Failed to get character data for codepoint');
    } finally {
      this.free(ptr);
    }
  }

  // mjb_status mjb_normalize(const char *buffer, size_t byte_length, mjb_normalization form, mjb_encoding
  // encoding, mjb_encoding output_encoding, mjb_result *result)
  normalize(input: MojibakeInput, form = Normalization.NFC,
    options: TextInputOptions = {}): Result | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const outputEncoding = this.resolveEncoding(options.outputEncoding ?? wasmInput.encoding);
    const resultPtr = this.malloc(12); // 4 + 4 + 1 + 3 padding for mjb_result
    let result: RawResult | null = null;

    try {
      const status = this.module._mjb_normalize(wasmInput.ptr, wasmInput.size,
        form, wasmInput.encoding, outputEncoding, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      result = this.pointerToResult(resultPtr);

      return this.rawResultToResult(result, outputEncoding);
    } finally {
      if(result?.transformed && result.output !== 0) {
        this.free(result.output);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // mjb_status mjb_nfkc_casefold(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_encoding output_encoding, mjb_result *result)
  nfkcCasefold(input: MojibakeInput, options: TextInputOptions = {}): Result | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const outputEncoding = this.resolveEncoding(options.outputEncoding ?? wasmInput.encoding);
    const resultPtr = this.malloc(12); // 4 + 4 + 1 + 3 padding for mjb_result
    let result: RawResult | null = null;

    try {
      const status = this.module._mjb_nfkc_casefold(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, outputEncoding, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      result = this.pointerToResult(resultPtr);

      return this.rawResultToResult(result, outputEncoding);
    } finally {
      if(result?.transformed && result.output !== 0) {
        this.free(result.output);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // mjb_quick_check_result mjb_string_is_normalized(const char *buffer, size_t byte_length, mjb_encoding
  // encoding, mjb_normalization form)
  stringIsNormalized(input: MojibakeInput, form = Normalization.NFC,
    options: TextInputOptions = {}): number {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_is_normalized(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, form);
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // mjb_status mjb_string_each_character(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_string_each_character callback)
  nextCharacter(input: MojibakeInput, options: TextInputOptions = {}): NextCharacter[] | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const previousCallback = (globalThis as any)._mjbEachCharacterCallback;
    const characters: NextCharacter[] = [];

    // See mjb_next_character function
    (globalThis as any)._mjbEachCharacterCallback = (character: Pointer, type: number) => {
      characters.push({
        character: this.pointerToCharacter(character),
        type
      });

      return true;
    };

    try {
      const status = this.module._mjb_string_each_character(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, 0);

      if(status !== Status.OK) {
        return null;
      }

      return characters;
    } finally {
      if(previousCallback === undefined) {
        delete (globalThis as any)._mjbEachCharacterCallback;
      } else {
        (globalThis as any)._mjbEachCharacterCallback = previousCallback;
      }

      this.free(wasmInput.ptr);
    }
  }

  // mjb_status mjb_string_filter(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_encoding output_encoding, mjb_filter filters, mjb_result *result)
  stringFilter(input: MojibakeInput, filters = FilterType.NONE,
    options: TextInputOptions = {}): Result | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const outputEncoding = this.resolveEncoding(options.outputEncoding ?? wasmInput.encoding);
    const resultPtr = this.malloc(12); // 4 + 4 + 1 + 3 padding for mjb_result
    let result: RawResult | null = null;

    try {
      const status = this.module._mjb_string_filter(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, outputEncoding, filters, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      result = this.pointerToResult(resultPtr);

      return this.rawResultToResult(result, outputEncoding);
    } finally {
      if(result?.transformed && result.output !== 0) {
        this.free(result.output);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // mjb_status mjb_codepoint_property_binary(mjb_codepoint codepoint, mjb_property property,
  // bool *value)
  codepointPropertyBinary(codepoint: Codepoint, property: number): boolean | null {
    const valuePtr = this.malloc(1);

    try {
      const status = this.module._mjb_codepoint_property_binary(codepoint, property, valuePtr);

      return status === Status.OK ? this.module.HEAPU8[valuePtr] !== 0 : null;
    } finally {
      this.free(valuePtr);
    }
  }

  // mjb_status mjb_codepoint_property_int(mjb_codepoint codepoint, mjb_property property,
  // int32_t *value)
  codepointPropertyInt(codepoint: Codepoint, property: number): number | null {
    const valuePtr = this.malloc(4);

    try {
      const status = this.module._mjb_codepoint_property_int(codepoint, property, valuePtr);

      return status === Status.OK ? this.module.HEAP32[valuePtr / 4] : null;
    } finally {
      this.free(valuePtr);
    }
  }

  // mjb_status mjb_codepoint_script_extensions(mjb_codepoint codepoint, mjb_script *scripts,
  // size_t *count);
  codepointScriptExtensions(codepoint: Codepoint): number[] | null {
    const countPtr = this.malloc(4);
    let scriptsPtr = 0;

    try {
      this.module.HEAP32[countPtr / 4] = 0;
      let status = this.module._mjb_codepoint_script_extensions(codepoint, 0, countPtr);

      if(status !== Status.OK) {
        return null;
      }

      const count = this.module.HEAP32[countPtr / 4];
      scriptsPtr = this.malloc(count * 4);
      this.module.HEAP32[countPtr / 4] = count;
      status = this.module._mjb_codepoint_script_extensions(codepoint, scriptsPtr, countPtr);

      if(status !== Status.OK) {
        return null;
      }

      return Array.from(this.module.HEAP32.subarray(scriptsPtr / 4, scriptsPtr / 4 + count));
    } finally {
      if(scriptsPtr !== 0) {
        this.free(scriptsPtr);
      }

      this.free(countPtr);
    }
  }

  // mjb_script mjb_codepoint_script(mjb_codepoint codepoint)
  codepointScript(codepoint: Codepoint): number {
    return this.module._mjb_codepoint_script(codepoint);
  }

  // mjb_encoding mjb_string_encoding(const char *buffer, size_t byte_length)
  stringEncoding(input: MojibakeInput, options: TextInputOptions = {}): number {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_encoding(wasmInput.ptr, wasmInput.size);
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // bool mjb_string_is_utf8(const char *buffer, size_t byte_length)
  stringIsUtf8(input: MojibakeInput, options: TextInputOptions = {}): boolean {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_is_utf8(wasmInput.ptr, wasmInput.size) ? true : false;
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // bool mjb_string_is_utf16(const char *buffer, size_t byte_length)
  stringIsUtf16(input: MojibakeInput, options: TextInputOptions = {}): boolean {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_is_utf16(wasmInput.ptr, wasmInput.size) ? true : false;
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // bool mjb_string_is_ascii(const char *buffer, size_t byte_length)
  stringIsAscii(input: MojibakeInput, options: TextInputOptions = {}): boolean {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_is_ascii(wasmInput.ptr, wasmInput.size) ? true : false;
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // unsigned int mjb_codepoint_encode(mjb_codepoint codepoint, char *buffer, size_t byte_length,
  // mjb_encoding encoding)
  codepointEncode(codepoint: Codepoint, encoding = Encoding.UTF_8): ComposedString | null {
    encoding = this.resolveEncoding(encoding);
    const bufferPtr = this.malloc(5);

    try {
      const size = this.module._mjb_codepoint_encode(codepoint, bufferPtr, 5, encoding);

      if(size === 0) {
        return null;
      }

      return this.decodeString(bufferPtr, size, encoding);
    } finally {
      this.free(bufferPtr);
    }
  }

  // mjb_status mjb_string_convert_encoding(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_encoding output_encoding, mjb_result *result)
  stringConvertEncoding(input: MojibakeInput, outputEncoding = Encoding.UTF_8,
    options: TextInputOptions = {}): Result | null {
    const wasmInput = this.copyInput(input, options.encoding);
    outputEncoding = this.resolveEncoding(outputEncoding);
    const resultPtr = this.malloc(24);
    let result: RawResult | null = null;

    try {
      const status = this.module._mjb_string_convert_encoding(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, outputEncoding, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      result = this.pointerToResult(resultPtr);

      return this.rawResultToResult(result, outputEncoding);
    } finally {
      if(result?.transformed && result.output !== 0) {
        this.free(result.output);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // size_t mjb_string_length(const char *buffer, size_t max_length, mjb_encoding encoding)
  stringLength(input: MojibakeInput, maxLength = 0x7FFFFFFF, options: TextInputOptions = {}): number {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_length(wasmInput.ptr, Math.min(maxLength, wasmInput.size),
        wasmInput.encoding);
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // int mjb_string_compare(const char *s1, size_t s1_byte_length, mjb_encoding s1_encoding,
  // const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding, mjb_collation_mode mode
  stringCompare(first: MojibakeInput, second: MojibakeInput,
    mode = CollationMode.NON_IGNORABLE, options: TextInputOptions = {}): number {
    const firstInput = this.copyInput(first, options.encoding);
    const secondInput = this.copyInput(second, options.additionalEncoding ?? options.encoding);

    try {
      return this.module._mjb_string_compare(firstInput.ptr, firstInput.size,
        firstInput.encoding, secondInput.ptr, secondInput.size, secondInput.encoding, mode);
    } finally {
      this.free(firstInput.ptr);
      this.free(secondInput.ptr);
    }
  }

  // mjb_status mjb_collation_key(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_collation_mode mode, mjb_result *result)
  collationKey(input: MojibakeInput, mode = CollationMode.NON_IGNORABLE,
    options: TextInputOptions = {}): Uint8Array | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const resultPtr = this.malloc(24);
    let result: RawResult | null = null;

    try {
      const status = this.module._mjb_collation_key(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, mode, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      result = this.pointerToResult(resultPtr);

      if(result.output === 0 || result.output_size === 0) {
        return new Uint8Array();
      }

      return new Uint8Array(this.module.HEAPU8.subarray(result.output,
        result.output + result.output_size));
    } finally {
      if(result?.transformed && result.output !== 0) {
        this.free(result.output);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // mjb_status mjb_case(const char *buffer, size_t byte_length, mjb_case_type type,
  // mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result)
  case(input: MojibakeInput, type: CaseType, options: TextInputOptions = {}): Result | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const outputEncoding = this.resolveEncoding(options.outputEncoding ?? wasmInput.encoding);
    const resultPtr = this.malloc(12); // 4 + 4 + 1 + 3 padding for mjb_result
    let result: RawResult | null = null;

    try {
      const status = this.module._mjb_case(wasmInput.ptr, wasmInput.size, type,
        wasmInput.encoding, outputEncoding, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      result = this.pointerToResult(resultPtr);

      return this.rawResultToResult(result, outputEncoding);
    } finally {
      if(result?.transformed && result.output !== 0) {
        this.free(result.output);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // bool mjb_codepoint_is_valid(mjb_codepoint codepoint)
  codepointIsValid(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_valid(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_graphic(mjb_codepoint codepoint)
  codepointIsGraphic(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_graphic(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_combining(mjb_codepoint codepoint)
  codepointIsCombining(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_combining(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_hangul_l(mjb_codepoint codepoint)
  // bool mjb_codepoint_is_hangul_v(mjb_codepoint codepoint)
  // bool mjb_codepoint_is_hangul_t(mjb_codepoint codepoint)
  // bool mjb_codepoint_is_hangul_jamo(mjb_codepoint codepoint)

  // bool mjb_codepoint_is_hangul_syllable(mjb_codepoint codepoint)
  codepointIsHangulSyllable(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_hangul_syllable(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_cjk_ideograph(mjb_codepoint codepoint)
  codepointIsCjkIdeograph(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_cjk_ideograph(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_cjk_ext(mjb_codepoint codepoint)
  codepointIsCjkExt(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_cjk_ext(codepoint) ? true : false;
  }

  // bool mjb_category_is_graphic(mjb_category category)
  categoryIsGraphic(category: number): boolean {
    return this.module._mjb_category_is_graphic(category) ? true : false;
  }

  // bool mjb_category_is_combining(mjb_category category)
  categoryIsCombining(category: number): boolean {
    return this.module._mjb_category_is_combining(category) ? true : false;
  }

  // mjb_status mjb_codepoint_numeric_value(mjb_codepoint codepoint, mjb_numeric_value *value)
  codepointNumericValue(codepoint: Codepoint): NumericValue | null {
    const ptr = this.malloc(24);

    try {
      const status = this.module._mjb_codepoint_numeric_value(codepoint, ptr);

      if(status !== Status.OK) {
        return null;
      }

      return this.pointerToNumericValue(ptr);
    } finally {
      this.free(ptr);
    }
  }

  // mjb_status mjb_codepoint_block(mjb_codepoint codepoint, mjb_block_info *block)
  codepointBlock(codepoint: Codepoint): BlockInfo | null {
    const ptr = this.malloc(144);

    try {
      const status = this.module._mjb_codepoint_block(codepoint, ptr);

      if(status !== Status.OK) {
        return null;
      }

      return this.pointerToBlockInfo(ptr);
    } finally {
      this.free(ptr);
    }
  }

  // mjb_codepoint mjb_codepoint_to_lowercase(mjb_codepoint codepoint)
  codepointToLowercase(codepoint: Codepoint): Codepoint {
    return this.module._mjb_codepoint_to_lowercase(codepoint);
  }

  // mjb_codepoint mjb_codepoint_to_uppercase(mjb_codepoint codepoint)
  codepointToUppercase(codepoint: Codepoint): Codepoint {
    return this.module._mjb_codepoint_to_uppercase(codepoint);
  }

  // mjb_codepoint mjb_codepoint_to_titlecase(mjb_codepoint codepoint)
  codepointToTitlecase(codepoint: Codepoint): Codepoint {
    return this.module._mjb_codepoint_to_titlecase(codepoint);
  }

  // mjb_break_type mjb_break_line(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_next_line_state *state)
  breakLine(input: MojibakeInput, options: TextInputOptions = {}): number[] {
    return this.collectBreaks(input, this.module._mjb_break_line, options);
  }

  // mjb_break_type mjb_break_word(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_next_word_state *state)
  breakWord(input: MojibakeInput, options: TextInputOptions = {}): number[] {
    return this.collectBreaks(input, this.module._mjb_break_word, options);
  }

  // size_t mjb_truncate_word(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // size_t max_segments)
  truncateWord(input: MojibakeInput, maxSegments: number,
    options: TextInputOptions = {}): number {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_truncate_word(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, maxSegments);
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // size_t mjb_truncate_word_width(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_width_context context, size_t max_columns)
  truncateWordWidth(input: MojibakeInput, context: WidthContext, maxColumns: number,
    options: TextInputOptions = {}): number {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_truncate_word_width(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, context, maxColumns);
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // mjb_break_type mjb_break_sentence(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_next_sentence_state *state)
  breakSentence(input: MojibakeInput, options: TextInputOptions = {}): number[] {
    return this.collectBreaks(input, this.module._mjb_break_sentence, options);
  }

  // mjb_break_type mjb_break_grapheme_cluster(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_next_state *state)
  breakGraphemeCluster(input: MojibakeInput, options: TextInputOptions = {}): number[] {
    return this.collectBreaks(input, this.module._mjb_break_grapheme_cluster, options);
  }

  // size_t mjb_truncate(const char *buffer, size_t byte_length, mjb_encoding encoding, size_t
  // max_graphemes)
  truncate(input: MojibakeInput, maxGraphemes: number, options: TextInputOptions = {}): number {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_truncate(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, maxGraphemes);
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // size_t mjb_truncate_width(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_width_context context, size_t max_columns)
  truncateWidth(input: MojibakeInput, context: WidthContext, maxColumns: number,
    options: TextInputOptions = {}): number {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_truncate_width(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, context, maxColumns);
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // mjb_status mjb_bidi_resolve(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_direction direction, mjb_bidi_paragraph *result)
  bidiResolve(input: MojibakeInput, direction = Direction.AUTO,
    options: TextInputOptions = {}): BidiParagraph | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const resultPtr = this.malloc(24);
    let charsPtr = 0;

    try {
      const status = this.module._mjb_bidi_resolve(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, direction, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      const paragraph = this.pointerToBidiParagraph(resultPtr);
      charsPtr = paragraph.charsPtr;

      return {
        chars: paragraph.chars,
        count: paragraph.count,
        paragraph_level: paragraph.paragraph_level,
        direction: paragraph.direction
      };
    } finally {
      if(charsPtr !== 0) {
        this.free(charsPtr);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // void mjb_bidi_free(mjb_bidi_paragraph *paragraph)
  // mjb_status mjb_bidi_reorder_line(const mjb_bidi_paragraph *paragraph, size_t line_start, size_t
  // line_end, size_t *visual_order)
  // mjb_status mjb_bidi_line_runs(const mjb_bidi_paragraph *paragraph, const size_t *visual_order,
  // size_t count, mjb_bidi_run *runs, size_t *run_count)

  // mjb_plane mjb_codepoint_plane(mjb_codepoint codepoint)
  codepointPlane(codepoint: Codepoint): number {
    return this.module._mjb_codepoint_plane(codepoint);
  }

  // bool mjb_plane_is_valid(mjb_plane plane)
  planeIsValid(plane: number): boolean {
    return this.module._mjb_plane_is_valid(plane) ? true : false;
  }

  // const char *mjb_plane_name(mjb_plane plane, bool abbreviation)
  planeName(plane: number, abbreviation = false): string | null {
    const ptr = this.module._mjb_plane_name(plane, abbreviation);

    if(ptr === 0) {
      return null;
    }

    return this.decodeString(ptr, null, Encoding.UTF_8).output;
  }

  // bool mjb_codepoint_is_id_start(mjb_codepoint codepoint)
  codepointIsIdStart(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_id_start(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_id_continue(mjb_codepoint codepoint)
  codepointIsIdContinue(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_id_continue(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_xid_start(mjb_codepoint codepoint)
  codepointIsXidStart(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_xid_start(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_xid_continue(mjb_codepoint codepoint)
  codepointIsXidContinue(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_xid_continue(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_pattern_syntax(mjb_codepoint codepoint)
  codepointIsPatternSyntax(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_pattern_syntax(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_pattern_white_space(mjb_codepoint codepoint)
  codepointIsPatternWhiteSpace(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_pattern_white_space(codepoint) ? true : false;
  }

  // bool mjb_string_is_identifier(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_identifier_profile profile)
  stringIsIdentifier(input: MojibakeInput, profile = IdentifierProfile.DEFAULT,
    options: TextInputOptions = {}): boolean {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_is_identifier(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, profile) ? true : false;
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // const char *mjb_property_name(mjb_property property)
  propertyName(property: number): string | null {
    const ptr = this.module._mjb_property_name(property);

    if(ptr === 0) {
      return null;
    }

    return this.decodeString(ptr, null, Encoding.UTF_8).output;
  }

  // mjb_status mjb_confusable_skeleton(const char *buffer, size_t byte_length,
  // mjb_encoding encoding, mjb_encoding output_encoding, mjb_result *result)
  confusableSkeleton(input: MojibakeInput, options: TextInputOptions = {}): Result | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const outputEncoding = this.resolveEncoding(options.outputEncoding ?? wasmInput.encoding);
    const resultPtr = this.malloc(12);
    let result: RawResult | null = null;

    try {
      const status = this.module._mjb_confusable_skeleton(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, outputEncoding, resultPtr);

      if(status !== Status.OK) {
        return null;
      }

      result = this.pointerToResult(resultPtr);
      return this.rawResultToResult(result, outputEncoding);
    } finally {
      if(result?.transformed && result.output !== 0) {
        this.free(result.output);
      }

      this.free(wasmInput.ptr);
      this.free(resultPtr);
    }
  }

  // bool mjb_string_is_confusable(const char *s1, size_t s1_byte_length, mjb_encoding s1_encoding,
  // const char *s2, size_t s2_byte_length, mjb_encoding s2_encoding)
  stringIsConfusable(s1: MojibakeInput, s2: MojibakeInput, options: TextInputOptions = {}): boolean {
    const wasmInput1 = this.copyInput(s1, options.encoding);
    const wasmInput2 = this.copyInput(s2, options.additionalEncoding ?? options.encoding);

    try {
      return this.module._mjb_string_is_confusable(wasmInput1.ptr, wasmInput1.size,
        wasmInput1.encoding, wasmInput2.ptr, wasmInput2.size, wasmInput2.encoding) ? true : false;
    } finally {
      this.free(wasmInput1.ptr);
      this.free(wasmInput2.ptr);
    }
  }

  // mjb_status mjb_codepoint_emoji(mjb_codepoint codepoint, mjb_emoji_properties *emoji)
  codepointEmoji(codepoint: Codepoint): EmojiProperties | null {
    const structSize = 16;
    const ptr = this.malloc(structSize);

    try {
      const status = this.module._mjb_codepoint_emoji(codepoint, ptr);

      if(status !== Status.OK) {
        return null;
      }

      return this.pointerToEmojiProperties(ptr);
    } finally {
      this.free(ptr);
    }
  }

  // bool mjb_codepoint_is_emoji(mjb_codepoint codepoint)
  codepointIsEmoji(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_emoji(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_emoji_presentation(mjb_codepoint codepoint)
  codepointIsEmojiPresentation(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_emoji_presentation(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_emoji_modifier(mjb_codepoint codepoint)
  codepointIsEmojiModifier(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_emoji_modifier(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_emoji_modifier_base(mjb_codepoint codepoint)
  codepointIsEmojiModifierBase(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_emoji_modifier_base(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_emoji_component(mjb_codepoint codepoint)
  codepointIsEmojiComponent(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_emoji_component(codepoint) ? true : false;
  }

  // bool mjb_codepoint_is_extended_pictographic(mjb_codepoint codepoint)
  codepointIsExtendedPictographic(codepoint: Codepoint): boolean {
    return this.module._mjb_codepoint_is_extended_pictographic(codepoint) ? true : false;
  }

  // mjb_status mjb_string_emoji_sequence(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_emoji_sequence *emoji)
  stringEmojiSequence(input: MojibakeInput, options: TextInputOptions = {}): EmojiSequence | null {
    const structSize = 12;
    const wasmInput = this.copyInput(input, options.encoding);
    const ptr = this.malloc(structSize);

    try {
      const status = this.module._mjb_string_emoji_sequence(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, ptr);

      if(status !== Status.OK) {
        return null;
      }

      return this.pointerToEmojiSequence(ptr);
    } finally {
      this.free(wasmInput.ptr);
      this.free(ptr);
    }
  }

  // bool mjb_string_is_emoji_sequence(const char *buffer, size_t byte_length, mjb_encoding encoding)
  stringIsEmojiSequence(input: MojibakeInput, options: TextInputOptions = {}): boolean {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_is_emoji_sequence(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding) ? true : false;
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // bool mjb_string_is_rgi_emoji(const char *buffer, size_t byte_length, mjb_encoding encoding)
  stringIsRgiEmoji(input: MojibakeInput, options: TextInputOptions = {}): boolean {
    const wasmInput = this.copyInput(input, options.encoding);

    try {
      return this.module._mjb_string_is_rgi_emoji(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding) ? true : false;
    } finally {
      this.free(wasmInput.ptr);
    }
  }

  // mjb_status mjb_hangul_syllable_name(mjb_codepoint codepoint, char *buffer, size_t byte_length)
  // mjb_status mjb_hangul_syllable_decomposition(mjb_codepoint codepoint,
  // mjb_codepoint *codepoints)
  // size_t mjb_hangul_syllable_composition(mjb_buffer_character *characters, size_t characters_len)

  // mjb_status mjb_codepoint_east_asian_width(mjb_codepoint codepoint,
  // mjb_east_asian_width *width);
  codepointEastAsianWidth(codepoint: Codepoint): number | null {
    const widthPtr = this.malloc(4);

    try {
      const status = this.module._mjb_codepoint_east_asian_width(codepoint, widthPtr);

      if(status === Status.OK) {
        return this.module.HEAP32[widthPtr / 4];
      }

      return null;
    } finally {
      this.free(widthPtr);
    }
  }

  // mjb_status mjb_display_width(const char *buffer, size_t byte_length, mjb_encoding encoding,
  // mjb_width_context context, size_t *width);
  displayWidth(input: MojibakeInput, context = WidthContext.AUTO,
    options: TextInputOptions = {}): number | null {
    const wasmInput = this.copyInput(input, options.encoding);
    const widthPtr = this.malloc(4);

    try {
      const status = this.module._mjb_display_width(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, context, widthPtr);

      if(status === Status.OK) {
        return this.module.HEAP32[widthPtr / 4];
      }

      return null;
    } finally {
      this.free(wasmInput.ptr);
      this.free(widthPtr);
    }
  }

  // mjb_status mjb_locale_parse(const char *id, size_t size, mjb_encoding encoding,
  // mjb_locale_id *locale, mjb_error *error);
  localeParse(id: MojibakeInput, options: TextInputOptions = {}): LocaleID {
    const localeStructSize = 9 + 12 + 5 + 4 + 32 + 128 + 128 + 32;
    const wasmInput = this.copyInput(id, options.encoding);
    const localePtr = this.malloc(localeStructSize);
    const errorPtr = this.malloc(4);

    try {
      const status = this.module._mjb_locale_parse(wasmInput.ptr, wasmInput.size,
        wasmInput.encoding, localePtr, errorPtr);

      if(status === Status.OK) {
        return this.pointerToLocaleId(localePtr);
      }

      throw new Error(`Failed to parse locale`);
    } finally {
      this.free(wasmInput.ptr);
      this.free(localePtr);
      this.free(errorPtr);
    }
  }

  // mjb_status mjb_locale_set(unsigned int locale);
  localeSet(locale: number): boolean {
    return this.module._mjb_locale_set(locale) === Status.OK;
  }

  // const char *mjb_version(void);
  version(): string | null {
    return this.decodeString(this.module._mjb_version(), null, Encoding.UTF_8).output;
  }

  // unsigned int mjb_version_number(void);
  versionNumber(): number {
    return this.module._mjb_version_number();
  }

  // const char *mjb_unicode_version(void);
  unicodeVersion(): string | null {
    return this.decodeString(this.module._mjb_unicode_version(), null, Encoding.UTF_8).output;
  }

  // mjb_status mjb_set_memory_functions(mjb_alloc_fn, mjb_realloc_fn, mjb_free_fn);
  // void mjb_shutdown(void);
  // void *mjb_alloc(size_t size);
  // void *mjb_realloc(void *ptr, size_t new_size);
  // void *mjb_free(void *ptr);

  private struct(ptr: Pointer, view = new DataView(this.module.HEAPU8.buffer)): StructReader {
    return new StructReader(ptr, this.module.HEAPU8, view, this.utf8Decoder);
  }

  // Create a mjb_character structure from a pointer to it in memory
  private pointerToCharacter(ptr: Pointer): Character {
    const reader = this.struct(ptr);
    const codepoint = reader.u32();
    const name = reader.str(128);
    const category = reader.u32();
    const combining = reader.u32();
    const bidirectional = reader.u16();
    const decomposition = reader.u32();
    const decimal = reader.i32();
    const digit = reader.i32();
    const numeric = reader.str(16);
    const mirrored = reader.u8();
    const uppercase = reader.u32();
    const lowercase = reader.u32();
    const titlecase = reader.u32();

    return {
      codepoint,
      name,
      category,
      combining,
      bidirectional,
      decomposition,
      decimal: decimal === -1 ? null : decimal,
      digit: digit === -1 ? null : digit,
      numeric,
      mirrored: mirrored !== 0,
      uppercase: uppercase === 0 ? null : uppercase,
      lowercase: lowercase === 0 ? null : lowercase,
      titlecase: titlecase === 0 ? null : titlecase
    };
  }

  // Create a mjb_locale_id structure from a pointer to it in memory
  private pointerToLocaleId(ptr: Pointer): LocaleID {
    const reader = this.struct(ptr);

    return {
      language: reader.str(9),
      extlang: reader.str(12),
      script: reader.str(5),
      region: reader.str(4),
      variant: reader.str(32),
      extensions: reader.str(128),
      private_use: reader.str(128),
      grandfathered: reader.str(32)
    };
  }

  // Create a mjb_numeric_value structure from a pointer to it in memory
  private pointerToNumericValue(ptr: Pointer): NumericValue {
    const reader = this.struct(ptr);
    const decimal = reader.i32();
    const digit = reader.i32();

    return {
      decimal: decimal === -1 ? null : decimal,
      digit: digit === -1 ? null : digit,
      numeric: reader.str(16)
    };
  }

  // Create a mjb_block_info structure from a pointer to it in memory
  private pointerToBlockInfo(ptr: Pointer): BlockInfo {
    const reader = this.struct(ptr);

    return {
      id: reader.u32(),
      name: reader.str(128),
      start: reader.u32(),
      end: reader.u32()
    };
  }

  // Create a mjb_bidi_paragraph structure from a pointer to it in memory
  private pointerToBidiParagraph(ptr: Pointer): BidiParagraph & { charsPtr: Pointer } {
    const view = new DataView(this.module.HEAPU8.buffer);
    const reader = this.struct(ptr, view);
    const charsPtr = reader.u32();
    const count = reader.u32();
    const paragraphLevel = reader.u8();
    const direction = reader.u32();
    const chars: BidiChar[] = new Array(count);
    const bidiCharSize = 20;

    for(let i = 0; i < count; ++i) {
      chars[i] = this.pointerToBidiChar(charsPtr + i * bidiCharSize, view);
    }

    return {
      chars,
      charsPtr,
      count,
      paragraph_level: paragraphLevel,
      direction
    };
  }

  // Create a mjb_bidi_char structure from a pointer to it in memory
  private pointerToBidiChar(ptr: Pointer, view = new DataView(this.module.HEAPU8.buffer)): BidiChar {
    const reader = this.struct(ptr, view);
    const codepoint = reader.u32();
    const byteOffset = reader.u32();
    const level = reader.u8();
    const resolvedClass = reader.u32();
    const mirroringGlyph = reader.u32();

    return {
      codepoint,
      byte_offset: byteOffset,
      level,
      resolved_class: resolvedClass,
      mirroring_glyph: mirroringGlyph === 0 ? null : mirroringGlyph
    };
  }

  // Create a mjb_result structure from a pointer to it in memory
  private pointerToResult(ptr: Pointer): RawResult {
    const reader = this.struct(ptr);

    return {
      output: reader.u32(),
      output_size: reader.u32(),
      transformed: reader.u8() !== 0
    };
  }

  private rawResultToResult(raw: RawResult, outputEncoding: Encoding): Result {
    const composed = this.decodeString(raw.output, raw.output_size, outputEncoding);

    return {
      bytes: composed.bytes,
      output: composed.output,
      outputSize: raw.output_size,
      transformed: raw.transformed
    }
  }

  // Create a mjb_emoji_properties structure from a pointer to it in memory
  private pointerToEmojiProperties(ptr: Pointer): EmojiProperties {
    const reader = this.struct(ptr);
    const codepoint = reader.u32();

    return {
      codepoint: codepoint === 0 ? null : codepoint,
      emoji: reader.u8() !== 0,
      presentation: reader.u8() !== 0,
      modifier: reader.u8() !== 0,
      modifier_base: reader.u8() !== 0,
      component: reader.u8() !== 0,
      extended_pictographic: reader.u8() !== 0
    };
  }

  // Create a mjb_emoji_sequence structure from a pointer to it in memory
  private pointerToEmojiSequence(ptr: Pointer): EmojiSequence {
    const reader = this.struct(ptr);

    return {
      type: reader.u32(),
      qualification: reader.u32(),
      codepoint_count: reader.u32()
    };
  }

  // Decode a string from a pointer in memory
  private decodeString(buffer: Pointer, size: number | null, encoding: Encoding): ComposedString {
    encoding = this.resolveEncoding(encoding);
    const ret: ComposedString = { bytes: null, output: '' };

    if(size === null) {
      size = this.nullTerminatedByteLength(buffer);

      if(size === 0) {
        return ret;
      }
    }

    const bytes = this.module.HEAPU8.subarray(buffer, buffer + size);
    ret.bytes = bytes;

    if(encoding == Encoding.ASCII || encoding == Encoding.UTF_8) {
      ret.output = this.utf8Decoder.decode(bytes);
    } else if(encoding == Encoding.UTF_16BE) {
      ret.output = this.utf16beDecoder.decode(bytes);
    } else if(encoding == Encoding.UTF_16LE) {
      ret.output = this.utf16leDecoder.decode(bytes);
    } else if(encoding == Encoding.UTF_32BE || encoding == Encoding.UTF_32LE) {
      // Javascript don't have a built-in UTF-32 decoder, so we need to manually decode it.
      let str = '';

      for(let i = 0; i + 3 < size; i += 4) {
        let codepoint = 0;

        if(encoding == Encoding.UTF_32BE) {
          codepoint = (this.module.HEAPU8[buffer + i] << 24) |
            (this.module.HEAPU8[buffer + i + 1] << 16) |
            (this.module.HEAPU8[buffer + i + 2] << 8) |
            this.module.HEAPU8[buffer + i + 3];
        } else {
          codepoint = this.module.HEAPU8[buffer + i] |
            (this.module.HEAPU8[buffer + i + 1] << 8) |
            (this.module.HEAPU8[buffer + i + 2] << 16) |
            (this.module.HEAPU8[buffer + i + 3] << 24);
        }

        // Coerce the codepoint into an unsigned 32-bit integer.
        str += String.fromCodePoint(codepoint >>> 0);
      }

      ret.output = str;
    }

    return ret;
  }

  private nullTerminatedByteLength(buffer: Pointer): number {
    let end = buffer;

    while(this.module.HEAPU8[end] !== 0) {
      ++end;
    }

    return end - buffer;
  }

  private collectBreaks(input: MojibakeInput, fn: BreakFunction,
    options: TextInputOptions = {}): number[] {
    const wasmInput = this.copyInput(input, options.encoding);
    const statePtr = this.malloc(256);
    const breaks: number[] = [];

    try {
      for(;;) {
        const type = fn(wasmInput.ptr, wasmInput.size, wasmInput.encoding, statePtr);

        if(type === BreakType.NOT_SET) {
          break;
        }

        breaks.push(type);
      }

      return breaks;
    } finally {
      this.free(wasmInput.ptr);
      this.free(statePtr);
    }
  }

  private copyInput(input: MojibakeInput, encoding: Encoding = Encoding.UTF_8): WasmInput {
    if(typeof encoding !== 'number') {
      encoding = Encoding.UTF_8;
    }

    encoding = this.resolveEncoding(encoding);
    let bytes: Uint8Array;

    if(typeof input === 'string') {
      bytes = this.encodeString(input, encoding);
    } else if(ArrayBuffer.isView(input)) {
      bytes = new Uint8Array(input.buffer, input.byteOffset, input.byteLength);
    } else {
      bytes = new Uint8Array(input);
    }

    return {
      ptr: this.memcpy(bytes),
      size: bytes.length,
      encoding
    };
  }

  private encodeString(str: string, encoding = Encoding.UTF_8): Uint8Array {
    encoding = this.resolveEncoding(encoding);
    this.assertWellFormedString(str);

    if(encoding == Encoding.ASCII) {
      const bytes = new Uint8Array(str.length);

      for(let i = 0; i < str.length; ++i) {
        const codeUnit = str.charCodeAt(i);

        if(codeUnit > 0x7F) {
          throw new Error('Cannot encode non-ASCII codepoint as ASCII');
        }

        bytes[i] = codeUnit;
      }

      return bytes;
    } else if(encoding == Encoding.UTF_8) {
      return this.utf8Encoder.encode(str);
    } else if(encoding == Encoding.UTF_16BE || encoding == Encoding.UTF_16LE) {
      const bytes = new Uint8Array(str.length * 2);
      const view = new DataView(bytes.buffer);
      const littleEndian = encoding == Encoding.UTF_16LE;

      for(let i = 0; i < str.length; ++i) {
        view.setUint16(i * 2, str.charCodeAt(i), littleEndian);
      }

      return bytes;
    } else if(encoding == Encoding.UTF_32BE || encoding == Encoding.UTF_32LE) {
      const bytes = new Uint8Array(str.length * 4);
      const view = new DataView(bytes.buffer);
      const littleEndian = encoding == Encoding.UTF_32LE;
      let offset = 0;

      for(const char of str) {
        const codepoint = char.codePointAt(0) as number;

        view.setUint32(offset, codepoint, littleEndian);
        offset += 4;
      }

      return bytes.subarray(0, offset);
    }

    throw new Error(`Unsupported text encoding: ${encoding}`);
  }

  private assertWellFormedString(str: string): void {
    for(let i = 0; i < str.length; ++i) {
      const codeUnit = str.charCodeAt(i);

      if(codeUnit >= 0xD800 && codeUnit <= 0xDBFF) {
        const next = str.charCodeAt(i + 1);

        if(i + 1 >= str.length || next < 0xDC00 || next > 0xDFFF) {
          throw new Error('Cannot encode malformed string: unpaired high surrogate');
        }

        ++i;
      } else if(codeUnit >= 0xDC00 && codeUnit <= 0xDFFF) {
        throw new Error('Cannot encode malformed string: unpaired low surrogate');
      }
    }
  }

  private resolveEncoding(encoding: Encoding): Encoding {
    if((encoding & Encoding.UTF_32LE) === Encoding.UTF_32LE) {
      return Encoding.UTF_32LE;
    }

    if((encoding & Encoding.UTF_32BE) === Encoding.UTF_32BE) {
      return Encoding.UTF_32BE;
    }

    if(encoding === Encoding.UTF_32) {
      return Encoding.UTF_32LE;
    }

    if((encoding & Encoding.UTF_16LE) === Encoding.UTF_16LE) {
      return Encoding.UTF_16LE;
    }

    if((encoding & Encoding.UTF_16BE) === Encoding.UTF_16BE) {
      return Encoding.UTF_16BE;
    }

    if(encoding === Encoding.UTF_16) {
      return Encoding.UTF_16LE;
    }

    if((encoding & Encoding.UTF_8) === Encoding.UTF_8) {
      return Encoding.UTF_8;
    }

    if((encoding & Encoding.ASCII) === Encoding.ASCII) {
      return Encoding.ASCII;
    }

    return encoding;
  }

  private malloc(size: number, zero = true): Pointer {
    const ptr = this.module._malloc(size);

    if(zero) {
      this.module.HEAPU8.fill(0, ptr, ptr + size);
    }

    return ptr;
  }

  private memcpy(bytes: Uint8Array): number {
    const ptr = this.malloc(bytes.length + 4, false);
    const heap = this.module.HEAPU8;

    heap.set(bytes, ptr);
    heap.fill(0, ptr + bytes.length, ptr + bytes.length + 4);

    return ptr;
  }

  private free(ptr: Pointer): void {
    this.module._free(ptr);
  }
}

export function createMojibake(options?: MojibakeModuleOptions): Promise<Mojibake> {
  return Mojibake.create(options);
}

export default createMojibake;
