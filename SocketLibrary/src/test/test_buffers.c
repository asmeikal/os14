#include <Buffers.h>

#include <stdlib.h>
#include <stdio.h>


int main(void)
{
    GBuffer my_buffer_double = GB_initDouble(10);
    double v = 1.0;
    int i;

    for(i = 0; i < 10; ++i) {
        GB_setDoubleValue(my_buffer_double, i, v);
        v += 1.0;
    }

    GB_print(my_buffer_double);

    fprintf(stderr, "Got value %f at slot %d\n", GB_getDoubleValue(my_buffer_double, 8), 8);

    /* This crashes, as it should */
    // GB_getDoubleValue(my_buffer_double, 10);

    return 0;
}

