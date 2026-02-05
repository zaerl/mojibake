import { Character } from '../character';
import { log } from '../log';
import { parsePropertyFile, ucdCodepointRange } from './utils';

export interface Property {
  id: number;
  enumCount: number;
  shortName: string;
  name: string;
  bool: boolean;
  values: { [key: string]: number };
}

export interface PropertyRange {
  start: number;
  end: number;
  properties: Uint8Array;  // Encoded BLOB
}

// Internal interface for collecting properties before encoding
interface PropertyEntry {
  propertyId: number;
  start: number;
  end: number;
  value: number;
}

async function readPropertyNamesValues(): Promise<Property[]> {
  log('READ PROPERTY NAMES VALUES');

  const properties: { [key: string]: Property } = {};

  for await (const split of parsePropertyFile('./UCD/PropertyAliases.txt')) {
    const property = split[0];
    const longName = split[1];

    if(property === 'blk' || property === 'na1') {
      continue;
    }

    if(properties[property] === undefined) {
      properties[property] = {
        id: 0,
        enumCount: 0,
        shortName: property,
        name: longName,
        bool: false,
        values: {}
      };
    } else {
      log(`Property ${property} already exists`);
    }
  }

  let valuesCount = 0;

  for await (const split of parsePropertyFile('./UCD/PropertyValueAliases.txt')) {
    const property = split[0];
    const value = split[1];

    if(property === 'blk' || property === 'na1') {
      continue;
    }

    if(properties[property] === undefined) {
      log(`Property ${property} does not exist`);
    }

    properties[property].values[value] = properties[property].enumCount++;
    ++valuesCount;
  }

  let bools = 0;
  let longestEnum = 0;
  let id = 0;

  for(const property in properties) {
    const value = properties[property];

    value.bool = value.values['N'] === 0;
    value.id = id++;

    if(value.bool) {
      ++bools;
    }

    if(value.enumCount > longestEnum && property !== 'blk') {
      longestEnum = value.enumCount;
    }

    if(value.enumCount > 256) {
      log(`Property ${property} has ${value.enumCount} values, which is greater than 256`);
    }
  }

  log(`Properties: ${id}`);
  log(`Bools: ${bools}`);
  log(`Values: ${valuesCount}`);
  log(`Longest enum: ${longestEnum}`);

  return Object.values(properties);
}

/**
 * Encode property range BLOB with separate boolean and enumerated sections.
 *
 * [bool_count] [bool_prop_id 1] [bool_prop_id 2] ...
 * [enum_count] [enum_prop_id] [value 2] [enum_prop_id 2] [value 2] ...
 */
function encodePropertyRange(
  properties: { propertyId: number, value: number }[],
  propertyMetadata: Property[]
): Uint8Array {
  const sorted = properties.sort((a, b) => a.propertyId - b.propertyId);

  const boolProps: number[] = [];
  const enumProps: {id: number, value: number}[] = [];

  for(const prop of sorted) {
    const metadata = propertyMetadata[prop.propertyId];

    if(!metadata) {
      log(`No metadata found for property ID ${prop.propertyId}`);
      enumProps.push({id: prop.propertyId, value: prop.value});

      continue;
    }

    if(metadata.bool) {
      // Store only ID
      if(prop.value !== 1) {
        log(
          `Boolean property ${prop.propertyId} (${metadata.name}) has value ${prop.value}, expected 1`
        );
      }
      boolProps.push(prop.propertyId);
    } else {
      // Store ID + value
      enumProps.push({id: prop.propertyId, value: prop.value});
    }
  }

  // Calculate size: [bool_count] + bool_ids + [enum_count] + enum_pairs
  const size = 1 + boolProps.length + 1 + 2 * enumProps.length;
  const blob = new Uint8Array(size);
  let offset = 0;

  // Encode boolean properties (just IDs, no values)
  blob[offset++] = boolProps.length;

  for(const id of boolProps) {
    blob[offset++] = id;
  }

  // Encode enumerated properties (ID + value pairs)
  blob[offset++] = enumProps.length;

  for(const prop of enumProps) {
    blob[offset++] = prop.id;
    blob[offset++] = prop.value;
  }

  return blob;
}

