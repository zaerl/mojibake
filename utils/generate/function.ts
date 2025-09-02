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

  formatC(): string {
    const attributes = this.attributes.length ? `${this.attributes.join(' ')} ` : '';
    return `// ${this.comment}\n${attributes}${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});`;
  }

  formatMD(): string {
    return `${this.comment}\n\n\`\`\`c\n${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});\n\`\`\``;
  }

  formatWASM(): string {
    return `\\"_mjb_${this.name}\\"`;
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
