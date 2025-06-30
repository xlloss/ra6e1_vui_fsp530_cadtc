#include "hal_data.h"
#include "base_types.h"
#include "RingBuffer.h"
#include "SpeexDecodeApi.h"
#include "R8Kto16K.h"
#include "PlaySpeex.h"
#include "voice_main.h"

#define SPEEX_FRAME_TIME          20
#define SPEEX_FRAME_SAMPLE_16K    (SPEEX_FRAME_TIME*16000/1000)
#define SPEEX_FRAME_SAMPLE_8K     (SPEEX_FRAME_TIME*8000/1000)
#define SPEEX_FRAME_SIZE_16K      (SPEEX_FRAME_SAMPLE_16K*2)
#define SPEEX_FRAME_SIZE_8K       (SPEEX_FRAME_SAMPLE_8K*2)
#define RBUF_PLAY_SIZE            (SPEEX_FRAME_SIZE_16K*3)

static HANDLE g_hRingBufferPlay = NULL;
static BYTE   g_byaRingBufferPlay[RING_BUFFER_GET_MEM_USAGE(RBUF_PLAY_SIZE)];

static const BYTE* UnpackSpeex(const BYTE *lpbyBin, int nIndex, DWORD *pdwUnpackBinSize, int *pnMapID);
static int GetSpeexPackCount(const BYTE *lpbyBin);
static int GetSpeexMapID(const BYTE *lpbyBin, int nIndex);
static void ScaleVolume(short *lpsSamplesIn, int nSampleCount, int nVolumeScalePercentage, short *lpsSamplesOut);

/* Callback function */
void g_timer_playback_callback(timer_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    int16_t sound_data;

    //Check ring buffer has sample data
    if (RingBufferGetDataSize(g_hRingBufferPlay) < 2)
        return;

    RingBufferGetData(g_hRingBufferPlay, &sound_data, 2);
    R_DAC_Write(&g_dac_playback_ctrl, (uint16_t)((((int)sound_data + 8) >> 4) + 0x0800));
}

int PlaySpeexGetWorkingMemoryUsage(int nSampleRate)
{
    return SpeexDecoderGetWorkingMemoryUsage(nSampleRate);
}

void PlaySpeexSetWorkingMemory(char *lpMemory, int nSize)
{
    SpeexDecoderSetWorkingMemory(lpMemory, nSize);
}

// Open play device
void PlaySpeexOpenDevice()
{
    R_DAC_Open(&g_dac_playback_ctrl, &g_dac_playback_cfg);
}

// Close play device
void PlaySpeexCloseDevice()
{
    R_DAC_Close(&g_dac_playback_ctrl);
}

void PlaySpeexStart()
{
    if (g_hRingBufferPlay == NULL)
    {
        if (RingBufferInit(g_byaRingBufferPlay, RING_BUFFER_GET_MEM_USAGE(RBUF_PLAY_SIZE),
                           RBUF_PLAY_SIZE, &g_hRingBufferPlay) != RING_BUFFER_SUCCESS)
            __BKPT(0);
    }

    R_DAC_Start(&g_dac_playback_ctrl);
    R_DAC_Write(&g_dac_playback_ctrl, 0x800);

    R_GPT_Open(&g_timer_playback_ctrl, &g_timer_playback_cfg);
    R_GPT_Start(&g_timer_playback_ctrl);
}

void PlaySpeexStop()
{
    R_GPT_Stop(&g_timer_playback_ctrl);
    R_GPT_Close(&g_timer_playback_ctrl);

    R_DAC_Stop(&g_dac_playback_ctrl);

    if (g_hRingBufferPlay != NULL)
    {
        RingBufferRelease(g_hRingBufferPlay);
        g_hRingBufferPlay = NULL;
    }
}

bool PlaySpeexMapID(const BYTE *lpbySpeexPack, int nMapID, int nVolumeScalePercentage)
{
    int nSpeexCount;

    nSpeexCount = GetSpeexPackCount(lpbySpeexPack);
    for (int i = 0; i < nSpeexCount; i++)
    {
        if (GetSpeexMapID(lpbySpeexPack, i) == nMapID)
        {
            return PlaySpeexIndex(lpbySpeexPack, i, nVolumeScalePercentage);
        }
    }

    return false;
}

