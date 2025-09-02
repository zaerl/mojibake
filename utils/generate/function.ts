import { readFileSync } from 'fs';
import path from 'path';

export class CFunction {
  constructor(
    private comment: string,
    private ret: string,
    private name: string,
    private attributes: string[] = [],
    private args: string[] = [],
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
    return this.args.length ? this.args : ['void'];
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

  formatHTML(): string {
    return `<section><div><code id="${this.name}" onclick="showFunctionCall('${this.getName()}')">` +
      `${this.ret}<span class="text-primary">${this.getName()}</span>(${this.getArgs().join(', ')});</code></div>` +
      `<div id="${this.getName()}" class="function-results"></div></section>`;
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
    item.wasm
  ));
}
