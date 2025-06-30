#ifndef __AUDIO_RECORD_H
#define __AUDIO_RECORD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AudioRecordInit - Initialize ring buffer and open I2S/SPI, timer... peripheral devices for recording.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
int AudioRecordInit();

/**
 * AudioRecordRelease - Release ring buffer and close I2S/SPI, timer... peripheral devices.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
int AudioRecordRelease(void);

/**
 * AudioRecordStart - Start record, the audio system will start to callback audio data.
 *                    The audio data will put into ring buffer when get it from callback.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
int AudioRecordStart(void);

/**
 * AudioRecordSop - Stop record, the audio system will stop to callback audio data.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
int AudioRecordStop(void);

/**
 * AudioRecordPause - Don't allow the audio record data to put into ring buffer.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
void AudioRecordPause(void);

/**
 * AudioRecordResume - Allow the audio record data to put into ring buffer.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
void AudioRecordResume(void);

/**
 * AudioRecordGetDataSize - Get the audio data size cached in the ring buffer.
 * Returns the audio data size on success, negative value on error.
 */
int AudioRecordGetDataSize(void);

/**
 * AudioRecordGetDatae - Get the audio data from the ring buffer.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
int AudioRecordGetData(void *lpBuffer, int nBufferSize);

/**
 * AudioRecordClearData - Clear all audio data in the ring buffer.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
int AudioRecordClearData(void);

/**
 * AudioRecordGetLostCount - Get the lost count of audio data.
 *                           If the ring buffer is full when we get audio
 *                           data callback, this data will be skipped and
 *                           the lost count will increase by one.
 * Returns 0(FSP_SUCCESS) on success, other value(FSP_ERR_XXX) on error.
 */
int AudioRecordGetLostCount(void);

int AudioRecordGetUnderRunCount(void);

#ifdef __cplusplus
}
#endif

#endif
