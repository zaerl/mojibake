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
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mjb {

/**
 * An error reported by the C library.
 *
 * The status is retained so callers do not have to parse the diagnostic text.
 */
class LibraryError : public std::runtime_error {
    mjb_status error_status;

  public:
    LibraryError(mjb_status status, std::string_view message)
        : std::runtime_error(std::string(message)), error_status(status) {
    }

    explicit LibraryError(std::string_view message)
        : LibraryError(MJB_STATUS_INVALID_ARGUMENT, message) {
    }

    [[nodiscard]] mjb_status status() const noexcept {
        return error_status;
    }
};

class LocaleError : public LibraryError {
    mjb_error parse_error;

  public:
    LocaleError(mjb_status status, mjb_error error, std::string_view message)
        : LibraryError(status, message), parse_error(error) {
    }

    [[nodiscard]] mjb_error error() const noexcept {
        return parse_error;
    }
};

namespace detail {

inline void check_status(mjb_status status, std::string_view message) {
    if(status != MJB_STATUS_OK) {
        throw LibraryError(status, message);
    }
}

struct ResultAccess;

} // namespace detail

/**
 * See the mbj_result struct for details.
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

    [[nodiscard]] bool transformed() const noexcept {
        return data.transformed;
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

    [[nodiscard]] bool is_hangul_l() const noexcept {
        return mjb_codepoint_is_hangul_l(data.codepoint);
    }

    [[nodiscard]] bool is_hangul_v() const noexcept {
        return mjb_codepoint_is_hangul_v(data.codepoint);
    }

    [[nodiscard]] bool is_hangul_t() const noexcept {
        return mjb_codepoint_is_hangul_t(data.codepoint);
    }

    [[nodiscard]] bool is_hangul_jamo() const noexcept {
        return mjb_codepoint_is_hangul_jamo(data.codepoint);
    }

    [[nodiscard]] bool is_cjk_ideograph() const noexcept {
        return mjb_codepoint_is_cjk_ideograph(data.codepoint);
    }

    [[nodiscard]] bool is_cjk_extension() const noexcept {
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

[[nodiscard]] inline bool is_hangul_l(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_l(codepoint);
}

[[nodiscard]] inline bool is_hangul_v(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_v(codepoint);
}

[[nodiscard]] inline bool is_hangul_t(mjb_codepoint codepoint) noexcept {
    return mjb_codepoint_is_hangul_t(codepoint);
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

[[nodiscard]] inline bool is_cjk_extension(mjb_codepoint codepoint) noexcept {
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

[[nodiscard]] inline NumericValue codepoint_numeric_value(mjb_codepoint codepoint) {
    NumericValue value;
    detail::check_status(mjb_codepoint_numeric_value(codepoint, &value.data),
        "Unable to read numeric value");

    return value;
}

[[nodiscard]] inline mjb_encoding detect_encoding(std::string_view input) noexcept {
    return mjb_detect_encoding(input.data(), input.size());
}

[[nodiscard]] inline bool is_ascii(std::string_view input) noexcept {
    return mjb_string_is_ascii(input.data(), input.size());
}

[[nodiscard]] inline bool is_utf8(std::string_view input) noexcept {
    return mjb_string_is_utf8(input.data(), input.size());
}

[[nodiscard]] inline bool is_utf16(std::string_view input) noexcept {
    return mjb_string_is_utf16(input.data(), input.size());
}

[[nodiscard]] inline size_t length(std::string_view input,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_count_codepoints(input.data(), input.size(), encoding);
}

[[nodiscard]] inline mjb_status each_character(std::string_view input,
    mjb_string_each_character_fn callback, mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_string_each_character(input.data(), input.size(), encoding, callback);
}

[[nodiscard]] inline TextResult convert_encoding_result(std::string_view input,
    mjb_encoding input_encoding, mjb_encoding output_encoding) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_convert_encoding(input.data(), input.size(), input_encoding,
        output_encoding, detail::ResultAccess::out(result));

    return detail::ResultAccess::checked(std::move(result), status, "Encoding conversion failed");
}

[[nodiscard]] inline std::string convert_encoding(std::string_view input,
    mjb_encoding input_encoding, mjb_encoding output_encoding) {
    return convert_encoding_result(input, input_encoding, output_encoding).str();
}

[[nodiscard]] inline bool is_identifier(std::string_view input,
    mjb_identifier_profile profile = MJB_IDENTIFIER_DEFAULT,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_string_is_identifier(input.data(), input.size(), encoding, profile);
}

[[nodiscard]] inline bool is_confusable(std::string_view s1, std::string_view s2,
    mjb_encoding s1_encoding = MJB_ENC_UTF_8, mjb_encoding s2_encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_are_confusable(s1.data(), s1.size(), s1_encoding, s2.data(), s2.size(), s2_encoding);
}

[[nodiscard]] inline TextResult confusable_skeleton_result(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_confusable_skeleton(input.data(), input.size(), input_encoding,
        output_encoding, detail::ResultAccess::out(result));

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
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_normalize(input.data(), input.size(), input_encoding,
        static_cast<mjb_normalization>(form), output_encoding, detail::ResultAccess::out(result));

    return detail::ResultAccess::checked(std::move(result), status, "Normalization failed");
}

[[nodiscard]] inline std::string normalize(std::string_view input, NormalizationForm form,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return normalize_result(input, form, input_encoding, output_encoding).str();
}

[[nodiscard]] inline std::string nfc(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return normalize(input, NormalizationForm::NFC, input_encoding, output_encoding);
}

[[nodiscard]] inline std::string nfd(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return normalize(input, NormalizationForm::NFD, input_encoding, output_encoding);
}

[[nodiscard]] inline std::string nfkc(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return normalize(input, NormalizationForm::NFKC, input_encoding, output_encoding);
}

[[nodiscard]] inline std::string nfkd(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return normalize(input, NormalizationForm::NFKD, input_encoding, output_encoding);
}

[[nodiscard]] inline mjb_quick_check_result normalization_quick_check(std::string_view input,
    NormalizationForm form, mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_normalization_quick_check(input.data(), input.size(), encoding,
        static_cast<mjb_normalization>(form));
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

[[nodiscard]] inline TextResult filter_result(std::string_view input, mjb_filter filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_string_filter(input.data(), input.size(), input_encoding, filters,
        output_encoding, detail::ResultAccess::out(result));

    return detail::ResultAccess::checked(std::move(result), status, "String filtering failed");
}

[[nodiscard]] inline std::string filter(std::string_view input, mjb_filter filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return filter_result(input, filters, input_encoding, output_encoding).str();
}

[[nodiscard]] inline TextResult filter_result(std::string_view input, Filter filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return filter_result(input, static_cast<mjb_filter>(filters), input_encoding, output_encoding);
}

[[nodiscard]] inline std::string filter(std::string_view input, Filter filters,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return filter_result(input, filters, input_encoding, output_encoding).str();
}

[[nodiscard]] inline TextResult nfkc_casefold_result(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_nfkc_casefold(input.data(), input.size(), input_encoding,
        output_encoding, detail::ResultAccess::out(result));
    return detail::ResultAccess::checked(std::move(result), status, "NFKC case folding failed");
}

[[nodiscard]] inline std::string nfkc_casefold(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return nfkc_casefold_result(input, input_encoding, output_encoding).str();
}

[[nodiscard]] inline TextResult case_map_result(std::string_view input, mjb_map_case_type type,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_map_case(input.data(), input.size(), input_encoding, type,
        output_encoding, detail::ResultAccess::out(result));

    return detail::ResultAccess::checked(std::move(result), status, "Case mapping failed");
}

[[nodiscard]] inline std::string case_map(std::string_view input, mjb_map_case_type type,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return case_map_result(input, type, input_encoding, output_encoding).str();
}

[[nodiscard]] inline std::string uppercase(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return case_map(input, MJB_CASE_UPPER, input_encoding, output_encoding);
}

[[nodiscard]] inline std::string lowercase(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return case_map(input, MJB_CASE_LOWER, input_encoding, output_encoding);
}

[[nodiscard]] inline std::string titlecase(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return case_map(input, MJB_CASE_TITLE, input_encoding, output_encoding);
}

[[nodiscard]] inline std::string casefold(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return case_map(input, MJB_CASE_CASEFOLD, input_encoding, output_encoding);
}

[[nodiscard]] inline std::string casefold_simple(std::string_view input,
    mjb_encoding input_encoding = MJB_ENC_UTF_8, mjb_encoding output_encoding = MJB_ENC_UTF_8) {
    return case_map(input, MJB_CASE_CASEFOLD_SIMPLE, input_encoding, output_encoding);
}

[[nodiscard]] inline int compare(std::string_view s1, std::string_view s2,
    mjb_collation_mode mode = MJB_COLLATION_NON_IGNORABLE, mjb_encoding s1_encoding = MJB_ENC_UTF_8,
    mjb_encoding s2_encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_collation_compare(s1.data(), s1.size(), s1_encoding, s2.data(), s2.size(),
        s2_encoding, mode);
}

[[nodiscard]] inline TextResult collation_key_result(std::string_view input,
    mjb_collation_mode mode = MJB_COLLATION_NON_IGNORABLE, mjb_encoding encoding = MJB_ENC_UTF_8) {
    TextResult result = detail::ResultAccess::create();
    const mjb_status status = mjb_collation_key(input.data(), input.size(), encoding, mode,
        detail::ResultAccess::out(result));

    return detail::ResultAccess::checked(std::move(result), status,
        "Collation key generation failed");
}

[[nodiscard]] inline std::string collation_key(std::string_view input,
    mjb_collation_mode mode = MJB_COLLATION_NON_IGNORABLE, mjb_encoding encoding = MJB_ENC_UTF_8) {
    return collation_key_result(input, mode, encoding).str();
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

[[nodiscard]] inline std::optional<EmojiSequence> emoji_sequence(std::string_view input,
    mjb_encoding encoding = MJB_ENC_UTF_8) {
    EmojiSequence sequence;
    const mjb_status status = mjb_classify_emoji_sequence(input.data(), input.size(), encoding,
        &sequence.data);

    if(status == MJB_STATUS_NOT_FOUND) {
        return std::nullopt;
    }

    detail::check_status(status, "Emoji sequence lookup failed");

    return sequence;
}

[[nodiscard]] inline bool is_emoji_sequence(std::string_view input,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_string_is_emoji_sequence(input.data(), input.size(), encoding);
}

[[nodiscard]] inline bool is_rgi_emoji(std::string_view input,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    return mjb_string_is_rgi_emoji(input.data(), input.size(), encoding);
}

[[nodiscard]] inline size_t display_width(std::string_view input,
    mjb_width_context context = MJB_WIDTH_CONTEXT_AUTO, mjb_encoding encoding = MJB_ENC_UTF_8) {
    size_t width = 0;
    detail::check_status(mjb_display_width(input.data(), input.size(), encoding, context, &width),
        "Display width calculation failed");

    return width;
}

[[nodiscard]] inline std::string hangul_syllable_name(mjb_codepoint codepoint) {
    std::array<char, 64> buffer{};
    detail::check_status(mjb_hangul_syllable_name(codepoint, buffer.data(), buffer.size()),
        "Hangul syllable name lookup failed");

    return std::string(buffer.data());
}

[[nodiscard]] inline std::array<mjb_codepoint, 3>
hangul_syllable_decomposition(mjb_codepoint codepoint) {
    std::array<mjb_codepoint, 3> codepoints{};
    detail::check_status(mjb_hangul_syllable_decomposition(codepoint, codepoints.data()),
        "Hangul syllable decomposition failed");

    return codepoints;
}

[[nodiscard]] inline std::vector<mjb_buffer_character>
hangul_syllable_composition(std::vector<mjb_buffer_character> characters) {
    const size_t size = mjb_hangul_syllable_composition(characters.data(), characters.size());
    characters.resize(size);

    return characters;
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
};

[[nodiscard]] inline LocaleId parse_locale(std::string_view id,
    mjb_encoding encoding = MJB_ENC_UTF_8) {
    LocaleId locale;
    mjb_error error = MJB_ERROR_NONE;
    const mjb_status status = mjb_locale_parse(id.data(), id.size(), encoding, &locale.data,
        &error);

    if(status != MJB_STATUS_OK) {
        throw LocaleError(status, error, "Locale parsing failed");
    }

    return locale;
}

inline void set_locale(mjb_locale locale) {
    detail::check_status(mjb_set_locale(static_cast<unsigned int>(locale)),
        "Locale selection failed");
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

inline void set_memory_functions(mjb_alloc_fn allocate_fn, mjb_realloc_fn reallocate_fn,
    mjb_free_fn free_fn) {
    detail::check_status(mjb_set_memory_functions(allocate_fn, reallocate_fn, free_fn),
        "Memory function selection failed");
}

inline void Reset() noexcept {
    mjb_reset();
}

[[nodiscard]] inline void *allocate(size_t byte_length) noexcept {
    return mjb_alloc(byte_length);
}

[[nodiscard]] inline void *reallocate(void *pointer, size_t byte_length) noexcept {
    return mjb_realloc(pointer, byte_length);
}

inline void deallocate(void *pointer) noexcept {
    mjb_free(pointer);
}

[[nodiscard]] inline std::string_view truncate_grapheme(std::string_view input,
    size_t max_graphemes, mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const size_t n = mjb_truncate_grapheme(input.data(), input.size(), encoding, max_graphemes);

    return input.substr(0, n);
}

[[nodiscard]] inline std::string_view truncate_grapheme_width(std::string_view input,
    size_t max_columns, mjb_width_context context = MJB_WIDTH_CONTEXT_AUTO,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const size_t n = mjb_truncate_grapheme_width(input.data(), input.size(), encoding, context,
        max_columns);

    return input.substr(0, n);
}

[[nodiscard]] inline std::string_view truncate_word(std::string_view input, size_t max_segments,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const size_t n = mjb_truncate_word(input.data(), input.size(), encoding, max_segments);

    return input.substr(0, n);
}

[[nodiscard]] inline std::string_view truncate_word_width(std::string_view input,
    size_t max_columns, mjb_width_context context = MJB_WIDTH_CONTEXT_AUTO,
    mjb_encoding encoding = MJB_ENC_UTF_8) noexcept {
    const size_t n = mjb_truncate_word_width(input.data(), input.size(), encoding, context,
        max_columns);

    return input.substr(0, n);
}

class BidiParagraph {
    mjb_bidi_paragraph data{};

  public:
    explicit BidiParagraph(std::string_view input, mjb_direction direction = MJB_DIRECTION_AUTO,
        mjb_encoding encoding = MJB_ENC_UTF_8) {
        detail::check_status(mjb_bidi_resolve(input.data(), input.size(), encoding, direction,
                                 &data),
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
    size_t index;
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

template <typename State, mjb_break_type (*BreakFn)(const char *, size_t, mjb_encoding, State *)>
class Breaker {
    std::string_view buffer;
    mjb_encoding encoding;
    State state{};
    bool done = false;

  public:
    explicit Breaker(std::string_view input, mjb_encoding input_encoding = MJB_ENC_UTF_8) noexcept
        : buffer(input), encoding(input_encoding) {
    }

    [[nodiscard]] std::optional<BreakResult> next() noexcept {
        if(done) {
            return std::nullopt;
        }

        const mjb_break_type type = BreakFn(buffer.data(), buffer.size(), encoding, &state);

        if(type == MJB_BT_NOT_SET) {
            done = true;
            return std::nullopt;
        }

        return BreakResult{ state.index, state.current_codepoint, type };
    }

    void reset() noexcept {
        state = {};
        done = false;
    }

    [[nodiscard]] bool is_done() const noexcept {
        return done;
    }

    [[nodiscard]] std::string_view input() const noexcept {
        return buffer;
    }

    [[nodiscard]] mjb_encoding input_encoding() const noexcept {
        return encoding;
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

using WordBreaker = Breaker<mjb_next_word_state, mjb_next_word_break>;
using SentenceBreaker = Breaker<mjb_next_sentence_state, mjb_next_sentence_break>;
using LineBreaker = Breaker<mjb_next_line_state, mjb_next_line_break>;
using GraphemeBreaker = Breaker<mjb_next_state, mjb_next_grapheme_break>;

} // namespace mjb

#endif // MJB_CPP_MOJIBAKE_HPP
