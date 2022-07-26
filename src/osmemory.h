// This source file is part of the Stratum project.
//
// Licensed under the Apache License v2.0

#ifndef STRATUM_OSMEMORY_H_
#define STRATUM_OSMEMORY_H_

#include <cstddef>

namespace stratum::os {
    void *Alloc(size_t size);

    void Free(void *ptr, size_t size);
} // namespace stratum::os

#endif // !STRATUM_OSMEMORY_H_
