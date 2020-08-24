/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MB_MOJIBAKE_H
#define MB_MOJIBAKE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MB_VERSION "1.0.0"
#define MB_VERSION_NUMBER 0x100 /* MAJOR << 8 && MINOR << 4 && REVISION */
#define MB_VERSION_MAJOR 1
#define MB_VERSION_MINOR 0
#define MB_VERSION_REVISION 0

#define MB_UNICODE_VERSION "13.0"
#define MB_UNICODE_VERSION_MAJOR 13
#define MB_UNICODE_VERSION_MINOR 0

#ifndef MB_EXTERN
#define MB_EXTERN extern
#endif

#ifndef MB_EXPORT
#define MB_EXPORT __attribute__((visibility("default")))
#endif

/* See c standard memory allocation functions */
typedef void* (*mb_alloc)(size_t size);
typedef void* (*mb_realloc)(void* ptr, size_t new_size);
typedef void (*mb_free)(void* ptr);

/*
 A unicode codepoint
 [see: https://www.unicode.org/glossary/#code_point]
 */
typedef uint32_t mb_codepoint;

#define MB_CODEPOINT_MIN 0x0
#define MB_CODEPOINT_MAX 0x10FFFF /* Maximum valid unicode code point */
#define MB_CODEPOINT_REPLACEMENT 0xFFFD /* The character used when there is invalid data */

#define MB_BLOCK_NUM 308

