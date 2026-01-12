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
    // Step 1: Build candidate prefix list with usage information
    const prefixCounts = new Map<string, number>();
    const prefixStrings = new Map<string, Set<number>>(); // prefix -> set of string indices

    for(let i = 0; i < this.characters.length; ++i) {
      const str = this.characters[i].name ?? '';

      for(let len = minPrefixLength; len <= str.length; ++len) {
        const prefix = str.substring(0, len);
        prefixCounts.set(prefix, (prefixCounts.get(prefix) || 0) + 1);

        if(!prefixStrings.has(prefix)) {
          prefixStrings.set(prefix, new Set());
        }
        prefixStrings.get(prefix)!.add(i);
      }
    }

    // Step 2: Sort candidates by initial savings (greedy starting point)
    const candidates = [...prefixCounts.entries()]
      .filter(([p, count]) => count >= minCount)
      .map(([prefix, count]) => ({
        prefix,
        count,
        savings: (count - 1) * prefix.length - (prefix.length + 1) // Net savings including dict cost
      }))
      .sort((a, b) => b.savings - a.savings);

    // Step 3: Greedily select prefixes, but allow overlaps and recalculate per string
    const selectedPrefixes = new Map<string, number>();
    const stringBestPrefix = new Map<number, string>(); // Track best prefix for each string
    let prefixId = 1;

    for(const { prefix } of candidates) {
      if(selectedPrefixes.size >= maxPrefixes) break;

      // Calculate actual benefit of adding this prefix
      let additionalSavings = 0;
      const affectedStrings = prefixStrings.get(prefix) || new Set();

      for(const strIdx of affectedStrings) {
        const currentBest = stringBestPrefix.get(strIdx) || '';

        // Only add this prefix if it's longer than current best for this string
        if(prefix.length > currentBest.length) {
          additionalSavings += (prefix.length - currentBest.length);
        }
      }

      // Account for dictionary cost
      const dictCost = prefix.length + 1;
      const netSavings = additionalSavings - dictCost;

      // Only add prefix if it provides net positive savings
      if(netSavings > 0) {
        selectedPrefixes.set(prefix, prefixId++);

        // Update best prefix tracking for affected strings
        for(const strIdx of affectedStrings) {
          const currentBest = stringBestPrefix.get(strIdx) || '';

          if(prefix.length > currentBest.length) {
            stringBestPrefix.set(strIdx, prefix);
          }
        }
      }
    }

    // Step 4: Final encoding. Assign strings to their best prefix
    const prefixes = [];

    // Add empty prefix for strings with no match
    prefixes.push({ id: 0, prefix: '' });

    for(const [prefix, id] of selectedPrefixes) {
      prefixes.push({ id, prefix });
    }

    // Sort prefixes by length descending for greedy matching (longest first)
    const sortedPrefixes = [...selectedPrefixes.entries()]
      .sort((a, b) => b[0].length - a[0].length);

    for(let i = 0; i < this.characters.length; ++i) {
      const str = this.characters[i].name ?? '';

      // Find the longest matching prefix and update the character if needed
      for(const [prefix, prefixIdValue] of sortedPrefixes) {
        if(str.startsWith(prefix)) {
          this.characters[i].prefix = prefixIdValue;
          this.characters[i].name = str.substring(prefix.length);

          break;
        }
      }
    }

    return prefixes;
  }
}
