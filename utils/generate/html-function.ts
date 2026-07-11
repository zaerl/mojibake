/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import functions, {
  MojibakeArg, MojibakeFunction, MojibakeReturnCase, MojibakeSpecRef, Section
} from './functions';
import { categories } from './types';
import hljs from 'highlight.js/lib/core';
import c from 'highlight.js/lib/languages/c';

const mojibakeTypes = new Set<string>();

for(const fn of functions) {
  for(const type of [fn.ret, ...fn.args.map(arg => arg.type)]) {
    for(const match of type.matchAll(/\bmjb_[a-z0-9_]+\b/g)) {
      mojibakeTypes.add(match[0]);
    }
  }
}

hljs.registerLanguage('c', api => {
  const language = c(api);
  const keywords = language.keywords as Record<string, string[]>;

  keywords.type.push(...mojibakeTypes);

  return language;
});

export class CFunction implements MojibakeFunction {
  public comment: string;
  public ret: string;
  public name: string;
  public attributes: string[];
  public args: MojibakeArg[];
  public wasm: boolean;
  public section: Section;
  public details?: string;
  public returns?: MojibakeReturnCase[];
  public example?: string;
  public related?: string[];
  public specs?: MojibakeSpecRef[];

  constructor(fn: MojibakeFunction) {
    this.comment = fn.comment;
    this.ret = fn.ret;
    this.name = fn.name;
    this.attributes = fn.attributes;
    this.args = fn.args;
    this.wasm = fn.wasm;
    this.section = fn.section;
    this.details = fn.details;
    this.returns = fn.returns;
    this.example = fn.example;
    this.related = fn.related;
    this.specs = fn.specs;

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
    return this.name;
  }

  getSection() {
    return this.section;
  }

  getLabelName(arg: number) {
    return this.args[arg].name.replace('_', ' ');
  }

  isInternal() {
    return this.name.startsWith('initialize');
  }

  formatC(): string {
    const attributes = ['MJB_EXPORT', ...this.attributes].join(' ');
    return `// ${this.comment}\n${attributes} ${this.ret}${this.getName()}(${this.getArgs().join(', ')});`;
  }

  formatMD(): string {
    let ret = `## \`${this.getName()}\`\n\n${this.comment}\n\n`;
    ret += `\`\`\`c\n${this.ret}${this.getName()}(${this.getArgs().join(', ')});\n\`\`\``;

    if(this.details) {
      ret += `\n\n${this.details}`;
    }

    if(this.args.length) {
      ret += '\n\n' + this.args.map(arg => {
        let line = `- \`${arg.name}\` — ${arg.description}`;

        if(arg.ownership) {
          line += `. ${arg.ownership}`;
        }

        return line;
      }).join('\n');
    }

    if(this.returns?.length) {
      ret += '\n\n**Returns**\n\n' + this.returns.map(
        value => `- \`${value.value}\` — ${value.description}`
      ).join('\n');
    }

    if(this.example) {
      ret += `\n\n**Example**\n\n\`\`\`c\n${this.example}\n\`\`\``;
    }

    if(this.related?.length) {
      ret += '\n\nSee also: ' + this.related.map(
        name => `[\`${name}\`](#${name})`
      ).join(', ') + '.';
    }

    if(this.specs?.length) {
      ret += '\n\nSpecifications: ' + this.specs.map(
        spec => `[${spec.name}](${spec.url})`
      ).join(', ') + '.';
    }

    return ret;
  }

  formatJSON(): string {
    return JSON.stringify({
      comment: this.comment,
      ret: this.ret.trimEnd(),
      name: this.getName(),
      args: this.args,
      wasm: this.wasm,
      section: this.section,
      details: this.details,
      returns: this.returns,
      example: this.example,
      related: this.related,
      specs: this.specs,
    });
  }

  formatWASM(): string {
    return `\\"_${this.getName()}\\"`;
  }

  formatHTML(relatedLinkTargets = new Set<string>()): string {
    const args = this.getArgs();
    const fn = args.length === 1 && args[0] === 'void' ?
      `${this.ret}${this.getName()}(void);` :
      `${this.ret}${this.getName()}(\n    ${this.getArgs().join(',\n    ')}\n);`;

    const searchText = CFunction.escapeHTML(`${this.getName()} ${this.comment}`.toLowerCase());

    return `<article class="function-reference" id="${this.getName()}" data-function-reference ` +
      `data-search="${searchText}">
      <h3 class="function-name"><a href="#${this.getName()}" ` +
      `aria-label="Link to ${this.getName()}">${this.getName()}</a></h3>
      <p class="function-call-comment">${this.comment}</p>
      <div class="function-call" id="${this.getName()}-function">
        <pre><code class="hljs language-c">${hljs.highlight(fn, { language: 'c' }).value}</code></pre>
        <button type="button" class="function-toggle" id="${this.getName()}-toggle"
          aria-expanded="false" aria-controls="${this.getName()}-card"
          aria-label="Show details and WASM form for ${this.getName()}"
          onclick="toggleFunctionCall('${this.getName()}')"></button>
      </div>
      <div class="function-card" id="${this.getName()}-card">
        ${this.documentationHTML(relatedLinkTargets)}
        <div>${this.isWASM() ? this.formInputHTML() : '' }</div>
        <div id="${this.getName()}-results" class="function-results code"></div>
      </div>
    </article>`;
  }

