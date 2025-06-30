#ifndef __RS8K_TO_16K_H__
#define __RS8K_TO_16K_H__

#include "base_types.h"

#define RS8Kto16K_SUCCESS					0
#define RS8Kto16K_IllegalParam				-1
#define RS8Kto16K_LeaveNoMemory				-2
#define RS8Kto16K_NotInit					-3

// At RA6M1 120 MHz CM4
// [22/10/20 - 11:44:56:793] R48Kto16K_Resample() start. Test 10000 ms 48K data.
// [22/10/20 - 11:44:57:007] R48Kto16K_Resample() stop
// 120 * 0.214 / 10 = 2.57 MCPS

// RS8Kto16K computation is similar to R48Kto16K, so should be 2.57 MCPS.

typedef struct tagRS8Kto16K
{
	short sFrameBuf[32];
	BOOL bUseMalloc;
}	RS8Kto16K;


#define RS8Kto16K_GET_MEM_USAGE() (sizeof(RS8Kto16K))

#ifdef __cplusplus
extern "C" {
#endif

// Return the memory usage of RS8Kto16K.
// Return the memory usage in bytes*/
int RS8Kto16K_GetMemUsage();

// Initial the 8KHz to 16KHz re-sampler. It will use (nFrameSamples8K*2 + 32)*2 heap size.
//   lpMem(IN): The outside memory. RS16Kto8K will use malloc() if it is NULL.
//   nMemSize: The memory size of lpMem.
//   lphRS8Kto16K(OUT): The handle of RS8Kto16K.
// Return: RS8Kto16K_SUCCESS or error code.
int RS8Kto16K_Init(void *lpMem, int nMemSize, HANDLE *lphRS8Kto16K);

// To re-sample 8K to 16K.
//   lpsFrame8K(IN): The 8KHz input audio data.
//   nFrameSamples8K(IN): The sample count of lpsFrame8K.
//   lpsFrame16K(OUT): The 16KHz output audio data.
//   nFrameSamples16K(IN): The sample count of lpsFrame16K, it must be nFrameSamples8K*2.
// Return: RS8Kto16K_SUCCESS or error code.
int RS8Kto16K_Resample(HANDLE hRS8Kto16K, const short *lpsFrame8K, int nFrameSamples8K, short *lpsFrame16K, int nFrameSamples16K);

// Release heap resource.
// Return: RS8Kto16K_SUCCESS or error code.
int RS8Kto16K_Release(HANDLE hRS8Kto16K);

#ifdef __cplusplus
}
#endif


#endif
