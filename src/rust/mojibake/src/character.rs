// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use mojibake_sys as sys;

use crate::bidi::BidiClass;
use crate::ffi;
use crate::property::Property;

/// The `General_Category` property of a character.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum GeneralCategory {
    Unassigned,
    UppercaseLetter,
    LowercaseLetter,
    TitlecaseLetter,
    ModifierLetter,
    OtherLetter,
    NonspacingMark,
    SpacingMark,
    EnclosingMark,
    DecimalNumber,
    LetterNumber,
    OtherNumber,
    ConnectorPunctuation,
    DashPunctuation,
    OpenPunctuation,
    ClosePunctuation,
    InitialPunctuation,
    FinalPunctuation,
    OtherPunctuation,
    MathSymbol,
    CurrencySymbol,
    ModifierSymbol,
    OtherSymbol,
    SpaceSeparator,
    LineSeparator,
    ParagraphSeparator,
    Control,
    Format,
    Surrogate,
    PrivateUse,
}

impl GeneralCategory {
    // Same order as `mjb_category` in src/unicode.h.
    const ALL: [Self; 30] = [
        Self::Unassigned,
        Self::UppercaseLetter,
        Self::LowercaseLetter,
        Self::TitlecaseLetter,
        Self::ModifierLetter,
        Self::OtherLetter,
        Self::NonspacingMark,
        Self::SpacingMark,
        Self::EnclosingMark,
        Self::DecimalNumber,
        Self::LetterNumber,
        Self::OtherNumber,
        Self::ConnectorPunctuation,
        Self::DashPunctuation,
        Self::OpenPunctuation,
        Self::ClosePunctuation,
        Self::InitialPunctuation,
        Self::FinalPunctuation,
        Self::OtherPunctuation,
        Self::MathSymbol,
        Self::CurrencySymbol,
        Self::ModifierSymbol,
        Self::OtherSymbol,
        Self::SpaceSeparator,
        Self::LineSeparator,
        Self::ParagraphSeparator,
        Self::Control,
        Self::Format,
        Self::Surrogate,
        Self::PrivateUse,
    ];

    const ABBREVIATIONS: [&'static str; 30] = [
        "Cn", "Lu", "Ll", "Lt", "Lm", "Lo", "Mn", "Mc", "Me", "Nd", "Nl", "No", "Pc", "Pd", "Ps",
        "Pe", "Pi", "Pf", "Po", "Sm", "Sc", "Sk", "So", "Zs", "Zl", "Zp", "Cc", "Cf", "Cs", "Co",
    ];

    fn from_raw(raw: sys::mjb_category) -> Self {
        usize::try_from(raw)
            .ok()
            .and_then(|i| Self::ALL.get(i).copied())
            .unwrap_or(Self::Unassigned)
    }

    /// The two-letter property value alias, such as `"Lu"`.
    pub fn abbreviation(self) -> &'static str {
        Self::ABBREVIATIONS[self as usize]
    }
}

/// The `East_Asian_Width` property of a character (UAX #11).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum EastAsianWidth {
    Ambiguous,
    Fullwidth,
    Halfwidth,
    Neutral,
    Narrow,
    Wide,
}

/// The emoji properties of a character (UTS #51).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub struct EmojiProperties {
    pub emoji: bool,
    pub presentation: bool,
    pub modifier: bool,
    pub modifier_base: bool,
    pub component: bool,
    pub extended_pictographic: bool,
}

