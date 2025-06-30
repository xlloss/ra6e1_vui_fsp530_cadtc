#include "voice_main.h"
#include "hal_data.h"

#include "base_types.h"
#include "AudioRecord.h"
#include "DbgTrace.h"
#include "RingBuffer.h"
#include "s_cache.h"

#define RECORD_FRAME_SIZE        320

#define RBUF_SIZE_RECORD         (RECORD_FRAME_SIZE*9)

static volatile bool g_bRecording = false;
static volatile bool g_bSkipRecordData = false;
static HANDLE g_hRingBuffer = NULL;
static BYTE   g_byaRingBuffer[RING_BUFFER_GET_MEM_USAGE(RBUF_SIZE_RECORD)];
static volatile int g_nRecordCount = 0;
static volatile int g_nRBufLostCount = 0;
static volatile int g_nUnderRunCount = 0;
static volatile int *g_pnRecordBuffer = NULL;

int AudioRecordInit()
{
    fsp_err_t  err = FSP_SUCCESS;
    int nRet;

    nRet = RingBufferInit(g_byaRingBuffer, sizeof(g_byaRingBuffer), RBUF_SIZE_RECORD, &g_hRingBuffer);
    if (nRet != RING_BUFFER_SUCCESS)
    {
        DBG_UART_TRACE("Record RingBufferInit() fail %d!\r\n", nRet);
        return FSP_ERR_OUT_OF_MEMORY;
    }

#if (AUDIO_RECORD == AUDIO_RECORD_I2S)
    /** GPT for I2S clock */
    err = R_GPT_Open(&g_i2s_clock_ctrl, &g_i2s_clock_cfg);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_GPT_Open\r\n");
        return err;
    }

    err = R_GPT_Start(&g_i2s_clock_ctrl);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_GPT_Start\r\n");
        R_GPT_Close(&g_i2s_clock_ctrl);
        return err;
    }

    /** I2S */
    err = R_SSI_Open(&g_i2s0_ctrl, &g_i2s0_cfg);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_SSI_Open\r\n");
        R_GPT_Stop(&g_i2s_clock_ctrl);
        R_GPT_Close(&g_i2s_clock_ctrl);
        return err;
    }

#elif (AUDIO_RECORD == AUDIO_RECORD_SPI)
    err = R_ELC_Open(&g_elc_ctrl, &g_elc_cfg);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_ELC_Open\r\n");
        __BKPT(0);
    }

    err = R_ELC_Enable(&g_elc_ctrl);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_ELC_Enable\r\n");
        __BKPT(0);
    }
    /** SW I2S */
    err = R_GPT_Open(&g_timer_sck_ctrl, &g_timer_sck_cfg);
    if(FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_GPT_Open: SCK\r\n");
        __BKPT(0);
    }

    err = R_GPT_Open(&g_timer_ws_ctrl, &g_timer_ws_cfg);
    if(FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_GPT_Open: WS\r\n");
        __BKPT(0);
    }

    /* Set initial counter value to supplement the missing clock of 32 times count-up in first cycle */
    err = R_GPT_CounterSet(&g_timer_ws_ctrl, g_timer_ws_cfg.period_counts - 1);
    if(FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_GPT_CounterSet: WS\r\n");
         __BKPT(0);
    }

    err = R_GPT_Start(&g_timer_ws_ctrl);
    if(FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_GPT_Start: WS\r\n");
        __BKPT(0);
    }

    err = R_SPI_Open(&g_spi_i2s_ctrl, &g_spi_i2s_cfg);
    if(FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_SPI_Open: I2S\r\n");
        __BKPT(0);
    }

    err = R_GPT_Start(&g_timer_sck_ctrl);
    if(FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_GPT_Start: SCK\r\n");
        __BKPT(0);
    }
