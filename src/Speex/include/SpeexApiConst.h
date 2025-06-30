#ifndef __SPEEX_API_CONST_H
#define __SPEEX_API_CONST_H

#define SPEEX_VERSION "speex-1.2.0"   /**< Speex version string. */


#define SPEEX_API_SUCCESS                      0
#define SPEEX_API_INVALID_PARAM                -1
#define SPEEX_API_NOT_ENOUGH_MEMORY            -2
#define SPEEX_API_ERROR                        -10

typedef struct SpeexDataHeader
{
	char szSpeexVersion[12];     // Speex version.
	short nHeaderSize;           // Total size of the header ( sizeof(SpeexHeader) )/
	short nChannelNum;           // The channel number of PCM wave data. It must be 1.
	short nSampleRate;           // The sampling rate of PCM wave data.
	unsigned short nBitRate;     // The speex encode bit-rate.
	                             // For 8 KHz narrow-band, bit rate/encode bytes per frame/quality:
	                             //    4000/10/very poor, 6000/15/poor, 8000/20/acceptable, 11200/28/more acceptable, 
	                             //    15200/38/more acceptable,
	                             // For 16 KHz wide-band (encode frame bytes/quality):
	                             //    8000/20/poor, 10000/25/bad, 12800/32/good but with defect, 16800/42/better but with defect, 
	                             //    20800/52/good enough with little defect, 24000/60/very good and near no defect
	short nFrameSamples;         // The sample count of PCM frame. Speex always use 20 ms per frame.
	                             // For 8 KHz narrow-band, it is 160. For 16 KHz wide-band, it is 320.
	short nEncodeSize;           // The size of encode data per frame.
} SpeexDataHeader;



#endif
