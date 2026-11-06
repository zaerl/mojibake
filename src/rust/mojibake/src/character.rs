// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

use mojibake_sys as sys;

use crate::ffi;

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
    /// The `General_Category` of the character.
    fn general_category(self) -> GeneralCategory;
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
}

impl CharExt for char {
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
}
