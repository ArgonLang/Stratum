// This source file is part of the Stratum project.
//
// Licensed under the Apache License v2.0

#ifndef STRATUM_ARENA_H_
#define STRATUM_ARENA_H_

#include <cstddef>
#include <cstdint>

#define STRATUM_PAGE_SIZE          4096             // Assume page size of 4096 bytes
#define STRATUM_ARENA_SIZE         (256u << 10u)    // 256 KiB
#define STRATUM_POOLS_AVAILABLE    (STRATUM_ARENA_SIZE / STRATUM_PAGE_SIZE)

#define STRATUM_QUANTUM            8
#define STRATUM_BLOCK_MAX_SIZE     1024
#define STRATUM_CLASSES            (STRATUM_BLOCK_MAX_SIZE / STRATUM_QUANTUM)

/*
 * Stratum memory layout:
 *                                 +--+
 * +-------------------------------+  |
 * | POOL  | POOL  | POOL  | POOL  |  |
 * | HEADER| HEADER| HEADER| HEADER|  |
 * +-------+-------+-------+-------|  |
 * | BLOCK |       | BLOCK |       |  |
 * |       | BLOCK +-------+   B   |  |
 * +-------+       | BLOCK |   I   |  | A
 * | BLOCK +-------+-------+   G   |  | R
 * |       |       | BLOCK |       |  | E . . .
 * +-------+ BLOCK +-------+   B   |  | N
 * | BLOCK |       | BLOCK |   L   |  | A
 * |       +-------+-------+   O   |  |
 * +-------+       | BLOCK |   C   |  |
 * +-------+ BLOCK +-------+   K   |  |
 * | ARENA |       | BLOCK |       |  |
 * +-------------------------------+  |
 *                                 +--+
 *     ^       ^       ^       ^
 *     +-------+-------+-------+--- < MEMORY PAGES (STRATUM_PAGE_SIZE)
 */

namespace stratum {
    struct alignas(STRATUM_QUANTUM)Arena {
        /* Total pools in the arena */
        unsigned int pools;

        /* Number of free pools in the arena */
        unsigned int free;

        /* Pointer to linked-list of available pools */
        struct Pool *pool;

        /* Pointers to next and prev arena, arenas are linked between each other with a doubly linked-list */
        struct Arena *next;
        struct Arena **prev;
    };

    struct alignas(STRATUM_QUANTUM)Pool {
        /* Pointer to Arena */
        Arena *arena;

        /* Total blocks in pool */
        unsigned short blocks;

        /* Free blocks in pool */
        unsigned short free;

        /* Size of single memory block */
        unsigned short blocksz;

        /* Pointer to linked-list of available blocks */
        void *block;

        /* Pointers to next and prev pool of a specific size-class */
        struct Pool *next;
        struct Pool **prev;
    };

    inline void *AlignDown(const void *ptr, const size_t sz) {
        return (void *) (((uintptr_t) ptr) & ~(sz - 1));
    }

    inline void *AlignUp(const void *ptr, const size_t sz) {
        return (void *) (((uintptr_t) ptr + sz) & ~(sz - 1));
    }

    inline size_t SizeToPoolClass(size_t size) {
        return (((size + (STRATUM_QUANTUM - 1)) & ~((size_t) STRATUM_QUANTUM - 1)) / STRATUM_QUANTUM) - 1;
    }

    inline size_t ClassToSize(size_t clazz) {
        return STRATUM_QUANTUM + (STRATUM_QUANTUM * clazz);
    }

    inline bool AddressInArenas(const void *ptr) {
        const auto *p = (Pool *) AlignDown(ptr, STRATUM_PAGE_SIZE);
        return p->arena != nullptr
               && (((uintptr_t) ptr - (uintptr_t) AlignDown(p->arena, STRATUM_PAGE_SIZE)) < STRATUM_PAGE_SIZE)
               && p->arena->pools == STRATUM_POOLS_AVAILABLE;
    }

    Arena *AllocArena();

    Pool *AllocPool(Arena *arena, size_t clazz);

    void FreeArena(Arena *arena);

    void FreePool(Pool *pool);

    void *AllocBlock(Pool *pool);

    void FreeBlock(Pool *pool, void *block);

} // namespace stratum

#endif // !STRATUM_ARENA_H_
