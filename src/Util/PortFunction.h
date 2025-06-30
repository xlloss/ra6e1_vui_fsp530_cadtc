#ifndef __PORT_FUNCTION_H_
#define __PORT_FUNCTION_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SoftwareDelayMillSec(uint32_t delay_in_ms);

void *PortMalloc(size_t size);
void *PortCalloc(size_t num, size_t size);
void *PortRealloc(void *ptr, size_t size);
void  PortFree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
