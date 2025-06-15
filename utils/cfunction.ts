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

export const cfns: CFunction[] = [
  new CFunction('Initialize the library. Not needed to be called',
    'bool', 'initialize', ['MJB_NODISCARD']),

  new CFunction('Initialize the library with custom values. Not needed to be called',
    'bool', 'initialize_v2', ['MJB_NODISCARD'],
    ['mjb_alloc_fn alloc_fn', 'mjb_realloc_fn realloc_fn', 'mjb_free_fn free_fn']),

  new CFunction('Shutdown the library. Not needed to be called',
    'void', 'shutdown'),

  new CFunction('Allocate and zero memory',
    'void *', 'alloc', ['MJB_NODISCARD'],
    ['size_t size']),

  new CFunction('Reallocate memory',
    'void *', 'realloc', ['MJB_NODISCARD', 'MJB_NONNULL(1)'],
    ['void *ptr', 'size_t new_size']),

  new CFunction('Free memory',
    'void', 'free', ['MJB_NONNULL(1)'],
    ['void *ptr']),

  new CFunction('Output the current library version (MJB_VERSION)',
    'const char *', 'version', ['MJB_CONST']),

  new CFunction('Output the current library version number (MJB_VERSION_NUMBER)',
    'unsigned int', 'version_number', ['MJB_CONST']),

  new CFunction('Output the current supported unicode version (MJB_UNICODE_VERSION)',
    'const char *', 'unicode_version', ['MJB_CONST']),

  new CFunction('Return true if the plane is valid',
    'bool', 'plane_is_valid', ['MJB_CONST'],
    ['mjb_plane plane']),

  new CFunction('Return the name of a plane, NULL if the place specified is not valid',
    'const char *', 'plane_name', ['MJB_CONST'],
    ['mjb_plane plane', 'bool abbreviation']),

  new CFunction('Return the string encoding (the most probable)',
    'mjb_encoding', 'string_encoding', ['MJB_PURE'],
    ['const char *buffer', 'size_t size']),

  new CFunction('Return true if the string is encoded in UTF-8',
    'bool', 'string_is_utf8', ['MJB_PURE'],
    ['const char *buffer', 'size_t size']),

  new CFunction('Return nexy codepoint in the string',
    'mjb_codepoint', 'string_next_codepoint', ['MJB_PURE'],
    ['const char *buffer', 'size_t size', 'size_t *next']),

  new CFunction('Return true if the string is encoded in ASCII',
    'bool', 'string_is_ascii', ['MJB_PURE'],
    ['const char *buffer', 'size_t size']),

  new CFunction('Encode a codepoint to a string',
    'unsigned int', 'codepoint_encode', [],
    ['mjb_codepoint codepoint', 'char *buffer', 'size_t size', 'mjb_encoding encoding']),

  new CFunction('Return true if the codepoint is valid',
    'bool', 'codepoint_is_valid', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Return the codepoint character',
    'bool', 'codepoint_character', ['MJB_NONNULL(1)'],
    ['mjb_character *character', 'mjb_codepoint codepoint']),

  new CFunction('Return hangul syllable name',
    'bool', 'hangul_syllable_name', ['MJB_NONNULL(2)'],
    ['mjb_codepoint codepoint', 'char *buffer', 'size_t size']),

  new CFunction('Hangul syllable decomposition',
    'bool', 'hangul_syllable_decomposition', ['MJB_NONNULL(2)'],
    ['mjb_codepoint codepoint', 'mjb_codepoint *codepoints']),

  new CFunction('Return if the codepoint is an hangul syllable',
    'bool', 'codepoint_is_hangul_syllable', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Return if the codepoint is CJK ideograph',
    'bool', 'codepoint_is_cjk_ideograph', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Return true if the codepoint has the category',
    'bool', 'codepoint_category_is', ['MJB_CONST'],
    ['mjb_codepoint codepoint', 'mjb_category category']),

  new CFunction('Return true if the codepoint has the block',
    'bool', 'codepoint_block_is', ['MJB_CONST'],
    ['mjb_codepoint codepoint', 'mjb_block block']),

  new CFunction('Return true if the codepoint is graphic',
    'bool', 'codepoint_is_graphic', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Return true if the codepoint is combining',
    'bool', 'codepoint_is_combining', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Return true if the category is combining',
    'bool', 'category_is_combining', ['MJB_CONST'],
    ['mjb_category category']),

  new CFunction('Return the codepoint lowercase codepoint',
    'mjb_codepoint', 'codepoint_to_lowercase', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Return the codepoint uppercase codepoint',
    'mjb_codepoint', 'codepoint_to_uppercase', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Return the codepoint titlecase codepoint',
    'mjb_codepoint', 'codepoint_to_titlecase', ['MJB_CONST'],
    ['mjb_codepoint codepoint']),

  new CFunction('Normalize a string',
    'char *', 'normalize', ['MJB_NONNULL(1, 3)'],
    ['char *buffer', 'size_t size', 'size_t *output_size', 'mjb_encoding encoding', 'mjb_normalization form']),

  new CFunction('Sort',
    'void', 'sort', ['MJB_NONNULL(1)'],
    ['mjb_character arr[]', 'size_t size']),
];
