#define _DEFAULT_SOURCE

#include "arena.h"

#include <string.h>

#define ARENA_BASE_POS (sizeof(mem_arena))
#define ARENA_ALIGN (sizeof(void*))

// round n up to the next multiple of p, p must be a power of 2
#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1))) 

u32 platform_mem_pagesize();
void* platform_mem_reserve(u64 size);
b32 platform_mem_commit(void* ptr, u64 size);
b32 platform_mem_decommit(void* ptr, u64 size); 
b32 platform_mem_release(void* ptr, u64 size); 

#if defined(__linux__)

#include <unistd.h>
#include <sys/mman.h>

u32 platform_mem_pagesize(void) {
    return (u32)sysconf(_SC_PAGESIZE);
}

void* platform_mem_reserve(u64 size) {
    void* out = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (out == MAP_FAILED) {
        return NULL;
    }
    return out;
}

b32 platform_mem_commit(void* ptr, u64 size) {
    s32 ret = mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return ret == 0;
}

b32 platform_mem_decommit(void* ptr, u64 size) {
    s32 ret = mprotect(ptr, size, PROT_NONE);
    if (ret != 0) return false;
    ret = madvise(ptr, size, MADV_DONTNEED);
    return ret == 0;
}

b32 platform_mem_release(void* ptr, u64 size) {
    s32 ret = munmap(ptr, size);
    return ret == 0;
}

#endif

mem_arena* arena_create(u64 reserve_size, u64 commit_size) {
    u32 pagesize = platform_mem_pagesize();
    reserve_size = ALIGN_UP_POW2(reserve_size, pagesize);
    commit_size = ALIGN_UP_POW2(commit_size, pagesize);
    if (commit_size > reserve_size) {
        return NULL;
    }
    mem_arena* arena = platform_mem_reserve(reserve_size);

    if (!arena) {
        return NULL;
    }

    if (!platform_mem_commit(arena, commit_size)) {
        platform_mem_release(arena, reserve_size);
        return NULL;
    }

    arena->reserve_size = reserve_size;
    arena->commit_size = commit_size;
    arena->current_pos = ARENA_BASE_POS;
    arena->commit_pos = commit_size;
    
    return arena;
}

void arena_destroy(mem_arena* arena) {
    if (arena) {
        platform_mem_release(arena, arena->reserve_size);
    }
}

void* arena_push(mem_arena* arena, u64 size, b32 non_zero) {

    if (!arena || size == 0) {
        return NULL;
    }
    u64 pos_aligned = ALIGN_UP_POW2(arena->current_pos, ARENA_ALIGN);
    u64 new_pos = pos_aligned + size;

    if (new_pos > arena->reserve_size) { return NULL; }

    if (new_pos > arena->commit_pos) {
        u64 new_commit_pos = new_pos;
        new_commit_pos += arena->commit_size - 1;
        new_commit_pos -= new_commit_pos % arena->commit_size;
        new_commit_pos = MIN(new_commit_pos, arena->reserve_size);

        u8* mem = (u8*)arena + arena->commit_pos;
        u64 commit_size = new_commit_pos - arena->commit_pos;

        if (!platform_mem_commit(mem, commit_size)) {
            return NULL;
        }

        arena->commit_pos = new_commit_pos;
    }

    arena->current_pos = new_pos;

    u8* out = (u8*)arena + pos_aligned;

    if (!non_zero) {
        memset(out, 0, size);
    }

    return out;
}

void arena_reset(mem_arena* arena) {
    if (arena) {
        arena->current_pos = ARENA_BASE_POS;
    }
}

void arena_pop(mem_arena* arena, u64 size) {
    if (!arena) {
        return;
    }
    size = ALIGN_UP_POW2(size, ARENA_ALIGN);
    size = MIN(size, arena->current_pos - ARENA_BASE_POS);
    arena->current_pos -= size;
}

void arena_pop_to(mem_arena* arena, u64 target_pos) {
    if (!arena) {
        return;
    }
    target_pos = MAX(target_pos, ARENA_BASE_POS);

    if (target_pos < arena->current_pos) {
        arena->current_pos = target_pos;
    }
}