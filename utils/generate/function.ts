/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import { readFileSync } from 'fs';
import path from 'path';
import { categories } from './types';
// @ts-expect-error - functions.js is a JavaScript file without type declarations
import functionsDefault from './functions.js';

export const functions: Function[] = functionsDefault;

export type FunctionArg = {
  name: string;
  type: string;
  description: string;
  wasm_generated: boolean;
};

export type Function = {
  comment: string;
  ret: string;
  name: string;
  attributes: string[];
  args: FunctionArg[];
  wasm: boolean;
};

export class CFunction implements Function {
  constructor(
    public comment: string,
    public ret: string,
    public name: string,
    public attributes: string[] = [],
    public args: FunctionArg[] = [],
    public wasm: boolean = false
  ) {
    if(!this.ret.endsWith('*')) {
      this.ret += ' ';
    }
  }

  isWASM() {
    return this.wasm;
  }

  getArgs(): string[] {
    const args = this.args.map((arg, index) => `${arg.type}${arg.type.endsWith('*') ? '' : ' '}${arg.name}`);

    return args.length ? args : ['void'];
  }

  getName() {
    return 'mjb_' + this.name;
  }

  getLabelName(arg: number) {
    return this.args[arg].name.replace('_', ' ');
  }

  isInternal() {
    return this.name.startsWith('initialize');
  }

  formatC(): string {
    const attributes = this.attributes.length ? `${this.attributes.join(' ')} ` : '';
    return `// ${this.comment}\n${attributes}${this.ret}${this.getName()}(${this.getArgs().join(', ')});`;
  }

  formatMD(): string {
    return `${this.comment}\n\n\`\`\`c\n${this.ret}${this.getName()}(${this.getArgs().join(', ')});\n\`\`\``;
  }

  formatJSON(): string {
    return JSON.stringify({
      comment: this.comment,
      ret: this.ret.trimEnd(),
      name: this.getName(),
      args: this.args,
      wasm: this.wasm,
    });
  }

  formatWASM(): string {
    return `\\"_${this.getName()}\\"`;
  }

  formatHTML(): string {
    return `<section id="${this.getName()}">
      <div class="code function-call" id="${this.getName()}-function" onclick="toggleFunctionCall('${this.getName()}')">
        <div class="function-call-comment">// ${this.comment}</div>
        <div>${this.ret}<span class="text-primary">${this.getName()}</span>(${this.getArgs().join(', ')});</div>
      </div>
      <div class="function-card">
        <div>${this.#formInputHTML()}</div>
        <div id="${this.getName()}-results" class="function-results code"></div>
      </div>
    </section>`;
  }

  #getDescription(arg: number): string {
    let ret = this.args[arg].wasm_generated ? `${this.args[arg].description} (automatically generated)` : this.args[arg].description;

    if(this.args[arg].type === 'mjb_codepoint') {
      ret += ' (U+XXXXX, 0xXXXXX or XXXXX hexadecimal format)';
    }

