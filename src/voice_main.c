/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer *
* Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/

/*******************************************************************************
 Include
*******************************************************************************/

#include "voice_main.h"
#include "hal_data.h"

#include "base_types.h"
#include "DSpotterSDKApi_Const.h"
#include "DSpotterSDKApi.h"
#include "DSpotterSDTrainApi.h"
#include "RingBuffer.h"
#include "AGCApi.h"

#include "Convert2TransferBuffer.h"
#include "DbgTrace.h"
#include "UartMgr.h"
#include "FlashMgr.h"
#include "PortFunction.h"
#include "CybModelInfor.h"
#include "Model/CybModel1.h"
#ifdef SUPPORT_RECOG_TWO_MODEL
#include "Model/CybModel2.h"
#endif

#if defined(SUPPORT_SPEEX_PLAY) || defined(SUPPORT_VOICE_TAG)
#include "Speex/PlaySpeex.h"
#include "Model/SpeexData1.h"
#ifdef SUPPORT_RECOG_TWO_MODEL
#include "Model/SpeexData2.h"
#endif
static void PlaySpeexByID(const BYTE *pbySpeexDataBegin, int nMapID);
#endif

#include "AudioRecord.h"

/*******************************************************************************
 Macro definitions
*******************************************************************************/

/** Data flash */
#define FLASH_DF_BLOCK_ADDRESS(nBlockIndex)     (BYTE*)(FLASH_DF_BLOCK_BASE_ADDRESS + nBlockIndex*FLASH_DF_BLOCK_SIZE)
#define FLASH_DF_TOTAL_BLOCKS                   (FLASH_DF_SIZE/FLASH_DF_BLOCK_SIZE)

#define DSPOTTER_LICENSE_MAX_SIZE               256
#define FLASH_DSPOTTER_ADDRESS_OFFSET           0
#define FLASH_DSPOTTER_BLOCK_INDEX              ((FLASH_DSPOTTER_ADDRESS_OFFSET + FLASH_DF_BLOCK_SIZE - 1)/FLASH_DF_BLOCK_SIZE)
#define FLASH_DSPOTTER_BLOCK_COUNT              ((DSPOTTER_LICENSE_MAX_SIZE + FLASH_DF_BLOCK_SIZE - 1)/FLASH_DF_BLOCK_SIZE)

/** DSpotter */
#ifdef SUPPORT_VOICE_TAG
    #define DSPOTTER_MEM_SIZE_1      80000               // Please modify this number by the return value of DSpotter_GetMemoryUsage_XXX().
#else
    #define DSPOTTER_MEM_SIZE_1      50000               // Please modify this number by the return value of DSpotter_GetMemoryUsage_XXX().
#endif

#define DSPOTTER_MEM_SIZE_2      50000                   // Please modify this number by the return value of DSpotter_GetMemoryUsage_XXX().

#ifndef MAX_COMMAND_TIME
#define MAX_COMMAND_TIME         (8000/10)               // Trigger and command must be spoke within 8000 milli-second (800 frames).
#endif
#define DSPOTTER_FRAME_SIZE      960                     // DSpotter compute every 30 milli-second.
#define RECORD_FRAME_SIZE        320                     // Must be multiple times of 4, 320 is equal to 10 milli-second data. Must be 320 for VAD.
#define RECORD_FRAME_SAMPLES     (RECORD_FRAME_SIZE/2)
#if AUDIO_RECORD == AUDIO_RECORD_AMIC
    #define VOLUME_SCALE_RECONG      1600                // The AGC volume scale percentage for recognition. It depends on original microphone data.
#else
    #define VOLUME_SCALE_RECONG      3200                // The AGC volume scale percentage for recognition. It depends on original microphone data.
#endif
#define VOLUME_SCALE_VOICE_TAG   400                     // The AGC volume scale percentage for record voice tag (near field).
#define COMMAND_STAGE_TIME_MIN   6000                    // When no result at command recognition stage, the minimum recording time in milli-second.
#define COMMAND_STAGE_TIME_MAX   8000                    // When no result at command recognition stage, the maximum recording time in milli-second.
#define VOICE_TAG_MAX_TIME       2200                    // The max speech time of a voice tag in milli-second.
#define VOICE_TAG_VOICE_SIZE     (1024*16*VOICE_TAG_MAX_TIME/3000)  // According SDK document, voice tag length can be 3000*12/16 milli-second.
#define VOICE_TAG_MODEL_SIZE     5120                    // Voice tag model, it has 340B header(H) and 400B for each voice tag(T).
#define AGC_MEM_SIZE             64                      // AGC memory size.
#define RECORD_VOICE_TAG_ID      10000                   // It shall be same as the defined value in DSMT project.
#define DELETE_VOICE_TAG_ID      10001                   // It shall be same as the defined value in DSMT project.
#define DELETE_ALL_VOICE_TAG_ID  10002                   // It shall be same as the defined value in DSMT project.

/*******************************************************************************
 Private global functions
*******************************************************************************/

static bool     voice_loop(void);
static void     voice_init(void);
static void     voice_release(void);
static bool     ds_decode_init(void);
static int      TestByteOrder();
static void     InitDSpotter(HANDLE hCyModel, int nGroupIndex, BYTE *pbyaDSpotterMem, int nMemSize, HANDLE *phDSpotter, BOOL bShowInfo);
static void     ReleaseDSpotter(HANDLE *lphDSpotter);
static void     SelectGroupModel(HANDLE hCyModel, int nGroupIndex, HANDLE hDSpotter, BOOL bShowInfo);
static void     PrintGroupCommandList(HANDLE hCybModel, int nGroupIndex);
static void     OnDataReadCompleteCallback(void);
static BOOL     CheckGroupHasID(HANDLE hDSpotter, HANDLE hCybModel, int nGroupIndex, int nMapID);


/*******************************************************************************
 Private global variables
*******************************************************************************/

static HANDLE   g_hDSpotter1 = NULL;
static BYTE     g_byaDSpotterMem1[DSPOTTER_MEM_SIZE_1];     // The memory for DSpotter engine.
static HANDLE   g_hCybModel1 = NULL;
static BYTE     g_byaCybModelMem1[CYBMODEL_GET_MEM_USAGE()];// The memory for g_hCybModel1 engine.
static volatile int g_nActiveGroupIndex;
static HANDLE   g_hAGC = NULL;
static BYTE     g_byaAGCMem[AGC_MEM_SIZE];
static int      g_nRecordFrameCount = 0;
static volatile bool g_bUartCheckAlive = false;
static BYTE     g_byaUartRxBuffer[1];
static volatile bool g_bVoiceTagRecording = false;
static volatile bool g_bVoiceTagDeleting = false;

#ifdef SUPPORT_UART_DUMP_RECORD
#define ALIGN(num, size)         (((num) + (size) - 1) / (size) * (size))
#define CHECKSUM_TYPE  eFourByteDataOneChecksum
static volatile bool g_bUartSendRecord = false;
#if (CHECKSUM_TYPE == eTwoByteDataOneChecksum)
    static BYTE     g_byaTxBuffer[ALIGN(RECORD_FRAME_SIZE*3/2, 32)]; //Align the UART TX buffer to be multiple of 32-byte
#elif (CHECKSUM_TYPE == eFourByteDataOneChecksum)
    static BYTE     g_byaTxBuffer[ALIGN(RECORD_FRAME_SIZE*5/4, 32)]; //Align the UART TX buffer to be multiple of 32-byte
#endif
#endif