// This function will use 640B extra stack size
bool PlaySpeexIndex(const BYTE *lpbySpeexPack, int nIndex, int nVolumeScalePercentage)
{
    short saSamples[SPEEX_FRAME_SAMPLE_16K];        //lpoSpeexDataHeader->nFrameSamples is 320 for 16 KHz.
    HANDLE hSpeexDecoder = NULL;
    short saSamples2[SPEEX_FRAME_SAMPLE_16K];
    HANDLE hR8Kto16K = NULL;
    int nSpeexCount;
    const BYTE* lpbyUnpack;
    DWORD dwUnpackBinSize;
    SpeexDataHeader *lpoSpeexDataHeader;
    BYTE *lpbyData;
    int nDataSize;
    int nErr;
    int n = 0;
    int nFreeSize = 0;
    int nSpeexFrameSize = SPEEX_FRAME_SAMPLE_16K*2;

    nSpeexCount = GetSpeexPackCount(lpbySpeexPack);
    if (nIndex < nSpeexCount)
    {
        lpbyUnpack = UnpackSpeex(lpbySpeexPack, nIndex, &dwUnpackBinSize, NULL);
        if (lpbyUnpack == NULL)
            return false;

        lpoSpeexDataHeader = (SpeexDataHeader *)lpbyUnpack;
        lpbyData = (BYTE*)lpbyUnpack + sizeof(SpeexDataHeader);
        nDataSize = (int)(dwUnpackBinSize - sizeof(SpeexDataHeader));
        if (SpeexDecoderCheckDataHeader(lpoSpeexDataHeader) != 0)
            return false;

        hSpeexDecoder = SpeexDecoderInit(lpoSpeexDataHeader->nChannelNum, lpoSpeexDataHeader->nSampleRate, lpoSpeexDataHeader->nBitRate, 1, &nErr);
        if (hSpeexDecoder == NULL)
            __BKPT(0);

        hR8Kto16K = R8Kto16K_Init(SPEEX_FRAME_SAMPLE_8K, NULL);
        if (hR8Kto16K == NULL)
            __BKPT(0);

        n = 0;
        while (n <= nDataSize - lpoSpeexDataHeader->nEncodeSize)
        {
            nErr = SpeexDecoderDo(hSpeexDecoder, lpbyData + n, lpoSpeexDataHeader->nEncodeSize, saSamples, lpoSpeexDataHeader->nFrameSamples);

            // Let Speex to produce interpolation frame if data corrupt.
            if (nErr < 0)
                SpeexDecoderDo(hSpeexDecoder, NULL, 0, saSamples, lpoSpeexDataHeader->nFrameSamples);

            n += lpoSpeexDataHeader->nEncodeSize;

            // Wait ring buffer has enough free size to put saSamples.
            while (1)
            {
                nFreeSize = RingBufferGetFreeSize(g_hRingBufferPlay);
                if (nFreeSize >= nSpeexFrameSize)
                    break;
            }

            if (lpoSpeexDataHeader->nSampleRate == 8000)
            {
                //For 8 KHz sample, we up-sampling to 16 KHz to play it.
                R8Kto16K_Resample(hR8Kto16K, saSamples, lpoSpeexDataHeader->nFrameSamples, saSamples2, SPEEX_FRAME_SAMPLE_16K);
                ScaleVolume(saSamples2, SPEEX_FRAME_SAMPLE_16K, nVolumeScalePercentage, saSamples);
            }
            else
            {
                ScaleVolume(saSamples, SPEEX_FRAME_SAMPLE_16K, nVolumeScalePercentage, saSamples);
            }

            RingBufferPutData(g_hRingBufferPlay, saSamples, SPEEX_FRAME_SAMPLE_16K*2);
        }

        SpeexDecoderRelease(hSpeexDecoder);
        R8Kto16K_Release(hR8Kto16K);
    }
    else
    {
        return false;
    }

    // Wait all ring buffer data played.
    while (1)
    {
        nDataSize = RingBufferGetDataSize(g_hRingBufferPlay);
        if (nDataSize == 0)
            break;
    }

    return true;
}

