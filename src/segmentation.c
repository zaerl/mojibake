/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "mojibake-internal.h"
#include "utf.h"

extern mojibake mjb_global;

// Word and Grapheme Cluster Breaking
// See: https://unicode.org/reports/tr29/
MJB_EXPORT bool mjb_segmentation(const char *buffer, size_t size, mjb_encoding encoding) {
    uint8_t state = MJB_UTF_ACCEPT;
    mjb_codepoint codepoint;
    // bool has_previous_character = false;
    // bool first_character = true;

    for(size_t i = 0; i < size; ++i) {
        if(!mjb_decode_step(buffer, size, &state, &i, encoding, &codepoint)) {
            break;
        }

        if(state == MJB_UTF_REJECT) {
            continue;
        }

        // Still not found a UTF-8 character, continue.
        if(state != MJB_UTF_ACCEPT) {
            continue;
        }

        // Break at the start and end of text, unless the text is empty.
        // GB1 sot ÷ Any
        // GB2 Any ÷ eot

        // Do not break between a CR and LF. Otherwise, break before and after controls.
        // GB3 CR × LF
        // GB4 (Control | CR | LF) ÷
        // GB5 ÷ (Control | CR | LF)

        // Do not break Hangul syllable or other conjoining sequences.
        // GB6 L × (L | V | LV | LVT)
        // GB7 (LV | V) × (V | T)
        // GB8 (LVT | T) × T

        // Do not break before extending characters or ZWJ.
        // GB9 × (Extend | ZWJ)

        // The GB9a and GB9b rules only apply to extended grapheme clusters:
        // Do not break before SpacingMarks, or after Prepend characters.
        // GB9a × SpacingMark
        // GB9b Prepend ×

        // The GB9c rule only applies to extended grapheme clusters:
        // Do not break within certain combinations with Indic_Conjunct_Break (InCB)=Linker.
        // GB9c \p{InCB=Consonant} [ \p{InCB=Extend} \p{InCB=Linker} ]* \p{InCB=Linker}
        //   [ \p{InCB=Extend} \p{InCB=Linker} ]* × \p{InCB=Consonant}

        // Do not break within emoji modifier sequences or emoji zwj sequences.
        // GB11 \p{Extended_Pictographic} Extend* ZWJ × \p{Extended_Pictographic}

        // Do not break within emoji flag sequences. That is, do not break between regiona
        // indicator (RI) symbols if there is an odd number of RI characters before the break point.
        // GB12 sot (RI RI)* RI × RI
        // GB13 [^RI] (RI RI)* RI × RI

        // Otherwise, break everywhere.
        // GB999 Any ÷ Any

        // Word Boundary Rules
        // Break at the start and end of text, unless the text is empty.
        // WB1 sot ÷ Any
        // WB2 Any ÷ eot

        // Do not break within CRLF.
        // WB3 CR × LF

        // Otherwise break before and after Newlines (including CR and LF)
        // WB3a (Newline | CR | LF) ÷
        // WB3b ÷ (Newline | CR | LF)

        // Do not break within emoji zwj sequences.
        // WB3c ZWJ × \p{Extended_Pictographic}

        // Keep horizontal whitespace together.
        // WB3d WSegSpace × WSegSpace

        // Ignore Format and Extend characters, except after sot, CR, LF, and Newline. (See Section
        // 6.2, Replacing Ignore Rules.) This also has the effect of: Any × (Format | Extend | ZWJ)
        // WB4 X (Extend | Format | ZWJ)* → X

        // Do not break between most letters.
        // WB5 AHLetter × AHLetter

        // Do not break letters across certain punctuation, such as within “e.g.” or “example.com”.
        // WB6 AHLetter × (MidLetter | MidNumLetQ) AHLetter
        // WB7 AHLetter (MidLetter | MidNumLetQ) × AHLetter
        // WB7a Hebrew_Letter × Single_Quote
        // WB7b Hebrew_Letter × Double_Quote Hebrew_Letter
        // WB7c Hebrew_Letter Double_Quote × Hebrew_Letter

        // Do not break within sequences of digits, or digits adjacent to letters (“3a”, or “A3”).
        // WB8 Numeric × Numeric
        // WB9 AHLetter × Numeric
        // WB10 Numeric × AHLetter

        // Do not break within sequences, such as “3.2” or “3,456.789”.
        // WB11 Numeric (MidNum | MidNumLetQ) × Numeric
        // WB12 Numeric × (MidNum | MidNumLetQ) Numeric

        // Do not break between Katakana.
        // WB13 Katakana × Katakana

        // Do not break from extenders.
        // WB13a (AHLetter | Numeric | Katakana | ExtendNumLet) × ExtendNumLet
        // WB13b ExtendNumLet × (AHLetter | Numeric | Katakana)

        // Do not break within emoji flag sequences. That is, do not break between regional
        //   indicator (RI) symbols if there is an odd number of RI characters before the break
        //   point.
        // WB15 sot (RI RI)* RI × RI
        // WB16 [^RI] (RI RI)* RI × RI

        // Otherwise, break everywhere (including around ideographs).
        // WB999 Any ÷ Any
    }

    return true;
}
