#include <GeneralBuffer.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/************************************************************
* Enum definition
************************************************************/

#define _GB_SUCCESS		0
#define _GB_FAILED		-1
#define _GB_INVALID		-2

enum _GB_status {
	_GB_STATUS_EMPTY = 0,
	_GB_STATUS_READY,
	_GB_STATUS_DELIVERED,
	_GB_STATUS_NUMBER
};

enum _GB_type {
	_GB_DOUBLE = 0,
	_GB_LONG_INT,
	_GB_TYPE_NUMBER
};

/************************************************************
* Types, structs, and unions
************************************************************/

union _GB_value {
	long int int_value;
	double double_value;
};

struct _GB_node {
	enum _GB_status status;
	union _GB_value value;
};

struct _GB_head {
	enum _GB_type type;
	int size;
	struct _GB_node *head;
};

/************************************************************
* Local functions declaration
************************************************************/

static GBuffer GB_init(const int size, const enum _GB_type type);
static int GB_check(GBuffer b, const char * const fname);

/************************************************************
* Initialization functions
************************************************************/

/**
 * Returns a pointer to a new GeneralBuffer<Double>
 */
GBuffer GB_initDouble(const int size)
{
	return GB_init(size, _GB_DOUBLE);
}

/**
 * Returns a pointer to a new GeneralBuffer<LongInt>
 */
GBuffer GB_initLongInt(const int size)
{
	return GB_init(size, _GB_LONG_INT);
}

/**
 * Returns a pointer to a new (and empty) GeneralBuffer<type>
 */
static GBuffer GB_init(const int size, const enum _GB_type type)
{
	if(_GB_TYPE_NUMBER <= type) {
		DEBUG_PRINT("GB_init: type not recognized.\n");
		return NULL;
	}

	if(0 >= size) {
		DEBUG_PRINT("GB_init: size %d is too small.\n", size);
		return NULL;
	}

	struct _GB_head *ret = (struct _GB_head *) calloc(1, sizeof *ret);
	if(NULL == ret) {
		DEBUG_PRINT("GB_init: calloc failed.\n");
		return NULL;
	}

	ret->head = (struct _GB_node *) calloc(size, sizeof (struct _GB_head));
	if(NULL == ret->head) {
		free(ret);
		DEBUG_PRINT("GB_init: calloc failed.\n");
		return NULL;
	}

	ret->type = type;
	ret->size = size;

	GB_empty(ret);

	return ret;
}

int GB_destroy(GBuffer b)
{
	if(_GB_SUCCESS != GB_check(b, "GB_destroy")) {
		return _GB_INVALID;
	}

	free(b->head);

	free(b);

	return _GB_SUCCESS;
}

/************************************************************
* Getters
************************************************************/

/**
 * Gets the double value at index [i]. Throws an error if
 * the value is not set, the buffer is not a double buffer,
 * or the index is out of range.
 */
int GB_getValue(GBuffer b, const int i, const void *ret)
{
	if(_GB_SUCCESS != GB_check(b, "GB_getValue")) {
		return _GB_INVALID;
	}

	if(i >= b->size) {
		DEBUG_PRINT("GB_getValue: index %d out of buffer range %d.\n", i, b->size);
		return _GB_FAILED;
	}

	if(_GB_STATUS_EMPTY == b->head[i].status) {
		DEBUG_PRINT("GB_getValue: buffer[%d] is not set.\n", i);
		return _GB_FAILED;
	}

	switch(b->type) {
		case _GB_DOUBLE:
			*((double *) ret) = b->head[i].value.double_value;
			break;
		case _GB_LONG_INT:
			*((long int *) ret) = b->head[i].value.int_value;
			break;
		default:
			DEBUG_PRINT("GB_getValue: buffer type not recognized, this should not have happened.\n");
			return _GB_INVALID;
	}

	return _GB_SUCCESS;
}

///**
// * Gets the long int value at index [i]. Throws an error if
// * the value is not set, the buffer is not a long int buffer,
// * or the index is out of range.
// */
//long int GB_getLongIntValue(GBuffer b, const int i)
//{
//	if(NULL == b) {
//		DEBUG_PRINT("GB_getLongIntValue: NULL pointer argument.\n");
//	}
//
//	if(_GB_LONG_INT != b->type) {
//		DEBUG_PRINT("GB_getLongIntValue: type mismatch.\n");
//	}
//
//	if(i >= b->size) {
//		DEBUG_PRINT("GB_getLongIntValue: index %d out of buffer range %d.\n", i, b->size);
//	}
//
//	if(_GB_STATUS_EMPTY == b->head[i].status) {
//		DEBUG_PRINT("GB_getLongIntValue: buffer[%d] is not set.\n", i);
//	}
//
//	return b->head[i].value.int_value;
//}

/************************************************************
* Setters
************************************************************/

