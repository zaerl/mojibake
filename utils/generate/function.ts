import { readFileSync } from 'fs';
import path from 'path';
import { categories } from './types';

export class CFunction {
  constructor(
    private comment: string,
    private ret: string,
    private name: string,
    private attributes: string[] = [],
    private args: string[] = [],
    private argsTypes: string[] = [],
    private argsDescription: string[] = [],
    private argsReturn: boolean[] = [],
    private wasm: boolean = false
  ) {
    if(!this.ret.endsWith('*')) {
      this.ret += ' ';
    }
  }

  isWASM() {
    return this.wasm;
  }

  getArgs(): string[] {
    const args = this.args.map((arg, index) => `${this.argsTypes[index]}${this.argsTypes[index].endsWith('*') ? '' : ' '}${arg}`);

    return args.length ? args : ['void'];
  }

  getName() {
    return 'mjb_' + this.name;
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
      ret: this.ret,
      name: this.getName(),
      args: this.getArgs()
    });
  }

  formatWASM(): string {
    return `\\"_${this.getName()}\\"`;
  }

  formatHTML(): string {
    return `<section id="${this.getName()}"><div><code id="${this.getName()}-function" class="is-full-width" onclick="toggleFunctionCall('${this.getName()}')">` +
      `${this.ret}<span class="text-primary">${this.getName()}</span>(${this.getArgs().join(', ')});</code></div><div class="function-card bd-light">` +
      this.#formInputHTML() +
      `<div><a class="button primary" onclick="callFunction('${this.getName()}')">Call function</a></div><div id="${this.getName()}-results" class="function-results"></div></div></section>`;
  }

  #getDescription(description: string, disabled = false): string {
    return disabled ? `${description} (automatically generated)` : description;
  }

  #getInput(arg: number, type = 'text'): string {
    const disabled = this.argsReturn[arg];
    let ret = `<p><label for="${this.getName()}-${this.args[arg]}" class="${disabled ? 'text-light' : ''}">${this.args[arg]}</label>`;
    ret += `<input type="text" id="${this.getName()}-${this.args[arg]}" placeholder="${this.#getDescription(this.argsDescription[arg], this.argsReturn[arg])}" ${disabled ? 'disabled' : ''}>`;

    return ret + '</p>';
  }

  #getNumberInput(arg: number): string {
    return this.#getInput(arg, 'number');
  }

  #getSelectInput(arg: number, options: string[], values: number[]|null = null): string {
    const disabled = this.argsReturn[arg];
    let ret = `<p><label for="${this.getName()}-${this.args[arg]}" class="${disabled ? 'text-light ' : ''}">${this.args[arg]}</label>`;
    ret += `<select id="${this.getName()}-${this.args[arg]}" placeholder="${this.#getDescription(this.argsDescription[arg], disabled)}" ${disabled ? 'disabled' : ''}>`;
    let i = 0;

    for(const option of options) {
      ret += `<option value="${values ? values[i] : i}">${option}</option>`;
      ++i;
    }

    ret += '</select>';

    return ret + '</select></p>';
  }

  #formInputHTML(): string {
    console.log(this.args);
    if(!this.args.length) {
      return '';
    }

    let ret = `<form id="${this.getName()}-form" class="function-form">`;
    let i = 0;

    for(const arg of this.argsTypes) {
      if(arg.startsWith('const char *')) {
        ret += this.#getInput(i);
      } else if(arg.startsWith('size_t *')) {
        ret += this.#getInput(i);
      } else if(arg.startsWith('mjb_character *')) {
        ret += this.#getInput(i);
      } else if(arg.startsWith('size_t')) {
        ret += this.#getNumberInput(i);
      } else if(arg.startsWith('mjb_codepoint')) {
        ret += this.#getNumberInput(i);
      } else if(arg.startsWith('mjb_category')) {
        ret += this.#getSelectInput(i, categories, null);
      } else if(arg.startsWith('mjb_encoding')) {
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
          0x20,
          0x80,
        ];

        ret += this.#getSelectInput(i, options, values);
      } else if(arg.startsWith('mjb_case_type')) {
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
      } else if(arg.startsWith('mjb_normalization')) {
        // See mjb_normalization on mojibake.h
        const options = [
          'MJB_NORMALIZATION_NFC',
          'MJB_NORMALIZATION_NFD',
          'MJB_NORMALIZATION_NFKC',
          'MJB_NORMALIZATION_NFKD',
        ];

        ret += this.#getSelectInput(i, options);
      }

      ++i;
    }

    ret += '</form>';

    return ret;
  }
}

export function cfns(): CFunction[] {
  const jsonPath = path.join(__dirname, 'functions.json');
  const jsonData = readFileSync(jsonPath, 'utf8');
  const data: any[] = JSON.parse(jsonData);

  return data.map(item => new CFunction(
    item.comment,
    item.ret,
    item.name,
    item.attributes,
    item.args,
    item.args_types,
    item.args_description,
    item.args_return,
    item.wasm
  ));
}
