/**
 * The Mojibake C++ library wrapper
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_CPP_MOJIBAKE_HPP
#define MJB_CPP_MOJIBAKE_HPP

#include "mojibake.h"

#include <string>
#include <string_view>
#include <optional>
#include <stdexcept>
#include <array>

namespace mjb {

class LibraryError : public std::runtime_error {
public:
    explicit LibraryError(std::string_view message) : std::runtime_error(std::string(message)) {}
};

/**
 * See mjb_character for details.
 */
class Character {
private:
    mjb_character data{};

    explicit Character(const mjb_character &character) noexcept : data(character) {}

public:
    explicit Character(mjb_codepoint codepoint) {
        if(mjb_codepoint_character(codepoint, &data) != MJB_STATUS_OK) {
            throw LibraryError("Invalid codepoint: " + std::to_string(codepoint));
        }
    }

    explicit Character(char32_t codepoint) : Character(static_cast<mjb_codepoint>(codepoint)) {}

    [[nodiscard]] static std::optional<Character> from(mjb_codepoint codepoint) noexcept {
        mjb_character data{};

        if(mjb_codepoint_character(codepoint, &data) != MJB_STATUS_OK) {
            return std::nullopt;
        }

        return Character(data);
    }

    bool operator==(const Character& other) const noexcept {
        return data.codepoint == other.data.codepoint;
    }

    bool operator!=(const Character& other) const noexcept {
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
        return data.decimal == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{data.decimal};
    }

    [[nodiscard]] std::optional<int> digit_value() const noexcept {
        return data.digit == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{data.digit};
    }

    [[nodiscard]] std::string_view numeric_value() const noexcept {
        return std::string_view(data.numeric);
    }

    [[nodiscard]] bool is_mirrored() const noexcept {
        return data.mirrored;
    }

    [[nodiscard]] std::optional<mjb_codepoint> uppercase() const noexcept {
        return data.uppercase == 0 ? std::nullopt : std::optional<mjb_codepoint>{data.uppercase};
    }

    [[nodiscard]] std::optional<mjb_codepoint> lowercase() const noexcept {
        return data.lowercase == 0 ? std::nullopt : std::optional<mjb_codepoint>{data.lowercase};
    }

    [[nodiscard]] std::optional<mjb_codepoint> titlecase() const noexcept {
        return data.titlecase == 0 ? std::nullopt : std::optional<mjb_codepoint>{data.titlecase};
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

    [[nodiscard]] bool is_cjk_ideograph() const noexcept {
        return mjb_codepoint_is_cjk_ideograph(data.codepoint);
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
            throw LibraryError("Failed to encode codepoint to UTF-8");
        }

        return std::string(buffer.data(), len);
    }

    [[nodiscard]] const mjb_character& raw() const noexcept {
        return data;
    }
};

struct NumericValue {
    mjb_numeric_value data{};

    [[nodiscard]] std::optional<int> decimal() const noexcept {
        return data.decimal == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{data.decimal};
    }

    [[nodiscard]] std::optional<int> digit() const noexcept {
        return data.digit == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{data.digit};
    }

    [[nodiscard]] std::string_view numeric() const noexcept {
        return std::string_view(data.numeric);
    }
};

inline NumericValue codepoint_numeric_value(mjb_codepoint codepoint) {
    NumericValue value;

    if(mjb_codepoint_numeric_value(codepoint, &value.data) != MJB_STATUS_OK) {
        throw LibraryError("Invalid codepoint: " + std::to_string(codepoint));
    }

    return value;
}

inline bool is_identifier(std::string_view input,
    mjb_identifier_profile profile = MJB_IDENTIFIER_DEFAULT) {
    return mjb_string_is_identifier(input.data(), input.size(), MJB_ENC_UTF_8, profile);
}

inline bool is_confusable(std::string_view s1, std::string_view s2) {
    return mjb_string_is_confusable(s1.data(), s1.size(), MJB_ENC_UTF_8, s2.data(), s2.size(),
        MJB_ENC_UTF_8);
}

enum class NormalizationForm {
    NFC = MJB_NORMALIZATION_NFC,
    NFD = MJB_NORMALIZATION_NFD,
    NFKC = MJB_NORMALIZATION_NFKC,
    NFKD = MJB_NORMALIZATION_NFKD
};

