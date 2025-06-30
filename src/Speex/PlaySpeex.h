#ifndef __PLAY_SPEEX_H_
#define __PLAY_SPEEX_H_

#include "base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Return the usage of working memory.
int PlaySpeexGetWorkingMemoryUsage(int nSampleRate);

// Use outside memory, otherwise, it will use 6KB heap, 2 KB stack.
void PlaySpeexSetWorkingMemory(char *lpMemory, int nSize);

// Open play device
void PlaySpeexOpenDevice();

// Close play device
void PlaySpeexCloseDevice();

// Start play device.
void PlaySpeexStart();

// Stop play device.
void PlaySpeexStop();

// Decode and play the Speex in the pack file by map ID.
bool PlaySpeexMapID(const BYTE *lpbySpeexPack, int nMapID, int nVolumeScalePercentage);

// Decode and play the Speex in the pack file by index.
bool PlaySpeexIndex(const BYTE *lpbySpeexPack, int nIndex, int nVolumeScalePercentage);

// Check the data format of lpbySpeexPack by map ID.
bool PlaySpeexIsDataValid(const BYTE *lpbySpeexPack, int nMapID, int *pnSampleRate);

#ifdef __cplusplus
}
#endif

#endif
