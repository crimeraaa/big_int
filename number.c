#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <float.h>

#define printfln(fmt, ...)  printf(fmt "\n", __VA_ARGS__)

int main(void)
{
    double max_int_in_double = pow(2, DBL_MANT_DIG);
    printfln("%%f   = %f", max_int_in_double);
    printfln("%%g   = %g", max_int_in_double);
    printfln("%%.0f = %.0f", max_int_in_double);
    printfln("%%.*f = %.*f", DBL_DECIMAL_DIG, max_int_in_double);
    return 0;
}