/**
 * Sets the double value at index [i] to [v]. Throws an error if
 * the buffer is not a double buffer or the index is out of range.
 * Gives a warning if the index was not empty.
 */
int GB_setValue(GBuffer b, const int i, const void * const  val)
{
	if(_GB_SUCCESS != GB_check(b, "GB_setValue")) {
		return _GB_INVALID;
	}

	if(i >= b->size) {
		DEBUG_PRINT("GB_setValue: index %d out of buffer range %d.\n", i, b->size);
		return _GB_FAILED;
	}

	switch(b->type) {
		case _GB_DOUBLE:
			if(_GB_STATUS_EMPTY != b->head[i].status) {
				DEBUG_PRINT("GB_setValue: buffer[%d] was already set to %e.\n", i, b->head[i].value.double_value);
			}
			b->head[i].value.double_value = *(double *) val;
			break;
		case _GB_LONG_INT:
			if(_GB_STATUS_EMPTY != b->head[i].status) {
				DEBUG_PRINT("GB_setValue: buffer[%d] was already set to %ld.\n", i, b->head[i].value.int_value);
			}
			b->head[i].value.int_value = *(long int *) val;
			break;
		default:
			DEBUG_PRINT("GB_setValue: buffer type not recognized, this should not have happened.\n");
			return _GB_INVALID;
	}

	b->head[i].status = _GB_STATUS_READY;
	return _GB_SUCCESS;
}

/**
 * Sets the long int value at index [i] to [v]. Throws an error if
 * the buffer is not a long int buffer or the index is out of range.
 * Gives a warning if the index was not empty.
 */
//void GB_setLongIntValue(GBuffer b, const int i, const long int v)
//{
//	if(NULL == b) {
//		DEBUG_PRINT("GB_setLongIntValue: NULL pointer argument.\n");
//	}
//
//	if(_GB_LONG_INT != b->type) {
//		DEBUG_PRINT("GB_setLongIntValue: type mismatch.\n");
//	}
//
//	if(i >= b->size) {
//		DEBUG_PRINT("GB_setLongIntValue: index %d out of buffer range %d.\n", i, b->size);
//	}
//
//	if(_GB_STATUS_EMPTY != b->head[i].status) {
//		WARNING("GB_setLongIntValue: buffer[%d] was already set to %ld.\n", i, b->head[i].value.int_value);
//	}
//
//	b->head[i].value.int_value = v;
//	b->head[i].status = _GB_STATUS_READY;
//}

/************************************************************
* Empty buffer utiliy function
************************************************************/

/**
 * Empties the buffer, i.e. marks all indexes 0..<b->size as empty.
 */
int GB_empty(GBuffer b)
{
	if(_GB_SUCCESS != GB_check(b, "GB_empty")) {
		return _GB_INVALID;
	}

	int i;
	for(i = 0; i < b->size; ++i) {
		GB_unset(b, i);
	}
	return _GB_SUCCESS;
}

/************************************************************
* Buffer status single element set/unset/deliver functions
************************************************************/

/**
 * Marks the index [i] as set. Gives a warning if the index was
 * already set.
 */
int GB_set(GBuffer b, const int i)
{
	if(_GB_SUCCESS != GB_check(b, "GB_set")) {
		return _GB_INVALID;
	}

	if(i >= b->size) {
		DEBUG_PRINT("GB_set: index %d out of buffer range %d.\n", i, b->size);
		return _GB_FAILED;
	}

	if(_GB_STATUS_EMPTY != b->head[i].status) {
		WARNING("GB_set: buffer[%d] was already set.\n", i);
	}

	b->head[i].status = _GB_STATUS_READY;
	return _GB_SUCCESS;
}

/**
 * Marks the index [i] as empty. Gives a warning if the index was
 * already empty.
 */
int GB_unset(GBuffer b, const int i)
{
	if(_GB_SUCCESS != GB_check(b, "GB_unset")) {
		return _GB_INVALID;
	}

	if(i >= b->size) {
		DEBUG_PRINT("GB_unset: index %d out of buffer range %d.\n", i, b->size);
		return _GB_FAILED;
	}

//	if(_GB_STATUS_EMPTY == b->head[i].status) {
//		WARNING("GB_unset: buffer[%d] was already empty.\n", i);
//	}

	b->head[i].status = _GB_STATUS_EMPTY;
	return _GB_SUCCESS;
}

/**
 * Marks the index [i] as delivered. Gives a warning if the index was
 * empty or already delivered.
 */
