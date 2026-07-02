/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

let att_valid_tests = 0;
let att_total_tests = 0;

let att_verbose = 1;
let att_show_colors = 1;

export function att_get_valid_tests(): number {
    return att_valid_tests;
}

export function att_get_total_tests(): number {
    return att_total_tests;
}

export function att_set_verbose(verbose: number): void {
    att_verbose = verbose;
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

    console.log(att_show_colors ? `${test ? ok : fail} [\x1b[36m${format}\x1b[0m] ${description}` :
      `${test ? ok : fail} [${format}] ${description}`);
  }

  return test;
}

export function ATT_ASSERT(value: any, expected: any, message: string): boolean {
  return att_assert(typeof value, value === expected, message);
}
