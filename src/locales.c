/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <string.h>

#include "mojibake-internal.h"
#include "utf.h"

// The two tables below are automatically generated. Do not edit.

// ISO 639-1 codes, in mjb_locale order.
static const char *mjb_locale_codes[] = {
    "",   // MJB_LOCALE_NOT_SET
    "aa", // MJB_LOCALE_AA
    "ab", // MJB_LOCALE_AB
    "af", // MJB_LOCALE_AF
    "ak", // MJB_LOCALE_AK
    "sq", // MJB_LOCALE_SQ
    "am", // MJB_LOCALE_AM
    "ar", // MJB_LOCALE_AR
    "an", // MJB_LOCALE_AN
    "hy", // MJB_LOCALE_HY
    "as", // MJB_LOCALE_AS
    "av", // MJB_LOCALE_AV
    "ae", // MJB_LOCALE_AE
    "ay", // MJB_LOCALE_AY
    "az", // MJB_LOCALE_AZ
    "ba", // MJB_LOCALE_BA
    "bm", // MJB_LOCALE_BM
    "eu", // MJB_LOCALE_EU
    "be", // MJB_LOCALE_BE
    "bn", // MJB_LOCALE_BN
    "bi", // MJB_LOCALE_BI
    "bs", // MJB_LOCALE_BS
    "br", // MJB_LOCALE_BR
    "bg", // MJB_LOCALE_BG
    "my", // MJB_LOCALE_MY
    "ca", // MJB_LOCALE_CA
    "ch", // MJB_LOCALE_CH
    "ce", // MJB_LOCALE_CE
    "zh", // MJB_LOCALE_ZH
    "cu", // MJB_LOCALE_CU
    "cv", // MJB_LOCALE_CV
    "kw", // MJB_LOCALE_KW
    "co", // MJB_LOCALE_CO
    "cr", // MJB_LOCALE_CR
    "cs", // MJB_LOCALE_CS
    "da", // MJB_LOCALE_DA
    "dv", // MJB_LOCALE_DV
    "nl", // MJB_LOCALE_NL
    "dz", // MJB_LOCALE_DZ
    "en", // MJB_LOCALE_EN
    "eo", // MJB_LOCALE_EO
    "et", // MJB_LOCALE_ET
    "ee", // MJB_LOCALE_EE
    "fo", // MJB_LOCALE_FO
    "fj", // MJB_LOCALE_FJ
    "fi", // MJB_LOCALE_FI
    "fr", // MJB_LOCALE_FR
    "fy", // MJB_LOCALE_FY
    "ff", // MJB_LOCALE_FF
    "ka", // MJB_LOCALE_KA
    "de", // MJB_LOCALE_DE
    "gd", // MJB_LOCALE_GD
    "ga", // MJB_LOCALE_GA
    "gl", // MJB_LOCALE_GL
    "gv", // MJB_LOCALE_GV
    "el", // MJB_LOCALE_EL
    "gn", // MJB_LOCALE_GN
    "gu", // MJB_LOCALE_GU
    "ht", // MJB_LOCALE_HT
    "ha", // MJB_LOCALE_HA
    "he", // MJB_LOCALE_HE
    "hz", // MJB_LOCALE_HZ
    "hi", // MJB_LOCALE_HI
    "ho", // MJB_LOCALE_HO
    "hr", // MJB_LOCALE_HR
    "hu", // MJB_LOCALE_HU
    "ig", // MJB_LOCALE_IG
    "is", // MJB_LOCALE_IS
    "io", // MJB_LOCALE_IO
    "ii", // MJB_LOCALE_II
    "iu", // MJB_LOCALE_IU
    "ie", // MJB_LOCALE_IE
    "ia", // MJB_LOCALE_IA
    "id", // MJB_LOCALE_ID
    "ik", // MJB_LOCALE_IK
    "it", // MJB_LOCALE_IT
    "jv", // MJB_LOCALE_JV
    "ja", // MJB_LOCALE_JA
    "kl", // MJB_LOCALE_KL
    "kn", // MJB_LOCALE_KN
    "ks", // MJB_LOCALE_KS
    "kr", // MJB_LOCALE_KR
    "kk", // MJB_LOCALE_KK
    "km", // MJB_LOCALE_KM
    "ki", // MJB_LOCALE_KI
    "rw", // MJB_LOCALE_RW
    "ky", // MJB_LOCALE_KY
    "kv", // MJB_LOCALE_KV
    "kg", // MJB_LOCALE_KG
    "ko", // MJB_LOCALE_KO
    "kj", // MJB_LOCALE_KJ
    "ku", // MJB_LOCALE_KU
    "lo", // MJB_LOCALE_LO
    "la", // MJB_LOCALE_LA
    "lv", // MJB_LOCALE_LV
    "li", // MJB_LOCALE_LI
    "ln", // MJB_LOCALE_LN
    "lt", // MJB_LOCALE_LT
    "lb", // MJB_LOCALE_LB
    "lu", // MJB_LOCALE_LU
    "lg", // MJB_LOCALE_LG
    "mk", // MJB_LOCALE_MK
    "mh", // MJB_LOCALE_MH
    "ml", // MJB_LOCALE_ML
    "mi", // MJB_LOCALE_MI
    "mr", // MJB_LOCALE_MR
    "ms", // MJB_LOCALE_MS
    "mg", // MJB_LOCALE_MG
    "mt", // MJB_LOCALE_MT
    "mn", // MJB_LOCALE_MN
    "na", // MJB_LOCALE_NA
    "nv", // MJB_LOCALE_NV
    "nr", // MJB_LOCALE_NR
    "nd", // MJB_LOCALE_ND
    "ng", // MJB_LOCALE_NG
    "ne", // MJB_LOCALE_NE
    "nn", // MJB_LOCALE_NN
    "nb", // MJB_LOCALE_NB
    "no", // MJB_LOCALE_NO
    "ny", // MJB_LOCALE_NY
    "oc", // MJB_LOCALE_OC
    "oj", // MJB_LOCALE_OJ
    "or", // MJB_LOCALE_OR
    "om", // MJB_LOCALE_OM
    "os", // MJB_LOCALE_OS
    "pa", // MJB_LOCALE_PA
    "fa", // MJB_LOCALE_FA
    "pi", // MJB_LOCALE_PI
    "pl", // MJB_LOCALE_PL
    "pt", // MJB_LOCALE_PT
    "ps", // MJB_LOCALE_PS
    "qu", // MJB_LOCALE_QU
    "rm", // MJB_LOCALE_RM
    "ro", // MJB_LOCALE_RO
    "rn", // MJB_LOCALE_RN
    "ru", // MJB_LOCALE_RU
    "sg", // MJB_LOCALE_SG
    "sa", // MJB_LOCALE_SA
    "si", // MJB_LOCALE_SI
    "sk", // MJB_LOCALE_SK
    "sl", // MJB_LOCALE_SL
    "se", // MJB_LOCALE_SE
    "sm", // MJB_LOCALE_SM
    "sn", // MJB_LOCALE_SN
    "sd", // MJB_LOCALE_SD
    "so", // MJB_LOCALE_SO
    "st", // MJB_LOCALE_ST
    "es", // MJB_LOCALE_ES
    "sc", // MJB_LOCALE_SC
    "sr", // MJB_LOCALE_SR
    "ss", // MJB_LOCALE_SS
    "su", // MJB_LOCALE_SU
    "sw", // MJB_LOCALE_SW
    "sv", // MJB_LOCALE_SV
    "ty", // MJB_LOCALE_TY
    "ta", // MJB_LOCALE_TA
    "tt", // MJB_LOCALE_TT
    "te", // MJB_LOCALE_TE
    "tg", // MJB_LOCALE_TG
    "tl", // MJB_LOCALE_TL
    "th", // MJB_LOCALE_TH
    "bo", // MJB_LOCALE_BO
    "ti", // MJB_LOCALE_TI
    "to", // MJB_LOCALE_TO
    "tn", // MJB_LOCALE_TN
    "ts", // MJB_LOCALE_TS
    "tk", // MJB_LOCALE_TK
    "tr", // MJB_LOCALE_TR
    "tw", // MJB_LOCALE_TW
    "ug", // MJB_LOCALE_UG
    "uk", // MJB_LOCALE_UK
    "ur", // MJB_LOCALE_UR
    "uz", // MJB_LOCALE_UZ
    "ve", // MJB_LOCALE_VE
    "vi", // MJB_LOCALE_VI
    "vo", // MJB_LOCALE_VO
    "cy", // MJB_LOCALE_CY
    "wa", // MJB_LOCALE_WA
    "wo", // MJB_LOCALE_WO
    "xh", // MJB_LOCALE_XH
    "yi", // MJB_LOCALE_YI
    "yo", // MJB_LOCALE_YO
    "za", // MJB_LOCALE_ZA
    "zu"  // MJB_LOCALE_ZU
};