#elif (AUDIO_RECORD == AUDIO_RECORD_AMIC)
    /** DMAC */

    R_DMAC_Open(&g_transfer_adc_ctrl, &g_transfer_adc_cfg);

    /** ADC */
    err = R_ADC_Open(&g_adc_amic_ctrl, &g_adc_amic_cfg);
    if(FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_ADC_Open: AMIC\r\n");
        __BKPT(0);
    }

    err = R_ADC_ScanCfg(&g_adc_amic_ctrl, &g_adc_amic_channel_cfg);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_ADC_ScanCfg: AMIC\r\n");
        __BKPT(0);
    }

    err = R_ADC_ScanStart(&g_adc_amic_ctrl);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_ADC_ScanStart: AMIC\r\n");
        __BKPT(0);
    }

    /** ELC */
    R_ELC_Open(&g_elc_ctrl, &g_elc_cfg);

    R_ELC_Enable(&g_elc_ctrl);

    /** AGT */
    err = R_AGT_Open(&g_timer_amic_ctrl, &g_timer_amic_cfg);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_AGT_Open: AMIC\r\n");
        __BKPT(0);
    }
#endif

    g_bRecording = false;
    g_bSkipRecordData = false;
    g_nRecordCount = 0;
    g_nRBufLostCount = 0;

    return err;
}

int AudioRecordRelease()
{
    AudioRecordStop();

#if (AUDIO_RECORD == AUDIO_RECORD_I2S)
    R_SSI_Close(&g_i2s0_ctrl);
    R_GPT_Stop(&g_i2s_clock_ctrl);
    R_GPT_Close(&g_i2s_clock_ctrl);
#elif (AUDIO_RECORD == AUDIO_RECORD_SPI)
    R_SPI_Close(&g_spi_i2s_ctrl);
    R_ELC_Disable(&g_elc_ctrl);
    R_ELC_Close(&g_elc_ctrl);
    R_GPT_Reset(&g_timer_ws_ctrl);
    R_GPT_Stop(&g_timer_ws_ctrl);
    R_GPT_Close(&g_timer_ws_ctrl);
    R_GPT_Stop(&g_timer_sck_ctrl);
    R_GPT_Close(&g_timer_sck_ctrl);
#elif (AUDIO_RECORD == AUDIO_RECORD_AMIC)
    R_AGT_Stop(&g_timer_amic_ctrl);
    R_AGT_Close(&g_timer_amic_ctrl);
    R_ELC_Disable(&g_elc_ctrl);
    R_ELC_Close(&g_elc_ctrl);
    R_ADC_ScanStop(&g_adc_amic_ctrl);
    R_ADC_Close(&g_adc_amic_ctrl);
    R_DMAC_Disable(&g_transfer_adc_ctrl);
    R_DMAC_Close(&g_transfer_adc_ctrl);
#endif

    RingBufferRelease(g_hRingBuffer);
    g_hRingBuffer = NULL;
    g_pnRecordBuffer = NULL;

    return FSP_SUCCESS;
}

int AudioRecordStart()
{
    fsp_err_t  err;
    void *lpFree1 = NULL;
    void *lpFree2 = NULL;
    int nFree1Size = 0;
    int nFree2Size = 0;

    g_bRecording = true;
    RingBufferReset(g_hRingBuffer);
    g_nRecordCount = 0;
    g_nRBufLostCount = 0;
    g_nUnderRunCount = 0;

    RingBufferGetFreeBuffer(g_hRingBuffer, RECORD_FRAME_SIZE, &lpFree1, &nFree1Size, &lpFree2, &nFree2Size);
    g_pnRecordBuffer = (int *)lpFree1;

#if (AUDIO_RECORD == AUDIO_RECORD_I2S)
    err = R_SSI_Read(&g_i2s0_ctrl, g_pnRecordBuffer, RECORD_FRAME_SIZE);
#elif (AUDIO_RECORD == AUDIO_RECORD_SPI)
    err = R_SPI_Read(&g_spi_i2s_ctrl, (void* const)g_pnRecordBuffer, RECORD_FRAME_SIZE/4, SPI_BIT_WIDTH_32_BITS);
#elif (AUDIO_RECORD == AUDIO_RECORD_AMIC)
    R_DMAC_Disable(&g_transfer_adc_ctrl);
    R_DMAC_Reset(&g_transfer_adc_ctrl,
                (void*)VD_PRV_ADC0_ADDR,
                (void*)g_pnRecordBuffer,
                RECORD_FRAME_SIZE/2);
    R_DMAC_Enable(&g_transfer_adc_ctrl);

    err = R_AGT_Start(&g_timer_amic_ctrl);
#endif

    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_SSI_Read\r\n");
        g_bRecording = false;
    }

    return err;
}