typedef enum mb_block_name {
    MB_BLOCK_BASIC_LATIN = 0,
    MB_BLOCK_LATIN_1_SUPPLEMENT = 1,
    MB_BLOCK_LATIN_EXTENDED_A = 2,
    MB_BLOCK_LATIN_EXTENDED_B = 3,
    MB_BLOCK_IPA_EXTENSIONS = 4,
    MB_BLOCK_SPACING_MODIFIER_LETTERS = 5,
    MB_BLOCK_COMBINING_DIACRITICAL_MARKS = 6,
    MB_BLOCK_GREEK_AND_COPTIC = 7,
    MB_BLOCK_CYRILLIC = 8,
    MB_BLOCK_CYRILLIC_SUPPLEMENT = 9,
    MB_BLOCK_ARMENIAN = 10,
    MB_BLOCK_HEBREW = 11,
    MB_BLOCK_ARABIC = 12,
    MB_BLOCK_SYRIAC = 13,
    MB_BLOCK_ARABIC_SUPPLEMENT = 14,
    MB_BLOCK_THAANA = 15,
    MB_BLOCK_NKO = 16,
    MB_BLOCK_SAMARITAN = 17,
    MB_BLOCK_MANDAIC = 18,
    MB_BLOCK_SYRIAC_SUPPLEMENT = 19,
    MB_BLOCK_ARABIC_EXTENDED_A = 20,
    MB_BLOCK_DEVANAGARI = 21,
    MB_BLOCK_BENGALI = 22,
    MB_BLOCK_GURMUKHI = 23,
    MB_BLOCK_GUJARATI = 24,
    MB_BLOCK_ORIYA = 25,
    MB_BLOCK_TAMIL = 26,
    MB_BLOCK_TELUGU = 27,
    MB_BLOCK_KANNADA = 28,
    MB_BLOCK_MALAYALAM = 29,
    MB_BLOCK_SINHALA = 30,
    MB_BLOCK_THAI = 31,
    MB_BLOCK_LAO = 32,
    MB_BLOCK_TIBETAN = 33,
    MB_BLOCK_MYANMAR = 34,
    MB_BLOCK_GEORGIAN = 35,
    MB_BLOCK_HANGUL_JAMO = 36,
    MB_BLOCK_ETHIOPIC = 37,
    MB_BLOCK_ETHIOPIC_SUPPLEMENT = 38,
    MB_BLOCK_CHEROKEE = 39,
    MB_BLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS = 40,
    MB_BLOCK_OGHAM = 41,
    MB_BLOCK_RUNIC = 42,
    MB_BLOCK_TAGALOG = 43,
    MB_BLOCK_HANUNOO = 44,
    MB_BLOCK_BUHID = 45,
    MB_BLOCK_TAGBANWA = 46,
    MB_BLOCK_KHMER = 47,
    MB_BLOCK_MONGOLIAN = 48,
    MB_BLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED = 49,
    MB_BLOCK_LIMBU = 50,
    MB_BLOCK_TAI_LE = 51,
    MB_BLOCK_NEW_TAI_LUE = 52,
    MB_BLOCK_KHMER_SYMBOLS = 53,
    MB_BLOCK_BUGINESE = 54,
    MB_BLOCK_TAI_THAM = 55,
    MB_BLOCK_COMBINING_DIACRITICAL_MARKS_EXTENDED = 56,
    MB_BLOCK_BALINESE = 57,
    MB_BLOCK_SUNDANESE = 58,
    MB_BLOCK_BATAK = 59,
    MB_BLOCK_LEPCHA = 60,
    MB_BLOCK_OL_CHIKI = 61,
    MB_BLOCK_CYRILLIC_EXTENDED_C = 62,
    MB_BLOCK_GEORGIAN_EXTENDED = 63,
    MB_BLOCK_SUNDANESE_SUPPLEMENT = 64,
    MB_BLOCK_VEDIC_EXTENSIONS = 65,
    MB_BLOCK_PHONETIC_EXTENSIONS = 66,
    MB_BLOCK_PHONETIC_EXTENSIONS_SUPPLEMENT = 67,
    MB_BLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT = 68,
    MB_BLOCK_LATIN_EXTENDED_ADDITIONAL = 69,
    MB_BLOCK_GREEK_EXTENDED = 70,
    MB_BLOCK_GENERAL_PUNCTUATION = 71,
    MB_BLOCK_SUPERSCRIPTS_AND_SUBSCRIPTS = 72,
    MB_BLOCK_CURRENCY_SYMBOLS = 73,
    MB_BLOCK_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS = 74,
    MB_BLOCK_LETTERLIKE_SYMBOLS = 75,
    MB_BLOCK_NUMBER_FORMS = 76,
    MB_BLOCK_ARROWS = 77,
    MB_BLOCK_MATHEMATICAL_OPERATORS = 78,
    MB_BLOCK_MISCELLANEOUS_TECHNICAL = 79,
    MB_BLOCK_CONTROL_PICTURES = 80,
    MB_BLOCK_OPTICAL_CHARACTER_RECOGNITION = 81,
    MB_BLOCK_ENCLOSED_ALPHANUMERICS = 82,
    MB_BLOCK_BOX_DRAWING = 83,
    MB_BLOCK_BLOCK_ELEMENTS = 84,
    MB_BLOCK_GEOMETRIC_SHAPES = 85,
    MB_BLOCK_MISCELLANEOUS_SYMBOLS = 86,
    MB_BLOCK_DINGBATS = 87,
    MB_BLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A = 88,
    MB_BLOCK_SUPPLEMENTAL_ARROWS_A = 89,
    MB_BLOCK_BRAILLE_PATTERNS = 90,
    MB_BLOCK_SUPPLEMENTAL_ARROWS_B = 91,
    MB_BLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B = 92,
    MB_BLOCK_SUPPLEMENTAL_MATHEMATICAL_OPERATORS = 93,
    MB_BLOCK_MISCELLANEOUS_SYMBOLS_AND_ARROWS = 94,
    MB_BLOCK_GLAGOLITIC = 95,
    MB_BLOCK_LATIN_EXTENDED_C = 96,
    MB_BLOCK_COPTIC = 97,
    MB_BLOCK_GEORGIAN_SUPPLEMENT = 98,
    MB_BLOCK_TIFINAGH = 99,
    MB_BLOCK_ETHIOPIC_EXTENDED = 100,
    MB_BLOCK_CYRILLIC_EXTENDED_A = 101,
    MB_BLOCK_SUPPLEMENTAL_PUNCTUATION = 102,
    MB_BLOCK_CJK_RADICALS_SUPPLEMENT = 103,
    MB_BLOCK_KANGXI_RADICALS = 104,
    MB_BLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS = 105,
    MB_BLOCK_CJK_SYMBOLS_AND_PUNCTUATION = 106,
    MB_BLOCK_HIRAGANA = 107,
    MB_BLOCK_KATAKANA = 108,
    MB_BLOCK_BOPOMOFO = 109,
    MB_BLOCK_HANGUL_COMPATIBILITY_JAMO = 110,
    MB_BLOCK_KANBUN = 111,
    MB_BLOCK_BOPOMOFO_EXTENDED = 112,
    MB_BLOCK_CJK_STROKES = 113,
    MB_BLOCK_KATAKANA_PHONETIC_EXTENSIONS = 114,
    MB_BLOCK_ENCLOSED_CJK_LETTERS_AND_MONTHS = 115,
    MB_BLOCK_CJK_COMPATIBILITY = 116,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A = 117,
    MB_BLOCK_YIJING_HEXAGRAM_SYMBOLS = 118,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS = 119,
    MB_BLOCK_YI_SYLLABLES = 120,
    MB_BLOCK_YI_RADICALS = 121,
    MB_BLOCK_LISU = 122,
    MB_BLOCK_VAI = 123,
    MB_BLOCK_CYRILLIC_EXTENDED_B = 124,
    MB_BLOCK_BAMUM = 125,
    MB_BLOCK_MODIFIER_TONE_LETTERS = 126,
    MB_BLOCK_LATIN_EXTENDED_D = 127,
    MB_BLOCK_SYLOTI_NAGRI = 128,
    MB_BLOCK_COMMON_INDIC_NUMBER_FORMS = 129,
    MB_BLOCK_PHAGS_PA = 130,
    MB_BLOCK_SAURASHTRA = 131,
    MB_BLOCK_DEVANAGARI_EXTENDED = 132,
    MB_BLOCK_KAYAH_LI = 133,
    MB_BLOCK_REJANG = 134,
    MB_BLOCK_HANGUL_JAMO_EXTENDED_A = 135,
    MB_BLOCK_JAVANESE = 136,
    MB_BLOCK_MYANMAR_EXTENDED_B = 137,
    MB_BLOCK_CHAM = 138,
    MB_BLOCK_MYANMAR_EXTENDED_A = 139,
    MB_BLOCK_TAI_VIET = 140,
    MB_BLOCK_MEETEI_MAYEK_EXTENSIONS = 141,
    MB_BLOCK_ETHIOPIC_EXTENDED_A = 142,
    MB_BLOCK_LATIN_EXTENDED_E = 143,
    MB_BLOCK_CHEROKEE_SUPPLEMENT = 144,
    MB_BLOCK_MEETEI_MAYEK = 145,
    MB_BLOCK_HANGUL_SYLLABLES = 146,
    MB_BLOCK_HANGUL_JAMO_EXTENDED_B = 147,
    MB_BLOCK_HIGH_SURROGATES = 148,
    MB_BLOCK_HIGH_PRIVATE_USE_SURROGATES = 149,
    MB_BLOCK_LOW_SURROGATES = 150,
    MB_BLOCK_PRIVATE_USE_AREA = 151,
    MB_BLOCK_CJK_COMPATIBILITY_IDEOGRAPHS = 152,
    MB_BLOCK_ALPHABETIC_PRESENTATION_FORMS = 153,
    MB_BLOCK_ARABIC_PRESENTATION_FORMS_A = 154,
    MB_BLOCK_VARIATION_SELECTORS = 155,
    MB_BLOCK_VERTICAL_FORMS = 156,
    MB_BLOCK_COMBINING_HALF_MARKS = 157,
    MB_BLOCK_CJK_COMPATIBILITY_FORMS = 158,
    MB_BLOCK_SMALL_FORM_VARIANTS = 159,
    MB_BLOCK_ARABIC_PRESENTATION_FORMS_B = 160,
    MB_BLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS = 161,
    MB_BLOCK_SPECIALS = 162,
    MB_BLOCK_LINEAR_B_SYLLABARY = 163,
    MB_BLOCK_LINEAR_B_IDEOGRAMS = 164,
    MB_BLOCK_AEGEAN_NUMBERS = 165,
    MB_BLOCK_ANCIENT_GREEK_NUMBERS = 166,
    MB_BLOCK_ANCIENT_SYMBOLS = 167,
    MB_BLOCK_PHAISTOS_DISC = 168,
    MB_BLOCK_LYCIAN = 169,
    MB_BLOCK_CARIAN = 170,
    MB_BLOCK_COPTIC_EPACT_NUMBERS = 171,
    MB_BLOCK_OLD_ITALIC = 172,
    MB_BLOCK_GOTHIC = 173,
    MB_BLOCK_OLD_PERMIC = 174,
    MB_BLOCK_UGARITIC = 175,
    MB_BLOCK_OLD_PERSIAN = 176,
    MB_BLOCK_DESERET = 177,
    MB_BLOCK_SHAVIAN = 178,
    MB_BLOCK_OSMANYA = 179,
    MB_BLOCK_OSAGE = 180,
    MB_BLOCK_ELBASAN = 181,
    MB_BLOCK_CAUCASIAN_ALBANIAN = 182,
    MB_BLOCK_LINEAR_A = 183,
    MB_BLOCK_CYPRIOT_SYLLABARY = 184,
    MB_BLOCK_IMPERIAL_ARAMAIC = 185,
    MB_BLOCK_PALMYRENE = 186,
    MB_BLOCK_NABATAEAN = 187,
    MB_BLOCK_HATRAN = 188,
    MB_BLOCK_PHOENICIAN = 189,
    MB_BLOCK_LYDIAN = 190,
    MB_BLOCK_MEROITIC_HIEROGLYPHS = 191,
    MB_BLOCK_MEROITIC_CURSIVE = 192,
    MB_BLOCK_KHAROSHTHI = 193,
    MB_BLOCK_OLD_SOUTH_ARABIAN = 194,
    MB_BLOCK_OLD_NORTH_ARABIAN = 195,
    MB_BLOCK_MANICHAEAN = 196,
    MB_BLOCK_AVESTAN = 197,
    MB_BLOCK_INSCRIPTIONAL_PARTHIAN = 198,
    MB_BLOCK_INSCRIPTIONAL_PAHLAVI = 199,
    MB_BLOCK_PSALTER_PAHLAVI = 200,
    MB_BLOCK_OLD_TURKIC = 201,
    MB_BLOCK_OLD_HUNGARIAN = 202,
    MB_BLOCK_HANIFI_ROHINGYA = 203,
    MB_BLOCK_RUMI_NUMERAL_SYMBOLS = 204,
    MB_BLOCK_YEZIDI = 205,
    MB_BLOCK_OLD_SOGDIAN = 206,
    MB_BLOCK_SOGDIAN = 207,
    MB_BLOCK_CHORASMIAN = 208,
    MB_BLOCK_ELYMAIC = 209,
    MB_BLOCK_BRAHMI = 210,
    MB_BLOCK_KAITHI = 211,
    MB_BLOCK_SORA_SOMPENG = 212,
    MB_BLOCK_CHAKMA = 213,
    MB_BLOCK_MAHAJANI = 214,
    MB_BLOCK_SHARADA = 215,
    MB_BLOCK_SINHALA_ARCHAIC_NUMBERS = 216,
    MB_BLOCK_KHOJKI = 217,
    MB_BLOCK_MULTANI = 218,
    MB_BLOCK_KHUDAWADI = 219,
    MB_BLOCK_GRANTHA = 220,
    MB_BLOCK_NEWA = 221,
    MB_BLOCK_TIRHUTA = 222,
    MB_BLOCK_SIDDHAM = 223,
    MB_BLOCK_MODI = 224,
    MB_BLOCK_MONGOLIAN_SUPPLEMENT = 225,
    MB_BLOCK_TAKRI = 226,
    MB_BLOCK_AHOM = 227,
    MB_BLOCK_DOGRA = 228,
    MB_BLOCK_WARANG_CITI = 229,
    MB_BLOCK_DIVES_AKURU = 230,
    MB_BLOCK_NANDINAGARI = 231,
    MB_BLOCK_ZANABAZAR_SQUARE = 232,
    MB_BLOCK_SOYOMBO = 233,
    MB_BLOCK_PAU_CIN_HAU = 234,
    MB_BLOCK_BHAIKSUKI = 235,
    MB_BLOCK_MARCHEN = 236,
    MB_BLOCK_MASARAM_GONDI = 237,
    MB_BLOCK_GUNJALA_GONDI = 238,
    MB_BLOCK_MAKASAR = 239,
    MB_BLOCK_LISU_SUPPLEMENT = 240,
    MB_BLOCK_TAMIL_SUPPLEMENT = 241,
    MB_BLOCK_CUNEIFORM = 242,
    MB_BLOCK_CUNEIFORM_NUMBERS_AND_PUNCTUATION = 243,
    MB_BLOCK_EARLY_DYNASTIC_CUNEIFORM = 244,
    MB_BLOCK_EGYPTIAN_HIEROGLYPHS = 245,
    MB_BLOCK_EGYPTIAN_HIEROGLYPH_FORMAT_CONTROLS = 246,
    MB_BLOCK_ANATOLIAN_HIEROGLYPHS = 247,
    MB_BLOCK_BAMUM_SUPPLEMENT = 248,
    MB_BLOCK_MRO = 249,
    MB_BLOCK_BASSA_VAH = 250,
    MB_BLOCK_PAHAWH_HMONG = 251,
    MB_BLOCK_MEDEFAIDRIN = 252,
    MB_BLOCK_MIAO = 253,
    MB_BLOCK_IDEOGRAPHIC_SYMBOLS_AND_PUNCTUATION = 254,
    MB_BLOCK_TANGUT = 255,
    MB_BLOCK_TANGUT_COMPONENTS = 256,
    MB_BLOCK_KHITAN_SMALL_SCRIPT = 257,
    MB_BLOCK_TANGUT_SUPPLEMENT = 258,
    MB_BLOCK_KANA_SUPPLEMENT = 259,
    MB_BLOCK_KANA_EXTENDED_A = 260,
    MB_BLOCK_SMALL_KANA_EXTENSION = 261,
    MB_BLOCK_NUSHU = 262,
    MB_BLOCK_DUPLOYAN = 263,
    MB_BLOCK_SHORTHAND_FORMAT_CONTROLS = 264,
    MB_BLOCK_BYZANTINE_MUSICAL_SYMBOLS = 265,
    MB_BLOCK_MUSICAL_SYMBOLS = 266,
    MB_BLOCK_ANCIENT_GREEK_MUSICAL_NOTATION = 267,
    MB_BLOCK_MAYAN_NUMERALS = 268,
    MB_BLOCK_TAI_XUAN_JING_SYMBOLS = 269,
    MB_BLOCK_COUNTING_ROD_NUMERALS = 270,
    MB_BLOCK_MATHEMATICAL_ALPHANUMERIC_SYMBOLS = 271,
    MB_BLOCK_SUTTON_SIGNWRITING = 272,
    MB_BLOCK_GLAGOLITIC_SUPPLEMENT = 273,
    MB_BLOCK_NYIAKENG_PUACHUE_HMONG = 274,
    MB_BLOCK_WANCHO = 275,
    MB_BLOCK_MENDE_KIKAKUI = 276,
    MB_BLOCK_ADLAM = 277,
    MB_BLOCK_INDIC_SIYAQ_NUMBERS = 278,
    MB_BLOCK_OTTOMAN_SIYAQ_NUMBERS = 279,
    MB_BLOCK_ARABIC_MATHEMATICAL_ALPHABETIC_SYMBOLS = 280,
    MB_BLOCK_MAHJONG_TILES = 281,
    MB_BLOCK_DOMINO_TILES = 282,
    MB_BLOCK_PLAYING_CARDS = 283,
    MB_BLOCK_ENCLOSED_ALPHANUMERIC_SUPPLEMENT = 284,
    MB_BLOCK_ENCLOSED_IDEOGRAPHIC_SUPPLEMENT = 285,
    MB_BLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS = 286,
    MB_BLOCK_EMOTICONS = 287,
    MB_BLOCK_ORNAMENTAL_DINGBATS = 288,
    MB_BLOCK_TRANSPORT_AND_MAP_SYMBOLS = 289,
    MB_BLOCK_ALCHEMICAL_SYMBOLS = 290,
    MB_BLOCK_GEOMETRIC_SHAPES_EXTENDED = 291,
    MB_BLOCK_SUPPLEMENTAL_ARROWS_C = 292,
    MB_BLOCK_SUPPLEMENTAL_SYMBOLS_AND_PICTOGRAPHS = 293,
    MB_BLOCK_CHESS_SYMBOLS = 294,
    MB_BLOCK_SYMBOLS_AND_PICTOGRAPHS_EXTENDED_A = 295,
    MB_BLOCK_SYMBOLS_FOR_LEGACY_COMPUTING = 296,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B = 297,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C = 298,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D = 299,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_E = 300,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_F = 301,
    MB_BLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT = 302,
    MB_BLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_G = 303,
    MB_BLOCK_TAGS = 304,
    MB_BLOCK_VARIATION_SELECTORS_SUPPLEMENT = 305,
    MB_BLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_A = 306,
    MB_BLOCK_SUPPLEMENTARY_PRIVATE_USE_AREA_B = 307
} mb_block_name;

