#include "arena.h"

Region *region_new(size cap)
{
    Region *r = cast(Region*, malloc(fam_sizeof(*r, r->buffer[0], cap)));
    if (r) {
        r->next = nullptr;
        r->free = r->buffer;
        r->end  = r->buffer + cap;
    }
    return r;
}

void region_free(Region *r)
{
    fam_free(Region, byte, r, r->capacity);
}

static void xalloc_test(Arena *scratch)
{
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
     */
    println("alloc `char`: len = default(1), extra = default(0)");
    char *select0 = arena_alloc(char, scratch);
    arena_print(scratch);

    /* 
    VISUALIZATION:
        [0x00]  *select0
        [0x01]  *select1
     */
    println("alloc `char`: len = 1, extra = default(0)");
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
    println("alloc `char`: len = 1, extra = 3");
    char *select2 = arena_alloc(char, scratch, 1, 3);
    arena_print(scratch);
    
    printfln("select0 = %p", select0);
    printfln("select1 = %p", select1);
    printfln("select2 = %p", select2);
}

static void align_test(Arena *scratch)
{
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
}

void arena_main(const LString args[], size count, Arena *arena)
{
    Arena scratch;
    DEBUG_PRINTLN("===NEW SCRATCH===");
    arena_init(&scratch, 128);
    arena_print(&scratch);

    DEBUG_PRINTLN("===XALLOC_TEST()===");
    xalloc_test(&scratch);
    arena_print(&scratch);

    DEBUG_PRINTLN("===ALIGN_TEST()===");
    align_test(&scratch);
    arena_print(&scratch);

    arena_free(&scratch);
    DEBUG_PRINTLN("===FREE SCRATCH===");
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

bool arena_rawinit(Arena *self, size cap, uint8_t flags, ErrorFn fn, void *ctx)
{
    Region *r     = region_new(cap);
    self->handler = (fn != nullptr) ? fn : &exit_errorfn;
    self->context = (fn != nullptr) ? ctx : nullptr;
    self->begin   = r;
    self->end     = r;
    self->flags   = flags;

    if (r == nullptr) {
        if (BITFLAG_ON(flags, ARENA_NOTHROW))
            return false;
        else
            self->handler("Failed to allocate new Region", cap, self->context);
        return false; // Unreachable
    }
    if (BITFLAG_OFF(flags, ARENA_NOZERO))
        memset(r->buffer, 0, cap);
    return true;
}

void arena_set_flags(Arena *self, uint8_t flags)
{
    self->flags |= flags & ARENA_NOZERO;
    self->flags |= flags & ARENA_NOTHROW;
    self->flags |= flags & ARENA_NOPADDING;
    self->flags |= flags & ARENA_NOSMARTREALLOC;
}

void arena_clear_flags(Arena *self, uint8_t flags)
{
    self->flags ^= flags & ARENA_NOZERO;
    self->flags ^= flags & ARENA_NOTHROW;
    self->flags ^= flags & ARENA_NOPADDING;
    self->flags ^= flags & ARENA_NOSMARTREALLOC;
}

size arena_active(const Arena *self)
{
    size n = 0;    
    for (const Region *it = self->begin; it != nullptr; it = it->next) {
        n += REGION_ACTIVE(it);
    }
    return n;
}

size arena_capacity(const Arena *self)
{
    size cap = 0;
    for (const Region *it = self->begin; it != nullptr; it = it->next) {
        cap += REGION_CAPACITY(it);
    }
    return cap;
}

void arena_reset(Arena *self)
{
    for (Region *it = self->begin; it != nullptr; it = it->next) {
        it->free = it->buffer;
    }
}

void arena_free(Arena *self)
{
#ifdef DEBUG_USE_PRINT
    int depth = 1;
    DEBUG_PRINTFLN("active=%td, capacity=%td", arena_active(self), arena_capacity(self));
#endif
    Region *next;
    for (Region *it = self->begin; it != nullptr; it = next) {
        DEBUG_PRINTFLN("[%i] Free " REGION_FMTSTR, depth++, REGION_FMTARGS(it));
        next = it->next; // Save now because it->next is about to be invalid.
        region_free(it);
    }
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
    Region *it  = self->begin;
    size    pad;
    for (;;) {
        size active = REGION_ACTIVE(it);
        size cap    = REGION_CAPACITY(it);
        if (BITFLAG_OFF(self->flags, ARENA_NOPADDING))
            pad = 0;
        else
            pad = ARENA_GETPADDING(align, active);
        // Requested size will be in range of the Region?
        if (active + rawsz + pad <= cap)
            break;
        // Need to chain a new region?
        if (it->next == nullptr) {
            size    reqsz = rawsz + pad;
            size    ncap  = (reqsz > cap) ? next_pow2(reqsz) : cap;
            Region *next  = region_new(ncap);
            if (next == nullptr) {
                if (BITFLAG_ON(self->flags, ARENA_NOTHROW))
                    return nullptr;
                else
                    self->handler("Failed to chain new Region", ncap, self->context);
            }
            it->next = next;
        }
        it = it->next;
    }
    uint8_t *data = it->free;
    it->free     += rawsz + pad;
    return cast(void*, data);
}

static bool get_last_write(Arena *self, void *hint, size sz, Region **out)
{
    for (Region *it = self->begin; it != nullptr; it = it->next) {
        size act    = REGION_ACTIVE(it);
        size cap    = REGION_CAPACITY(it);
        size base_i = act - sz;
        // Base index of last allocation is in range for this Arena?
        if (0 <= base_i && base_i < cap) {
            // Last allocation is exactly the same as `hint`?
            void *ptr = &it->buffer[base_i];
            if (ptr == hint) {
                *out = it;
                return true;
            }
        }
    }
    return false;
}

void *arena_rawrealloc(Arena *self, void *ptr, size oldsz, size newsz, size align)
{
    if (BITFLAG_OFF(self->flags, ARENA_NOSMARTREALLOC)) {
        Region *tgt;
        if (ptr != nullptr && get_last_write(self, ptr, oldsz, &tgt)) {
            size reqsz = newsz - oldsz;
            // Our simple resize can fit in the given Arena?
            if (tgt->free + reqsz < tgt->end) {
                tgt->free += reqsz;
                return ptr;
            }
        }
    }

    // TODO: Implement some sort of rudimentary freeing system?
    if (oldsz >= newsz) {
        return ptr;
    }

    // NOTE: Only receive `nullptr` when `ARENA_NOTHROW` is on.
    void *next = arena_rawalloc(self, newsz, align);
    if (next == nullptr)
        return nullptr;
    return memcpy(next, ptr, oldsz);
}

void arena_print(const Arena *self)
{
    int depth = 1;
    for (const Region *it = self->begin; it != nullptr; ++depth, it = it->next) {
        printf("[%i] " REGION_FMTSTRLN, depth, REGION_FMTARGS(it));
    }
}
