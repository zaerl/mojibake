/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

let verbose = false;

export function setVerbose(value: boolean) {
  verbose = value;
}

export function isVerbose() {
  return verbose;
}

export function log(message: string, ...optionalParams: any[]) {
  if(verbose) {
    console.log(message, ...optionalParams);
  }
}

export function iLog(message: string, ...optionalParams: any[]) {
  console.log(message, ...optionalParams);
}