#ifdef SUPPORT_RECOG_TWO_MODEL
static HANDLE   g_hDSpotter2 = NULL;
static BYTE     g_byaDSpotterMem2[DSPOTTER_MEM_SIZE_2];     // The memory for DSpotter engine.
static HANDLE   g_hCybModel2 = NULL;
static BYTE     g_byaCybModelMem2[CYBMODEL_GET_MEM_USAGE()];// The memory for g_hCybModel2 engine.
#endif

#ifdef SUPPORT_VOICE_TAG
#define FLASH_VOICE_TAG_ADDRESS_OFFSET          1024
#define FLASH_VOICE_TAG_BLOCK_INDEX             ((FLASH_VOICE_TAG_ADDRESS_OFFSET + FLASH_DF_BLOCK_SIZE - 1)/FLASH_DF_BLOCK_SIZE)
#define FLASH_VOICE_TAG_BLOCK_COUNT             ((VOICE_TAG_MODEL_SIZE + FLASH_DF_BLOCK_SIZE - 1)/FLASH_DF_BLOCK_SIZE)

#define PLAY_ID_START_RECORD_VTG                10010 // It shall be same as the defined value in WaveToSpeexPack project.
#define PLAY_ID_NO_CLEAR_VOICE                  10011 // It shall be same as the defined value in WaveToSpeexPack project.
#define PLAY_ID_RECORD_VTG_OK                   10012 // It shall be same as the defined value in WaveToSpeexPack project.
#define PLAY_ID_ALL_VTG_DELETED                 10020 // It shall be same as the defined value in WaveToSpeexPack project.
#define PLAY_ID_COMMAND_ALL_VTG_DELETED         10021 // It shall be same as the defined value in WaveToSpeexPack project.
#define PLAY_ID_VTG_DELETED                     10022 // It shall be same as the defined value in WaveToSpeexPack project.
#define PLAY_ID_COMMAND_NO_VTG                  10023 // It shall be same as the defined value in WaveToSpeexPack project.

static volatile bool g_bVoiceTagButtonPressed = false;
static BYTE g_byaSDVoice[VOICE_TAG_VOICE_SIZE];       // The memory for store DSpotter SD record data
static BYTE g_byaSDModel[VOICE_TAG_MODEL_SIZE];       // The memory for store DSpotter SD trained model, 300 bytes header, 100~300 bytes for every voice tag.

static int  SD_Delete(int nMapIDorIndex, BOOL bMapID);
static int  SD_Train(int nMapID, short *lpsRecordSample, const short *lpsInputSample, int nSampleCount);
static int  SD_Delete_All();
static void EnableAllGroupModel(HANDLE hCybModel, HANDLE hDSpotter);
static void SaveSDModel();
static void PrintSDModelInfor();
#endif

/*******************************************************************************
* Function Name   : voice_main
* Description     : Voice application main process
* Arguments       : none
* Return value    : none
*******************************************************************************/
void voice_main(void)
{
    int nRet = true;

    TM1637_init(1/*enable*/, 1/*brightness*/);
    TM1637_clear();

    voice_init();
    AHT10_Reset();

    while (true == nRet)
    {
        nRet = voice_loop();
#ifdef INC_FREERTOS_H
        vTaskDelay (1); // 1 tick
        //vTaskDelay(pdMS_TO_TICKS(1)); // 1 ms.
        //At current time: pdMS_TO_TICKS(1) = 1
#endif
    }

    //Go to here when DSPOTTER_ERR_Expired (over trial limit).
    voice_release();
}
/*******************************************************************************
 End of function voice_main
*******************************************************************************/

/*******************************************************************************
* Function Name: voice_init
* Description  : Initialize
* Arguments    : none
* Return Value : none
*******************************************************************************/
static void voice_init(void)
{
    fsp_err_t   err = FSP_SUCCESS;

    /** UART for DSpotter */
    err = UartOpen(&g_uart_ds, g_uart_ds.p_cfg->channel, -1);   //baud rate: use configuration.xml setting
    if (FSP_SUCCESS != err)
        DBGTRACE("Failed to open UART %d! error: %d\r\n", g_uart_ds.p_cfg->channel, err);

#ifndef MCU_BOARD
    DBG_UART_TRACE("\r\n[Error] Please define MCU_BOARD first.\r\n");
    __BKPT(0);
#endif

    /** LED */
    TurnOffLED(LED_R);       //Red     = OFF
    TurnOffLED(LED_G);       //Green   = OFF
    TurnOffLED(LED_B);       //Yellow  = OFF

    //DBG_UART_TRACE("voice_init() Start\r\n");

    /** Cyberon DSpotter */
    ds_decode_init();

#ifdef SUPPORT_VOICE_TAG
    err = R_ICU_ExternalIrqOpen(&g_irq_button_voice_tag_ctrl, &g_irq_button_voice_tag_cfg);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_ICU_ExternalIrqOpen\r\n");
        __BKPT(0);
    }

    err = R_ICU_ExternalIrqEnable(&g_irq_button_voice_tag_ctrl);
    if (FSP_SUCCESS != err)
    {
        DBG_UART_TRACE("\r\n[Error] R_ICU_ExternalIrqEnable\r\n");
        __BKPT(0);
    }
#endif
    err = AudioRecordInit();
    if (FSP_SUCCESS != err)
        __BKPT(0);
    err = AudioRecordStart();
    if (FSP_SUCCESS != err)
        __BKPT(0);

    TurnOnLED(LED_R);     //Red   = ON

    // Process UART read.
    UartAsyncRead(g_byaUartRxBuffer, 1, OnDataReadCompleteCallback);

    //DBG_UART_TRACE("voice_init() End\r\n");
}
/*******************************************************************************
 End of function voice_init
*******************************************************************************/

/*******************************************************************************
* Function Name   : voice_loop
* Description     : Voice application process
* Arguments       : none
* Return value    : none
*******************************************************************************/
static bool voice_loop(void)
{
    short   saRecordSample[RECORD_FRAME_SAMPLES];
    int     nRet1 = DSPOTTER_ERR_NeedMoreSample;
    int     nRet2 = DSPOTTER_ERR_NeedMoreSample;
    static  int s_nRBufLostCount = 0;
    static  int s_nUnderRunCount = 0;
    static  int s_nLedTurnOnCount = 0;
    static  int s_nCommandRecordSample = 0;
    static  int s_nCommandRecognizeLimit = COMMAND_STAGE_TIME_MIN;

#ifdef SUPPORT_VOICE_TAG
    if (g_bVoiceTagButtonPressed)
    {
        g_bVoiceTagButtonPressed = false;
        if (!g_bVoiceTagRecording)
        {
            g_bVoiceTagRecording = true;
            DBG_UART_TRACE("Please say the command name to record the voice tag.\r\n");
            PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, RECORD_VOICE_TAG_ID);
            EnableAllGroupModel(g_hCybModel1, g_hDSpotter1);
        #ifdef SUPPORT_RECOG_TWO_MODEL
            ReleaseDSpotter(&g_hDSpotter2);
        #endif
        }
        return true;
    }
#endif

    if (g_bUartCheckAlive)
    {
        const char *lpszAck = "live\r\n";
        g_bUartCheckAlive = false;
        UartWaitWriteReady();
        UartAsyncWrite((const BYTE *)lpszAck, strlen(lpszAck));
    }

    if (AudioRecordGetDataSize() < RECORD_FRAME_SIZE)
        return true;

    if (g_nRecordFrameCount++ % 100 == 0)
    {
        DBG_UART_TRACE(".");
        TurnOffLED(LED_B);
    }

    // If the blue LED keeps flashing, it means that the computing power is insufficient
    // and the recording data is lost, please reduce the command number.
    if (s_nRBufLostCount != AudioRecordGetLostCount())
    {
        s_nRBufLostCount = AudioRecordGetLostCount();
        ToggleLED(LED_B);
        //DBG_UART_TRACE("\r\nLost = %d.\r\n", s_nRBufLostCount);
    }

    if (s_nUnderRunCount != AudioRecordGetUnderRunCount())
    {
        s_nUnderRunCount = AudioRecordGetUnderRunCount();
        ToggleLED(LED_B);
        //DBG_UART_TRACE("\r\nUnderRun = %d.\r\n", s_nUnderRunCount);
    }

    if (--s_nLedTurnOnCount == 0)
        TurnOnLED(LED_R);

    // Get recognize data from recognize ring buffer.
    if (AudioRecordGetData((void*)saRecordSample, RECORD_FRAME_SIZE) != 0)
        DBG_UART_TRACE("\r\nAudioRecordGetData() problem \r\n");

