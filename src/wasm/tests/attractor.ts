/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

// This is the javascript implementation of the Attractor test suite.
let att_valid_tests = 0;
let att_total_tests = 0;

let att_verbose = 1;
let att_show_error = true;
let att_show_colors = false;

export function att_get_valid_tests(): number {
  return att_valid_tests;
}

export function att_get_total_tests(): number {
  return att_total_tests;
}

export function att_set_verbose(verbose: number): void {
  att_verbose = verbose;
}

export function att_set_show_error(showError: boolean): void {
  att_show_error = showError;
}

export function att_set_show_colors(showColors: boolean): void {
  att_show_colors = showColors;
}

function att_type(value: unknown): string {
  if(value === null) {
    return "null";
  }

  if(Array.isArray(value)) {
    return "array";
  }

  if(ArrayBuffer.isView(value)) {
    return value.constructor.name;
  }

  if(is_buffer(value)) {
    return value.constructor.name;
  }

  return typeof value;
}

function att_format(value: unknown): string {
  if(typeof value === "string") {
    return JSON.stringify(value);
  }

  if(typeof value !== "object" || value === null) {
    return String(value);
  }

  if(ArrayBuffer.isView(value)) {
    return `${value.constructor.name} ${att_format_bytes(view_bytes(value))}`;
  }

  if(is_buffer(value)) {
    return `${value.constructor.name} ${att_format_bytes(new Uint8Array(value))}`;
  }

  try {
    return JSON.stringify(value);
  } catch {
    return String(value);
  }
}

function att_format_bytes(bytes: ArrayLike<number>): string {
  return `[${Array.from(bytes).join(", ")}]`;
}

function is_buffer(value: unknown): value is ArrayBufferLike {
  return value instanceof ArrayBuffer ||
    (typeof SharedArrayBuffer !== "undefined" && value instanceof SharedArrayBuffer);
}

function is_plain_object(value: unknown): value is Record<string, unknown> {
  if(typeof value !== "object" || value === null) {
    return false;
  }

  const prototype = Object.getPrototypeOf(value);

  return prototype === Object.prototype || prototype === null;
}

function view_bytes(value: ArrayBufferView): Uint8Array {
  return new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
}

function bytes_equal(value: ArrayLike<number>, expected: ArrayLike<number>): boolean {
  if(value.length !== expected.length) {
    return false;
  }

  for(let i = 0; i < value.length; ++i) {
    if(value[i] !== expected[i]) {
      return false;
    }
  }

  return true;
}

function att_equal(value: unknown, expected: unknown): boolean {
  if(value === expected) {
    return true;
  }

  if(ArrayBuffer.isView(value) && ArrayBuffer.isView(expected)) {
    return bytes_equal(view_bytes(value), view_bytes(expected));
  }

  if(ArrayBuffer.isView(value) && Array.isArray(expected)) {
    return bytes_equal(view_bytes(value), expected);
  }

  if(Array.isArray(value) && ArrayBuffer.isView(expected)) {
    return bytes_equal(value, view_bytes(expected));
  }

  if(is_buffer(value) && is_buffer(expected)) {
    return bytes_equal(new Uint8Array(value), new Uint8Array(expected));
  }

  if(is_buffer(value) && Array.isArray(expected)) {
    return bytes_equal(new Uint8Array(value), expected);
  }

  if(Array.isArray(value) && is_buffer(expected)) {
    return bytes_equal(value, new Uint8Array(expected));
  }

  if(Array.isArray(value) && Array.isArray(expected)) {
    if(value.length !== expected.length) {
      return false;
    }

    for(let i = 0; i < value.length; ++i) {
      if(!att_equal(value[i], expected[i])) {
        return false;
      }
    }

    return true;
  }

  if(is_plain_object(value) && is_plain_object(expected)) {
    const valueKeys = Object.keys(value).sort();
    const expectedKeys = Object.keys(expected).sort();

    if(!att_equal(valueKeys, expectedKeys)) {
      return false;
    }

    for(const key of valueKeys) {
      if(!att_equal(value[key], expected[key])) {
        return false;
      }
    }

    return true;
  }

  return false;
}

function att_assert(format: string, test: boolean, description: string): boolean {
  if(((att_total_tests)++) === 0) {
    // Start of the test suite
  }

  if(test) {
    ++att_valid_tests;
  }

  if(att_verbose === 0) {
    // Do nothing
  } else if(att_verbose === 1) {
    process.stdout.write(test ? "." : (att_show_colors ? "\x1B[31mF\x1B[0m" : "F"));

    if(!test) {
      console.log("");
    }
  } else {
    const ok = att_show_colors ? "\x1B[32mOK\x1B[0m" : "OK";
    const fail = att_show_colors ? "\x1B[31mNO\x1B[0m" : "NO";

    console.log(
      att_show_colors ? `${test ? ok : fail} [\x1b[36m${format}\x1b[0m] ${description}` :
        `${test ? ok : fail} [${format}] ${description}`
    );
  }

  return test;
}

export function ATT_ASSERT(value: unknown, expected: unknown, message: string): boolean {
  const test = att_equal(value, expected);

  const result = att_assert(att_type(value), test, message);

  if(!result && att_show_error) {
    console.log(`${message}: Expected ${att_format(expected)}, got ${att_format(value)}`);
  }

  return result;
}
