#pragma once

/// local
#include "common.h"

#define REGION_FMTSTR       "Region(active=%td, capacity=%td)"
#define REGION_FMTARGS(r)   region_active(r), region_capacity(r)
#define REGION_DEFAULTSIZE  (1024 * 8)

/**
 * OVERVIEW:
 *     Bit flags allow us to combine multiple boolean flags into a single unsigned
 *     integer.
 *
 * ENABLING:
 *     To enable particular flags, simply bitwise OR all the flags you'd like, e.g:
 * 
 *     u8 myflags = ARENA_FZEROINIT | ARENA_FTHROW;
 *     myflags |= ARENA_FGROW;
 *
 * DISABLING: 
 *     Conversely, to disable particular flags, use `ARENA_FDEFAULT' as a bitmask,
 *     then bitwise XOR the masks you don't want, e.g:
 *     
 *     u8 myflags = ARENA_FDEFAULT ^ ARENA_FALIGN ^ ARENA_FSMARTREALLOC;
 *     myflags ^= ARENA_FGROW;
 *     
 * CHECKING:
 *      To check if a given bitflag is enabled use the bitwise AND.
 *  
 *      u8 test = ARENA_FDEFAULT;
 *      if (test & ARENA_FALIGN)
 *          printf("Alignment is enabled!\n");
 *      else
 *          printf("Alignment is not enabled.\n");
 *      
 *      To check if a given bitflag is NOT enabled, you have 2 options.
 *      1.) Simply logical NOT the result of the bitwise AND.
 *      
 *      u8 test = ARENA_FNODEFAULT;
 *      if (!(test & ARENA_FALIGN))
 *          printf("Alignment is not enabled!\n");
 *      else
 *          printf("Alignment is enabled!\n");
 *          
 *      2.) Bitwise XOR the result of the bitwise AND with the flag.
 * 
 *      u8 test = ARENA_FNODEFAULT;
 *      if ((test & ARENA_FALIGN) ^ ARENA_FALIGN)
 *          printf("Alignment is not enabled.\n");
 *      else
 *          printf("Alignment is enabled!\n");
 * 
 */
enum {
    ARENA_FNODEFAULT    = 0x0, // All succeeding bit flags unset.
    ARENA_FZEROINIT     = 0x1, // Memset Region buffer to 0 on init?
    ARENA_FGROW         = 0x2, // Allow Region to grow if allocation too large?
    ARENA_FTHROW        = 0x4, // Use error handler on malloc error?
    ARENA_FALIGN        = 0x8, // Align the returned pointers?
    ARENA_FSMARTREALLOC = 0x10, // Try to find last allocation when realloc'ing?
    ARENA_FDEFAULT      = (ARENA_FZEROINIT | ARENA_FGROW | ARENA_FTHROW 
                          | ARENA_FALIGN   | ARENA_FSMARTREALLOC),
};

#define BITFLAG_ON(n, flag)     ((n) & (flag))
#define BITFLAG_OFF(n, flag)    ((n) & (flag) ^ (flag))

// https://github.com/tsoding/arena/blob/master/arena.h
typedef struct Region Region;
typedef struct Arena  Arena;
typedef void (*ErrorFn)(Arena *self, const char *msg, size sz, void *ctx);

struct Region {
    Region *next;     // Regions can link to each other.
    byte   *free;     // Points to the first free slot.
    byte   *end;      // &buffer[capacity]. NOT dereferencable!
    byte    buffer[]; // C99 Flexible array member. NOT valid in standard C++!
};

struct Arena {
    ErrorFn handler;
    void   *context;  // The 3rd argument to `handler`.
    Region *begin;
    Region *end;
    u8      flags;
};

typedef struct ArenaArgs {
    ErrorFn handler;
    void   *context;
    size    capacity;
    u8      flags;
} ArenaArgs;

Region *region_new(size cap);
void    region_free(Region *r);
size    region_active(const Region *r);
size    region_capacity(const Region *r);

ArenaArgs arena_defaultargs(void);
ArenaArgs arena_nodefaultargs(void);

bool    arena_init(Arena *self, const ArenaArgs *args);
void    arena_set_flags(Arena *self, u8 flags);
void    arena_clear_flags(Arena *self, u8 flags);
void    arena_reset(Arena *self);
void    arena_free(Arena *self);
size    arena_active(const Arena *self);
size    arena_capacity(const Arena *self);
void    arena_print(const Arena *self);
void   *arena_rawalloc(Arena *self, size rawsz, size align);
void   *arena_rawrealloc(Arena *self, void *ptr, size oldsz, size newsz, size align);
void    arena_main(const StringView args[], size count, Arena *arena);

#define arena__xrawalloc(T, a, sz)  cast(T*, arena_rawalloc(a, sz, alignof(T)))
#define arena__xalloc4(T, a, n, ex) arena__xrawalloc(T, a, sizeof(T) * (n) + (ex))
#define arena__xalloc3(T, a, n)     arena__xalloc4(T, a, n, 0)
#define arena__xalloc2(T, a)        arena__xalloc3(T, a, 1)