#if VOLUME_SCALE_RECONG != 100
    AGC_Do(g_hAGC, saRecordSample, RECORD_FRAME_SAMPLES, saRecordSample);
#endif

#ifdef SUPPORT_UART_DUMP_RECORD
    if (g_bUartSendRecord)
    {
        int nTransferSize;
        nTransferSize = Convert2TransferBuffer((BYTE *)saRecordSample, RECORD_FRAME_SIZE, g_byaTxBuffer, sizeof(g_byaTxBuffer), CHECKSUM_TYPE);
        UartWaitWriteReady();
        UartAsyncWrite(g_byaTxBuffer, (uint32_t)nTransferSize);
    }
#endif

#if RECOG_FLOW == RECOG_FLOW_NONE
    return true;
#endif

    // DSpotter AddSample
    nRet1 = DSpotter_AddSample(g_hDSpotter1, saRecordSample, RECORD_FRAME_SAMPLES);
#ifdef SUPPORT_RECOG_TWO_MODEL
    nRet2 = DSpotter_AddSample(g_hDSpotter2, saRecordSample, RECORD_FRAME_SAMPLES);
#endif
    if (nRet1 == DSPOTTER_SUCCESS || nRet2 == DSPOTTER_SUCCESS)
    {
        int  nCmdIndex, nCmdScore, nCmdSGDiff, nCmdEnergy, nMapID;
        char szCommand[64];
        bool bVoiceTag = false;
        int nTriggerWordCount = 0;
        int nCommandWordCount = 0;

        DBG_UART_TRACE("\r\n");
        szCommand[0] = 0;
        nMapID = -100;
        if (nRet1 == DSPOTTER_SUCCESS)
        {
            nTriggerWordCount = CybModelGetCommandCount(g_hCybModel1, 0);
            nCommandWordCount = CybModelGetCommandCount(g_hCybModel1, 1);

            if (nCommandWordCount < 0)
                nCommandWordCount = 0;
            DSpotter_GetResultScore(g_hDSpotter1, &nCmdScore, &nCmdSGDiff, NULL);
            nCmdIndex = DSpotter_GetResult(g_hDSpotter1);
            nCmdEnergy = DSpotter_GetCmdEnergy(g_hDSpotter1);
            DSpotter_Continue(g_hDSpotter1);
        #ifdef SUPPORT_RECOG_TWO_MODEL
            DSpotter_Continue(g_hDSpotter2);
        #endif

            //Change the multi-model command index to active group model command index.
            if (nCmdIndex < nTriggerWordCount)
            {
                CybModelGetCommandInfo(g_hCybModel1, 0, nCmdIndex, szCommand, sizeof(szCommand), &nMapID, NULL);
            }
            else if (nCmdIndex >= nTriggerWordCount && nCmdIndex < nTriggerWordCount + nCommandWordCount)
            {
                CybModelGetCommandInfo(g_hCybModel1, 1, nCmdIndex - nTriggerWordCount, szCommand, sizeof(szCommand), &nMapID, NULL);
            }
            else
            {
            #ifdef SUPPORT_VOICE_TAG
                int nSDNumCmd;
                int nSDIndex;

                bVoiceTag = true;
                nSDIndex = nCmdIndex - nTriggerWordCount - nCommandWordCount;
                nSDNumCmd = DSpotter_GetNumWord(g_byaSDModel);
                if (nSDIndex >= 0 && nSDIndex < nSDNumCmd)
                {
                    SHORT *lpsMapID;
                    lpsMapID = DSpotter_GetModelMapIDList(g_byaSDModel);
                    nMapID = lpsMapID[nSDIndex];
                    CybModelGetCommandInfoByID(g_hCybModel1, nMapID, NULL, NULL, szCommand, 64);
                }
                else
                {
                    DBG_UART_TRACE("SD command index error! nSDIndex = %d, nSDNumCmd = %d\r\n", nSDIndex, nSDNumCmd);
                }
                //DBG_UART_TRACE("Get voice tag, Score=%d, SG_Diff=%d, Energy=%d\r\n", nCmdScore, nCmdSGDiff, nCmdEnergy);
            #else
                return true;
            #endif
            }

            #ifdef NOT_SHOW_MULTI_PRONUNCIATION
            if (strchr(szCommand, '^') != NULL)
                strchr(szCommand, '^')[0] = '\0';
            #endif

            if (CheckGroupHasID(g_hDSpotter1, g_hCybModel1, g_nActiveGroupIndex, nMapID) || g_bVoiceTagRecording || g_bVoiceTagDeleting)
            {
                if (!bVoiceTag)
                {
                    DBG_UART_TRACE("Get %s word %s: Index=%d, Score=%d, SG_Diff=%d, Energy=%d, MapID=%d\r\n",
                        (g_nActiveGroupIndex == 0) ? "trigger" : "command", szCommand, nCmdIndex, nCmdScore, nCmdSGDiff, nCmdEnergy, nMapID);
                }
                else
                {
                    DBG_UART_TRACE("Get voice tag on %s: SD Index=%d, Score=%d, SG_Diff=%d, Energy=%d, MapID=%d\r\n",
                        szCommand, nCmdIndex - nTriggerWordCount - nCommandWordCount, nCmdScore, nCmdSGDiff, nCmdEnergy, nMapID);
                }
            }
            else
            {
                // Recognition at wrong stage.
                return true;
            }
        }
#ifdef SUPPORT_RECOG_TWO_MODEL
        else
        {
            DSpotter_GetResultScore(g_hDSpotter2, &nCmdScore, &nCmdSGDiff, NULL);
            nCmdIndex = DSpotter_GetResult(g_hDSpotter2);
            nCmdEnergy = DSpotter_GetCmdEnergy(g_hDSpotter2);
            DSpotter_Continue(g_hDSpotter1);
            DSpotter_Continue(g_hDSpotter2);
            //Change the multi-model command index to active group model command index.
            for (int i = 0; i < g_nActiveGroupIndex; i++)
                nCmdIndex -= CybModelGetCommandCount(g_hCybModel2, i);
            CybModelGetCommandInfo(g_hCybModel2, g_nActiveGroupIndex, nCmdIndex, szCommand, sizeof(szCommand), &nMapID, NULL);
        #ifdef NOT_SHOW_MULTI_PRONUNCIATION
            if (strchr(szCommand, '^') != NULL) {
                strchr(szCommand, '^')[0] = '\0';
            }
        #endif
            if (CheckGroupHasID(g_hDSpotter2, g_hCybModel2, g_nActiveGroupIndex, nMapID) || g_bVoiceTagRecording || g_bVoiceTagDeleting)
            {
                DBG_UART_TRACE("Get %s word(%d): %s, Score=%d, SG_Diff=%d, Energy=%d, MapID=%d\r\n",
                    (g_nActiveGroupIndex == 0) ? "trigger" : "command", nCmdIndex, szCommand, nCmdScore, nCmdSGDiff, nCmdEnergy, nMapID);
            }
            else
            {
                // Recognition at wrong stage.
                return true;
            }
        }
#endif
        
        s_nCommandRecordSample = 0;
        s_nCommandRecognizeLimit = COMMAND_STAGE_TIME_MIN;

    #if defined(SUPPORT_VOICE_TAG)
        if (!g_bVoiceTagRecording && nMapID == RECORD_VOICE_TAG_ID)
        {
            g_bVoiceTagRecording = true;
            DBG_UART_TRACE("Please say the command name to record the voice tag.\r\n");
            PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, nMapID);
            EnableAllGroupModel(g_hCybModel1, g_hDSpotter1);
            return true;
        }
        else if (!g_bVoiceTagDeleting && nMapID == DELETE_VOICE_TAG_ID)
        {
            g_bVoiceTagDeleting = true;
            DBG_UART_TRACE("Please say the command name to delete the voice tag.\r\n");
            PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, nMapID);
            EnableAllGroupModel(g_hCybModel1, g_hDSpotter1);
            return true;
        }

        if (g_bVoiceTagRecording)
        {
            g_bVoiceTagRecording = false;
            // Release DSpotter
            ReleaseDSpotter(&g_hDSpotter1);
        #ifdef SUPPORT_RECOG_TWO_MODEL
            ReleaseDSpotter(&g_hDSpotter2);
        #endif

            // SD training, the training ID is nMapID.
            SD_Train(nMapID, saRecordSample, NULL, 0);

            // Re-init DSpotter.
            InitDSpotter(g_hCybModel1, GROUP_INDEX_TRIGGER, g_byaDSpotterMem1, sizeof(g_byaDSpotterMem1), &g_hDSpotter1, TRUE);
        #ifdef SUPPORT_RECOG_TWO_MODEL
            InitDSpotter(g_hCybModel2, GROUP_INDEX_TRIGGER, g_byaDSpotterMem2, sizeof(g_byaDSpotterMem2), &g_hDSpotter2, TRUE);
        #endif
            s_nCommandRecordSample = 0;
            return true;
        }
        else if (g_bVoiceTagDeleting)
        {
            g_bVoiceTagDeleting = false;
            // Release DSpotter
            ReleaseDSpotter(&g_hDSpotter1);
        #ifdef SUPPORT_RECOG_TWO_MODEL
            ReleaseDSpotter(&g_hDSpotter2);
        #endif

            if (bVoiceTag)
            {
                int nSDIndex = nCmdIndex - nTriggerWordCount - nCommandWordCount;
                if (SD_Delete(nSDIndex, FALSE) == DSPOTTER_SUCCESS)
                {
                    DBG_UART_TRACE("This voice tag has been deleted.\r\n");
                    PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, PLAY_ID_VTG_DELETED);
                }
            }
            else
            {
                // Init SD engine and delete the voice tag with ID == nMapID
                if (SD_Delete(nMapID, TRUE) == DSPOTTER_SUCCESS)
                {
                    DBG_UART_TRACE("All voice tags for this command have been deleted.\r\n");
                    PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, PLAY_ID_COMMAND_ALL_VTG_DELETED);
                }
                else
                {
                    DBG_UART_TRACE("This command does not have voice tag.\r\n");
                    PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, PLAY_ID_COMMAND_NO_VTG);
                }
            }

            // Re-init DSpotter.
            InitDSpotter(g_hCybModel1, GROUP_INDEX_TRIGGER, g_byaDSpotterMem1, sizeof(g_byaDSpotterMem1), &g_hDSpotter1, TRUE);
        #ifdef SUPPORT_RECOG_TWO_MODEL
            InitDSpotter(g_hCybModel2, GROUP_INDEX_TRIGGER, g_byaDSpotterMem2, sizeof(g_byaDSpotterMem2), &g_hDSpotter2, TRUE);
        #endif
            s_nCommandRecordSample = 0;
            return true;
        }
    #else   // SUPPORT_VOICE_TAG
        if (nMapID == RECORD_VOICE_TAG_ID || nMapID == DELETE_VOICE_TAG_ID)
        {
            DBG_UART_TRACE("Please define SUPPORT_VOICE_TAG to support voice tag function.\r\n");
            return true;
        }
    #endif  // SUPPORT_VOICE_TAG

        // Flicker red LED to indicate the success of recognition.
        TurnOffLED(LED_R); //Red    = OFF
        s_nLedTurnOnCount = 10;

    #ifdef SUPPORT_SPEEX_PLAY
        const BYTE *pbySpeexDataBegin = (const BYTE *)&g_uSpeexData1Begin;

        #ifdef SUPPORT_RECOG_TWO_MODEL
            if (nRet2 == DSPOTTER_SUCCESS)
                pbySpeexDataBegin = (const BYTE *)&g_uSpeexData2Begin;
        #endif

        PlaySpeexByID(pbySpeexDataBegin, nMapID);
    #endif
        DBG_UART_TRACE("\r\n");

        switch (nMapID)
        {
        case 0:
        case 1:
            //user's application
            break;
    #if defined(SUPPORT_VOICE_TAG)
        case DELETE_ALL_VOICE_TAG_ID:
            if (SD_Delete_All() == 0)
                PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, PLAY_ID_ALL_VTG_DELETED);
            break;
    #endif
        default:
            //invalid argument
            break;
        }

        s_nCommandRecordSample = 0;
        s_nCommandRecognizeLimit = COMMAND_STAGE_TIME_MIN;

        if (g_nActiveGroupIndex == GROUP_INDEX_TRIGGER && CybModelGetGroupCount(g_hCybModel1) >= 2)
        {
            // After trigger word recognized, switch to command recognition mode
            SelectGroupModel(g_hCybModel1, GROUP_INDEX_COMMAND, g_hDSpotter1, TRUE);
        #ifdef SUPPORT_RECOG_TWO_MODEL
            SelectGroupModel(g_hCybModel2, GROUP_INDEX_COMMAND, g_hDSpotter2, TRUE);
        #endif
        }
    }
    else if (nRet1 == DSPOTTER_ERR_Expired || nRet2 == DSPOTTER_ERR_Expired)
    {
        DBG_UART_TRACE("\r\nThe trial version DSpotter reach the max trial usage count, please reset system.\r\n");
        TurnOffLED(LED_R);       //Red     = OFF
        TurnOffLED(LED_G);       //Green   = OFF
        TurnOffLED(LED_B);       //Yellow  = OFF

        return false;
    }
    else if (nRet1 == DSPOTTER_ERR_NeedMoreSample || nRet2 == DSPOTTER_ERR_NeedMoreSample)
    {
        if (g_bVoiceTagRecording || g_bVoiceTagDeleting)
        {
            s_nCommandRecordSample += RECORD_FRAME_SAMPLES;
            if (s_nCommandRecordSample > 16000 * 8)
            {
                DBG_UART_TRACE("\r\nTimeout for recognizing the command name!\r\n");

                s_nCommandRecordSample = 0;
                s_nCommandRecognizeLimit = COMMAND_STAGE_TIME_MIN;
                g_bVoiceTagRecording = FALSE;
                g_bVoiceTagDeleting = FALSE;
                SelectGroupModel(g_hCybModel1, GROUP_INDEX_TRIGGER, g_hDSpotter1, TRUE);
            #ifdef SUPPORT_RECOG_TWO_MODEL
                InitDSpotter(g_hCybModel2, GROUP_INDEX_TRIGGER, g_byaDSpotterMem2, sizeof(g_byaDSpotterMem2), &g_hDSpotter2, TRUE);
            #endif
            }
        }
        else if (g_nActiveGroupIndex == GROUP_INDEX_COMMAND)
        {
            // Check timeout for command recognition mode
            s_nCommandRecordSample += RECORD_FRAME_SAMPLES;
            if (s_nCommandRecordSample > 16000 / 1000 * s_nCommandRecognizeLimit)
            {
            #ifdef SUPPORT_RECOG_TWO_MODEL
                if (DSpotter_IsKeywordAlive(g_hDSpotter1) || DSpotter_IsKeywordAlive(g_hDSpotter2))
            #else
                if (DSpotter_IsKeywordAlive(g_hDSpotter1))
            #endif
                {
                    if (s_nCommandRecognizeLimit < COMMAND_STAGE_TIME_MAX)
                    {
                        s_nCommandRecognizeLimit += 500;
                        return true;
                    }
                }
                DBG_UART_TRACE("\r\nTimeout for command stage, switch to trigger stage.\r\n");
                s_nCommandRecognizeLimit = COMMAND_STAGE_TIME_MIN;

                SelectGroupModel(g_hCybModel1, GROUP_INDEX_TRIGGER, g_hDSpotter1, TRUE);
            #ifdef SUPPORT_RECOG_TWO_MODEL
                SelectGroupModel(g_hCybModel2, GROUP_INDEX_TRIGGER, g_hDSpotter2, TRUE);
            #endif
            }
        }
    }

    return true;
}
/*******************************************************************************
 End of function voice_loop
*******************************************************************************/