int GB_markAsDelivered(GBuffer b, const int i)
{
	if(_GB_SUCCESS != GB_check(b, "GB_markAsDelivered")) {
		return _GB_INVALID;
	}

	if(i >= b->size) {
		DEBUG_PRINT("GB_markAsDelivered: index %d out of buffer range %d.\n", i, b->size);
	}

	if(_GB_STATUS_EMPTY == b->head[i].status) {
		WARNING("GB_markAsDelivered: buffer[%d] was not set.\n", i);
	}

	if(_GB_STATUS_DELIVERED == b->head[i].status) {
		WARNING("GB_markAsDelivered: buffer[%d] was already marked as delivered.\n", i);
	}

	b->head[i].status = _GB_STATUS_DELIVERED;

	return _GB_SUCCESS;
}

/************************************************************
* Buffer single index check function
************************************************************/

/**
 * Returns 0 if the index [i] is empty, non-zero otherwise.
 */
int GB_isSet(GBuffer b, const int i)
{
	if(_GB_SUCCESS != GB_check(b, "GB_isSet")) {
		return _GB_INVALID;
	}

	if(i >= b->size) {
		DEBUG_PRINT("GB_isSet: index %d out of buffer range %d.\n", i, b->size);
	}

	return b->head[i].status != _GB_STATUS_EMPTY;
}

/**
 * Returns 0 if the index [i] is not marked as delivered,
 * non-zero otherwise.
 */
int GB_isDelivered(GBuffer b, const int i)
{
	if(_GB_SUCCESS != GB_check(b, "GB_isDelivered")) {
		return _GB_INVALID;
	}

	if(i >= b->size) {
		DEBUG_PRINT("GB_isDelivered: index %d out of buffer range %d.\n", i, b->size);
	}

	return b->head[i].status == _GB_STATUS_DELIVERED;
}

/************************************************************
* Full buffer check functions
************************************************************/

/**
 * Returns 0 if there is at least one index [i] in the buffer [b]
 * that is marked as empty, non-zero otherwise.
 */
int GB_isFull(GBuffer b)
{
	if(_GB_SUCCESS != GB_check(b, "GB_isFull")) {
		return _GB_INVALID;
	}

	int ret = 1, i;

	for(i = 0; i < b->size; ++i) {
		ret &= (b->head[i].status != _GB_STATUS_EMPTY);
	}

	return ret;
}

/**
 * Returns 0 if all indexes [i] in the buffer [b] are marked
 * as empty, non-zero otherwise.
 */
int GB_isEmpty(GBuffer b)
{
	if(_GB_SUCCESS != GB_check(b, "GB_isEmpty")) {
		return _GB_INVALID;
	}

	int ret = 1, i;

	for(i = 0; i < b->size; ++i) {
		ret &= (b->head[i].status == _GB_STATUS_EMPTY);
	}

	return ret;
}

/**
 * Returns 1 if all indexes [i] in the buffer [b] are marked
 * as delivered, non-zero otherwise.
 */
int GB_isAllDelivered(GBuffer b)
{
	if(_GB_SUCCESS != GB_check(b, "GB_isAllDelivered")) {
		return _GB_INVALID;
	}

	int ret = 1, i;

	for(i = 0; i < b->size; ++i) {
		ret &= (_GB_STATUS_DELIVERED == b->head[i].status);
	}

	return ret;
}

/************************************************************
* Buffer printer function
************************************************************/

static const char * const itoa(const int d)
{
	static char tmp[16];

	snprintf(tmp, 16, "%d", d);

	return tmp;
}

int GB_print(GBuffer b, PrintFunction f)
{
	if(_GB_SUCCESS != GB_check(b, "GB_print")) {
		return _GB_INVALID;
	}

	if(NULL == f) {
		f = (PrintFunction) itoa;
	}

	int i;

	char *header = (char *) calloc(100, sizeof(char));

	switch(b->type) {
		case _GB_DOUBLE:
			snprintf(header, 100, "=== GeneralBuffer<%s>[%d] ===", "Double", b->size);
			break;
		case _GB_LONG_INT:
			snprintf(header, 100, "=== GeneralBuffer<%s>[%d] ===", "LongInt", b->size);
			break;
		default:
			DEBUG_PRINT("GB_print: buffer type not recognized, this should not have happened.\n");
			free(header);
			return _GB_INVALID;
	}

	DEBUG_PRINT("\n%s\n", header);

	for(i = 0; i < b->size; ++i) {
		if(_GB_STATUS_EMPTY == b->head[i].status) {
			DEBUG_PRINT("[%s]: (empty)\n", f(i));
		}
		else {
			switch(b->type) {
				case _GB_DOUBLE:
					DEBUG_PRINT("[%s]: %.8e", f(i), b->head[i].value.double_value);
					break;
				case _GB_LONG_INT:
					DEBUG_PRINT("[%s]: %ld", f(i), b->head[i].value.int_value);
					break;
				default:
					DEBUG_PRINT("GB_print: buffer type not recognized, this should not have happened.\n");
					return _GB_INVALID;
			}
			DEBUG_PRINT("%s\n", (_GB_STATUS_DELIVERED == b->head[i].status) ? " (delivered)" : "");
		}
	}

	snprintf(header, strlen(header) + 1, "=============================================================================");

	DEBUG_PRINT("%s\n\n", header);
	free(header);

	return _GB_SUCCESS;
}

