#ifndef __SPEEX_ENCODE_API_H
#define __SPEEX_ENCODE_API_H

#include "base_types.h"
#include "SpeexApiConst.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * SpeexEncoderInit - Initialize a speex encoder object.
 *   nChannelNum(IN): At current time, we only support mono channel, so it must be 1.
 *   nSampleRate(IN): It could be 8000 or 16000.
 *   nExpectBitRate(IN): The expect bit rate for encoder. The valid range is 2400 ~ 64000.
 *                    This value should be a multiple of 400.
 *                    The real bit rate will near to this value and can be got in
 *                    SpeexHeader.nBitRate. We use constant bit rate to encode.
 *                    For 8 KHz narrow-band, the supported SpeexDataHeader.nBitRate are:
 *                       4000(very poor), 6000(poor), 8000(acceptable), 11200(acceptable), 15200(acceptable)
 *                    For 16 KHz wide-band, the supported SpeexDataHeader.nBitRate are:
 *                       8000(poor), 10000(bad), 12800(good), 16800(better), 20800(good enough), 24000(very good)
 *   nComlexity(IN):  The valid range is 1 ~ 10. It require more CPU power for the larger value.
 *                    When nComlexity is 10, become very slow but get better quality.
 *   pnErr(OUT):      The error code.
 * Returns handle on success, NULL on error.
 */
HANDLE SpeexEncoderInit(int nChannelNum, int nSampleRate, int nExpectBitRate, int nComlexity, int *pnErr);

/**
 * SpeexEncoderRelease - Release a speex encoder object.
 *   hSpeexEncoder(IN): The handle of speex object.
 * Returns 0 on success, negative on error.
 */
int SpeexEncoderRelease(HANDLE hSpeexEncoder);

/**
 * SpeexEncoderGetDataHeader - Get the data header of speex.
 *   hSpeexEncoder(IN): The handle of speex object.
 *   lpoSpeexDataHeader(OUT): The structure of speex data header, it contains many properties of speex encode data.
 *                        We put it to the header of *.spx file. The speex decoder need these information to
 *                        do initialization.
 * Returns 0 on success, negative on error.
 */
int SpeexEncoderGetDataHeader(HANDLE hSpeexEncoder, SpeexDataHeader *lpoSpeexDataHeader);

/**
 * SpeexEncoderDo - Encode the input PCM frame to output data.
 *   hSpeexEncoder(IN): The handle of speex encoder object.
 *   lpsInFrame(IN): The input 16 bits PCM data frame.
 *   nInFrameSamples(IN): The sample count of lpsInFrame, it must be equal to SpeexDataHeader.nFrameSamples.
 *   lpbyOutEncode(OUT): The output of encoded data.
 *   nOutEncodeSize(IN): The byte count of lpbyOutEncode, it must be equal to SpeexDataHeader.nEncodeSize.
 * Returns 0 on success, negative on error.
 */
int SpeexEncoderDo(HANDLE hSpeexEncoder, const short *lpsInFrame, int nInFrameSamples, BYTE *lpbyOutEncode, int nOutEncodeSize);


#ifdef __cplusplus
}
#endif

#endif
