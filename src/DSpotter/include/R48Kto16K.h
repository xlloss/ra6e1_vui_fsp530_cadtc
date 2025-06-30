#ifndef __48K_TO_16K_H__
#define __48K_TO_16K_H__

#include "base_types.h"

#define R48Kto16K_SUCCESS					0
#define R48Kto16K_IllegalParam				-1
#define R48Kto16K_LeaveNoMemory				-2
#define R48Kto16K_NotInit					-3

// At RA6M1 120 MHz CM4
// [22/10/20 - 11:44:56:793] R48Kto16K_Resample() start. Test 10000 ms 48K data.
// [22/10/20 - 11:44:57:007] R48Kto16K_Resample() stop
// 120 * 0.214 / 10 = 2.57 MCPS

typedef struct tagR48Kto16K
{
	int nFrameSamples48K;
	short *lpsFrame48K;
	BOOL bUseMalloc;
}	R48Kto16K;


#define R48KTO16K_GET_MEM_USAGE(nFrameSamples48K) (sizeof(R48Kto16K) + (nFrameSamples48K + 32)*sizeof(short) + sizeof(int))


#ifdef __cplusplus
extern "C" {
#endif

// Return the memory usage of R48Kto16K.
//   nFrameSamples48K(IN): The frame samples for convert. It must be multiples of 3 and great than 32.
// Return the memory usage in bytes*/
int R48Kto16K_GetMemUsage(const int nFrameSamples48K);

// Initial the 48KHz to 16KHz re-sampler. It will use (nFrameSamples48K + 32)*2 heap size.
//   lpMem(IN): The outside memory. R48Kto16K will use malloc() if it is NULL.
//   nMemSize: The memory size of lpMem.
//   nFrameSamples48K(IN): The frame samples for convert. It must be multiples of 3 and great than 32.
//   lphR48Kto16K(OUT): The handle of R48Kto16K.
// Return: R48Kto16K_SUCCESS or error code.
int R48Kto16K_Init(void *lpMem, int nMemSize, int nFrameSamples48K, HANDLE *lphR48Kto16K);

// To re-sample 48K to 16K.
//   hR48Kto16K(IN): The handle of R48Kto16K.
//   lpsFrame48K(IN): The 48KHz input audio data.
//   nFrameSamples48K(IN): The sample count of lpsFrame48K.
//   lpsFrame16K(OUT): The 16KHz output audio data.
//   nFrameSamples16K(IN): The sample count of lpsFrame16K, it must be nFrameSamples48K/3.
// Return: R48Kto16K_SUCCESS or error code.
int R48Kto16K_Resample(HANDLE hR48Kto16K, const short *lpsFrame48K, int nFrameSamples48K, short *lpsFrame16K, int nFrameSamples16K);

// Release memory resource.
//   hR48Kto16K(in): The handle of R48Kto16K.
// Return: R48Kto16K_SUCCESS or error code.
int R48Kto16K_Release(HANDLE hR48Kto16K);

#ifdef __cplusplus
}
#endif


#endif
