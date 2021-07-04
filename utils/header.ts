import { writeFileSync } from "fs";
import { cfns } from "./cfunction";
import { footer, header } from "./format";
import { Block, Categories, characterDecompositionMapping } from "./types";

export function generateHeader(blocks: Block[], categories: string[]) {
  const categoryEnums: string[] = [];

  for(let i = 0; i < categories.length; ++i) {
    categoryEnums.push(`    MJB_CATEGORY_${Categories[i].toUpperCase()} = 0x${(1 << i).toString(16).padStart(8, '0')}${ i === categories.length - 1 ? '' : ','} /* ${i} (${Categories[i]}) ${categories[i]} */`);
  }

  const fheader =
`${header('mojibake')}

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MJB_VERSION          "1.0.0"
#define MJB_VERSION_NUMBER   0x100 /* MAJOR << 8 && MINOR << 4 && REVISION */
#define MJB_VERSION_MAJOR    1
#define MJB_VERSION_MINOR    0
#define MJB_VERSION_REVISION 0

#define MJB_UNICODE_VERSION       "13.0"
#define MJB_UNICODE_VERSION_MAJOR 13
#define MJB_UNICODE_VERSION_MINOR 0

#ifndef MJB_EXTERN
#define MJB_EXTERN extern
#endif

#ifndef MJB_EXPORT
#define MJB_EXPORT __attribute__((visibility("default")))
#endif

/*
 * Mojibake is represented by a pointer to an instance of the opaque structure
 * named "mojibake". The [mjb_initialize()] and [mjb_initialize_v2()] functions
 * are its constructor. Every function accept an instance to this allocated
 * pointer. This is used to ensure reentrancy.
 */
typedef struct mojibake mojibake;

/* See c standard memory allocation functions */
typedef void *(*mjb_alloc_fn)(size_t size);
typedef void *(*mjb_realloc_fn)(void *ptr, size_t new_size);
typedef void (*mjb_free_fn)(void *ptr);

/*
 * A unicode codepoint, a value in the range 0 to 0x10FFFF
 * [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mjb_codepoint;

#define MJB_CODEPOINT_MIN         0x0
#define MJB_CODEPOINT_MAX         0x10FFFF /* Maximum valid unicode code point */
#define MJB_CODEPOINT_REPLACEMENT 0xFFFD /* The character used when there is invalid data */

/*
 * Unicode encoding
 * [see: https://www.unicode.org/glossary/#character_encoding_scheme]
 */
typedef enum mjb_encoding {
    MJB_ENCODING_UNKNOWN =   0x0,
    MJB_ENCODING_ASCII =     0x1,
    MJB_ENCODING_UTF_8 =     0x2,
    MJB_ENCODING_UTF_16 =    0x4,
    MJB_ENCODING_UTF_16_BE = 0x8,
    MJB_ENCODING_UTF_16_LE = 0x10,
    MJB_ENCODING_UTF_32 =    0x20,
    MJB_ENCODING_UTF_32_BE = 0x40,
    MJB_ENCODING_UTF_32_LE = 0x80
} mjb_encoding;

typedef enum mjb_block {
${blocks.map((value: Block, index: number) => `    ${value.enumName} = ${index}`).join(',\n')}
} mjb_block;

#define MJB_BLOCK_NUM ${blocks.length}

/*
 * Unicode block
 * [see: https://www.unicode.org/glossary/#block]
 */
typedef struct mjb_codepoint_block {
    char *name;
    uint32_t start;
    uint32_t end;
} mjb_codepoint_block;

/*
 * Unicode codepoint general category
 * [see: https://www.unicode.org/glossary/#general_category]
 */
typedef enum mjb_category {
${categoryEnums.join('\n')}
} mjb_category;

#define MJB_CATEGORY_COUNT ${categoryEnums.length}

/*
 * Decomposition
 * [see: https://www.unicode.org/glossary/#combining_class]
 */
typedef enum mjb_canonical_combining_class {
    MJB_CCC_SPACING =                0, /* Spacing, split, enclosing, reordrant, and Tibetan subjoined */
    MJB_CCC_OVERLAYS =               1, /* Overlays and interior */
    MJB_CCC_NUKTAS =                 7, /* Nuktas */
    MJB_CCC_HIRAGANA_KATAKANA =      8, /* Hiragana/Katakana voicing marks */
    MJB_CCC_VIRAMAS =                9, /* Viramas */
    MJB_CCC_FIXED_START =           10, /* Start of fixed position classes */
    MJB_CCC_FIXED_END =            199, /* End of fixed position classes */
    MJB_CCC_BELOW_LEFT_ATTACHED =  200, /* Below left attached */
    MJB_CCC_BELOW_ATTACHED =       202, /* Below attached */
    MJB_CCC_BELOW_RIGHT_ATTACHED = 204, /* Below right attached */
    MJB_CCC_LEFT_ATTACHED =        208, /* Left attached (reordrant around single base character) */
    MJB_CCC_RIGHT_ATTACHED =       210, /* Right attached */
    MJB_CCC_ABOVE_LEFT_ATTACHED =  212, /* Above left attached */
    MJB_CCC_ABOVE_ATTACHED =       214, /* Above attached */
    MJB_CCC_ABOVE_RIGHT_ATTACHED = 216, /* Above right attached */
    MJB_CCC_BELOW_LEFT =           218, /* Below left */
    MJB_CCC_BELOW =                220, /* Below */
    MJB_CCC_BELOW_RIGHT =          222, /* Below right */
    MJB_CCC_LEFT =                 224, /* Left (reordrant around single base character) */
    MJB_CCC_RIGHT =                226, /* Right */
    MJB_CCC_ABOVE_LEFT =           228, /* Above left */
    MJB_CCC_ABOVE =                230, /* Above */
    MJB_CCC_ABOVE_RIGHT =          232, /* Above right */
    MJB_CCC_DOUBLE_BELOW =         233, /* Double below */
    MJB_CCC_DOUBLE_ABOVE =         234, /* Double above */
    MJB_CCC_BELOW_IOTA =           240  /* Below (iota subscript) */
} mjb_canonical_combining_class;

#define MJB_CCC_COUNT 26

/*
 * Bidirectional categories
 * [see: https://www.unicode.org/glossary/#combining_class]
 */
typedef enum mjb_bidi_categories {
    NONE, /* Nothing specified */
    L,	  /* Left-to-right */
    R,	  /* Right-to-left */
    AL,	  /* Right-to-left arabic */
    EN,	  /* European number */
    ES,	  /* European number separator */
    ET,	  /* European number terminator */
    AN,	  /* Arabic number */
    CS,	  /* Common number separator */
    NSM,  /* Nonspacing mark */
    BN,	  /* Boundary neutral */
    B,    /* Paragraph separator */
    S,    /* Segment separator */
    WS,   /* Whitespace */
    ON,	  /* Other neutrals */
    LRE,  /* Left-to-right embedding */
    LRO,  /* Left-to-right override */
    RLE,  /* Right-to-left embedding */
    RLO,  /* Right-to-left override */
    PDF,  /* Pop Directional format */
    LRI,  /* Left-to-right isolate */
    RLI,  /* Right-to-Left isolate */
    FSI,  /* First strong isolate */
    PDI	  /* Pop directional isolate */
} mjb_bidi_categories;

#define MJB_BIDI_COUNT 24

/*
 * Unicode plane
 * [see: https://www.unicode.org/glossary/#plane]
 */
typedef enum mjb_plane {
    MJB_PLANE_BMP =    0,
    MJB_PLANE_SMP =    1,
    MJB_PLANE_SIP =    2,
    MJB_PLANE_TIP =    3,
    MJB_PLANE_SSP =   14,
    MJB_PLANE_PUA_A = 15,
    MJB_PLANE_PUA_B = 16
} mjb_plane;

#define MJB_PLANE_NUM 17 /* 17 planes */
#define MJB_PLANE_SIZE 65536 /* 2^16 code points per plane */

/*
 * Normalization form
 * [see: https://www.unicode.org/glossary/#normalization_form]
 */
typedef enum mjb_normalization {
    MJB_NORMALIZATION_NFD =  0, /* Canonical decomposition and ordering */
    MJB_NORMALIZATION_NFC =  1, /* Composition after canonical decomposition and ordering */
    MJB_NORMALIZATION_NFKD = 2, /* Compatible decomposition and ordering */
    MJB_NORMALIZATION_NFKC = 3  /* Composition after compatible decomposition and ordering */
} mjb_normalization;

/*
 * Decomposition
 * [see: https://www.unicode.org/glossary/#compatibility_decomposition]
 */
typedef enum mjb_decomposition {
${Object.keys(characterDecompositionMapping).map((value: string, index: number) => `    MJB_DECOMPOSITION_${value.toUpperCase().replace(/[<>]/g, '')} = ${index}`).join(',\n')}
} mjb_decomposition;

/*
 * A unicode character
 * [see: https://www.unicode.org/glossary/#character]
 */
typedef struct mjb_character {
    mjb_codepoint codepoint;
    char *name;
    mjb_block block;
    mjb_category category;
    mjb_canonical_combining_class combining;
    unsigned short bidirectional;
    char *decimal;
    char *digit;
    char *numeric;
    bool mirrored;
    mjb_codepoint uppercase;
    mjb_codepoint lowercase;
    mjb_codepoint titlecase;
} mjb_character;

${cfns.map(value => value.formatC()).join('\n\n')}

#ifdef __cplusplus
}
#endif

${footer('mojibake')}
`;

  writeFileSync('../src/mojibake.h', fheader);
}
