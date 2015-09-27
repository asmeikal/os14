#include <Buffers.h>

#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>

/************************************************************
* Enum definition
************************************************************/

enum buffer_status_t {
    BUFFER_STATUS_EMPTY = 0,
    BUFFER_STATUS_READY,
    BUFFER_STATUS_DELIVERED,
    BUFFER_STATUS_NUMBER
};

enum buffer_type_t {
    BUFFER_DOUBLE = 0,
    BUFFER_LONG_INT,
    BUFFER_TYPE_NUMBER
};

/************************************************************
* Types, structs, and unions
************************************************************/

union buffer_general_value_t {
    long int int_value;
    double double_value;
};

struct buffer_general_t {
    enum buffer_status_t status;
    union buffer_general_value_t value;
};

struct buffer_general {
    enum buffer_type_t type;
    int size;
    struct buffer_general_t *head;
};

/************************************************************
* Local functions declaration
************************************************************/

static GBuffer GB_init(const int size, const enum buffer_type_t type);

static void GB_printBufferDouble(GBuffer b);
static void GB_printBufferLongInt(GBuffer b);
static void GB_printBufferDoubleDecorated(GBuffer b, PrintFunction f);
static void GB_printBufferLongIntDecorated(GBuffer b, PrintFunction f);

/************************************************************
* Initialization functions
************************************************************/

/**
 * Returns a pointer to a new GeneralBuffer<Double>
 */
GBuffer GB_initDouble(const int size)
{
    return GB_init(size, BUFFER_DOUBLE);
}

/**
 * Returns a pointer to a new GeneralBuffer<LongInt>
 */
GBuffer GB_initLongInt(const int size)
{
    return GB_init(size, BUFFER_LONG_INT);
}

/**
 * Returns a pointer to a new (and empty) GeneralBuffer<type>
 */
static GBuffer GB_init(const int size, const enum buffer_type_t type)
{
    if(BUFFER_TYPE_NUMBER <= type) {
        ERROR("GB_init: type not recognized.\n");
    }

    if(0 >= size) {
        ERROR("GB_init: size %d is too small.\n", size);
    }

    struct buffer_general *ret = (struct buffer_general *) malloc(sizeof(ret));
    if(NULL == ret) {
        ERROR("GB_init: malloc failed.\n");
    }

    ret->head = (struct buffer_general_t *) malloc(sizeof(struct buffer_general_t) * size);
    if(NULL == ret->head) {
        // free(ret);
        ERROR("GB_init: malloc failed.\n");
    }

    ret->type = type;
    ret->size = size;

    GB_empty(ret);

    return ret;
}

/************************************************************
* Getters
************************************************************/

/**
 * Gets the double value at index [i]. Throws an error if 
 * the value is not set, the buffer is not a double buffer, 
 * or the index is out of range.
 */
double GB_getDoubleValue(GBuffer b, const int i)
{
    if(NULL == b) {
        ERROR("GB_getDoubleValue: NULL pointer argument.\n");
    }

    if(BUFFER_DOUBLE != b->type) {
        ERROR("GB_getDoubleValue: type mismatch.\n");
    }

    if(i >= b->size) {
        ERROR("GB_getDoubleValue: index %d out of buffer range %d.\n", i, b->size);
    }

    if(BUFFER_STATUS_EMPTY == b->head[i].status) {
        ERROR("GB_getDoubleValue: buffer[%d] is not set.\n", i);
    }

    return b->head[i].value.double_value;
}

/**
 * Gets the long int value at index [i]. Throws an error if 
 * the value is not set, the buffer is not a long int buffer, 
 * or the index is out of range.
 */
long int GB_getLongIntValue(GBuffer b, const int i)
{
    if(NULL == b) {
        ERROR("GB_getLongIntValue: NULL pointer argument.\n");
    }

    if(BUFFER_LONG_INT != b->type) {
        ERROR("GB_getLongIntValue: type mismatch.\n");
    }

    if(i >= b->size) {
        ERROR("GB_getLongIntValue: index %d out of buffer range %d.\n", i, b->size);
    }

    if(BUFFER_STATUS_EMPTY == b->head[i].status) {
        ERROR("GB_getLongIntValue: buffer[%d] is not set.\n", i);
    }

    return b->head[i].value.int_value;
}

/************************************************************
* Setters
************************************************************/

/**
 * Sets the double value at index [i] to [v]. Throws an error if 
 * the buffer is not a double buffer or the index is out of range.
 * Gives a warning if the index was not empty.
 */
void GB_setDoubleValue(GBuffer b, const int i, const double v)
{
    if(NULL == b) {
        ERROR("GB_setDoubleValue: NULL pointer argument.\n");
    }

    if(BUFFER_DOUBLE != b->type) {
        ERROR("GB_setDoubleValue: type mismatch.\n");
    }

    if(i >= b->size) {
        ERROR("GB_setDoubleValue: index %d out of buffer range %d.\n", i, b->size);
    }

    if(BUFFER_STATUS_EMPTY != b->head[i].status) {
        WARNING("GB_setDoubleValue: buffer[%d] was already set to %e.\n", i, b->head[i].value.double_value);
    }

    b->head[i].value.double_value = v;
    b->head[i].status = BUFFER_STATUS_READY;
}

