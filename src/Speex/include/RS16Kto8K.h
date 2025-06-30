#ifndef __RS16K_TO_8K_H__
#define __RS16K_TO_8K_H__

#include "base_types.h"

#define RS16Kto8K_SUCCESS					0
#define RS16Kto8K_IllegalParam				-1
#define RS16Kto8K_LeaveNoMemory				-2
#define RS16Kto8K_NotInit					-3

// At RA6M1 120 MHz CM4
// [22/10/20 - 11:44:56:793] R48Kto16K_Resample() start. Test 10000 ms 48K data.
// [22/10/20 - 11:44:57:007] R48Kto16K_Resample() stop
// 120 * 0.214 / 10 = 2.57 MCPS

// R16Kto8K computation is about half of R48Kto16K, so should be 1.3 MCPS.

typedef struct tagRS16Kto8K
{
	short sFrameBuf[32];
	BOOL bUseMalloc;
}	RS16Kto8K;


#define RS16Kto8K_GET_MEM_USAGE() (sizeof(RS16Kto8K))


#ifdef __cplusplus
extern "C" {
#endif

// Return the memory usage of RS16Kto8K.
// Return the memory usage in bytes*/
int RS16Kto8K_GetMemUsage();

// Initial the 16KHz to 8KHz re-sampler. It will use sizeof(RS16Kto8K) heap size.
//   lpMem(IN): The outside memory. RS16Kto8K will use malloc() if it is NULL.
//   nMemSize: The memory size of lpMem.
//   lphRS16Kto8K(OUT): The handle of RS16Kto8K.
// Return: RS16Kto8K_SUCCESS or error code.
int RS16Kto8K_Init(void *lpMem, int nMemSize, HANDLE *lphRS16Kto8K);

// To re-sample 16K to 8K.
//   hRS16Kto8K(IN): The handle of RS16Kto8K.
//   lpsFrame16K(IN): The 16KHz input audio data.
//   nFrameSamples16K(IN): The sample count of lpsFrame16K.
//   lpsFrame8K(OUT): The 8KHz output audio data.
//   nFrameSamples8K(IN): The sample count of lpsFrame8K, it must be nFrameSamples16K/2.
// Return: RS16Kto8K_SUCCESS or error code.
int RS16Kto8K_Resample(HANDLE hRS16Kto8K, const short *lpsFrame16K, int nFrameSamples16K, short *lpsFrame8K, int nFrameSamples8K);

// Release memory resource.
//   hRS16Kto8K(in): The handle of RS16Kto8K.
// Return: RS16Kto8K_SUCCESS or error code.
int RS16Kto8K_Release(HANDLE hRS16Kto8K);

#ifdef __cplusplus
}
#endif


#endif
