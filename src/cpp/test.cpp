#define PSEUDO_ODIN_IMPLEMENTATION
#include "cpp_odin.hpp"

#include <cstdio>

int main()
{
    String s = string_literal("Hi mom!");
    std::printf("s = %s (len=%ti)\n", cstring(s), len(s));
    return 0;
}
