/**
 * The Mojibake C++ library wrapper
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#pragma once

#ifndef MJB_CPP_MOJIBAKE_HPP
#define MJB_CPP_MOJIBAKE_HPP

// MSVC reports __cplusplus as 199711L unless /Zc:__cplusplus is set, so read _MSVC_LANG there.
// Any mode after C++17 is accepted: GCC 10 reports 201709L for -std=c++20.
#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
#if _MSVC_LANG <= 201703L
#error "mojibake.hpp requires C++20 or later"
#endif
#elif __cplusplus <= 201703L
#error "mojibake.hpp requires C++20 or later"
#endif

#include "mojibake.h"

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

// Lets Clang warn when a view outlives the string it was made from
#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(clang::lifetimebound)
#define MJB_LIFETIMEBOUND [[clang::lifetimebound]]
#endif
#endif

#ifndef MJB_LIFETIMEBOUND
#define MJB_LIFETIMEBOUND
#endif

namespace mjb {

// Errors

[[nodiscard]] inline std::string_view status_message(mjb_status status) noexcept {
    const char *message = mjb_status_message(status);

    return message == nullptr ? std::string_view{} : std::string_view(message);
}

/**
 * The std::error_category of mjb_status values, so a status can be stored and compared as a
 * std::error_code.
 */
[[nodiscard]] inline const std::error_category &error_category() noexcept {
    class Category final : public std::error_category {
      public:
        [[nodiscard]] const char *name() const noexcept override {
            return "mojibake";
        }

        [[nodiscard]] std::string message(int status) const override {
            return std::string(status_message(static_cast<mjb_status>(status)));
        }
    };

    static const Category category;

    return category;
}

[[nodiscard]] inline std::error_code make_error_code(mjb_status status) noexcept {
    return { static_cast<int>(status), error_category() };
}

/**
 * An error reported by the C library.
 *
 * what() describes the failed operation followed by the library's message for the status, for
 * example "Normalization failed: The normalization form is invalid". The status is retained so
 * callers do not have to parse that text.
 */
class LibraryError : public std::system_error {
  public:
    LibraryError(mjb_status status, const char *operation)
        : std::system_error(make_error_code(status), operation) {
    }

    [[nodiscard]] mjb_status status() const noexcept {
        return static_cast<mjb_status>(code().value());
    }
};

namespace detail {

inline void check_status(mjb_status status, const char *operation) {
    if(status != MJB_STATUS_OK) {
        throw LibraryError(status, operation);
    }
}

} // namespace detail

// Text input

inline constexpr mjb_encoding native_utf16_encoding = std::endian::native == std::endian::big ?
    MJB_ENC_UTF_16BE :
    MJB_ENC_UTF_16LE;
inline constexpr mjb_encoding native_utf32_encoding = std::endian::native == std::endian::big ?
    MJB_ENC_UTF_32BE :
    MJB_ENC_UTF_32LE;

// The code unit types of UTF-8, UTF-16, and UTF-32 strings.
template <typename T>
concept code_unit = std::same_as<T, char> || std::same_as<T, char8_t> ||
    std::same_as<T, char16_t> || std::same_as<T, char32_t>;

namespace detail {

// The code unit type of a string argument, or void when it is not a UTF string.
template <typename T> constexpr auto text_char_tag() noexcept {
    if constexpr(std::is_same_v<T, std::nullptr_t>) {
        return std::type_identity<void>{};
    } else if constexpr(std::is_convertible_v<const T &, std::string_view>) {
        return std::type_identity<char>{};
    } else if constexpr(std::is_convertible_v<const T &, std::u8string_view>) {
        return std::type_identity<char8_t>{};
    } else if constexpr(std::is_convertible_v<const T &, std::u16string_view>) {
        return std::type_identity<char16_t>{};
    } else if constexpr(std::is_convertible_v<const T &, std::u32string_view>) {
        return std::type_identity<char32_t>{};
    } else {
        return std::type_identity<void>{};
    }
}

template <typename T>
using text_char_t = typename decltype(text_char_tag<std::remove_cvref_t<T>>())::type;

template <code_unit CharT>
inline constexpr mjb_encoding encoding_of = std::same_as<CharT, char16_t> ?
    native_utf16_encoding :
    (std::same_as<CharT, char32_t> ? native_utf32_encoding : MJB_ENC_UTF_8);

template <typename T> struct is_owning_string : std::false_type {};

template <typename CharT, typename Traits, typename Allocator>
struct is_owning_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

} // namespace detail

/**
 * A UTF-8, UTF-16, or UTF-32 string or string view in the platform's byte order: std::string,
 * std::u8string, std::u16string, std::u32string, their views, and literals of each.
 */
template <typename T>
concept text_like = !std::is_void_v<detail::text_char_t<T>>;

/**
 * A view of text and its encoding, the C++ counterpart of mjb_text.
 *
 * It converts implicitly from any text_like string, so analysis functions take any of them. Pass
 * an explicit encoding for another byte order or for bytes read from a file:
 * mjb::Text(bytes, MJB_ENC_UTF_16BE). Like std::string_view, it does not own the text, which must
 * outlive it.
 */
class Text {
    mjb_text data;

    template <code_unit CharT>
    [[nodiscard]] static mjb_text make(std::basic_string_view<CharT> units,
        mjb_encoding encoding) noexcept {
        return mjb_text_make(reinterpret_cast<const char *>(units.data()),
            units.size() * sizeof(CharT), encoding);
    }

  public:
    // An empty UTF-8 text
    Text() noexcept : data(mjb_text_make("", 0, MJB_ENC_UTF_8)) {
    }

    template <text_like T>
    Text(const T &input MJB_LIFETIMEBOUND) noexcept // NOLINT(google-explicit-constructor)
        : data(make(std::basic_string_view<detail::text_char_t<T>>(input),
              detail::encoding_of<detail::text_char_t<T>>)) {
    }

