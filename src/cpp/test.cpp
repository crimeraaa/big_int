#define CPP_ODIN_IMPLEMENTATION
#include "cpp_odin.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

static const Allocator heap_allocator{
    /**
     * @brief
     *      The standard C `malloc` family already ensures correct alignment and
     *      does book-keeping for us.
     */
    [](const Allocator_Proc_Args &args, Allocator_Error *e) -> rawptr
    {
        using Error = Allocator_Error;
        using Mode  = Allocator_Mode;

        rawptr ptr{nullptr};
        if (e) {
            *e = Error::None;
        }
        switch (args.mode) {
        case Mode::Alloc:
        case Mode::Alloc_Non_Zeroed:
            ptr = std::malloc(args.new_size);
            goto check_allocation;
        case Mode::Resize:
        case Mode::Resize_Non_Zeroed:
            ptr = std::realloc(args.memory.data, args.new_size);

            check_allocation:
            if (!ptr) {
                // User is interested in handling the error themselves?
                if (e) {
                    *e = Error::Out_Of_Memory;
                } else {
                    std::fprintf(stderr, "[FATAL]: %s\n", "[Re]allocation failure");
                    std::fflush(stderr);
                    std::abort();
                }
            }
            break;
        case Mode::Free:
            std::free(args.memory.data);
            break;
        }
        
        switch (args.mode) {
        case Mode::Alloc:
        case Mode::Resize:
            std::memset(ptr, 0, args.new_size);
            break;
        case Mode::Free:
        case Mode::Alloc_Non_Zeroed:
        case Mode::Resize_Non_Zeroed:
            break;
        }
        return ptr;
    },
    // Allocator::context
    nullptr,
};

int main()
{
    Pointer<int> ptr_i = ptr_new<int>(heap_allocator, 4);

    *ptr_i   = 13;
    ptr_i[1] = 14;

    std::printf("ptr_i[0] = %i, ptr_i[1] = %i\n", *ptr_i, ptr_i[1]);
    std::printf("ptr_i = %p, len(ptr_i) = %ti\n", &ptr_i[0], len(ptr_i));
    ptr_resize(heap_allocator, &ptr_i, 10);
    std::printf("ptr_i = %p, len(ptr_i) = %ti\n", &ptr_i[0], len(ptr_i));
    ptr_free(heap_allocator, &ptr_i);
    return 0;
}
