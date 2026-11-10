// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use std::fmt;
use std::str::FromStr;

use mojibake_sys as sys;

use crate::bidi::BidiClass;
use crate::error::Error;
use crate::ffi;
use crate::plane::Plane;
use crate::property::Property;
use crate::script::{Block, Script};

/// The `General_Category` property of a character.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum GeneralCategory {
    /// `Cn`: an unassigned code point or a noncharacter, such as U+0378.
    Unassigned,
    /// `Lu`: an uppercase letter, such as `'A'`.
    UppercaseLetter,
    /// `Ll`: a lowercase letter, such as `'a'`.
    LowercaseLetter,
    /// `Lt`: a digraph whose first part is uppercase, such as `'ǅ'`.
    TitlecaseLetter,
    /// `Lm`: a modifier letter, such as `'ʰ'`.
    ModifierLetter,
    /// `Lo`: a letter without case, such as `'א'` or `'中'`.
    OtherLetter,
    /// `Mn`: a nonspacing combining mark, such as U+0301 COMBINING ACUTE ACCENT.
    NonspacingMark,
    /// `Mc`: a spacing combining mark, such as U+0903 DEVANAGARI SIGN VISARGA.
    SpacingMark,
    /// `Me`: an enclosing combining mark, such as U+20DD COMBINING ENCLOSING CIRCLE.
    EnclosingMark,
    /// `Nd`: a decimal digit, such as `'7'` or `'٤'`.
    DecimalNumber,
    /// `Nl`: a letterlike number, such as `'Ⅻ'`.
    LetterNumber,
    /// `No`: another number, such as `'½'` or `'²'`.
    OtherNumber,
    /// `Pc`: a connecting punctuation mark, such as `'_'`.
    ConnectorPunctuation,
    /// `Pd`: a dash or hyphen, such as `'-'`.
    DashPunctuation,
    /// `Ps`: an opening punctuation mark of a pair, such as `'('`.
    OpenPunctuation,
    /// `Pe`: a closing punctuation mark of a pair, such as `')'`.
    ClosePunctuation,
    /// `Pi`: an initial quotation mark, such as `'«'`.
    InitialPunctuation,
    /// `Pf`: a final quotation mark, such as `'»'`.
    FinalPunctuation,
    /// `Po`: another punctuation mark, such as `'!'`.
    OtherPunctuation,
    /// `Sm`: a mathematical symbol, such as `'+'`.
    MathSymbol,
    /// `Sc`: a currency sign, such as `'€'`.
    CurrencySymbol,
    /// `Sk`: a non-letterlike modifier symbol, such as `'^'`.
    ModifierSymbol,
    /// `So`: another symbol, such as `'©'`.
    OtherSymbol,
    /// `Zs`: a space character of nonzero width, such as `' '`.
    SpaceSeparator,
    /// `Zl`: U+2028 LINE SEPARATOR.
    LineSeparator,
    /// `Zp`: U+2029 PARAGRAPH SEPARATOR.
    ParagraphSeparator,
    /// `Cc`: a C0 or C1 control code, such as `'\n'`.
    Control,
    /// `Cf`: a format control character, such as U+200D ZERO WIDTH JOINER.
    Format,
    /// `Cs`: a surrogate code point. A Rust `char` is never one.
    Surrogate,
    /// `Co`: a private-use character, such as U+E000.
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

    const NAMES: [&'static str; 30] = [
        "Unassigned",
        "Uppercase_Letter",
        "Lowercase_Letter",
        "Titlecase_Letter",
        "Modifier_Letter",
        "Other_Letter",
        "Nonspacing_Mark",
        "Spacing_Mark",
        "Enclosing_Mark",
        "Decimal_Number",
        "Letter_Number",
        "Other_Number",
        "Connector_Punctuation",
        "Dash_Punctuation",
        "Open_Punctuation",
        "Close_Punctuation",
        "Initial_Punctuation",
        "Final_Punctuation",
        "Other_Punctuation",
        "Math_Symbol",
        "Currency_Symbol",
        "Modifier_Symbol",
        "Other_Symbol",
        "Space_Separator",
        "Line_Separator",
        "Paragraph_Separator",
        "Control",
        "Format",
        "Surrogate",
        "Private_Use",
    ];

    /// The two-letter property value alias, such as `"Lu"`.
    #[must_use]
    pub fn abbreviation(self) -> &'static str {
        Self::ABBREVIATIONS[self as usize]
    }

    /// The long property value alias, such as `"Uppercase_Letter"`.
    #[must_use]
    pub fn name(self) -> &'static str {
        Self::NAMES[self as usize]
    }
}

