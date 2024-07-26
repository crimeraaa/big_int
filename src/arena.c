#include "arena.h"

#define BITFLAG_IS_ON(n, flag)  ((n) & (flag))
#define BITFLAG_IS_OFF(n, flag) (!BITFLAG_IS_ON(n, flag))

#define DEBUG_MEMERR_NEW        1
#define DEBUG_MEMERR_ALLOC      2
#if DEBUG_MEMERR == DEBUG_MEMERR_NEW
#   define malloc(...)     NULL
#endif

static void xalloc_test()
{
    DEBUG_PRINTLN("===New scratch arena===");
    Arena *scratch = arena_new(128);
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
     */
    DEBUG_PRINTLN("alloc `char`: len = default(1), extra = default(0)");
    char *select0 = arena_alloc(char, scratch);
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
        [0x01]  *select1
     */
    DEBUG_PRINTLN("alloc `char`: len = 1, extra = default(0)");
    char *select1 = arena_alloc(char, scratch, 1);
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
    char *select2 = arena_alloc(char, scratch, 1, 3);
    arena_print(scratch);
    
    DEBUG_PRINTFLN("select0 = %p", select0);
    DEBUG_PRINTFLN("select1 = %p", select1);
    DEBUG_PRINTFLN("select2 = %p", select2);
    arena_free(&scratch);
    DEBUG_PRINTLN("===Free scratch arena===\n");
}

static void align_test()
{
    DEBUG_PRINTLN("===New scratch arena===");
    Arena *scratch = arena_new(64);
    /*
    ARENA STATE
        active = 1, capacity = 64
    VISUALIZATION
        [0x0]   char
     */
    char *cptr = arena_alloc(char, scratch);
    *cptr = 'a';
    printf("cptr = %p, *cptr = '%c'\n", cptr, *cptr);
    arena_print(scratch);

    /*
    ARENA STATE
        active = 16, capacity = 64
    VISUALIZATION
        [0x0]   char
        [0x1]   <padding>
        ...     -
        [0x8]   size[0]
        ...     -
        [0xf]   -
     */
    size *szptr = arena_alloc(size, scratch);
    *szptr = 12;
    printf("szptr = %p, *szptr = %td\n", szptr, *szptr);
    arena_print(scratch);
    
    /* 
    ARENA STATE
        active = 18, capacity = 64
    VISUALIZATION
        [0x0]   char
        [0x1]   <padding>
        ...     -
        [0x8]   size
        ...     -
        [0xf]   -
        [0x10]  short
        ...     -
        [0x12]   <inactive>
     */
    short *hptr = arena_alloc(short, scratch);
    *hptr = -16000;
    printf("hptr = %p, *hptr = %hi\n", hptr, *hptr);
    arena_print(scratch);
    
    /* 
    ARENA STATE
        active = 26, capacity = 64
    VISUALIZATION
        [0x0]   char
        [0x1]   <padding>
        ...     -
        [0x8]   size
        ...     -
        [0x10]  short
        [0x11]  -
        [0x12]  'H'
        [0x13]  'i'
        [0x14]  'm'
        [0x15]  ' '
        [0x16]  'o'
        [0x17]  'm'
        [0x18]  '!'
        [0x19]  '\0'
        [0x1a]  <inactive>
     */
    LString msg  = lstr_literal("Hi mom!");
    char   *cstr = arena_alloc(char, scratch, 8);
    memcpy(cstr, msg.data, msg.length);
    cstr[7] = '\0';
    printf("cstr = %p, cstr = \"%s\"\n", cstr, cstr);
    arena_print(scratch);
    
    /* 
    ARENA STATE
        active = 36, capacity = 64
    VISUALIZATION
        [0x0]   char
        [0x1]   <padding>
        ...     -
        [0x8]   size
        ...     -
        [0x10]  short
        [0x11]  -
        [0x12]  "Hi mom!"
        ...     -
        [0x1a]  <padding>
        ...     -
        [0x1c]  int[0]
        ...     -
        [0x20]  int[1]
        ...     -
        [0x24]  <inactive>
     */
    int *iaptr = arena_alloc(int, scratch, 2);
    iaptr[0] = 943;
    iaptr[1] = -57;
    printf("iaptr = %p, iaptr[0] = %i, iaptr[1] = %i\n", iaptr, iaptr[0], iaptr[1]);
    
    arena_print(scratch);
    arena_free(&scratch);
    DEBUG_PRINTLN("===Free scratch arena===\n");
}

