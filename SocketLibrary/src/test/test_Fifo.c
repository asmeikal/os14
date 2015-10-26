#include <Fifo.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void myPrint(void *obj)
{
	fprintf(stderr, "My object is the integer '%d'.\n", *((int *) obj));
}

int main(void)
{
	
	FIFO f = fifo_init();
	assert(NULL != f);

	int *i;
	i = malloc(sizeof *i);
	*i = 12;

	fprintf(stderr, "Insert return value is %d.\n", fifo_insert(f, i));

	fifo_print(f, myPrint);

	int *rm = fifo_peek(f);

	fprintf(stderr, "Top of fifo is:\n");
	myPrint(rm);

	rm = fifo_pop(f);
	fprintf(stderr, "Top of fifo was:\n");
	myPrint(rm);
	free(rm);

	int j;

	for(j = 0; j < 100; ++j) {
		i = malloc(sizeof *i);
		assert(NULL != i);
		*i = j;
		if(fifo_insert(f, i)) {
			free(i);
			fprintf(stderr, "Unable to insert item %d.\n", j);
		}
	}

	fifo_print(f, myPrint);

	while(NULL != (rm = fifo_pop(f))) {
		fprintf(stderr, "Removed item:\n");
		myPrint(rm);
		free(rm);
	}

	fifo_print(f, myPrint);

	fifo_destroy(f);

	return 0;
}

