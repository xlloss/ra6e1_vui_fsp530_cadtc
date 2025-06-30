#ifndef __CYB_VAD_h__
#define __CYB_VAD_h__

#include "base_types.h"

enum {
	CYB_VAD_STATUS_INIT = 0,
	CYB_VAD_STATUS_NO_SPEECH,
	CYB_VAD_STATUS_START,
	CYB_VAD_STATUS_ACTIVE,
	CYB_VAD_STATUS_END,
};

#define CYBVAD_SUCCESS                     0
#define CYBVAD_ERR_FEA_BUF_INSUFFICIENT    -2
#define CYBVAD_ERR_ILLEGAL_PARAM           -9



#if defined(_WIN32)
#ifdef CYBVAD_EXPORTS
#define CYBVADDLL_API __declspec(dllexport)
#endif
#endif

#ifndef CYBVADDLL_API
#define CYBVADDLL_API
#endif

#ifdef __cplusplus
extern "C"{
#endif

/**
* CybVAD_GetMemoryUsage - Get the memory usage.
*   nSampleRate(IN): It must be 16000.
* Returns the memory usage in bytes.
*/
CYBVADDLL_API INT CybVAD_GetMemoryUsage(INT nSampleRate);

/**
 * CybVAD_Init - Initialize a VAD object.
 *   nSampleRate(IN): It must be 16000.
 *   lpbyMemPool(IN): Memory buffer for the VAD.
 *   nMemSize(IN): The size in bytes of lpbyMemPool.
 *   lpbyLicense(IN): The license data.
 *   pnErr(OUT): The error code.
 * Returns handle on success, NULL on error.
 */
CYBVADDLL_API HANDLE CybVAD_Init(INT nSampleRate, BYTE *lpbyMemPool, INT nMemSize, const BYTE *lpbyLicense, INT *pnErr);

/**
 * CybVAD_Release - Release a VAD object.
 *   hCybVAD(IN): The handle of VAD.
 * Returns 0 on success, otherwise on error.
 */
CYBVADDLL_API INT CybVAD_Release(HANDLE hCybVAD);

//CYBVADDLL_API INT CybVAD_SetEndSilenceLength(HANDLE hCybVAD, INT nSilenceIn10ms);

/**
 * CybVAD_CheckEPD - Check EPD status of current frame.
 *   hCybVAD(IN): The handle of VAD.
 *   lpsSample(IN): The 16 bits, mono PCM audio input data.
 *   nNumSample(IN): The sample count of audio buffer.
 * Returns 0 if in initial state, 1 if silence, 2 if voice start, 3 if voice still active, 4 if voice end.
 */
CYBVADDLL_API INT CybVAD_CheckEPD(HANDLE hCybVAD, const SHORT *lpsSample, INT nNumSample);

/**
* CybVAD_ClearEPD - To clear VAD state for prepare the next detection.
*   hCybVAD(IN): The handle of VAD.
* Returns 0 on success, otherwise on error.
*/
CYBVADDLL_API INT CybVAD_ClearEPD(HANDLE hCybVAD);

/**
* CybVAD_SetActiveThreshold - Set the active threshold of VAD.
*   hCybVAD(IN): The handle of VAD.
*   nActiveThreshold(IN): The value range is 1 ~ 6, default value is 1, lower threshold means the VAD is more easy to become active.
* Returns 0 on success, otherwise on error.
*/
CYBVADDLL_API INT CybVAD_SetActiveThreshold(HANDLE hCybVAD, INT nActiveThreshold);

/**
* CybVAD_SetEndSilenceLength - Set the end silence of VAD.
*   hCybVAD(IN): The handle of VAD.
*   nSilenceIn10ms(IN): The length of ending silence, unit is 10 ms.
* Returns 0 on success, otherwise on error.
*/
CYBVADDLL_API INT CybVAD_SetEndSilenceLength(HANDLE hCybVAD, INT nSilenceIn10ms);

#ifdef __cplusplus
}
#endif

#endif	//__DVAD_h__