/*******************************************************************************
* Function Name: voice_release
* Description  : Release resource
* Arguments    : none
* Return Value : none
*******************************************************************************/
static void voice_release(void)
{
#ifdef SUPPORT_VOICE_TAG
    R_ICU_ExternalIrqDisable(&g_irq_button_voice_tag_ctrl);
    R_ICU_ExternalIrqClose(&g_irq_button_voice_tag_ctrl);
#endif

    AudioRecordRelease();

    ReleaseDSpotter(&g_hDSpotter1);
    CybModelRelease(g_hCybModel1);
    g_hCybModel1 = NULL;
#ifdef SUPPORT_RECOG_TWO_MODEL
    ReleaseDSpotter(&g_hDSpotter2);
    CybModelRelease(g_hCybModel2);
    g_hCybModel2 = NULL;
#endif

    AGC_Release(g_hAGC);
    g_hAGC = NULL;

    UartClose();
}
/*******************************************************************************
 End of function voice_release
*******************************************************************************/

static int TestByteOrder()
{
    short nTestWord = 0x0001;      //Little endian memory is 0x01 0x00, big endian memory is 0x00 0x01.
    char *b = (char *)&nTestWord;  //Little endian b[0] is 0x01, big endian memory b[0] is 0x00.
    return (int)b[0];              //Return 1/0 for little/big endian.
}

