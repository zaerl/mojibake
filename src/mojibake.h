/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_MOJIBAKE_H
#define MJB_MOJIBAKE_H

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
    MJB_BLOCK_BASIC_LATIN = 0,
    MJB_BLOCK_LATIN_1_SUPPLEMENT = 1,
    MJB_BLOCK_LATIN_EXTENDED_A = 2,
    MJB_BLOCK_LATIN_EXTENDED_B = 3,
    MJB_BLOCK_IPA_EXTENSIONS = 4,
    MJB_BLOCK_SPACING_MODIFIER_LETTERS = 5,
    MJB_BLOCK_COMBINING_DIACRITICAL_MARKS = 6,
    MJB_BLOCK_GREEK_AND_COPTIC = 7,
    MJB_BLOCK_CYRILLIC = 8,
    MJB_BLOCK_CYRILLIC_SUPPLEMENT = 9,
    MJB_BLOCK_ARMENIAN = 10,
    MJB_BLOCK_HEBREW = 11,
    MJB_BLOCK_ARABIC = 12,
    MJB_BLOCK_SYRIAC = 13,
    MJB_BLOCK_ARABIC_SUPPLEMENT = 14,
    MJB_BLOCK_THAANA = 15,
    MJB_BLOCK_NKO = 16,
    MJB_BLOCK_SAMARITAN = 17,
    MJB_BLOCK_MANDAIC = 18,
    MJB_BLOCK_SYRIAC_SUPPLEMENT = 19,
    MJB_BLOCK_ARABIC_EXTENDED_A = 20,
    MJB_BLOCK_DEVANAGARI = 21,
    MJB_BLOCK_BENGALI = 22,
    MJB_BLOCK_GURMUKHI = 23,
    MJB_BLOCK_GUJARATI = 24,
    MJB_BLOCK_ORIYA = 25,
    MJB_BLOCK_TAMIL = 26,
    MJB_BLOCK_TELUGU = 27,
    MJB_BLOCK_KANNADA = 28,
    MJB_BLOCK_MALAYALAM = 29,
    MJB_BLOCK_SINHALA = 30,
    MJB_BLOCK_THAI = 31,
    MJB_BLOCK_LAO = 32,
    MJB_BLOCK_TIBETAN = 33,
    MJB_BLOCK_MYANMAR = 34,
    MJB_BLOCK_GEORGIAN = 35,
    MJB_BLOCK_HANGUL_JAMO = 36,
    MJB_BLOCK_ETHIOPIC = 37,
    MJB_BLOCK_ETHIOPIC_SUPPLEMENT = 38,
    MJB_BLOCK_CHEROKEE = 39,
    MJB_BLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS = 40,
    MJB_BLOCK_OGHAM = 41,
    MJB_BLOCK_RUNIC = 42,
    MJB_BLOCK_TAGALOG = 43,
    MJB_BLOCK_HANUNOO = 44,
    MJB_BLOCK_BUHID = 45,
    MJB_BLOCK_TAGBANWA = 46,
    MJB_BLOCK_KHMER = 47,
    MJB_BLOCK_MONGOLIAN = 48,
    MJB_BLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED = 49,
    MJB_BLOCK_LIMBU = 50,
    MJB_BLOCK_TAI_LE = 51,
    MJB_BLOCK_NEW_TAI_LUE = 52,
    MJB_BLOCK_KHMER_SYMBOLS = 53,
    MJB_BLOCK_BUGINESE = 54,
    MJB_BLOCK_TAI_THAM = 55,
    MJB_BLOCK_COMBINING_DIACRITICAL_MARKS_EXTENDED = 56,
    MJB_BLOCK_BALINESE = 57,
    MJB_BLOCK_SUNDANESE = 58,
    MJB_BLOCK_BATAK = 59,
    MJB_BLOCK_LEPCHA = 60,
    MJB_BLOCK_OL_CHIKI = 61,
    MJB_BLOCK_CYRILLIC_EXTENDED_C = 62,
    MJB_BLOCK_GEORGIAN_EXTENDED = 63,
    MJB_BLOCK_SUNDANESE_SUPPLEMENT = 64,
    MJB_BLOCK_VEDIC_EXTENSIONS = 65,
    MJB_BLOCK_PHONETIC_EXTENSIONS = 66,
    MJB_BLOCK_PHONETIC_EXTENSIONS_SUPPLEMENT = 67,
    MJB_BLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT = 68,
    MJB_BLOCK_LATIN_EXTENDED_ADDITIONAL = 69,
    MJB_BLOCK_GREEK_EXTENDED = 70,
    MJB_BLOCK_GENERAL_PUNCTUATION = 71,
    MJB_BLOCK_SUPERSCRIPTS_AND_SUBSCRIPTS = 72,
    MJB_BLOCK_CURRENCY_SYMBOLS = 73,
    MJB_BLOCK_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS = 74,
    MJB_BLOCK_LETTERLIKE_SYMBOLS = 75,
    MJB_BLOCK_NUMBER_FORMS = 76,
    MJB_BLOCK_ARROWS = 77,
    MJB_BLOCK_MATHEMATICAL_OPERATORS = 78,
    MJB_BLOCK_MISCELLANEOUS_TECHNICAL = 79,
    MJB_BLOCK_CONTROL_PICTURES = 80,
    MJB_BLOCK_OPTICAL_CHARACTER_RECOGNITION = 81,
    MJB_BLOCK_ENCLOSED_ALPHANUMERICS = 82,
    MJB_BLOCK_BOX_DRAWING = 83,
    MJB_BLOCK_BLOCK_ELEMENTS = 84,
    MJB_BLOCK_GEOMETRIC_SHAPES = 85,
    MJB_BLOCK_MISCELLANEOUS_SYMBOLS = 86,
    MJB_BLOCK_DINGBATS = 87,
    MJB_BLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A = 88,
    MJB_BLOCK_SUPPLEMENTAL_ARROWS_A = 89,
    MJB_BLOCK_BRAILLE_PATTERNS = 90,
    MJB_BLOCK_SUPPLEMENTAL_ARROWS_B = 91,
    MJB_BLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B = 92,
    MJB_BLOCK_SUPPLEMENTAL_MATHEMATICAL_OPERATORS = 93,
    MJB_BLOCK_MISCELLANEOUS_SYMBOLS_AND_ARROWS = 94,
    MJB_BLOCK_GLAGOLITIC = 95,
    MJB_BLOCK_LATIN_EXTENDED_C = 96,
    MJB_BLOCK_COPTIC = 97,
    MJB_BLOCK_GEORGIAN_SUPPLEMENT = 98,
    MJB_BLOCK_TIFINAGH = 99,
    MJB_BLOCK_ETHIOPIC_EXTENDED = 100,
    MJB_BLOCK_CYRILLIC_EXTENDED_A = 101,
    MJB_BLOCK_SUPPLEMENTAL_PUNCTUATION = 102,
    MJB_BLOCK_CJK_RADICALS_SUPPLEMENT = 103,
    MJB_BLOCK_KANGXI_RADICALS = 104,
    MJB_BLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS = 105,
    MJB_BLOCK_CJK_SYMBOLS_AND_PUNCTUATION = 106,
    MJB_BLOCK_HIRAGANA = 107,
    MJB_BLOCK_KATAKANA = 108,
    MJB_BLOCK_BOPOMOFO = 109,
    MJB_BLOCK_HANGUL_COMPATIBILITY_JAMO = 110,
    MJB_BLOCK_KANBUN = 111,
    MJB_BLOCK_BOPOMOFO_EXTENDED = 112,
    MJB_BLOCK_CJK_STROKES = 113,
    MJB_BLOCK_KATAKANA_PHONETIC_EXTENSIONS = 114,
    MJB_BLOCK_ENCLOSED_CJK_LETTERS_AND_MONTHS = 115,
    MJB_BLOCK_CJK_COMPATIBILITY = 116,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A = 117,
    MJB_BLOCK_YIJING_HEXAGRAM_SYMBOLS = 118,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS = 119,
    MJB_BLOCK_YI_SYLLABLES = 120,
    MJB_BLOCK_YI_RADICALS = 121,
    MJB_BLOCK_LISU = 122,
    MJB_BLOCK_VAI = 123,
    MJB_BLOCK_CYRILLIC_EXTENDED_B = 124,
    MJB_BLOCK_BAMUM = 125,
    MJB_BLOCK_MODIFIER_TONE_LETTERS = 126,
    MJB_BLOCK_LATIN_EXTENDED_D = 127,
    MJB_BLOCK_SYLOTI_NAGRI = 128,
    MJB_BLOCK_COMMON_INDIC_NUMBER_FORMS = 129,
    MJB_BLOCK_PHAGS_PA = 130,
    MJB_BLOCK_SAURASHTRA = 131,
    MJB_BLOCK_DEVANAGARI_EXTENDED = 132,
    MJB_BLOCK_KAYAH_LI = 133,
    MJB_BLOCK_REJANG = 134,
    MJB_BLOCK_HANGUL_JAMO_EXTENDED_A = 135,
    MJB_BLOCK_JAVANESE = 136,
    MJB_BLOCK_MYANMAR_EXTENDED_B = 137,
    MJB_BLOCK_CHAM = 138,
    MJB_BLOCK_MYANMAR_EXTENDED_A = 139,
    MJB_BLOCK_TAI_VIET = 140,
    MJB_BLOCK_MEETEI_MAYEK_EXTENSIONS = 141,
    MJB_BLOCK_ETHIOPIC_EXTENDED_A = 142,
    MJB_BLOCK_LATIN_EXTENDED_E = 143,
    MJB_BLOCK_CHEROKEE_SUPPLEMENT = 144,
    MJB_BLOCK_MEETEI_MAYEK = 145,
    MJB_BLOCK_HANGUL_SYLLABLES = 146,
    MJB_BLOCK_HANGUL_JAMO_EXTENDED_B = 147,
    MJB_BLOCK_HIGH_SURROGATES = 148,
    MJB_BLOCK_HIGH_PRIVATE_USE_SURROGATES = 149,
    MJB_BLOCK_LOW_SURROGATES = 150,
    MJB_BLOCK_PRIVATE_USE_AREA = 151,
    MJB_BLOCK_CJK_COMPATIBILITY_IDEOGRAPHS = 152,
    MJB_BLOCK_ALPHABETIC_PRESENTATION_FORMS = 153,
    MJB_BLOCK_ARABIC_PRESENTATION_FORMS_A = 154,
    MJB_BLOCK_VARIATION_SELECTORS = 155,
    MJB_BLOCK_VERTICAL_FORMS = 156,
    MJB_BLOCK_COMBINING_HALF_MARKS = 157,
    MJB_BLOCK_CJK_COMPATIBILITY_FORMS = 158,
    MJB_BLOCK_SMALL_FORM_VARIANTS = 159,
    MJB_BLOCK_ARABIC_PRESENTATION_FORMS_B = 160,
    MJB_BLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS = 161,
    MJB_BLOCK_SPECIALS = 162,
    MJB_BLOCK_LINEAR_B_SYLLABARY = 163,
    MJB_BLOCK_LINEAR_B_IDEOGRAMS = 164,
    MJB_BLOCK_AEGEAN_NUMBERS = 165,
    MJB_BLOCK_ANCIENT_GREEK_NUMBERS = 166,
    MJB_BLOCK_ANCIENT_SYMBOLS = 167,
    MJB_BLOCK_PHAISTOS_DISC = 168,
    MJB_BLOCK_LYCIAN = 169,
    MJB_BLOCK_CARIAN = 170,
    MJB_BLOCK_COPTIC_EPACT_NUMBERS = 171,
    MJB_BLOCK_OLD_ITALIC = 172,
    MJB_BLOCK_GOTHIC = 173,
    MJB_BLOCK_OLD_PERMIC = 174,
    MJB_BLOCK_UGARITIC = 175,
    MJB_BLOCK_OLD_PERSIAN = 176,
    MJB_BLOCK_DESERET = 177,
    MJB_BLOCK_SHAVIAN = 178,
    MJB_BLOCK_OSMANYA = 179,
    MJB_BLOCK_OSAGE = 180,
    MJB_BLOCK_ELBASAN = 181,
    MJB_BLOCK_CAUCASIAN_ALBANIAN = 182,
    MJB_BLOCK_LINEAR_A = 183,
    MJB_BLOCK_CYPRIOT_SYLLABARY = 184,
    MJB_BLOCK_IMPERIAL_ARAMAIC = 185,
    MJB_BLOCK_PALMYRENE = 186,
    MJB_BLOCK_NABATAEAN = 187,
    MJB_BLOCK_HATRAN = 188,
    MJB_BLOCK_PHOENICIAN = 189,
    MJB_BLOCK_LYDIAN = 190,
    MJB_BLOCK_MEROITIC_HIEROGLYPHS = 191,
    MJB_BLOCK_MEROITIC_CURSIVE = 192,
    MJB_BLOCK_KHAROSHTHI = 193,
    MJB_BLOCK_OLD_SOUTH_ARABIAN = 194,
    MJB_BLOCK_OLD_NORTH_ARABIAN = 195,
    MJB_BLOCK_MANICHAEAN = 196,
    MJB_BLOCK_AVESTAN = 197,
    MJB_BLOCK_INSCRIPTIONAL_PARTHIAN = 198,
    MJB_BLOCK_INSCRIPTIONAL_PAHLAVI = 199,
    MJB_BLOCK_PSALTER_PAHLAVI = 200,
    MJB_BLOCK_OLD_TURKIC = 201,
    MJB_BLOCK_OLD_HUNGARIAN = 202,
    MJB_BLOCK_HANIFI_ROHINGYA = 203,
    MJB_BLOCK_RUMI_NUMERAL_SYMBOLS = 204,
    MJB_BLOCK_YEZIDI = 205,
    MJB_BLOCK_OLD_SOGDIAN = 206,
    MJB_BLOCK_SOGDIAN = 207,
    MJB_BLOCK_CHORASMIAN = 208,
    MJB_BLOCK_ELYMAIC = 209,
    MJB_BLOCK_BRAHMI = 210,
    MJB_BLOCK_KAITHI = 211,
    MJB_BLOCK_SORA_SOMPENG = 212,
    MJB_BLOCK_CHAKMA = 213,
    MJB_BLOCK_MAHAJANI = 214,
    MJB_BLOCK_SHARADA = 215,
    MJB_BLOCK_SINHALA_ARCHAIC_NUMBERS = 216,
    MJB_BLOCK_KHOJKI = 217,
    MJB_BLOCK_MULTANI = 218,
    MJB_BLOCK_KHUDAWADI = 219,
    MJB_BLOCK_GRANTHA = 220,
    MJB_BLOCK_NEWA = 221,
    MJB_BLOCK_TIRHUTA = 222,
    MJB_BLOCK_SIDDHAM = 223,
    MJB_BLOCK_MODI = 224,
    MJB_BLOCK_MONGOLIAN_SUPPLEMENT = 225,
    MJB_BLOCK_TAKRI = 226,
    MJB_BLOCK_AHOM = 227,
    MJB_BLOCK_DOGRA = 228,
    MJB_BLOCK_WARANG_CITI = 229,
    MJB_BLOCK_DIVES_AKURU = 230,
    MJB_BLOCK_NANDINAGARI = 231,
    MJB_BLOCK_ZANABAZAR_SQUARE = 232,
    MJB_BLOCK_SOYOMBO = 233,
    MJB_BLOCK_PAU_CIN_HAU = 234,
    MJB_BLOCK_BHAIKSUKI = 235,
    MJB_BLOCK_MARCHEN = 236,
    MJB_BLOCK_MASARAM_GONDI = 237,
    MJB_BLOCK_GUNJALA_GONDI = 238,
    MJB_BLOCK_MAKASAR = 239,
    MJB_BLOCK_LISU_SUPPLEMENT = 240,
    MJB_BLOCK_TAMIL_SUPPLEMENT = 241,
    MJB_BLOCK_CUNEIFORM = 242,
    MJB_BLOCK_CUNEIFORM_NUMBERS_AND_PUNCTUATION = 243,
    MJB_BLOCK_EARLY_DYNASTIC_CUNEIFORM = 244,
    MJB_BLOCK_EGYPTIAN_HIEROGLYPHS = 245,
    MJB_BLOCK_EGYPTIAN_HIEROGLYPH_FORMAT_CONTROLS = 246,
    MJB_BLOCK_ANATOLIAN_HIEROGLYPHS = 247,
    MJB_BLOCK_BAMUM_SUPPLEMENT = 248,
    MJB_BLOCK_MRO = 249,
    MJB_BLOCK_BASSA_VAH = 250,
    MJB_BLOCK_PAHAWH_HMONG = 251,
    MJB_BLOCK_MEDEFAIDRIN = 252,
    MJB_BLOCK_MIAO = 253,
    MJB_BLOCK_IDEOGRAPHIC_SYMBOLS_AND_PUNCTUATION = 254,
    MJB_BLOCK_TANGUT = 255,
    MJB_BLOCK_TANGUT_COMPONENTS = 256,
    MJB_BLOCK_KHITAN_SMALL_SCRIPT = 257,
    MJB_BLOCK_TANGUT_SUPPLEMENT = 258,
    MJB_BLOCK_KANA_SUPPLEMENT = 259,
    MJB_BLOCK_KANA_EXTENDED_A = 260,
    MJB_BLOCK_SMALL_KANA_EXTENSION = 261,
    MJB_BLOCK_NUSHU = 262,
    MJB_BLOCK_DUPLOYAN = 263,
    MJB_BLOCK_SHORTHAND_FORMAT_CONTROLS = 264,
    MJB_BLOCK_BYZANTINE_MUSICAL_SYMBOLS = 265,
    MJB_BLOCK_MUSICAL_SYMBOLS = 266,
    MJB_BLOCK_ANCIENT_GREEK_MUSICAL_NOTATION = 267,
    MJB_BLOCK_MAYAN_NUMERALS = 268,
    MJB_BLOCK_TAI_XUAN_JING_SYMBOLS = 269,
    MJB_BLOCK_COUNTING_ROD_NUMERALS = 270,
    MJB_BLOCK_MATHEMATICAL_ALPHANUMERIC_SYMBOLS = 271,
    MJB_BLOCK_SUTTON_SIGNWRITING = 272,
    MJB_BLOCK_GLAGOLITIC_SUPPLEMENT = 273,
    MJB_BLOCK_NYIAKENG_PUACHUE_HMONG = 274,
    MJB_BLOCK_WANCHO = 275,
    MJB_BLOCK_MENDE_KIKAKUI = 276,
    MJB_BLOCK_ADLAM = 277,
    MJB_BLOCK_INDIC_SIYAQ_NUMBERS = 278,
    MJB_BLOCK_OTTOMAN_SIYAQ_NUMBERS = 279,
    MJB_BLOCK_ARABIC_MATHEMATICAL_ALPHABETIC_SYMBOLS = 280,
    MJB_BLOCK_MAHJONG_TILES = 281,
    MJB_BLOCK_DOMINO_TILES = 282,
    MJB_BLOCK_PLAYING_CARDS = 283,
    MJB_BLOCK_ENCLOSED_ALPHANUMERIC_SUPPLEMENT = 284,
    MJB_BLOCK_ENCLOSED_IDEOGRAPHIC_SUPPLEMENT = 285,
    MJB_BLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS = 286,
    MJB_BLOCK_EMOTICONS = 287,
    MJB_BLOCK_ORNAMENTAL_DINGBATS = 288,
    MJB_BLOCK_TRANSPORT_AND_MAP_SYMBOLS = 289,
    MJB_BLOCK_ALCHEMICAL_SYMBOLS = 290,
    MJB_BLOCK_GEOMETRIC_SHAPES_EXTENDED = 291,
    MJB_BLOCK_SUPPLEMENTAL_ARROWS_C = 292,
    MJB_BLOCK_SUPPLEMENTAL_SYMBOLS_AND_PICTOGRAPHS = 293,
    MJB_BLOCK_CHESS_SYMBOLS = 294,
    MJB_BLOCK_SYMBOLS_AND_PICTOGRAPHS_EXTENDED_A = 295,
    MJB_BLOCK_SYMBOLS_FOR_LEGACY_COMPUTING = 296,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B = 297,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C = 298,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D = 299,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_E = 300,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_F = 301,
    MJB_BLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT = 302,
    MJB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_G = 303,
    MJB_BLOCK_TAGS = 304,
    MJB_BLOCK_VARIATION_SELECTORS_SUPPLEMENT = 305,
    MJB_BLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_A = 306,
    MJB_BLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_B = 307
} mjb_block;