/// The `Decomposition_Type` of a character with a decomposition mapping.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum DecompositionType {
    /// A canonical decomposition, such as `é` to `e` + U+0301.
    Canonical,
    /// `<circle>`: an encircled form, such as `①`.
    Circle,
    /// `<compat>`: another compatibility form, such as `ﬁ`.
    Compat,
    /// `<final>`: an Arabic final presentation form.
    Final,
    /// `<font>`: a font variant, such as `ℌ`.
    Font,
    /// `<fraction>`: a vulgar fraction, such as `½`.
    Fraction,
    /// `<initial>`: an Arabic initial presentation form.
    Initial,
    /// `<isolated>`: an Arabic isolated presentation form.
    Isolated,
    /// `<medial>`: an Arabic medial presentation form.
    Medial,
    /// `<narrow>`: a halfwidth form, such as `ｱ`.
    Narrow,
    /// `<noBreak>`: a no-break version of a space or hyphen.
    NoBreak,
    /// `<small>`: a small variant, such as `﹐`.
    Small,
    /// `<square>`: a CJK squared form, such as `㎏`.
    Square,
    /// `<sub>`: a subscript, such as `₂`.
    Sub,
    /// `<super>`: a superscript, such as `²`.
    Super,
    /// `<vertical>`: a vertical layout presentation form.
    Vertical,
    /// `<wide>`: a fullwidth form, such as `Ａ`.
    Wide,
}

impl DecompositionType {
    fn from_raw(raw: sys::mjb_decomposition) -> Option<Self> {
        Some(match raw {
            sys::MJB_DECOMPOSITION_CANONICAL => Self::Canonical,
            sys::MJB_DECOMPOSITION_CIRCLE => Self::Circle,
            sys::MJB_DECOMPOSITION_COMPAT => Self::Compat,
            sys::MJB_DECOMPOSITION_FINAL => Self::Final,
            sys::MJB_DECOMPOSITION_FONT => Self::Font,
            sys::MJB_DECOMPOSITION_FRACTION => Self::Fraction,
            sys::MJB_DECOMPOSITION_INITIAL => Self::Initial,
            sys::MJB_DECOMPOSITION_ISOLATED => Self::Isolated,
            sys::MJB_DECOMPOSITION_MEDIAL => Self::Medial,
            sys::MJB_DECOMPOSITION_NARROW => Self::Narrow,
            sys::MJB_DECOMPOSITION_NOBREAK => Self::NoBreak,
            sys::MJB_DECOMPOSITION_SMALL => Self::Small,
            sys::MJB_DECOMPOSITION_SQUARE => Self::Square,
            sys::MJB_DECOMPOSITION_SUB => Self::Sub,
            sys::MJB_DECOMPOSITION_SUPER => Self::Super,
            sys::MJB_DECOMPOSITION_VERTICAL => Self::Vertical,
            sys::MJB_DECOMPOSITION_WIDE => Self::Wide,
            _ => return None,
        })
    }

    /// True for a canonical decomposition, false for a compatibility one.
    pub fn is_canonical(self) -> bool {
        self == Self::Canonical
    }
}

/// The `UnicodeData.txt` record of an assigned character, read in one lookup by
/// [`CharExt::info`].
///
/// ```
/// use mojibake::bidi::BidiClass;
/// use mojibake::{CharExt, DecompositionType};
///
/// let info = '½'.info().unwrap();
/// assert_eq!(info.decomposition_type, Some(DecompositionType::Fraction));
/// assert_eq!(info.numeric_value, Some(0.5));
/// assert_eq!(info.bidi_class, BidiClass::OtherNeutral);
/// ```
#[derive(Clone, Copy, Debug, PartialEq)]
#[non_exhaustive]
pub struct CharInfo {
    /// `General_Category`.
    pub general_category: GeneralCategory,
    /// `Canonical_Combining_Class`: 0 for starters, 230 for most marks above.
    pub canonical_combining_class: u8,
    /// `Bidi_Class`.
    pub bidi_class: BidiClass,
    /// `Bidi_Mirrored`: the glyph is mirrored in right-to-left text, as for `(`.
    pub bidi_mirrored: bool,
    /// `Decomposition_Type`, or `None` when the character has no decomposition mapping.
    pub decomposition_type: Option<DecompositionType>,
    /// The value of a decimal digit (`Numeric_Type=Decimal`), such as 4 for `'٤'`.
    pub decimal_value: Option<u8>,
    /// The value of a digit (`Numeric_Type=Decimal` or `Digit`), such as 2 for `'²'`.
    pub digit_value: Option<u8>,
    /// `Numeric_Value`, as in [`CharExt::numeric_value`].
    pub numeric_value: Option<f64>,
    /// `Simple_Uppercase_Mapping`: the character itself when it has none.
    pub simple_uppercase: char,
    /// `Simple_Lowercase_Mapping`: the character itself when it has none.
    pub simple_lowercase: char,
    /// `Simple_Titlecase_Mapping`: the uppercase mapping when it has none.
    pub simple_titlecase: char,
}