static void InitDSpotter(HANDLE hCybModel, int nGroupIndex, BYTE *pbyaDSpotterMem, int nMemSize, HANDLE *phDSpotter, BOOL bShowInfo)
{
    int nRet = DSPOTTER_ERR_IllegalParam;
    BYTE *lppbyModel[2 + 1]; // Two group model and one voice tag model.
    int n;
    int nGroupCount = CybModelGetGroupCount(hCybModel);

    // In this sample code, we only use one or two group model and treat the first group
    // as trigger word, the second group as command word.
    if (nGroupCount > 2)
        nGroupCount = 2;

    if (nGroupIndex >= nGroupCount)
    {
        DBG_UART_TRACE("Invalid nGroupIndex parameter!\r\n");
        return;
    }

    /*
     * For official version of DSpotter, DSpotter_Init_Multi() will use the data flash API and
     * its instance to check license. In configuration.xml, please:
     *   1. Add r_flash_hp component.
     *   2. Add Flash Driver stack, named "g_flash0" and set callback as "flash0_bgo_callback".
     * For RA6M1, The data flash address start at 0x40100000U, total 128 blocks, each block has
     * 64 bytes, total 8K bytes. DSpotter need 256 bytes(4 blocks) to save license information.
     * So, the valid flash address is FLASH_DF_BLOCK_ADDRESS(0) ~ FLASH_DF_BLOCK_ADDRESS(123).
    */
    if (FLASH_DSPOTTER_BLOCK_INDEX + FLASH_DSPOTTER_BLOCK_COUNT > FLASH_DF_TOTAL_BLOCKS)
    {
        DBG_UART_TRACE("FLASH_DSPOTTER_BLOCK_INDEX error!\r\n");
        __BKPT(0);
    }

    ReleaseDSpotter(phDSpotter);

    for (n = 0; n < nGroupCount; n++)
        lppbyModel[n] = (BYTE*)CybModelGetGroup(hCybModel, n);

#ifdef SUPPORT_VOICE_TAG
    if (hCybModel == g_hCybModel1 && DSpotter_GetNumWord(g_byaSDModel) > 0)
    {
        // Initial with voice tag model.
        lppbyModel[n] = g_byaSDModel;
        n++;
    }
#endif

    *phDSpotter = DSpotter_Init_Multi((BYTE *)CybModelGetBase(hCybModel), lppbyModel, n, MAX_COMMAND_TIME,
                                      pbyaDSpotterMem, nMemSize, FLASH_DF_BLOCK_ADDRESS(FLASH_DSPOTTER_BLOCK_INDEX), FLASH_DF_BLOCK_SIZE, &nRet);
    if (*phDSpotter == NULL)
    {
    #ifdef SUPPORT_VOICE_TAG
        if (hCybModel == g_hCybModel1 && lppbyModel[n - 1] == g_byaSDModel)
        {
            n--;
            *phDSpotter = DSpotter_Init_Multi((BYTE *)CybModelGetBase(hCybModel), lppbyModel, n, MAX_COMMAND_TIME,
                                              pbyaDSpotterMem, nMemSize, FLASH_DF_BLOCK_ADDRESS(FLASH_DSPOTTER_BLOCK_INDEX), FLASH_DF_BLOCK_SIZE, &nRet);
            if (*phDSpotter == NULL)
            {
                DBG_UART_TRACE("DSpotter_Init_XXX() fail, error = %d!\r\n", nRet);
                __BKPT(0);
            }
            else
            {
                // Erase voice tag model, it may be trained with another model.
                FlashOpen();
                FlashErase(FLASH_DF_BLOCK_ADDRESS(FLASH_VOICE_TAG_BLOCK_INDEX), FLASH_VOICE_TAG_BLOCK_COUNT);
                FlashClose();

                DBG_UART_TRACE("Voice tag model is incompatible, we erase it!\r\n");
            }
        }
    #else
        DBG_UART_TRACE("DSpotter_Init_XXX() fail, error = %d!\r\n", nRet);
        __BKPT(0);
    #endif
    }

    if (!DSpotter_ModelHasMapIDList(*phDSpotter))
    {
        for (n = 0; n < nGroupCount; n++)
        {
            SHORT *lpsMapID;
            int nCommandCount;
            int nCommandIndex;
            int nMapID;

            // This old model data which doesn't include map ID.
            // For use with new API DSpotter_GetCmdMapIDList(), we use DSpotter_SetCmdMapIDList() for old model.
            nCommandCount = CybModelGetCommandCount(hCybModel, n);
            lpsMapID = (SHORT *)PortMalloc((size_t)nCommandCount * 2);
            if (lpsMapID != NULL)
            {
                for (nCommandIndex = 0; nCommandIndex < nCommandCount; nCommandIndex++)
                {
                    CybModelGetCommandInfo(hCybModel, n, nCommandIndex, NULL, 0, &nMapID, NULL);
                    lpsMapID[nCommandIndex] = (SHORT)nMapID;
                }
                DSpotter_SetCmdMapIDList(*phDSpotter, n, lpsMapID, nCommandCount);
                PortFree(lpsMapID);
            }
            else
            {
                DBG_UART_TRACE("Heap too small!\r\n");
                __BKPT(0);
            }
        }
    }

    SelectGroupModel(hCybModel, nGroupIndex, *phDSpotter, bShowInfo);

    AGC_SetMaxGain(g_hAGC, VOLUME_SCALE_RECONG/100);
}