void arena_main(const LString args[], size count, Arena *arena)
{
    xalloc_test();
    align_test();
    for (size i = 0; i < count; i++) {
        printf("args[%td](data=\"%s\", length=%td)\n",
               i, args[i].data, args[i].length);
    }
    printf("\n");
    arena_print(arena);
}

// By default we choose to report the bad allocation and immediately abort.
static void exit_errorfn(const char *msg, size req, void *ctx)
{
    cast(void, ctx);
    eprintfln("[FATAL]: %s (requested %td bytes)", msg, req);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

// Does not throw by itself. Callers must determine if they should throw.
static Arena *raw_new(size cap, uint8_t flags, ErrorFn fn, void *ctx)
{
    Arena *self = fam_new(Arena, self->buffer, cap);
    if (self != nullptr) {
        self->next     = nullptr;
        self->flags    = flags;
        self->handler  = (fn != nullptr) ? fn  : &exit_errorfn;
        self->context  = (fn != nullptr) ? ctx : nullptr;
        self->active   = 0;
        self->capacity = cap;
        if (BITFLAG_IS_OFF(flags, ARENA_NOZERO))
            memset(self->buffer, 0, array_sizeof(self->buffer, cap));
    }
    return self;
}

static Arena *raw_chain(Arena *parent, size cap)
{
    Arena *child = raw_new(cap, parent->flags, parent->handler, parent->context);
    parent->next = child;
    return child;
}

Arena *arena_rawnew(size cap, uint8_t flags, ErrorFn fn, void *ctx)
{
    Arena *self = raw_new(cap, flags, fn, ctx);
    if (self == nullptr) {
        if (BITFLAG_IS_ON(flags, ARENA_NOTHROW))
            return nullptr;
        if (fn == nullptr)
            fn = &exit_errorfn;
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
#ifdef DEBUG_USE_PRINT
    int depth = 1;
#endif
    for (Arena *it = self; it != nullptr; it = it->next) {
        DEBUG_PRINTFLN("[%i] Reset " ARENA_FMTSTR, depth++, ARENA_FMTARGS(it));
        it->active = 0;
    }
}

void arena_free(Arena **self)
{
#ifdef DEBUG_USE_PRINT
    int depth = 1;
#endif
    for (Arena *it = *self, *next; it != nullptr; it = next) {
        DEBUG_PRINTFLN("[%i] Free " ARENA_FMTSTR, depth++, ARENA_FMTARGS(it));
        next = it->next; // Save now because it->next is about to be invalid.
        fam_free(Arena, it->buffer, it, it->capacity);
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

void *arena_rawalloc(Arena *self, size rawsz, size align)
{
    // Try to find or chain a new arena that can accomodate our allocation.
    Arena *it  = self;
    size   pad = ARENA_GETPADDING(align, it->active);
    while (it->active + rawsz + pad > it->capacity) {
        if (it->next == nullptr) {
            size   reqsz = rawsz + pad;
            size   ncap  = (reqsz > it->capacity) ? next_pow2(reqsz) : it->capacity;
#if DEBUG_MEMERR == DEBUG_MEMERR_ALLOC
            Arena *next = nullptr;
#else
            Arena *next = raw_chain(it, ncap);
#endif
            if (next == nullptr) {
                if (BITFLAG_IS_ON(it->flags, ARENA_NOTHROW))
                    return nullptr;
                else
                    it->handler("Failed to chain new Arena", ncap, it->context);
            }
        }
        it  = it->next;
        pad = ARENA_GETPADDING(align, it->active);
    }
    uint8_t *data  = &it->buffer[it->active + pad];
    it->active    += rawsz + pad;
    return cast(void*, data);
}

void *arena_rawrealloc(Arena *self, void *prev, size oldsz, size newsz, size align)
{
    // Don't waste our time!
    if (oldsz >= newsz)
        return prev;
    // NOTE: Only receive `nullptr` when `ARENA_NOTHROW` is on.
    void *next = arena_rawalloc(self, newsz, align);
    if (next == nullptr)
        return nullptr;
    return memcpy(next, prev, oldsz);
}

void arena_print(const Arena *self)
{
    int depth = 1;
    for (const Arena *it = self; it != nullptr; ++depth, it = it->next) {
        printf("[%i] " ARENA_FMTSTRLN, depth, ARENA_FMTARGS(it));
    }
}
