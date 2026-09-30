
#ifndef BITMASK_UTIL_H
#define BITMASK_UTIL_H 1

#include <stdint.h>

/*
* Caskey, Damon V
* 2026-06-01
*
* Get a bitmask with a 1 in the position of the 
* index and 0s elsewhere. Used for array scans
* to avoid empty slots when we have a sparse array 
* of indices.
*/

static inline uint16_t bitmask16_from_index(unsigned int index) {
    if (index >= 16) {
        return 0;
    }

    return (uint16_t)(UINT16_C(1) << index);
}

static inline uint32_t bitmask32_from_index(unsigned int index) {
    if (index >= 32) {
        return 0;
    }

    return (uint32_t)(UINT32_C(1) << index);
}

static inline uint64_t bitmask64_from_index(unsigned int index) {
    if (index >= 64) {
        return 0;
    }

    return (uint64_t)(UINT64_C(1) << index);
}

/*
* Caskey, Damon V
* 2026-06-01
*
* Get the index of the lowest set bit in a bitmask.
* Returns bit width when no bits are set.
*/

static inline unsigned int bitmask16_get_lowest_index(uint16_t mask) {
    unsigned int index = 0;

    if (!mask) {
        return 16;
    }

    while (!(mask & UINT16_C(1))) {
        mask >>= 1;
        index++;
    }

    return index;
}

static inline unsigned int bitmask32_get_lowest_index(uint32_t mask) {
    unsigned int index = 0;

    if (!mask) {
        return 32;
    }

    while (!(mask & UINT32_C(1))) {
        mask >>= 1;
        index++;
    }

    return index;
}

static inline unsigned int bitmask64_get_lowest_index(uint64_t mask) {
    unsigned int index = 0;

    if (!mask) {
        return 64;
    }

    while (!(mask & UINT64_C(1))) {
        mask >>= 1;
        index++;
    }

    return index;
}

#endif /* BITMASK_UTIL_H */