//void GB_printDecorated(GBuffer b)
//{
//	if(_GB_SUCCESS != GB_check(b, "GB_getValue")) {
//		return _GB_INVALID;
//	}
//	if(_GB_DOUBLE == b->type) {
//		GB_printBufferDoubleDecorated(b, f);
//	}
//	else {
//		GB_printBufferLongIntDecorated(b, f);
//	}
//}
//
//static void GB_printBufferDouble(GBuffer b)
//{
//	if(NULL == b) {
//		DEBUG_PRINT("GB_printBufferDouble: NULL pointer argument.\n");
//	}
//
//	int i;
//
//	DEBUG_PRINT("\n=== GeneralBuffer<Double>[%d] ===\n", b->size);
//	for(i = 0; i < b->size; ++i) {
//		if(b->head[i].status == _GB_STATUS_EMPTY) {
//			DEBUG_PRINT("[%d]: (empty)\n", i);
//		}
//		else {
//			DEBUG_PRINT("[%d]: %.8e%s\n", i, b->head[i].value.double_value, (b->head[i].status == _GB_STATUS_DELIVERED) ? " (delivered)" : "");
//		}
//	}
//	DEBUG_PRINT("================================\n\n");
//}
//
//static void GB_printBufferLongInt(GBuffer b)
//{
//	if(NULL == b) {
//		DEBUG_PRINT("GB_printBufferLongInt: NULL pointer argument.\n");
//	}
//
//	int i;
//
//	DEBUG_PRINT("\n=== GeneralBuffer<LongInt>[%d] ===\n", b->size);
//	for(i = 0; i < b->size; ++i) {
//		if(b->head[i].status == _GB_STATUS_EMPTY) {
//			DEBUG_PRINT("[%d]: (empty)\n", i);
//		}
//		else {
//			DEBUG_PRINT("[%d]: %ld%s\n", i, b->head[i].value.int_value, (b->head[i].status == _GB_STATUS_DELIVERED) ? " (delivered)" : "");
//		}
//	}
//	DEBUG_PRINT("=================================\n\n");
//}
//
//static void GB_printBufferDoubleDecorated(GBuffer b, PrintFunction f)
//{
//	if((NULL == b) || (NULL == f)) {
//		DEBUG_PRINT("GB_printBufferDouble: NULL pointer argument.\n");
//	}
//
//	int i;
//
//	DEBUG_PRINT("\n=== GeneralBuffer<Double>[%d] ===\n", b->size);
//	for(i = 0; i < b->size; ++i) {
//		if(b->head[i].status == _GB_STATUS_EMPTY) {
//			DEBUG_PRINT("\"%s\": (empty)\n", f(i));
//		}
//		else {
//			DEBUG_PRINT("\"%s\": %.8e%s\n", f(i), b->head[i].value.double_value, (b->head[i].status == _GB_STATUS_DELIVERED) ? " (delivered)" : "");
//		}
//	}
//	DEBUG_PRINT("================================\n\n");
//}
//
//static void GB_printBufferLongIntDecorated(GBuffer b, PrintFunction f)
//{
//	if((NULL == b) || (NULL == f)) {
//		DEBUG_PRINT("GB_printBufferLongInt: NULL pointer argument.\n");
//	}
//
//	int i;
//
//	DEBUG_PRINT("\n=== GeneralBuffer<LongInt>[%d] ===\n", b->size);
//	for(i = 0; i < b->size; ++i) {
//		if(b->head[i].status == _GB_STATUS_EMPTY) {
//			DEBUG_PRINT("\"%s\": (empty)\n", f(i));
//		}
//		else {
//			DEBUG_PRINT("\"%s\": %ld%s\n", f(i), b->head[i].value.int_value, (b->head[i].status == _GB_STATUS_DELIVERED) ? " (delivered)" : "");
//		}
//	}
//	DEBUG_PRINT("=================================\n\n");
//}

static int GB_check(GBuffer b, const char * const fname)
{

	if((NULL == b) || (NULL == b->head)) {
		DEBUG_PRINT("%s: NULL pointer argument.\n", fname);
		return _GB_INVALID;
	}

	if(_GB_TYPE_NUMBER <= b->type) {
		DEBUG_PRINT("%s: buffer type is not valid.\n", fname);
		return _GB_INVALID;
	}

	if(0 >= b->size) {
		DEBUG_PRINT("%s: buffer size (%d) is too little.\n", fname, b->size);
		return _GB_INVALID;
	}

	return _GB_SUCCESS;
}