bool PlaySpeexIsDataValid(const BYTE *lpbySpeexPack, int nMapID, int *pnSampleRate)
{
    short nSpeexCount;
    const BYTE *lpbyUnpack;
    DWORD dwUnpackBinSize;
    SpeexDataHeader *lpoSpeexDataHeader;
    bool bRet = false;

    nSpeexCount = (short)GetSpeexPackCount(lpbySpeexPack);
    for (int i = 0; i < nSpeexCount; i++)
    {
        if (GetSpeexMapID(lpbySpeexPack, i) == nMapID)
        {
            lpbyUnpack = UnpackSpeex(lpbySpeexPack, i, &dwUnpackBinSize, NULL);
            if (lpbyUnpack == NULL)
                break;

            lpoSpeexDataHeader = (SpeexDataHeader *)lpbyUnpack;
            if (lpoSpeexDataHeader == NULL || lpoSpeexDataHeader->nChannelNum != 1 ||
                lpoSpeexDataHeader->nHeaderSize != sizeof(SpeexDataHeader) ||
                strcmp(lpoSpeexDataHeader->szSpeexVersion, SPEEX_VERSION) != 0 )
            {
                break;
            }

            if (pnSampleRate != NULL)
            {
                *pnSampleRate = lpoSpeexDataHeader->nSampleRate;
            }

            if (lpoSpeexDataHeader->nSampleRate != 16000 && lpoSpeexDataHeader->nSampleRate != 8000)
                break;

            if (SpeexDecoderCheckDataHeader(lpoSpeexDataHeader) != 0)
                break;

            bRet = true;
            break;
        }
    }

    return bRet;
}


/*
Packed bin format:

Bin Number ----- 4 bytes, assume its value is N.
Bin 1 size ----- 4 bytes, assume its value is nBin1Size.
...
Bin N size ----- 4 bytes, assume its value is nBinNSize.
Bin 1 ID   ----- 4 bytes
...
Bin N ID   ----- 4 bytes
Bin 1 data ----- nBin1Size bytes
...
Bin N data ----- nBinNSize bytes
*/
static const BYTE* UnpackSpeex(const BYTE *lpbyBin, int nIndex, DWORD *pdwUnpackBinSize, int *pnMapID)
{
    DWORD *lpnBin = (DWORD *)lpbyBin;
    DWORD nNumBin = lpnBin[0];
    DWORD *lpnBinSize = lpnBin + 1;
    int *lpnMapID = (int *)lpnBinSize + nNumBin;
    const BYTE *lpbyModel = NULL;
    DWORD i;

    lpbyModel = (const BYTE *)(lpnBinSize + nNumBin*2);
    if (pdwUnpackBinSize != NULL)
        *pdwUnpackBinSize = lpnBinSize[nIndex];
    if (pnMapID != NULL)
        *pnMapID = lpnMapID[nIndex];
    if (nIndex == 0)
        return lpbyModel;

    for (i = 1; i < nNumBin; i++)
    {
        lpbyModel = lpbyModel + lpnBinSize[i - 1];
        if (i == (DWORD)nIndex)
            break;
    }

    return lpbyModel;
}

static int GetSpeexPackCount(const BYTE *lpbyBin)
{
    int *lpnBin = (int *)lpbyBin;
    int nNumBin = lpnBin[0];
    return nNumBin;
}

static int GetSpeexMapID(const BYTE *lpbyBin, int nIndex)
{
    DWORD *lpnBin = (DWORD *)lpbyBin;
    DWORD nNumBin = lpnBin[0];
    DWORD *lpnBinSize = lpnBin + 1;
    int *lpnMapID = (int *)lpnBinSize + nNumBin;

    if (nIndex < (int)nNumBin)
        return lpnMapID[nIndex];
    else
        return -1;
}

static void ScaleVolume(short *lpsSamplesIn, int nSampleCount, int nVolumeScalePercentage, short *lpsSamplesOut)
{
    int nTemp;

    for (int i = 0; i < nSampleCount; i++)
    {
        nTemp = nVolumeScalePercentage * lpsSamplesIn[i] / 100;

        if (nTemp > 32767)
            nTemp = 32767;
        else if (nTemp < -32768)
            nTemp = -32768;

        lpsSamplesOut[i] = (short)nTemp;
    }
}
