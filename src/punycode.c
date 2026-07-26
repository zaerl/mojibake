/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdint.h>
#include <string.h>

#include "punycode.h"

#define MJB_PUNYCODE_BASE UINT64_C(36)
#define MJB_PUNYCODE_TMIN UINT64_C(1)
#define MJB_PUNYCODE_TMAX UINT64_C(26)
#define MJB_PUNYCODE_SKEW UINT64_C(38)
#define MJB_PUNYCODE_DAMP UINT64_C(700)
#define MJB_PUNYCODE_INITIAL_BIAS UINT64_C(72)
#define MJB_PUNYCODE_INITIAL_N UINT64_C(128)

static char mjb_punycode_encode_digit(uint64_t digit) {
    return digit < 26 ? (char)('a' + digit) : (char)('0' + digit - 26);
}

static bool mjb_punycode_decode_digit(char character, uint64_t *digit) {
    if(character >= 'a' && character <= 'z') {
        *digit = (uint64_t)(character - 'a');
        return true;
    }

    if(character >= 'A' && character <= 'Z') {
        *digit = (uint64_t)(character - 'A');
        return true;
    }

    if(character >= '0' && character <= '9') {
        *digit = (uint64_t)(character - '0' + 26);
        return true;
    }

    return false;
}

static uint64_t mjb_punycode_threshold(uint64_t k, uint64_t bias) {
    if(k <= bias + MJB_PUNYCODE_TMIN) {
        return MJB_PUNYCODE_TMIN;
    }

    if(k >= bias + MJB_PUNYCODE_TMAX) {
        return MJB_PUNYCODE_TMAX;
    }

    return k - bias;
}

static uint64_t mjb_punycode_adapt(uint64_t delta, uint64_t points, bool first_time) {
    delta = first_time ? delta / MJB_PUNYCODE_DAMP : delta / 2;
    delta += delta / points;
    uint64_t k = 0;
    uint64_t limit = ((MJB_PUNYCODE_BASE - MJB_PUNYCODE_TMIN) * MJB_PUNYCODE_TMAX) / 2;

    while(delta > limit) {
        delta /= MJB_PUNYCODE_BASE - MJB_PUNYCODE_TMIN;
        k += MJB_PUNYCODE_BASE;
    }

    return k + ((MJB_PUNYCODE_BASE - MJB_PUNYCODE_TMIN + 1) * delta) / (delta + MJB_PUNYCODE_SKEW);
}

mjb_status mjb_punycode_encode(const mjb_codepoint *codepoints, size_t count, mjb_output *output) {
    if((codepoints == NULL && count > 0) || output == NULL) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    size_t basic_count = 0;

    for(size_t i = 0; i < count; ++i) {
        if(codepoints[i] > MJB_CODEPOINT_MAX ||
            (codepoints[i] >= 0xD800 && codepoints[i] <= 0xDFFF)) {
            return MJB_STATUS_INVALID_CODEPOINT;
        }

        if(codepoints[i] < 0x80) {
            char character = (char)codepoints[i];
            mjb_status status = mjb_output_write(output, &character, 1);

            if(status != MJB_STATUS_OK) {
                return status;
            }

            ++basic_count;
        }
    }

    if(basic_count > 0 && basic_count < count) {
        const char delimiter = '-';
        mjb_status status = mjb_output_write(output, &delimiter, 1);

        if(status != MJB_STATUS_OK) {
            return status;
        }
    }

    uint64_t n = MJB_PUNYCODE_INITIAL_N;
    uint64_t delta = 0;
    uint64_t bias = MJB_PUNYCODE_INITIAL_BIAS;
    size_t handled = basic_count;

    while(handled < count) {
        uint64_t next = UINT64_MAX;

        for(size_t i = 0; i < count; ++i) {
            if(codepoints[i] >= n && codepoints[i] < next) {
                next = codepoints[i];
            }
        }

        if(next == UINT64_MAX || handled == SIZE_MAX) {
            return MJB_STATUS_OVERFLOW;
        }

        uint64_t points = (uint64_t)handled + 1;

        if(next - n > (UINT64_MAX - delta) / points) {
            return MJB_STATUS_OVERFLOW;
        }

        delta += (next - n) * points;
        n = next;

        for(size_t i = 0; i < count; ++i) {
            uint64_t codepoint = codepoints[i];

            if(codepoint < n) {
                if(delta == UINT64_MAX) {
                    return MJB_STATUS_OVERFLOW;
                }

                ++delta;
            } else if(codepoint == n) {
                uint64_t q = delta;

                for(uint64_t k = MJB_PUNYCODE_BASE;; k += MJB_PUNYCODE_BASE) {
                    uint64_t threshold = mjb_punycode_threshold(k, bias);

                    if(q < threshold) {
                        break;
                    }

                    uint64_t digit = threshold + (q - threshold) % (MJB_PUNYCODE_BASE - threshold);
                    char character = mjb_punycode_encode_digit(digit);
                    mjb_status status = mjb_output_write(output, &character, 1);

                    if(status != MJB_STATUS_OK) {
                        return status;
                    }

                    q = (q - threshold) / (MJB_PUNYCODE_BASE - threshold);
                }

                char character = mjb_punycode_encode_digit(q);
                mjb_status status = mjb_output_write(output, &character, 1);

                if(status != MJB_STATUS_OK) {
                    return status;
                }

                bias = mjb_punycode_adapt(delta, points, handled == basic_count);
                delta = 0;
                ++handled;
            }
        }

        if(delta == UINT64_MAX || n == UINT64_MAX) {
            return MJB_STATUS_OVERFLOW;
        }

        ++delta;
        ++n;
    }

    return MJB_STATUS_OK;
}

