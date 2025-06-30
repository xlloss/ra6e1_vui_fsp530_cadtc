#ifndef __8K_TO_16K_H__
#define __8K_TO_16K_H__

#include "base_types.h"

#define R8Kto16K_SUCCESS					0
#define R8Kto16K_IllegalParam				-1
#define R8Kto16K_LeaveNoMemory				-2
#define R8Kto16K_NotInit					-3

// At RA6M1 120 MHz CM4
// [22/10/20 - 11:44:56:793] R48Kto16K_Resample() start. Test 10000 ms 48K data.
// [22/10/20 - 11:44:57:007] R48Kto16K_Resample() stop
// 120 * 0.214 / 10 = 2.57 MCPS

// R8Kto16K computation is need to R48Kto16K, so should be 2.57 MCPS.

#ifdef __cplusplus
extern "C" {
#endif

// Initial the 8KHz to 16KHz re-sampler. It will use (nFrameSamples8K*2 + 32)*2 heap size.
// nFrameSamples8K(IN): The frame samples for convert. It must be multiples of 2 and great than 32.
// Return: R8Kto16K_SUCCESS or error code.
HANDLE R8Kto16K_Init(int nFrameSamples8K, int *pnErr);

// To re-sample 8K to 16K.
// lpsFrame8K(IN): The 8KHz input audio data.
// nFrameSamples8K(IN): The sample count of lpsFrame8K.
// lpsFrame16K(OUT): The 16KHz output audio data.
// nFrameSamples16K(IN): The sample count of lpsFrame16K, it must be nFrameSamples8K*2.
// Return: R8Kto16K_SUCCESS or error code.
int R8Kto16K_Resample(HANDLE hR8Kto16K, const short *lpsFrame8K, int nFrameSamples8K, short *lpsFrame16K, int nFrameSamples16K);

// Release heap resource.
// Return: R8Kto16K_SUCCESS or error code.
int R8Kto16K_Release(HANDLE hR8Kto16K);

#ifdef __cplusplus
}
#endif


#endif
