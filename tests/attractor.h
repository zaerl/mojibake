/**
 * The Attractor Unit Test library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 *
 * Single-header library. In exactly one source file define ATT_IMPLEMENTATION
 * before including this header:
 *
 * #define ATT_IMPLEMENTATION
 * #include "attractor.h"
 *
 * Every other file just includes the header. Usage:
 *
 * int var_to_test = 1;
 * int expected_value = 1;
 *
 * ATT_ASSERT(var_to_test, expected_value, "one == one");
 */

#ifndef ATTRACTOR_H
#define ATTRACTOR_H

#include <stdbool.h>
#include <stddef.h>

#define ATT_VERSION_NUMBER 0x110 // MAJOR << 8 | MINOR << 4 | REVISION
#define ATT_VERSION_MAJOR 1
#define ATT_VERSION_MINOR 1
#define ATT_VERSION_REVISION 0

#define ATT_VERSION "1.1.0"

#ifdef __cplusplus
// MSVC reports __cplusplus as 199711L unless /Zc:__cplusplus is passed, while _MSVC_LANG
// always holds the selected standard. Use whichever is newer.
#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
    #define MJB_CPLUSPLUS _MSVC_LANG
#else
    #define MJB_CPLUSPLUS __cplusplus
#endif

#if MJB_CPLUSPLUS < 201703L
    #error "attractor.h requires C++17 or later"
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

extern "C" {
#else
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
        #error "attractor.h requires C11 or later"
    #endif
#endif

#ifndef ATT_API
    #define ATT_API
#endif

#ifndef ATT_MAX_TESTS
    #define ATT_MAX_TESTS 512
#endif

// Verbosity: 0 = only failures, 1 = plus the test case names, 2 = plus a dot per assertion,
// 3 = a line per assertion, 4 = like 3 plus a "Valid/run: X/Y" summary after each test case.
#ifndef ATT_VERBOSE
    #define ATT_VERBOSE 2
#endif

#ifndef ATT_SHOW_ERROR
    #define ATT_SHOW_ERROR 1
#endif

// Emit TAP (Test Anything Protocol) output.
#ifndef ATT_TAP
    #define ATT_TAP 0
#endif

// Tolerance for float, double and long double comparisons.
#ifndef ATT_FLOAT_EPSILON
    #define ATT_FLOAT_EPSILON 0
#endif

#ifndef ATT_STRING_AS_POINTERS
    #define ATT_STRING_AS_POINTERS 0
#endif

#ifndef ATT_CUSTOM_TYPES
    #define ATT_CUSTOM_TYPES
#endif

// Comparison operator applied by an assertion.
typedef enum att_op {
    ATT_OP_EQ,
    ATT_OP_NE,
    ATT_OP_LT,
    ATT_OP_LE,
    ATT_OP_GT,
    ATT_OP_GE
} att_op;

// clang-format off
#ifndef __cplusplus
// For an integer VALUE the assertion function also depends on the type of EXPECTED, so that the
// expected value is never silently converted: a floating point EXPECTED compares as floating
// point, an EXPECTED of the opposite signedness compares the two values mathematically.
#define ATT_GENERIC_SIGNED(EXPECTED, FN) _Generic((EXPECTED), \
    float: att_assert_lf, double: att_assert_lf, long double: att_assert_Lf, \
    unsigned short: att_assert_lld_llu, unsigned int: att_assert_lld_llu, \
    unsigned long: att_assert_lld_llu, unsigned long long: att_assert_lld_llu, \
    default: FN)
#define ATT_GENERIC_UNSIGNED(EXPECTED, FN) _Generic((EXPECTED), \
    float: att_assert_lf, double: att_assert_lf, long double: att_assert_Lf, \
    short: att_assert_llu_lld, int: att_assert_llu_lld, \
    long: att_assert_llu_lld, long long: att_assert_llu_lld, \
    default: FN)