int AudioRecordStop()
{
    g_bRecording = false;
#if (AUDIO_RECORD == AUDIO_RECORD_AMIC)
    R_AGT_Stop(&g_timer_amic_ctrl);
#endif
    return FSP_SUCCESS;
}

void AudioRecordPause()
{
    g_bSkipRecordData = true;
}

void AudioRecordResume()
{
    g_bSkipRecordData = false;
}

int AudioRecordGetDataSize(void)
{
#if (AUDIO_RECORD == AUDIO_RECORD_I2S)
    // Audio is 16 bits data, but ring buffer store 32 bits data, so we need to divide by 2.
    return RingBufferGetDataSize(g_hRingBuffer) / 2;
#elif (AUDIO_RECORD == AUDIO_RECORD_SPI)
    // Audio is 16 bits data, but ring buffer store 32 bits data, so we need to divide by 2.
    return RingBufferGetDataSize(g_hRingBuffer) / 2;
#elif (AUDIO_RECORD == AUDIO_RECORD_AMIC)
    return RingBufferGetDataSize(g_hRingBuffer);
#endif
}

int AudioRecordGetData(void *lpBuffer, int nBufferSize)
{
    // Must be 16 bits audio data.
    if (nBufferSize % 2 != 0)
        return -FSP_ERR_INVALID_ARGUMENT;

#if (AUDIO_RECORD == AUDIO_RECORD_I2S)
    void *lpData1;
    void *lpData2;
    int nData1Size, nData2Size;
    int32_t *pnDataBuffer;
    short *psDestBuffer = (short *)lpBuffer;
    int i;

    // Ring buffer 32 bits data shall have double size.
    if (RingBufferGetDataSize(g_hRingBuffer) < nBufferSize * 2)
        return -FSP_ERR_INVALID_SIZE;

    if (RingBufferGetDataBuffer(g_hRingBuffer, nBufferSize * 2, &lpData1, &nData1Size, &lpData2, &nData2Size) != RING_BUFFER_SUCCESS)
        return -FSP_ERR_INVALID_SIZE;

    if (nData1Size % 4 != 0 || nData2Size % 4 != 0)
        return -FSP_ERR_ASSERTION;

    pnDataBuffer = (int32_t *)lpData1;
    nData1Size >>= 2;
    for (i = 0; i < nData1Size; i++)
        *psDestBuffer++ = (short)(pnDataBuffer[i] >> 16);

    pnDataBuffer = (int32_t *)lpData2;
    nData2Size >>= 2;
    for (i = 0; i < nData2Size; i++)
        *psDestBuffer++ = (short)(pnDataBuffer[i] >> 16);

    RingBufferDequeueData(g_hRingBuffer, nBufferSize * 2);
#elif (AUDIO_RECORD == AUDIO_RECORD_SPI)
    void *lpData1;
    void *lpData2;
    int nData1Size, nData2Size;
    int32_t *pnDataBuffer;
    short *psDestBuffer = (short *)lpBuffer;
    int i;

    // Ring buffer 32 bits data shall have double size.
    if (RingBufferGetDataSize(g_hRingBuffer) < nBufferSize * 2)
        return -FSP_ERR_INVALID_SIZE;

    if (RingBufferGetDataBuffer(g_hRingBuffer, nBufferSize * 2, &lpData1, &nData1Size, &lpData2, &nData2Size) != RING_BUFFER_SUCCESS)
        return -FSP_ERR_INVALID_SIZE;

    if (nData1Size % 4 != 0 || nData2Size % 4 != 0)
        return -FSP_ERR_ASSERTION;

    pnDataBuffer = (int32_t *)lpData1;
    nData1Size >>= 2;
    for (i = 0; i < nData1Size; i++)
        *psDestBuffer++ = (short)(pnDataBuffer[i] >> 15);

    pnDataBuffer = (int32_t *)lpData2;
    nData2Size >>= 2;
    for (i = 0; i < nData2Size; i++)
        *psDestBuffer++ = (short)(pnDataBuffer[i] >> 15);

    RingBufferDequeueData(g_hRingBuffer, nBufferSize * 2);
#elif (AUDIO_RECORD == AUDIO_RECORD_AMIC)
    int nSamples = nBufferSize / 2;
    short *lpsData = (short *)lpBuffer;
    int32_t tmp32;

    if (RingBufferGetData(g_hRingBuffer, lpBuffer, nBufferSize) != RING_BUFFER_SUCCESS)
       return -FSP_ERR_INVALID_SIZE;

    for (int i = 0; i < nSamples; i++)
    {
        tmp32 = lpsData[i];
        tmp32 = (tmp32 - (0x0FFF / 2)); /* 12bit AD */
        if (tmp32 < VD_PRV_INT16_MIN)
        {
            lpsData[i] = VD_PRV_INT16_MIN;
        }
        else if (tmp32 > VD_PRV_INT16_MAX)
        {
            lpsData[i] = VD_PRV_INT16_MAX;
        }
        else
        {
            /** int32_t -> int16_t */
            lpsData[i] = (int16_t)tmp32;
        }
    }
#endif

    return FSP_SUCCESS;
}

