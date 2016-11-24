#include <stdlib.h>
#include <stdarg.h>

/* Some math help here */
/*
  Write our own integer exponentiation function - the power
  functions in math.h only handle doubles and floats.
*/
long power(long base, long exp) {
    if (exp == 0)
        return 1;
    else if (exp % 2)
        return base * power(base, exp - 1);
    else {
        long temp = power(base, exp / 2);
        return temp * temp;
    }
}

/* WIP - need to think this through
long min(int count,...) {
    va_list ap;
}

*/
