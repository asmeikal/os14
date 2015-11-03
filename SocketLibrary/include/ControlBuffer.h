#ifndef __BUFFER_CONTROL_H
#define __BUFFER_CONTROL_H

#include <GeneralBuffer.h>
#include <stdint.h>

typedef struct control_buffer *ControlBuffer;

ControlBuffer CB_init(const int size);
int CB_destroy(ControlBuffer b);

int CB_getControl(ControlBuffer b, int32_t *res);
int CB_setControl(ControlBuffer b, const int32_t * const val);

GBuffer CB_getBuffer(ControlBuffer b);

void CB_print(ControlBuffer b);

#endif

