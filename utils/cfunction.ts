export class CFunction {
  constructor(
    private comment: string,
    private ret: string,
    private name: string,
    private args: string[] = []
  ) {
    if(!this.ret.endsWith('*')) {
      this.ret += ' ';
    }
  }

  formatC(): string {
    return `/* ${this.comment} */\n${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});`;
  }

  formatMD(): string {
    return `${this.comment}\n\`\`\`c\n${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});\n\`\`\``;
  }
}

export const cfns: CFunction[] = [
  new CFunction('Initialize the library',
    'bool', 'initialize',
    ['mojibake **mjb']),

  new CFunction('Initialize the library with custom values',
    'bool', 'initialize_v2',
    ['mojibake **mjb', 'mjb_alloc_fn alloc_fn', 'mjb_realloc_fn realloc_fn', 'mjb_free_fn free_fn']),

  new CFunction('The library is ready',
    'bool', 'ready',
    ['mojibake *mjb']),

  new CFunction('Allocate and zero memory',
    'void *', 'alloc',
    ['mojibake *mjb', 'size_t size']),

  new CFunction('Reallocate memory',
    'void *', 'realloc',
    ['mojibake *mjb', 'void *ptr', 'size_t new_size']),

  new CFunction('Free memory',
    'void', 'free',
    ['mojibake *mjb', 'void *ptr']),

  new CFunction('Output the current library version (MJB_VERSION)',
    'char *', 'version'),

  new CFunction('Output the current library version number (MJB_VERSION_NUMBER)',
    'unsigned int', 'version_number'),

  new CFunction('Output the current supported unicode version (MJB_UNICODE_VERSION)',
    'char *', 'unicode_version'),

  new CFunction('Return true if the plane is valid',
    'bool', 'plane_is_valid',
    ['mjb_plane plane']),

  new CFunction('Return the name of a plane, NULL if the place specified is not valid',
    'const char *', 'plane_name',
    ['mjb_plane plane', 'bool abbreviation']),

  new CFunction('Return the string encoding (the most probable)',
    'mjb_encoding', 'string_encoding',
    ['const char *buffer', 'size_t size']),

  new CFunction('Return true if the string is encoded in UTF-8',
    'bool', 'string_is_utf8',
    ['const char *buffer', 'size_t size']),

  new CFunction('Return true if the string is encoded in ASCII',
    'bool', 'string_is_ascii',
    ['const char *buffer', 'size_t size']),

  new CFunction('Return true if the codepoint is valid',
    'bool', 'codepoint_is_valid',
    ['mojibake *mjb', 'mjb_codepoint codepoint']),

  new CFunction('Return the codepoint character',
    'bool', 'codepoint_character',
    ['mojibake *mjb', 'mjb_character *character', 'mjb_codepoint codepoint']),

  new CFunction('Return true if the codepoint has the category',
    'bool', 'codepoint_is',
    ['mojibake *mjb', 'mjb_codepoint codepoint', 'mjb_category category']),

  new CFunction('Return true if the codepoint is graphic',
    'bool', 'codepoint_is_graphic',
    ['mojibake *mjb', 'mjb_codepoint codepoint']),

  new CFunction('Return the codepoint lowercase codepoint',
    'mjb_codepoint', 'codepoint_to_lowercase',
    ['mojibake *mjb', 'mjb_codepoint codepoint']),

  new CFunction('Return the codepoint uppercase codepoint',
    'mjb_codepoint', 'codepoint_to_uppercase',
    ['mojibake *mjb', 'mjb_codepoint codepoint']),

  new CFunction('Return the codepoint titlecase codepoint',
    'mjb_codepoint', 'codepoint_to_titlecase',
    ['mojibake *mjb', 'mjb_codepoint codepoint']),

  new CFunction('Normalize a string',
    'void *', 'normalize',
    ['mojibake *mjb', 'void *source', 'size_t source_size', 'size_t *output_size', 'mjb_encoding encoding', 'mjb_normalization form']),
];
