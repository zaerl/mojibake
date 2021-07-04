/**
 * The mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include "../src/array.h"
#include "test.h"

MJB_EXPORT void mjb_array_test() {
    mojibake *mjb = NULL;
    mjb_array array;
    bool res;

    /*mjb_assert("Array null", !mjb_array_init(mjb, NULL, 1, 1));
    mjb_assert("Libray not initialized", !mjb_array_init(mjb, &array, 1, 1));
    mjb_assert("Element size 0", !mjb_array_init(mjb, &array, 0, 1));

    mjb_array_init(mjb, &array, 1, 1);

    mjb_assert("Used 0", array.used == 0);
    mjb_assert("Element size", array.element_size == 1);
    mjb_assert("Size", array.size == 1);

    char el1 = 'A';
    char el2 = 'B';
    char at;

    res = mjb_array_push(mjb, &array, &el1);

    mjb_assert("Array push", res);
    mjb_assert("Array used", array.used == 1);
    mjb_assert("Array at void", !mjb_array_at(mjb, &array, NULL, 0));
    mjb_assert("Array at", mjb_array_at(mjb, &array, &at, 0) && at == el1);

    res = mjb_array_set(mjb, &array, &el2, 0);
    mjb_assert("Array set", res);
    mjb_assert("Array used", array.used == 1);

    res = mjb_array_at(mjb, &array, &at, 0);
    mjb_assert("Array at", mjb_array_at(mjb, &array, &at, 0) && at == el2);
    mjb_assert("Array wrong index at", mjb_array_at(mjb, &array, &at, 1) == false);
    mjb_assert("Array wrong index at", mjb_array_at(mjb, &array, &at, 2) == false);

    res = mjb_array_free(mjb, &array);
    mjb_assert("Array free", res);
    mjb_assert("Array free used", array.used == 0);
    mjb_assert("Array free size", array.size == 0);
    mjb_assert("Array at after free", mjb_array_at(mjb, &array, &at, 0) == false);*/

    mjb_initialize(&mjb);

    /* Grow */
    unsigned int i;
    uint32_t grow_el = 0x100;
    uint32_t *grow_at;
    mjb_assert("1: Array grow init", mjb_array_init(mjb, &array, 4, 1));

    for(i = 0; i < 10; ++i) {
        mjb_assert("\n1: Array grow push", mjb_array_push(mjb, &array, (char*)&grow_el));

        grow_at = (uint32_t*)mjb_array_at(mjb, &array, i);

        mjb_assert("1: Array grow at", grow_at != NULL && *grow_at == grow_el);
        printf("1: [%u -- %u]", grow_el, *grow_at);
        break;
    }
}
