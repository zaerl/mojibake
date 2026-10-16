/**
 * The Mojibake C++ library wrapper
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_CPP_MOJIBAKE_HPP
#define MJB_CPP_MOJIBAKE_HPP

#include "mojibake.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <exception>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace mjb {

[[nodiscard]] inline std::string_view status_message(mjb_status status) noexcept {
    const char *message = mjb_status_message(status);

    return message == nullptr ? std::string_view{} : std::string_view(message);
}

/**
 * An error reported by the C library.
 *
 * what() describes the failed operation followed by the library's message for the status, for
 * example "Normalization failed: The normalization form is invalid". The status is retained so
 * callers do not have to parse that text.
 */
class LibraryError : public std::runtime_error {
    mjb_status error_status;

    [[nodiscard]] static std::string describe(mjb_status status, std::string_view message) {
        std::string text(message);
        const std::string_view detail = status_message(status);

        if(!detail.empty()) {
            text += ": ";
            text += detail;
        }

        return text;
    }

  public:
    LibraryError(mjb_status status, std::string_view message)
        : std::runtime_error(describe(status, message)), error_status(status) {
    }

    explicit LibraryError(std::string_view message)
        : LibraryError(MJB_STATUS_INVALID_ARGUMENT, message) {
    }

    [[nodiscard]] mjb_status status() const noexcept {
        return error_status;
    }
};

class LocaleError : public LibraryError {
  public:
    using LibraryError::LibraryError;
};

namespace detail {

inline void check_status(mjb_status status, std::string_view message) {
    if(status != MJB_STATUS_OK) {
        throw LibraryError(status, message);
    }
}

struct ResultAccess;

// True for a temporary std::string passed by value: a TextResult that borrows it would dangle.
template <typename T>
inline constexpr bool is_temporary_string_v = std::is_same_v<std::remove_const_t<T>, std::string>;

} // namespace detail

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
inline constexpr mjb_encoding native_utf16_encoding = MJB_ENC_UTF_16BE;
inline constexpr mjb_encoding native_utf32_encoding = MJB_ENC_UTF_32BE;
#else
// MSVC, and every compiler that does not define __BYTE_ORDER__, targets little-endian platforms.
inline constexpr mjb_encoding native_utf16_encoding = MJB_ENC_UTF_16LE;
inline constexpr mjb_encoding native_utf32_encoding = MJB_ENC_UTF_32LE;
#endif

namespace detail {

// The code unit type of a UTF-16 or UTF-32 string argument; other types are not unit strings.
template <typename T, typename = void> struct unit_char {};

template <typename T>
struct unit_char<T, std::enable_if_t<std::is_convertible_v<const T &, std::u16string_view>>> {
    using type = char16_t;
};

template <typename T>
struct unit_char<T, std::enable_if_t<std::is_convertible_v<const T &, std::u32string_view>>> {
    using type = char32_t;
};

template <typename T> using unit_char_t = typename unit_char<T>::type;

// The code unit type of any text argument: char for UTF-8, then char16_t or char32_t.
template <typename T, typename = void> struct text_char : unit_char<T> {};

template <typename T>
struct text_char<T, std::enable_if_t<std::is_convertible_v<const T &, std::string_view>>> {
    using type = char;
};

template <typename T> using text_char_t = typename text_char<T>::type;

template <typename CharT>
inline constexpr mjb_encoding unit_encoding = std::is_same_v<CharT, char16_t> ?
    native_utf16_encoding :
    native_utf32_encoding;

template <typename CharT>
inline constexpr mjb_encoding text_encoding = std::is_same_v<CharT, char> ? MJB_ENC_UTF_8 :
                                                                            unit_encoding<CharT>;

template <typename CharT>
[[nodiscard]] std::string_view as_bytes(std::basic_string_view<CharT> units) noexcept {
    return std::string_view(reinterpret_cast<const char *>(units.data()),
        units.size() * sizeof(CharT));
}

template <typename CharT>
[[nodiscard]] std::basic_string<CharT> units_from_bytes(std::string_view bytes) {
    std::basic_string<CharT> units(bytes.size() / sizeof(CharT), CharT());

    if(!units.empty()) {
        std::memcpy(&units[0], bytes.data(), units.size() * sizeof(CharT));
    }

    return units;
}

} // namespace detail

/**
 * A view of text and its encoding, the C++ counterpart of mjb_text.
 *
 * It converts implicitly from UTF-8 strings (std::string, std::string_view, and literals) and from
 * UTF-16 and UTF-32 strings (std::u16string, u"...", and so on) in the platform's byte order, so
 * analysis functions take any of them. Pass an explicit encoding for other byte orders or for
 * bytes read from a file: mjb::Text(bytes, MJB_ENC_UTF_16BE). Like std::string_view, it does not
 * own the text, which must outlive it.
 */
class Text {
    mjb_text data;

    template <typename CharT>
    [[nodiscard]] static mjb_text make(std::basic_string_view<CharT> units,
        mjb_encoding encoding) noexcept {
        const std::string_view bytes = detail::as_bytes(units);

        return mjb_text_make(bytes.data(), bytes.size(), encoding);
    }

  public:
    template <typename T, typename CharT = detail::text_char_t<T>>
    Text(const T &input) noexcept // NOLINT(google-explicit-constructor): implicit by design
        : data(make(std::basic_string_view<CharT>(input), detail::text_encoding<CharT>)) {
    }

    template <typename T, typename CharT = detail::text_char_t<T>>
    Text(const T &input, mjb_encoding encoding) noexcept
        : data(make(std::basic_string_view<CharT>(input), encoding)) {
    }

    [[nodiscard]] const mjb_text *get() const noexcept {
        return &data;
    }

    [[nodiscard]] std::string_view bytes() const noexcept {
        return std::string_view(data.buffer, data.byte_length);
    }

    [[nodiscard]] mjb_encoding encoding() const noexcept {
        return data.encoding;
    }
};

/**
 * The result of an allocating transformation. See the mjb_result struct for details.
 *
 * When the output equals the input, the result borrows it instead of copying: owned() is false
 * and view() points into the input, which must outlive the result. Functions that can borrow
 * reject a temporary std::string at compile time; a std::string_view over temporary storage is
 * still the caller's responsibility.
 */
class TextResult {
    mjb_result data{};

    TextResult() = default;

    friend struct detail::ResultAccess;

  public:
    TextResult(const TextResult &) = delete;
    TextResult &operator=(const TextResult &) = delete;

    TextResult(TextResult &&other) noexcept : data(other.data) {
        other.data = {};
    }

    TextResult &operator=(TextResult &&other) noexcept {
        if(this != &other) {
            release();

            data = other.data;
            other.data = {};
        }

        return *this;
    }

    ~TextResult() {
        release();
    }

    [[nodiscard]] bool empty() const noexcept {
        return data.output_size == 0;
    }

    [[nodiscard]] size_t size() const noexcept {
        return data.output_size;
    }

    [[nodiscard]] bool owned() const noexcept {
        return data.owned;
    }

    [[nodiscard]] std::string_view view() const noexcept {
        return data.output_size == 0 ? std::string_view{} :
                                       std::string_view(data.output, data.output_size);
    }

    [[nodiscard]] std::string str() const {
        if(data.output_size == 0) {
            return {};
        }

        return std::string(data.output, data.output_size);
    }

    [[nodiscard]] const mjb_result &raw() const noexcept {
        return data;
    }

  private:
    void release() noexcept {
        const mjb_status status = mjb_result_free(&data);
        (void)status;
    }
};

namespace detail {

struct ResultAccess {
    [[nodiscard]] static TextResult create() noexcept {
        return TextResult{};
    }

    [[nodiscard]] static mjb_result *out(TextResult &result) noexcept {
        return &result.data;
    }

    [[nodiscard]] static TextResult checked(TextResult result, mjb_status status,
        std::string_view message) {
        check_status(status, message);

        if(result.data.output_size != 0 && result.data.output == nullptr) {
            throw LibraryError(MJB_STATUS_INVALID_ARGUMENT, message);
        }

        return result;
    }
};

} // namespace detail

[[nodiscard]] inline std::optional<bool> property_binary(mjb_codepoint codepoint,
    mjb_property property) noexcept {
    bool value = false;

    if(mjb_codepoint_property_binary(codepoint, property, &value) != MJB_STATUS_OK) {
        return std::nullopt;
    }

    return value;
}

