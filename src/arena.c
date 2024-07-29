#define LOG_WANT_IMPLEMENTATION 1
#include "arena.h"
#include "log.h"

Region *region_new(size cap)
{
    Region *r;
    log_tracecall();
    r = cast(Region*, malloc(fam_sizeof(*r, r->buffer[0], cap)));
    if (r) {
        r->next = nullptr;
        r->free = r->buffer;
        r->end  = r->buffer + cap;
    }
    return r;
}

void region_free(Region *r)
{
    log_tracecall();
    // C Standard Library does bookkeeping for us.
    free(cast(void*, r));
}

size region_active(const Region *r)
{
    return r->free - r->buffer;
}

size region_capacity(const Region *r)
{
    return r->end - r->buffer;
}

static void xalloc_test(Arena *scratch)
{
    log_tracecall();
    arena_print(scratch);

    /**
     * VISUALIZATION:
     *      [0x00]  *select0
     */
    println("alloc `char`: len = default(1), extra = default(0)");
    char *select0 = arena_alloc(char, scratch);
    arena_print(scratch);

    /**
     * VISUALIZATION:
     *      [0x00]  *select0
     *      [0x01]  *select1
     */
    println("alloc `char`: len = 1, extra = default(0)");
    char *select1 = arena_alloc(char, scratch, 1);
    arena_print(scratch);

    /**
     * VISUALIZATION:
     *      [0x00]  *select0
     *      [0x01]  *select1
     *      [0x02]  *select2
     *      [0x03]  <padding>
     *      [0x04]  -
     *      [0x05]  -
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
    log_tracecall();
    /**
     * ARENA STATE:
     *      active = 1, capacity = 64
     * VISUALIZATION:
     *      [0x0]   char
     */
    char *cptr = arena_alloc(char, scratch);
    *cptr = 'a';
    printf("cptr = %p, *cptr = '%c'\n", cptr, *cptr);
    arena_print(scratch);

    /**
     * ARENA STATE:
     *      active = 16, capacity = 64
     * VISUALIZATION:
     *      [0x0]   char
     *      [0x1]   <padding>
     *      ...     -
     *      [0x8]   size[0]
     *      ...     -
     *      [0xf]   -
     */
    size *szptr = arena_alloc(size, scratch);
    *szptr = 12;
    printf("szptr = %p, *szptr = %td\n", szptr, *szptr);
    arena_print(scratch);
    
    /**
     * ARENA STATE:
     *      active = 18, capacity = 64
     * VISUALIZATION:
     *      [0x0]   char
     *      [0x1]   <padding>
     *      ...     -
     *      [0x8]   size
     *      ...     -
     *      [0xf]   -
     *      [0x10]  short
     *      ...     -
     *      [0x12]   <inactive>
     */
    short *hptr = arena_alloc(short, scratch);
    *hptr = -16000;
    printf("hptr = %p, *hptr = %hi\n", hptr, *hptr);
    arena_print(scratch);
    
    /**
     * STATE:
     *      active = 26, capacity = 64
     * VISUALIZATION:
     *      [0x0]   char
     *      [0x1]   <padding>
     *      ...     -
     *      [0x8]   size
     *      ...     -
     *      [0x10]  short
     *      [0x11]  -
     *      [0x12]  'H'
     *      [0x13]  'i'
     *      [0x14]  'm'
     *      [0x15]  ' '
     *      [0x16]  'o'
     *      [0x17]  'm'
     *      [0x18]  '!'
     *      [0x19]  '\0'
     *      [0x1a]  <inactive>
     */
    StringView msg  = lstr_literal("Hi mom!");
    char   *cstr = arena_alloc(char, scratch, 8);
    memcpy(cstr, msg.data, msg.length);
    cstr[7] = '\0';
    printf("cstr = %p, cstr = \"%s\"\n", cstr, cstr);
    arena_print(scratch);
    
    /**
     * STATE:
     *      active = 36, capacity = 64
     * VISUALIZATION:
     *      [0x0]   char
     *      [0x1]   <padding>
     *      ...     -
     *      [0x8]   size
     *      ...     -
     *      [0x10]  short
     *      [0x11]  -
     *      [0x12]  "Hi mom!"
     *      ...     -
     *      [0x1a]  <padding>
     *      ...     -
     *      [0x1c]  int[0]
     *      ...     -
     *      [0x20]  int[1]
     *      ...     -
     *      [0x24]  <inactive>
     */
    int *iaptr = arena_alloc(int, scratch, 2);
    iaptr[0] = 943;
    iaptr[1] = -57;
    printf("iaptr = %p, iaptr[0] = %i, iaptr[1] = %i\n", iaptr, iaptr[0], iaptr[1]);
    
    arena_print(scratch);
}

void arena_main(const StringView args[], size count, Arena *arena)
{
    Arena     scratch;
    ArenaArgs init = arena_defaultargs();
    log_tracecall();
    init.flags    ^= ARENA_FALIGN;
    init.capacity  = 128;
    arena_init(&scratch, &init);
    arena_print(&scratch);

    arena_reset(&scratch);
    xalloc_test(&scratch);
    arena_print(&scratch);

    arena_reset(&scratch);
    align_test(&scratch);
    arena_print(&scratch);

    arena_free(&scratch);
    eprintln("===FREE SCRATCH===");
    for (size i = 0; 0 <= i && i < count; i++) {
        printf("args[%td](data=\"%s\", length=%td)\n",
               i, args[i].data, args[i].length);
    }
    printf("\n");
    arena_print(arena);
}

