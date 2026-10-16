# Mojibake C++17 wrapper

`mojibake.hpp` is a header-only C++17 interface over the C API. C-owned string results and
bidirectional paragraphs are released with RAII, and `mjb::LibraryError::status()` preserves the
original `mjb_status` on failure; `what()` adds the library's message for it.

Text arguments:

- Analysis functions, such as `codepoint_count`, `grapheme_count`, `terminal_width`,
  `is_identifier`, and `collation_compare`, take an `mjb::Text`. It converts implicitly from UTF-8
  strings (`std::string`, `std::string_view`, and literals) and from UTF-16 and UTF-32 strings
  (`std::u16string`, `u"..."`, `U"..."`) in the platform's byte order. For another encoding or
  byte order, construct it explicitly: `mjb::Text(bytes, MJB_ENC_UTF_16BE)`.
- Transforms, such as `nfc`, `uppercase`, and `filter`, take a `std::string_view` with explicit
  input and output encodings and return a `std::string`, or take a UTF-16 or UTF-32 string and
  return the same string type. The truncation helpers return views of their input's type.

```cpp
#include <mojibake.hpp>

std::string text = mjb::nfc("Cafe\xCC\x81");
std::string folded = mjb::casefold(text);
std::u16string upper = mjb::uppercase(u"stra\u00DFe"); // u"STRASSE"

size_t graphemes = mjb::grapheme_count(u"A\U0001F1EE\U0001F1F9"); // 2
size_t from_file = mjb::codepoint_count(mjb::Text(file_bytes, MJB_ENC_UTF_16BE));

size_t vowels = 0;
(void)mjb::for_each_codepoint(text,
    [&vowels](const mjb_character &character, mjb_character_position) {
        vowels += character.codepoint == U'a' || character.codepoint == U'e';
        return true;
    });

for(std::string_view cluster : mjb::graphemes(text)) {
    // text is NFC: "C", "a", "f", "\xC3\xA9"
}

for(std::u16string_view word : mjb::words(u"Hello, world")) {
    // u"Hello", u",", u" ", u"world"
}

for(char32_t codepoint : mjb::codepoints(u"A\U0001F642")) {
    // U'A', U'\U0001F642'
}

mjb::BidiParagraph paragraph("English \xD7\xA2\xD7\x91\xD7\xA8\xD7\x99\xD7\xAA");
std::vector<size_t> visual_order = paragraph.visual_order();

// Preserve the C API's zero-copy fast path when a transformation is unnecessary.
mjb::TextResult result = mjb::normalize_result(text, mjb::NormalizationForm::NFC);
std::string_view normalized = result.view();
```

`mjb::Text`, a `TextResult` whose `owned()` is false, the segment and codepoint ranges, and the
breaker classes hold a non-owning view of their input, so that input must remain alive while those
objects are used. The ranges reject a temporary string, which a range-based for loop would destroy
before the loop body.
`normalize_result`, `filter_result`, and `convert_encoding_result` reject a temporary
`std::string` at compile time for that reason.