[[nodiscard]] inline std::optional<int32_t> property_int(mjb_codepoint codepoint,
    mjb_property property) noexcept {
    int32_t value = 0;
    if(mjb_codepoint_property_int(codepoint, property, &value) != MJB_STATUS_OK) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] inline std::string_view property_name(mjb_property property) noexcept {
    const char *name = mjb_property_name(property);

    return name == nullptr ? std::string_view{} : std::string_view(name);
}

[[nodiscard]] inline mjb_script script(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_script(codepoint);
}

[[nodiscard]] inline std::vector<mjb_script> script_extensions(mjb_codepoint codepoint) {
    size_t count = 0;
    detail::check_status(mjb_codepoint_script_extensions(codepoint, nullptr, &count),
        "Unable to count Script_Extensions");

    std::vector<mjb_script> scripts(count);

    detail::check_status(mjb_codepoint_script_extensions(codepoint, scripts.data(), &count),
        "Unable to read Script_Extensions");

    // Probably not necessary.
    scripts.resize(count);

    return scripts;
}

struct ResolvedScriptSet {
    mjb_script_set_kind kind = MJB_SCRIPT_SET_EMPTY;
    std::vector<mjb_script> scripts;

    [[nodiscard]] bool is_mixed_script() const noexcept {
        return kind == MJB_SCRIPT_SET_EMPTY;
    }

    [[nodiscard]] bool is_all() const noexcept {
        return kind == MJB_SCRIPT_SET_ALL;
    }
};

[[nodiscard]] inline ResolvedScriptSet resolved_script_set(Text input) {
    ResolvedScriptSet result;
    size_t count = 0;
    detail::check_status(mjb_resolved_script_set(input.get(), nullptr, &count, &result.kind),
        "Unable to count resolved scripts");

    result.scripts.resize(count);

    if(count > 0) {
        detail::check_status(mjb_resolved_script_set(input.get(), result.scripts.data(), &count,
                                 &result.kind),
            "Unable to read resolved scripts");
        result.scripts.resize(count);
    }

    return result;
}

/**
 * See mjb_character for details.
 */
class Character {
  private:
    mjb_character data{};

    explicit Character(const mjb_character &character) noexcept : data(character) {
    }

  public:
    explicit Character(mjb_codepoint codepoint) {
        detail::check_status(mjb_codepoint_info(codepoint, &data),
            "Unable to read codepoint character");
    }

    [[nodiscard]] static std::optional<Character> from(mjb_codepoint codepoint) noexcept {
        mjb_character data{};

        if(mjb_codepoint_info(codepoint, &data) != MJB_STATUS_OK) {
            return std::nullopt;
        }

        return Character(data);
    }

    bool operator==(const Character &other) const noexcept {
        return data.codepoint == other.data.codepoint;
    }

    bool operator!=(const Character &other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] mjb_codepoint codepoint() const noexcept {
        return data.codepoint;
    }

    [[nodiscard]] std::string_view name() const noexcept {
        return std::string_view(data.name);
    }

    [[nodiscard]] mjb_category category() const noexcept {
        return data.category;
    }

    [[nodiscard]] mjb_canonical_combining_class combining_class() const noexcept {
        return data.combining;
    }

    [[nodiscard]] unsigned short bidirectional() const noexcept {
        return data.bidirectional;
    }

    [[nodiscard]] mjb_decomposition decomposition_type() const noexcept {
        return data.decomposition;
    }

    [[nodiscard]] std::optional<int> decimal_value() const noexcept {
        return data.decimal == MJB_NUMBER_NOT_VALID ? std::nullopt :
                                                      std::optional<int>{ data.decimal };
    }

    [[nodiscard]] std::optional<int> digit_value() const noexcept {
        return data.digit == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{ data.digit };
    }

    [[nodiscard]] std::string_view numeric_value() const noexcept {
        return std::string_view(data.numeric);
    }

    [[nodiscard]] bool is_mirrored() const noexcept {
        return data.mirrored;
    }

    [[nodiscard]] std::optional<mjb_codepoint> uppercase() const noexcept {
        return data.uppercase == 0 ? std::nullopt : std::optional<mjb_codepoint>{ data.uppercase };
    }

    [[nodiscard]] std::optional<mjb_codepoint> lowercase() const noexcept {
        return data.lowercase == 0 ? std::nullopt : std::optional<mjb_codepoint>{ data.lowercase };
    }

    [[nodiscard]] std::optional<mjb_codepoint> titlecase() const noexcept {
        return data.titlecase == 0 ? std::nullopt : std::optional<mjb_codepoint>{ data.titlecase };
    }

    [[nodiscard]] bool is_combining() const noexcept {
        return mjb_category_is_combining(data.category);
    }

    [[nodiscard]] bool is_graphic() const noexcept {
        return mjb_category_is_graphic(data.category);
    }

    [[nodiscard]] bool is_hangul_syllable() const noexcept {
        return mjb_codepoint_is_hangul_syllable(data.codepoint);
    }

    [[nodiscard]] bool is_hangul_leading_jamo() const noexcept {
        return mjb_codepoint_is_hangul_leading_jamo(data.codepoint);
    }

    [[nodiscard]] bool is_hangul_vowel_jamo() const noexcept {
        return mjb_codepoint_is_hangul_vowel_jamo(data.codepoint);
    }

    [[nodiscard]] bool is_hangul_trailing_jamo() const noexcept {
        return mjb_codepoint_is_hangul_trailing_jamo(data.codepoint);
    }

    [[nodiscard]] bool is_hangul_jamo() const noexcept {
        return mjb_codepoint_is_hangul_jamo(data.codepoint);
    }

    [[nodiscard]] bool is_cjk_ideograph() const noexcept {
        return mjb_codepoint_is_cjk_ideograph(data.codepoint);
    }

    [[nodiscard]] bool is_cjk_extension_ideograph() const noexcept {
        return mjb_codepoint_is_cjk_extension_ideograph(data.codepoint);
    }

    [[nodiscard]] bool is_emoji() const noexcept {
        return mjb_codepoint_is_emoji(data.codepoint);
    }

    [[nodiscard]] bool is_emoji_presentation() const noexcept {
        return mjb_codepoint_is_emoji_presentation(data.codepoint);
    }

    [[nodiscard]] bool is_emoji_modifier() const noexcept {
        return mjb_codepoint_is_emoji_modifier(data.codepoint);
    }

    [[nodiscard]] bool is_emoji_modifier_base() const noexcept {
        return mjb_codepoint_is_emoji_modifier_base(data.codepoint);
    }

    [[nodiscard]] bool is_emoji_component() const noexcept {
        return mjb_codepoint_is_emoji_component(data.codepoint);
    }

    [[nodiscard]] bool is_extended_pictographic() const noexcept {
        return mjb_codepoint_is_extended_pictographic(data.codepoint);
    }

    [[nodiscard]] bool is_id_start() const noexcept {
        return mjb_codepoint_is_id_start(data.codepoint);
    }

    [[nodiscard]] bool is_id_continue() const noexcept {
        return mjb_codepoint_is_id_continue(data.codepoint);
    }

    [[nodiscard]] bool is_xid_start() const noexcept {
        return mjb_codepoint_is_xid_start(data.codepoint);
    }

    [[nodiscard]] bool is_xid_continue() const noexcept {
        return mjb_codepoint_is_xid_continue(data.codepoint);
    }

    [[nodiscard]] bool is_pattern_syntax() const noexcept {
        return mjb_codepoint_is_pattern_syntax(data.codepoint);
    }

    [[nodiscard]] bool is_pattern_white_space() const noexcept {
        return mjb_codepoint_is_pattern_white_space(data.codepoint);
    }

    [[nodiscard]] mjb_plane plane() const noexcept {
        return mjb_codepoint_plane(data.codepoint);
    }

    [[nodiscard]] std::string to_utf8() const {
        std::array<char, 5> buffer{};
        unsigned int len = mjb_codepoint_encode(data.codepoint, buffer.data(), buffer.size(),
            MJB_ENC_UTF_8);

        if(len == 0) {
            throw LibraryError(MJB_STATUS_INVALID_CODEPOINT, "Failed to encode codepoint to UTF-8");
        }

        return std::string(buffer.data(), len);
    }

    [[nodiscard]] const mjb_character &raw() const noexcept {
        return data;
    }
};

struct BlockInfo {
    mjb_block_info data{};

    [[nodiscard]] mjb_block id() const noexcept {
        return data.id;
    }

