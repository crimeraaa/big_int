#include "arena.h"

#define DEBUG_MEMERR_NEW    1
#define DEBUG_MEMERR_ALLOC  2

#if DEBUG_MEMERR == DEBUG_MEMERR_NEW
#   define malloc(...)     NULL
#endif

static void xalloc_test()
{
    Arena *scratch = arena_xnew(128);
    DEBUG_PRINTLN("New scratch arena");
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
     */
    DEBUG_PRINTLN("alloc `char`: len = default(1), extra = default(0)");
    char *select0 = arena_xalloc(char, scratch);
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
        [0x01]  *select1
     */
    DEBUG_PRINTLN("alloc `char`: len = 1, extra = default(0)");
    char *select1 = arena_xalloc(char, scratch, 1);
    arena_print(scratch);

    /*  
    VISUALIZATION:
        [0x00]  *select0
        [0x01]  *select1
        [0x02]  *select2
        [0x03]  <padding>
        [0x04]  -
        [0x05]  -
    */
    DEBUG_PRINTLN("alloc `char`: len = 1, extra = 3");
    char *select2 = arena_xalloc(char, scratch, 1, 3);
    arena_print(scratch);
    
    printf("[DEBUG]: select0 = %p\n", select0);
    printf("[DEBUG]: select1 = %p\n", select1);
    printf("[DEBUG]: select2 = %p\n", select2);
    arena_free(&scratch);
}

void arena_main(const LString args[], size count, Arena *arena)
{
    xalloc_test();
    for (size i = 0; i < count; i++) {
        printf("args[%td](data=\"%s\", length=%td)\n",
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
        [0x8]   size[0]
        ...     -
        [0xf]   -
     */
    size *szptr = arena_xalloc(size, arena);
    *szptr = 12;
    printf("szptr = %p, *szptr = %td\n", szptr, *szptr);
    arena_print(arena);
    
    /* 
    ARENA STATE
        active = 18, capacity = 1024
    VISUALIZATION
        [0x0]   char
        [0x1]   <padding>
        ...     -
        [0x8]   size
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
    
    LString msg = lstr_literal("Hi mom!");
    char  *cstr = arena_xalloc(char, arena, 8);
    memcpy(cstr, msg.data, msg.length);
    cstr[7] = '\0';
    printf("cstr = %p, cstr = \"%s\"\n", cstr, cstr);
    arena_print(arena);
    
    int *iaptr = arena_xalloc(int, arena, 2);
    iaptr[0] = 943;
    iaptr[1] = -57;
    printf("iaptr = %p, iaptr[0] = %i, iaptr[1] = %i\n", iaptr, iaptr[0], iaptr[1]);
    arena_print(arena);
}

static void s_errorfn(const char *msg, size req, void *ctx)
{
    cast(void, ctx);
    eprintfln("[FATAL]: %s (requested %td bytes)", msg, req);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

static Arena *s_new(size cap, uint8_t flags, ErrorFn fn, void *ctx)
{
    Arena *self = fam_new(Arena, self->buffer, cap);
    if (self != nullptr) {
        self->next     = nullptr;
        self->flags    = flags;
        self->handler  = (fn != nullptr) ? fn  : &s_errorfn;
        self->context  = (fn != nullptr) ? ctx : nullptr;
        self->active   = 0;
        self->capacity = cap;
        if (!(flags & ARENA_NOZERO))
            memset(self->buffer, 0, array_sizeof(self->buffer, cap));
    }
    return self;
}

static Arena *s_chain(Arena *parent, size cap)
{
    Arena *child = s_new(cap, parent->flags, parent->handler, parent->context);
    parent->next = child;
    return child;
}

Arena *arena_new(size cap, uint8_t flags, ErrorFn fn, void *ctx)
{
    Arena *self = s_new(cap, flags, fn, ctx);
    if (self == nullptr) {
        if (flags & ARENA_NOTHROW)
            return nullptr;
        if (fn == nullptr)
            fn = &s_errorfn;
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

static size next_pow2(size n)
{
    size p2 = 1;
    while (p2 < n)
        p2 *= 2;
    return p2;
}

void *arena_alloc(Arena *self, size sz, size align)
{
    Arena *it  = self;
    size   pad = ARENA_GETPADDING(align, it->active);
    
    // Try to find or chain an arena that can accomodate our allocation.
    while (it->active + sz + pad > it->capacity) {
        if (it->next == nullptr) {
            size req  = sz + pad;
            size ncap = (req > it->capacity) ? next_pow2(req) : it->capacity;
#if DEBUG_MEMERR == DEBUG_MEMERR_ALLOC
            Arena *next = nullptr;
#else
            Arena *next = s_chain(it, ncap);
#endif
            if (next == nullptr) {
                if (it->flags & ARENA_NOTHROW)
                    return nullptr;
                else
                    it->handler("Failed to chain new Arena", ncap, it->context);
            }
        }
        it  = it->next;
        pad = ARENA_GETPADDING(align, it->active);
        DEBUG_PRINTFLN("sz = %td, align = %td, pad = %td", sz, align, pad);
    }

    uint8_t *data  = &it->buffer[it->active + pad];
    it->active    += sz + pad;
    return cast(void*, data);
}

void arena_print(const Arena *self)
{
    int depth = 1;
    for (const Arena *it = self; it != nullptr; ++depth, it = it->next) {
        printf("[%i] " ARENA_FMTSTRLN, depth, ARENA_FMTARGS(it));
    }
}
