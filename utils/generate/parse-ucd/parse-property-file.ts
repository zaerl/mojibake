import { open } from 'fs/promises';

// Parse files like DerivedNormalizationProps.txt, SpecialCasing.txt, etc.
export async function* parsePropertyFile(path: string, starts: string[] = [], divider: string = ';', filterEmpty: boolean = true) {
  const file = await open(path);

  for await (let line of file.readLines()) {
    line = line.trim();

    // Comment or empty line
    if(!line || line === '' || line.startsWith('#') || starts.some(start => line.startsWith(start))) {
      continue
    }

    const split = line.split(divider).map(s => s.trim());

    if(split.length) {
      // If the last element contains a '#', return the left part
      const last = split[split.length - 1];

      if(last.includes('#')) {
        split[split.length - 1] = last.split('#')[0].trim();
      }
    }

    yield filterEmpty ? split.filter(s => s !== '') : split;
  }

  file.close();
}