/*
 Unicode block
 [see: https://www.unicode.org/glossary/#block]
*/
typedef struct mb_block {
    char* name;
    uint32_t start;
    uint32_t end;
} mb_block;

#define MB_CATEGORY_COUNT 30

/*
 Unicode codepoint general category
 [see: https://www.unicode.org/glossary/#general_category]
 */
typedef enum mb_category {
    MB_CATEGORY_LU = 0x1, /* 0 (Lu) Letter, Uppercase */
    MB_CATEGORY_LL = 0x2, /* 1 (Ll) Letter, Lowercase */
    MB_CATEGORY_LT = 0x4, /* 2 (Lt) Letter, Titlecase */
    MB_CATEGORY_LM = 0x8, /* 3 (Lm) Letter, Modifier */
    MB_CATEGORY_LO = 0x10, /* 4 (Lo) Letter, Other */
    MB_CATEGORY_MN = 0x20, /* 5 (Mn) Mark, Non-Spacing */
    MB_CATEGORY_MC = 0x40, /* 6 (Mc) Mark, Spacing Combining */
    MB_CATEGORY_ME = 0x80, /* 7 (Me) Mark, Enclosing */
    MB_CATEGORY_ND = 0x100, /* 8 (Nd) Number, Decimal Digit */
    MB_CATEGORY_NL = 0x200, /* 9 (Nl) Number, Letter */
    MB_CATEGORY_NO = 0x400, /* 10 (No) Number, Other */
    MB_CATEGORY_PC = 0x800, /* 11 (Pc) Punctuation, Connector */
    MB_CATEGORY_PD = 0x1000, /* 12 (Pd) Punctuation, Dash */
    MB_CATEGORY_PS = 0x2000, /* 13 (Ps) Punctuation, Open */
    MB_CATEGORY_PE = 0x4000, /* 14 (Pe) Punctuation, Close */
    MB_CATEGORY_PI = 0x8000, /* 15 (Pi) Punctuation, Initial quote */
    MB_CATEGORY_PF = 0x10000, /* 16 (Pf) Punctuation, Final quote */
    MB_CATEGORY_PO = 0x20000, /* 17 (Po) Punctuation, Other */
    MB_CATEGORY_SM = 0x40000, /* 18 (Sm) Symbol, Math */
    MB_CATEGORY_SC = 0x80000, /* 19 (Sc) Symbol, Currency */
    MB_CATEGORY_SK = 0x100000, /* 20 (Sk) Symbol, Modifier */
    MB_CATEGORY_SO = 0x200000, /* 21 (So) Symbol, Other */
    MB_CATEGORY_ZS = 0x400000, /* 22 (Zs) Separator, Space */
    MB_CATEGORY_ZL = 0x800000, /* 23 (Zl) Separator, Line */
    MB_CATEGORY_ZP = 0x1000000, /* 24 (Zp) Separator, Paragraph */
    MB_CATEGORY_CC = 0x2000000, /* 25 (Cc) Other, Control */
    MB_CATEGORY_CF = 0x4000000, /* 26 (Cf) Other, Format */
    MB_CATEGORY_CS = 0x8000000, /* 27 (Cs) Other, Surrogate */
    MB_CATEGORY_CO = 0x10000000, /* 28 (Co) Other, Private Use */
    MB_CATEGORY_CN = 0x20000000 /* 29 (Cn) Other, Not Assigned */
} mb_category;