// ... = self [, len [, extra] ]
#define arena_alloc(T, ...)                                                    \
    x__xselect4(                                                               \
        __VA_ARGS__,                                                           \
        arena__xalloc4,                                                        \
        arena__xalloc3,                                                        \
        arena__xalloc2                                                         \
    )(T, __VA_ARGS__)

#define arena__xrawrealloc(T, arena, ptr, oldsz, newsz) \
    cast(T*, arena_rawrealloc(arena, ptr, oldsz, newsz, alignof(T)))

#define arena_realloc_extra(T, arena, ptr, oldlen, newlen, extra)              \
    arena__xrawrealloc(                                                        \
        T,                                                                     \
        arena,                                                                 \
        ptr,                                                                   \
        sizeof(T) * (oldlen) + (extra),                                        \
        sizeof(T) * (newlen) + (extra)                                         \
    )

#define arena_realloc(T, arena, ptr, oldlen, newlen) \
    arena_realloc_extra(T, arena, ptr, oldlen, newlen, 0)

// Assumes `da` has members `data`, `length` and `capacity`.
// Inspired by: https://nullprogram.com/blog/2023/10/05/
// and: https://github.com/tsoding/arena/blob/master/arena.h#L89
#define dynarray_push(T, da, val, arena)                                       \
do {                                                                           \
    if ((da)->length >= (da)->capacity) {                                      \
        size oldn_     = (da)->capacity;                                       \
        size newn_     = (oldn_ < 8) ? 8 : oldn_ * 2;                          \
        (da)->data     = arena_realloc(T, arena, (da)->data, oldn_, newn_);    \
        (da)->capacity = newn_;                                                \
    }                                                                          \
    (da)->data[(da)->length++] = (val);                                        \
} while (false)

#ifdef ARENA_INCLUDE_IMPLEMENTATION

/// local
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
    StringView msg  = sv_literal("Hi mom!");
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
    log_fatalf("%s (requested %td bytes)", msg, req);
    fflush(stderr);
    abort();
}

static void *arena_throw(Arena *self, const char *msg, size cap)
{
    if (BITFLAG_ON(self->flags, ARENA_FTHROW))
        self->handler(self, msg, cap, self->context);
    return nullptr;
}

static const ArenaArgs ARENA_DEFAULTARGS = {
    /* handler  */  &exit_errorfn,
    /* context  */  nullptr,
    /* capacity */  REGION_DEFAULTSIZE,
    /* flags    */  ARENA_FDEFAULT,
};

static const ArenaArgs ARENA_NODEFAULTARGS = {
    /* handler  */  nullptr,
    /* context  */  nullptr,
    /* capacity */  0,
    /* flags    */  ARENA_FNODEFAULT,
};

ArenaArgs arena_defaultargs(void)
{
    return ARENA_DEFAULTARGS;
}

ArenaArgs arena_nodefaultargs(void)
{
    return ARENA_NODEFAULTARGS;
}

bool arena_init(Arena *self, const ArenaArgs *args)
{
    Region *r;
    log_tracecall();
    if (!args)
        args = &ARENA_DEFAULTARGS;
    
    r = region_new(args->capacity);
    self->handler = args->handler;
    self->context = args->context;
    self->begin   = r;
    self->end     = r;
    self->flags   = args->flags;

    if (!r)
        return arena_throw(self, "Failed to allocate new Region", args->capacity);
    if (BITFLAG_ON(args->flags, ARENA_FZEROINIT))
        memset(r->buffer, 0, array_sizeof(r->buffer[0], args->capacity));
    return true;
}

void arena_set_flags(Arena *self, u8 flags)
{
    self->flags |= flags & ARENA_FZEROINIT;
    self->flags |= flags & ARENA_FGROW;
    self->flags |= flags & ARENA_FTHROW;
    self->flags |= flags & ARENA_FALIGN;
    self->flags |= flags & ARENA_FSMARTREALLOC;
}

void arena_clear_flags(Arena *self, u8 flags)
{
    self->flags ^= flags & ARENA_FZEROINIT;
    self->flags ^= flags & ARENA_FGROW;
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
    // log_tracecall();
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
            size    ncap;
            Region *next;
            if (reqsz > cap) {
                ncap = next_pow2(reqsz);
                if (BITFLAG_OFF(self->flags, ARENA_FGROW))
                    return arena_throw(self, "Cannot grow Arena", ncap);
            } else {
                ncap = cap;
            }
            next = region_new(ncap);
            if (!next)
                return arena_throw(self, "Failed to chain new Region", ncap);
            it->next = next;
        }
        it = it->next;
    }
    byte *data = it->free;
    it->free  += rawsz + pad;
    return cast(void*, data);
}

static Region *get_owning_region_of_last_alloc(Arena *self, void *hint, size sz)
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
        Region *dst = get_owning_region_of_last_alloc(self, ptr, oldsz);
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

#endif  // ARENA_INCLUDE_IMPLEMENTATION
