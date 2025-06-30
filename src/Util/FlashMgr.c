#include <stdio.h>
#include <stdarg.h>
#include "voice_main.h"
#include "FlashMgr.h"
#include "DbgTrace.h"
#include "UartMgr.h"
#if BSP_FEATURE_FLASH_LP_VERSION != 0
	#include "r_flash_lp.h"
#else
	#include "r_flash_hp.h"
#endif

extern const flash_instance_t g_flash0;

static const flash_instance_t *g_pFlashInst = NULL;
static volatile bool g_b_flash_event_not_blank = false;
static volatile bool g_b_flash_event_blank = false;
static volatile bool g_b_flash_event_erase_complete = false;
static volatile bool g_b_flash_event_write_complete = false;


void flash0_bgo_callback(flash_callback_args_t *p_args);

int FlashGetDataSize(void)
{
	return FLASH_DF_SIZE;
}

int FlashGetBlockSize(void)
{
	return FLASH_DF_BLOCK_SIZE;
}

void* FlashGetBaseAddress(void)
{
	return (void *)FLASH_DF_BLOCK_BASE_ADDRESS;
}

void* FlashGetBlockAddress(int nBlockIndex)
{
	if (nBlockIndex < 0 || nBlockIndex >= (int)(FLASH_DF_SIZE / FLASH_DF_BLOCK_SIZE))
		return NULL;

	return (void *)(FLASH_DF_BLOCK_BASE_ADDRESS + (uint32_t)nBlockIndex*FLASH_DF_BLOCK_SIZE);
}

int FlashOpen(void)
{
    fsp_err_t err = FSP_SUCCESS;

    if (g_pFlashInst != NULL)
        return FSP_ERR_ALREADY_OPEN;

    g_pFlashInst = &g_flash0;

#if BSP_FEATURE_FLASH_LP_VERSION != 0
    err = R_FLASH_LP_Open(g_pFlashInst->p_ctrl, g_pFlashInst->p_cfg);
#else
    err = R_FLASH_HP_Open(g_pFlashInst->p_ctrl, g_pFlashInst->p_cfg);
#endif

    if (err != FSP_SUCCESS)
    {
        g_pFlashInst = NULL;
        DbgUartTrace("R_FLASH_HP_Open() err = %d!\n", err);
    }

    return err;
}


int FlashClose(void)
{
    fsp_err_t err;

    if (g_pFlashInst == NULL)
        return FSP_SUCCESS;

#if BSP_FEATURE_FLASH_LP_VERSION != 0
    err = R_FLASH_LP_Close(g_pFlashInst->p_ctrl);
#else
    err = R_FLASH_HP_Close(g_pFlashInst->p_ctrl);
#endif

    if (err != FSP_SUCCESS)
        DbgUartTrace("R_FLASH_HP_Close() err = %d!\n", err);

    g_pFlashInst = NULL;

    return err;
}


bool FlashIsOpened(void)
{
    return (g_pFlashInst != NULL);
}


int FlashErase(void *lpFlashBlockAddress, int nNumBlocks)
{
    fsp_err_t err;
    int nCount;

    if (g_pFlashInst == NULL)
        return FSP_ERR_NOT_OPEN;

    // Check the low boundary of data flash.
    if ((uint32_t)lpFlashBlockAddress < FLASH_DF_BLOCK_BASE_ADDRESS)
    	return FSP_ERR_INVALID_ADDRESS;

    // Check the high boundary of data flash.
    if ((uint32_t)lpFlashBlockAddress + (uint32_t)nNumBlocks*FLASH_DF_BLOCK_SIZE > FLASH_DF_BLOCK_BASE_ADDRESS + FLASH_DF_SIZE)
    	return FSP_ERR_INVALID_ARGUMENT;

    // Check the address is the start of block.
    if (((uint32_t)lpFlashBlockAddress - FLASH_DF_BLOCK_BASE_ADDRESS) % FLASH_DF_BLOCK_SIZE != 0)
    	return FSP_ERR_INVALID_ARGUMENT;

#if BSP_FEATURE_FLASH_LP_VERSION != 0
    err = R_FLASH_LP_Erase(g_pFlashInst->p_ctrl, (uint32_t)lpFlashBlockAddress, (uint32_t)nNumBlocks);
#else
    err = R_FLASH_HP_Erase(g_pFlashInst->p_ctrl, (uint32_t)lpFlashBlockAddress, (uint32_t)nNumBlocks);
#endif

    if (FSP_SUCCESS != err)
    {
        DbgUartTrace("R_FLASH_HP_Erase() failed! %d\r\n", err);
        return err;
    }

    // Wait for the erase complete event flag, if BGO is SET.
    if (true == g_pFlashInst->p_cfg->data_flash_bgo)
    {
        //UartTrace("BGO has enabled\r\n");
        nCount = 0;
        while (!g_b_flash_event_erase_complete)
        {
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            if (nCount++ >= 3000)
                break;
        }
        if (nCount >= 3000)
        {
            DbgUartTrace("R_FLASH_HP_Erase() timeout failed!\r\n");
            err = FSP_ERR_ASSERTION;
        }
        g_b_flash_event_erase_complete = false;
    }

    return err;
}