int AudioRecordClearData()
{
    if (RingBufferDequeueData(g_hRingBuffer, RingBufferGetDataSize(g_hRingBuffer)) != RING_BUFFER_SUCCESS)
        return FSP_ERR_INVALID_SIZE;

    return FSP_SUCCESS;
}

int AudioRecordGetLostCount(void)
{
    return g_nRBufLostCount;
}

int AudioRecordGetUnderRunCount(void)
{
    return g_nUnderRunCount;
}

#if (AUDIO_RECORD == AUDIO_RECORD_I2S)

/* Callback function */
void g_audio_cb(i2s_callback_args_t *p_args)
{
    fsp_err_t err = FSP_SUCCESS;

    switch(p_args->event)
    {
        case I2S_EVENT_IDLE: ///< Communication is idle
            if (g_bRecording)
            {
                // Note: The audio recording may interrupt by some reason(system busy, printf at audio thread...),
                // so we need to resume it, but this also cause burst noise in record data.
                // Note: Don't printf() here, it will cause record fail and buffer overflow.
                g_nUnderRunCount++;
                err = R_SSI_Read(&g_i2s0_ctrl, g_pnRecordBuffer, RECORD_FRAME_SIZE);
                if (FSP_SUCCESS != err)
                {
                    DBGTRACE("i2s re-start read fail. err = 0x%x\r\n", err);
                    ToggleLED(LED_B);
                }
            }
            else
            {
                // Normal stop.
                int nDataSize = RingBufferGetDataSize(g_hRingBuffer);
                DBGTRACE("callback: I2S_EVENT_IDLE %d\r\n", nDataSize);
            }
            break;

        case I2S_EVENT_TX_EMPTY: ///< Transmit buffer is below FIFO trigger level
            DBGTRACE("callback: I2S_EVENT_TX_EMPTY\r\n");
            break;

        case I2S_EVENT_RX_FULL: ///< Receive buffer is above FIFO trigger level
            // g_pnRecordBuffer has new record data.
            if (g_bRecording)
            {
                if (g_bSkipRecordData)
                {
                    // Don't call RingBufferEnqueueData() to enlarge data size.
                    // Use g_pnRecordBuffer to record repeatedly.
                }
                else
                {
                    int nFreeSize;
                    void *lpFree1 = NULL;
                    void *lpFree2 = NULL;
                    int nFree1Size = 0;
                    int nFree2Size = 0;

                    RingBufferEnqueueData(g_hRingBuffer, RECORD_FRAME_SIZE);
                    nFreeSize = RingBufferGetFreeSize(g_hRingBuffer);
                    if (nFreeSize < (int)RECORD_FRAME_SIZE)
                    {
                        g_nRBufLostCount++;
                        RingBufferDequeueData(g_hRingBuffer, RECORD_FRAME_SIZE);
                    }
                    RingBufferGetFreeBuffer(g_hRingBuffer, RECORD_FRAME_SIZE, &lpFree1, &nFree1Size, &lpFree2, &nFree2Size);
                    g_pnRecordBuffer = (int *)lpFree1;
                    if (lpFree2 != NULL || nFree2Size != 0)
                        __BKPT(0);
                }

                err = R_SSI_Read(&g_i2s0_ctrl, g_pnRecordBuffer, RECORD_FRAME_SIZE);
                if (FSP_SUCCESS != err)
                {
                    //DBGTRACE("i2s continue read fail! err = 0x%x\r\n", err);
                    ToggleLED(LED_B);
                }
            }

            g_nRecordCount++;
            break;

        default:
            break;
    }
}

#elif (AUDIO_RECORD == AUDIO_RECORD_SPI)

