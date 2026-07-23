/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

import hljs from 'highlight.js/lib/core';
import c from 'highlight.js/lib/languages/c';
import functions, {
  MojibakeArg, MojibakeFunction, MojibakeReturnCase, MojibakeSpecRef, Section
} from './functions';
import { caseModes, caseType, caseTypeValues, categories, collationModes, directions, encodings, encodingValues, filterFlags, filterFlagValues, identifierProfiles, normalizations, planes, planeValues, widthContexts } from './types';

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
  public variadic?: boolean;
  public wasm: boolean;
  public wasmName?: string;
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
    this.variadic = fn.variadic;
    this.wasm = fn.wasm;
    this.wasmName = fn.wasmName;
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

  public static wasmFunctionName(wasmName?: string, name?: string): string {
    return wasmName ?? CFunction.snakeToCamel(CFunction.functionName(name ?? ''));
  }

  getArgs(): string[] {
    const args = this.args.map((arg, index) => `${arg.type}${arg.type.endsWith('*') ? '' : ' '}${arg.name}`);

    if(this.variadic) {
      args.push('...');
    }

    return args.length ? args : ['void'];
  }

  getName() {
    return this.name;
  }

  getSection() {
    return this.section;
  }

  getLabelName(arg: number) {
    return this.args[arg].name.replace(/_/g, ' ');
  }

  isInternal() {
    return this.name.startsWith('initialize');
  }

  formatC(): string {
    const attributes = ['MJB_EXPORT', ...this.attributes].join(' ');
    return `// ${this.comment}\n${attributes} ${this.ret}${this.getName()}(${this.getArgs().join(', ')});`;
  }

  formatSignature(): string {
    const args = this.getArgs();
    const fn = args.length === 1 && args[0] === 'void' ?
      `${this.ret}${this.getName()}(void);` :
      `${this.ret}${this.getName()}(\n    ${this.getArgs().join(',\n    ')}\n);`;

    return fn;
  }

  formatMD(): string {
    let ret = `## \`${this.getName()}\`\n\n${this.comment}\n\n`;
    ret += `\`\`\`c\n${this.formatSignature()}\n\`\`\``;

    if(this.details) {
      ret += `\n\n${this.details}`;
    }

    if(this.args.length) {
      ret += '\n\n' + this.args.map(arg => {
        let line = `- \`${arg.name}\` - ${arg.description}`;

        if(arg.ownership) {
          line += `. ${arg.ownership}`;
        }

        return line;
      }).join('\n');
    }

    if(this.returns?.length) {
      ret += '\n\n**Returns**\n\n' + this.returns.map(
        value => `- \`${value.value}\` - ${value.description}`
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
      variadic: this.variadic,
      wasm: this.wasm,
      wasmName: CFunction.wasmFunctionName(this.wasmName, this.name),
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
    const fn = this.formatSignature();
    const hasWASMForm = this.isWASM();
    const toggleLabel = hasWASMForm ? 'Try it' : 'Details';
    const toggleAriaLabel = hasWASMForm ?
      `Try ${this.getName()} in your browser` :
      `Show details for ${this.getName()}`;

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
          aria-label="${toggleAriaLabel}" data-wasm="${hasWASMForm}"
          onclick="toggleFunctionCall('${this.getName()}')">${toggleLabel}</button>
      </div>
      <div class="function-card" id="${this.getName()}-card">
        ${this.documentationHTML(relatedLinkTargets)}
        <div>${hasWASMForm ? this.formInputHTML() : '' }</div>
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
        ret += `\n            <li><code>${CFunction.escapeHTML(returnCase.value)}</code> - ` +
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

    let ret = `<div class="function-checkbox"><input id="${name}" type="checkbox" name="${name}" ${disabled ? 'disabled' : ''}>` +
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

      switch(type) {
        case 'bool':
          ret += this.getCheckbox(i);
          break;
        // case 'bool *':
        case 'char *':
        case 'const char *':
          ret += this.getInput(i);
          break;
        // case 'const mjb_bidi_paragraph *':
        // case 'const size_t *':
        // case 'int *':
        // case 'int32_t *':
        // case 'mjb_alloc_fn':
        // case 'mjb_bidi_paragraph *':
        // case 'mjb_bidi_run *':
        // case 'mjb_block_info *':
        // case 'mjb_buffer_character *':
        case 'mjb_caseless_mode':
          ret += this.getSelectInput(i, caseModes, null);
          break;
        case 'mjb_category':
          ret += this.getSelectInput(i, categories, null);
          break;
        // case 'mjb_character *':
        case 'mjb_codepoint':
          ret += this.getInput(i);
          break;
        // case 'mjb_codepoint *':
        case 'mjb_collation_mode':
          ret += this.getSelectInput(i, collationModes, null);
          break;
        case 'mjb_direction':
          ret += this.getSelectInput(i, directions, null);
          break;
        // case 'mjb_east_asian_width *':
        // case 'mjb_emoji_properties *':
        // case 'mjb_emoji_sequence *':
        case 'mjb_encoding':
          ret += this.getSelectInput(i, encodings, encodingValues);
          break;
        // case 'mjb_error *':
        case 'mjb_filter_flags':
          ret += this.getBitfieldInput(i, filterFlags, filterFlagValues,
            'Leave all unchecked for MJB_FILTER_NONE.');
          break;
        // case 'mjb_for_each_character_fn':
        // case 'mjb_free_fn':
        case 'mjb_identifier_profile':
          ret += this.getSelectInput(i, identifierProfiles, null);
          break;
        case 'mjb_locale':
          ret += this.getInput(i);
          break;
        // case 'mjb_locale_id *':
        case 'mjb_map_case_type':
          ret += this.getSelectInput(i, caseType, caseTypeValues);
          break;
        // case 'mjb_next_line_state *':
        // case 'mjb_next_sentence_state *':
        // case 'mjb_next_state *':
        // case 'mjb_next_word_state *':
        case 'mjb_normalization':
          ret += this.getSelectInput(i, normalizations);
          break;
        // case 'mjb_numeric_value *':
        case 'mjb_plane':
          ret += this.getSelectInput(i, planes, planeValues);
          break;
        case 'mjb_property': // TODO: generate from mjb_property enum
          ret += this.getInput(i);
          break;
        // case 'mjb_quick_check_result *':
        // case 'mjb_realloc_fn':
        // case 'mjb_result *':
        // case 'mjb_script *':
        case 'mjb_width_context':
          ret += this.getSelectInput(i, widthContexts);
          break;
        case 'size_t':
          ret += this.getInput(i);
          break;
        // case 'size_t *':
        // case 'va_list':
        // case 'void *':
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