#define MJB_BLOCK_NUM 308

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
    MJB_CATEGORY_LU = 0x00000001, /* 0 (Lu) Letter, uppercase */
    MJB_CATEGORY_LL = 0x00000002, /* 1 (Ll) Letter, lowercase */
    MJB_CATEGORY_LT = 0x00000004, /* 2 (Lt) Letter, titlecase */
    MJB_CATEGORY_LM = 0x00000008, /* 3 (Lm) Letter, modifier */
    MJB_CATEGORY_LO = 0x00000010, /* 4 (Lo) Letter, other */
    MJB_CATEGORY_MN = 0x00000020, /* 5 (Mn) Mark, non-spacing */
    MJB_CATEGORY_MC = 0x00000040, /* 6 (Mc) Mark, spacing combining */
    MJB_CATEGORY_ME = 0x00000080, /* 7 (Me) Mark, enclosing */
    MJB_CATEGORY_ND = 0x00000100, /* 8 (Nd) Number, decimal digit */
    MJB_CATEGORY_NL = 0x00000200, /* 9 (Nl) Number, letter */
    MJB_CATEGORY_NO = 0x00000400, /* 10 (No) Number, other */
    MJB_CATEGORY_PC = 0x00000800, /* 11 (Pc) Punctuation, connector */
    MJB_CATEGORY_PD = 0x00001000, /* 12 (Pd) Punctuation, dash */
    MJB_CATEGORY_PS = 0x00002000, /* 13 (Ps) Punctuation, open */
    MJB_CATEGORY_PE = 0x00004000, /* 14 (Pe) Punctuation, close */
    MJB_CATEGORY_PI = 0x00008000, /* 15 (Pi) Punctuation, initial quote */
    MJB_CATEGORY_PF = 0x00010000, /* 16 (Pf) Punctuation, final quote */
    MJB_CATEGORY_PO = 0x00020000, /* 17 (Po) Punctuation, other */
    MJB_CATEGORY_SM = 0x00040000, /* 18 (Sm) Symbol, math */
    MJB_CATEGORY_SC = 0x00080000, /* 19 (Sc) Symbol, currency */
    MJB_CATEGORY_SK = 0x00100000, /* 20 (Sk) Symbol, modifier */
    MJB_CATEGORY_SO = 0x00200000, /* 21 (So) Symbol, other */
    MJB_CATEGORY_ZS = 0x00400000, /* 22 (Zs) Separator, space */
    MJB_CATEGORY_ZL = 0x00800000, /* 23 (Zl) Separator, line */
    MJB_CATEGORY_ZP = 0x01000000, /* 24 (Zp) Separator, paragraph */
    MJB_CATEGORY_CC = 0x02000000, /* 25 (Cc) Other, control */
    MJB_CATEGORY_CF = 0x04000000, /* 26 (Cf) Other, format */
    MJB_CATEGORY_CS = 0x08000000, /* 27 (Cs) Other, surrogate */
    MJB_CATEGORY_CO = 0x10000000, /* 28 (Co) Other, private use */
    MJB_CATEGORY_CN = 0x20000000 /* 29 (Cn) Other, not assigned */
} mjb_category;

