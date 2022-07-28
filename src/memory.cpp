// This source file is part of the Stratum project.
//
// Licensed under the Apache License v2.0

#include <cassert>
#include <mutex>

#include "memutil.h"
#include "memory.h"

using namespace stratum;

struct Emb {
    size_t size;
    size_t offset;
};

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

Pool *Memory::AllocatePool(size_t clazz) {
    std::scoped_lock alck(this->m_arenas_);

    auto *arena = this->FindOrCreateArena();
    if (arena == nullptr)
        return nullptr;

    return AllocPool(arena, clazz);
}

Pool *Memory::GetPool(size_t clazz) {
    Pool *pool = this->pools_[clazz].FindFree();

    if (pool == nullptr) {
        if ((pool = this->AllocatePool(clazz)) == nullptr)
            return nullptr;

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
        if (pool == nullptr)
            return nullptr;

        return AllocBlock(pool);
    }

    unsigned char *ptr;
    if ((ptr = (unsigned char *) malloc(size + sizeof(Emb) + STRATUM_QUANTUM)) == nullptr)
        return nullptr;

    auto tmp = (uintptr_t) (ptr + sizeof(Emb));
    auto ptr_a = (unsigned char *) (tmp + (STRATUM_QUANTUM - (tmp % STRATUM_QUANTUM)));

    auto *emb = (Emb *) ptr_a - sizeof(Emb);
    emb->size = size;
    emb->offset = ptr_a - ptr;

    return ptr_a;
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

    const auto *emb = (Emb *) ((unsigned char *) ptr - sizeof(Emb));
    free(((unsigned char *) ptr) - emb->offset);
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
        const auto *emb = (Emb *) (((unsigned char *) ptr) - sizeof(Emb));

        src_sz = emb->size;

        if (size > STRATUM_BLOCK_MAX_SIZE && src_sz >= size)
            return ptr;
    }

    if ((tmp = Alloc(size)) == nullptr)
        return nullptr;

    util::MemoryCopy(tmp, ptr, src_sz > size ? size : src_sz);
    this->Free(ptr);
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
