
#include <Fifo.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>

/****************************************
* Defines
****************************************/

#define _FIFO_SUCCESS	0
#define _FIFO_INVALID	-1
#define _FIFO_FAILED	-2

/****************************************
* Local functions declaration
****************************************/

static int _fifo_check(FIFO f, const char * const fname);

/****************************************
* Local structs definition
****************************************/

/* This holds the whole FIFO */
struct _fifo {
	struct _fifoNode *first;
};

/* This holds a single item in the FIFO */
struct _fifoNode {
	struct _fifoNode *next;
	void *item;
};

/****************************************
* Function definition
****************************************/

/**
 * Creates an empty FIFO.
 * Returns NULL on failure.
 */
FIFO fifo_init(void)
{
	struct _fifo *ret = (struct _fifo *) calloc(1, sizeof *ret);
	if(NULL == ret) {
		DEBUG_PRINT("fifo_init: calloc failed.\n");
		return NULL;
	}

	ret->first = NULL;

	return ret;
}

/**
 * Frees a FIFO.
 * The FIFO should be manually emptied before destroying it, to avoid memory leaks.
 */
void fifo_destroy(FIFO f)
{
	if(_FIFO_SUCCESS != _fifo_check(f, "fifo_destroy")) {
		return;
	}

	if(NULL != f->first) {
		DEBUG_PRINT("fifo_destroy: deleting non empty fifo.\n");
	}

	free(f);
}

/**
 * Inserts an element in the FIFO. 
 * Returns 0 on success, non zero on failure.
 */
int fifo_insert(FIFO f, void *item)
{
	if(_FIFO_SUCCESS != _fifo_check(f, "fifo_insert")) {
		return _FIFO_FAILED;
	}

	struct _fifoNode *new = (struct _fifoNode *) calloc(1, sizeof *new);
	if(NULL == new) {
		DEBUG_PRINT("fifo_insert: calloc failed.\n");
		return _FIFO_FAILED;
	}
	
	new->item = item;
	new->next = NULL;

	if(NULL == f->first) {
		f->first = new;
		return _FIFO_SUCCESS;
	}
	
	struct _fifoNode *last = f->first;
	while(NULL != last->next) {
		last = last->next;
	}
	last->next = new;

	return _FIFO_SUCCESS;
}

/**
 * Returns the first element of the FIFO.
 * The element should not be freed while it's still in the FIFO.
 * On error, returns a NULL pointer.
 */
void *fifo_peek(FIFO f)
{
	if(_FIFO_SUCCESS != _fifo_check(f, "fifo_peek")) {
		return NULL;
	}

	if(NULL == f->first) {
		return NULL;
	}
	else {
		return f->first->item;
	}
}

/**
 * Removes the first element of the FIFO.
 * The element should then be manually freed.
 * On error, returns a NULL pointer.
 */
void *fifo_pop(FIFO f)
{
	if(_FIFO_SUCCESS != _fifo_check(f, "fifo_pop")) {
		return NULL;
	}

	struct _fifoNode *top = f->first;
	if(NULL != top) {
		f->first = top->next;
		void *res = top->item;
		free(top);
		return res;
	}
	return NULL;
}

/**
 * Prints the contents of the FIFO, using the printFunction 
 * callback.
 */
void fifo_print(FIFO f, void (*printFunction)(void *))
{
	if(_FIFO_SUCCESS != _fifo_check(f, "fifo_print")) {
		return;
	}

	DEBUG_PRINT("Printing FIFO.\n");
	struct _fifoNode *current = f->first;
	int i = 0;
	while(NULL != current) {
		++i;
		DEBUG_PRINT("Printing item %d at position %p:\n", i, current->item);
		printFunction(current->item);
		current = current->next;
	}
	if(0 == i) {
		DEBUG_PRINT("FIFO is empty.\n");
	}
	DEBUG_PRINT("\n");
}

/****************************************
* Local utility functions definition
****************************************/

/**
 * Checks if the FIFO is in fact a FIFO.
 */
static int _fifo_check(FIFO f, const char * const fname)
{
	if(NULL == f) {
		DEBUG_PRINT("%s: NULL pointer argument.\n", fname);
		return _FIFO_INVALID;
	}
	return _FIFO_SUCCESS;
}