// This macro is automatically generated. Do not edit.
#define ATT_ASSERT_OP(VALUE, EXPECTED, OP, MESSAGE) \
    (att_set_assert_context(#VALUE, __FILE__, __LINE__), _Generic(VALUE, \
    ATT_CUSTOM_TYPES \
    char: att_assert_c, \
    unsigned char: att_assert_u_c, \
    char *: att_assert_p_c, \
    const char *: att_assert_cp_c, \
    short: ATT_GENERIC_SIGNED(EXPECTED, att_assert_hd), \
    unsigned short: ATT_GENERIC_UNSIGNED(EXPECTED, att_assert_u_hu), \
    int: ATT_GENERIC_SIGNED(EXPECTED, att_assert_d), \
    unsigned int: ATT_GENERIC_UNSIGNED(EXPECTED, att_assert_u_u), \
    long: ATT_GENERIC_SIGNED(EXPECTED, att_assert_ld), \
    unsigned long: ATT_GENERIC_UNSIGNED(EXPECTED, att_assert_u_lu), \
    long long: ATT_GENERIC_SIGNED(EXPECTED, att_assert_lld), \
    unsigned long long: ATT_GENERIC_UNSIGNED(EXPECTED, att_assert_u_llu), \
    float: att_assert_f, \
    double: att_assert_lf, \
    long double: att_assert_Lf, \
    void *: att_assert_p_p, \
    const void *: att_assert_cp_p, \
    _Bool: att_assert_b, \
    default: att_assert_unknown \
)(VALUE, EXPECTED, OP, MESSAGE, __FILE__, __LINE__))
#else
#define ATT_ASSERT_OP(VALUE, EXPECTED, OP, MESSAGE) \
    (att_set_assert_context(#VALUE, __FILE__, __LINE__), \
     att_assert_cpp(VALUE, EXPECTED, OP, MESSAGE, __FILE__, __LINE__))
#endif

#define ATT_ASSERT(VALUE, EXPECTED, MESSAGE) ATT_ASSERT_OP(VALUE, EXPECTED, ATT_OP_EQ, MESSAGE)
#define ATT_ASSERT_NE(VALUE, EXPECTED, MESSAGE) ATT_ASSERT_OP(VALUE, EXPECTED, ATT_OP_NE, MESSAGE)
#define ATT_ASSERT_LT(VALUE, EXPECTED, MESSAGE) ATT_ASSERT_OP(VALUE, EXPECTED, ATT_OP_LT, MESSAGE)
#define ATT_ASSERT_LE(VALUE, EXPECTED, MESSAGE) ATT_ASSERT_OP(VALUE, EXPECTED, ATT_OP_LE, MESSAGE)
#define ATT_ASSERT_GT(VALUE, EXPECTED, MESSAGE) ATT_ASSERT_OP(VALUE, EXPECTED, ATT_OP_GT, MESSAGE)
#define ATT_ASSERT_GE(VALUE, EXPECTED, MESSAGE) ATT_ASSERT_OP(VALUE, EXPECTED, ATT_OP_GE, MESSAGE)

// This functions list is automatically generated. Do not edit.
ATT_API unsigned int att_assert_c(char result, char expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_u_c(unsigned char result, unsigned char expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_p_c(const char *result, const char *expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_cp_c(const char *result, const char *expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_hd(short result, short expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_u_hu(unsigned short result, unsigned short expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_d(int result, int expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_u_u(unsigned int result, unsigned int expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_ld(long result, long expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_u_lu(unsigned long result, unsigned long expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_lld(long long result, long long expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_u_llu(unsigned long long result, unsigned long long expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_f(float result, float expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_lf(double result, double expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_Lf(long double result, long double expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_p_p(void *result, void *expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_cp_p(const void *result, const void *expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_b(bool result, bool expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_unknown(const void *result, const void *expected, att_op op, const char *description, const char *file, unsigned int line);

ATT_API unsigned int att_get_valid_tests(void);
ATT_API unsigned int att_get_total_tests(void);

// Integer comparisons between a signed and an unsigned value. The values are compared
// mathematically: a negative number is smaller than every unsigned one.
ATT_API unsigned int att_assert_lld_llu(long long result, unsigned long long expected, att_op op, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_llu_lld(unsigned long long result, long long expected, att_op op, const char *description, const char *file, unsigned int line);

// Assert that a condition is true. Works in both C and C++.
#define ATT_ASSERT_TRUE(VALUE, MESSAGE) \
    (att_set_assert_context(#VALUE, __FILE__, __LINE__), \
     att_assert_true((VALUE) ? 1 : 0, MESSAGE, __FILE__, __LINE__))

// Assert that two memory regions of SIZE bytes are equal. Two NULL pointers are
// considered equal; a NULL and a non-NULL pointer are not.
#define ATT_ASSERT_MEM(VALUE, EXPECTED, SIZE, MESSAGE) \
    (att_set_assert_context(#VALUE, __FILE__, __LINE__), \
     att_assert_mem(VALUE, EXPECTED, SIZE, MESSAGE, __FILE__, __LINE__))

ATT_API unsigned int att_assert_true(int result, const char *description, const char *file, unsigned int line);
ATT_API unsigned int att_assert_mem(const void *result, const void *expected, size_t size, const char *description, const char *file, unsigned int line);

// Base function every assertion goes through.
ATT_API int att_assert(const char *format, int test, const char *description);

// Print a summary of the tests run ("Tests valid/run: X/Y") and return 0 if all tests passed.
ATT_API int att_report(void);

ATT_API void att_set_verbose(unsigned int verbose);
ATT_API void att_set_show_error(unsigned int show_error);
ATT_API void att_set_tap(unsigned int tap);

// Tolerance used for float, double and long double comparisons.
ATT_API long double att_get_float_epsilon(void);
ATT_API void att_set_float_epsilon(long double epsilon);

// The same tolerance as a double, for callers that have no long double type.
ATT_API double att_get_double_epsilon(void);
ATT_API void att_set_double_epsilon(double epsilon);

// A callback that receives everything the library would print to stdout. TEXT is a fragment of
// output, not necessarily a whole line. Pass NULL to print to stdout again.
typedef void (*att_output_callback)(const char *text, void *data);

ATT_API void att_set_output_callback(att_output_callback callback, void *data);

// A callback to be used when the default comparison fails.
typedef int (*att_generic_callback)(void *result, void *expected, att_op op,
    const char *description);

ATT_API void att_set_generic_callback(att_generic_callback callback);

// A callback to be used when an test occurs.
typedef int (*att_test_callback)(int test, const char *description, const char *expression,
    const char *file, unsigned int line);

ATT_API void att_set_test_callback(att_test_callback callback);

// Stores the expression for the next assertion. Used internally by ATT_ASSERT.
ATT_API void att_set_assert_context(const char *expression, const char *file, unsigned int line);

// Named test cases. ATT_TEST(name) defines a test case function and registers it
// automatically before main() runs:
//
// ATT_TEST(parser_empty) {
//     ATT_ASSERT(parse(""), NULL, "empty input yields NULL");
// }
//
// int main(void) {
//     att_run_tests(NULL);
//     return att_report();
// }
typedef void (*att_test_fn)(void);

// A test case function that receives the pointer it was registered with.
typedef void (*att_test_data_fn)(void *data);

// Register a test case. NAME is not copied and must stay valid for the whole run.
ATT_API void att_register_test(const char *name, att_test_fn fn);

// Register a test case that is called with DATA, e.g. a closure from another language.
ATT_API void att_register_test_data(const char *name, att_test_data_fn fn, void *data);

// Number of registered test cases.
ATT_API unsigned int att_get_test_count(void);

// Name of the registered test case at index.
ATT_API const char *att_get_test_name(unsigned int index);

// Run the registered test cases whose name contains FILTER (all of them when NULL). Returns the
// number of failed cases, or -1 when a filter matches no test case, which also counts as a
// failed assertion.
ATT_API int att_run_tests(const char *filter);

// A callback att_run_tests calls before running each test case.
typedef void (*att_test_start_callback)(const char *name);

ATT_API void att_set_test_start_callback(att_test_start_callback callback);

#if defined(__cplusplus)
// C++: a static object initializer runs before main on every compiler.
#define ATT_TEST(NAME) \
    static void att_test_fn_##NAME(void); \
    [[maybe_unused]] static const int att_test_reg_##NAME = \
        (att_register_test(#NAME, att_test_fn_##NAME), 0); \
    static void att_test_fn_##NAME(void)
#elif defined(_MSC_VER)
// MSVC trick.
#pragma section(".CRT$XCU", read)
#if defined(_WIN64)
#define ATT_TEST_SYM_PREFIX ""
#else
#define ATT_TEST_SYM_PREFIX "_"
#endif
#define ATT_TEST(NAME) \
    static void att_test_fn_##NAME(void); \
    static void __cdecl att_test_reg_##NAME(void) { \
        att_register_test(#NAME, att_test_fn_##NAME); \
    } \
    __pragma(comment(linker, "/include:" ATT_TEST_SYM_PREFIX "att_test_reg_ptr_" #NAME)) \
    __declspec(allocate(".CRT$XCU")) void(__cdecl *att_test_reg_ptr_##NAME)(void) = \
        att_test_reg_##NAME; \
    static void att_test_fn_##NAME(void)
#elif defined(__GNUC__)
// The family.
#define ATT_TEST(NAME) \
    static void att_test_fn_##NAME(void); \
    __attribute__((constructor)) static void att_test_reg_##NAME(void) { \
        att_register_test(#NAME, att_test_fn_##NAME); \
    } \
    static void att_test_fn_##NAME(void)
#else
// No constructor support.
#define ATT_TEST_MANUAL 1
#define ATT_TEST(NAME) static void att_test_fn_##NAME(void)
#endif

#ifdef __cplusplus
}
#endif

// clang-format on

#ifdef __cplusplus
// Helper to check if common_type exists
template <typename T1, typename T2, typename = void> struct att_has_common_type : std::false_type {};

template <typename T1, typename T2>
struct att_has_common_type<T1, T2, std::void_t<typename std::common_type<T1, T2>::type>>
    : std::true_type {};

// Integer types wider than a char, excluding bool.
template <typename T>
inline constexpr bool att_is_wide_integer =
    std::is_integral_v<T> && !std::is_same_v<T, bool> && sizeof(T) > 1;

// C++ template function for att_assert to handle type deduction properly
template <typename T1, typename T2>
inline unsigned int att_assert_cpp(T1 result, T2 expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    if constexpr(att_is_wide_integer<T1> && att_is_wide_integer<T2> &&
                 std::is_signed_v<T1> != std::is_signed_v<T2>) {
        // The common type of a signed and an unsigned integer is unsigned, which would turn a
        // negative value into a huge one. Compare the two values mathematically instead.
        if constexpr(std::is_signed_v<T1>) {
            return att_assert_lld_llu(result, expected, op, description, file, line);
        } else {
            return att_assert_llu_lld(result, expected, op, description, file, line);
        }
    } else if constexpr(att_has_common_type<T1, T2>::value) {
        // Convert both to the common type for comparison
        // Convert both to a common type for comparison
        using common_type = std::common_type_t<T1, T2>;
        common_type converted_result = static_cast<common_type>(result);
        common_type converted_expected = static_cast<common_type>(expected);

        // Handle different common types
        if constexpr(std::is_same_v<common_type, char>) {
            return att_assert_c(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, unsigned char>) {
            return att_assert_u_c(converted_result, converted_expected, op, description, file,
                line);
        } else if constexpr(std::is_same_v<common_type, short>) {
            return att_assert_hd(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, unsigned short>) {
            return att_assert_u_hu(converted_result, converted_expected, op, description, file,
                line);
        } else if constexpr(std::is_same_v<common_type, int>) {
            return att_assert_d(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, unsigned int>) {
            return att_assert_u_u(converted_result, converted_expected, op, description, file,
                line);
        } else if constexpr(std::is_same_v<common_type, long>) {
            return att_assert_ld(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, unsigned long>) {
            return att_assert_u_lu(converted_result, converted_expected, op, description, file,
                line);
        } else if constexpr(std::is_same_v<common_type, long long>) {
            return att_assert_lld(converted_result, converted_expected, op, description, file,
                line);
        } else if constexpr(std::is_same_v<common_type, unsigned long long>) {
            return att_assert_u_llu(converted_result, converted_expected, op, description, file,
                line);
        } else if constexpr(std::is_same_v<common_type, float>) {
            return att_assert_f(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, double>) {
            return att_assert_lf(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, long double>) {
            return att_assert_Lf(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, bool>) {
            return att_assert_b(converted_result, converted_expected, op, description, file, line);
        } else if constexpr(std::is_same_v<common_type, std::string>) {
            return att_assert_cp_c(converted_result.c_str(), converted_expected.c_str(), op,
                description, file, line);
        } else {
            // Unsupported type with common_type - compile error
            static_assert(sizeof(T1) == 0,
                "ATT_ASSERT: Unsupported type. Supported types are: char, short, int, long, long "
                "long (signed/unsigned), float, double, long double, bool, std::string, and "
                "pointer types.");
            return 0;
        }
    } else {
        // No common type - compile error
        static_assert(sizeof(T1) == 0,
            "ATT_ASSERT: Cannot compare incompatible types with no common type.");
        return 0;
    }
}

// Overloads for pointer types (can't use template easily for these)
inline unsigned int att_assert_cpp(char *result, char *expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_p_c(result, expected, op, description, file, line);
}

inline unsigned int att_assert_cpp(const char *result, const char *expected, att_op op,
    const char *description, const char *file, unsigned int line) {
    return att_assert_cp_c(result, expected, op, description, file, line);
}

inline unsigned int att_assert_cpp(void *result, void *expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_p_p(result, expected, op, description, file, line);
}

// Convert the int form of NULL through a pointer-sized integer. On LLP64 platforms (64-bit
// Windows), long is narrower than a pointer.
template <typename T> inline T *att_pointer_from_long(long value) {
    return value == 0 ? nullptr : reinterpret_cast<T *>(static_cast<std::intptr_t>(value));
}

// Special overloads for legacy NULL pointer comparisons
inline unsigned int att_assert_cpp(const char *result, long expected, att_op op,
    const char *description, const char *file, unsigned int line) {
    return att_assert_cp_c(result, att_pointer_from_long<const char>(expected), op, description,
        file, line);
}

inline unsigned int att_assert_cpp(char *result, long expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_p_c(result, att_pointer_from_long<char>(expected), op, description, file,
        line);
}

inline unsigned int att_assert_cpp(void *result, long expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_p_p(result, att_pointer_from_long<void>(expected), op, description, file,
        line);
}

// Modern C++ null pointer comparisons
inline unsigned int att_assert_cpp(const char *result, std::nullptr_t, att_op op,
    const char *description, const char *file, unsigned int line) {
    return att_assert_cp_c(result, nullptr, op, description, file, line);
}

inline unsigned int att_assert_cpp(char *result, std::nullptr_t, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_p_c(result, nullptr, op, description, file, line);
}

inline unsigned int att_assert_cpp(void *result, std::nullptr_t, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_p_p(result, nullptr, op, description, file, line);
}

// Mixed char pointer types (char* vs const char*)
inline unsigned int att_assert_cpp(char *result, const char *expected, att_op op,
    const char *description, const char *file, unsigned int line) {
    return att_assert_cp_c(result, expected, op, description, file, line);
}

inline unsigned int att_assert_cpp(const char *result, char *expected, att_op op,
    const char *description, const char *file, unsigned int line) {
    return att_assert_cp_c(result, expected, op, description, file, line);
}

template <typename T1, typename T2>
inline unsigned int att_assert_cpp(T1 *result, T2 *expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_unknown((const void *)result, (const void *)expected, op, description, file,
        line);
}

template <typename T>
inline unsigned int att_assert_cpp(T *result, std::nullptr_t, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_unknown((const void *)result, nullptr, op, description, file, line);
}

template <typename T>
inline unsigned int att_assert_cpp(T *result, long expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_unknown((const void *)result, att_pointer_from_long<void>(expected), op,
        description, file, line);
}

// NULL is the plain integer 0 on some compilers (MSVC) instead of a long constant.
template <typename T>
inline unsigned int att_assert_cpp(T *result, int expected, att_op op, const char *description,
    const char *file, unsigned int line) {
    return att_assert_cpp(result, static_cast<long>(expected), op, description, file, line);
}
#endif

#endif /* ATTRACTOR_H */

#ifdef ATT_IMPLEMENTATION
#ifndef ATT_IMPLEMENTATION_INCLUDED
#define ATT_IMPLEMENTATION_INCLUDED

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #ifndef STDOUT_FILENO
        #define STDOUT_FILENO _fileno(stdout)
    #endif
#else
    #include <unistd.h>
#endif

// ATT_FETCH_INCREMENT atomically increments and returns the previous value.
#ifdef __cplusplus
    #include <atomic>
    typedef std::atomic<unsigned int> att_atomic_uint;
    #define ATT_FETCH_INCREMENT(value) ((value)++)
#elif defined(_MSC_VER)
    // MSVC ships no C11 <stdatomic.h> when compiling as C (error C1189 without the
    // /experimental:c11atomics switch), so use the Interlocked intrinsics instead.
    typedef volatile LONG att_atomic_uint;
    #define ATT_FETCH_INCREMENT(value) ((unsigned int)InterlockedIncrement(&(value)) - 1)
#else
    #include <stdatomic.h>
    typedef atomic_uint att_atomic_uint;
    #define ATT_FETCH_INCREMENT(value) ((value)++)
#endif
// clang-format on

#if defined(__GNUC__)
    #define ATT_PRINTF_FORMAT __attribute__((format(printf, 1, 2)))
#else
    #define ATT_PRINTF_FORMAT
#endif

// Where the output goes: the callback if one is set, stdout otherwise.
static att_output_callback att_output_callback_fn = NULL;
static void *att_output_callback_data = NULL;

void att_set_output_callback(att_output_callback callback, void *data) {
    att_output_callback_fn = callback;
    att_output_callback_data = data;
}

static void att_puts(const char *text) {
    if(att_output_callback_fn) {
        att_output_callback_fn(text, att_output_callback_data);
    } else {
        fputs(text, stdout);
    }
}

static void att_printf(const char *format, ...) ATT_PRINTF_FORMAT;

static void att_printf(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_list args_copy;
    int length;

    va_start(args, format);

    if(!att_output_callback_fn) {
        vprintf(format, args);
        va_end(args);

        return;
    }

    va_copy(args_copy, args);
    length = vsnprintf(buffer, sizeof(buffer), format, args_copy);
    va_end(args_copy);

    if(length >= 0 && (size_t)length < sizeof(buffer)) {
        att_output_callback_fn(buffer, att_output_callback_data);
    } else if(length >= 0) {
        // The text does not fit the stack buffer.
        char *heap = (char *)malloc((size_t)length + 1);

        if(heap) {
            vsnprintf(heap, (size_t)length + 1, format, args);
            att_output_callback_fn(heap, att_output_callback_data);
            free(heap);
        }
    }

    va_end(args);
}

static void att_flush(void) {
    if(!att_output_callback_fn) {
        fflush(stdout);
    }
}

#define ATT_ERROR_MESSAGE(RESULT, FORMAT_1, FORMAT_2, EXPECTED, OP) \
    do { \
        if(att_show_error) { \
            if(att_tap) { \
                att_puts("# "); \
            } \
            att_printf(att_show_colors ? "\x1B[90m%s:%u:\x1B[0m " : "%s:%u: ", file, line); \
            if(att_verbose < 3 && !att_tap) { \
                att_printf("%s: ", description); \
            } \
            att_puts("Expected "); \
            if((OP) != ATT_OP_EQ) { \
                att_printf("%s ", att_op_symbols[OP]); \
            } \
            if(att_show_colors) { \
                att_puts("\x1B[32m"); \
            } \
            att_printf(FORMAT_1, EXPECTED); \
            att_puts(att_show_colors ? "\x1B[0m, got \x1B[31m" : ", got "); \
            att_printf(FORMAT_2, RESULT); \
            att_puts(att_show_colors ? "\x1B[0m\n" : "\n"); \
            if(!att_tap) { \
                att_puts("\n"); \
            } \
        } \
        att_flush(); \
    } while(0)

// Evaluate the comparison selected by OP. Arguments are evaluated more than once.
#define ATT_COMPARE(RESULT, EXPECTED, OP) \
    ((OP) == ATT_OP_EQ        ? (RESULT) == (EXPECTED) : \
            (OP) == ATT_OP_NE ? (RESULT) != (EXPECTED) : \
            (OP) == ATT_OP_LT ? (RESULT) < (EXPECTED) : \
            (OP) == ATT_OP_LE ? (RESULT) <= (EXPECTED) : \
            (OP) == ATT_OP_GT ? (RESULT) > (EXPECTED) : \
                                (RESULT) >= (EXPECTED))

// Indexed by att_op.
static const char *att_op_symbols[] = { "==", "!=", "<", "<=", ">", ">=" };

// The generic callback takes non-const pointers. Going through an integer drops the qualifier
// without a cast that discards const.
#define ATT_UNCONST(POINTER) ((void *)(uintptr_t)(POINTER))

// Printable form of a character: the character itself, or \xNN outside printable ASCII.
// BUFFER must hold at least 8 bytes.
static const char *att_char_text(unsigned char value, char *buffer) {
    if(value < 32 || value >= 127) {
        snprintf(buffer, 8, "\\x%02X", (unsigned int)value);
    } else {
        buffer[0] = (char)value;
        buffer[1] = '\0';
    }

    return buffer;
}

static att_atomic_uint att_valid_tests = 0;
static att_atomic_uint att_total_tests = 0;
static unsigned int att_verbose = ATT_VERBOSE;
static unsigned int att_show_error = ATT_SHOW_ERROR;
static unsigned int att_tap = ATT_TAP;
static int att_show_colors = 0;
// Whether stdout currently ends with an unterminated line of verbose = 1 dots.
static int att_dots_pending = 0;
static long double att_float_epsilon = (long double)(ATT_FLOAT_EPSILON);

// A callback to be used when the default comparison fails.
static att_generic_callback att_generic_callback_fn = NULL;

// A callback to be used when an test occurs.
static att_test_callback att_test_callback_fn = NULL;

// Variables stored by each assertion to provide context for the next assertion.
static const char *att_assert_expression = "";
static const char *att_assert_file = "";
static unsigned int att_assert_line = 0;

unsigned int att_get_valid_tests(void) {
    return (unsigned int)att_valid_tests;
}

unsigned int att_get_total_tests(void) {
    return (unsigned int)att_total_tests;
}

void att_set_verbose(unsigned int verbose) {
    att_verbose = verbose;
}

void att_set_show_error(unsigned int show_error) {
    att_show_error = show_error;
}

void att_set_tap(unsigned int tap) {
    att_tap = tap;
}

long double att_get_float_epsilon(void) {
    return att_float_epsilon;
}

void att_set_float_epsilon(long double epsilon) {
    att_float_epsilon = epsilon;
}

double att_get_double_epsilon(void) {
    return (double)att_float_epsilon;
}

void att_set_double_epsilon(double epsilon) {
    att_float_epsilon = (long double)epsilon;
}

void att_set_generic_callback(att_generic_callback callback) {
    att_generic_callback_fn = callback;
}

void att_set_test_callback(att_test_callback callback) {
    att_test_callback_fn = callback;
}

void att_set_assert_context(const char *expression, const char *file, unsigned int line) {
    att_assert_expression = expression ? expression : "";
    att_assert_file = file ? file : "";
    att_assert_line = line;
}

static int att_console_ready = 0;

// Detect color support (and, on Windows, enable ANSI escape codes).
static void att_console_init(void) {
    if(att_console_ready) {
        return;
    }

    att_console_ready = 1;

    if(isatty(STDOUT_FILENO)) {
        const char *no_color = getenv("NO_COLOR");

#ifdef _WIN32
        // On Windows, the TERM env is usually not set, so enable colors by default if NO_COLOR
        // is not set
        att_show_colors = no_color == NULL;
#else
        // On Unix, check TERM environment variable
        const char *term = getenv("TERM");
        att_show_colors = no_color == NULL && term != NULL && strcmp(term, "dumb") != 0;
#endif
    }

#ifdef _WIN32
    if(att_show_colors) {
        // On Windows, we need to enable ANSI escape codes for stdout and stderr
        HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE h_err = GetStdHandle(STD_ERROR_HANDLE);
        DWORD mode_out, mode_err;

        // Enable ANSI escape codes for stdout
        if(h_out != INVALID_HANDLE_VALUE && GetConsoleMode(h_out, &mode_out)) {
            mode_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

            if(!SetConsoleMode(h_out, mode_out)) {
                // If we can't enable ANSI codes, disable colors
                att_show_colors = 0;
            }
        } else {
            // If we can't get console mode, disable colors
            att_show_colors = 0;
        }

        // Enable ANSI escape codes for stderr
        if(att_show_colors && h_err != INVALID_HANDLE_VALUE && GetConsoleMode(h_err, &mode_err)) {
            mode_err |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(h_err, mode_err);
        }
    }
#endif
}

// Print a TAP description: a '#' would start a directive, so it is escaped, and line breaks
// would end the test line, so they become spaces.
static void att_put_tap_description(const char *description) {
    while(*description) {
        size_t length = strcspn(description, "#\r\n");

        if(length > 0) {
            att_printf("%.*s", (int)length, description);
            description += length;
        }

        if(*description) {
            att_puts(*description == '#' ? "\\#" : " ");
            ++description;
        }
    }
}

// Base function for assertions. Returns 1 if the test passed, 0 if it failed.
int att_assert(const char *format, int test, const char *description) {
    unsigned int index = ATT_FETCH_INCREMENT(att_total_tests);

    // Initialize the library on the first assertion. The fetch-increment returns the
    // previous value atomically, so exactly one thread observes zero here.
    if(index == 0) {
        att_console_init();
    }

    if(test) {
        ATT_FETCH_INCREMENT(att_valid_tests);
    }

    if(att_test_callback_fn) {
        att_test_callback_fn(test, description, att_assert_expression, att_assert_file,
            att_assert_line);
    }

    if(att_tap) {
        att_printf(test ? "ok %u - " : "not ok %u - ", index + 1);
        att_put_tap_description(description);
        att_puts("\n");
    } else if(att_verbose < 2) {
        // Do nothing
    } else if(att_verbose == 2) {
        att_puts(test ? "." : (att_show_colors ? "\x1B[31mF\x1B[0m" : "F"));
        att_dots_pending = test != 0;

        if(!test) {
            att_puts("\n");
        }
    } else {
        const char *ok = att_show_colors ? "\x1B[32mOK\x1B[0m" : "OK";
        const char *fail = att_show_colors ? "\x1B[31mNO\x1B[0m" : "NO";

        att_printf(att_show_colors ? "%s [\x1b[36m%s\x1b[0m] %s\n" : "%s [%s] %s\n", test ? ok : fail,
            format, description);
    }

    return test;
}

// Returns 0 and prints an error message for unsupported operators.
static unsigned int att_assert_op_unsupported(att_op op, const char *description, const char *file,
    unsigned int line) {
    att_assert("unsupported", 0, description);

    if(att_show_error) {
        if(att_tap) {
            att_puts("# ");
        }

        att_printf(att_show_colors ? "\x1B[90m%s:%u:\x1B[0m " : "%s:%u: ", file, line);

        if(att_verbose < 3 && !att_tap) {
            att_printf("%s: ", description);
        }

        att_printf("Operator %s is not supported for this type\n%s", att_op_symbols[op],
            att_tap ? "" : "\n");
    }

    att_flush();

    return 0;
}

// Custom assertion functions for ATT_ASSERT_TRUE.
ATT_API unsigned int att_assert_true(int result, const char *description, const char *file,
    unsigned int line) {
    int test = att_assert("true", result != 0, description);

    if(!test) {
        ATT_ERROR_MESSAGE("false", "%s", "%s", "true", ATT_OP_EQ);
    }

    return (unsigned int)test;
}

// Signed result against unsigned expected. A negative result is below every unsigned value.
ATT_API unsigned int att_assert_lld_llu(long long result, unsigned long long expected, att_op op,
    const char *description, const char *file, unsigned int line) {
    int order = result < 0 || (unsigned long long)result < expected ? -1 :
        (unsigned long long)result > expected;
    int test = att_assert("long long", ATT_COMPARE(order, 0, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%llu", "%lld", expected, op);
    }

    return (unsigned int)test;
}

// Unsigned result against signed expected. A negative expected is below every unsigned value.
ATT_API unsigned int att_assert_llu_lld(unsigned long long result, long long expected, att_op op,
    const char *description, const char *file, unsigned int line) {
    int order = expected < 0 || result > (unsigned long long)expected ? 1 :
        -(result < (unsigned long long)expected);
    int test = att_assert("unsigned long long", ATT_COMPARE(order, 0, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lld", "%llu", expected, op);
    }

    return (unsigned int)test;
}

// Custom assertion functions for ATT_ASSERT_MEM.
ATT_API unsigned int att_assert_mem(const void *result, const void *expected, size_t size,
    const char *description, const char *file, unsigned int line) {
    int test = att_assert("memory",
        (result == expected) || (result && expected && memcmp(result, expected, size) == 0),
        description);

    if(!test) {
        if(!result || !expected) {
            ATT_ERROR_MESSAGE(result, "%p", "%p", expected, ATT_OP_EQ);
        } else if(att_show_error) {
            const unsigned char *res = (const unsigned char *)result;
            const unsigned char *exp = (const unsigned char *)expected;
            size_t offset = 0;

            while(offset < size && res[offset] == exp[offset]) {
                ++offset;
            }

            if(att_tap) {
                att_puts("# ");
            }

            att_printf(att_show_colors ? "\x1B[90m%s:%u:\x1B[0m " : "%s:%u: ", file, line);

            if(att_verbose < 3 && !att_tap) {
                att_printf("%s: ", description);
            }

            att_printf(att_show_colors ?
                    "Byte %zu: expected \x1B[32m0x%02X\x1B[0m, got \x1B[31m0x%02X\x1B[0m\n%s" :
                    "Byte %zu: expected 0x%02X, got 0x%02X\n%s",
                offset, exp[offset], res[offset], att_tap ? "" : "\n");
        }

        att_flush();
    }

    return (unsigned int)test;
}

int att_report(void) {
    unsigned int total = (unsigned int)att_total_tests;
    unsigned int valid = (unsigned int)att_valid_tests;
    int all_valid = valid == total;

    if(att_tap) {
        // A trailing plan is valid TAP and avoids knowing the test count upfront.
        att_printf("1..%u\n# Tests valid/run: %u/%u\n", total, valid, total);

        return all_valid ? 0 : 1;
    }

    // Close the line of verbose = 1 dots if it is still open.
    if(att_dots_pending) {
        att_puts("\n");
        att_dots_pending = 0;
    }

    att_printf(att_show_colors ? (all_valid ? "Tests valid/run: \x1B[32m%u/%u\x1B[0m\n" :
        "Tests valid/run: \x1B[31m%u/%u\x1B[0m\n") : "Tests valid/run: %u/%u\n", valid, total);

    return all_valid ? 0 : 1;
}

typedef struct att_test_case {
    const char *name;
    att_test_fn fn;
    att_test_data_fn data_fn;
    void *data;
    // The att_run_tests call in which the case last failed, 0 if it never did.
    unsigned int failed_run;
} att_test_case;

static att_test_case att_tests[ATT_MAX_TESTS];
static att_atomic_uint att_registered_tests = 0;
// Counts the att_run_tests calls, so that each one recaps only its own failures.
static unsigned int att_run_count = 0;

// A callback to be called before each test case run by att_run_tests.
static att_test_start_callback att_test_start_callback_fn = NULL;

void att_set_test_start_callback(att_test_start_callback callback) {
    att_test_start_callback_fn = callback;
}

static void att_register_test_case(const char *name, att_test_fn fn, att_test_data_fn data_fn,
    void *data) {
    unsigned int index = ATT_FETCH_INCREMENT(att_registered_tests);

    if(index < ATT_MAX_TESTS) {
        att_tests[index].name = name ? name : "";
        att_tests[index].fn = fn;
        att_tests[index].data_fn = data_fn;
        att_tests[index].data = data;
        att_tests[index].failed_run = 0;
    }
}

void att_register_test(const char *name, att_test_fn fn) {
    att_register_test_case(name, fn, NULL, NULL);
}

void att_register_test_data(const char *name, att_test_data_fn fn, void *data) {
    att_register_test_case(name, NULL, fn, data);
}

unsigned int att_get_test_count(void) {
    return (unsigned int)att_registered_tests;
}

const char *att_get_test_name(unsigned int index) {
    unsigned int registered = (unsigned int)att_registered_tests;
    unsigned int count = registered < ATT_MAX_TESTS ? registered : ATT_MAX_TESTS;

    return index < count ? att_tests[index].name : NULL;
}

static int att_test_matches(const char *name, const char *filter) {
    return filter == NULL || strstr(name, filter) != NULL;
}

int att_run_tests(const char *filter) {
    unsigned int registered = (unsigned int)att_registered_tests;
    unsigned int count = registered < ATT_MAX_TESTS ? registered : ATT_MAX_TESTS;
    unsigned int run = 0;
    unsigned int failed = 0;
    unsigned int run_id = ++att_run_count;
    unsigned int i;

    att_console_init();

    if(registered > count) {
        att_printf("%swarning: %u test cases dropped, define ATT_MAX_TESTS above %u\n",
            att_tap ? "# " : "", registered - count, (unsigned int)ATT_MAX_TESTS);
    }

    for(i = 0; i < count; ++i) {
        unsigned int total_before;
        unsigned int valid_before;
        unsigned int total_delta;
        unsigned int valid_delta;
        int case_failed;

        if(!att_test_matches(att_tests[i].name, filter) ||
            (!att_tests[i].fn && !att_tests[i].data_fn)) {
            continue;
        }

        if(att_tap) {
            att_printf("# test: %s\n", att_tests[i].name);
        } else if(att_verbose == 1 || att_verbose >= 3) {
            att_printf(att_show_colors ? "Test: \x1b[1;32m%s\x1b[0m\n" : "Test: %s\n", att_tests[i].name);
        } else if(att_verbose == 2) {
            if(att_dots_pending) {
                att_puts("\n");
            }

            // The dots of the case follow the name on the same line.
            att_printf(att_show_colors ? "Test: \x1b[1;32m%s\x1b[0m " : "Test: %s ", att_tests[i].name);
            att_dots_pending = 1;
        }

        total_before = (unsigned int)att_total_tests;
        valid_before = (unsigned int)att_valid_tests;

        // The snapshot precedes the callback, so its assertions count toward this case.
        if(att_test_start_callback_fn) {
            att_test_start_callback_fn(att_tests[i].name);
        }

        if(att_tests[i].fn) {
            att_tests[i].fn();
        } else {
            att_tests[i].data_fn(att_tests[i].data);
        }

        total_delta = (unsigned int)att_total_tests - total_before;
        valid_delta = (unsigned int)att_valid_tests - valid_before;
        case_failed = total_delta != valid_delta;

        if(case_failed) {
            att_tests[i].failed_run = run_id;
        }

        // Close the name + dots line of verbose 2, if a failure did not already.
        if(!att_tap && att_verbose == 2 && att_dots_pending) {
            att_puts("\n");
            att_dots_pending = 0;
        }

        // At verbose 4 each test case ends with its own valid/run summary.
        if(!att_tap && att_verbose >= 4) {
            att_printf(att_show_colors ? (case_failed ? "Valid/run: \x1B[31m%u/%u\x1B[0m\n\n" :
                "Valid/run: \x1B[32m%u/%u\x1B[0m\n\n") : "Valid/run: %u/%u\n\n", valid_delta,
                total_delta);
        }

        failed += (unsigned int)case_failed;
        ++run;
    }

    // Close the line of verbose = 1 dots if it is still open.
    if(att_dots_pending) {
        att_puts("\n");
        att_dots_pending = 0;
    }

    // A filter that selects nothing is most likely a typo: report it as a failed assertion, so
    // that att_report() does not return success for a run that tested nothing.
    if(run == 0 && filter != NULL) {
        att_assert("filter", 0, "A test case matches the filter");

        if(att_show_error) {
            att_printf("%sNo test case matches the filter \"%s\"\n%s", att_tap ? "# " : "", filter,
                att_tap ? "" : "\n");
        }

        att_flush();

        return -1;
    }

    for(i = 0; failed > 0 && i < count; ++i) {
        if(att_tests[i].failed_run == run_id) {
            if(att_tap) {
                att_printf("# failed: %s\n", att_tests[i].name);
            } else {
                att_printf(att_show_colors ? "Failed: \x1B[31m%s\x1B[0m\n" : "Failed: %s\n",
                    att_tests[i].name);
            }
        }
    }

    if(att_tap) {
        att_printf("# Test cases valid/run: %u/%u\n", run - failed, run);
    } else {
        att_printf(att_show_colors ? (failed == 0 ? "Test cases valid/run: \x1B[32m%u/%u\x1B[0m\n" :
            "Test cases valid/run: \x1B[31m%u/%u\x1B[0m\n") : "Test cases valid/run: %u/%u\n",
            run - failed, run);
    }

    att_flush();

    return (int)failed;
}

// These functions are automatically generated. Do not edit.
ATT_API unsigned int att_assert_c(char result, char expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("char", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        char text_1[8];
        char text_2[8];

        ATT_ERROR_MESSAGE(att_char_text((unsigned char)result, text_2), "%s", "%s", att_char_text((unsigned char)expected, text_1), op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_u_c(unsigned char result, unsigned char expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned char", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        char text_1[8];
        char text_2[8];

        ATT_ERROR_MESSAGE(att_char_text((unsigned char)result, text_2), "%s", "%s", att_char_text((unsigned char)expected, text_1), op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_p_c(const char *result, const char *expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test;

    if(result && expected) {
        test = ATT_COMPARE(strcmp(result, expected), 0, op);
    } else {
        test = op == ATT_OP_EQ ? result == expected : (op == ATT_OP_NE ? result != expected : 0);
    }

    test = att_assert("char *", test, description);

    if(!test) {
        if(ATT_STRING_AS_POINTERS == 1) {
            ATT_ERROR_MESSAGE((const void *)result, "%p", "%p", (const void *)expected, op);
        } else {
            // A NULL string is printed as an unquoted NULL instead of being passed to %s.
            ATT_ERROR_MESSAGE(result ? result : "NULL", expected ? "\"%s\"" : "%s", result ? "\"%s\"" : "%s", expected ? expected : "NULL", op);
        }
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_cp_c(const char *result, const char *expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test;

    if(result && expected) {
        test = ATT_COMPARE(strcmp(result, expected), 0, op);
    } else {
        test = op == ATT_OP_EQ ? result == expected : (op == ATT_OP_NE ? result != expected : 0);
    }

    test = att_assert("const char *", test, description);

    if(!test) {
        if(ATT_STRING_AS_POINTERS == 1) {
            ATT_ERROR_MESSAGE((const void *)result, "%p", "%p", (const void *)expected, op);
        } else {
            // A NULL string is printed as an unquoted NULL instead of being passed to %s.
            ATT_ERROR_MESSAGE(result ? result : "NULL", expected ? "\"%s\"" : "%s", result ? "\"%s\"" : "%s", expected ? expected : "NULL", op);
        }
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_hd(short result, short expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("short", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%hd", "%hd", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_u_hu(unsigned short result, unsigned short expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned short", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%hu", "%hu", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_d(int result, int expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("int", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%d", "%d", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_u_u(unsigned int result, unsigned int expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned int", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%u", "%u", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_ld(long result, long expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("long", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%ld", "%ld", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_u_lu(unsigned long result, unsigned long expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned long", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lu", "%lu", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_lld(long long result, long long expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("long long", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%lld", "%lld", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_u_llu(unsigned long long result, unsigned long long expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test = att_assert("unsigned long long", ATT_COMPARE(result, expected, op), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%llu", "%llu", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_f(float result, float expected, att_op op, const char *description, const char *file, unsigned int line) {
    int equal = (result == expected) || ((long double)(result > expected ? result - expected : expected - result) <= att_float_epsilon);
    int test = att_assert("float", op == ATT_OP_EQ ? equal : (op == ATT_OP_NE ? !equal : ATT_COMPARE(result, expected, op)), description);

    if(!test) {
        ATT_ERROR_MESSAGE((double)result, "%.9g", "%.9g", (double)expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_lf(double result, double expected, att_op op, const char *description, const char *file, unsigned int line) {
    int equal = (result == expected) || ((long double)(result > expected ? result - expected : expected - result) <= att_float_epsilon);
    int test = att_assert("double", op == ATT_OP_EQ ? equal : (op == ATT_OP_NE ? !equal : ATT_COMPARE(result, expected, op)), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%.17g", "%.17g", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_Lf(long double result, long double expected, att_op op, const char *description, const char *file, unsigned int line) {
    int equal = (result == expected) || ((long double)(result > expected ? result - expected : expected - result) <= att_float_epsilon);
    int test = att_assert("long double", op == ATT_OP_EQ ? equal : (op == ATT_OP_NE ? !equal : ATT_COMPARE(result, expected, op)), description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%.21Lg", "%.21Lg", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_p_p(void *result, void *expected, att_op op, const char *description, const char *file, unsigned int line) {
    if(op != ATT_OP_EQ && op != ATT_OP_NE) {
        return att_assert_op_unsupported(op, description, file, line);
    }

    int test = att_assert("void *", op == ATT_OP_EQ ? result == expected : result != expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%p", "%p", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_cp_p(const void *result, const void *expected, att_op op, const char *description, const char *file, unsigned int line) {
    if(op != ATT_OP_EQ && op != ATT_OP_NE) {
        return att_assert_op_unsupported(op, description, file, line);
    }

    int test = att_assert("const void *", op == ATT_OP_EQ ? result == expected : result != expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%p", "%p", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_b(bool result, bool expected, att_op op, const char *description, const char *file, unsigned int line) {
    if(op != ATT_OP_EQ && op != ATT_OP_NE) {
        return att_assert_op_unsupported(op, description, file, line);
    }

    int test = att_assert("_Bool", op == ATT_OP_EQ ? result == expected : result != expected, description);

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%d", "%d", expected, op);
    }

    return (unsigned int)test;
}

ATT_API unsigned int att_assert_unknown(const void *result, const void *expected, att_op op, const char *description, const char *file, unsigned int line) {
    int test;

    if(att_generic_callback_fn) {
        test = att_assert("callback", att_generic_callback_fn(ATT_UNCONST(result), ATT_UNCONST(expected), op, description), description);
    } else if(op != ATT_OP_EQ && op != ATT_OP_NE) {
        return att_assert_op_unsupported(op, description, file, line);
    } else {
        test = att_assert("default", op == ATT_OP_EQ ? result == expected : result != expected, description);
    }

    if(!test) {
        ATT_ERROR_MESSAGE(result, "%p", "%p", expected, op);
    }

    return (unsigned int)test;
}
#endif /* ATT_IMPLEMENTATION_INCLUDED */
#endif /* ATT_IMPLEMENTATION */
