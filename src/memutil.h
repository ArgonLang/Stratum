// This source file is part of the Stratum project.
//
// Licensed under the Apache License v2.0

#ifndef STRATUM_MEMUTIL_H_
#define STRATUM_MEMUTIL_H_

#include <cstddef>

namespace stratum::util {
    int MemoryCompare(const void *ptr1, const void *ptr2, size_t num);

    void *MemoryConcat(void *dest, size_t sized, void *s1, size_t size1, void *s2, size_t size2);

    void *MemoryCopy(void *dest, const void *src, size_t size);

    void *MemoryFind(const void *buf, unsigned char value, size_t size);

    void *MemorySet(void *ptr, int value, size_t num);

    inline void *MemoryZero(void *ptr, size_t num) { return MemorySet(ptr, 0x00, num); }
}


#endif // !STRATUM_MEMUTIL_H_