static void ReleaseDSpotter(HANDLE *lphDSpotter)
{
    if (lphDSpotter != NULL)
    {
        DSpotter_Release(*lphDSpotter);
        *lphDSpotter = NULL;
    }
}

static void SelectGroupModel(HANDLE hCybModel, int nGroupIndex, HANDLE hDSpotter, BOOL bShowInfo)
{
    int nGroupCount = CybModelGetGroupCount(hCybModel);

    // In this sample code, we only use one or two group model and treat the first group
    // as trigger word, the second group as command word.
    if (nGroupCount > 2)
        nGroupCount = 2;

    if (nGroupIndex >= nGroupCount)
        return;

    for (int i = 0; i < nGroupCount; i++)
        DSpotter_EnableModel(hDSpotter, i, i == nGroupIndex);

    g_nActiveGroupIndex = nGroupIndex;
    if (bShowInfo)
    {
        DBG_UART_TRACE("\r\n");
    #ifdef SUPPORT_RECOG_TWO_MODEL
        if (hCybModel == g_hCybModel1)
            DBG_UART_TRACE("Model 0: %s group active.\r\n", nGroupIndex == 0 ? "Wake-up" : "Command");
        else
            DBG_UART_TRACE("Model 1: %s group active.\r\n", nGroupIndex == 0 ? "Wake-up" : "Command");
    #else
        DBG_UART_TRACE("%s group active.\r\n", nGroupIndex == 0 ? "Wake-up" : "Command");
    #endif

        PrintGroupCommandList(hCybModel, nGroupIndex);
    }
}

static bool ds_decode_init(void)
{
    const bsp_unique_id_t   *devi_uniq_id;
    BYTE *lppbyModel[2];
    int  nGroupCount;
    int  nMemSize;

    //DBG_UART_TRACE("ds_decode_init() Begin\r\n");
    /** Memory size check, device authentication and ring buffer initialization */
    // Get unique ID
    devi_uniq_id = R_BSP_UniqueIdGet();

    int nByteOrder = TestByteOrder();
    DBG_UART_TRACE("%s\r\n", nByteOrder == 1 ? "Little Endian" : "Big Endian");
    DBG_UART_TRACE("MCU board: %s\r\n", MCU_BOARD_STRING);
    DBG_UART_TRACE("FSP version: %d.%d\r\n", FSP_VERSION_MAJOR, FSP_VERSION_MINOR);
    DBG_UART_TRACE("DSpotter version: %s", DSpotter_VerInfo());
    DBG_UART_TRACE("Sample code build at %s, %s\r\n", __DATE__, __TIME__);
    DBG_UART_TRACE("Unique ID: %X %X %X %X\r\n", devi_uniq_id->unique_id_words[0],
            devi_uniq_id->unique_id_words[1], devi_uniq_id->unique_id_words[2], devi_uniq_id->unique_id_words[3]);

    if (nByteOrder == 0)
    {
        DBG_UART_TRACE("The DSpotter use Renesas RA default compile setting: CM4/HardFPU/little endian, not big endian!\r\n");
        return FSP_ERR_ASSERTION;
    }

    g_hCybModel1 = CybModelInit((const BYTE*)&uCYModel1Begin, g_byaCybModelMem1, sizeof(g_byaCybModelMem1), NULL);
#ifdef SUPPORT_RECOG_TWO_MODEL
    g_hCybModel2 = CybModelInit((const BYTE*)&uCYModel2Begin, g_byaCybModelMem2, sizeof(g_byaCybModelMem2), NULL);
#endif

#ifdef SUPPORT_VOICE_TAG
    memcpy(g_byaSDModel, FLASH_DF_BLOCK_ADDRESS(FLASH_VOICE_TAG_BLOCK_INDEX), sizeof(g_byaSDModel));
#endif

    // Check memory for every group and show their commands.
    for (int nModel = 0; nModel < 2; nModel++)
    {
        HANDLE hCybModel;
        int nDSpotterMem;

        if (nModel == 0)
        {
            hCybModel = g_hCybModel1;
            nDSpotterMem = (int)sizeof(g_byaDSpotterMem1);
        }
        else
        {
        #ifdef SUPPORT_RECOG_TWO_MODEL
            hCybModel = g_hCybModel2;
            nDSpotterMem = (int)sizeof(g_byaDSpotterMem2);
        #else
            break;
        #endif
        }

        DBG_UART_TRACE("\r\n");
        DBG_UART_TRACE("The DSpotter model %d declare memory buffer size = %d\r\n", nModel, nDSpotterMem);
        nGroupCount = CybModelGetGroupCount(hCybModel);
        if (nGroupCount > 2)
            nGroupCount = 2;
        for (int nGroup = 0; nGroup < nGroupCount; nGroup++)
        {
            lppbyModel[nGroup] = (BYTE*)CybModelGetGroup(hCybModel, nGroup);

            DBG_UART_TRACE("The model %d, command list of group index %d: \r\n", nModel, nGroup);
            PrintGroupCommandList(hCybModel, nGroup);
        }
        nMemSize = DSpotter_GetMemoryUsage_Multi((BYTE *)CybModelGetBase(hCybModel), lppbyModel, nGroupCount, MAX_COMMAND_TIME);
        DBG_UART_TRACE("The DSpotter model %d has %d groups needed working memory size = %d\r\n", nModel, nGroupCount, nMemSize);
        if (nDSpotterMem < nMemSize)
        {
            DBG_UART_TRACE("The DSpotter model %d declare memory buffer size is tool small!\r\n", nModel);
            __BKPT(0);
        }
#ifdef SUPPORT_VOICE_TAG
        if (nModel == 0)
        {
            if (CybModelGetTriMap(g_hCybModel1) == NULL)
            {
                DBG_UART_TRACE("Please use XXX_pack_WithTxtAndTri.bin!\r\n");
                __BKPT(0);
            }

            nMemSize = DSpotterSD_GetMemoryUsage((BYTE *)CybModelGetBase(g_hCybModel1), (BYTE *)CybModelGetTriMap(g_hCybModel1));
            DBG_UART_TRACE("The voice tag needed working memory size = %d\r\n", nMemSize);
            if (nDSpotterMem < nMemSize)
            {
                DBG_UART_TRACE("The DSpotter model %d declare memory buffer size is tool small!\r\n", nModel);
                __BKPT(0);
            }
            PrintSDModelInfor();
        }
#endif
    }

    g_hAGC = AGC_Init(g_byaAGCMem, sizeof(g_byaAGCMem), NULL);
    if (g_hAGC == NULL)
    {
        DBG_UART_TRACE("AGC initial fail!\r\n");
        __BKPT(0);
    }

    InitDSpotter(g_hCybModel1, GROUP_INDEX_TRIGGER, g_byaDSpotterMem1, sizeof(g_byaDSpotterMem1), &g_hDSpotter1, TRUE);
#ifdef SUPPORT_RECOG_TWO_MODEL
    InitDSpotter(g_hCybModel2, GROUP_INDEX_TRIGGER, g_byaDSpotterMem2, sizeof(g_byaDSpotterMem2), &g_hDSpotter2, TRUE);
#endif

    //DBG_UART_TRACE("ds_decode_init() End\r\n");

    return true;
}

