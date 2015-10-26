#include <GeneralBuffer.h>

#include <stdlib.h>
#include <stdio.h>


int main(void)
{
	GBuffer my_buffer_double = GB_initDouble(10);
	double v = 1.0;
	int i;

	for(i = 0; i < 10; ++i) {
		GB_setValue(my_buffer_double, i, &v);
		v += 1.0;
	}

	GB_print(my_buffer_double, NULL);

	double extracted;

	fprintf(stderr, "Extraction return value: %d\n", GB_getValue(my_buffer_double, 8, &extracted));

	fprintf(stderr, "Got value %f at slot %d\n", extracted, 8);

	fprintf(stderr, "GB_isSet(%d) returns: %d\n", 3, GB_isSet(my_buffer_double, 3));

	fprintf(stderr, "GB_unset(%d) returns: %d\n", 3, GB_unset(my_buffer_double, 3));

	fprintf(stderr, "GB_isSet(%d) returns: %d\n", 3, GB_isSet(my_buffer_double, 3));

	GB_destroy(my_buffer_double);

	return 0;
}