// ISO 15924 codes, in mjb_script order.
static const char *mjb_script_codes[] = {
    "",     // MJB_SC_NOT_SET
    "Adlm", // MJB_SC_ADLM
    "Aghb", // MJB_SC_AGHB
    "Ahom", // MJB_SC_AHOM
    "Arab", // MJB_SC_ARAB
    "Armi", // MJB_SC_ARMI
    "Armn", // MJB_SC_ARMN
    "Avst", // MJB_SC_AVST
    "Bali", // MJB_SC_BALI
    "Bamu", // MJB_SC_BAMU
    "Bass", // MJB_SC_BASS
    "Batk", // MJB_SC_BATK
    "Beng", // MJB_SC_BENG
    "Berf", // MJB_SC_BERF
    "Bhks", // MJB_SC_BHKS
    "Bopo", // MJB_SC_BOPO
    "Brah", // MJB_SC_BRAH
    "Brai", // MJB_SC_BRAI
    "Bugi", // MJB_SC_BUGI
    "Buhd", // MJB_SC_BUHD
    "Cakm", // MJB_SC_CAKM
    "Cans", // MJB_SC_CANS
    "Cari", // MJB_SC_CARI
    "Cham", // MJB_SC_CHAM
    "Cher", // MJB_SC_CHER
    "Chrs", // MJB_SC_CHRS
    "Copt", // MJB_SC_COPT
    "Cpmn", // MJB_SC_CPMN
    "Cprt", // MJB_SC_CPRT
    "Cyrl", // MJB_SC_CYRL
    "Deva", // MJB_SC_DEVA
    "Diak", // MJB_SC_DIAK
    "Dogr", // MJB_SC_DOGR
    "Dsrt", // MJB_SC_DSRT
    "Dupl", // MJB_SC_DUPL
    "Egyp", // MJB_SC_EGYP
    "Elba", // MJB_SC_ELBA
    "Elym", // MJB_SC_ELYM
    "Ethi", // MJB_SC_ETHI
    "Gara", // MJB_SC_GARA
    "Geor", // MJB_SC_GEOR
    "Glag", // MJB_SC_GLAG
    "Gong", // MJB_SC_GONG
    "Gonm", // MJB_SC_GONM
    "Goth", // MJB_SC_GOTH
    "Gran", // MJB_SC_GRAN
    "Grek", // MJB_SC_GREK
    "Gujr", // MJB_SC_GUJR
    "Gukh", // MJB_SC_GUKH
    "Guru", // MJB_SC_GURU
    "Hang", // MJB_SC_HANG
    "Hani", // MJB_SC_HANI
    "Hano", // MJB_SC_HANO
    "Hatr", // MJB_SC_HATR
    "Hebr", // MJB_SC_HEBR
    "Hira", // MJB_SC_HIRA
    "Hluw", // MJB_SC_HLUW
    "Hmng", // MJB_SC_HMNG
    "Hmnp", // MJB_SC_HMNP
    "Hrkt", // MJB_SC_HRKT
    "Hung", // MJB_SC_HUNG
    "Ital", // MJB_SC_ITAL
    "Java", // MJB_SC_JAVA
    "Jurc", // MJB_SC_JURC
    "Kali", // MJB_SC_KALI
    "Kana", // MJB_SC_KANA
    "Kawi", // MJB_SC_KAWI
    "Khar", // MJB_SC_KHAR
    "Khmr", // MJB_SC_KHMR
    "Khoj", // MJB_SC_KHOJ
    "Kits", // MJB_SC_KITS
    "Knda", // MJB_SC_KNDA
    "Krai", // MJB_SC_KRAI
    "Kthi", // MJB_SC_KTHI
    "Lana", // MJB_SC_LANA
    "Laoo", // MJB_SC_LAOO
    "Latn", // MJB_SC_LATN
    "Lepc", // MJB_SC_LEPC
    "Limb", // MJB_SC_LIMB
    "Lina", // MJB_SC_LINA
    "Linb", // MJB_SC_LINB
    "Lisu", // MJB_SC_LISU
    "Lyci", // MJB_SC_LYCI
    "Lydi", // MJB_SC_LYDI
    "Mahj", // MJB_SC_MAHJ
    "Maka", // MJB_SC_MAKA
    "Mand", // MJB_SC_MAND
    "Mani", // MJB_SC_MANI
    "Marc", // MJB_SC_MARC
    "Medf", // MJB_SC_MEDF
    "Mend", // MJB_SC_MEND
    "Merc", // MJB_SC_MERC
    "Mero", // MJB_SC_MERO
    "Mlym", // MJB_SC_MLYM
    "Modi", // MJB_SC_MODI
    "Mong", // MJB_SC_MONG
    "Mroo", // MJB_SC_MROO
    "Mtei", // MJB_SC_MTEI
    "Mult", // MJB_SC_MULT
    "Mymr", // MJB_SC_MYMR
    "Nagm", // MJB_SC_NAGM
    "Nand", // MJB_SC_NAND
    "Narb", // MJB_SC_NARB
    "Nbat", // MJB_SC_NBAT
    "Newa", // MJB_SC_NEWA
    "Nkoo", // MJB_SC_NKOO
    "Nshu", // MJB_SC_NSHU
    "Ogam", // MJB_SC_OGAM
    "Olck", // MJB_SC_OLCK
    "Onao", // MJB_SC_ONAO
    "Orkh", // MJB_SC_ORKH
    "Orya", // MJB_SC_ORYA
    "Osge", // MJB_SC_OSGE
    "Osma", // MJB_SC_OSMA
    "Ougr", // MJB_SC_OUGR
    "Palm", // MJB_SC_PALM
    "Pauc", // MJB_SC_PAUC
    "Pcun", // MJB_SC_PCUN
    "Perm", // MJB_SC_PERM
    "Phag", // MJB_SC_PHAG
    "Phli", // MJB_SC_PHLI
    "Phlp", // MJB_SC_PHLP
    "Phnx", // MJB_SC_PHNX
    "Plrd", // MJB_SC_PLRD
    "Prti", // MJB_SC_PRTI
    "Rjng", // MJB_SC_RJNG
    "Rohg", // MJB_SC_ROHG
    "Runr", // MJB_SC_RUNR
    "Samr", // MJB_SC_SAMR
    "Sarb", // MJB_SC_SARB
    "Saur", // MJB_SC_SAUR
    "Seal", // MJB_SC_SEAL
    "Sgnw", // MJB_SC_SGNW
    "Shaw", // MJB_SC_SHAW
    "Shrd", // MJB_SC_SHRD
    "Sidd", // MJB_SC_SIDD
    "Sidt", // MJB_SC_SIDT
    "Sind", // MJB_SC_SIND
    "Sinh", // MJB_SC_SINH
    "Sogd", // MJB_SC_SOGD
    "Sogo", // MJB_SC_SOGO
    "Sora", // MJB_SC_SORA
    "Soyo", // MJB_SC_SOYO
    "Sund", // MJB_SC_SUND
    "Sunu", // MJB_SC_SUNU
    "Sylo", // MJB_SC_SYLO
    "Syrc", // MJB_SC_SYRC
    "Tagb", // MJB_SC_TAGB
    "Takr", // MJB_SC_TAKR
    "Tale", // MJB_SC_TALE
    "Talu", // MJB_SC_TALU
    "Taml", // MJB_SC_TAML
    "Tang", // MJB_SC_TANG
    "Tavt", // MJB_SC_TAVT
    "Tayo", // MJB_SC_TAYO
    "Telu", // MJB_SC_TELU
    "Tfng", // MJB_SC_TFNG
    "Tglg", // MJB_SC_TGLG
    "Thaa", // MJB_SC_THAA
    "Thai", // MJB_SC_THAI
    "Tibt", // MJB_SC_TIBT
    "Tirh", // MJB_SC_TIRH
    "Tnsa", // MJB_SC_TNSA
    "Todr", // MJB_SC_TODR
    "Tols", // MJB_SC_TOLS
    "Toto", // MJB_SC_TOTO
    "Tutg", // MJB_SC_TUTG
    "Ugar", // MJB_SC_UGAR
    "Vaii", // MJB_SC_VAII
    "Vith", // MJB_SC_VITH
    "Wara", // MJB_SC_WARA
    "Wcho", // MJB_SC_WCHO
    "Xpeo", // MJB_SC_XPEO
    "Xsux", // MJB_SC_XSUX
    "Yezi", // MJB_SC_YEZI
    "Yiii", // MJB_SC_YIII
    "Zanb", // MJB_SC_ZANB
    "Zinh", // MJB_SC_ZINH
    "Zyyy", // MJB_SC_ZYYY
    "Zzzz", // MJB_SC_ZZZZ
    "Hanb", // MJB_SC_HANB
    "Jpan", // MJB_SC_JPAN
    "Kore"  // MJB_SC_KORE
};