/**
 * Sets the long int value at index [i] to [v]. Throws an error if 
 * the buffer is not a long int buffer or the index is out of range.
 * Gives a warning if the index was not empty.
 */
void GB_setLongIntValue(GBuffer b, const int i, const long int v)
{
    if(NULL == b) {
        ERROR("GB_setLongIntValue: NULL pointer argument.\n");
    }

    if(BUFFER_LONG_INT != b->type) {
        ERROR("GB_setLongIntValue: type mismatch.\n");
    }

    if(i >= b->size) {
        ERROR("GB_setLongIntValue: index %d out of buffer range %d.\n", i, b->size);
    }

    if(BUFFER_STATUS_EMPTY != b->head[i].status) {
        WARNING("GB_setLongIntValue: buffer[%d] was already set to %ld.\n", i, b->head[i].value.int_value);
    }

    b->head[i].value.int_value = v;
    b->head[i].status = BUFFER_STATUS_READY;
}

/************************************************************
* Empty buffer utiliy function
************************************************************/

/**
 * Empties the buffer, i.e. marks all indexes 0..<b->size as empty.
 */
void GB_empty(GBuffer b)
{
    if(NULL == b) {
        ERROR("GB_empty: NULL pointer argument.\n");
    }

    int i;
    for(i = 0; i < b->size; ++i) {
        GB_unset(b, i);
    }
}

/************************************************************
* Buffer status single element set/unset/deliver functions
************************************************************/

/**
 * Marks the index [i] as set. Gives a warning if the index was 
 * already set.
 */
void GB_set(GBuffer b, const int i)
{
    if(NULL == b) {
        ERROR("GB_set: NULL pointer argument.\n");
    }

    if(i >= b->size) {
        ERROR("GB_set: index %d out of buffer range %d.\n", i, b->size);
    }

    if(BUFFER_STATUS_EMPTY != b->head[i].status) {
        WARNING("GB_set: buffer[%d] was already set.\n", i);
    }

    b->head[i].status = BUFFER_STATUS_READY;
}

/**
 * Marks the index [i] as empty. Gives a warning if the index was 
 * already empty.
 */
void GB_unset(GBuffer b, const int i)
{
    if(NULL == b) {
        ERROR("GB_unset: NULL pointer argument.\n");
    }

    if(i >= b->size) {
        ERROR("GB_unset: index %d out of buffer range %d.\n", i, b->size);
    }

    if(BUFFER_STATUS_EMPTY == b->head[i].status) {
        WARNING("GB_unset: buffer[%d] was already empty.\n", i);
    }

    b->head[i].status = BUFFER_STATUS_EMPTY;
}

/**
 * Marks the index [i] as delivered. Gives a warning if the index was 
 * empty or already delivered.
 */
void GB_markAsDelivered(GBuffer b, const int i)
{
    if(NULL == b) {
        ERROR("GB_markAsDelivered: NULL pointer argument.\n");
    }

    if(i >= b->size) {
        ERROR("GB_markAsDelivered: index %d out of buffer range %d.\n", i, b->size);
    }

    if(BUFFER_STATUS_EMPTY == b->head[i].status) {
        WARNING("GB_markAsDelivered: buffer[%d] was not set.\n", i);
    }

    if(BUFFER_STATUS_DELIVERED == b->head[i].status) {
        WARNING("GB_markAsDelivered: buffer[%d] was already marked as delivered.\n", i);
    }

    b->head[i].status = BUFFER_STATUS_DELIVERED;

}

/************************************************************
* Buffer single index check function
************************************************************/

/**
 * Returns 0 if the index [i] is empty, non-zero otherwise.
 */
int GB_isSet(GBuffer b, const int i)
{
    if(NULL == b) {
        ERROR("GB_isSet: NULL pointer argument.\n");
    }

    if(i >= b->size) {
        ERROR("GB_isSet: index %d out of buffer range %d.\n", i, b->size);
    }

    return b->head[i].status != BUFFER_STATUS_EMPTY;
}

/**
 * Returns 0 if the index [i] is not marked as delivered, 
 * non-zero otherwise.
 */
int GB_isDelivered(GBuffer b, const int i)
{
    if(NULL == b) {
        ERROR("GB_isDelivered: NULL pointer argument.\n");
    }

    if(i >= b->size) {
        ERROR("GB_isDelivered: index %d out of buffer range %d.\n", i, b->size);
    }

    return b->head[i].status == BUFFER_STATUS_DELIVERED;
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
    if(NULL == b) {
        ERROR("GB_isFull: NULL pointer argument.\n");
    }

    int ret = 1, i;

    for(i = 0; i < b->size; ++i) {
        ret &= (b->head[i].status != BUFFER_STATUS_EMPTY);
    }

    return ret;
}