// By default we choose to report the bad allocation and immediately abort.
static void exit_errorfn(Arena *self, const char *msg, size req, void *ctx)
{
    unused(self, ctx);
    eprintfln("[FATAL] %s (requested %td bytes)", msg, req);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

static void arena_throw(Arena *self, const char *msg, size cap)
{
    if (BITFLAG_ON(self->flags, ARENA_FTHROW))
        self->handler(self, msg, cap, self->context);
}

ArenaArgs arena_defaultargs(void)
{
    ArenaArgs args;
    args.handler   = &exit_errorfn;
    args.context   = nullptr;
    args.capacity  = REGION_DEFAULTSIZE;
    args.flags     = ARENA_FDEFAULT;
    return args;
}

bool arena_init(Arena *self, const ArenaArgs *args)
{
    Region *r;
    log_tracecall();
    r = region_new(args->capacity);
    
    // We assume that `init` is never null.
    self->handler = args->handler;
    self->context = args->context;
    self->begin   = r;
    self->end     = r;
    self->flags   = args->flags;

    if (!r) {
        arena_throw(self, "Failed to allocate new Region", args->capacity);
        return false;
    }
    if (BITFLAG_ON(args->flags, ARENA_FZEROINIT))
        memset(r->buffer, 0, array_sizeof(r->buffer[0], args->capacity));
    return true;
}

void arena_set_flags(Arena *self, u8 flags)
{
    self->flags |= flags & ARENA_FZEROINIT;
    self->flags |= flags & ARENA_FTHROW;
    self->flags |= flags & ARENA_FALIGN;
    self->flags |= flags & ARENA_FSMARTREALLOC;
}

void arena_clear_flags(Arena *self, u8 flags)
{
    self->flags ^= flags & ARENA_FZEROINIT;
    self->flags ^= flags & ARENA_FTHROW;
    self->flags ^= flags & ARENA_FALIGN;
    self->flags ^= flags & ARENA_FSMARTREALLOC;
}

size arena_active(const Arena *self)
{
    size n = 0;
    for (const Region *it = self->begin; it != nullptr; it = it->next) {
        n += region_active(it);
    }
    return n;
}

size arena_capacity(const Arena *self)
{
    size cap = 0;
    for (const Region *it = self->begin; it != nullptr; it = it->next) {
        cap += region_capacity(it);
    }
    return cap;
}

void arena_reset(Arena *self)
{
    log_tracecall();
    for (Region *it = self->begin; it != nullptr; it = it->next) {
        it->free = it->buffer;
    }
}

void arena_free(Arena *self)
{
    int depth = 1;
    log_tracecall();
    log_debugf("active=%td, capacity=%td", arena_active(self), arena_capacity(self));
    for (Region *it = self->begin, *next;
         it != nullptr;
         it = next)
    {
        log_debugf("[%i] Free " REGION_FMTSTR, depth++, REGION_FMTARGS(it));
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

// https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding
static size get_padding(size align, size offset)
{
    return align - (offset % align) % align;
}

void *arena_rawalloc(Arena *self, size rawsz, size align)
{
    // Try to find or chain a new arena that can accomodate our allocation.
    Region *it  = self->begin;
    size    pad = 0;
    
    for (;;) {
        size active = region_active(it);
        size cap    = region_capacity(it);
        if (BITFLAG_ON(self->flags, ARENA_FALIGN))
            pad = get_padding(align, active);
        // Requested size will be in range of the Region?
        if (active + rawsz + pad <= cap)
            break;
        // Need to chain a new region?
        if (!it->next) {
            size    reqsz = rawsz + pad;
            size    ncap  = (reqsz > cap) ? next_pow2(reqsz) : cap;
            Region *next  = region_new(ncap);
            if (!next) {
                arena_throw(self, "Failed to chain new Region", ncap);
                return nullptr;
            }
            it->next = next;
        }
        it = it->next;
    }
    byte *data = it->free;
    it->free  += rawsz + pad;
    return cast(void*, data);
}

static Region *get_owning_region(Arena *self, void *hint, size sz)
{
    if (!hint)
        return nullptr;
    for (Region *it = self->begin; it != nullptr; it = it->next) {
        size base_i = region_active(it) - sz;
        // Base index of last allocation is in range for this Arena?
        if (0 <= base_i && base_i < region_capacity(it)) {
            // Last allocation is exactly the same as `hint`?
            if (&it->buffer[base_i] == hint)
                return it;
        }
    }
    return nullptr;
}

void *arena_rawrealloc(Arena *self, void *ptr, size oldsz, size newsz, size align)
{
    if (BITFLAG_ON(self->flags, ARENA_FSMARTREALLOC)) {
        Region *dst = get_owning_region(self, ptr, oldsz);
        if (dst) {
            size reqsz = newsz - oldsz;
            // Our simple resize can fit in the given Arena?
            if (dst->free + reqsz < dst->end) {
                dst->free += reqsz;
                return ptr;
            }
        }
    }

    // TODO: Implement some sort of rudimentary freeing system?
    if (oldsz >= newsz) {
        return ptr;
    }

    // NOTE: Only receive `nullptr` when `ARENA_FTHROW` is on.
    void *next = arena_rawalloc(self, newsz, align);
    if (!next)
        return nullptr;
    return memcpy(next, ptr, oldsz);
}

void arena_print(const Arena *self)
{
    int depth = 1;
    for (const Region *it = self->begin; it != nullptr; ++depth, it = it->next) {
        printfln("[%i] " REGION_FMTSTR, depth, REGION_FMTARGS(it));
    }
}
