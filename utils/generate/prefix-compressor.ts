import { Character } from './character';

export interface Prefix {
  id: number;
  prefix: string;
}

export class PrefixCompressor {
  private characters: Character[] = [];

  constructor(characters: Character[]) {
    this.characters = characters;
  }

  public compress(minPrefixLength: number = 4, minCount: number = 2, maxPrefixes: number = 1000): Prefix[] {
    // Step 1: Find valuable prefixes
    const prefixCounts = new Map<string, number>();

    for(const char of this.characters) {
      const str = char.name ?? '';

      for(let len = minPrefixLength; len <= str.length; ++len) {
        const prefix = str.substring(0, len);
        prefixCounts.set(prefix, (prefixCounts.get(prefix) || 0) + 1);
      }
    }

    // Step 2: Select best non-overlapping prefixes greedily
    const candidates = [...prefixCounts.entries()]
      .filter(([p, count]) => count >= minCount)
      .map(([prefix, count]) => ({ prefix, count, savings: (count - 1) * prefix.length }))
      .sort((a, b) => b.savings - a.savings);

    const selectedPrefixes = new Map<string, number>();
    let prefixId = 1;

    for(const { prefix } of candidates) {
      // Check if this string would benefit from this prefix
      let dominated = false;

      for(const [existing] of selectedPrefixes) {
        if(existing.startsWith(prefix) || prefix.startsWith(existing)) {
          dominated = true;
          break;
        }
      }

      if(!dominated) {
        selectedPrefixes.set(prefix, prefixId++);
      }

      if(selectedPrefixes.size >= maxPrefixes) break; // Limit prefix table size
    }

    // Step 3: Encode strings
    const prefixes = [];

    // Add empty prefix for strings with no match
    prefixes.push({ id: 0, prefix: '' });

    for(const [prefix, id] of selectedPrefixes) {
      prefixes.push({ id, prefix });
    }

    // Sort prefixes by length descending for greedy matching
    const sortedPrefixes = [...selectedPrefixes.entries()]
      .sort((a, b) => b[0].length - a[0].length);

    for(let i = 0; i < this.characters.length; ++i) {
      const str = this.characters[i].name ?? '';

      for(const [prefix, prefixId] of sortedPrefixes) {
        if(str.startsWith(prefix)) {
          // Update character
          this.characters[i].prefix = prefixId;
          this.characters[i].name = str.substring(prefix.length);

          break;
        }
      }
    }

    return prefixes;
  }
}
