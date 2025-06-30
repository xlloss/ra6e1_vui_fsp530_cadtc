#ifndef __ELAPSED_TIME_H_
#define __ELAPSED_TIME_H_

//https://embeddedcomputing.com/technology/processing/measuring-code-execution-time-on-arm-cortex-m-mcus

/*
********************************************************************************
*                       MODULE TO MEASURE EXECUTION TIME
********************************************************************************
*/

/*
********************************************************************************
*                MAXIMUM NUMBER OF ELAPSED TIME MEASUREMENT SECTIONS
********************************************************************************
*/

#include <stdint.h>

#define  ELAPSED_TIME_MAX_SECTIONS  10

typedef  struct  elapsed_time {
    uint32_t  start;
    uint32_t  current;
    uint32_t  max;
    uint32_t  min;
} ELAPSED_TIME;

extern ELAPSED_TIME elapsed_time_tbl[ELAPSED_TIME_MAX_SECTIONS];

/*
********************************************************************************
*                             FUNCTION PROTOTYPES
********************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

void  elapsed_time_clr   (uint32_t  uSection);      // Clear measured values
void  elapsed_time_init  (void);                    // Module initialization
void  elapsed_time_start (uint32_t  uSection);      // Start measurement
void  elapsed_time_stop  (uint32_t  uSection);      // Stop  measurement
uint32_t get_dwt_cycle();

#ifdef __cplusplus
}
#endif

#endif
