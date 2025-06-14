/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../src/mojibake.h"
#include "test.h"

void *test_sort(void *arg) {
    #define SORT_ARRAY(NAME) \
        mjb_sort(NAME, sizeof(NAME) / sizeof(NAME[0]));

    /*int arr[] = { 1 };
    SORT_ARRAY(arr)
    ATT_ASSERT(arr[0], 1, "Sort with one element")

    int arr_2[] = {2, 1};
    SORT_ARRAY(arr_2)
    ATT_ASSERT(arr_2[0], 1, "Sort with two elements")

    int arr_3[] = {2, 2, 1, 3};
    SORT_ARRAY(arr_3)
    ATT_ASSERT(arr_3[0], 1, "Sort with four elements")
    ATT_ASSERT(arr_3[3], 3, "Sort with four elements")*/

    // LATIN CAPITAL LETTER A
    mjb_character arr[1];
    mjb_codepoint_character(&arr[0], 0x0041);

    SORT_ARRAY(arr)
    ATT_ASSERT(arr[0].codepoint, 0x0041, "Sort with one element")

    return NULL;
}
