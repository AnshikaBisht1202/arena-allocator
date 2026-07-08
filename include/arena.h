#ifndef ARENA_H
#define ARENA_H

#include "base.h"

typedef struct {
    u64 reserve_size;
    u64 commit_size;
    u64 current_pos;
    u64 commit_pos;
} mem_arena;

mem_arena* arena_create(u64 reserve_size, u64 commit_size);

void arena_destroy(mem_arena* arena);

void* arena_push(mem_arena* arena, u64 size, b32 non_zero);

void arena_pop(mem_arena* arena, u64 size);

void arena_pop_to(mem_arena* arena, u64 target_pos);

void arena_reset(mem_arena* arena);

#endif