void OnDataReadCompleteCallback(void)
{
    static char s_szCmd[4] = { 0 };
    static int8_t s_nIndex = 0;
    char ch = (char)g_byaUartRxBuffer[0];

    if (s_nIndex < 4)
    {
        s_szCmd[s_nIndex++] = ch;
    }
    else
    {
        memmove(s_szCmd, s_szCmd + 1, 3);
        s_szCmd[3] = ch;
        if (strncmp(s_szCmd, "CYB", 3) == 0)
        {
        #ifdef SUPPORT_UART_DUMP_RECORD
            if (ch == 0x31)
                g_bUartSendRecord = true;
            else if (ch == 0x32)
                g_bUartSendRecord = false;
        #endif
            if (ch == 0x33)
                g_bUartCheckAlive = true;
        }
    }

    //Continue to read next byte.
    UartAsyncRead(g_byaUartRxBuffer, 1, OnDataReadCompleteCallback);
}


#ifdef SUPPORT_VOICE_TAG

void g_irq_button_voice_tag_cb(external_irq_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    g_bVoiceTagButtonPressed = true;
}

// Save g_byaSDModel to flash.
static void SaveSDModel()
{
    int nRet;

    if ((nRet = FlashOpen()) == FSP_SUCCESS)
    {
        if ((nRet = FlashErase( FLASH_DF_BLOCK_ADDRESS(FLASH_VOICE_TAG_BLOCK_INDEX), FLASH_VOICE_TAG_BLOCK_COUNT)) == FSP_SUCCESS)
        {
            if ((nRet = FlashWrite( FLASH_DF_BLOCK_ADDRESS(FLASH_VOICE_TAG_BLOCK_INDEX), g_byaSDModel, sizeof(g_byaSDModel))) == FSP_SUCCESS)
                DBG_UART_TRACE("Write voice tag to flash.\r\n");
            else
                DBG_UART_TRACE("Write voice tag to flash fail!\r\n");

            if (memcmp(FLASH_DF_BLOCK_ADDRESS(FLASH_VOICE_TAG_BLOCK_INDEX), g_byaSDModel, sizeof(g_byaSDModel)) != 0)
                DBG_UART_TRACE("Voice tag verify fail!.\r\n");
        }
        else
        {
            DBG_UART_TRACE("Flash erase fail!\r\n");
        }
        FlashClose();
    }
    else
    {
        DBG_UART_TRACE("Flash open fail!\r\n");
    }
}

static void PrintSDModelInfor()
{
    int nSDNum;
    SHORT *lpsMapID;

    nSDNum = DSpotter_GetNumWord(g_byaSDModel);
    if (nSDNum < 0)
        nSDNum = 0;
    DBG_UART_TRACE("Total %d voice tags.\r\n", nSDNum / 3);

#ifdef CHECK_GET_CMD_MAP_ID_LIST
    lpsMapID = DSpotter_GetCmdMapIDList(g_hDSpotter, 1, NULL);
    for (int i = 0; i < nSDNum; i++)
        DBG_UART_TRACE("DSpotter_GetCmdMapIDList():   SD model index = %d, ID = %d\r\n", i, lpsMapID[i]);
#endif

    lpsMapID = DSpotter_GetModelMapIDList(g_byaSDModel);
    for (int i = 0; i < nSDNum; i++)
    {
        char szCommand[64];
        int  nGroupIndex, nCmdIndex;
        if (CybModelGetCommandInfoByID(g_hCybModel1, lpsMapID[i], &nGroupIndex, &nCmdIndex, szCommand, 64) != NULL)
            DBG_UART_TRACE("    SD model index = %d, ID = %d, %s, nGroupIndex = %d, nCmdIndex = %d\r\n", i, lpsMapID[i], szCommand, nGroupIndex, nCmdIndex);
        else if (lpsMapID[i] != -1)
            DBG_UART_TRACE("    SD model index = %d, ID = %d\r\n", i, lpsMapID[i]);
    }
}

static int SD_Train(int nMapID, short *lpsRecordSample, const short *lpsInputSample, int nSampleCount)
{
    HANDLE hDSpotterSD;
    int nRet;
    int nIndex;

    if (lpsInputSample != NULL)
    {
        DBG_UART_TRACE("Input begins.\r\n");
        nIndex = 0;
        AGC_SetMaxGain(g_hAGC, 1);
    }
    else
    {
        DBG_UART_TRACE("Start recording voice tag, please finish speaking within 5 seconds.\r\n");
        PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, PLAY_ID_START_RECORD_VTG);

        // Set gain for record voice tag.
        AGC_SetMaxGain(g_hAGC, VOLUME_SCALE_VOICE_TAG/100);
    }

    ReleaseDSpotter(&g_hDSpotter1);
    hDSpotterSD = DSpotterSD_Init((BYTE *)CybModelGetBase(g_hCybModel1), (BYTE *)CybModelGetTriMap(g_hCybModel1),
        g_byaDSpotterMem1, sizeof(g_byaDSpotterMem1), &nRet);
    if (hDSpotterSD == NULL)
    {
        DBG_UART_TRACE("DSpotterSD_Init() fail, nRet = %d!\r\n", nRet);
        return DSPOTTER_ERR_IllegalHandle;
    }

    nRet = DSpotterSD_AddUttrStart(hDSpotterSD, (SHORT*)g_byaSDVoice, VOICE_TAG_VOICE_SIZE);
    if (nRet != DSPOTTER_SUCCESS)
    {
        DBG_UART_TRACE("DSpotterSD_AddUttrStart() fail, nRet = %d!\r\n", nRet);
        DSpotterSD_Release(hDSpotterSD);
        return DSPOTTER_ERR_IllegalHandle;
    }

    int nTotalTime = 0;
    while (nTotalTime < 5000)
    {
        if (lpsInputSample != NULL)
        {
            if (nSampleCount - nIndex < RECORD_FRAME_SAMPLES * 2)
                break;
            memcpy(lpsRecordSample, lpsInputSample + nIndex, RECORD_FRAME_SAMPLES * 2);
            nIndex += RECORD_FRAME_SAMPLES;
        }
        else
        {
            if (AudioRecordGetDataSize() < RECORD_FRAME_SIZE)
                continue;

            AudioRecordGetData(lpsRecordSample, RECORD_FRAME_SIZE);
            AGC_Do(g_hAGC, lpsRecordSample, RECORD_FRAME_SAMPLES, lpsRecordSample);
        }
        nTotalTime += (RECORD_FRAME_SAMPLES / 16);

        nRet = DSpotterSD_AddSample(hDSpotterSD, lpsRecordSample, RECORD_FRAME_SAMPLES);
        if (nRet != DSPOTTER_ERR_NeedMoreSample)
            break;
    }

    if (nRet != DSPOTTER_SUCCESS)
    {
        DBG_UART_TRACE("No clear voice detected! nRet = %d\r\n", nRet);
        DSpotterSD_Release(hDSpotterSD);

        PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, PLAY_ID_NO_CLEAR_VOICE);
    }
    else
    {
        nRet = DSpotterSD_AddUttrEnd(hDSpotterSD);
        if (nRet != DSPOTTER_SUCCESS)
        {
            DBG_UART_TRACE("DSpotterSD_AddUttrEnd() fail, nRet = %d!\r\n", nRet);
        }
        else
        {
            int nUsedSize;

            // Training stage
            if (lpsInputSample != NULL)
                DBG_UART_TRACE("Input ends, training begins.\r\n");
            else
                DBG_UART_TRACE("Recording ends, training begins.\r\n");

            nRet = DSpotterSD_TrainWord_WithMapID(hDSpotterSD, (char *)g_byaSDModel, sizeof(g_byaSDModel), &nUsedSize, (short)nMapID);
            if (nRet != DSPOTTER_SUCCESS)
            {
                DBG_UART_TRACE("Training fail! nRet = %d!\r\n", nRet);
            }
            else
            {
                INT nStart, nEnd;

                DSpotterSD_GetUttrEPD(hDSpotterSD, &nStart, &nEnd);
                DBG_UART_TRACE("Training OK. The speech start and end time: %d ~ %d ms.\r\n", nStart / 16, nEnd / 16);
                DBG_UART_TRACE("Total %d voice tags, the used model size is %d.\r\n", DSpotter_GetNumWord(g_byaSDModel) / 3, nUsedSize);

                SaveSDModel();
                PrintSDModelInfor();
            }
        }
        DSpotterSD_Release(hDSpotterSD);

        if (nRet == DSPOTTER_SUCCESS)
            PlaySpeexByID((const BYTE *)&g_uSpeexData1Begin, PLAY_ID_RECORD_VTG_OK);
    }

    return nRet;
}