impl CharInfo {
    fn new(c: char, raw: &sys::mjb_character) -> Self {
        let digit = |value: std::ffi::c_int| u8::try_from(value).ok().filter(|&v| v <= 9);
        let mapping = |value: sys::mjb_codepoint, fallback: char| {
            if value == 0 {
                fallback
            } else {
                char::from_u32(value).unwrap_or(fallback)
            }
        };
        let simple_uppercase = mapping(raw.uppercase, c);

        Self {
            general_category: GeneralCategory::from_raw(raw.category),
            canonical_combining_class: u8::try_from(raw.combining).unwrap_or(0),
            // `Bidi_Class` defaults to L; the library fills it for every assigned character.
            bidi_class: BidiClass::from_raw(sys::mjb_bidi_class::from(raw.bidirectional))
                .unwrap_or(BidiClass::LeftToRight),
            bidi_mirrored: raw.mirrored,
            decomposition_type: DecompositionType::from_raw(raw.decomposition),
            decimal_value: digit(raw.decimal),
            digit_value: digit(raw.digit),
            numeric_value: parse_numeric(&ffi::array_str(&raw.numeric)),
            simple_uppercase,
            simple_lowercase: mapping(raw.lowercase, c),
            simple_titlecase: mapping(raw.titlecase, simple_uppercase),
        }
    }
}

fn info(c: char) -> Option<sys::mjb_character> {
    // SAFETY: mjb_character is plain data; all-zero is a valid value.
    let mut character: sys::mjb_character = unsafe { std::mem::zeroed() };
    // SAFETY: the pointer is valid for the duration of the call.
    let status = unsafe { sys::mjb_codepoint_info(c as u32, &mut character) };

    (status == sys::MJB_STATUS_OK).then_some(character)
}

/// Parse a `UnicodeData.txt` numeric field such as `"5"`, `"-1/2"`, or `"1/160"`.
fn parse_numeric(field: &str) -> Option<f64> {
    match field.split_once('/') {
        Some((numerator, denominator)) => {
            Some(numerator.parse::<f64>().ok()? / denominator.parse::<f64>().ok()?)
        }
        None => field.parse().ok(),
    }
}