impl fmt::Display for GeneralCategory {
    /// Writes the two-letter alias, such as `Lu`.
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.abbreviation())
    }
}

impl FromStr for GeneralCategory {
    type Err = Error;

    /// Parse a two-letter alias such as `"Lu"` or a long alias such as `"Uppercase_Letter"`.
    /// Long aliases match loosely (UAX44-LM3): case, spaces, `-`, and `_` are ignored. Returns
    /// [`Error::NotFound`] for any other string.
    fn from_str(s: &str) -> Result<Self, Error> {
        let loose = |name: &str| -> String {
            name.chars()
                .filter(|c| !matches!(c, ' ' | '-' | '_'))
                .map(|c| c.to_ascii_lowercase())
                .collect()
        };
        let wanted = loose(s);

        Self::ALL
            .iter()
            .copied()
            .find(|category| category.abbreviation() == s || loose(category.name()) == wanted)
            .ok_or(Error::NotFound)
    }
}

/// The `East_Asian_Width` property of a character (UAX #11).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum EastAsianWidth {
    /// `A`: narrow in most text, wide in East Asian text, such as `'±'`.
    Ambiguous,
    /// `F`: a fullwidth form of a narrow character, such as `'Ａ'`.
    Fullwidth,
    /// `H`: a halfwidth form of a wide character, such as `'ｱ'`.
    Halfwidth,
    /// `N`: a character that does not occur in East Asian legacy text, such as `'א'`.
    Neutral,
    /// `Na`: a narrow character with a fullwidth counterpart, such as `'a'`.
    Narrow,
    /// `W`: a wide character, such as `'日'` or `'😀'`.
    Wide,
}

/// The emoji properties of a character (UTS #51).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub struct EmojiProperties {
    /// `Emoji`: the character is an emoji, including ones shown as text by default, such as `'#'`.
    pub emoji: bool,
    /// `Emoji_Presentation`: the character is shown as an emoji by default.
    pub presentation: bool,
    /// `Emoji_Modifier`: a skin-tone modifier, U+1F3FB to U+1F3FF.
    pub modifier: bool,
    /// `Emoji_Modifier_Base`: the character can take a skin-tone modifier, such as `'👍'`.
    pub modifier_base: bool,
    /// `Emoji_Component`: the character is used in emoji sequences, such as a regional indicator.
    pub component: bool,
    /// `Extended_Pictographic`: the character is pictographic or reserved for future emoji; used by
    /// grapheme segmentation.
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
    #[must_use]
    pub fn is_canonical(self) -> bool {
        self == Self::Canonical
    }
}

/// How a [`NumericValue`] is used (the `Numeric_Type` property).
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum NumericType {
    /// A decimal digit of a positional number system, such as `'٤'` (4).
    Decimal,
    /// A digit that is not used positionally, such as `'²'` (2).
    Digit,
    /// Any other number, such as `'Ⅻ'` (12) or `'½'` (1/2).
    Numeric,
}

/// The exact `Numeric_Value` of a character, as a fraction, with its `Numeric_Type`.
///
/// ```
/// use mojibake::{CharExt, NumericType};
///
/// let half = '½'.numeric_value().unwrap();
/// assert_eq!((half.numerator, half.denominator), (1, 2));
/// assert_eq!(half.numeric_type, NumericType::Numeric);
/// assert_eq!(half.as_f64(), 0.5);
/// assert_eq!(half.to_string(), "1/2");
///
/// assert_eq!('٤'.numeric_value().unwrap().as_integer(), Some(4));
/// assert_eq!('A'.numeric_value(), None);
/// ```
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct NumericValue {
    /// The `Numeric_Type` of the character.
    pub numeric_type: NumericType,
    /// The numerator, negative for a negative value such as `'༳'` (-1/2).
    pub numerator: i64,
    /// The denominator, 1 for an integer.
    pub denominator: u64,
}

