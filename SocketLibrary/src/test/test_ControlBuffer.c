#include <ControlBuffer.h>
#include <GeneralBuffer.h>

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	ControlBuffer my_control_buffer = CB_init(10);

	GBuffer my_buffer = CB_getBuffer(my_control_buffer);

	CB_print(my_control_buffer);

	int32_t i = 12;

	CB_setControl(my_control_buffer, &i);

	i = 45;

	double v = 15.0;

	GB_setValue(my_buffer, 1, &v);

	CB_getControl(my_control_buffer, &i);
	printf("Control extracted is %ld\n", i);
	CB_print(my_control_buffer);

	CB_destroy(my_control_buffer);

	return 0;
}