/**
 * Returns 0 if all indexes [i] in the buffer [b] are marked
 * as empty, non-zero otherwise.
 */
int GB_isEmpty(GBuffer b)
{
    if(NULL == b) {
        ERROR("GB_isEmpty: NULL pointer argument.\n");
    }

    int ret = 1, i;

    for(i = 0; i < b->size; ++i) {
        ret &= (b->head[i].status == BUFFER_STATUS_EMPTY);
    }

    return ret;
}

/**
 * Returns 0 if all indexes [i] in the buffer [b] are marked
 * as delivered, non-zero otherwise.
 */
int GB_isAllDelivered(GBuffer b)
{
    if(NULL == b) {
        ERROR("GB_isDelivered: NULL pointer argument.\n");
    }

    int ret = 1, i;

    for(i = 0; i < b->size; ++i) {
        ret &= (b->head[i].status == BUFFER_STATUS_DELIVERED);
    }

    return ret;
}

/************************************************************
* Buffer printer function
************************************************************/

void GB_print(GBuffer b)
{
    if(BUFFER_DOUBLE == b->type) {
        GB_printBufferDouble(b);
    }
    else {
        GB_printBufferLongInt(b);
    }
}

void GB_printDecorated(GBuffer b, PrintFunction f)
{
    if(BUFFER_DOUBLE == b->type) {
        GB_printBufferDoubleDecorated(b, f);
    }
    else {
        GB_printBufferLongIntDecorated(b, f);
    }
}

static void GB_printBufferDouble(GBuffer b)
{
    if(NULL == b) {
        ERROR("GB_printBufferDouble: NULL pointer argument.\n");
    }

    int i;

    DEBUG_PRINT("\n=== GeneralBuffer<Double>[%d] ===\n", b->size);
    for(i = 0; i < b->size; ++i) {
        if(b->head[i].status == BUFFER_STATUS_EMPTY) {
            DEBUG_PRINT("[%d]: (empty)\n", i);
        }
        else {
            DEBUG_PRINT("[%d]: %.8e%s\n", i, b->head[i].value.double_value, (b->head[i].status == BUFFER_STATUS_DELIVERED) ? " (delivered)" : "");
        }
    }
    DEBUG_PRINT("================================\n\n");
}

static void GB_printBufferLongInt(GBuffer b)
{
    if(NULL == b) {
        ERROR("GB_printBufferLongInt: NULL pointer argument.\n");
    }

    int i;

    DEBUG_PRINT("\n=== GeneralBuffer<LongInt>[%d] ===\n", b->size);
    for(i = 0; i < b->size; ++i) {
        if(b->head[i].status == BUFFER_STATUS_EMPTY) {
            DEBUG_PRINT("[%d]: (empty)\n", i);
        }
        else {
            DEBUG_PRINT("[%d]: %ld%s\n", i, b->head[i].value.int_value, (b->head[i].status == BUFFER_STATUS_DELIVERED) ? " (delivered)" : "");
        }
    }
    DEBUG_PRINT("=================================\n\n");
}

static void GB_printBufferDoubleDecorated(GBuffer b, PrintFunction f)
{
    if((NULL == b) || (NULL == f)) {
        ERROR("GB_printBufferDouble: NULL pointer argument.\n");
    }

    int i;

    DEBUG_PRINT("\n=== GeneralBuffer<Double>[%d] ===\n", b->size);
    for(i = 0; i < b->size; ++i) {
        if(b->head[i].status == BUFFER_STATUS_EMPTY) {
            DEBUG_PRINT("\"%s\": (empty)\n", f(i));
        }
        else {
            DEBUG_PRINT("\"%s\": %.8e%s\n", f(i), b->head[i].value.double_value, (b->head[i].status == BUFFER_STATUS_DELIVERED) ? " (delivered)" : "");
        }
    }
    DEBUG_PRINT("================================\n\n");
}

static void GB_printBufferLongIntDecorated(GBuffer b, PrintFunction f)
{
    if((NULL == b) || (NULL == f)) {
        ERROR("GB_printBufferLongInt: NULL pointer argument.\n");
    }

    int i;

    DEBUG_PRINT("\n=== GeneralBuffer<LongInt>[%d] ===\n", b->size);
    for(i = 0; i < b->size; ++i) {
        if(b->head[i].status == BUFFER_STATUS_EMPTY) {
            DEBUG_PRINT("\"%s\": (empty)\n", f(i));
        }
        else {
            DEBUG_PRINT("\"%s\": %ld%s\n", f(i), b->head[i].value.int_value, (b->head[i].status == BUFFER_STATUS_DELIVERED) ? " (delivered)" : "");
        }
    }
    DEBUG_PRINT("=================================\n\n");
}