impl NumericValue {
    /// Parse a `UnicodeData.txt` numeric field such as `"5"`, `"-1/2"`, or `"1/160"`, with the
    /// decimal and digit fields that give its type.
    fn new(field: &str, decimal: std::ffi::c_int, digit: std::ffi::c_int) -> Option<Self> {
        let (numerator, denominator) = match field.split_once('/') {
            Some((numerator, denominator)) => (numerator.parse().ok()?, denominator.parse().ok()?),
            None => (field.parse().ok()?, 1),
        };

        if denominator == 0 {
            return None;
        }

        let numeric_type = if (0..=9).contains(&decimal) {
            NumericType::Decimal
        } else if (0..=9).contains(&digit) {
            NumericType::Digit
        } else {
            NumericType::Numeric
        };

        Some(Self {
            numeric_type,
            numerator,
            denominator,
        })
    }

    /// The value as a floating-point number. Fractions such as 1/3 are rounded.
    #[allow(clippy::cast_precision_loss)] // Unicode numeric values fit an f64 mantissa.
    #[must_use]
    pub fn as_f64(self) -> f64 {
        self.numerator as f64 / self.denominator as f64
    }

    /// The value if it is an integer, as for `'Ⅻ'` (12), or `None` for a fraction.
    #[must_use]
    pub fn as_integer(self) -> Option<i64> {
        (self.denominator == 1).then_some(self.numerator)
    }
}