#define MJB_CATEGORY_COUNT 30

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
    MJB_DECOMPOSITION_CANONICAL = 0,
    MJB_DECOMPOSITION_CIRCLE = 1,
    MJB_DECOMPOSITION_COMPAT = 2,
    MJB_DECOMPOSITION_FINAL = 3,
    MJB_DECOMPOSITION_FONT = 4,
    MJB_DECOMPOSITION_FRACTION = 5,
    MJB_DECOMPOSITION_INITIAL = 6,
    MJB_DECOMPOSITION_ISOLATED = 7,
    MJB_DECOMPOSITION_MEDIAL = 8,
    MJB_DECOMPOSITION_NARROW = 9,
    MJB_DECOMPOSITION_NOBREAK = 10,
    MJB_DECOMPOSITION_SMALL = 11,
    MJB_DECOMPOSITION_SQUARE = 12,
    MJB_DECOMPOSITION_SUB = 13,
    MJB_DECOMPOSITION_SUPER = 14,
    MJB_DECOMPOSITION_VERTICAL = 15,
    MJB_DECOMPOSITION_WIDE = 16
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

/* Initialize the library */
bool mjb_initialize(mojibake **mjb);

/* Initialize the library with custom values */
bool mjb_initialize_v2(mojibake **mjb, mjb_alloc_fn alloc_fn, mjb_realloc_fn realloc_fn, mjb_free_fn free_fn);

