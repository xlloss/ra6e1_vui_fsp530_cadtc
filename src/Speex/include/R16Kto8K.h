#ifndef __16K_TO_8K_H__
#define __16K_TO_8K_H__

#include "base_types.h"

#define R16Kto8K_SUCCESS					0
#define R16Kto8K_IllegalParam				-1
#define R16Kto8K_LeaveNoMemory				-2
#define R16Kto8K_NotInit					-3

// At RA6M1 120 MHz CM4
// [22/10/20 - 11:44:56:793] R48Kto16K_Resample() start. Test 10000 ms 48K data.
// [22/10/20 - 11:44:57:007] R48Kto16K_Resample() stop
// 120 * 0.214 / 10 = 2.57 MCPS

// R16Kto8K computation is about half of R48Kto16K, so should be 1.3 MCPS.

#ifdef __cplusplus
extern "C" {
#endif

// Initial the 16KHz to 8KHz re-sampler. It will use (nFrameSamples16K + 32)*2 heap size.
// nFrameSamples16K(IN): The frame samples for convert. It must be multiples of 2 and great than 32.
// Return: R16Kto8K_SUCCESS or error code.
HANDLE R16Kto8K_Init(int nFrameSamples16K, int *pnErr);

// To re-sample 16K to 8K.
// lpsFrame16K(IN): The 16KHz input audio data.
// nFrameSamples16K(IN): The sample count of lpsFrame16K.
// lpsFrame8K(OUT): The 8KHz output audio data.
// nFrameSamples8K(IN): The sample count of lpsFrame8K, it must be nFrameSamples16K/2.
// Return: R16Kto8K_SUCCESS or error code.
int R16Kto8K_Resample(HANDLE hR16Kto8K, const short *lpsFrame16K, int nFrameSamples16K, short *lpsFrame8K, int nFrameSamples8K);

// Release heap resource.
// Return: R16Kto8K_SUCCESS or error code.
int R16Kto8K_Release(HANDLE hR16Kto8K);

#ifdef __cplusplus
}
#endif


#endif