    template <text_like T>
    Text(const T &input MJB_LIFETIMEBOUND, mjb_encoding encoding) noexcept
        : data(make(std::basic_string_view<detail::text_char_t<T>>(input), encoding)) {
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
 * Anything a function that reads text accepts: a text_like string or an mjb::Text. Transforms
 * return the input's own string type in the input's encoding, so an mjb::Text with an explicit
 * encoding gives a std::string of bytes in that encoding.
 */
template <typename T>
concept text_input = text_like<T> || std::same_as<std::remove_cvref_t<T>, Text>;

/**
 * A text_input that a returned view can point into: anything except a temporary std::string,
 * std::u16string, and so on, which would be destroyed before the view is used.
 */
template <typename T>
concept borrowed_text = text_input<T> &&
    (std::is_lvalue_reference_v<T> || !detail::is_owning_string<std::remove_cvref_t<T>>::value);

namespace detail {

// The code unit type of the views and strings a function returns for an input.
template <typename T> struct input_char {
    using type = text_char_t<T>;
};

template <> struct input_char<Text> {
    using type = char;
};

template <text_input T> using input_char_t = typename input_char<std::remove_cvref_t<T>>::type;

template <text_input T>
[[nodiscard]] std::basic_string_view<input_char_t<T>>
view_of(const T &input MJB_LIFETIMEBOUND) noexcept {
    if constexpr(std::same_as<std::remove_cvref_t<T>, Text>) {
        return input.bytes();
    } else {
        return std::basic_string_view<input_char_t<T>>(input);
    }
}

} // namespace detail

// Options

/**
 * How a function handles malformed input. MJB_MALFORMED_STOP throws, MJB_MALFORMED_REPLACE
 * substitutes U+FFFD, and MJB_MALFORMED_SKIP drops the malformed bytes. diagnostic, when set,
 * receives where the first malformed subsequence is.
 *
 * Use designated initializers: mjb::nfc(input, { .malformed = MJB_MALFORMED_REPLACE }).
 */
struct Options {
    mjb_malformed_policy malformed = MJB_MALFORMED_STOP;
    mjb_diagnostic *diagnostic = nullptr;
};

// Options of case mapping: the locale whose tailorings apply, then the Options fields.
struct CaseOptions {
    mjb_locale locale = MJB_LOCALE_NOT_SET;
    mjb_malformed_policy malformed = MJB_MALFORMED_STOP;
    mjb_diagnostic *diagnostic = nullptr;
};

// Options of functions that compare two texts: other_diagnostic reports the second one.
struct MatchOptions {
    mjb_malformed_policy malformed = MJB_MALFORMED_STOP;
    mjb_diagnostic *diagnostic = nullptr;
    mjb_diagnostic *other_diagnostic = nullptr;
};

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

// Options of collation_compare and collation_key, which ignores other_diagnostic.
struct CollationOptions {
    CollationStrength strength = CollationStrength::Tertiary;
    CollationVariableWeighting variable_weighting = CollationVariableWeighting::NonIgnorable;
    mjb_malformed_policy malformed = MJB_MALFORMED_STOP;
    mjb_diagnostic *diagnostic = nullptr;
    mjb_diagnostic *other_diagnostic = nullptr;
};

enum class NormalizationForm {
    NFC = MJB_NORMALIZATION_NFC,
    NFD = MJB_NORMALIZATION_NFD,
    NFKC = MJB_NORMALIZATION_NFKC,
    NFKD = MJB_NORMALIZATION_NFKD
};

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

// Output

/**
 * Output bytes in any encoding, for a transform that writes into existing storage:
 * mjb::nfc(input, mjb::encoded(bytes, MJB_ENC_UTF_16LE)).
 */
class EncodedOutput {
    std::string *output;
    mjb_encoding output_encoding;

  public:
    EncodedOutput(std::string &bytes MJB_LIFETIMEBOUND, mjb_encoding encoding) noexcept
        : output(&bytes), output_encoding(encoding) {
    }

    [[nodiscard]] std::string &bytes() const noexcept {
        return *output;
    }

    [[nodiscard]] mjb_encoding encoding() const noexcept {
        return output_encoding;
    }
};

[[nodiscard]] inline EncodedOutput encoded(std::string &bytes MJB_LIFETIMEBOUND,
    mjb_encoding encoding) noexcept {
    return EncodedOutput(bytes, encoding);
}

namespace detail {

template <typename T> struct is_unit_string : std::false_type {};

template <code_unit CharT, typename Allocator>
struct is_unit_string<std::basic_string<CharT, std::char_traits<CharT>, Allocator>>
    : std::true_type {};

} // namespace detail

/**
 * Where a transform can write: a std::string (UTF-8), std::u8string, std::u16string, or
 * std::u32string lvalue, or an EncodedOutput. The output is replaced, not appended to, and its
 * capacity is reused, so calling a transform in a loop with the same string avoids an allocation
 * per call.
 */
template <typename T>
concept output_target = (std::is_lvalue_reference_v<T> &&
                            !std::is_const_v<std::remove_reference_t<T>> &&
                            detail::is_unit_string<std::remove_cvref_t<T>>::value) ||
    std::same_as<std::remove_cvref_t<T>, EncodedOutput>;

namespace detail {

/**
 * Write the output of a C _into function into out. write(buffer) calls the function. The first
 * call offers out's capacity, or 1.5 * size_hint bytes if that is larger; when the output does not
 * fit, the second call offers the required size it reported. On failure out is left empty and
 * LibraryError is thrown.
 */
template <code_unit CharT, typename Allocator, typename Write>
void write_into(std::basic_string<CharT, std::char_traits<CharT>, Allocator> &out,
    mjb_encoding encoding, size_t size_hint, const Write &write, const char *operation) {
    // Half again the hint covers the usual growth, such as NFD of accented letters or "ß" to "SS",
    // so a second, full pass is rare
    const size_t bytes = size_hint + size_hint / 2;
    size_t units = std::max(out.capacity(), (bytes + sizeof(CharT) - 1) / sizeof(CharT));

    for(bool retried = false;; retried = true) {
        // A non-empty string always has a writable, non-null buffer
        out.resize(std::max<size_t>(units, 1));
        mjb_buffer buffer = mjb_buffer_make(out.data(), out.size() * sizeof(CharT), encoding);
        const mjb_status status = write(&buffer);

        if(status == MJB_STATUS_OK) {
            out.resize(buffer.size / sizeof(CharT));

            return;
        }

        if(status != MJB_STATUS_OUTPUT_TOO_SMALL || retried) {
            out.clear();
            check_status(status, operation);
        }

        units = (buffer.size + sizeof(CharT) - 1) / sizeof(CharT);
    }
}

template <code_unit CharT, typename Allocator, typename Write>
void write_output(std::basic_string<CharT, std::char_traits<CharT>, Allocator> &out,
    size_t size_hint, const Write &write, const char *operation) {
    write_into(out, encoding_of<CharT>, size_hint, write, operation);
}

template <typename Write>
void write_output(const EncodedOutput &out, size_t size_hint, const Write &write,
    const char *operation) {
    write_into(out.bytes(), out.encoding(), size_hint, write, operation);
}

/**
 * The two forms of every transform. kernel(text, buffer) calls a C _into function:
 *
 *     std::string out = mjb::nfc(input, options);   // the input's string type and encoding
 *     mjb::nfc(input, out, options);                // into an output_target
 */
template <text_input T, typename Kernel>
[[nodiscard]] std::basic_string<input_char_t<T>> run_transform(const T &input, const Kernel &kernel,
    const char *operation) {
    const Text text(input);
    std::basic_string<input_char_t<T>> out;
    write_into(
        out, text.encoding(), text.bytes().size(),
        [&](mjb_buffer *buffer) { return kernel(text.get(), buffer); }, operation);

    return out;
}

template <text_input T, output_target Out, typename Kernel>
void run_transform(const T &input, Out &&out, const Kernel &kernel, const char *operation) {
    const Text text(input);
    write_output(
        out, text.bytes().size(), [&](mjb_buffer *buffer) { return kernel(text.get(), buffer); },
        operation);
}

// A transform with fixed parameters, such as mjb::nfc, as a function object.
template <typename Opts> class Transform {
  public:
    using kernel_type = mjb_status (*)(const mjb_text *, mjb_buffer *, const Opts &);

    constexpr Transform(kernel_type function, const char *failure) noexcept
        : kernel(function), operation(failure) {
    }

    template <text_input T>
    [[nodiscard]] std::basic_string<input_char_t<T>> operator()(const T &input,
        const Opts &options = {}) const {
        return detail::run_transform(input, bind(options), operation);
    }

    template <text_input T, output_target Out>
    void operator()(const T &input, Out &&out, const Opts &options = {}) const {
        detail::run_transform(input, out, bind(options), operation);
    }

  private:
    kernel_type kernel;
    const char *operation;

    [[nodiscard]] auto bind(const Opts &options) const noexcept {
        return [function = kernel, &options](const mjb_text *text, mjb_buffer *buffer) {
            return function(text, buffer, options);
        };
    }
};

template <mjb_normalization Form>
mjb_status normalize_kernel(const mjb_text *text, mjb_buffer *buffer, const Options &options) {
    return mjb_normalize_into(text, options.malformed, Form, buffer, options.diagnostic);
}

template <mjb_map_case_type Type>
mjb_status case_kernel(const mjb_text *text, mjb_buffer *buffer, const CaseOptions &options) {
    return mjb_map_case_into(text, options.malformed, Type, options.locale, buffer,
        options.diagnostic);
}

inline auto normalizer(NormalizationForm form, const Options &options) noexcept {
    return [form, &options](const mjb_text *text, mjb_buffer *buffer) {
        return mjb_normalize_into(text, options.malformed, static_cast<mjb_normalization>(form),
            buffer, options.diagnostic);
    };
}

inline auto case_mapper(mjb_map_case_type type, const CaseOptions &options) noexcept {
    return [type, &options](const mjb_text *text, mjb_buffer *buffer) {
        return mjb_map_case_into(text, options.malformed, type, options.locale, buffer,
            options.diagnostic);
    };
}

inline auto filterer(Filter filters, const Options &options) noexcept {
    return [filters, &options](const mjb_text *text, mjb_buffer *buffer) {
        return mjb_filter_into(text, options.malformed, static_cast<mjb_filter_flags>(filters),
            buffer, options.diagnostic);
    };
}

inline mjb_status nfkc_casefold_kernel(const mjb_text *text, mjb_buffer *buffer,
    const Options &options) {
    return mjb_nfkc_casefold_into(text, options.malformed, buffer, options.diagnostic);
}

inline mjb_status convert_kernel(const mjb_text *text, mjb_buffer *buffer, const Options &options) {
    return mjb_convert_encoding_into(text, options.malformed, buffer, options.diagnostic);
}

// The skeleton always stops at malformed input, so it has no options.
struct NoOptions {};

inline mjb_status skeleton_kernel(const mjb_text *text, mjb_buffer *buffer, const NoOptions &) {
    return mjb_confusable_skeleton_into(text, buffer);
}

inline mjb_status collation_key_kernel(const mjb_text *text, mjb_buffer *buffer,
    const CollationOptions &options) {
    return mjb_collation_key_into(text, options.malformed,
        static_cast<mjb_collation_variable_weighting>(options.variable_weighting),
        static_cast<mjb_collation_strength>(options.strength), buffer, options.diagnostic);
}

} // namespace detail

// Transforms

inline constexpr detail::Transform<Options> nfc{ &detail::normalize_kernel<MJB_NORMALIZATION_NFC>,
    "Normalization failed" };
inline constexpr detail::Transform<Options> nfd{ &detail::normalize_kernel<MJB_NORMALIZATION_NFD>,
    "Normalization failed" };
inline constexpr detail::Transform<Options> nfkc{ &detail::normalize_kernel<MJB_NORMALIZATION_NFKC>,
    "Normalization failed" };
inline constexpr detail::Transform<Options> nfkd{ &detail::normalize_kernel<MJB_NORMALIZATION_NFKD>,
    "Normalization failed" };
inline constexpr detail::Transform<Options> nfkc_casefold{ &detail::nfkc_casefold_kernel,
    "NFKC case folding failed" };

inline constexpr detail::Transform<CaseOptions> uppercase{ &detail::case_kernel<MJB_CASE_UPPER>,
    "Case mapping failed" };
inline constexpr detail::Transform<CaseOptions> lowercase{ &detail::case_kernel<MJB_CASE_LOWER>,
    "Case mapping failed" };
inline constexpr detail::Transform<CaseOptions> titlecase{ &detail::case_kernel<MJB_CASE_TITLE>,
    "Case mapping failed" };
inline constexpr detail::Transform<CaseOptions> casefold{ &detail::case_kernel<MJB_CASE_CASEFOLD>,
    "Case mapping failed" };
inline constexpr detail::Transform<CaseOptions> casefold_simple{
    &detail::case_kernel<MJB_CASE_CASEFOLD_SIMPLE>, "Case mapping failed"
};

// Map the text to the skeleton of UTS #39 confusable detection.
inline constexpr detail::Transform<detail::NoOptions> confusable_skeleton{ &detail::skeleton_kernel,
    "Confusable skeleton generation failed" };

// Write the binary sort key, compared in memcmp order, into key. It is bytes whatever the input's
// encoding. A key needs a few bytes per codepoint for each level, so the first attempt offers
// about 6 bytes per input byte.
inline void collation_key(Text input, std::string &key, const CollationOptions &options = {}) {
    detail::write_into(
        key, MJB_ENC_UTF_8, input.bytes().size() * 4,
        [&](mjb_buffer *buffer) {
            return detail::collation_key_kernel(input.get(), buffer, options);
        },
        "Collation key generation failed");
}

[[nodiscard]] inline std::string collation_key(Text input, const CollationOptions &options = {}) {
    std::string key;
    collation_key(input, key, options);

    return key;
}

/**
 * Convert to the encoding of the output: mjb::convert_encoding(input, out16) writes UTF-16 into a
 * std::u16string, and mjb::convert_encoding(input, mjb::encoded(bytes, MJB_ENC_UTF_16BE)) writes
 * UTF-16BE bytes.
 */
template <text_input T, output_target Out>
void convert_encoding(const T &input, Out &&out, const Options &options = {}) {
    detail::run_transform(
        input, out,
        [&options](const mjb_text *text, mjb_buffer *buffer) {
            return detail::convert_kernel(text, buffer, options);
        },
        "Encoding conversion failed");
}

// A new string of CharT in the platform's byte order: mjb::convert_encoding<char16_t>("abc").
template <code_unit CharT>
[[nodiscard]] std::basic_string<CharT> convert_encoding(Text input, const Options &options = {}) {
    std::basic_string<CharT> out;
    convert_encoding(input, out, options);

    return out;
}

// New bytes in any encoding.
[[nodiscard]] inline std::string convert_encoding(Text input, mjb_encoding encoding,
    const Options &options = {}) {
    std::string out;
    convert_encoding(input, encoded(out, encoding), options);

    return out;
}

template <text_input T>
[[nodiscard]] std::basic_string<detail::input_char_t<T>> normalize(const T &input,
    NormalizationForm form, const Options &options = {}) {
    return detail::run_transform(input, detail::normalizer(form, options), "Normalization failed");
}

template <text_input T, output_target Out>
void normalize(const T &input, NormalizationForm form, Out &&out, const Options &options = {}) {
    detail::run_transform(input, out, detail::normalizer(form, options), "Normalization failed");
}

template <text_input T>
[[nodiscard]] std::basic_string<detail::input_char_t<T>> case_map(const T &input,
    mjb_map_case_type type, const CaseOptions &options = {}) {
    return detail::run_transform(input, detail::case_mapper(type, options), "Case mapping failed");
}

template <text_input T, output_target Out>
void case_map(const T &input, mjb_map_case_type type, Out &&out, const CaseOptions &options = {}) {
    detail::run_transform(input, out, detail::case_mapper(type, options), "Case mapping failed");
}

template <text_input T>
[[nodiscard]] std::basic_string<detail::input_char_t<T>> filter(const T &input, Filter filters,
    const Options &options = {}) {
    return detail::run_transform(input, detail::filterer(filters, options),
        "String filtering failed");
}

template <text_input T, output_target Out>
void filter(const T &input, Filter filters, Out &&out, const Options &options = {}) {
    detail::run_transform(input, out, detail::filterer(filters, options),
        "String filtering failed");
}

// Internationalized domain names

// The result of IDNA processing: the domain name and its mjb_idna_error flags.
template <code_unit CharT = char> struct IdnaResult {
    std::basic_string<CharT> text;
    uint32_t errors = MJB_IDNA_ERROR_NONE;

    [[nodiscard]] bool valid() const noexcept {
        return errors == MJB_IDNA_ERROR_NONE;
    }

    [[nodiscard]] bool has_error(mjb_idna_error error) const noexcept {
        return (errors & static_cast<uint32_t>(error)) != 0;
    }
};

namespace detail {

using idna_function = mjb_status (*)(const mjb_text *, mjb_idna_info *, mjb_buffer *);

template <text_input T>
[[nodiscard]] IdnaResult<input_char_t<T>> idna(const T &input, idna_function function,
    const char *operation) {
    const Text text(input);
    mjb_idna_info info{};
    IdnaResult<input_char_t<T>> result;
    write_into(
        result.text, text.encoding(), text.bytes().size(),
        [&](mjb_buffer *buffer) { return function(text.get(), &info, buffer); }, operation);
    result.errors = info.errors;

    return result;
}

template <output_target Out>
[[nodiscard]] uint32_t idna_into(Text text, Out &&out, idna_function function,
    const char *operation) {
    mjb_idna_info info{};
    write_output(
        out, text.bytes().size(),
        [&](mjb_buffer *buffer) { return function(text.get(), &info, buffer); }, operation);

    return info.errors;
}

} // namespace detail

template <text_input T> [[nodiscard]] auto idna_to_ascii(const T &input) {
    return detail::idna(input, mjb_idna_to_ascii_into, "IDNA ToASCII failed");
}

template <text_input T> [[nodiscard]] auto idna_to_unicode(const T &input) {
    return detail::idna(input, mjb_idna_to_unicode_into, "IDNA ToUnicode failed");
}

// Write the ASCII form of a domain name into out and return its mjb_idna_error flags.
template <text_input T, output_target Out>
[[nodiscard]] uint32_t idna_to_ascii(const T &input, Out &&out) {
    return detail::idna_into(input, out, mjb_idna_to_ascii_into, "IDNA ToASCII failed");
}

// Write the Unicode form of a domain name into out and return its mjb_idna_error flags.
template <text_input T, output_target Out>
[[nodiscard]] uint32_t idna_to_unicode(const T &input, Out &&out) {
    return detail::idna_into(input, out, mjb_idna_to_unicode_into, "IDNA ToUnicode failed");
}

// Analysis

[[nodiscard]] inline mjb_encoding detect_encoding(std::string_view bytes) noexcept {
    return mjb_detect_encoding(bytes.data(), bytes.size());
}

[[nodiscard]] inline bool is_ascii(std::string_view bytes) noexcept {
    return mjb_is_ascii(bytes.data(), bytes.size());
}

[[nodiscard]] inline bool is_utf8(std::string_view bytes) noexcept {
    return mjb_is_utf8(bytes.data(), bytes.size());
}

[[nodiscard]] inline bool is_utf16(std::string_view bytes) noexcept {
    return mjb_is_utf16(bytes.data(), bytes.size());
}

[[nodiscard]] inline mjb_status validate_encoding(Text input,
    mjb_diagnostic *diagnostic = nullptr) noexcept {
    return mjb_validate_encoding(input.get(), diagnostic);
}

// Decode the codepoint at offset, a byte offset, and advance offset past it.
[[nodiscard]] inline mjb_status decode_next(Text input, size_t &offset, mjb_codepoint &codepoint,
    const Options &options = {}) noexcept {
    return mjb_decode_next(input.get(), options.malformed, &offset, &codepoint, options.diagnostic);
}

// Decode the codepoint that ends at offset, a byte offset, and move offset to its start.
[[nodiscard]] inline mjb_status decode_previous(Text input, size_t &offset,
    mjb_codepoint &codepoint, const Options &options = {}) noexcept {
    return mjb_decode_previous(input.get(), options.malformed, &offset, &codepoint,
        options.diagnostic);
}

[[nodiscard]] inline size_t codepoint_count(Text input, const Options &options = {}) {
    size_t count = 0;
    detail::check_status(mjb_codepoint_count(input.get(), options.malformed, &count,
                             options.diagnostic),
        "Codepoint count failed");

    return count;
}

[[nodiscard]] inline size_t grapheme_count(Text input, const Options &options = {}) {
    size_t count = 0;
    detail::check_status(mjb_grapheme_count(input.get(), options.malformed, &count,
                             options.diagnostic),
        "Grapheme count failed");

    return count;
}

[[nodiscard]] inline size_t word_count(Text input, const Options &options = {}) {
    size_t count = 0;
    detail::check_status(mjb_word_count(input.get(), options.malformed, &count, options.diagnostic),
        "Word count failed");

    return count;
}

[[nodiscard]] inline size_t sentence_count(Text input, const Options &options = {}) {
    size_t count = 0;
    detail::check_status(mjb_sentence_count(input.get(), options.malformed, &count,
                             options.diagnostic),
        "Sentence count failed");

    return count;
}

[[nodiscard]] inline size_t terminal_width(Text input, mjb_terminal_width_profile profile,
    const Options &options = {}) {
    size_t width = 0;
    detail::check_status(mjb_terminal_width(input.get(), options.malformed, profile, &width,
                             options.diagnostic),
        "Terminal width calculation failed");

    return width;
}

[[nodiscard]] inline size_t terminal_width(Text input, const Options &options = {}) {
    return terminal_width(input, MJB_TERMINAL_WIDTH_NARROW, options);
}

[[nodiscard]] inline mjb_quick_check_result normalization_quick_check(Text input,
    NormalizationForm form) {
    mjb_quick_check_result quick_check = MJB_QC_NO;
    detail::check_status(mjb_normalization_quick_check(input.get(),
                             static_cast<mjb_normalization>(form), &quick_check),
        "Normalization quick-check failed");

    return quick_check;
}

[[nodiscard]] inline bool is_identifier(Text input,
    mjb_identifier_profile profile = MJB_IDENTIFIER_DEFAULT) noexcept {
    return mjb_is_identifier(input.get(), profile);
}

[[nodiscard]] inline bool confusable_match(Text first, Text second,
    const MatchOptions &options = {}) {
    bool confusable = false;
    detail::check_status(mjb_confusable_match(first.get(), second.get(), options.malformed,
                             &confusable, options.diagnostic, options.other_diagnostic),
        "Confusable comparison failed");

    return confusable;
}

enum class CaselessMode {
    Canonical = MJB_CASELESS_CANONICAL,
    Unnormalized = MJB_CASELESS_UNNORMALIZED,
    Compatibility = MJB_CASELESS_COMPATIBILITY,
    Identifier = MJB_CASELESS_IDENTIFIER
};

[[nodiscard]] inline bool caseless_match(Text first, Text second, CaselessMode mode,
    const MatchOptions &options = {}) {
    bool matches = false;
    detail::check_status(mjb_caseless_match(first.get(), second.get(), options.malformed,
                             static_cast<mjb_caseless_mode>(mode), &matches, options.diagnostic,
                             options.other_diagnostic),
        "Caseless matching failed");

    return matches;
}

[[nodiscard]] inline bool caseless_match(Text first, Text second,
    const MatchOptions &options = {}) {
    return caseless_match(first, second, CaselessMode::Canonical, options);
}

// Compare two texts with the DUCET: negative, zero, or positive, like std::strcmp.
[[nodiscard]] inline int collation_compare(Text first, Text second,
    const CollationOptions &options = {}) {
    int order = 0;
    detail::check_status(mjb_collation_compare(first.get(), second.get(), options.malformed,
                             static_cast<mjb_collation_variable_weighting>(options
                                     .variable_weighting),
                             static_cast<mjb_collation_strength>(options.strength), &order,
                             options.diagnostic, options.other_diagnostic),
        "Collation comparison failed");

    return order;
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

    if(count > 0) {
        result.scripts.resize(count);
        detail::check_status(mjb_resolved_script_set(input.get(), result.scripts.data(), &count,
                                 &result.kind),
            "Unable to read resolved scripts");
        result.scripts.resize(count);
    }

    return result;
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
template <typename Fn>
    requires std::is_invocable_r_v<bool, Fn &, const mjb_character &, mjb_character_position>
[[nodiscard]] mjb_status for_each_codepoint(Text input, Fn &&fn, const Options &options = {}) {
    using Callable = std::remove_reference_t<Fn>;
    detail::CodepointVisitor<Callable> visitor{ &fn, nullptr };
    const mjb_status status = mjb_for_each_codepoint(input.get(), options.malformed,
        detail::visit_codepoint<Callable>, &visitor, options.diagnostic);

    if(visitor.error) {
        std::rethrow_exception(visitor.error);
    }

    return status;
}

// Prefixes: the returned view points into the input and is counted in its code units

template <borrowed_text T>
[[nodiscard]] std::basic_string_view<detail::input_char_t<T>>
grapheme_truncate(T &&input MJB_LIFETIMEBOUND, size_t max_graphemes) noexcept {
    const Text text(input);
    const auto units = detail::view_of(input);

    return units.substr(0,
        mjb_grapheme_prefix_length(text.get(), max_graphemes) / sizeof(detail::input_char_t<T>));
}

template <borrowed_text T>
[[nodiscard]] std::basic_string_view<detail::input_char_t<T>>
grapheme_truncate_width(T &&input MJB_LIFETIMEBOUND, size_t max_columns,
    mjb_terminal_width_profile profile = MJB_TERMINAL_WIDTH_NARROW) noexcept {
    const Text text(input);
    const auto units = detail::view_of(input);

    return units.substr(0,
        mjb_grapheme_prefix_length_for_width(text.get(), profile, max_columns) /
            sizeof(detail::input_char_t<T>));
}

template <borrowed_text T>
[[nodiscard]] std::basic_string_view<detail::input_char_t<T>>
word_truncate(T &&input MJB_LIFETIMEBOUND, size_t max_segments) noexcept {
    const Text text(input);
    const auto units = detail::view_of(input);

    return units.substr(0,
        mjb_word_prefix_length(text.get(), max_segments) / sizeof(detail::input_char_t<T>));
}

template <borrowed_text T>
[[nodiscard]] std::basic_string_view<detail::input_char_t<T>>
word_truncate_width(T &&input MJB_LIFETIMEBOUND, size_t max_columns,
    mjb_terminal_width_profile profile = MJB_TERMINAL_WIDTH_NARROW) noexcept {
    const Text text(input);
    const auto units = detail::view_of(input);

    return units.substr(0,
        mjb_word_prefix_length_for_width(text.get(), profile, max_columns) /
            sizeof(detail::input_char_t<T>));
}

// Segmentation

struct BreakResult {
    // Byte offset of the reported position: the start of `codepoint`, or the text length at the
    // end. Positions start after the first codepoint; there is none for the start of the text.
    size_t position = 0;
    // The codepoint that starts at `position`: U+FFFD for a malformed subsequence, and
    // MJB_CODEPOINT_NOT_VALID at the end of the text.
    mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;
    mjb_break_type type = MJB_BT_NOT_SET;

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

    friend bool operator==(const BreakResult &, const BreakResult &) = default;
};

template <typename State> using break_function = mjb_break_type (*)(const mjb_text *, State *);

/**
 * Every position of a text and its break type, as reported by a C break iterator. To iterate the
 * segments themselves, use graphemes(), words(), sentences(), or line_segments() instead. Like
 * all ranges here, it is a view: the input must outlive it.
 */
template <typename State, break_function<State> BreakFn>
class BreakRange : public std::ranges::view_interface<BreakRange<State, BreakFn>> {
    Text text;

  public:
    class iterator {
        Text text;
        State state{};
        BreakResult current;
        bool done = true;

        // Decode at the reported position rather than reading the state's current codepoint,
        // which the iterators leave unchanged when a rule absorbs a character (SB5, WB4).
        [[nodiscard]] mjb_codepoint codepoint_at(size_t position) const noexcept {
            size_t offset = position;
            mjb_codepoint codepoint = MJB_CODEPOINT_NOT_VALID;

            if(mjb_decode_next(text.get(), MJB_MALFORMED_REPLACE, &offset, &codepoint, nullptr) !=
                MJB_STATUS_OK) {
                return MJB_CODEPOINT_NOT_VALID;
            }

            return codepoint;
        }

        void advance() noexcept {
            const mjb_break_type type = BreakFn(text.get(), &state);

            if(type == MJB_BT_NOT_SET) {
                done = true;

                return;
            }

            current = BreakResult{ state.position, codepoint_at(state.position), type };
        }

      public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = BreakResult;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        explicit iterator(Text input) noexcept : text(input), done(false) {
            advance();
        }

        [[nodiscard]] const BreakResult &operator*() const noexcept {
            return current;
        }

        [[nodiscard]] const BreakResult *operator->() const noexcept {
            return &current;
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

        [[nodiscard]] friend bool operator==(const iterator &left, const iterator &right) noexcept {
            return left.done == right.done && (left.done || left.current == right.current);
        }

        [[nodiscard]] friend bool operator==(const iterator &it, std::default_sentinel_t) noexcept {
            return it.done;
        }
    };

    BreakRange() = default;

    explicit BreakRange(Text input) noexcept : text(input) {
    }

    [[nodiscard]] iterator begin() const noexcept {
        return iterator(text);
    }

    [[nodiscard]] std::default_sentinel_t end() const noexcept {
        return std::default_sentinel;
    }
};

using GraphemeBreaks = BreakRange<mjb_grapheme_break_state, mjb_grapheme_break_next>;
using WordBreaks = BreakRange<mjb_word_break_state, mjb_word_break_next>;
using SentenceBreaks = BreakRange<mjb_sentence_break_state, mjb_sentence_break_next>;
using LineBreaks = BreakRange<mjb_line_break_state, mjb_line_break_next>;

template <borrowed_text T> [[nodiscard]] GraphemeBreaks grapheme_breaks(T &&input) noexcept {
    return GraphemeBreaks(Text(input));
}

template <borrowed_text T> [[nodiscard]] WordBreaks word_breaks(T &&input) noexcept {
    return WordBreaks(Text(input));
}

template <borrowed_text T> [[nodiscard]] SentenceBreaks sentence_breaks(T &&input) noexcept {
    return SentenceBreaks(Text(input));
}

// The line break opportunities of UAX #14.
template <borrowed_text T> [[nodiscard]] LineBreaks line_breaks(T &&input) noexcept {
    return LineBreaks(Text(input));
}

/**
 * The segments of a text, as views of the input counted in its code units. A malformed
 * subsequence counts as one codepoint, and an incomplete subsequence at the end forms the last
 * segment.
 */
template <code_unit CharT, typename State, break_function<State> BreakFn>
class SegmentRange : public std::ranges::view_interface<SegmentRange<CharT, State, BreakFn>> {
    std::basic_string_view<CharT> units;
    Text text;

  public:
    class iterator {
        std::basic_string_view<CharT> units;
        Text text;
        State state{};
        size_t start = 0;
        size_t end = 0;
        bool done = true;

        void advance() noexcept {
            start = end;
            const size_t length = text.bytes().size();
            mjb_break_type type;

            while((type = BreakFn(text.get(), &state)) != MJB_BT_NOT_SET) {
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
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = std::basic_string_view<CharT>;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        iterator(std::basic_string_view<CharT> input_units, Text input) noexcept
            : units(input_units), text(input), done(false) {
            advance();
        }

        [[nodiscard]] value_type operator*() const noexcept {
            return units.substr(start / sizeof(CharT), (end - start) / sizeof(CharT));
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

        [[nodiscard]] friend bool operator==(const iterator &left, const iterator &right) noexcept {
            return left.done == right.done && (left.done || left.end == right.end);
        }

        [[nodiscard]] friend bool operator==(const iterator &it, std::default_sentinel_t) noexcept {
            return it.done;
        }
    };

    SegmentRange() = default;

    SegmentRange(std::basic_string_view<CharT> input_units, Text input) noexcept
        : units(input_units), text(input) {
    }

    [[nodiscard]] iterator begin() const noexcept {
        return iterator(units, text);
    }

    [[nodiscard]] std::default_sentinel_t end() const noexcept {
        return std::default_sentinel;
    }
};

template <code_unit CharT>
using GraphemeRange = SegmentRange<CharT, mjb_grapheme_break_state, mjb_grapheme_break_next>;
template <code_unit CharT>
using WordRange = SegmentRange<CharT, mjb_word_break_state, mjb_word_break_next>;
template <code_unit CharT>
using SentenceRange = SegmentRange<CharT, mjb_sentence_break_state, mjb_sentence_break_next>;
template <code_unit CharT>
using LineSegmentRange = SegmentRange<CharT, mjb_line_break_state, mjb_line_break_next>;

// The extended grapheme clusters (user-perceived characters).
template <borrowed_text T>
[[nodiscard]] GraphemeRange<detail::input_char_t<T>> graphemes(T &&input) noexcept {
    return { detail::view_of(input), Text(input) };
}

// The word segments of UAX #29, including the spaces and punctuation between words.
template <borrowed_text T>
[[nodiscard]] WordRange<detail::input_char_t<T>> words(T &&input) noexcept {
    return { detail::view_of(input), Text(input) };
}

// The sentences of UAX #29.
template <borrowed_text T>
[[nodiscard]] SentenceRange<detail::input_char_t<T>> sentences(T &&input) noexcept {
    return { detail::view_of(input), Text(input) };
}

// The line segments of UAX #14: each one ends at a line break opportunity and keeps its trailing
// spaces.
template <borrowed_text T>
[[nodiscard]] LineSegmentRange<detail::input_char_t<T>> line_segments(T &&input) noexcept {
    return { detail::view_of(input), Text(input) };
}

/**
 * The codepoints of a text, decoded on the fly. A malformed subsequence follows `malformed`: the
 * default replaces it with U+FFFD, because the range has no other way to report it;
 * MJB_MALFORMED_STOP ends the iteration and MJB_MALFORMED_SKIP drops it.
 */
class CodepointRange : public std::ranges::view_interface<CodepointRange> {
    Text text;
    mjb_malformed_policy policy = MJB_MALFORMED_REPLACE;

  public:
    class iterator {
        Text text;
        mjb_malformed_policy policy = MJB_MALFORMED_REPLACE;
        size_t offset = 0;
        char32_t current = 0;
        bool done = true;

        void advance() noexcept {
            mjb_codepoint codepoint = 0;

            if(mjb_decode_next(text.get(), policy, &offset, &codepoint, nullptr) != MJB_STATUS_OK) {
                done = true;

                return;
            }

            current = static_cast<char32_t>(codepoint);
        }

      public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = char32_t;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        iterator(Text input, mjb_malformed_policy malformed) noexcept
            : text(input), policy(malformed), done(false) {
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

        [[nodiscard]] friend bool operator==(const iterator &left, const iterator &right) noexcept {
            return left.done == right.done && (left.done || left.offset == right.offset);
        }

        [[nodiscard]] friend bool operator==(const iterator &it, std::default_sentinel_t) noexcept {
            return it.done;
        }
    };

    CodepointRange() = default;

    CodepointRange(Text input, mjb_malformed_policy malformed) noexcept
        : text(input), policy(malformed) {
    }

    [[nodiscard]] iterator begin() const noexcept {
        return iterator(text, policy);
    }

    [[nodiscard]] std::default_sentinel_t end() const noexcept {
        return std::default_sentinel;
    }
};

template <borrowed_text T>
[[nodiscard]] CodepointRange codepoints(T &&input,
    mjb_malformed_policy malformed = MJB_MALFORMED_REPLACE) noexcept {
    return CodepointRange(Text(input), malformed);
}

// Bidirectional text

/**
 * A paragraph resolved by the Unicode Bidirectional Algorithm (UAX #9). It owns its characters
 * and does not refer to the input once constructed.
 */
class BidiParagraph {
    mjb_bidi_paragraph data{};

  public:
    explicit BidiParagraph(Text input, mjb_direction direction = MJB_DIRECTION_AUTO) {
        detail::check_status(mjb_bidi_resolve(input.get(), direction, &data),
            "Bidirectional paragraph resolution failed");
    }

    BidiParagraph(const BidiParagraph &) = delete;
    BidiParagraph &operator=(const BidiParagraph &) = delete;

    BidiParagraph(BidiParagraph &&other) noexcept : data(std::exchange(other.data, {})) {
    }

    BidiParagraph &operator=(BidiParagraph &&other) noexcept {
        if(this != &other) {
            mjb_bidi_paragraph_free(&data);
            data = std::exchange(other.data, {});
        }

        return *this;
    }

    ~BidiParagraph() {
        mjb_bidi_paragraph_free(&data);
    }

    [[nodiscard]] std::span<const mjb_bidi_char> chars() const noexcept {
        return data.chars == nullptr ? std::span<const mjb_bidi_char>{} :
                                       std::span<const mjb_bidi_char>(data.chars, data.count);
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

    [[nodiscard]] auto begin() const noexcept {
        return chars().begin();
    }

    [[nodiscard]] auto end() const noexcept {
        return chars().end();
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

    // The characters of [line_start, line_end) in visual order, as indices into chars().
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

    [[nodiscard]] std::vector<mjb_bidi_run> line_runs(std::span<const size_t> visual_order) const {
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

// Characters and properties

/**
 * The properties of a codepoint as read by mjb_codepoint_info. Codepoint predicates such as
 * mjb::is_emoji are free functions, which do not need this lookup.
 */
class Character {
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

    [[nodiscard]] friend bool operator==(const Character &left, const Character &right) noexcept {
        return left.data.codepoint == right.data.codepoint;
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

    [[nodiscard]] const mjb_character &raw() const noexcept {
        return data;
    }
};

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
    scripts.resize(count);

    return scripts;
}

[[nodiscard]] inline std::string_view script_code(mjb_script script) noexcept {
    const char *code = mjb_script_code(script);

    return code == nullptr ? std::string_view{} : std::string_view(code);
}

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

// The bytes of a codepoint in an encoding, UTF-8 by default.
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

// Locales

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
    detail::check_status(mjb_locale_parse(id.get(), &locale.data), "Locale parsing failed");

    return locale;
}

[[nodiscard]] inline std::string_view locale_code(mjb_locale locale) noexcept {
    const char *code = mjb_locale_code(locale);

    return code == nullptr ? std::string_view{} : std::string_view(code);
}

// Library

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

} // namespace mjb

// The ranges hold copies of their input views, so their iterators stay valid after the range
// itself is destroyed.
namespace std::ranges {

template <typename State, mjb::break_function<State> BreakFn>
inline constexpr bool enable_borrowed_range<mjb::BreakRange<State, BreakFn>> = true;

template <mjb::code_unit CharT, typename State, mjb::break_function<State> BreakFn>
inline constexpr bool enable_borrowed_range<mjb::SegmentRange<CharT, State, BreakFn>> = true;

template <> inline constexpr bool enable_borrowed_range<mjb::CodepointRange> = true;

} // namespace std::ranges

#endif // MJB_CPP_MOJIBAKE_HPP