int FlashBlankCheck(void *lpFlashAddress, int nNumBytes)
{
    fsp_err_t err;
    flash_result_t blank_check_result = 0;
    int nCount;

    if (g_pFlashInst == NULL)
        return FSP_ERR_NOT_OPEN;

    if ((uint32_t)lpFlashAddress < FLASH_DF_BLOCK_BASE_ADDRESS)
    	return FSP_ERR_INVALID_ADDRESS;

    if ((uint32_t)lpFlashAddress + (uint32_t)nNumBytes > FLASH_DF_BLOCK_BASE_ADDRESS + FLASH_DF_SIZE)
    	return FSP_ERR_INVALID_ARGUMENT;

#if BSP_FEATURE_FLASH_LP_VERSION != 0
    err = R_FLASH_LP_BlankCheck(g_pFlashInst->p_ctrl, (uint32_t)lpFlashAddress, (uint32_t)nNumBytes, &blank_check_result);
#else
    err = R_FLASH_HP_BlankCheck(g_pFlashInst->p_ctrl, (uint32_t)lpFlashAddress, (uint32_t)nNumBytes, &blank_check_result);
#endif

    if (FSP_SUCCESS != err)
    {
        DbgUartTrace("R_FLASH_HP_BlankCheck() failed! %d\r\n", err);
        return err;
    }

    // Validate the blank check result.
    if (FLASH_RESULT_BLANK == blank_check_result)
    {
        //DbgUartTrace("BlankCheck is successful\r\n");
    }
    else if (FLASH_RESULT_NOT_BLANK == blank_check_result)
    {
        DbgUartTrace("BlankCheck is not blank, not to write the data!\r\n");
        return FLASH_RESULT_NOT_BLANK;
    }
    else if (FLASH_RESULT_BGO_ACTIVE == blank_check_result)
    {
        // BlankCheck will update in Callback
        // Event flag will be updated in the blank check function when BGO is enabled.

        // Wait for callback function to set flag.
        nCount = 0;
        while (!(g_b_flash_event_not_blank || g_b_flash_event_blank))
        {
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            if (nCount++ >= 3000)
                break;
        }
        if (nCount >= 3000)
        {
            DbgUartTrace("R_FLASH_HP_BlankCheck() timeout failed!\r\n");
            return FLASH_STATUS_BUSY;
        }

        if (g_b_flash_event_not_blank)
        {
            //DbgUartTrace("Flash is not blank, need erase before writing data!\n\r");
            // Reset Flag
            g_b_flash_event_not_blank = false;
            return FLASH_EVENT_NOT_BLANK;
        }
        else
        {
            // UartTrace("Flash is blank.\r\n");
            // Reset Flag
            g_b_flash_event_blank = false;
        }
    }
    else
    {
        // No Operation
    }

    return err;
}


int FlashWrite(void *lpFlashAddress, const uint8_t* lpData, int nDataSize)
{
    fsp_err_t err;
    int nCount;

    if (g_pFlashInst == NULL)
        return FSP_ERR_NOT_OPEN;

    if ((uint32_t)lpFlashAddress < FLASH_DF_BLOCK_BASE_ADDRESS)
    	return FSP_ERR_INVALID_ADDRESS;

    if ((uint32_t)lpFlashAddress + (uint32_t)nDataSize > FLASH_DF_BLOCK_BASE_ADDRESS + FLASH_DF_SIZE)
    	return FSP_ERR_INVALID_ARGUMENT;

#if BSP_FEATURE_FLASH_LP_VERSION != 0
    err = R_FLASH_LP_Write(g_pFlashInst->p_ctrl, (uint32_t)lpData, (uint32_t)lpFlashAddress, (uint32_t)nDataSize);
#else
    err = R_FLASH_HP_Write(g_pFlashInst->p_ctrl, (uint32_t)lpData, (uint32_t)lpFlashAddress, (uint32_t)nDataSize);
#endif

    if (FSP_SUCCESS != err)
    {
        DbgUartTrace("R_FLASH_HP_Write() failed! %d\r\n", err);
        return err;
    }
    // Wait for the write complete event flag, if BGO is SET.
    if (true == g_pFlashInst->p_cfg->data_flash_bgo)
    {
        nCount = 0;
        while (!g_b_flash_event_write_complete)
        {
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            if (nCount++ >= 3000)
                break;
        }
        if (nCount >= 3000)
        {
            DbgUartTrace("R_FLASH_HP_Write() timeout failed!\r\n");
            return FLASH_STATUS_BUSY;
        }
        g_b_flash_event_write_complete = false;
    }
    //DbgUartTrace("Writing flash data is successful.\r\n");

    // Comparing the write_buffer and read_buffer
    if (0 != memcmp((uint8_t *)(uint32_t)lpFlashAddress, lpData, (size_t)nDataSize))
    {
        DbgUartTrace("Read and Write buffer is verified fail!\r\n");
        return FSP_ERR_WRITE_FAILED;
    }

    return err;
}


// Callback function for FLASH HP HAL
void flash0_bgo_callback(flash_callback_args_t *p_args)
{
    if (FLASH_EVENT_NOT_BLANK == p_args->event)
    {
        g_b_flash_event_not_blank = true;
    }
    else if (FLASH_EVENT_BLANK == p_args->event)
    {
        g_b_flash_event_blank = true;
    }
    else if (FLASH_EVENT_ERASE_COMPLETE == p_args->event)
    {
        g_b_flash_event_erase_complete = true;
    }
    else if (FLASH_EVENT_WRITE_COMPLETE == p_args->event)
    {
        g_b_flash_event_write_complete = true;
    }
    else
    {
        // No operation
    }
}