    [[nodiscard]] std::string_view name() const noexcept {
        return std::string_view(data.name);
    }

    [[nodiscard]] mjb_codepoint first() const noexcept {
        return data.start;
    }

    [[nodiscard]] mjb_codepoint last() const noexcept {
        return data.end;
    }

    [[nodiscard]] bool contains(mjb_codepoint codepoint) const noexcept {
        return codepoint >= data.start && codepoint <= data.end;
    }
};

[[nodiscard]] inline std::optional<BlockInfo> block_info(mjb_codepoint codepoint) {
    BlockInfo block;
    const mjb_status status = mjb_codepoint_block(codepoint, &block.data);

    if(status == MJB_STATUS_NOT_FOUND) {
        return std::nullopt;
    }

    detail::check_status(status, "Unable to read codepoint block");

    return block;
}

struct EmojiProperties {
    mjb_emoji_properties data{};

    [[nodiscard]] mjb_codepoint codepoint() const noexcept {
        return data.codepoint;
    }

    [[nodiscard]] bool is_emoji() const noexcept {
        return data.emoji;
    }

    [[nodiscard]] bool is_presentation() const noexcept {
        return data.presentation;
    }

    [[nodiscard]] bool is_modifier() const noexcept {
        return data.modifier;
    }

    [[nodiscard]] bool is_modifier_base() const noexcept {
        return data.modifier_base;
    }

    [[nodiscard]] bool is_component() const noexcept {
        return data.component;
    }

    [[nodiscard]] bool is_extended_pictographic() const noexcept {
        return data.extended_pictographic;
    }
};

[[nodiscard]] inline std::optional<EmojiProperties> emoji_properties(mjb_codepoint codepoint) {
    EmojiProperties properties;
    const mjb_status status = mjb_codepoint_emoji_properties(codepoint, &properties.data);

    if(status == MJB_STATUS_NOT_FOUND) {
        return std::nullopt;
    }

    detail::check_status(status, "Unable to read emoji properties");

    return properties;
}

[[nodiscard]] inline bool is_valid(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_valid(codepoint);
}

[[nodiscard]] inline bool is_graphic(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_graphic(codepoint);
}

[[nodiscard]] inline bool is_combining(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_combining(codepoint);
}

[[nodiscard]] inline bool category_is_graphic(mjb_category category) noexcept {
    return mjb_category_is_graphic(category);
}

[[nodiscard]] inline bool category_is_combining(mjb_category category) noexcept {
    return mjb_category_is_combining(category);
}

[[nodiscard]] inline bool is_hangul_leading_jamo(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_leading_jamo(codepoint);
}

[[nodiscard]] inline bool is_hangul_vowel_jamo(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_vowel_jamo(codepoint);
}

[[nodiscard]] inline bool is_hangul_trailing_jamo(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_trailing_jamo(codepoint);
}

[[nodiscard]] inline bool is_hangul_jamo(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_jamo(codepoint);
}

[[nodiscard]] inline bool is_hangul_syllable(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_syllable(codepoint);
}

[[nodiscard]] inline bool is_cjk_ideograph(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_cjk_ideograph(codepoint);
}

[[nodiscard]] inline bool is_cjk_extension_ideograph(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_cjk_extension_ideograph(codepoint);
}

[[nodiscard]] inline bool is_emoji(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_emoji(codepoint);
}

[[nodiscard]] inline bool is_emoji_presentation(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_emoji_presentation(codepoint);
}

[[nodiscard]] inline bool is_emoji_modifier(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_emoji_modifier(codepoint);
}

[[nodiscard]] inline bool is_emoji_modifier_base(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_emoji_modifier_base(codepoint);
}

[[nodiscard]] inline bool is_emoji_component(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_emoji_component(codepoint);
}

[[nodiscard]] inline bool is_extended_pictographic(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_extended_pictographic(codepoint);
}

[[nodiscard]] inline bool is_id_start(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_id_start(codepoint);
}

[[nodiscard]] inline bool is_id_continue(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_id_continue(codepoint);
}

[[nodiscard]] inline bool is_xid_start(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_xid_start(codepoint);
}

[[nodiscard]] inline bool is_xid_continue(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_xid_continue(codepoint);
}

[[nodiscard]] inline bool is_pattern_syntax(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_pattern_syntax(codepoint);
}

[[nodiscard]] inline bool is_pattern_white_space(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_pattern_white_space(codepoint);
}

[[nodiscard]] inline mjb_plane plane(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_plane(codepoint);
}

[[nodiscard]] inline bool is_valid(mjb_plane plane) noexcept {
    return mjb_plane_is_valid(plane);
}

[[nodiscard]] inline std::string_view plane_name(mjb_plane plane,
    bool abbreviation = false) noexcept {
    const char *name = mjb_plane_name(plane, abbreviation);

    return name == nullptr ? std::string_view{} : std::string_view(name);
}

[[nodiscard]] inline mjb_east_asian_width east_asian_width(mjb_codepoint codepoint) {
    mjb_east_asian_width width = MJB_EAW_NOT_SET;
    detail::check_status(mjb_codepoint_east_asian_width(codepoint, &width),
        "Unable to read East_Asian_Width");

    return width;
}

[[nodiscard]] inline std::string encode(mjb_codepoint codepoint,
    mjb_encoding encoding = MJB_ENC_UTF_8) {
    std::array<char, 5> buffer{};
    const unsigned int size = mjb_codepoint_encode(codepoint, buffer.data(), buffer.size(),
        encoding);

    if(size == 0) {
        const mjb_status status = mjb_codepoint_is_valid(codepoint) ? MJB_STATUS_UNSUPPORTED :
                                                                      MJB_STATUS_INVALID_CODEPOINT;
        throw LibraryError(status, "Unable to encode codepoint");
    }

    return std::string(buffer.data(), size);
}

struct NumericValue {
    mjb_numeric_value data{};

    [[nodiscard]] std::optional<int> decimal() const noexcept {
        return data.decimal == MJB_NUMBER_NOT_VALID ? std::nullopt :
                                                      std::optional<int>{ data.decimal };
    }

    [[nodiscard]] std::optional<int> digit() const noexcept {
        return data.digit == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{ data.digit };
    }

    [[nodiscard]] std::string_view numeric() const noexcept {
        return std::string_view(data.numeric);
    }
};

[[nodiscard]] inline NumericValue numeric_value(mjb_codepoint codepoint) {
    NumericValue value;
    detail::check_status(mjb_codepoint_numeric_value(codepoint, &value.data),
        "Unable to read numeric value");

    return value;
}

[[nodiscard]] inline mjb_encoding detect_encoding(std::string_view input) noexcept {
    return mjb_detect_encoding(input.data(), input.size());
}

[[nodiscard]] inline bool is_ascii(std::string_view input) noexcept {
    return mjb_is_ascii(input.data(), input.size());
}

[[nodiscard]] inline bool is_utf8(std::string_view input) noexcept {
    return mjb_is_utf8(input.data(), input.size());
}

[[nodiscard]] inline bool is_utf16(std::string_view input) noexcept {
    return mjb_is_utf16(input.data(), input.size());
}

[[nodiscard]] inline mjb_status validate_encoding(Text input,
    mjb_diagnostic *diagnostic = nullptr) noexcept {
    return mjb_validate_encoding(input.get(), diagnostic);
}

[[nodiscard]] inline mjb_status decode_next(std::string_view input, size_t &offset,
    mjb_codepoint &codepoint, mjb_encoding encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) noexcept {
    const mjb_text text = mjb_text_make(input.data(), input.size(), encoding);
    return mjb_decode_next(&text, malformed_policy, &offset, &codepoint, diagnostic);
}

[[nodiscard]] inline mjb_status decode_previous(std::string_view input, size_t &offset,
    mjb_codepoint &codepoint, mjb_encoding encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) noexcept {
    const mjb_text text = mjb_text_make(input.data(), input.size(), encoding);
    return mjb_decode_previous(&text, malformed_policy, &offset, &codepoint, diagnostic);
}

[[nodiscard]] inline size_t codepoint_count(Text input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    size_t count = 0;
    detail::check_status(mjb_codepoint_count(input.get(), malformed_policy, &count, diagnostic),
        "Codepoint count failed");

    return count;
}

[[nodiscard]] inline mjb_status for_each_codepoint(Text input, mjb_for_each_codepoint_fn callback,
    void *context = nullptr, mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) noexcept {
    return mjb_for_each_codepoint(input.get(), malformed_policy, callback, context, diagnostic);
}

