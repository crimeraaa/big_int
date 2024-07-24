#include <cstring>
#include "arena.hpp"

void arena_main(LString args[], size_t count, Arena* arena)
{
    for (size_t i = 0; i < count; i++) {
        printf("args[%zu](data=\"%s\", length=%zu)\n",
               i, args[i].data, args[i].length);
    }
    printf("\n");
    
    /*
    ARENA STATE
        active = 1, capacity = 1024
    VISUALIZATION
        [0x0]   char
     */
    char* cptr = arena_alloc_1(char, arena);
    *cptr = 'a';
    printf("cptr = %p, *cptr = '%c'\n", cptr, *cptr);
    arena_print(arena);

    /*
    ARENA STATE
        active = 16, capacity = 1024
    VISUALIZATION
        [0x0]   char
        [0x1]   <padding>
        ...     -
        [0x8]   size_t[0]
        ...     -
        [0xf]   -
     */
    size_t* szptr = arena_alloc_1(size_t, arena);
    *szptr = 12;
    printf("szptr = %p, *szptr = %zu\n", szptr, *szptr);
    arena_print(arena);
    
    /* 
    ARENA STATE
        active = 18, capacity = 1024
    VISUALIZATION
        [0x0]   char
        [0x1]   <padding>
        ...     -
        [0x8]   size_t
        ...     -
        [0xf]   -
        [0xa]   short
        ...     -
        [0xc]   <inactive>
     */
    short* hptr = arena_alloc_1(short, arena);
    *hptr = -16000;
    printf("hptr = %p, *hptr = %hi\n", hptr, *hptr);
    arena_print(arena);
    
    char* cstr = arena_alloc_n(char, arena, 8);
    std::memcpy(cstr, "Hi mom!", sizeof("Hi mom!") - 1);
    cstr[7] = '\0';
    printf("cstr = %p, cstr = \"%s\"\n", cstr, cstr);
    arena_print(arena);
    
    int* iaptr = arena_alloc_n(int, arena, 2);
    iaptr[0] = 943;
    iaptr[1] = -57;
    printf("iaptr = %p, iaptr[0] = %i, iaptr[1] = %i\n", iaptr, iaptr[0], iaptr[1]);
    arena_print(arena);
}

Arena* arena_new(Size cap)
{
    // We subtract 1 since `Arena::buffer[1]` is included in `sizeof(Arena)`.
    Size   sz   = sizeof(Arena) + sizeof(Arena::buffer[0]) * (cap - 1);
    Arena* self = cast(Arena*, std::malloc(sz)); // should be aligned!
    if (self == nullptr) {
        DEBUG_PRINTFLN("Cannot allocate %zu bytes", cap);
        exit(EXIT_FAILURE);
    } else {
        self->next      = nullptr;
        self->active    = 0;
        self->capacity  = cap;
    }
    return self;
}

void arena_reset(Arena* self)
{
    int depth = 1;
    for (Arena* it = self; it != nullptr; depth++, it = it->next) {
        DEBUG_PRINTFLN("[%i] Reset " ARENA_FMTSTR, depth, ARENA_FMTARGS(it));
        it->active = 0;
    }
}

void arena_free(Arena*& self)
{
    int    depth = 1;
    Arena* next;
    for (Arena* it = self; it != nullptr; depth++, it = next) {
        DEBUG_PRINTFLN("[%i] Free " ARENA_FMTSTR, depth, ARENA_FMTARGS(it));
        // Save now as when `it` is freed, `it->next` will be invalid.
        next = it->next;
        std::free(cast(void*, it));
    }
    self = nullptr;
}

static Size next_pow2(Size n)
{
    Size p2 = 1;
    while (p2 < n)
        p2 *= 2;
    return p2;
}

void* arena_alloc(Arena* self, Size sz, Size align)
{
    Arena* it  = self;
    Size   pad = ARENA_GETPADDING(align, it->active);
    
    // Try to find or chain an arena that can accomodate our allocation.
    while (it->active + sz + pad > it->capacity) {
        if (it->next == nullptr)
            it->next = arena_new(next_pow2(it->active + sz + pad));
        it  = it->next;
        pad = ARENA_GETPADDING(align, it->active);
        DEBUG_PRINTFLN("sz = %zu, align = %zu, pad = %zu", sz, align, pad);
    }

    Byte* data  = &it->buffer[it->active + pad];
    it->active += sz + pad;
    return cast(void*, data);
}

void arena_print(const Arena* self)
{
    int depth = 1;
    for (const Arena* it = self; it != nullptr; it = it->next) {
        std::printf("[%i] " ARENA_FMTSTRLN, depth, ARENA_FMTARGS(it));
        depth += 1;
    }
#ifdef BIGINT_DEBUG
    std::printf("\n");
#endif
}
