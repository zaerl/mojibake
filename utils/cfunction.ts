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
    return `// ${this.comment}\n${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});`;
  }

  formatMD(): string {
    return `${this.comment}\n\`\`\`c\n${this.ret}mjb_${this.name}(${this.args.length ? this.args.join(', ') : 'void'});\n\`\`\``;
  }
}

export const cfns: CFunction[] = [
  new CFunction('Initialize the library. Not needed to be called',
    'bool', 'initialize'),

  new CFunction('Initialize the library with custom values. Not needed to be called',
    'bool', 'initialize_v2',
    ['mjb_alloc_fn alloc_fn', 'mjb_realloc_fn realloc_fn', 'mjb_free_fn free_fn']),

  new CFunction('Shutdown the library. Not needed to be called',
    'void', 'shutdown'),

  new CFunction('Allocate and zero memory',
    'void *', 'alloc',
    ['size_t size']),

  new CFunction('Reallocate memory',
    'void *', 'realloc',
    ['void *ptr', 'size_t new_size']),

  new CFunction('Free memory',
    'void', 'free',
    ['void *ptr']),

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

  new CFunction('Return nexy codepoint in the string',
    'mjb_codepoint', 'string_next_codepoint',
    ['const char *buffer', 'size_t size', 'size_t *next']),

  new CFunction('Return true if the string is encoded in ASCII',
    'bool', 'string_is_ascii',
    ['const char *buffer', 'size_t size']),

  new CFunction('Encode a codepoint to a string',
    'unsigned int', 'codepoint_encode',
    ['mjb_codepoint codepoint', 'char *buffer', 'size_t size', 'mjb_encoding encoding']),

  new CFunction('Return true if the codepoint is valid',
    'bool', 'codepoint_is_valid',
    ['mjb_codepoint codepoint']),

  new CFunction('Return the codepoint character',
    'bool', 'codepoint_character',
    ['mjb_character *character', 'mjb_codepoint codepoint']),

  new CFunction('Return hangul syllable name',
    'bool', 'hangul_syllable_name',
    ['mjb_codepoint codepoint', 'char *buffer', 'size_t size']),

  new CFunction('Hangul syllable decomposition',
    'bool', 'hangul_syllable_decomposition',
    ['mjb_codepoint codepoint', 'mjb_codepoint *codepoints']),

  new CFunction('Return if the codepoint is an hangul syllable',
    'bool', 'codepoint_is_hangul_syllable',
    ['mjb_codepoint codepoint']),

  new CFunction('Return if the codepoint is CJK ideograph',
    'bool', 'codepoint_is_cjk_ideograph',
    ['mjb_codepoint codepoint']),

  new CFunction('Return true if the codepoint has the category',
    'bool', 'codepoint_category_is',
    ['mjb_codepoint codepoint', 'mjb_category category']),

  new CFunction('Return true if the codepoint has the block',
    'bool', 'codepoint_block_is',
    ['mjb_codepoint codepoint', 'mjb_block block']),

  new CFunction('Return true if the codepoint is graphic',
    'bool', 'codepoint_is_graphic',
    ['mjb_codepoint codepoint']),

  new CFunction('Return true if the codepoint is combining',
    'bool', 'codepoint_is_combining',
    ['mjb_codepoint codepoint']),

  new CFunction('Return true if the category is combining',
    'bool', 'category_is_combining',
    ['mjb_category category']),

  new CFunction('Return the codepoint lowercase codepoint',
    'mjb_codepoint', 'codepoint_to_lowercase',
    ['mjb_codepoint codepoint']),

  new CFunction('Return the codepoint uppercase codepoint',
    'mjb_codepoint', 'codepoint_to_uppercase',
    ['mjb_codepoint codepoint']),

  new CFunction('Return the codepoint titlecase codepoint',
    'mjb_codepoint', 'codepoint_to_titlecase',
    ['mjb_codepoint codepoint']),

  new CFunction('Normalize a string',
    'char *', 'normalize',
    ['char *buffer', 'size_t size', 'size_t *output_size', 'mjb_encoding encoding', 'mjb_normalization form']),

  new CFunction('Sort',
    'void', 'sort',
    ['mjb_character arr[]', 'size_t size']),
];
