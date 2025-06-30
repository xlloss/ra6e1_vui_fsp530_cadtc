#include <stdlib.h>
#include "hal_data.h"
#include "PortFunction.h"

void SoftwareDelayMillSec(uint32_t delay_in_ms)
{
    R_BSP_SoftwareDelay(delay_in_ms, BSP_DELAY_UNITS_MILLISECONDS);
}

#ifdef INC_FREERTOS_H

void *PortMalloc(size_t size)
{
    void *p;

    p = pvPortMalloc(size);
    if(p==NULL)
    {
        __BKPT(0);
    }
    return p;
}

void *PortCalloc(size_t num, size_t size)
{
    void *p;

    /* allocate 'count' objects of size 'size' */
    p = pvPortMalloc(num * size);
    if (p) {
      /* zero the memory */
      memset(p, 0, num * size);
    }
    else
    {
        __BKPT(0);
    }
    return p;
}

void *PortRealloc(void *ptr, size_t size)
{
    if (size == 0)
    {
        vPortFree(ptr);
        return NULL;
    }

    void *p;
    p = pvPortMalloc(size);
    if (p) {
        /* zero the memory */
        if (ptr != NULL) {
            memcpy(p, ptr, size);
            vPortFree(ptr);
        }
    }

    return p;
}

void  PortFree(void *ptr)
{
    vPortFree(ptr);
}

#else

void *PortMalloc(size_t size)
{
    void *p = malloc(size);

    if (!p)
        __BKPT(0);
    return p;
}

void *PortCalloc(size_t num, size_t size)
{
    void *p = calloc(num, size);

    if (!p)
        __BKPT(0);
    return p;
}

void *PortRealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);

    if (!p)
        __BKPT(0);
    return p;
}

void  PortFree(void *ptr)
{
    free(ptr);
}

#endif
