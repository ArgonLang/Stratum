// This source file is part of the Stratum project.
//
// Licensed under the Apache License v2.0

#include <cassert>
#include <mutex>

#include "memutil.h"
#include "memory.h"

using namespace stratum;

Memory default_allocator;

Arena *Memory::FindOrCreateArena() {
    Arena *arena = this->arenas_.FindFree();

    if (arena == nullptr) {
        arena = AllocArena();
        this->arenas_.Insert(arena);
    }

    return arena;
}

bool Memory::Initialize() {
    for (int i = 0; i < STRATUM_MINIMUM_POOL; i++) {
        auto *arena = AllocArena();

        if (arena == nullptr) {
            if (i > 0) {
                this->FinalizeMemory();
            }
            return false;
        }

        this->arenas_.Insert(arena);
    }

    return true;
}

Pool *Memory::GetPool(size_t clazz) {
    Pool *pool = this->pools_[clazz].FindFree();

    if (pool == nullptr) {
        {
            std::scoped_lock alck(this->m_arenas_);
            Arena *arena = this->FindOrCreateArena();
            pool = AllocPool(arena, clazz);
        }

        this->pools_[clazz].Insert(pool);
    }

    return pool;
}

void *Memory::Alloc(size_t size) noexcept {
    size_t clazz = SizeToPoolClass(size);

    assert(size > 0);

    if (size <= STRATUM_BLOCK_MAX_SIZE) {
        std::scoped_lock lck(this->m_pools_[clazz]);
        auto *pool = GetPool(clazz);

        return AllocBlock(pool);
    }

    // TODO

    return nullptr;
}

void Memory::FinalizeMemory() {
    Arena *arena;

    while ((arena = this->arenas_.Pop()) != nullptr) {
        FreeArena(arena);
    }
}

void Memory::Free(void *ptr) {
    if (ptr == nullptr)
        return;

    auto *pool = (Pool *) AlignDown(ptr, STRATUM_PAGE_SIZE);

    if (AddressInArenas(ptr)) {
        size_t clazz = SizeToPoolClass(pool->blocksz);

        std::scoped_lock lck(this->m_pools_[clazz]);

        FreeBlock(pool, ptr);
        this->TryReleaseMemory(pool, clazz);
        return;
    }

    // TODO
}

void *Memory::Realloc(void *ptr, size_t size) {
    void *tmp;
    size_t src_sz;

    if (ptr == nullptr)
        return this->Alloc(size);

    const auto *pool = (Pool *) AlignDown(ptr, STRATUM_PAGE_SIZE);

    if (AddressInArenas(ptr)) {
        size_t actual = SizeToPoolClass(pool->blocksz);
        size_t desired = SizeToPoolClass(size);
        src_sz = pool->blocksz;

        if (actual > desired && (actual - desired < STRATUM_REALLOC_THRESHOLD))
            return ptr;
    } else {
        // TODO
    }

    if ((tmp = Alloc(size)) == nullptr)
        return nullptr;

    util::MemoryCopy(tmp, ptr, src_sz > size ? size : src_sz);
    Free(ptr);
    return tmp;
}

void Memory::TryReleaseMemory(Pool *pool, size_t clazz) {
    Arena *arena = pool->arena;

    if (pool->free == pool->blocks) {
        std::scoped_lock lck(this->m_arenas_);
        this->pools_[clazz].Remove(pool);

        FreePool(pool);

        if (arena->free != arena->pools)
            this->arenas_.Sort(arena);
        else if (this->arenas_.Count() > STRATUM_MINIMUM_POOL) {
            this->arenas_.Remove(arena);
            FreeArena(arena);
        }

        return;
    }

    this->pools_[clazz].Sort(pool);
}

// DEFAULT ALLOCATOR

void *stratum::Alloc(size_t size) noexcept {
    return default_allocator.Alloc(size);
}

void stratum::Free(void *ptr) {
    return default_allocator.Free(ptr);
}

void *stratum::Realloc(void *ptr, size_t size) {
    return default_allocator.Realloc(ptr, size);
}

bool stratum::InitializeMemory() {
    return default_allocator.Initialize();
}

void stratum::FinalizeMemory() {
    default_allocator.FinalizeMemory();
}