/* The library is ready */
bool mjb_ready(mojibake *mjb);

/* Allocate and zero memory */
void *mjb_alloc(mojibake *mjb, size_t size);

/* Reallocate memory */
void *mjb_realloc(mojibake *mjb, void *ptr, size_t new_size);

/* Free memory */
void mjb_free(mojibake *mjb, void *ptr);

/* Output the current library version (MJB_VERSION) */
char *mjb_version(void);

/* Output the current library version number (MJB_VERSION_NUMBER) */
unsigned int mjb_version_number(void);

/* Output the current supported unicode version (MJB_UNICODE_VERSION) */
char *mjb_unicode_version(void);

/* Return true if the plane is valid */
bool mjb_plane_is_valid(mjb_plane plane);

/* Return the name of a plane, NULL if the place specified is not valid */
const char *mjb_plane_name(mjb_plane plane, bool abbreviation);

/* Return the string encoding (the most probable) */
mjb_encoding mjb_string_encoding(const char *buffer, size_t size);

/* Return true if the string is encoded in UTF-8 */
bool mjb_string_is_utf8(const char *buffer, size_t size);

/* Return true if the string is encoded in ASCII */
bool mjb_string_is_ascii(const char *buffer, size_t size);

/* Return true if the codepoint is valid */
bool mjb_codepoint_is_valid(mojibake *mjb, mjb_codepoint codepoint);

/* Return the codepoint character */
bool mjb_codepoint_character(mojibake *mjb, mjb_character *character, mjb_codepoint codepoint);

/* Return true if the codepoint has the category */
bool mjb_codepoint_is(mojibake *mjb, mjb_codepoint codepoint, mjb_category category);

/* Return true if the codepoint is graphic */
bool mjb_codepoint_is_graphic(mojibake *mjb, mjb_codepoint codepoint);

/* Return the codepoint lowercase codepoint */
mjb_codepoint mjb_codepoint_to_lowercase(mojibake *mjb, mjb_codepoint codepoint);

/* Return the codepoint uppercase codepoint */
mjb_codepoint mjb_codepoint_to_uppercase(mojibake *mjb, mjb_codepoint codepoint);

/* Return the codepoint titlecase codepoint */
mjb_codepoint mjb_codepoint_to_titlecase(mojibake *mjb, mjb_codepoint codepoint);

/* Normalize a string */
void *mjb_normalize(mojibake *mjb, void *source, size_t source_size, size_t *output_size, mjb_encoding encoding, mjb_normalization form);

#ifdef __cplusplus
}
#endif

#endif /* MJB_MOJIBAKE_H */
