#define CPP_ODIN_IMPLEMENTATION
#include "cpp_odin.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

using namespace odin;

static const mem::Allocator heap_allocator{
    /**
     * @brief
     *      The standard C `malloc` family already ensures correct alignment and
     *      does book-keeping for us.
     */
    [](const mem::Allocator_Proc_Args &args, mem::Allocator_Error *e) -> rawptr
    {
        using Error = mem::Allocator_Error;
        using Mode  = mem::Allocator_Mode;

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
    // odin::mem::Allocator::data
    nullptr,
};

int main()
{
    Pointer<int> ptr   = mem::new_ptr<int>(heap_allocator);
    Slice<int>   array = mem::make_slice<int>(heap_allocator, 4);
    
    *ptr = 13;
    // ptr[0] = 14; // error: overload resolution selected deleted operator
    std::printf("*ptr = %i\n", *ptr);
    
    // *array = 13; // expected: compile error
    array[0] = 8;
    array[1] = 5;
    array[2] = 13;
    array[3] = 21;
    // array[4] = 32; // error: abort when debug else UB

    for (isize i = 0; i < len(array); i++) {
        std::printf("array[%ti] = %i\n", i, array[i]);
    }
    
    mem::free_ptr(heap_allocator, ptr);
    mem::delete_slice(heap_allocator, array);
    return 0;
}
