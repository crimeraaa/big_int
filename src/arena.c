#include "arena.h"

#define DEBUG_MEMERR_NEW    1
#define DEBUG_MEMERR_ALLOC      2

#if DEBUG_MEMERR == DEBUG_MEMERR_NEW
#   define malloc(...)     NULL
#endif

static void xalloc_test()
{
    Arena *scratch = arena_new(128, ARENA_DEFAULT, nullptr, nullptr);
    DEBUG_PRINTLN("New scratch arena");
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
     */
    DEBUG_PRINTLN("alloc `char`: len = align = extra = default");
    char *select0 = arena_xalloc(char, scratch);
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
        [0x01]  *select1
     */
    DEBUG_PRINTLN("alloc `char`: len = 1, align = extra = default");
    char *select1 = arena_xalloc(char, scratch, 1);
    arena_print(scratch);

    /*  
    VISUALIZATION:
        [0x00]  *select0
        [0x01]  *select1
        [0x02]  *select2
        [0x03]  <padding>
    */
    DEBUG_PRINTLN("alloc `char`: len = 1, align = 2, extra = default");
    char *select2 = arena_xalloc(char, scratch, 1, 2);
    arena_print(scratch);

    /*  
    VISUALIZATION:
        [0x00]  *select0
        [0x01]  *select1
        [0x02]  *select2
        [0x03]  <padding>
        [0x04]  *select3
        [0x05]  <extra>
        [0x06]  -
        [0x07]  -
    */
    DEBUG_PRINTLN("alloc `char`: len = 1, align = 2, extra = 3");
    char *select3 = arena_xalloc(char, scratch, 1, 2, 3);
    arena_print(scratch);
    
    printf("[DEBUG]: select0 = %p\n", select0);
    printf("[DEBUG]: select1 = %p\n", select1);
    printf("[DEBUG]: select2 = %p\n", select2);
    printf("[DEBUG]: select3 = %p\n", select3);
    arena_free(&scratch);
}

void arena_main(const LString args[], Size count, Arena *arena)
{
    xalloc_test();
    for (Size i = 0; i < count; i++) {
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
    char *cptr = arena_xalloc(char, arena);
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
    size_t *szptr = arena_xalloc(size_t, arena);
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
    short *hptr = arena_xalloc(short, arena);
    *hptr = -16000;
    printf("hptr = %p, *hptr = %hi\n", hptr, *hptr);
    arena_print(arena);
    
    char *cstr = arena_xalloc(char, arena, 8);
    memcpy(cstr, "Hi mom!", sizeof("Hi mom!") - 1);
    cstr[7] = '\0';
    printf("cstr = %p, cstr = \"%s\"\n", cstr, cstr);
    arena_print(arena);
    
    int *iaptr = arena_xalloc(int, arena, 2);
    iaptr[0] = 943;
    iaptr[1] = -57;
    printf("iaptr = %p, iaptr[0] = %i, iaptr[1] = %i\n", iaptr, iaptr[0], iaptr[1]);
    arena_print(arena);
}

static void default_error_handler(const char *msg, Size req, void *ctx)
{
    cast(void, ctx);
    fprintf(stderr, "[FATAL]: %s (requested %zu bytes)\n", msg, req);
    exit(EXIT_FAILURE);
}

static Arena *arena__new(Size cap, uint8_t flags, ErrorFn fn, void *ctx)
{
    Arena *self = fam_new(Arena, self->buffer, cap);
    if (self != nullptr) {
        self->next     = nullptr;
        self->flags    = flags;
        self->handler  = (fn != nullptr) ? fn  : &default_error_handler;
        self->context  = (fn != nullptr) ? ctx : nullptr;
        self->active   = 0;
        self->capacity = cap;
        if (!(flags & ARENA_NOZERO))
            memset(self->buffer, 0, array_sizeof(self->buffer, cap));
    }
    return self;
}

static Arena *arena__chain(Arena *parent, Size cap)
{
    Arena *child = arena__new(cap, parent->flags, parent->handler, parent->context);
    parent->next = child;
    return child;
}

Arena *arena_new(Size cap, uint8_t flags, ErrorFn fn, void *ctx)
{
    Arena *self = arena__new(cap, flags, fn, ctx);
    if (self == nullptr) {
        if (flags & ARENA_NOTHROW)
            return nullptr;
        else
            fn("Failed to allocate new Arena", cap, ctx);
    }
    return self;
}

void arena_set_flags(Arena *self, uint8_t flags)
{
    for (Arena *it = self; it != nullptr; it = it->next) {
        it->flags |= flags & ARENA_NOZERO;
        it->flags |= flags & ARENA_NOTHROW;
    }
}

void arena_clear_flags(Arena *self, uint8_t flags)
{
    for (Arena *it = self; it != nullptr; it = it->next) {
        it->flags ^= flags & ARENA_NOZERO;
        it->flags ^= flags & ARENA_NOTHROW;
    }
}

void arena_reset(Arena *self)
{
    int depth = 1;
    for (Arena *it = self; it != nullptr; depth++, it = it->next) {
        DEBUG_PRINTFLN("[%i] Reset " ARENA_FMTSTR, depth, ARENA_FMTARGS(it));
        it->active = 0;
    }
}

void arena_free(Arena **self)
{
    int    depth = 1;
    Arena *next;
    for (Arena *it = *self; it != nullptr; depth++, it = next) {
        DEBUG_PRINTFLN("[%i] Free " ARENA_FMTSTR, depth, ARENA_FMTARGS(it));
        next = it->next; // Save now as when `it` is freed, `it->next` will be invalid.
        free(cast(void*, it));
    }
    *self = nullptr;
}

static Size next_pow2(Size n)
{
    Size p2 = 1;
    while (p2 < n)
        p2 *= 2;
    return p2;
}

void *arena_alloc(Arena *self, Size sz, Size align)
{
    Arena *it  = self;
    Size   pad = ARENA_GETPADDING(align, it->active);
    
    // Try to find or chain an arena that can accomodate our allocation.
    while (it->active + sz + pad > it->capacity) {
        if (it->next == nullptr) {
            Size   ncap = next_pow2(it->active + sz + pad);
#if DEBUG_MEMERR == DEBUG_MEMERR_ALLOC
            Arena *next = nullptr;
#else
            Arena *next = arena__chain(it, ncap);
#endif
            if (next == nullptr) {
                if (it->flags & ARENA_NOTHROW)
                    return nullptr;
                else
                    it->handler("Failed to chain new Arena", ncap, it->context);
            }
            it->next    = next;
        }
        it  = it->next;
        pad = ARENA_GETPADDING(align, it->active);
        DEBUG_PRINTFLN("sz = %zu, align = %zu, pad = %zu", sz, align, pad);
    }

    Byte *data  = &it->buffer[it->active + pad];
    it->active += sz + pad;
    return cast(void*, data);
}

void arena_print(const Arena *self)
{
    int depth = 1;
    for (const Arena *it = self; it != nullptr; ++depth, it = it->next) {
        printf("[%i] " ARENA_FMTSTRLN, depth, ARENA_FMTARGS(it));
    }
#ifdef DEBUG_USE_PRINT
    printf("\n");
#endif
}
