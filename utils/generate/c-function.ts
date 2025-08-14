import fs from 'fs';
import path from 'path';

export class CFunction {
  constructor(
    private comment: string,
    private ret: string,
    private name: string,
    private attributes: string[] = [],
    private args: string[] = []
  ) {
    if(!this.ret.endsWith('*')) {
      this.ret += ' ';
    }
  }

  formatC(): string {
    const attributes = this.attributes.length ? `${this.attributes.join(' ')} ` : '';
    return `// ${this.comment}\n${attributes}${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});`;
  }

  formatMD(): string {
    return `${this.comment}\n\n\`\`\`c\n${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});\n\`\`\``;
  }
}

export function cfns(): CFunction[] {
  const jsonPath = path.join(__dirname, 'c-functions.json');
  const jsonData = fs.readFileSync(jsonPath, 'utf8');
  const data: any[] = JSON.parse(jsonData);

  return data.map(item => new CFunction(
    item.comment,
    item.ret,
    item.name,
    item.attributes,
    item.args
  ));
}