/// Unicode character properties that `char` does not provide.
// `char` is `Copy`, so `is_*(self)` matches the standard library.
#[allow(clippy::wrong_self_convention)]
pub trait CharExt {
    /// The `UnicodeData.txt` record of the character, or `None` if it is unassigned. Use it to
    /// read several fields with one lookup.
    fn info(self) -> Option<CharInfo>;
    /// The `General_Category` of the character.
    fn general_category(self) -> GeneralCategory;
    /// The `Canonical_Combining_Class` of the character: 0 for starters and unassigned
    /// characters, 230 for most marks above.
    fn canonical_combining_class(self) -> u8;
    /// The `Bidi_Class` of the character, or `None` if it is unassigned.
    fn bidi_class(self) -> Option<BidiClass>;
    /// True if the character is `Bidi_Mirrored`, as `(` and `<` are.
    fn is_bidi_mirrored(self) -> bool;
    /// The `Decomposition_Type` of the character, or `None` if it has no decomposition mapping.
    fn decomposition_type(self) -> Option<DecompositionType>;
    /// The `Simple_Uppercase_Mapping` of the character: one character, unlike
    /// [`char::to_uppercase`]. The character itself when it has none.
    fn simple_uppercase(self) -> char;
    /// The `Simple_Lowercase_Mapping` of the character. The character itself when it has none.
    fn simple_lowercase(self) -> char;
    /// The `Simple_Titlecase_Mapping` of the character: `'ǆ'` becomes `'ǅ'`. The simple
    /// uppercase mapping when it has none.
    fn simple_titlecase(self) -> char;
    /// The character name, such as `"LATIN SMALL LETTER E WITH ACUTE"`, if it has one.
    #[cfg(feature = "names")]
    fn name(self) -> Option<String>;
    /// The name of the block that contains the character, such as `"Basic Latin"`.
    fn block(self) -> Option<String>;
    /// The ISO 15924 code of the character's `Script`, such as `"Latn"`.
    fn script(self) -> Option<&'static str>;
    /// The `Numeric_Value` of the character: `'Ⅻ'` is 12, `'½'` is 0.5.
    fn numeric_value(self) -> Option<f64>;
    /// The `East_Asian_Width` of the character.
    fn east_asian_width(self) -> Option<EastAsianWidth>;
    /// The emoji properties of the character.
    fn emoji_properties(self) -> EmojiProperties;
    /// True if the character is graphic (letter, mark, number, punctuation, symbol, or space).
    fn is_graphic(self) -> bool;
    /// True if the character is a combining mark.
    fn is_combining(self) -> bool;
    /// UAX #31 `XID_Start`.
    fn is_xid_start(self) -> bool;
    /// UAX #31 `XID_Continue`.
    fn is_xid_continue(self) -> bool;
    /// UAX #31 `ID_Start`.
    fn is_id_start(self) -> bool;
    /// UAX #31 `ID_Continue`.
    fn is_id_continue(self) -> bool;
    /// UAX #31 `Pattern_Syntax`.
    fn is_pattern_syntax(self) -> bool;
    /// UAX #31 `Pattern_White_Space`.
    fn is_pattern_white_space(self) -> bool;
    /// The ISO 15924 codes of the character's `Script_Extensions`, or of its `Script` when it has
    /// no explicit extensions: `'ー'` (U+30FC) is used by both `"Hira"` and `"Kana"`.
    fn script_extensions(self) -> Vec<&'static str>;
    /// The value of a binary property, such as `Alphabetic`. False for a non-binary property.
    fn has_property(self, property: Property) -> bool;
    /// The value of an enumerated property in the C library's property table, as the raw C enum
    /// value from `src/unicode.h`, such as an `mjb_script` for `Script`.
    ///
    /// The table holds `Script`, `East_Asian_Width`, `Grapheme_Cluster_Break`,
    /// `Indic_Conjunct_Break`, `Joining_Type`, `Line_Break`, `Sentence_Break`, and `Word_Break`.
    /// Other properties have dedicated methods, such as
    /// [`general_category`](CharExt::general_category), or are not stored.
    ///
    /// `None` when the character has no stored value for the property, and for binary
    /// properties, which [`has_property`](CharExt::has_property) reads.
    fn property_value(self, property: Property) -> Option<i32>;
}

impl CharExt for char {
    fn info(self) -> Option<CharInfo> {
        info(self).map(|raw| CharInfo::new(self, &raw))
    }

    fn canonical_combining_class(self) -> u8 {
        self.info().map_or(0, |info| info.canonical_combining_class)
    }

    fn bidi_class(self) -> Option<BidiClass> {
        self.info().map(|info| info.bidi_class)
    }

    fn is_bidi_mirrored(self) -> bool {
        self.info().is_some_and(|info| info.bidi_mirrored)
    }

    fn decomposition_type(self) -> Option<DecompositionType> {
        self.info().and_then(|info| info.decomposition_type)
    }

    fn simple_uppercase(self) -> char {
        self.info().map_or(self, |info| info.simple_uppercase)
    }

    fn simple_lowercase(self) -> char {
        self.info().map_or(self, |info| info.simple_lowercase)
    }

    fn simple_titlecase(self) -> char {
        self.info().map_or(self, |info| info.simple_titlecase)
    }

    fn general_category(self) -> GeneralCategory {
        info(self).map_or(GeneralCategory::Unassigned, |c| {
            GeneralCategory::from_raw(c.category)
        })
    }

    #[cfg(feature = "names")]
    fn name(self) -> Option<String> {
        info(self)
            .map(|c| ffi::array_str(&c.name))
            .filter(|name| !name.is_empty())
    }

