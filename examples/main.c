#include <stdio.h>

#include "arena.h"

int main(void) {
    
    mem_arena* perm_arena = arena_create(GiB(1), MiB(1));

    if (!perm_arena) {
        fprintf(stderr, "Failed to create permanent arena\n");
        return 1;
    }
    
    while (1) {
        arena_push(perm_arena, MiB(16), false);
        getc(stdin);
    }

    arena_destroy(perm_arena);

    return 0;
}