namespace detail {

template <typename Fn> struct CodepointVisitor {
    Fn *fn;
    std::exception_ptr error;
};

// Exceptions must not unwind through the C library, so they are caught here and rethrown by
// for_each_codepoint once the C call has returned.
template <typename Fn>
bool visit_codepoint(const mjb_character *character, mjb_character_position position,
    void *context) noexcept {
    auto *visitor = static_cast<CodepointVisitor<Fn> *>(context);

    try {
        return static_cast<bool>((*visitor->fn)(*character, position));
    } catch(...) {
        visitor->error = std::current_exception();

        return false;
    }
}

} // namespace detail

/**
 * Call `fn(const mjb_character &, mjb_character_position)` for each codepoint. `fn` can be any
 * callable, including a lambda with captures; returning false stops the iteration with
 * MJB_STATUS_CALLBACK_STOPPED. An exception thrown by `fn` stops the iteration and is rethrown.
 */
template <typename Fn,
    std::enable_if_t<
        std::is_invocable_r_v<bool, Fn &, const mjb_character &, mjb_character_position>, int> = 0>
[[nodiscard]] mjb_status for_each_codepoint(Text input, Fn &&fn,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    using Callable = std::remove_reference_t<Fn>;
    detail::CodepointVisitor<Callable> visitor{ &fn, nullptr };
    const mjb_status status = mjb_for_each_codepoint(input.get(), malformed_policy,
        detail::visit_codepoint<Callable>, &visitor, diagnostic);

    if(visitor.error) {
        std::rethrow_exception(visitor.error);
    }

    return status;
}

[[nodiscard]] inline TextResult convert_encoding_result(std::string_view input,
    mjb_encoding input_encoding, mjb_encoding output_encoding,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    TextResult result = detail::ResultAccess::create();
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_convert_encoding(&text, malformed_policy, output_encoding,
        detail::ResultAccess::out(result), diagnostic);

    return detail::ResultAccess::checked(std::move(result), status, "Encoding conversion failed");
}

// The result can borrow the input, so a temporary std::string would leave it dangling.
template <typename T, typename... Args, std::enable_if_t<detail::is_temporary_string_v<T>, int> = 0>
TextResult convert_encoding_result(T &&input, Args &&...args) = delete;

[[nodiscard]] inline std::string convert_encoding(std::string_view input,
    mjb_encoding input_encoding, mjb_encoding output_encoding,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return convert_encoding_result(input, input_encoding, output_encoding, malformed_policy,
        diagnostic)
        .str();
}

[[nodiscard]] inline bool is_identifier(Text input,
    mjb_identifier_profile profile = MJB_IDENTIFIER_DEFAULT) noexcept {
    return mjb_is_identifier(input.get(), profile);
}

[[nodiscard]] inline bool confusable_match(Text s1, Text s2,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *s1_diagnostic = nullptr, mjb_diagnostic *s2_diagnostic = nullptr) {
    bool confusable = false;
    detail::check_status(mjb_confusable_match(s1.get(), s2.get(), malformed_policy, &confusable,
                             s1_diagnostic, s2_diagnostic),
        "Confusable comparison failed");

    return confusable;
}

[[nodiscard]] inline TextResult confusable_skeleton_result(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_confusable_skeleton(&text, output_encoding,
        detail::ResultAccess::out(result));

    return detail::ResultAccess::checked(std::move(result), status,
        "Confusable skeleton generation failed");
}

[[nodiscard]] inline std::string confusable_skeleton(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return confusable_skeleton_result(input, input_encoding, output_encoding).str();
}

enum class NormalizationForm {
    NFC = MJB_NORMALIZATION_NFC,
    NFD = MJB_NORMALIZATION_NFD,
    NFKC = MJB_NORMALIZATION_NFKC,
    NFKD = MJB_NORMALIZATION_NFKD
};

[[nodiscard]] inline TextResult normalize_result(std::string_view input, NormalizationForm form,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    TextResult result = detail::ResultAccess::create();
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_normalize(&text, malformed_policy,
        static_cast<mjb_normalization>(form), output_encoding, detail::ResultAccess::out(result),
        diagnostic);

    return detail::ResultAccess::checked(std::move(result), status, "Normalization failed");
}

// The result can borrow the input, so a temporary std::string would leave it dangling.
template <typename T, typename... Args, std::enable_if_t<detail::is_temporary_string_v<T>, int> = 0>
TextResult normalize_result(T &&input, Args &&...args) = delete;

[[nodiscard]] inline std::string normalize(std::string_view input, NormalizationForm form,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize_result(input, form, input_encoding, output_encoding, malformed_policy,
        diagnostic)
        .str();
}

