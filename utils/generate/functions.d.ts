/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

export type MojibakeFunction = {
  comment: string;
  ret: string;
  name: string;
  attributes: string[];
  args: {
    name: string;
    type: string;
    description: string;
    wasm_generated: boolean;
  }[];
  wasm: boolean;
};

declare const functions: MojibakeFunction[];
export default functions;
