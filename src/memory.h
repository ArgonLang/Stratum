// This source file is part of the Stratum project.
//
// Licensed under the Apache License v2.0
// EasterEgg: \qpu ;)

#ifndef STRATUM_MEMORY_H_
#define STRATUM_MEMORY_H_

#include <cstddef>

#include <support/linklist.h>

#include "arena.h"

/* Minimum number of arenas, Stratum WILL NEVER release this memory to the OS. */
#define STRATUM_MINIMUM_POOL       16

#define STRATUM_REALLOC_THRESHOLD  10

namespace stratum {
    class Memory {
        /* Arena Linked-List */
        support::LinkedList<Arena> arenas_;
        std::mutex m_arenas_;

        /* Memory pools organized by size-class */
        support::LinkedList<Pool> pools_[STRATUM_CLASSES];
        std::mutex m_pools_[STRATUM_CLASSES];

        Arena *FindOrCreateArena();

        Pool *AllocatePool(size_t clazz);

        Pool *GetPool(size_t clazz);

        void TryReleaseMemory(Pool *pool, size_t clazz);

    public:
        bool Initialize();

        void *Alloc(size_t size) noexcept;

        void FinalizeMemory();

        void Free(void *ptr);

        void *Realloc(void *ptr, size_t size);
    };

    void *Alloc(size_t size) noexcept;

    bool InitializeMemory();

    void FinalizeMemory();

    void Free(void *ptr);

    void *Realloc(void *ptr, size_t size);
} // namespace stratum

#endif // !STRATUM_MEMORY_H_