/*
 A unicode character
 [see: https://www.unicode.org/glossary/#character]
 */
typedef struct mb_character {
    mb_codepoint codepoint;
    unsigned char name[128];
    unsigned short block;
    mb_category category;
    unsigned short combining;
    unsigned short bidirectional;
    unsigned short decomposition;
    unsigned char decimal[128];
    unsigned char digit[128];
    unsigned char numeric[128];
    bool mirrored;
    mb_codepoint uppercase;
    mb_codepoint lowercase;
    mb_codepoint titlecase;
} mb_character;

/*
 Unicode plane
 [see: https://www.unicode.org/glossary/#plane]
*/
typedef uint8_t mb_plane;

#define MB_PLANE_NUM 17 /* 17 planes */
#define MB_PLANE_SIZE 65536 /* 2^16 code points per plane */

/*
 Unicode encoding
 [see: https://www.unicode.org/glossary/#character_encoding_scheme]
 */
typedef uint32_t mb_encoding;

#define MB_ENCODING_UNKNOWN 0
#define MB_ENCODING_ASCII 0x1
#define MB_ENCODING_UTF_8 0x2
#define MB_ENCODING_UTF_16 0x4
#define MB_ENCODING_UTF_16_BE 0x8
#define MB_ENCODING_UTF_16_LE 0x10
#define MB_ENCODING_UTF_32 0x20
#define MB_ENCODING_UTF_32_BE 0x40
#define MB_ENCODING_UTF_32_LE 0x80

