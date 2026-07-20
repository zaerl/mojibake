# Mojibake C++17 wrapper

`mojibake.hpp` is a header-only C++17 interface over the complete C API. Text inputs use
`std::string_view` and UTF-8 by default; overload parameters retain access to every encoding
supported by the C library. C-owned string results and bidirectional paragraphs are released with
RAII, and `mjb::LibraryError::status()` preserves the original `mjb_status` on failure.

```cpp
#include <mojibake.hpp>

std::string text = mjb::nfc("Cafe\xCC\x81");
std::string folded = mjb::casefold(text);

mjb::BidiParagraph paragraph("English \xD7\xA2\xD7\x91\xD7\xA8\xD7\x99\xD7\xAA");
std::vector<size_t> visual_order = paragraph.visual_order();

// Preserve the C API's zero-copy fast path when a transformation is unnecessary.
mjb::TextResult result = mjb::normalize_result(text, mjb::NormalizationForm::NFC);
std::string_view normalized = result.view();
```

An untransformed `TextResult` and the streaming breaker classes hold a non-owning view of their
input, so that input must remain alive while those objects are used.