impl fmt::Display for NumericValue {
    /// Writes the value as in `UnicodeData.txt`: `12`, `1/2`, or `-1/2`.
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.denominator == 1 {
            write!(f, "{}", self.numerator)
        } else {
            write!(f, "{}/{}", self.numerator, self.denominator)
        }
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
/// assert_eq!(info.numeric_value.unwrap().as_f64(), 0.5);
/// assert_eq!(info.bidi_class, BidiClass::OtherNeutral);
/// ```
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
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
    pub numeric_value: Option<NumericValue>,
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
            numeric_value: NumericValue::new(&ffi::array_str(&raw.numeric), raw.decimal, raw.digit),
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

/// Unicode character properties that `char` does not provide.
// `char` is `Copy`, so `is_*(self)` matches the standard library.
#[allow(clippy::wrong_self_convention)]
pub trait CharExt {
    /// The `UnicodeData.txt` record of the character, or `None` if it is unassigned. Use it to
    /// read several fields with one lookup.
    #[must_use]
    fn info(self) -> Option<CharInfo>;
    /// The `General_Category` of the character.
    #[must_use]
    fn general_category(self) -> GeneralCategory;
    /// The `Canonical_Combining_Class` of the character: 0 for starters and unassigned
    /// characters, 230 for most marks above.
    #[must_use]
    fn canonical_combining_class(self) -> u8;
    /// The `Bidi_Class` of the character, or `None` if it is unassigned.
    #[must_use]
    fn bidi_class(self) -> Option<BidiClass>;
    /// True if the character is `Bidi_Mirrored`, as `(` and `<` are.
    #[must_use]
    fn is_bidi_mirrored(self) -> bool;
    /// The `Decomposition_Type` of the character, or `None` if it has no decomposition mapping.
    #[must_use]
    fn decomposition_type(self) -> Option<DecompositionType>;
    /// The `Simple_Uppercase_Mapping` of the character: one character, unlike
    /// [`char::to_uppercase`]. The character itself when it has none.
    #[must_use]
    fn simple_uppercase(self) -> char;
    /// The `Simple_Lowercase_Mapping` of the character. The character itself when it has none.
    #[must_use]
    fn simple_lowercase(self) -> char;
    /// The `Simple_Titlecase_Mapping` of the character: `'ǆ'` becomes `'ǅ'`. The simple
    /// uppercase mapping when it has none.
    #[must_use]
    fn simple_titlecase(self) -> char;
    /// The character name, such as `"LATIN SMALL LETTER E WITH ACUTE"`, if it has one.
    #[cfg(feature = "names")]
    #[must_use]
    fn name(self) -> Option<String>;
    /// The block that contains the character, such as `Basic Latin`. See [`Block::of`].
    #[must_use]
    fn block(self) -> Option<Block>;
    /// The `Script` of the character, such as [`Script`] `Latn` for `'a'`. Characters shared by
    /// several scripts are [`Script::COMMON`], and unassigned ones [`Script::UNKNOWN`].
    #[must_use]
    fn script(self) -> Option<Script>;
    /// The exact `Numeric_Value` of the character: `'Ⅻ'` is 12, `'½'` is 1/2.
    #[must_use]
    fn numeric_value(self) -> Option<NumericValue>;
    /// The `East_Asian_Width` of the character.
    #[must_use]
    fn east_asian_width(self) -> Option<EastAsianWidth>;
    /// The emoji properties of the character.
    #[must_use]
    fn emoji_properties(self) -> EmojiProperties;
    /// True if the character is graphic (letter, mark, number, punctuation, symbol, or space).
    #[must_use]
    fn is_graphic(self) -> bool;
    /// True if the character is a combining mark.
    #[must_use]
    fn is_combining(self) -> bool;
    /// UAX #31 `XID_Start`.
    #[must_use]
    fn is_xid_start(self) -> bool;
    /// UAX #31 `XID_Continue`.
    #[must_use]
    fn is_xid_continue(self) -> bool;
    /// The Unicode plane that contains the character.
    #[must_use]
    fn plane(self) -> Plane;
    /// True for a CJK unified ideograph: the main block, U+4E00 to U+9FFF, or one of its
    /// extensions. Compatibility ideographs are not included.
    #[must_use]
    fn is_cjk_ideograph(self) -> bool;
    /// True for a CJK unified ideograph of one of the extension blocks, A to J.
    #[must_use]
    fn is_cjk_extension_ideograph(self) -> bool;
    /// UAX #31 `ID_Start`.
    #[must_use]
    fn is_id_start(self) -> bool;
    /// UAX #31 `ID_Continue`.
    #[must_use]
    fn is_id_continue(self) -> bool;
    /// UAX #31 `Pattern_Syntax`.
    #[must_use]
    fn is_pattern_syntax(self) -> bool;
    /// UAX #31 `Pattern_White_Space`.
    #[must_use]
    fn is_pattern_white_space(self) -> bool;
    /// The character's `Script_Extensions`, or its `Script` when it has no explicit
    /// extensions: `'ー'` (U+30FC) is used by both `Hira` and `Kana`.
    #[must_use]
    fn script_extensions(self) -> Vec<Script>;
    /// The value of a binary property, such as `Alphabetic`. False for a non-binary property.
    #[must_use]
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
    #[must_use]
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

    fn block(self) -> Option<Block> {
        Block::of(self)
    }

    fn script(self) -> Option<Script> {
        // SAFETY: a pure function of its argument.
        Script::from_raw(unsafe { sys::mjb_codepoint_script(self as u32) })
    }

    fn numeric_value(self) -> Option<NumericValue> {
        // SAFETY: mjb_numeric_value is plain data; all-zero is a valid value.
        let mut value: sys::mjb_numeric_value = unsafe { std::mem::zeroed() };
        // SAFETY: the pointer is valid for the duration of the call.
        let status = unsafe { sys::mjb_codepoint_numeric_value(self as u32, &mut value) };

        if status != sys::MJB_STATUS_OK {
            return None;
        }

        NumericValue::new(&ffi::array_str(&value.numeric), value.decimal, value.digit)
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

    fn plane(self) -> Plane {
        Plane::of(self)
    }

    fn is_cjk_ideograph(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_cjk_ideograph(self as u32) }
    }

    fn is_cjk_extension_ideograph(self) -> bool {
        // SAFETY: a pure function of its argument.
        unsafe { sys::mjb_codepoint_is_cjk_extension_ideograph(self as u32) }
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

    fn script_extensions(self) -> Vec<Script> {
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