/*
 Normalization form
 https://www.unicode.org/glossary/#normalization_form
*/
typedef unsigned short mb_normalization;

#define MB_NORMALIZATION_NFD 0
#define MB_NORMALIZATION_NFC 1
#define MB_NORMALIZATION_NFKD 2
#define MB_NORMALIZATION_NFKC 3

/* Initialize the library */
bool mb_initialize(const char* filename);

/* The library is ready */
bool mb_ready();

/* Close the library */
bool mb_close();

/* Output the current library version (MB_VERSION) */
char* mb_version();

/* Output the current library version number (MB_VERSION_NUMBER) */
unsigned int mb_version_number();

/* Output the current supported unicode version (MB_UNICODE_VERSION) */
char* mb_unicode_version();

/* Return true if the codepoint is valid */
bool mb_codepoint_is_valid(mb_codepoint codepoint);

/* Return true if the plane is valid */
bool mb_plane_is_valid(mb_plane plane);

/* Return the name of a plane, NULL if the place specified is not valid */
const char* mb_plane_name(mb_plane plane, bool abbreviation);

/* Return the string encoding (the most probable) */
mb_encoding mb_string_encoding(const char* buffer, size_t size);

/* Return true if the string is encoded in UTF-8 */
bool mb_string_is_utf8(const char* buffer, size_t size);

/* Return true if the string is encoded in ASCII */
bool mb_string_is_ascii(const char* buffer, size_t size);

/* Return the codepoint character */
bool mb_codepoint_character(mb_character* character, mb_codepoint codepoint);

/* Return true if the codepoint has the category */
bool mb_codepoint_is(mb_codepoint codepoint, mb_category category);

/* Return true if the codepoint is graphic */
bool mb_codepoint_is_graphic(mb_codepoint codepoint);

/* Return the codepoint lowercase codepoint */
mb_codepoint mb_codepoint_to_lowercase(mb_codepoint codepoint);

/* Return the codepoint uppercase codepoint */
mb_codepoint mb_codepoint_to_uppercase(mb_codepoint codepoint);

/* Return the codepoint titlecase codepoint */
mb_codepoint mb_codepoint_to_titlecase(mb_codepoint codepoint);

/* Normalize a string */
void mb_normalize(const char* buffer, size_t size, mb_normalization form);

#ifdef __cplusplus
}
#endif

#endif /* MB_MOJIBAKE_H */
