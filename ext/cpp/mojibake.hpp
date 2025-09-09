/**
 * The Mojibake C++ library wrapper
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_CPP_MOJIBAKE_HPP
#define MJB_CPP_MOJIBAKE_HPP

#include "../../src/mojibake.h"

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

class Character {
private:
    mjb_character data;
    bool valid;

    void ensure_valid() const {
        if (!valid) {
            throw LibraryError("Library is not initialized");
        }
    }

public:
    Character() = default;
    Character(const Character&) = default;
    Character& operator=(const Character&) = default;
    Character(Character&&) noexcept = default;
    Character& operator=(Character&&) noexcept = default;
    ~Character() = default;

    explicit Character(mjb_codepoint codepoint) : data{}, valid(false) {
        valid = mjb_codepoint_character(codepoint, &data);

        if (!valid) {
            throw LibraryError("Invalid codepoint: " + std::to_string(codepoint));
        }
    }

    explicit Character(char32_t codepoint) : Character(static_cast<mjb_codepoint>(codepoint)) {}

    bool operator==(const Character& other) const noexcept {
        if (!valid || !other.valid) return false;
        return data.codepoint == other.data.codepoint;
    }

    bool operator!=(const Character& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return valid;
    }

    [[nodiscard]] mjb_codepoint codepoint() const {
        ensure_valid();
        return data.codepoint;
    }

    [[nodiscard]] std::string_view name() const {
        ensure_valid();
        return std::string_view(data.name);
    }

    [[nodiscard]] mjb_category category() const {
        ensure_valid();
        return data.category;
    }

    [[nodiscard]] mjb_canonical_combining_class combining_class() const {
        ensure_valid();
        return data.combining;
    }

    [[nodiscard]] unsigned short bidirectional() const {
        ensure_valid();
        return data.bidirectional;
    }

    [[nodiscard]] mjb_decomposition decomposition_type() const {
        ensure_valid();
        return data.decomposition;
    }

    [[nodiscard]] std::optional<int> decimal_value() const {
        ensure_valid();
        return data.decimal == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{data.decimal};
    }

    [[nodiscard]] std::optional<int> digit_value() const {
        ensure_valid();
        return data.digit == MJB_NUMBER_NOT_VALID ? std::nullopt : std::optional<int>{data.digit};
    }

    [[nodiscard]] std::string_view numeric_value() const {
        ensure_valid();
        return std::string_view(data.numeric);
    }

    [[nodiscard]] bool is_mirrored() const {
        ensure_valid();
        return data.mirrored;
    }

    [[nodiscard]] std::optional<mjb_codepoint> uppercase() const {
        ensure_valid();
        return data.uppercase == 0 ? std::nullopt : std::optional<mjb_codepoint>{data.uppercase};
    }

    [[nodiscard]] std::optional<mjb_codepoint> lowercase() const {
        ensure_valid();
        return data.lowercase == 0 ? std::nullopt : std::optional<mjb_codepoint>{data.lowercase};
    }

    [[nodiscard]] std::optional<mjb_codepoint> titlecase() const {
        ensure_valid();
        return data.titlecase == 0 ? std::nullopt : std::optional<mjb_codepoint>{data.titlecase};
    }

    [[nodiscard]] bool is_combining() const {
        ensure_valid();
        return mjb_category_is_combining(data.category);
    }

    [[nodiscard]] bool is_graphic() const {
        ensure_valid();
        return mjb_category_is_graphic(data.category);
    }

    [[nodiscard]] bool is_hangul_syllable() const {
        ensure_valid();
        return mjb_codepoint_is_hangul_syllable(data.codepoint);
    }

    [[nodiscard]] bool is_cjk_ideograph() const {
        ensure_valid();
        return mjb_codepoint_is_cjk_ideograph(data.codepoint);
    }

    [[nodiscard]] mjb_plane plane() const {
        ensure_valid();
        return mjb_codepoint_plane(data.codepoint);
    }

    [[nodiscard]] std::string to_utf8() const {
        ensure_valid();

        std::array<char, 5> buffer{};
        unsigned int len = mjb_codepoint_encode(data.codepoint, buffer.data(), buffer.size(),
            MJB_ENCODING_UTF_8);

        if (len == 0) {
            throw LibraryError("Failed to encode codepoint to UTF-8");
        }

        return std::string(buffer.data(), len);
    }

    [[nodiscard]] const mjb_character& raw() const {
        ensure_valid();
        return data;
    }
};

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

    mjb_normalization_result result{};
    bool success = mjb_normalize(input.data(), input.size(), MJB_ENCODING_UTF_8,
        static_cast<mjb_normalization>(form), &result);

    if(!success) {
        throw LibraryError("Normalization failed");
    }

    if(!result.output) {
        throw LibraryError("Normalization returned null output");
    }

    std::string normalized_string(result.output, result.output_size);

    if(result.normalized) {
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

} // namespace mjb

#endif // MJB_CPP_MOJIBAKE_HPP