typedef struct mjb_locale_subtag {
    const char *start;
    size_t length;
} mjb_locale_subtag;

typedef enum mjb_locale_case {
    MJB_LOCALE_CASE_LOWER,
    MJB_LOCALE_CASE_UPPER,
    MJB_LOCALE_CASE_TITLE
} mjb_locale_case;

#define MJB_LOCALE_SUBTAG_MATCHES(subtag, literal) \
    mjb_locale_subtag_matches((subtag), (literal), sizeof(literal) - 1)

static char mjb_locale_ascii_lower(char c) {
    if(c >= 'A' && c <= 'Z') {
        return (char)(c + ('a' - 'A'));
    }

    return c;
}

static char mjb_locale_ascii_upper(char c) {
    if(c >= 'a' && c <= 'z') {
        return (char)(c - ('a' - 'A'));
    }

    return c;
}

static bool mjb_locale_ascii_is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool mjb_locale_ascii_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool mjb_locale_subtag_is_alpha(const mjb_locale_subtag *subtag) {
    for(size_t i = 0; i < subtag->length; ++i) {
        if(!mjb_locale_ascii_is_alpha(subtag->start[i])) {
            return false;
        }
    }

    return true;
}

static bool mjb_locale_subtag_is_digit(const mjb_locale_subtag *subtag) {
    for(size_t i = 0; i < subtag->length; ++i) {
        if(!mjb_locale_ascii_is_digit(subtag->start[i])) {
            return false;
        }
    }

    return true;
}

