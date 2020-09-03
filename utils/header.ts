import { header, footer } from "./format";
import { cfns } from "./cfunction";
import { writeFileSync } from "fs";
import { Block, characterDecompositionMapping } from "./types";

export function generateHeader(blocks: Block[], categoryEnums: string[]) {
  const fheader =
`${header('mojibake')}

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MJB_VERSION "1.0.0"
#define MJB_VERSION_NUMBER 0x100 /* MAJOR << 8 && MINOR << 4 && REVISION */
#define MJB_VERSION_MAJOR 1
#define MJB_VERSION_MINOR 0
#define MJB_VERSION_REVISION 0

#define MJB_UNICODE_VERSION "13.0"
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
 * A unicode codepoint
 * [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mjb_codepoint;

#define MJB_CODEPOINT_MIN 0x0
#define MJB_CODEPOINT_MAX 0x10FFFF /* Maximum valid unicode code point */
#define MJB_CODEPOINT_REPLACEMENT 0xFFFD /* The character used when there is invalid data */

typedef enum mjb_block_name {
${blocks.map((value: Block, index: number) => `    ${value.enumName} = ${index}`).join(',\n')}
} mjb_block_name;

#define MJB_BLOCK_NUM ${blocks.length}

/*
 * Unicode block
 * [see: https://www.unicode.org/glossary/#block]
 */
typedef struct mjb_block {
    char *name;
    uint32_t start;
    uint32_t end;
} mjb_block;

/*
 * Unicode codepoint general category
 * [see: https://www.unicode.org/glossary/#general_category]
 */
typedef enum mjb_category {
${categoryEnums.join('\n')}
} mjb_category;

#define MJB_CATEGORY_COUNT ${categoryEnums.length}

/*
 * Unicode plane
 * [see: https://www.unicode.org/glossary/#plane]
 */
typedef enum mjb_plane {
    MJB_PLANE_BMP = 0,
    MJB_PLANE_SMP = 1,
    MJB_PLANE_SIP = 2,
    MJB_PLANE_TIP = 3,
    MJB_PLANE_SSP = 14,
    MJB_PLANE_PUA_A = 15,
    MJB_PLANE_PUA_B = 16
} mjb_plane;

#define MJB_PLANE_NUM 17 /* 17 planes */
#define MJB_PLANE_SIZE 65536 /* 2^16 code points per plane */

/*
 * Unicode encoding
 * [see: https://www.unicode.org/glossary/#character_encoding_scheme]
 */
typedef enum mjb_encoding {
    MJB_ENCODING_UNKNOWN = 0,
    MJB_ENCODING_ASCII = 0x1,
    MJB_ENCODING_UTF_8 = 0x2,
    MJB_ENCODING_UTF_16 = 0x4,
    MJB_ENCODING_UTF_16_BE = 0x8,
    MJB_ENCODING_UTF_16_LE = 0x10,
    MJB_ENCODING_UTF_32 = 0x20,
    MJB_ENCODING_UTF_32_BE = 0x40,
    MJB_ENCODING_UTF_32_LE = 0x80
} mjb_encoding;

/*
 * Normalization form
 * [see: https://www.unicode.org/glossary/#normalization_form]
 */
typedef enum mjb_normalization {
    MJB_NORMALIZATION_NFD = 0, /* Canonical decomposition and ordering */
    MJB_NORMALIZATION_NFC = 1, /* Composition after canonical decomposition and ordering */
    MJB_NORMALIZATION_NFKD = 2, /* Compatible decomposition and ordering */
    MJB_NORMALIZATION_NFKC = 3 /* Composition after compatible decomposition and ordering */
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
    unsigned char name[128];
    unsigned short block;
    mjb_category category;
    unsigned short combining;
    unsigned short bidirectional;
    unsigned char decimal[128];
    unsigned char digit[128];
    unsigned char numeric[128];
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