mjb_status mjb_punycode_decode(const char *buffer, size_t byte_length, mjb_codepoint **codepoints,
    size_t *count) {
    if(codepoints == NULL || count == NULL || (buffer == NULL && byte_length > 0)) {
        return MJB_STATUS_INVALID_ARGUMENT;
    }

    *codepoints = NULL;
    *count = 0;

    if(byte_length == 0) {
        return MJB_STATUS_OK;
    }

    if(byte_length > SIZE_MAX / sizeof(mjb_codepoint)) {
        return MJB_STATUS_OVERFLOW;
    }

    mjb_codepoint *decoded = (mjb_codepoint *)mjb_alloc(byte_length * sizeof(mjb_codepoint));

    if(decoded == NULL) {
        return MJB_STATUS_NO_MEMORY;
    }

    size_t delimiter = SIZE_MAX;

    for(size_t i = 0; i < byte_length; ++i) {
        if((uint8_t)buffer[i] >= 0x80) {
            mjb_free(decoded);
            return MJB_STATUS_MALFORMED_INPUT;
        }

        if(buffer[i] == '-') {
            delimiter = i;
        }
    }

    size_t input_index = 0;

    if(delimiter != SIZE_MAX) {
        for(size_t i = 0; i < delimiter; ++i) {
            decoded[*count] = (uint8_t)buffer[i];
            ++*count;
        }

        input_index = delimiter + 1;
    }

    uint64_t n = MJB_PUNYCODE_INITIAL_N;
    uint64_t index = 0;
    uint64_t bias = MJB_PUNYCODE_INITIAL_BIAS;

    while(input_index < byte_length) {
        uint64_t old_index = index;
        uint64_t weight = 1;

        for(uint64_t k = MJB_PUNYCODE_BASE;; k += MJB_PUNYCODE_BASE) {
            if(input_index >= byte_length) {
                mjb_free(decoded);
                *count = 0;
                return MJB_STATUS_MALFORMED_INPUT;
            }

            uint64_t digit;

            if(!mjb_punycode_decode_digit(buffer[input_index++], &digit) ||
                digit > (UINT64_MAX - index) / weight) {
                mjb_free(decoded);
                *count = 0;
                return MJB_STATUS_MALFORMED_INPUT;
            }

            index += digit * weight;
            uint64_t threshold = mjb_punycode_threshold(k, bias);

            if(digit < threshold) {
                break;
            }

            uint64_t factor = MJB_PUNYCODE_BASE - threshold;

            if(weight > UINT64_MAX / factor) {
                mjb_free(decoded);
                *count = 0;
                return MJB_STATUS_MALFORMED_INPUT;
            }

            weight *= factor;
        }

        uint64_t points = (uint64_t)*count + 1;
        bias = mjb_punycode_adapt(index - old_index, points, old_index == 0);

        if(index / points > UINT64_MAX - n) {
            mjb_free(decoded);
            *count = 0;
            return MJB_STATUS_MALFORMED_INPUT;
        }

        n += index / points;
        index %= points;

        if(n > MJB_CODEPOINT_MAX || (n >= 0xD800 && n <= 0xDFFF)) {
            mjb_free(decoded);
            *count = 0;
            return MJB_STATUS_MALFORMED_INPUT;
        }

        size_t insertion = (size_t)index;
        memmove(&decoded[insertion + 1], &decoded[insertion],
            (*count - insertion) * sizeof(mjb_codepoint));
        decoded[insertion] = (mjb_codepoint)n;
        ++*count;
        ++index;
    }

    *codepoints = decoded;

    return MJB_STATUS_OK;
}