inline std::string normalize(std::string_view input, NormalizationForm form) {
    if(input.empty()) {
        return std::string{};
    }

    mjb_result result{};
    bool success = mjb_normalize(input.data(), input.size(), static_cast<mjb_normalization>(form),
        MJB_ENC_UTF_8, MJB_ENC_UTF_8, &result) == MJB_STATUS_OK;

    if(!success) {
        throw LibraryError("Normalization failed");
    }

    if(!result.output) {
        throw LibraryError("Normalization returned null output");
    }

    std::string normalized_string(result.output, result.output_size);

    if(result.transformed) {
        // A new string is returned, it must be freed.
        mjb_free(result.output);
    }

    return normalized_string;
}

inline std::string nfc(std::string_view input) {
    return normalize(input, NormalizationForm::NFC);
}

inline std::string nfd(std::string_view input) {
    return normalize(input, NormalizationForm::NFD);
}

inline std::string nfkc(std::string_view input) {
    return normalize(input, NormalizationForm::NFKC);
}

inline std::string nfkd(std::string_view input) {
    return normalize(input, NormalizationForm::NFKD);
}

inline std::string collation_key(std::string_view input,
    mjb_collation_mode mode = MJB_COLLATION_NON_IGNORABLE) {
    if(input.empty()) {
        return std::string{};
    }

    mjb_result result{};
    mjb_status status = mjb_collation_key(input.data(), input.size(), MJB_ENC_UTF_8, mode,
        &result);

    if(status != MJB_STATUS_OK) {
        throw LibraryError("Collation key generation failed");
    }

    std::string key(result.output, result.output_size);

    if(result.transformed) {
        mjb_free(result.output);
    }

    return key;
}

inline std::string_view truncate(std::string_view input, size_t max_graphemes) {
    size_t n = mjb_truncate(input.data(), input.size(), MJB_ENC_UTF_8, max_graphemes);
    return input.substr(0, n);
}

inline std::string_view truncate_width(std::string_view input, size_t max_columns,
    mjb_width_context context = MJB_WIDTH_CONTEXT_AUTO) {
    size_t n = mjb_truncate_width(input.data(), input.size(), MJB_ENC_UTF_8, context,
        max_columns);
    return input.substr(0, n);
}

inline std::string_view truncate_word(std::string_view input, size_t max_segments) {
    size_t n = mjb_truncate_word(input.data(), input.size(), MJB_ENC_UTF_8, max_segments);
    return input.substr(0, n);
}

inline std::string_view truncate_word_width(std::string_view input, size_t max_columns,
    mjb_width_context context = MJB_WIDTH_CONTEXT_AUTO) {
    size_t n = mjb_truncate_word_width(input.data(), input.size(), MJB_ENC_UTF_8, context,
        max_columns);
    return input.substr(0, n);
}

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

template<typename State, mjb_break_type(*BreakFn)(const char*, size_t, mjb_encoding, State*)>
class Breaker {
    std::string_view buffer;
    State state{};
    bool done = false;

public:
    explicit Breaker(std::string_view input) noexcept : buffer(input) {}

    [[nodiscard]] std::optional<BreakResult> next() {
        if(done) {
            return std::nullopt;
        }

        mjb_break_type type = BreakFn(buffer.data(), buffer.size(), MJB_ENC_UTF_8, &state);

        if(type == MJB_BT_NOT_SET) {
            done = true;
            return std::nullopt;
        }

        return BreakResult{state.index, state.current_codepoint, type};
    }

    void reset() noexcept {
        state = {};
        done = false;
    }

    [[nodiscard]] bool is_done() const noexcept {
        return done;
    }

    template<typename Fn>
    void for_each(Fn&& fn) {
        while(auto result = next()) {
            fn(*result);
        }
    }
};

using WordBreaker = Breaker<mjb_next_word_state, mjb_break_word>;
using SentenceBreaker = Breaker<mjb_next_sentence_state, mjb_break_sentence>;
using LineBreaker = Breaker<mjb_next_line_state, mjb_break_line>;
using GraphemeBreaker = Breaker<mjb_next_state, mjb_break_grapheme_cluster>;

} // namespace mjb

#endif // MJB_CPP_MOJIBAKE_HPP