static bool mjb_locale_subtag_is_variant(const mjb_locale_subtag *subtag) {
    if(subtag->length >= 5 && subtag->length <= 8) {
        return true;
    }

    return subtag->length == 4 && mjb_locale_ascii_is_digit(subtag->start[0]);
}

static bool mjb_locale_subtag_is_singleton(const mjb_locale_subtag *subtag) {
    // See https://datatracker.ietf.org/doc/html/rfc5646 "singleton" section.
    return subtag->length == 1 && mjb_locale_ascii_lower(subtag->start[0]) != 'x';
}

static unsigned int mjb_locale_singleton_index(const mjb_locale_subtag *subtag) {
    char c = mjb_locale_ascii_lower(subtag->start[0]);

    if(mjb_locale_ascii_is_digit(c)) {
        return (unsigned int)(c - '0');
    }

    return (unsigned int)(10 + c - 'a');
}

static bool mjb_locale_subtag_matches(const mjb_locale_subtag *subtag, const char *literal,
    size_t literal_size) {
    if(subtag->length != literal_size) {
        return false;
    }

    for(size_t i = 0; i < literal_size; ++i) {
        if(mjb_locale_ascii_lower(subtag->start[i]) != literal[i]) {
            return false;
        }
    }

    return true;
}

static bool mjb_locale_subtag_equal(const mjb_locale_subtag *a, const mjb_locale_subtag *b) {
    if(a->length != b->length) {
        return false;
    }

    for(size_t i = 0; i < a->length; ++i) {
        if(mjb_locale_ascii_lower(a->start[i]) != mjb_locale_ascii_lower(b->start[i])) {
            return false;
        }
    }

    return true;
}

