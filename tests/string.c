/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

 #include <stdio.h>
 #include <string.h>

 #include "test.h"

 void *test_string(void *arg) {
    mjb_encoding encoding = MJB_ENCODING_UTF_8;

     ATT_ASSERT(mjb_strnlen("Hello", 5, encoding), 5, "UTF-8 length: Hello")
     ATT_ASSERT(mjb_strnlen("Hello", 4, encoding), 4, "UTF-8 length: Hello")
     ATT_ASSERT(mjb_strnlen("Hello", 3, encoding), 3, "UTF-8 length: Hello")
     ATT_ASSERT(mjb_strnlen("Hello", 2, encoding), 2, "UTF-8 length: Hello")
     ATT_ASSERT(mjb_strnlen("Hello", 1, encoding), 1, "UTF-8 length: Hello")
     ATT_ASSERT(mjb_strnlen("Hello", 0, encoding), 0, "UTF-8 length: Hello")
     ATT_ASSERT(mjb_strnlen(NULL, 0, encoding), 0, "UTF-8 length: NULL")

     ATT_ASSERT(mjb_strnlen("Héllö", 7, encoding), 5, "UTF-8 length: Héllö")
     ATT_ASSERT(mjb_strnlen("Héllö", 4, encoding), 3, "UTF-8 length: Héllö")
     ATT_ASSERT(mjb_strnlen("Héllö", 2, encoding), 1, "UTF-8 length: Héllö")
     ATT_ASSERT(mjb_strnlen("Héllö", 0, encoding), 0, "UTF-8 length: Héllö")
     ATT_ASSERT(mjb_strnlen("Hèllõ ツ", 11, encoding), 7, "UTF-8 length: Hèllõ ツ")
     ATT_ASSERT(mjb_strnlen("Hèllõ ツ", 5, encoding), 4, "UTF-8 length: Hèllõ ツ")
     ATT_ASSERT(mjb_strnlen("こんにちは", 15, encoding), 5, "UTF-8 length: こんにちは")
     ATT_ASSERT(mjb_strnlen("Γειά σου", 15, encoding), 8, "UTF-8 length: Γειά σου")

     ATT_ASSERT(mjb_strnlen("Héllö", 1, encoding), 1, "UTF-8 length: Héllö (1 max value)")

     return NULL;
 }
