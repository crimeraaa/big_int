#include "arena.hpp"

Arena* arena_new(size_t cap)
{
    // We subtract 1 since `Arena::buffer[1]` is included in `sizeof(Arena)`.
    size_t sz       = sizeof(Arena) + sizeof(Arena::buffer[0]) * (cap - 1);
    Arena* self     = static_cast<Arena*>(std::malloc(sz));
    self->active    = 0;
    self->capacity  = cap;
    self->next      = nullptr;
    return self;
}

void arena_reset(Arena* self)
{
    int depth = 1;
    for (Arena* it = self; it != nullptr; depth++, it = it->next) {
        DEBUG_PRINTFLN("[%i] Resetting " ARENA_FMTSTR, depth, ARENA_FMTARGS(it));
        it->active = 0;
    }
}

void arena_free(Arena*& self)
{
    int    depth = 1;
    Arena* next;
    for (Arena* it = self; it != nullptr; depth++, it = next) {
        // Save now as when `it` is freed, `it->next` will be invalid.
        next = it->next;
        DEBUG_PRINTFLN("[%i] Freeing " ARENA_FMTSTR, depth, ARENA_FMTARGS(it));
        std::free(static_cast<void*>(it));
    }
    self = nullptr;
}

// TODO: Handle alignment
void* internal_alloc(Arena* self, size_t size)
{
    if (size > self->capacity) {
        DEBUG_PRINTFLN("Cannot allocate %zu bytes, capacity = %zu", size, self->capacity);
        return nullptr;
    }
    
    // Find a link which can accomodate our allocation.
    Arena* it = self;
    while (it->active + size > it->capacity) {
        // At end of life? Make a new arena then chain it.
        if (it->next == nullptr)
            it->next = arena_new(self->capacity);
        // Switch to the next in the list.
        it = it->next;
    }

    byte* data  = &it->buffer[it->active];
    it->active += size;
    return static_cast<void*>(data);
}

template<class T = void>
T* arena_alloc(Arena* self, size_t size)
{
    return static_cast<T*>(internal_alloc(self, size));
}

void arena_print(const Arena* self)
{
    int depth = 1;
    for (const Arena* it = self; it != nullptr; it = it->next) {
        std::printf("[%i] " ARENA_FMTSTRLN, depth, ARENA_FMTARGS(it));
        depth += 1;
    }
}

int main()
{
    Arena* a = arena_new(1024);
    arena_print(a);

    void* ptr1 = arena_alloc<>(a, 1);
    printf("ptr1 = %p\n", ptr1);
    arena_print(a);

    void* ptr2 = arena_alloc<>(a, 16);
    printf("ptr2 = %p\n", ptr2);
    arena_print(a);
    
    void* ptr3 = arena_alloc<>(a, 256);
    printf("ptr3 = %p\n", ptr3);
    arena_print(a);
    
    void* ptr4 = arena_alloc<>(a, 1000);
    printf("ptr4 = %p\n", ptr4);
    arena_print(a);
    
    void* ptr5 = arena_alloc<>(a, 123456789);
    printf("ptr5 = %p\n", ptr5);
    arena_print(a);

    arena_reset(a);
    arena_free(a);
    return 0;
}
