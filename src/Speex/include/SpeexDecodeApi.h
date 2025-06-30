#ifndef __SPEEX_DECODE_API_H
#define __SPEEX_DECODE_API_H

#include "base_types.h"
#include "SpeexApiConst.h"

#ifdef __cplusplus
extern "C" {
#endif

int SpeexDecoderGetWorkingMemoryUsage(int nSampleRate);

int SpeexDecoderSetWorkingMemory(char *lpMemory, int nSize);

/**
* SpeexDecoderCheckDataHeader - Check the validation of speex data header.
*   lpoSpeexDataHeader(IN): The speex data header.
* Returns 0 on success, negative on error.
*/
int SpeexDecoderCheckDataHeader(const SpeexDataHeader *lpoSpeexDataHeader);

/**
 * SpeexDecoderInit - Initialize a speex decoder object.
 *   nChannelNum(IN):     At current time, we only support mono channel, so it must be 1.
 *   nSampleRate(IN):     It could be 8000, 16000. Please refer to SpeexDataHeader.nSampleRate.
 *   nBitRate(IN):        The encode bit rate. Please refer to SpeexDataHeader.nBitRate.
 *   bUseEnhancement(IN): We use it for a little better quality but more CPU power.
 *   pnErr(OUT):          The error code.
 * Returns handle on success, NULL on error.
 */
HANDLE SpeexDecoderInit(int nChannelNum, int nSampleRate, int nBitRate, BOOL bUseEnhancement, int *pnErr);

/**
 * SpeexDecoderRelease - Release a speex decoder object.
 *   hSpeexDecoder(IN): The handle of speex decoder object.
 * Returns 0 on success, negative on error.
 */
int SpeexDecoderRelease(HANDLE hSpeexDecoder);

/**
 * SpeexDecoderGetEncodeFrameSize - Get the encode size per frame.
 *   hSpeexDecoder(IN): The handle of speex decoder object.
 * Returns positive value for the encode size per frame, negative on error.
 */
int SpeexDecoderGetEncodeFrameSize(HANDLE hSpeexDecoder);

/**
 * SpeexDecoderGetFrameSamples - Get the PCM sample count per frame.
 *   hSpeexDecoder(IN): The handle of speex decoder object.
 * Returns positive value for the PCM sample count per frame, negative on error.
 */
int SpeexDecoderGetFrameSamples(HANDLE hSpeexDecoder);

/**
 * SpeexDecoderDo - Decode the input data to PCM samples.
 *   hSpeexDecoder(IN): The handle of speex decoder object.
 *   lpbyEncode(IN): The input of encoded data.
 *   nEncodeSize(IN): The byte count of lpbyEncode, it must be equal to the return value of SpeexDecoderGetEncodeSize().
 *   lpsOutFrame(Out): The output 16 bits PCM data frame.
 *   nOutFrameSamples(IN): The sample count of lpsOutFrame, it must be equal to the return value of SpeexDecoderGetFrameSamples().
 * Returns 0 on success, negative on error.
 */
int SpeexDecoderDo(HANDLE hSpeexDecoder, const BYTE *lpbyEncode, int nEncodeSize, short *lpsOutFrame, int nOutFrameSamples);


#ifdef __cplusplus
}
#endif

#endif