[[nodiscard]] inline std::string nfc(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFC, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline std::string nfd(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFD, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline std::string nfkc(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFKC, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline std::string nfkd(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFKD, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline mjb_quick_check_result normalization_quick_check(Text input,
    NormalizationForm form) {
    mjb_quick_check_result quick_check = MJB_QC_NO;
    detail::check_status(mjb_normalization_quick_check(input.get(),
                             static_cast<mjb_normalization>(form), &quick_check),
        "Normalization quick-check failed");

    return quick_check;
}

enum class Filter : unsigned int {
    None = MJB_FILTER_NONE,
    Normalize = MJB_FILTER_NORMALIZE,
    Spaces = MJB_FILTER_SPACES,
    CollapseSpaces = MJB_FILTER_COLLAPSE_SPACES,
    Controls = MJB_FILTER_CONTROLS,
    Numeric = MJB_FILTER_NUMERIC,
    LimitCombining = MJB_FILTER_LIMIT_COMBINING
};

[[nodiscard]] constexpr Filter operator|(Filter left, Filter right) noexcept {
    return static_cast<Filter>(static_cast<unsigned int>(left) | static_cast<unsigned int>(right));
}

[[nodiscard]] constexpr Filter operator&(Filter left, Filter right) noexcept {
    return static_cast<Filter>(static_cast<unsigned int>(left) & static_cast<unsigned int>(right));
}

constexpr Filter &operator|=(Filter &left, Filter right) noexcept {
    left = left | right;

    return left;
}

[[nodiscard]] inline TextResult filter_result(std::string_view input, mjb_filter_flags filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    TextResult result = detail::ResultAccess::create();
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_filter(&text, malformed_policy, filters, output_encoding,
        detail::ResultAccess::out(result), diagnostic);

    return detail::ResultAccess::checked(std::move(result), status, "String filtering failed");
}

[[nodiscard]] inline std::string filter(std::string_view input, mjb_filter_flags filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return filter_result(input, filters, input_encoding, output_encoding, malformed_policy,
        diagnostic)
        .str();
}

[[nodiscard]] inline TextResult filter_result(std::string_view input, Filter filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return filter_result(input, static_cast<mjb_filter_flags>(filters), input_encoding,
        output_encoding, malformed_policy, diagnostic);
}

// The result can borrow the input, so a temporary std::string would leave it dangling.
template <typename T, typename... Args, std::enable_if_t<detail::is_temporary_string_v<T>, int> = 0>
TextResult filter_result(T &&input, Args &&...args) = delete;

[[nodiscard]] inline std::string filter(std::string_view input, Filter filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return filter_result(input, filters, input_encoding, output_encoding, malformed_policy,
        diagnostic)
        .str();
}

[[nodiscard]] inline TextResult nfkc_casefold_result(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    TextResult result = detail::ResultAccess::create();
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_nfkc_casefold(&text, malformed_policy, output_encoding,
        detail::ResultAccess::out(result), diagnostic);
    return detail::ResultAccess::checked(std::move(result), status, "NFKC case folding failed");
}

[[nodiscard]] inline std::string nfkc_casefold(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return nfkc_casefold_result(input, input_encoding, output_encoding, malformed_policy,
        diagnostic)
        .str();
}

class IdnaResult {
    TextResult text_result;
    uint32_t error_flags;

  public:
    IdnaResult(TextResult result, uint32_t errors) noexcept
        : text_result(std::move(result)), error_flags(errors) {
    }

    [[nodiscard]] bool valid() const noexcept {
        return error_flags == MJB_IDNA_ERROR_NONE;
    }

    [[nodiscard]] uint32_t errors() const noexcept {
        return error_flags;
    }

    [[nodiscard]] bool has_error(mjb_idna_error error) const noexcept {
        return (error_flags & static_cast<uint32_t>(error)) != 0;
    }

    [[nodiscard]] std::string_view view() const noexcept {
        return text_result.view();
    }

    [[nodiscard]] std::string str() const {
        return text_result.str();
    }

    [[nodiscard]] const TextResult &text() const noexcept {
        return text_result;
    }
};

[[nodiscard]] inline IdnaResult idna_to_ascii(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    mjb_idna_info info{};
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_idna_to_ascii(&text, output_encoding, &info,
        detail::ResultAccess::out(result));

    result = detail::ResultAccess::checked(std::move(result), status, "IDNA ToASCII failed");
    return IdnaResult(std::move(result), info.errors);
}

[[nodiscard]] inline IdnaResult idna_to_unicode(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    mjb_idna_info info{};
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_idna_to_unicode(&text, output_encoding, &info,
        detail::ResultAccess::out(result));

    result = detail::ResultAccess::checked(std::move(result), status, "IDNA ToUnicode failed");
    return IdnaResult(std::move(result), info.errors);
}

[[nodiscard]] inline TextResult case_map_result(std::string_view input, mjb_map_case_type type,
    mjb_locale locale = MJB_LOCALE_NOT_SET, mjb_encoding input_encoding = MJB_ENC_UTF_8,
    mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    TextResult result = detail::ResultAccess::create();
    const mjb_text text = mjb_text_make(input.data(), input.size(), input_encoding);
    const mjb_status status = mjb_map_case(&text, malformed_policy, type, locale, output_encoding,
        detail::ResultAccess::out(result), diagnostic);

    return detail::ResultAccess::checked(std::move(result), status, "Case mapping failed");
}

[[nodiscard]] inline std::string case_map(std::string_view input, mjb_map_case_type type,
    mjb_locale locale = MJB_LOCALE_NOT_SET, mjb_encoding input_encoding = MJB_ENC_UTF_8,
    mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map_result(input, type, locale, input_encoding, output_encoding, malformed_policy,
        diagnostic)
        .str();
}

[[nodiscard]] inline std::string uppercase(std::string_view input,
    mjb_locale locale = MJB_LOCALE_NOT_SET, mjb_encoding input_encoding = MJB_ENC_UTF_8,
    mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_UPPER, locale, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline std::string lowercase(std::string_view input,
    mjb_locale locale = MJB_LOCALE_NOT_SET, mjb_encoding input_encoding = MJB_ENC_UTF_8,
    mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_LOWER, locale, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline std::string titlecase(std::string_view input,
    mjb_locale locale = MJB_LOCALE_NOT_SET, mjb_encoding input_encoding = MJB_ENC_UTF_8,
    mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_TITLE, locale, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline std::string casefold(std::string_view input,
    mjb_locale locale = MJB_LOCALE_NOT_SET, mjb_encoding input_encoding = MJB_ENC_UTF_8,
    mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_CASEFOLD, locale, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

[[nodiscard]] inline std::string casefold_simple(std::string_view input,
    mjb_locale locale = MJB_LOCALE_NOT_SET, mjb_encoding input_encoding = MJB_ENC_UTF_8,
    mjb_encoding output_encoding = MJB_ENC_UTF_8,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_CASEFOLD_SIMPLE, locale, input_encoding, output_encoding,
        malformed_policy, diagnostic);
}

enum class CaselessMode {
    Canonical = MJB_CASELESS_CANONICAL,
    Unnormalized = MJB_CASELESS_UNNORMALIZED,
    Compatibility = MJB_CASELESS_COMPATIBILITY,
    Identifier = MJB_CASELESS_IDENTIFIER
};

[[nodiscard]] inline bool caseless_match(Text s1, Text s2,
    CaselessMode mode = CaselessMode::Canonical,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *s1_diagnostic = nullptr, mjb_diagnostic *s2_diagnostic = nullptr) {
    bool matches = false;
    detail::check_status(mjb_caseless_match(s1.get(), s2.get(), malformed_policy,
                             static_cast<mjb_caseless_mode>(mode), &matches, s1_diagnostic,
                             s2_diagnostic),
        "Caseless matching failed");

    return matches;
}

enum class CollationStrength {
    Primary = MJB_COLLATION_PRIMARY,
    Secondary = MJB_COLLATION_SECONDARY,
    Tertiary = MJB_COLLATION_TERTIARY,
    Quaternary = MJB_COLLATION_QUATERNARY
};

enum class CollationVariableWeighting {
    NonIgnorable = MJB_COLLATION_NON_IGNORABLE,
    Shifted = MJB_COLLATION_SHIFTED
};

[[nodiscard]] inline int collation_compare(Text s1, Text s2,
    CollationVariableWeighting variable_weighting = CollationVariableWeighting::NonIgnorable,
    CollationStrength strength = CollationStrength::Tertiary,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *s1_diagnostic = nullptr, mjb_diagnostic *s2_diagnostic = nullptr) {
    int order = 0;
    detail::check_status(mjb_collation_compare(s1.get(), s2.get(), malformed_policy,
                             static_cast<mjb_collation_variable_weighting>(variable_weighting),
                             static_cast<mjb_collation_strength>(strength), &order, s1_diagnostic,
                             s2_diagnostic),
        "Collation comparison failed");

    return order;
}

// The key is always newly allocated: unlike the other _result functions, it never borrows the
// input, so it can take a Text.
[[nodiscard]] inline TextResult collation_key_result(Text input,
    CollationVariableWeighting variable_weighting = CollationVariableWeighting::NonIgnorable,
    CollationStrength strength = CollationStrength::Tertiary,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_collation_key(input.get(), malformed_policy,
        static_cast<mjb_collation_variable_weighting>(variable_weighting),
        static_cast<mjb_collation_strength>(strength), detail::ResultAccess::out(result),
        diagnostic);

    return detail::ResultAccess::checked(std::move(result), status,
        "Collation key generation failed");
}

[[nodiscard]] inline std::string collation_key(Text input,
    CollationVariableWeighting variable_weighting = CollationVariableWeighting::NonIgnorable,
    CollationStrength strength = CollationStrength::Tertiary,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return collation_key_result(input, variable_weighting, strength, malformed_policy, diagnostic)
        .str();
}

struct EmojiSequence {
    mjb_emoji_sequence data{};

    [[nodiscard]] mjb_emoji_sequence_type type() const noexcept {
        return data.type;
    }

    [[nodiscard]] mjb_emoji_qualification qualification() const noexcept {
        return data.qualification;
    }

    [[nodiscard]] size_t size() const noexcept {
        return data.codepoint_count;
    }
};

[[nodiscard]] inline std::optional<EmojiSequence> emoji_sequence(Text input) {
    EmojiSequence sequence;
    const mjb_status status = mjb_emoji_sequence_info(input.get(), &sequence.data);

    if(status == MJB_STATUS_NOT_FOUND) {
        return std::nullopt;
    }

    detail::check_status(status, "Emoji sequence lookup failed");

    return sequence;
}

[[nodiscard]] inline bool is_emoji_sequence(Text input) noexcept {
    return mjb_is_emoji_sequence(input.get());
}

[[nodiscard]] inline bool is_rgi_emoji(Text input) noexcept {
    return mjb_is_rgi_emoji(input.get());
}

[[nodiscard]] inline size_t terminal_width(Text input,
    mjb_terminal_width_profile profile = MJB_TERMINAL_WIDTH_NARROW,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    size_t width = 0;
    detail::check_status(mjb_terminal_width(input.get(), malformed_policy, profile, &width,
                             diagnostic),
        "Terminal width calculation failed");

    return width;
}

[[nodiscard]] inline std::string hangul_syllable_name(mjb_codepoint codepoint) {
    std::array<char, 64> buffer{};
    detail::check_status(mjb_hangul_syllable_name(codepoint, buffer.data(), buffer.size()),
        "Hangul syllable name lookup failed");

    return std::string(buffer.data());
}

[[nodiscard]] inline std::array<mjb_codepoint, 3>
hangul_syllable_decompose(mjb_codepoint codepoint) {
    std::array<mjb_codepoint, 3> codepoints{};
    detail::check_status(mjb_hangul_syllable_decompose(codepoint, codepoints.data()),
        "Hangul syllable decomposition failed");

    return codepoints;
}

[[nodiscard]] inline mjb_codepoint hangul_syllable_compose(mjb_codepoint leading,
    mjb_codepoint vowel, mjb_codepoint trailing = 0) {
    mjb_codepoint syllable = MJB_CODEPOINT_NOT_VALID;
    detail::check_status(mjb_hangul_syllable_compose(leading, vowel, trailing, &syllable),
        "Hangul syllable composition failed");

    return syllable;
}

struct LocaleId {
    mjb_locale_id data{};

    [[nodiscard]] std::string_view language() const noexcept {
        return std::string_view(data.language);
    }

    [[nodiscard]] std::string_view extlang() const noexcept {
        return std::string_view(data.extlang);
    }

    [[nodiscard]] std::string_view script() const noexcept {
        return std::string_view(data.script);
    }

    [[nodiscard]] std::string_view region() const noexcept {
        return std::string_view(data.region);
    }

    [[nodiscard]] std::string_view variant() const noexcept {
        return std::string_view(data.variant);
    }

    [[nodiscard]] std::string_view extensions() const noexcept {
        return std::string_view(data.extensions);
    }

    [[nodiscard]] std::string_view private_use() const noexcept {
        return std::string_view(data.private_use);
    }

    [[nodiscard]] std::string_view grandfathered() const noexcept {
        return std::string_view(data.grandfathered);
    }

    [[nodiscard]] std::string_view code() const noexcept {
        return std::string_view(data.code);
    }

    [[nodiscard]] mjb_locale locale() const noexcept {
        return data.locale;
    }
};

[[nodiscard]] inline LocaleId locale_parse(Text id) {
    LocaleId locale;
    const mjb_status status = mjb_locale_parse(id.get(), &locale.data);

    if(status != MJB_STATUS_OK) {
        throw LocaleError(status, "Locale parsing failed");
    }

    return locale;
}

[[nodiscard]] inline std::string_view locale_code(mjb_locale locale) noexcept {
    const char *code = mjb_locale_code(locale);

    return code == nullptr ? std::string_view{} : std::string_view(code);
}

[[nodiscard]] inline std::string_view script_code(mjb_script script) noexcept {
    const char *code = mjb_script_code(script);

    return code == nullptr ? std::string_view{} : std::string_view(code);
}

[[nodiscard]] inline std::string_view version() noexcept {
    return std::string_view(mjb_version());
}

[[nodiscard]] inline unsigned int version_number() noexcept {
    return mjb_version_number();
}

[[nodiscard]] inline std::string_view unicode_version() noexcept {
    return std::string_view(mjb_unicode_version());
}

inline void set_allocator(const mjb_allocator *allocator) {
    detail::check_status(mjb_set_allocator(allocator), "Allocator selection failed");
}

[[nodiscard]] inline std::string_view grapheme_truncate(std::string_view input,
    size_t max_graphemes, mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const mjb_text text = mjb_text_make(input.data(), input.size(), encoding);
    const size_t n = mjb_grapheme_prefix_length(&text, max_graphemes);

    return input.substr(0, n);
}

[[nodiscard]] inline size_t grapheme_count(Text input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    size_t count = 0;
    detail::check_status(mjb_grapheme_count(input.get(), malformed_policy, &count, diagnostic),
        "Grapheme count failed");

    return count;
}

[[nodiscard]] inline size_t sentence_count(Text input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    size_t count = 0;
    detail::check_status(mjb_sentence_count(input.get(), malformed_policy, &count, diagnostic),
        "Sentence count failed");

    return count;
}

[[nodiscard]] inline size_t word_count(Text input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    size_t count = 0;
    detail::check_status(mjb_word_count(input.get(), malformed_policy, &count, diagnostic),
        "Word count failed");

    return count;
}

[[nodiscard]] inline std::string_view grapheme_truncate_width(std::string_view input,
    size_t max_columns, mjb_terminal_width_profile profile = MJB_TERMINAL_WIDTH_NARROW,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const mjb_text text = mjb_text_make(input.data(), input.size(), encoding);
    const size_t n = mjb_grapheme_prefix_length_for_width(&text, profile, max_columns);

    return input.substr(0, n);
}

[[nodiscard]] inline std::string_view word_truncate(std::string_view input, size_t max_segments,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const mjb_text text = mjb_text_make(input.data(), input.size(), encoding);
    const size_t n = mjb_word_prefix_length(&text, max_segments);

    return input.substr(0, n);
}

[[nodiscard]] inline std::string_view word_truncate_width(std::string_view input,
    size_t max_columns, mjb_terminal_width_profile profile = MJB_TERMINAL_WIDTH_NARROW,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const mjb_text text = mjb_text_make(input.data(), input.size(), encoding);
    const size_t n = mjb_word_prefix_length_for_width(&text, profile, max_columns);

    return input.substr(0, n);
}

class BidiParagraph {
    mjb_bidi_paragraph data{};

  public:
    explicit BidiParagraph(std::string_view input, mjb_direction direction = MJB_DIRECTION_AUTO,
        mjb_encoding encoding = MJB_ENC_UTF_8) {
        const mjb_text text = mjb_text_make(input.data(), input.size(), encoding);
        detail::check_status(mjb_bidi_resolve(&text, direction, &data),
            "Bidirectional paragraph resolution failed");
    }

    BidiParagraph(const BidiParagraph &) = delete;
    BidiParagraph &operator=(const BidiParagraph &) = delete;

    BidiParagraph(BidiParagraph &&other) noexcept : data(other.data) {
        other.data = {};
    }

    BidiParagraph &operator=(BidiParagraph &&other) noexcept {
        if(this != &other) {
            mjb_bidi_paragraph_free(&data);
            data = other.data;
            other.data = {};
        }

        return *this;
    }

    ~BidiParagraph() {
        mjb_bidi_paragraph_free(&data);
    }

    [[nodiscard]] bool empty() const noexcept {
        return data.count == 0;
    }

    [[nodiscard]] size_t size() const noexcept {
        return data.count;
    }

    [[nodiscard]] uint8_t paragraph_level() const noexcept {
        return data.paragraph_level;
    }

    [[nodiscard]] mjb_direction direction() const noexcept {
        return data.direction;
    }

    [[nodiscard]] const mjb_bidi_char *begin() const noexcept {
        return data.chars;
    }

    [[nodiscard]] const mjb_bidi_char *end() const noexcept {
        return data.chars == nullptr ? nullptr : data.chars + data.count;
    }

    [[nodiscard]] const mjb_bidi_char &operator[](size_t index) const noexcept {
        return data.chars[index];
    }

    [[nodiscard]] const mjb_bidi_char &at(size_t index) const {
        if(index >= data.count) {
            throw std::out_of_range("BidiParagraph character index out of range");
        }

        return data.chars[index];
    }

    [[nodiscard]] std::vector<size_t> visual_order(size_t line_start, size_t line_end) const {
        if(line_start > line_end || line_end > data.count) {
            throw LibraryError(MJB_STATUS_INVALID_ARGUMENT, "Invalid bidirectional line range");
        }

        if(line_start == line_end) {
            return {};
        }

        std::vector<size_t> order(line_end - line_start);
        detail::check_status(mjb_bidi_reorder_line(&data, line_start, line_end, order.data()),
            "Bidirectional line reordering failed");

        return order;
    }

    [[nodiscard]] std::vector<size_t> visual_order() const {
        return visual_order(0, data.count);
    }

    [[nodiscard]] std::vector<mjb_bidi_run>
    line_runs(const std::vector<size_t> &visual_order) const {
        for(size_t index : visual_order) {
            if(index >= data.count) {
                throw LibraryError(MJB_STATUS_INVALID_ARGUMENT,
                    "Invalid bidirectional visual order");
            }
        }

        size_t count = 0;
        detail::check_status(mjb_bidi_line_runs(&data, visual_order.data(), visual_order.size(),
                                 nullptr, &count),
            "Unable to count bidirectional line runs");

        std::vector<mjb_bidi_run> runs(count);
        detail::check_status(mjb_bidi_line_runs(&data, visual_order.data(), visual_order.size(),
                                 runs.data(), &count),
            "Unable to read bidirectional line runs");
        runs.resize(count);

        return runs;
    }

    [[nodiscard]] std::vector<mjb_bidi_run> line_runs() const {
        return line_runs(visual_order());
    }

    [[nodiscard]] const mjb_bidi_paragraph &raw() const noexcept {
        return data;
    }
};

struct BreakResult {
    // Byte offset of the reported position: the start of `codepoint`, or the text length at the
    // end. Positions start after the first codepoint; there is none for the start of the text.
    size_t position;
    // The codepoint that starts at `position`: U+FFFD for a malformed subsequence, and
    // MJB_CODEPOINT_NOT_VALID at the end of the text.
    mjb_codepoint codepoint;
    mjb_break_type type;

    [[nodiscard]] bool is_mandatory() const noexcept {
        return type == MJB_BT_MANDATORY;
    }

    [[nodiscard]] bool is_allowed() const noexcept {
        return type == MJB_BT_ALLOWED;
    }

    [[nodiscard]] bool is_no_break() const noexcept {
        return type == MJB_BT_NO_BREAK;
    }

    [[nodiscard]] bool is_break() const noexcept {
        return type == MJB_BT_MANDATORY || type == MJB_BT_ALLOWED;
    }
};

/**
 * A low-level break iterator that reports every position and its break type. To iterate the
 * segments themselves, use graphemes(), words(), sentences(), or line_segments() instead.
 */
template <typename State, mjb_break_type (*BreakFn)(const mjb_text *, State *)> class Breaker {
    Text text;
    State state{};
    bool done = false;

    // Decode at the reported position rather than reading the state's current codepoint, which
    // the iterators leave unchanged when a rule absorbs a character (SB5, WB4).
    [[nodiscard]] mjb_codepoint codepoint_at(size_t position) const noexcept {
        size_t offset = position;
        mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;

        if(mjb_decode_next(text.get(), MJB_MALFORMED_REPLACE, &offset, &codepoint, nullptr) !=
            MJB_STATUS_OK) {
            return MJB_CODEPOINT_NOT_VALID;
        }

        return codepoint;
    }

  public:
    explicit Breaker(Text input) noexcept : text(input) {
    }

    [[nodiscard]] std::optional<BreakResult> next() noexcept {
        if(done) {
            return std::nullopt;
        }

        const mjb_break_type type = BreakFn(text.get(), &state);

        if(type == MJB_BT_NOT_SET) {
            done = true;
            return std::nullopt;
        }

        return BreakResult{ state.position, codepoint_at(state.position), type };
    }

    void reset() noexcept {
        state = {};
        done = false;
    }

    [[nodiscard]] bool is_done() const noexcept {
        return done;
    }

    [[nodiscard]] std::string_view input() const noexcept {
        return text.bytes();
    }

    [[nodiscard]] mjb_encoding input_encoding() const noexcept {
        return text.encoding();
    }

    [[nodiscard]] const State &raw_state() const noexcept {
        return state;
    }

    template <typename Fn> void for_each(Fn &&fn) {
        while(auto result = next()) {
            fn(*result);
        }
    }
};

using WordBreaker = Breaker<mjb_word_break_state, mjb_word_break_next>;
using SentenceBreaker = Breaker<mjb_sentence_break_state, mjb_sentence_break_next>;
using LineBreaker = Breaker<mjb_line_break_state, mjb_line_break_next>;
using GraphemeBreaker = Breaker<mjb_grapheme_break_state, mjb_grapheme_break_next>;

namespace detail {

// True for a temporary std::string, std::u16string, or std::u32string: a range over it would
// dangle, because a range-based for loop destroys temporaries in its range expression before the
// loop body runs.
template <typename T> struct is_owning_string : std::false_type {};

template <typename CharT, typename Traits, typename Allocator>
struct is_owning_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

template <typename T>
inline constexpr bool is_temporary_owning_string_v = !std::is_lvalue_reference_v<T> &&
    is_owning_string<std::remove_cv_t<std::remove_reference_t<T>>>::value;

} // namespace detail

/**
 * The segments of a text, as views of the input counted in its code units. Iterate with a
 * range-based for loop. A malformed subsequence counts as one codepoint, and an incomplete
 * subsequence at the end forms the last segment. The range and its segments are views: the
 * input must outlive them.
 */
template <typename CharT, typename State, mjb_break_type (*BreakFn)(const mjb_text *, State *)>
class SegmentRange {
    std::basic_string_view<CharT> units;
    Text text;

  public:
    class iterator {
        const SegmentRange *range = nullptr;
        State state{};
        size_t start = 0;
        size_t end = 0;
        bool done = true;

        void advance() noexcept {
            start = end;
            const size_t length = range->text.bytes().size();
            mjb_break_type type;

            while((type = BreakFn(range->text.get(), &state)) != MJB_BT_NOT_SET) {
                if(type != MJB_BT_NO_BREAK && state.position > start) {
                    end = state.position;

                    return;
                }
            }

            // The iterator stops early before an incomplete subsequence at the end of the text,
            // which forms the last segment.
            if(start < length) {
                end = length;

                return;
            }

            done = true;
        }

      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::basic_string_view<CharT>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type *;
        using reference = value_type;

        iterator() noexcept = default;

        explicit iterator(const SegmentRange *segments) noexcept : range(segments), done(false) {
            advance();
        }

        [[nodiscard]] value_type operator*() const noexcept {
            return range->units.substr(start / sizeof(CharT), (end - start) / sizeof(CharT));
        }

        iterator &operator++() noexcept {
            advance();

            return *this;
        }

        iterator operator++(int) noexcept {
            iterator previous = *this;
            advance();

            return previous;
        }

        // Iterators of one range are equal when both are finished or both are at the same
        // segment.
        [[nodiscard]] bool operator==(const iterator &other) const noexcept {
            return done == other.done && (done || end == other.end);
        }

        [[nodiscard]] bool operator!=(const iterator &other) const noexcept {
            return !(*this == other);
        }
    };

    SegmentRange(std::basic_string_view<CharT> input, Text input_text) noexcept
        : units(input), text(input_text) {
    }

    [[nodiscard]] iterator begin() const noexcept {
        return iterator(this);
    }

    [[nodiscard]] iterator end() const noexcept {
        return iterator();
    }
};

template <typename CharT>
using GraphemeRange = SegmentRange<CharT, mjb_grapheme_break_state, mjb_grapheme_break_next>;
template <typename CharT>
using WordRange = SegmentRange<CharT, mjb_word_break_state, mjb_word_break_next>;
template <typename CharT>
using SentenceRange = SegmentRange<CharT, mjb_sentence_break_state, mjb_sentence_break_next>;
template <typename CharT>
using LineSegmentRange = SegmentRange<CharT, mjb_line_break_state, mjb_line_break_next>;

// The extended grapheme clusters (user-perceived characters). UTF-8, UTF-16, and UTF-32 strings
// give views of their own type; an mjb::Text with an explicit encoding gives byte views.
template <typename T, typename CharT = detail::text_char_t<T>,
    std::enable_if_t<!detail::is_temporary_owning_string_v<T>, int> = 0>
[[nodiscard]] GraphemeRange<CharT> graphemes(T &&input) noexcept {
    const std::basic_string_view<CharT> units(input);

    return GraphemeRange<CharT>(units, Text(units));
}

[[nodiscard]] inline GraphemeRange<char> graphemes(Text input) noexcept {
    return GraphemeRange<char>(input.bytes(), input);
}

template <typename T, std::enable_if_t<detail::is_temporary_owning_string_v<T>, int> = 0>
void graphemes(T &&input) = delete;

// The word segments of UAX #29, including the spaces and punctuation between words. UTF-8, UTF-16,
// and UTF-32 strings give views of their own type; an mjb::Text with an explicit encoding gives
// byte views.
template <typename T, typename CharT = detail::text_char_t<T>,
    std::enable_if_t<!detail::is_temporary_owning_string_v<T>, int> = 0>
[[nodiscard]] WordRange<CharT> words(T &&input) noexcept {
    const std::basic_string_view<CharT> units(input);

    return WordRange<CharT>(units, Text(units));
}

[[nodiscard]] inline WordRange<char> words(Text input) noexcept {
    return WordRange<char>(input.bytes(), input);
}

template <typename T, std::enable_if_t<detail::is_temporary_owning_string_v<T>, int> = 0>
void words(T &&input) = delete;

// The sentences of UAX #29. UTF-8, UTF-16, and UTF-32 strings give views of their own type; an
// mjb::Text with an explicit encoding gives byte views.
template <typename T, typename CharT = detail::text_char_t<T>,
    std::enable_if_t<!detail::is_temporary_owning_string_v<T>, int> = 0>
[[nodiscard]] SentenceRange<CharT> sentences(T &&input) noexcept {
    const std::basic_string_view<CharT> units(input);

    return SentenceRange<CharT>(units, Text(units));
}

[[nodiscard]] inline SentenceRange<char> sentences(Text input) noexcept {
    return SentenceRange<char>(input.bytes(), input);
}

template <typename T, std::enable_if_t<detail::is_temporary_owning_string_v<T>, int> = 0>
void sentences(T &&input) = delete;

// The line segments of UAX #14: each one ends at a line break opportunity and keeps its trailing
// spaces. UTF-8, UTF-16, and UTF-32 strings give views of their own type; an mjb::Text with an
// explicit encoding gives byte views.
template <typename T, typename CharT = detail::text_char_t<T>,
    std::enable_if_t<!detail::is_temporary_owning_string_v<T>, int> = 0>
[[nodiscard]] LineSegmentRange<CharT> line_segments(T &&input) noexcept {
    const std::basic_string_view<CharT> units(input);

    return LineSegmentRange<CharT>(units, Text(units));
}

[[nodiscard]] inline LineSegmentRange<char> line_segments(Text input) noexcept {
    return LineSegmentRange<char>(input.bytes(), input);
}

template <typename T, std::enable_if_t<detail::is_temporary_owning_string_v<T>, int> = 0>
void line_segments(T &&input) = delete;

/**
 * The codepoints of a text, decoded on the fly. A malformed subsequence follows
 * `malformed_policy`: the default replaces it with U+FFFD, because the range has no other way to
 * report it; MJB_MALFORMED_STOP ends the iteration and MJB_MALFORMED_SKIP drops it. The range is a
 * view: the input must outlive it.
 */
class CodepointRange {
    Text text;
    mjb_malformed_policy policy;

  public:
    class iterator {
        const CodepointRange *range = nullptr;
        size_t offset = 0;
        char32_t current = 0;
        bool done = true;

        void advance() noexcept {
            mjb_codepoint codepoint = 0;
            const mjb_status status = mjb_decode_next(range->text.get(), range->policy, &offset,
                &codepoint, nullptr);

            if(status != MJB_STATUS_OK) {
                done = true;

                return;
            }

            current = static_cast<char32_t>(codepoint);
        }

      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = char32_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const char32_t *;
        using reference = char32_t;

        iterator() noexcept = default;

        explicit iterator(const CodepointRange *codepoints) noexcept
            : range(codepoints), done(false) {
            advance();
        }

        [[nodiscard]] char32_t operator*() const noexcept {
            return current;
        }

        iterator &operator++() noexcept {
            advance();

            return *this;
        }

        iterator operator++(int) noexcept {
            iterator previous = *this;
            advance();

            return previous;
        }

        [[nodiscard]] bool operator==(const iterator &other) const noexcept {
            return done == other.done && (done || offset == other.offset);
        }

        [[nodiscard]] bool operator!=(const iterator &other) const noexcept {
            return !(*this == other);
        }
    };

    CodepointRange(Text input, mjb_malformed_policy malformed_policy) noexcept
        : text(input), policy(malformed_policy) {
    }

    [[nodiscard]] iterator begin() const noexcept {
        return iterator(this);
    }

    [[nodiscard]] iterator end() const noexcept {
        return iterator();
    }
};

[[nodiscard]] inline CodepointRange codepoints(Text input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_REPLACE) noexcept {
    return CodepointRange(input, malformed_policy);
}

template <typename T, std::enable_if_t<detail::is_temporary_owning_string_v<T>, int> = 0>
void codepoints(T &&input, mjb_malformed_policy malformed_policy = MJB_MALFORMED_REPLACE) = delete;

/**
 * UTF-16 and UTF-32 transforms and prefixes.
 *
 * Analysis functions take an mjb::Text, which already accepts UTF-16 and UTF-32 strings. These
 * functions return text, so they are templates that return the input's own string type: for
 * example nfc(u"...") returns a std::u16string, and grapheme_truncate(u"...") a std::u16string_view
 * counted in code units. Text is in the platform's byte order.
 */
// UTF-16 and UTF-32 transforms: the output has the input's string type and byte order

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> normalize(const T &input, NormalizationForm form,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return detail::units_from_bytes<
        CharT>(normalize_result(detail::as_bytes(std::basic_string_view<CharT>(input)), form,
        detail::unit_encoding<CharT>, detail::unit_encoding<CharT>, malformed_policy, diagnostic)
            .view());
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> nfc(const T &input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFC, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> nfd(const T &input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFD, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> nfkc(const T &input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFKC, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> nfkd(const T &input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return normalize(input, NormalizationForm::NFKD, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> filter(const T &input, mjb_filter_flags filters,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return detail::units_from_bytes<
        CharT>(filter_result(detail::as_bytes(std::basic_string_view<CharT>(input)), filters,
        detail::unit_encoding<CharT>, detail::unit_encoding<CharT>, malformed_policy, diagnostic)
            .view());
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> filter(const T &input, Filter filters,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return filter(input, static_cast<mjb_filter_flags>(filters), malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> nfkc_casefold(const T &input,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return detail::units_from_bytes<
        CharT>(nfkc_casefold_result(detail::as_bytes(std::basic_string_view<CharT>(input)),
        detail::unit_encoding<CharT>, detail::unit_encoding<CharT>, malformed_policy, diagnostic)
            .view());
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> confusable_skeleton(const T &input) {
    return detail::units_from_bytes<
        CharT>(confusable_skeleton_result(detail::as_bytes(std::basic_string_view<CharT>(input)),
        detail::unit_encoding<CharT>, detail::unit_encoding<CharT>)
            .view());
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> case_map(const T &input, mjb_map_case_type type,
    mjb_locale locale = MJB_LOCALE_NOT_SET,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return detail::units_from_bytes<
        CharT>(case_map_result(detail::as_bytes(std::basic_string_view<CharT>(input)), type, locale,
        detail::unit_encoding<CharT>, detail::unit_encoding<CharT>, malformed_policy, diagnostic)
            .view());
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> uppercase(const T &input,
    mjb_locale locale = MJB_LOCALE_NOT_SET,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_UPPER, locale, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> lowercase(const T &input,
    mjb_locale locale = MJB_LOCALE_NOT_SET,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_LOWER, locale, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> titlecase(const T &input,
    mjb_locale locale = MJB_LOCALE_NOT_SET,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_TITLE, locale, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> casefold(const T &input,
    mjb_locale locale = MJB_LOCALE_NOT_SET,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_CASEFOLD, locale, malformed_policy, diagnostic);
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string<CharT> casefold_simple(const T &input,
    mjb_locale locale = MJB_LOCALE_NOT_SET,
    mjb_malformed_policy malformed_policy = MJB_MALFORMED_STOP,
    mjb_diagnostic *diagnostic = nullptr) {
    return case_map(input, MJB_CASE_CASEFOLD_SIMPLE, locale, malformed_policy, diagnostic);
}

// UTF-16 and UTF-32 prefixes: the returned view is counted in code units

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string_view<CharT> grapheme_truncate(const T &input,
    size_t max_graphemes) noexcept {
    const std::basic_string_view<CharT> units(input);

    return units.substr(0,
        grapheme_truncate(detail::as_bytes(units), max_graphemes, detail::unit_encoding<CharT>)
                .size() /
            sizeof(CharT));
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string_view<CharT> grapheme_truncate_width(const T &input,
    size_t max_columns, mjb_terminal_width_profile profile = MJB_TERMINAL_WIDTH_NARROW) noexcept {
    const std::basic_string_view<CharT> units(input);

    return units.substr(0,
        grapheme_truncate_width(detail::as_bytes(units), max_columns, profile,
            detail::unit_encoding<CharT>)
                .size() /
            sizeof(CharT));
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string_view<CharT> word_truncate(const T &input,
    size_t max_segments) noexcept {
    const std::basic_string_view<CharT> units(input);

    return units.substr(0,
        word_truncate(detail::as_bytes(units), max_segments, detail::unit_encoding<CharT>).size() /
            sizeof(CharT));
}

template <typename T, typename CharT = detail::unit_char_t<T>>
[[nodiscard]] std::basic_string_view<CharT> word_truncate_width(const T &input, size_t max_columns,
    mjb_terminal_width_profile profile = MJB_TERMINAL_WIDTH_NARROW) noexcept {
    const std::basic_string_view<CharT> units(input);

    return units.substr(0,
        word_truncate_width(detail::as_bytes(units), max_columns, profile,
            detail::unit_encoding<CharT>)
                .size() /
            sizeof(CharT));
}

} // namespace mjb

#endif // MJB_CPP_MOJIBAKE_HPP
