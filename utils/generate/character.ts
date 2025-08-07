import { QuickCheckResult } from './quick-check';
import { BidirectionalCategories, CharacterDecomposition } from './types';

export class Character {
  constructor(
    public codepoint: number,
    public name: string,
    public category: number,
    public combining: number,
    public bidirectional: BidirectionalCategories | null,
    public decomposition: CharacterDecomposition,
    public decompositions: number[],
    public decimal: number | null,
    public digit: number | null,
    public numeric: string | null,
    public mirrored: boolean,
    // unicode 1.0 name
    // comment
    public uppercase: number | null,
    public lowercase: number | null,
    public titlecase: number | null,
    public quickCheck: QuickCheckResult | null
  ) {}

  formatC(): string {
    return `{ ${this.fmt(this.codepoint)}, ${this.fmt(this.name)}, ${this.fmt(this.category)}, ` +
      `${this.fmt(this.combining)}, ${this.fmt(this.bidirectional)}, ${this.fmt(this.decomposition)}, ` +
      `${this.fmt(this.decimal)}, ${this.fmt(this.digit)}, ` + `${this.fmt(this.numeric)}, ${this.mirrored}, ` +
      `${this.fmt(this.uppercase)}, ${this.fmt(this.lowercase)}, ${this.fmt(this.titlecase)} }`;
  }

  public fmt(value: string | number | null, defaultC = 'NULL'): string {
    if(value === null) {
      return defaultC;
    } else {
      return typeof value === 'number' ? `${value === 0 ? 0 : '0x' + value.toString(16).toUpperCase()}` : `"${value}"`;
    }
  }
}