    return ret;
  }

  #getInput(arg: number, type = 'text'): string {
    const disabled = this.args[arg].wasm_generated;
    const name = `${this.getName()}-${this.args[arg].name}`;
    const description = this.#getDescription(arg);

    let ret = `<div><label for="${name}"${disabled ? ' class="text-secondary"' : ''}>${this.getLabelName(arg)}</label>`;
    ret += `<input id="${name}" type="${type}" name="${name}" placeholder="${description}" ${disabled ? 'disabled' : ''}>`;

    return ret + '</div>';
  }

  #getCheckbox(arg: number): string {
    const disabled = this.args[arg].wasm_generated;
    const name = `${this.getName()}-${this.args[arg].name}`;
    const description = this.#getDescription(arg);

    let ret = `<div><input id="${name}" type="checkbox" name="${name}" ${disabled ? 'disabled' : ''}>` +
      `<label for="${name}"${disabled ? ' class="text-secondary"' : ''}>${this.getLabelName(arg)}</label>`;

    return ret + '</div>';
  }

  #getSelectInput(arg: number, options: string[], values: number[]|null = null): string {
    const disabled = this.args[arg].wasm_generated;
    const name = `${this.getName()}-${this.args[arg].name}`;
    const description = this.#getDescription(arg);

    let ret = `<div><label for="${name}"${disabled ? ' class="text-secondary"' : ''}>${this.getLabelName(arg)}</label>`;
    ret += `<select id="${name}" name="${name}" placeholder="${description}" ${disabled ? 'disabled' : ''}>`;
    let i = 0;

    for(const option of options) {
      ret += `<option value="${values ? values[i] : i}">${option}</option>`;
      ++i;
    }

    return ret + '</select></div>';
  }

  #formInputHTML(): string {
    let ret = `\n          <form id="${this.getName()}-form" class="function-form" onsubmit="return false;">`;
    let i = 0;

    for(const arg of this.args) {
      ret += `\n            `;
      const type = arg.type;
      if(type.startsWith('const char *')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('char *')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('size_t *')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('mjb_character *')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('mjb_emoji_properties *')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('size_t')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('mjb_codepoint')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('bool')) {
        ret += this.#getCheckbox(i);
      } else if(type.startsWith('mjb_category')) {
        ret += this.#getSelectInput(i, categories, null);
      } else if(type.startsWith('mjb_encoding')) {
        const options = [
          'MJB_ENCODING_UTF_8',
          'MJB_ENCODING_UTF_16_BE',
          'MJB_ENCODING_UTF_16_LE',
          'MJB_ENCODING_UTF_32_BE',
          'MJB_ENCODING_UTF_32_LE',
        ];

        // See mjb_encoding on mojibake.h
        const values = [
          0x2,
          0x8,
          0x10,
          0x40,
          0x80,
        ];

        ret += this.#getSelectInput(i, options, values);
      } else if(type.startsWith('mjb_case_type')) {
        // See mjb_case_type on mojibake.h
        const options = [
          'MJB_CASE_UPPER',
          'MJB_CASE_LOWER',
          'MJB_CASE_TITLE',
          'MJB_CASE_CASEFOLD',
        ];

        // See mjb_encoding on mojibake.h
        const values = [
          1,
          2,
          3,
          4,
        ];

        ret += this.#getSelectInput(i, options, values);
      } else if(type.startsWith('mjb_result')) {
        ret += this.#getInput(i);
      } else if(type.startsWith('mjb_normalization')) {
        // See mjb_normalization on mojibake.h
        const options = [
          'MJB_NORMALIZATION_NFC',
          'MJB_NORMALIZATION_NFD',
          'MJB_NORMALIZATION_NFKC',
          'MJB_NORMALIZATION_NFKD',
        ];

        ret += this.#getSelectInput(i, options);
      } else if(type.startsWith('mjb_plane')) {
        // See mjb_plane on unicode.h
        const options = [
          'MJB_PLANE_BMP',
          'MJB_PLANE_SMP',
          'MJB_PLANE_SIP',
          'MJB_PLANE_TIP',
          'MJB_PLANE_SSP',
          'MJB_PLANE_PUA_A',
          'MJB_PLANE_PUA_B',
        ];

        // See mjb_plane on unicode.h
        const values = [
          0,
          1,
          2,
          3,
          4,
          5,
          16,
        ];

        ret += this.#getSelectInput(i, options, values);
      }

      ++i;
    }

    ret += `\n            <div class="function-form-button">
              <button type="submit" onclick="callFunction('${this.getName()}')">Call function</button>
            </div>\n          </form>\n`;

    return ret;
  }
}

export function cfns(): CFunction[] {
  return functions.map(item => new CFunction(
    item.comment,
    item.ret,
    item.name,
    item.attributes,
    item.args,
    item.wasm
  ));
}