    fn block(self) -> Option<String> {
        // SAFETY: mjb_block_info is plain data; all-zero is a valid value.
        let mut block: sys::mjb_block_info = unsafe { std::mem::zeroed() };
        // SAFETY: the pointer is valid for the duration of the call.
        let status = unsafe { sys::mjb_codepoint_block(self as u32, &mut block) };

        (status == sys::MJB_STATUS_OK).then(|| ffi::array_str(&block.name))
    }

    fn script(self) -> Option<&'static str> {
        // SAFETY: mjb_script_code returns NULL or a static string.
        unsafe { ffi::static_str(sys::mjb_script_code(sys::mjb_codepoint_script(self as u32))) }
    }

    fn numeric_value(self) -> Option<f64> {
        // SAFETY: mjb_numeric_value is plain data; all-zero is a valid value.
        let mut value: sys::mjb_numeric_value = unsafe { std::mem::zeroed() };
        // SAFETY: the pointer is valid for the duration of the call.
        let status = unsafe { sys::mjb_codepoint_numeric_value(self as u32, &mut value) };

        if status != sys::MJB_STATUS_OK {
            return None;
        }

        parse_numeric(&ffi::array_str(&value.numeric))
    }

    fn east_asian_width(self) -> Option<EastAsianWidth> {
        let mut width = 0;
        // SAFETY: the pointer is valid for the duration of the call.
        let status = unsafe { sys::mjb_codepoint_east_asian_width(self as u32, &mut width) };

        if status != sys::MJB_STATUS_OK {
            return None;
        }

        // Same order as `mjb_east_asian_width` in src/unicode.h; 0 is "not set".
        match width {
            1 => Some(EastAsianWidth::Ambiguous),
            2 => Some(EastAsianWidth::Fullwidth),
            3 => Some(EastAsianWidth::Halfwidth),
            4 => Some(EastAsianWidth::Neutral),
            5 => Some(EastAsianWidth::Narrow),
            6 => Some(EastAsianWidth::Wide),
            _ => None,
        }
    }

    fn emoji_properties(self) -> EmojiProperties {
        let mut emoji = sys::mjb_emoji_properties::default();
        // SAFETY: the pointer is valid for the duration of the call.
        let status = unsafe { sys::mjb_codepoint_emoji_properties(self as u32, &mut emoji) };

        if status != sys::MJB_STATUS_OK {
            return EmojiProperties::default();
        }

        EmojiProperties {
            emoji: emoji.emoji,
            presentation: emoji.presentation,
            modifier: emoji.modifier,
            modifier_base: emoji.modifier_base,
            component: emoji.component,
            extended_pictographic: emoji.extended_pictographic,
        }
    }

    fn is_graphic(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_graphic(self as u32) }
    }

    fn is_combining(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_combining(self as u32) }
    }

    fn is_xid_start(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_xid_start(self as u32) }
    }

    fn is_xid_continue(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_xid_continue(self as u32) }
    }

    fn is_id_start(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_id_start(self as u32) }
    }

    fn is_id_continue(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_id_continue(self as u32) }
    }

    fn is_pattern_syntax(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_pattern_syntax(self as u32) }
    }

    fn is_pattern_white_space(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_pattern_white_space(self as u32) }
    }

    fn script_extensions(self) -> Vec<&'static str> {
        ffi::scripts(|scripts, count| {
            // SAFETY: `count` is valid, and `scripts` is NULL or has room for `count` values.
            unsafe { sys::mjb_codepoint_script_extensions(self as u32, scripts, count) }
        })
        .unwrap_or_default()
    }

    fn has_property(self, property: Property) -> bool {
        let mut value = false;
        // SAFETY: the pointer is valid for the duration of the call.
        let status =
            unsafe { sys::mjb_codepoint_property_binary(self as u32, property.0, &mut value) };

        status == sys::MJB_STATUS_OK && value
    }

    fn property_value(self, property: Property) -> Option<i32> {
        let mut value = 0;
        // SAFETY: the pointer is valid for the duration of the call.
        let status =
            unsafe { sys::mjb_codepoint_property_int(self as u32, property.0, &mut value) };

        (status == sys::MJB_STATUS_OK).then_some(value)
    }
}