void g_spi_cb(spi_callback_args_t *p_args)
{
    if (NULL != p_args)
    {
        /* capture callback event for validating the i2s transfer event*/
        if (p_args->event == SPI_EVENT_TRANSFER_COMPLETE)
        {
            flush_s_cache();
            // g_pnRecordBuffer has new record data.
            if (g_bRecording)
            {
                if (g_bSkipRecordData)
                {
                    // Don't call RingBufferEnqueueData() to enlarge data size.
                    // Use g_pnRecordBuffer to record repeatedly.
                }
                else
                {
                    int nFreeSize;
                    void *lpFree1 = NULL;
                    void *lpFree2 = NULL;
                    int nFree1Size = 0;
                    int nFree2Size = 0;

                    RingBufferEnqueueData(g_hRingBuffer, RECORD_FRAME_SIZE);
                    nFreeSize = RingBufferGetFreeSize(g_hRingBuffer);
                    if (nFreeSize < RECORD_FRAME_SIZE)
                    {
                        g_nRBufLostCount++;
                        RingBufferDequeueData(g_hRingBuffer, RECORD_FRAME_SIZE);
                    }
                    RingBufferGetFreeBuffer(g_hRingBuffer, RECORD_FRAME_SIZE, &lpFree1, &nFree1Size, &lpFree2, &nFree2Size);
                    g_pnRecordBuffer = (int *)lpFree1;
                    if (lpFree2 != NULL || nFree2Size != 0)
                        __BKPT(0);
                }

                R_SPI_Read(&g_spi_i2s_ctrl, (void* const)g_pnRecordBuffer, RECORD_FRAME_SIZE/4, SPI_BIT_WIDTH_32_BITS);
            }

            g_nRecordCount++;
        }
        else if (p_args->event == SPI_EVENT_ERR_MODE_UNDERRUN)
        {
            if (g_bRecording)
            {
                g_nUnderRunCount++;
                R_SPI_Close(&g_spi_i2s_ctrl);
                R_SPI_Open(&g_spi_i2s_ctrl, &g_spi_i2s_cfg);
                R_SPI_Read(&g_spi_i2s_ctrl, (void* const)g_pnRecordBuffer, RECORD_FRAME_SIZE/4, SPI_BIT_WIDTH_32_BITS);
            }
        }
    }
    return;
}

#elif (AUDIO_RECORD == AUDIO_RECORD_AMIC)

/**
* DMA0 callback function.\n
* Give the semaphore (g_wait_for_dma_callback_semaphore) in the function.
*
* @param[in] p_args The DMA callback parameter.
*/
void cb_dma_end(dmac_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    flush_s_cache();

    // g_pnRecordBuffer has new record data.
    if (g_bRecording)
    {
        if (g_bSkipRecordData)
        {
            // Don't call RingBufferEnqueueData() to enlarge data size.
            // Use g_pnRecordBuffer to record repeatedly.
        }
        else
        {
            int nFreeSize;
            void *lpFree1 = NULL;
            void *lpFree2 = NULL;
            int nFree1Size = 0;
            int nFree2Size = 0;

            // Enlarge data size.
            RingBufferEnqueueData(g_hRingBuffer, RECORD_FRAME_SIZE);
            nFreeSize = RingBufferGetFreeSize(g_hRingBuffer);
            if (nFreeSize < RECORD_FRAME_SIZE)
            {
                g_nRBufLostCount++;
                RingBufferDequeueData(g_hRingBuffer, RECORD_FRAME_SIZE);
            }
            RingBufferGetFreeBuffer(g_hRingBuffer, RECORD_FRAME_SIZE, &lpFree1, &nFree1Size, &lpFree2, &nFree2Size);
            g_pnRecordBuffer = (int *)lpFree1;
            if (lpFree2 != NULL || nFree2Size != 0)
                __BKPT(0);
        }

        /* DMA0 */
        R_DMAC_Disable(&g_transfer_adc_ctrl);
        R_DMAC_Reset(&g_transfer_adc_ctrl,
                        (void*)VD_PRV_ADC0_ADDR,
                        (void*)g_pnRecordBuffer,
                        RECORD_FRAME_SIZE/2);
        R_DMAC_Enable(&g_transfer_adc_ctrl);
    }
    g_nRecordCount++;
}

#endif