static char mjb_locale_apply_case(char c, mjb_locale_case letter_case, size_t index) {
    if(letter_case == MJB_LOCALE_CASE_UPPER ||
        (letter_case == MJB_LOCALE_CASE_TITLE && index == 0)) {
        return mjb_locale_ascii_upper(c);
    }

    if(letter_case == MJB_LOCALE_CASE_LOWER || letter_case == MJB_LOCALE_CASE_TITLE) {
        return mjb_locale_ascii_lower(c);
    }

    return c;
}

static bool mjb_locale_append_subtag(char *output, size_t output_size,
    const mjb_locale_subtag *subtag, mjb_locale_case letter_case) {
    size_t output_index = 0;

    while(output_index < output_size && output[output_index] != '\0') {
        ++output_index;
    }

    if(output_index >= output_size) {
        return false;
    }

    size_t separator = output_index == 0 ? 0 : 1;

    if(output_index + separator + subtag->length >= output_size) {
        return false;
    }

    if(separator != 0) {
        output[output_index++] = '-';
    }

    for(size_t i = 0; i < subtag->length; ++i) {
        output[output_index++] = mjb_locale_apply_case(subtag->start[i], letter_case, i);
    }

    output[output_index] = '\0';

    return true;
}

static bool mjb_locale_copy_subtag(char *output, size_t output_size,
    const mjb_locale_subtag *subtag, mjb_locale_case letter_case) {
    if(output_size == 0 || subtag->length >= output_size) {
        return false;
    }

    output[0] = '\0';

    return mjb_locale_append_subtag(output, output_size, subtag, letter_case);
}

