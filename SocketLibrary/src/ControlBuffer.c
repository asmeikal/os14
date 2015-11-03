#include <ControlBuffer.h>

#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>

enum _CB_return_values {
	_CB_SUCCESS = 0,
	_CB_FAILED,
	_CB_INVALID
};

struct control_buffer {
	int32_t control;
	GBuffer buffer;
};

static int CB_check(ControlBuffer b, const char * const fname);

ControlBuffer CB_init(const int size)
{
	if (0 >= size) {
		DEBUG_PRINT("CB_init: illegal size %d.\n", size);
		return NULL;
	}

	struct control_buffer *ret = calloc(1, sizeof(*ret));
	if (NULL == ret) {
		DEBUG_PRINT("CB_init: calloc failed.\n");
		return NULL;
	}

	ret->buffer = GB_initDouble(size);
	if (NULL == ret->buffer) {
		DEBUG_PRINT("CB_init: GBuffer init failed.\n");
		free(ret);
		return NULL;
	}
	ret->control = 0;

	return ret;
}

int CB_getControl(ControlBuffer b, int32_t *res)
{
	if (_CB_SUCCESS != CB_check(b, "CB_getControl")) {
		return -_CB_FAILED;
	}

	if (NULL == res) {
		DEBUG_PRINT("CB_getControl: NULL pointer argument.\n");
		return -_CB_FAILED;
	}

	*res = b->control;

	return _CB_SUCCESS;
}

int CB_setControl(ControlBuffer b, const int32_t * const val)
{
	if (_CB_SUCCESS != CB_check(b, "CB_getControl")) {
		return -_CB_FAILED;
	}

	if (NULL == val) {
		DEBUG_PRINT("CB_getControl: NULL pointer argument.\n");
		return -_CB_FAILED;
	}

	b->control = *val;

	return _CB_SUCCESS;

}

int CB_destroy(ControlBuffer b)
{
	if (_CB_SUCCESS != CB_check(b, "CB_getBuffer")) {
		return -_CB_FAILED;
	}

	if (0 != GB_destroy(b->buffer)) {
		return -_CB_FAILED;
	}
	free(b);
	return _CB_SUCCESS;
}

GBuffer CB_getBuffer(ControlBuffer b)
{
	if (_CB_SUCCESS != CB_check(b, "CB_getBuffer")) {
		return NULL;
	}

	return b->buffer;
}

static int CB_check(ControlBuffer b, const char * const fname)
{
	if (NULL == b) {
		DEBUG_PRINT("%s: NULL pointer argument.\n", fname);
		return -_CB_FAILED;
	}
	if (NULL == b->buffer) {
		DEBUG_PRINT("%s: NULL buffer pointer.\n", fname);
		return -_CB_FAILED;
	}
	return _CB_SUCCESS;
}

void CB_print(ControlBuffer b)
{
	if (_CB_SUCCESS != CB_check(b, "CB_print")) {
		return;
	}

	DEBUG_PRINT("Control is %d\n", b->control);
	GB_print(b->buffer, NULL);
}