static int SD_Delete(int nMapIDorIndex, BOOL bMapID)
{
    HANDLE hDSpotterSD = NULL;
    int nRet;
    int nSDNum = DSpotter_GetNumWord(g_byaSDModel);
    SHORT *lpsMapID = DSpotter_GetModelMapIDList(g_byaSDModel);

    ReleaseDSpotter(&g_hDSpotter1);
    hDSpotterSD = DSpotterSD_Init((BYTE *)CybModelGetBase(g_hCybModel1), (BYTE *)CybModelGetTriMap(g_hCybModel1),
        g_byaDSpotterMem1, sizeof(g_byaDSpotterMem1), &nRet);
    if (hDSpotterSD == NULL)
    {
        DBG_UART_TRACE("DSpotterSD_Init() fail, nRet = %d!\r\n", nRet);
        return nRet;
    }

    nRet = DSPOTTER_ERR_IllegalParam;
    if (bMapID)
    {
        int nMapID = nMapIDorIndex;

        // Delete voice tag if its map ID is m_naMapID[wParam]
        for (int i = nSDNum - 1; i >= 0; i--)
        {
            if ( (lpsMapID[i] == nMapID) || ((nMapID == -100) && (i % 3 == 0)) )
            {
                DSpotterSD_DeleteWord(hDSpotterSD, (char *)g_byaSDModel, i, NULL);
                DBG_UART_TRACE("DSpotterSD_DeleteWord() OK, SD index = %d, ID = %d\r\n", i, nMapID);
                nRet = DSPOTTER_SUCCESS;
            }
        }
    }
    else
    {
        int nSDIndex = nMapIDorIndex;

        if (nSDIndex < nSDNum && nSDIndex % 3 == 0)
        {
            DSpotterSD_DeleteWord(hDSpotterSD, (char *)g_byaSDModel, nSDIndex, NULL);
            DBG_UART_TRACE("DSpotterSD_DeleteWord() OK, SD index = %d, ID = %d\r\n", nSDIndex, lpsMapID[nSDIndex]);
            nRet = DSPOTTER_SUCCESS;
        }
        else
        {
            DBG_UART_TRACE("Invalid SD index, nSDIndex = %d, nSDNum = %d\r\n", nSDIndex, nSDNum);
        }
    }

    DSpotterSD_Release(hDSpotterSD);

    nSDNum = DSpotter_GetNumWord(g_byaSDModel);
    if (nSDNum <= 0)
        memset(g_byaSDModel, 0, sizeof(g_byaSDModel));

    SaveSDModel();
    PrintSDModelInfor();

    return nRet;
}

static int SD_Delete_All()
{
    return SD_Delete(-100, TRUE);
}

static void EnableAllGroupModel(HANDLE hCybModel, HANDLE hDSpotter)
{
    int nGroupCount = CybModelGetGroupCount(hCybModel);

    // In this sample code, we only use one or two group model and treat the first group
    // as trigger word, the second group as command word.
    if (nGroupCount > 2) {
        nGroupCount = 2;
    }

    for (int i = 0; i < nGroupCount; i++) {
        DSpotter_EnableModel(hDSpotter, i, TRUE);
    }
}
#endif

static void PrintGroupCommandList(HANDLE hCybModel, int nGroupIndex)
{
    char szCommand[64];
    int  nMapID;

    for (int i = 0; i < CybModelGetCommandCount(hCybModel, nGroupIndex); i++)
    {
        CybModelGetCommandInfo(hCybModel, nGroupIndex, i, szCommand, sizeof(szCommand), &nMapID, NULL);
        if (strlen(szCommand) > 0)
        {
        #ifdef NOT_SHOW_MULTI_PRONUNCIATION
            if (strchr(szCommand, '^') != NULL)
                continue;
        #endif
            DBG_UART_TRACE("    %s, Map ID = %d\r\n", szCommand, nMapID);
        }
    }
    DBG_UART_TRACE("\r\n");
}

#if defined(SUPPORT_SPEEX_PLAY) || defined(SUPPORT_VOICE_TAG)
static void PlaySpeexByID(const BYTE *pbySpeexDataBegin, int nMapID)
{
    int nSampleRate;

    if (PlaySpeexIsDataValid(pbySpeexDataBegin, nMapID, &nSampleRate))
    {
        ReleaseDSpotter(&g_hDSpotter1);

        TurnOffLED(LED_R);  //Red   = OFF
        AudioRecordPause(); // Skip record data to avoid data lost or DSpotter recognize it again.
        PlaySpeexOpenDevice();
        PlaySpeexSetWorkingMemory((char *)g_byaDSpotterMem1, sizeof(g_byaDSpotterMem1));
        PlaySpeexStart();
        DBG_UART_TRACE("Start play.\r\n");
        if (!PlaySpeexMapID(pbySpeexDataBegin, nMapID, 100))
            DBG_UART_TRACE("Fail to play Speex by MapID(%d).\r\n", nMapID);
        DBG_UART_TRACE("Stop play.\r\n");
        PlaySpeexStop();
        PlaySpeexCloseDevice();

        InitDSpotter(g_hCybModel1, g_nActiveGroupIndex, g_byaDSpotterMem1, sizeof(g_byaDSpotterMem1), &g_hDSpotter1, FALSE);
        AudioRecordClearData();
        AudioRecordResume();
        TurnOnLED(LED_R);   //Red   = ON
    }
    else
    {
        DBG_UART_TRACE("No ID(%d) data to play! \r\n", nMapID);
    }
}
#endif

static BOOL CheckGroupHasID(HANDLE hDSpotter, HANDLE hCybModel, int nGroupIndex, int nMapID)
{
    int nCommandIndex;
    int nCommandCount;
    SHORT *lpsMapID;

    lpsMapID = DSpotter_GetCmdMapIDList(hDSpotter, nGroupIndex, NULL);
    if (lpsMapID != NULL)
    {
        nCommandCount = CybModelGetCommandCount(hCybModel, nGroupIndex);
        for (nCommandIndex = 0; nCommandIndex < nCommandCount; nCommandIndex++)
        {
            if (lpsMapID[nCommandIndex] == nMapID)
                return TRUE;
        }
    }

    return FALSE;
}

void ToggleLED(bsp_io_port_pin_t oLED)
{
    bsp_io_level_t level;

    R_IOPORT_PinRead(&g_ioport_ctrl, oLED, &level);
    if (level == ON)
        R_IOPORT_PinWrite(&g_ioport_ctrl, oLED, OFF);
    else
        R_IOPORT_PinWrite(&g_ioport_ctrl, oLED, ON);
}

void TurnOnLED(bsp_io_port_pin_t oLED)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, oLED, ON);
}

void TurnOffLED(bsp_io_port_pin_t oLED)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, oLED, OFF);
}


/*******************************************************************************
 End Of File
*******************************************************************************/