static bool mjb_locale_copy_subtags(char *output, size_t output_size,
    const mjb_locale_subtag *subtags, size_t start, size_t end, mjb_locale_case letter_case) {
    if(output_size == 0) {
        return false;
    }

    output[0] = '\0';

    for(size_t i = start; i < end; ++i) {
        if(!mjb_locale_append_subtag(output, output_size, &subtags[i], letter_case)) {
            return false;
        }
    }

    return true;
}

// Match grandfathered tags made of a fixed prefix and one suffix.
static bool mjb_locale_is_grandfathered(const mjb_locale_subtag *subtags, size_t count) {
    if(count == 2) {
        // See https://datatracker.ietf.org/doc/html/rfc5646 "grandfathered" section.
        return (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "i") &&
                   (MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "ami") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "bnn") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "default") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "enochian") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "hak") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "klingon") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "lux") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "mingo") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "navajo") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "pwn") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "tao") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "tay") ||
                       MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "tsu"))) ||
            (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "art") &&
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "lojban")) ||
            (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "cel") &&
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "gaulish")) ||
            (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "no") &&
                (MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "bok") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "nyn"))) ||
            (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "zh") &&
                (MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "guoyu") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "hakka") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "min") ||
                    MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "xiang")));
    }

    if(count == 3) {
        // Match grandfathered tags that require all three subtags.
        return (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "en") &&
                   MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "gb") &&
                   MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "oed")) ||
            (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "sgn") &&
                ((MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "be") &&
                     (MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "fr") ||
                         MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "nl"))) ||
                    (MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "ch") &&
                        MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "de")))) ||
            (MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "zh") &&
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[1], "min") &&
                MJB_LOCALE_SUBTAG_MATCHES(&subtags[2], "nan"));
    }

    return false;
}

// Resolve the lowercase two-letter language subtag to a supported locale, if any.
static void mjb_locale_match(mjb_locale_id *locale) {
    for(size_t i = MJB_LOCALE_NOT_SET + 1; i < MJB_LOCALE_NUM; ++i) {
        if(strcmp(locale->language, mjb_locale_codes[i]) == 0) {
            locale->locale = (mjb_locale)i;
            memcpy(locale->code, mjb_locale_codes[i], sizeof(locale->code));

            return;
        }
    }
}