export async function buildPropertyRanges(characters: Character[]): Promise<{ propertyRanges: PropertyRange[], properties: Property[] }> {
  log('READ PROPERTY RANGES');

  const properties = await readPropertyNamesValues();
  const propertyMap: { [key: string]: number } = {};
  const propertyEntries: PropertyEntry[] = [];

  for(let i = 0; i < properties.length; ++i) {
    propertyMap[properties[i].name] = i;
    propertyMap[properties[i].shortName] = i;
  }

  const files = [
    './UCD/PropList.txt',
    './UCD/DerivedCoreProperties.txt',
    './UCD/EastAsianWidth.txt',
    './UCD/LineBreak.txt',
    'UCD/auxiliary/GraphemeBreakProperty.txt',
    // './UCD/Scripts.txt',
  ];

  const defaultProperties = [
    '',
    '',
    'ea',
    'lb',
    'GCB',
    // 'sc',
  ]

  // Collect all property entries
  for(let i = 0; i < files.length; ++i) {
    const file = files[i];
    log(`Reading ${file}`);

    for await (const split of parsePropertyFile(file)) {
      const hasDefault = defaultProperties[i].length > 0;
      const range = ucdCodepointRange(split[0]);
      const property = hasDefault ? defaultProperties[i] : split[1];
      const value = hasDefault ? split[1] : split[2];

      if(property && propertyMap[property]) {
        let valueId = 1;

        if(typeof value !== 'undefined') {
          if(properties[propertyMap[property]].bool) {
            log(`Boolean property: ${property} has value: ${value}`);
          }

          if(typeof properties[propertyMap[property]].values[value] === 'number') {
            valueId = properties[propertyMap[property]].values[value];
          } else {
            log(`Unknown property value: ${value}`);
          }
        } else {
          if(!properties[propertyMap[property]].bool) {
            log(`Non-boolean property: ${property} has no value`);
          }
        }

        propertyEntries.push({
          propertyId: properties[propertyMap[property]].id,
          start: range.codepointStart,
          end: range.codepointEnd,
          value: valueId,
        });
      } else {
        log(`Unknown property: ${property}`);
      }
    }
  }

  log(`Property entries: ${propertyEntries.length}`);

  const rangeMap = new Map<string, PropertyEntry[]>();

  // Group by range
  for(const entry of propertyEntries) {
    const key = `${entry.start}-${entry.end}`;

    if(!rangeMap.has(key)) {
      rangeMap.set(key, []);
    }

    rangeMap.get(key)!.push(entry);
  }

  log(`Unique ranges: ${rangeMap.size}`);

  // Encode each range using property metadata
  const propertyRanges: PropertyRange[] = [];
  let totalBlobSize = 0;
  let boolCount = 0;
  let enumCount = 0;

  for(const [key, entries] of rangeMap) {
    const [start, end] = key.split('-').map(Number);

    // Create properties array for encoding
    const props = entries.map(e => ({
      propertyId: e.propertyId,
      value: e.value
    }));

    for(const prop of props) {
      const metadata = properties[prop.propertyId];
      if(metadata && metadata.bool) {
        ++boolCount;
      } else {
        ++enumCount;
      }
    }

    // Encode the BLOB with property metadata
    const blob = encodePropertyRange(props, properties);
    totalBlobSize += blob.length;

    propertyRanges.push({
      start,
      end,
      properties: blob
    });
  }

  // Sort by start codepoint for consistent ordering
  propertyRanges.sort((a, b) => {
    if(a.start !== b.start) {
      return a.start - b.start;
    }
    return a.end - b.end;
  });

  log(`Encoded ${propertyRanges.length} ranges`);
  log(`Total BLOB size: ${totalBlobSize} bytes (avg: ${(totalBlobSize / propertyRanges.length).toFixed(2)} bytes/range)`);
  log(`Property breakdown: ${boolCount} boolean, ${enumCount} enumerated`);

  // Log enumerated properties for C code generation
  /*const enumProperties = properties.filter(p => !p.bool);

  if(enumProperties.length > 0) {
    log(`\nEnumerated properties (for C code):`);

    for(const prop of enumProperties) {
      log(`  ${prop.id}: ${prop.name} (${prop.shortName}) - ${prop.enumCount} values`);
    }
  }*/

  return { propertyRanges, properties };
}

/**
 * Get property metadata for use in C code generation
 * Returns information about which properties are boolean vs enumerated
 */
export async function getPropertyMetadata(): Promise<{
  all: Property[],
  boolean: Property[],
  enumerated: Property[]
}> {
  const properties = await readPropertyNamesValues();
  const booleanProps = properties.filter(p => p.bool);
  const enumeratedProps = properties.filter(p => !p.bool);

  return {
    all: properties,
    boolean: booleanProps,
    enumerated: enumeratedProps
  };
}
