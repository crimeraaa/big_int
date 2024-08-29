#define CPP_ODIN_IMPLEMENTATION
#include "cpp_odin.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

static const Allocator generic_heap_allocator{
    /**
     * @brief
     *      The standard C `malloc` family already ensures correct alignment and
     *      does book-keeping for us.
     */
    [](void *ctx, Allocator_Mode mode, Allocator_Proc_Args args) -> Raw_Allocation
    {
        using Error = Allocator_Error;
        using Mode  = Allocator_Mode;

        Error error{Error::None};
        void *ptr{nullptr};
        unused(ctx);

        switch (mode) {
        case Mode::Alloc:
        case Mode::Alloc_Non_Zeroed:
            ptr = std::malloc(args.new_size);
            goto check_allocation;
        case Mode::Resize:
        case Mode::Resize_Non_Zeroed:
            ptr = std::realloc(args.old.data, args.new_size);

            check_allocation: if (!ptr) {
                error = Error::Out_Of_Memory;
                std::fprintf(stderr, "[FATAL]: %s\n", "[Re]allocation failure");
                std::fflush(stderr);
                std::abort();
            }
            break;
        case Mode::Free:
            std::free(args.old.data);
            break;
        }
        
        switch (mode) {
        case Mode::Alloc:
        case Mode::Resize:
            std::memset(ptr, 0, args.new_size);
            break;
        case Mode::Free:
        case Mode::Alloc_Non_Zeroed:
        case Mode::Resize_Non_Zeroed:
            break;
        }
        return {ptr, error};
    },
    // Allocator::context
    nullptr,
};

int main()
{
    Pointer<int> pi = ptr_new<int>(generic_heap_allocator, 4);
    *pi   = 13;
    pi[1] = 14;
    // pi[4] = 19;
    std::printf("pi[0] = %i, pi[1] = %i\n", *pi, pi[1]);
    std::printf("pi = %p, len(pi) = %ti\n", &pi[0], pi.len);
    ptr_resize(generic_heap_allocator, &pi, 10);
    std::printf("pi = %p, len(pi) = %ti\n", &pi[0], pi.len);
    ptr_free(generic_heap_allocator, &pi);
    return 0;
}