  private static escapeHTML(value: string): string {
    return value.replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  private static formatInlineText(value: string): string {
    return CFunction.escapeHTML(value).replace(/`([^`]+)`/g, '<code>$1</code>');
  }

  private documentationHTML(relatedLinkTargets: Set<string>): string {
    let ret = '';

    if(this.details) {
      ret += `\n          <p>${CFunction.formatInlineText(this.details)}</p>`;
    }

    if(this.returns?.length) {
      ret += '\n          <h3>Returns</h3>\n          <ul>';

      for(const returnCase of this.returns) {
        ret += `\n            <li><code>${CFunction.escapeHTML(returnCase.value)}</code> — ` +
          `${CFunction.formatInlineText(returnCase.description)}</li>`;
      }

      ret += '\n          </ul>';
    }

    if(this.example) {
      ret += '\n          <h3>Example</h3>' +
        `\n          <pre><code class="hljs language-c">${
          hljs.highlight(this.example, { language: 'c' }).value
        }</code></pre>`;
    }

    if(this.related?.length) {
      ret += '\n          <h3>Related</h3>\n          <ul>';

      for(const name of this.related) {
        const escapedName = CFunction.escapeHTML(name);

        if(relatedLinkTargets.has(name)) {
          ret += `\n            <li><a href="#${escapedName}"><code>${escapedName}</code></a></li>`;
        } else {
          ret += `\n            <li><code>${escapedName}</code></li>`;
        }
      }

      ret += '\n          </ul>';
    }

    if(this.specs?.length) {
      ret += '\n          <h3>Specifications</h3>\n          <ul>';

      for(const spec of this.specs) {
        ret += `\n            <li><a href="${CFunction.escapeHTML(spec.url)}" ` +
          `target="_blank" rel="noopener">${CFunction.escapeHTML(spec.name)}</a></li>`;
      }

      ret += '\n          </ul>';
    }

    if(!ret) {
      return '';
    }

    return `<div class="function-docs">${ret}\n        </div>`;
  }

  private getDescription(arg: number): string {
    let ret = this.args[arg].wasm_generated ? `${this.args[arg].description} (automatically generated)` : this.args[arg].description;

    if(this.args[arg].type === 'mjb_codepoint') {
      ret += ' (U+XXXXX, 0xXXXXX or XXXXX hexadecimal format)';
    }

    return ret;
  }

  private getInput(arg: number, type = 'text'): string {
    const disabled = this.args[arg].wasm_generated;
    const name = `${this.getName()}-${this.args[arg].name}`;
    const description = this.getDescription(arg);

    let ret = `<div><label for="${name}"${disabled ? ' class="text-secondary"' : ''}>${this.getLabelName(arg)}</label>`;
    ret += `<input id="${name}" type="${type}" name="${name}" placeholder="${description}" ${disabled ? 'disabled' : ''}>`;

    return ret + '</div>';
  }

  private getCheckbox(arg: number): string {
    const disabled = this.args[arg].wasm_generated;
    const name = `${this.getName()}-${this.args[arg].name}`;
    const description = this.getDescription(arg);

    let ret = `<div><input id="${name}" type="checkbox" name="${name}" ${disabled ? 'disabled' : ''}>` +
      `<label for="${name}"${disabled ? ' class="text-secondary"' : ''}>${this.getLabelName(arg)}</label>`;

    return ret + '</div>';
  }

  private getSelectInput(arg: number, options: string[], values: number[]|null = null): string {
    const disabled = this.args[arg].wasm_generated;
    const name = `${this.getName()}-${this.args[arg].name}`;
    const description = this.getDescription(arg);

    let ret = `<div><label for="${name}"${disabled ? ' class="text-secondary"' : ''}>${this.getLabelName(arg)}</label>`;
    ret += `<select id="${name}" name="${name}" title="${description}" ${disabled ? 'disabled' : ''}>`;
    let i = 0;

    for(const option of options) {
      ret += `<option value="${values ? values[i] : i}">${option}</option>`;
      ++i;
    }

    return ret + '</select></div>';
  }

  private getBitfieldInput(arg: number, options: string[], values: number[], message = ''): string {
    const disabled = this.args[arg].wasm_generated;
    const name = `${this.getName()}-${this.args[arg].name}`;
    const description = this.getDescription(arg);
    let ret = `<fieldset class="function-bitfield"><legend${disabled ? ' class="text-secondary"' : ''}>` +
      `${this.getLabelName(arg)}</legend><span class="function-bitfield-hint">${description}. ` +
      `${message}</span><div class="function-bitfield-options">`;

    for(let i = 0; i < options.length; ++i) {
      const id = `${name}-${i}`;
      ret += `<div><input id="${id}" type="checkbox" name="${name}" value="${values[i]}" ` +
        `${disabled ? 'disabled' : ''}><label for="${id}"${disabled ? ' class="text-secondary"' : ''}>` +
        `${options[i]}</label></div>`;
    }

    return ret + '</div></fieldset>';
  }

  private formInputHTML(): string {
    let ret = `\n          <form id="${this.getName()}-wasm-form" class="function-form" ` +
      'onsubmit="return false;">';
    let i = 0;

    for(const arg of this.args) {
      ret += `\n            `;
      const type = arg.type;
      if(type.startsWith('const char *')) {
        ret += this.getInput(i);
      } else if(type.startsWith('char *')) {
        ret += this.getInput(i);
      } else if(type.startsWith('size_t *')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_character *')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_emoji_properties *')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_emoji_sequence *')) {
        ret += this.getInput(i);
      } else if(type.startsWith('size_t')) {
        ret += this.getInput(i);
      } else if(type.startsWith('unsigned int')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_codepoint')) {
        ret += this.getInput(i);
      } else if(type.startsWith('bool')) {
        ret += this.getCheckbox(i);
      } else if(type.startsWith('mjb_category')) {
        ret += this.getSelectInput(i, categories, null);
      } else if(type.startsWith('mjb_encoding')) {
        const options = [
          'MJB_ENC_UTF_8',
          'MJB_ENC_UTF_16BE',
          'MJB_ENC_UTF_16LE',
          'MJB_ENC_UTF_32BE',
          'MJB_ENC_UTF_32LE',
        ];

        // See mjb_encoding on mojibake.h
        const values = [
          0x2,
          0x8,
          0x10,
          0x40,
          0x80,
        ];

        ret += this.getSelectInput(i, options, values);
      } else if(type.startsWith('mjb_filter')) {
        const options = [
          'MJB_FILTER_NORMALIZE',
          'MJB_FILTER_SPACES',
          'MJB_FILTER_COLLAPSE_SPACES',
          'MJB_FILTER_CONTROLS',
          'MJB_FILTER_NUMERIC',
          'MJB_FILTER_LIMIT_COMBINING',
        ];
        const values = [
          0x1,
          0x2,
          0x4,
          0x8,
          0x10,
          0x20,
        ];

        ret += this.getBitfieldInput(i, options, values, 'Leave all unchecked for MJB_FILTER_NONE.');
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

        ret += this.getSelectInput(i, options, values);
      } else if(type.startsWith('mjb_result')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_property')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_locale_id')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_error')) {
        ret += this.getInput(i);
      } else if(type.startsWith('mjb_normalization')) {
        // See mjb_normalization on mojibake.h
        const options = [
          'MJB_NORMALIZATION_NFC',
          'MJB_NORMALIZATION_NFD',
          'MJB_NORMALIZATION_NFKC',
          'MJB_NORMALIZATION_NFKD',
        ];

        ret += this.getSelectInput(i, options);
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

        ret += this.getSelectInput(i, options, values);
      }

      ++i;
    }

    ret += `\n            <div class="function-form-button">
              <button type="submit" id="${this.getName()}-submit">Call function</button>
            </div>\n          </form>\n`;

    return ret;
  }

  public formatEventListener(): string {
    return `document.getElementById('${this.getName()}-submit').addEventListener('click', () => { callFunction('${this.getName()}'); });`
  }

  public static snakeToCamel(str: string): string {
    return str.replace(/_([a-z])/g, (_, letter) => letter.toUpperCase());
  }

  public static functionName(cName: string): string {
    return cName.replace(/^mjb_/, '');
  }

  private methodCall(): string {
    const args = this.args.filter(arg => !arg.wasm_generated);
    const argStrings = args.map(arg => `getArg('${this.getName()}-${arg.name}')`);

    return `mojibake.${CFunction.snakeToCamel(this.name)}(${argStrings.join(', ')})`;
  }
}

export function cfns(): CFunction[] {
  const names = new Set(functions.map(item => item.name));

  for(const item of functions) {
    for(const related of item.related ?? []) {
      if(!names.has(related)) {
        throw new Error(`${item.name}: unknown related function "${related}"`);
      }
    }
  }

  return functions.map(item => new CFunction(item));
}