static mjb_status mjb_locale_parse_ascii(const char *ascii_id, size_t ascii_size,
    mjb_locale_id *locale) {
    mjb_locale_subtag *subtags = NULL;
    size_t subtag_capacity = 0;

#define MJB_LOCALE_PARSE_RETURN(status) \
    do { \
        mjb_free(subtags, subtag_capacity * sizeof(*subtags)); \
        return (status); \
    } while(0)

    if(ascii_size == 0) {
        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
    }

    subtag_capacity = ascii_size / 2 + 1;
    subtags = (mjb_locale_subtag *)mjb_alloc(subtag_capacity * sizeof(*subtags));

    if(subtags == NULL) {
        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_NO_MEMORY);
    }

    const char *subtag_start = NULL;
    size_t subtag_length = 0;
    size_t subtag_count = 0;

    // Split the ASCII tag into hyphen-separated alphanumeric subtags.
    for(size_t i = 0; i < ascii_size; ++i) {
        char c = ascii_id[i];

        if(mjb_locale_ascii_is_alpha(c) || mjb_locale_ascii_is_digit(c)) {
            if(subtag_length == 0) {
                subtag_start = ascii_id + i;
            }

            ++subtag_length;

            if(subtag_length > 8) {
                MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
            }

            continue;
        }

        if(c == '-') {
            if(subtag_length == 0 || subtag_count >= subtag_capacity) {
                MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
            }

            subtags[subtag_count].start = subtag_start;
            subtags[subtag_count].length = subtag_length;
            ++subtag_count;
            subtag_start = NULL;
            subtag_length = 0;
            continue;
        }

        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
    }

    if(subtag_length == 0 || subtag_count >= subtag_capacity) {
        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
    }

    subtags[subtag_count].start = subtag_start;
    subtags[subtag_count].length = subtag_length;
    ++subtag_count;

    // Clear the output only after the input has passed lexical validation.
    memset(locale, 0, sizeof(*locale));

    // i-klingon
    // ^ grandfathered tag
    if(mjb_locale_is_grandfathered(subtags, subtag_count)) {
        if(!mjb_locale_copy_subtags(locale->grandfathered, sizeof(locale->grandfathered), subtags,
               0, subtag_count, MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_OK);
    }

    // x-whatever
    // ^ private-use-only tag
    if(MJB_LOCALE_SUBTAG_MATCHES(&subtags[0], "x")) {
        // See https://datatracker.ietf.org/doc/html/rfc5646 "privateuse" section.
        if(subtag_count < 2 ||
            !mjb_locale_copy_subtags(locale->private_use, sizeof(locale->private_use), subtags, 0,
                subtag_count, MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_OK);
    }

    size_t index = 0;
    const mjb_locale_subtag *language = &subtags[index];

    if(!mjb_locale_subtag_is_alpha(language)) {
        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    // ^ primary language, with up to three optional extlang subtags
    if(language->length >= 2 && language->length <= 3) {
        if(!mjb_locale_copy_subtag(locale->language, sizeof(locale->language), language,
               MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        if(language->length == 2) {
            mjb_locale_match(locale);
        }

        ++index;

        // See https://datatracker.ietf.org/doc/html/rfc5646 "extlang" section.
        for(size_t extlang_count = 0; extlang_count < 3 && index < subtag_count &&
            subtags[index].length == 3 && mjb_locale_subtag_is_alpha(&subtags[index]);
            ++extlang_count, ++index) {
            if(!mjb_locale_append_subtag(locale->extlang, sizeof(locale->extlang), &subtags[index],
                   MJB_LOCALE_CASE_LOWER)) {
                MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
            }
        }
    } else if(language->length >= 4 && language->length <= 8) {
        if(!mjb_locale_copy_subtag(locale->language, sizeof(locale->language), language,
               MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        ++index;
    } else {
        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //          ^ optional script
    if(index < subtag_count && subtags[index].length == 4 &&
        mjb_locale_subtag_is_alpha(&subtags[index])) {
        if(!mjb_locale_copy_subtag(locale->script, sizeof(locale->script), &subtags[index],
               MJB_LOCALE_CASE_TITLE)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        ++index;
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                   ^ optional region
    if(index < subtag_count &&
        ((subtags[index].length == 2 && mjb_locale_subtag_is_alpha(&subtags[index])) ||
            (subtags[index].length == 3 && mjb_locale_subtag_is_digit(&subtags[index])))) {
        // See https://datatracker.ietf.org/doc/html/rfc5646 "region" section.
        if(!mjb_locale_copy_subtag(locale->region, sizeof(locale->region), &subtags[index],
               MJB_LOCALE_CASE_UPPER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        ++index;
    }

    size_t variant_start = index;

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                            ^ optional variants
    while(index < subtag_count && mjb_locale_subtag_is_variant(&subtags[index])) {
        for(size_t i = variant_start; i < index; ++i) {
            if(mjb_locale_subtag_equal(&subtags[i], &subtags[index])) {
                MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
            }
        }

        if(!mjb_locale_append_subtag(locale->variant, sizeof(locale->variant), &subtags[index],
               MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        ++index;
    }

    bool seen_singletons[36] = { false };

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                                         ^ optional extensions
    while(index < subtag_count && mjb_locale_subtag_is_singleton(&subtags[index])) {
        unsigned int singleton = mjb_locale_singleton_index(&subtags[index]);

        if(seen_singletons[singleton]) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        seen_singletons[singleton] = true;

        if(!mjb_locale_append_subtag(locale->extensions, sizeof(locale->extensions),
               &subtags[index], MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        ++index;

        if(index >= subtag_count || subtags[index].length < 2) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        while(index < subtag_count && subtags[index].length >= 2) {
            if(!mjb_locale_append_subtag(locale->extensions, sizeof(locale->extensions),
                   &subtags[index], MJB_LOCALE_CASE_LOWER)) {
                MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
            }

            ++index;
        }
    }

    // language[-script][-region][-variant...][-extension...][-x-private...]
    //                                                        ^ optional private-use tail
    if(index < subtag_count && MJB_LOCALE_SUBTAG_MATCHES(&subtags[index], "x")) {
        if(index + 1 >= subtag_count ||
            !mjb_locale_copy_subtags(locale->private_use, sizeof(locale->private_use), subtags,
                index, subtag_count, MJB_LOCALE_CASE_LOWER)) {
            MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
        }

        index = subtag_count;
    }

    if(index != subtag_count) {
        MJB_LOCALE_PARSE_RETURN(MJB_STATUS_INVALID_ARGUMENT);
    }

    MJB_LOCALE_PARSE_RETURN(MJB_STATUS_OK);

#undef MJB_LOCALE_PARSE_RETURN
}

// Parses a BCP 47 language tag and returns the corresponding locale ID.
// language[-script][-region][-variant...][-extension...][-x-private...]
MJB_EXPORT mjb_status mjb_locale_parse(const mjb_text *id, mjb_locale_id *locale) {
    if(id == NULL || locale == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    const char *buffer = id->buffer;
    size_t size = id->byte_length;
    mjb_encoding encoding = id->encoding;

    if(buffer == NULL && size > 0) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    mjb_status status = mjb_resolve_input_byte_length(buffer, &size, encoding);

    if(status != MJB_STATUS_OK) {
        return status;
    }

    const char *ascii_id = buffer;
    size_t ascii_size = size;
    mjb_result converted = { NULL, 0, 0, false };

    // Parse byte-oriented ASCII after validating or converting the input encoding.
    if(encoding == MJB_ENC_ASCII) {
        if(!mjb_is_ascii(buffer, size)) {
            return MJB_STATUS_INVALID_ARGUMENT;
        }
    } else if(encoding == MJB_ENC_UTF_8 && mjb_is_ascii(buffer, size)) {
        // Already suitable for the byte-oriented locale parser.
    } else {
        mjb_text text = mjb_text_make(buffer, size, encoding);
        status = mjb_convert_encoding(&text, MJB_MALFORMED_STOP, MJB_ENC_ASCII, &converted, NULL);

        if(status == MJB_STATUS_OK) {
            ascii_id = converted.output;
            ascii_size = converted.output_size;
        } else {
            return status == MJB_STATUS_NO_MEMORY ? status : MJB_STATUS_INVALID_ARGUMENT;
        }
    }

    status = mjb_locale_parse_ascii(ascii_id, ascii_size, locale);

    mjb_result_free(&converted);

    return status;
}

MJB_EXPORT const char *mjb_locale_code(mjb_locale locale) {
    if((unsigned int)locale >= MJB_LOCALE_NUM) {
        return NULL;
    }

    return mjb_locale_codes[locale];
}

MJB_EXPORT const char *mjb_script_code(mjb_script script) {
    if((unsigned int)script >= MJB_SC_COUNT) {
        return NULL;
    }

    return mjb_script_codes[script];
}
