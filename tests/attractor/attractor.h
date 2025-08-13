/**
 * 2025-08-13
 *
 * The Attractor Unit Test library
 *
 * Usage:
 *
 * #include <attractor.h>
 *
 * int var_to_test = 1;
 * int expected_value = 1;
 *
 * ATT_ASSERT(var_to_test, expected_value, "one == one");
 */

#ifndef ATT_TEST_H
#define ATT_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ATT_API
#define ATT_API
#endif

#ifndef ATT_VERBOSE
#define ATT_VERBOSE 1
#endif

#ifndef ATT_SHOW_ERROR
#define ATT_SHOW_ERROR 1
#endif

#ifndef ATT_STRING_AS_POINTERS
#define ATT_STRING_AS_POINTERS 0
#endif

#define ATT_ASSERT(VALUE, EXPECTED, MESSAGE) _Generic(VALUE, \
    char: att_assert_c, \
    unsigned char: att_assert_u_c, \
    char*: att_assert_p_c, \
    const char*: att_assert_cp_c, \
    short: att_assert_hd, \
    unsigned short: att_assert_u_hu, \
    int: att_assert_d, \
    unsigned int: att_assert_u_u, \
    long: att_assert_ld, \
    unsigned long: att_assert_u_lu, \
    long long: att_assert_lld, \
    unsigned long long: att_assert_u_llu, \
    float: att_assert_f, \
    double: att_assert_lf, \
    long double: att_assert_Lf, \
    void*: att_assert_p_p, \
    _Bool: att_assert_b, \
    default: att_assert_unknown \
)(VALUE, EXPECTED, MESSAGE);

ATT_API unsigned int att_assert_c(char result, char expected, const char *description);
ATT_API unsigned int att_assert_u_c(unsigned char result, unsigned char expected, const char *description);
ATT_API unsigned int att_assert_p_c(char* result, char* expected, const char *description);
ATT_API unsigned int att_assert_cp_c(const char* result, const char* expected, const char *description);
ATT_API unsigned int att_assert_hd(short result, short expected, const char *description);
ATT_API unsigned int att_assert_u_hu(unsigned short result, unsigned short expected, const char *description);
ATT_API unsigned int att_assert_d(int result, int expected, const char *description);
ATT_API unsigned int att_assert_u_u(unsigned int result, unsigned int expected, const char *description);
ATT_API unsigned int att_assert_ld(long result, long expected, const char *description);
ATT_API unsigned int att_assert_u_lu(unsigned long result, unsigned long expected, const char *description);
ATT_API unsigned int att_assert_lld(long long result, long long expected, const char *description);
ATT_API unsigned int att_assert_u_llu(unsigned long long result, unsigned long long expected, const char *description);
ATT_API unsigned int att_assert_f(float result, float expected, const char *description);
ATT_API unsigned int att_assert_lf(double result, double expected, const char *description);
ATT_API unsigned int att_assert_Lf(long double result, long double expected, const char *description);
ATT_API unsigned int att_assert_p_p(void* result, void* expected, const char *description);
ATT_API unsigned int att_assert_b(_Bool result, _Bool expected, const char *description);
ATT_API unsigned int att_assert_unknown(void* result, void* expected, const char *description);

unsigned int att_get_valid_tests(void);
unsigned int att_get_total_tests(void);

void att_set_verbose(unsigned int verbose);
void att_set_show_error(unsigned int show_error);

typedef int (*att_generic_callback)(void*, void*, const char*);

// A callback to be used when the default comparison fails.
void att_set_generic_callback(att_generic_callback callback);

typedef int (*att_test_callback)(int, const char*);

// A callback to be used when an test occurs.
void att_set_test_callback(att_test_callback callback);

#ifdef __cplusplus
}
#endif

#endif /* ATT_TEST_H */
