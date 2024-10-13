#include "mojibake.h"

MJB_EXPORT void mjb_sort(mjb_character array[], size_t size) {
    for(size_t step = 1; step < size; ++step) {
        mjb_character key = array[step];
        int j = step - 1;

        // Move elements of array[0..step-1], that are greater than key,
        // to one position ahead of their current position
        while(j >= 0 && array[j].combining > key.combining) {
            array[j + 1] = array[j];
            j = j - 1;
        }

        array[j + 1] = key;
    }
}
