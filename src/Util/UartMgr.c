#include <stdio.h>
#include <stdarg.h>
#include "UartMgr.h"
#include "DbgTrace.h"
#include "r_sci_uart.h"


static const uart_instance_t *g_pUart = NULL;
static int g_nUartChannel = -1;

static volatile uint32_t g_nUartWriteCount = 0;
static volatile bool g_bUartDataWriting = false;
static volatile bool g_bUartDataReading = false;
static volatile bool g_bUartClosing = true;
static PfnOnDataReadCompleteCallback g_pfnOnDataReadCompleteCallback = NULL;

static int UartAbortAyncIO(int nDir);

void user_uart_callback(uart_callback_args_t * p_args);


int UartOpen(const void *p, int nUartChannel, int nBaudRate)
{
    uart_info_t uart_info;
    fsp_err_t err;
    const uart_instance_t *pUart = (const uart_instance_t *)p;

    if (pUart == NULL || nUartChannel < 0)
        return FSP_ERR_INVALID_ARGUMENT;

    err = pUart->p_api->open(pUart->p_ctrl, pUart->p_cfg);
    if (err != FSP_SUCCESS)
    {
        DBGTRACE("pUart->p_api->open() err = %d!\n", err);
        return err;
    }

    err = pUart->p_api->infoGet(pUart->p_ctrl, &uart_info);
    if (err != FSP_SUCCESS)
    {
        DBGTRACE("pUart->p_api->infoGet() err = %d!\n", err);
        return err;
    }

    if (nBaudRate > 0)
    {
        baud_setting_t oBaudSetting;

        R_SCI_UART_BaudCalculate((uint32_t)nBaudRate,
                ((sci_uart_instance_ctrl_t *)pUart->p_ctrl)->bitrate_modulation,
                5000, &oBaudSetting);
        err = pUart->p_api->baudSet(pUart->p_ctrl, &oBaudSetting);
        if (err != FSP_SUCCESS)
        {
            DBGTRACE("pUart->p_api->baudSet() err = %d!\n", err);
            return err;
        }
    }


    DBGTRACE("Maximum bytes that can be written at this time is: 0x%lx\n", (uint32_t) uart_info.write_bytes_max);
    DBGTRACE("Maximum bytes that are available to read at one time is: 0x%lx\n", (uint32_t) uart_info.read_bytes_max);

    g_pUart = pUart;
    g_nUartChannel = nUartChannel;
    g_bUartClosing = false;

    return FSP_SUCCESS;
}


int UartClose(void)
{
    fsp_err_t err;

    if (g_pUart == NULL)
        return FSP_SUCCESS;

    g_bUartClosing = true;
    g_pUart->p_api->communicationAbort(g_pUart->p_ctrl, UART_DIR_RX_TX);
    g_pfnOnDataReadCompleteCallback = NULL;
    g_bUartDataWriting = false;
    g_bUartDataReading = false;

    err = g_pUart->p_api->close(g_pUart->p_ctrl);

    g_pUart = NULL;
    g_nUartChannel = -1;

    return err;
}


int UartAsyncRead(uint8_t* const p_dest, const uint32_t bytes, PfnOnDataReadCompleteCallback pfnOnDataReadCompleteCallback)
{
    fsp_err_t err;

    if (g_pUart == NULL || g_bUartClosing)
        return FSP_ERR_NOT_OPEN;

    if (g_bUartDataReading)
        return FSP_ERR_IN_USE;

    g_bUartDataReading = true;
    err = g_pUart->p_api->read(g_pUart->p_ctrl, p_dest, bytes);
    if (err != FSP_SUCCESS)
    {
        g_bUartDataReading = false;
        DBGTRACE("g_pUart->p_api->read() err = %d!\n", err);
        return err;
    }

    g_pfnOnDataReadCompleteCallback = pfnOnDataReadCompleteCallback;

    return FSP_SUCCESS;
}


int UartAsyncWrite(const uint8_t* const p_src, const uint32_t bytes)
{
    fsp_err_t err;

    if (g_pUart == NULL || g_bUartClosing)
        return FSP_ERR_NOT_OPEN;

    if (g_bUartDataWriting)
        return FSP_ERR_IN_USE;

    g_bUartDataWriting = true;
    err = g_pUart->p_api->write(g_pUart->p_ctrl, p_src, bytes);
    if (err != FSP_SUCCESS)
    {
        g_bUartDataWriting = false;
        DBGTRACE("g_pUart->p_api->write() err = %d!\n", err);
        return err;
    }

    g_nUartWriteCount = bytes;

    return FSP_SUCCESS;
}


int UartSyncRead(uint8_t* const p_dest, const uint32_t bytes)
{
    int nRet;

    nRet = UartAsyncRead(p_dest, bytes, NULL);
    if (nRet == FSP_SUCCESS)
        UartWaitReadReady();

    return nRet;
}


int UartSyncWrite(const uint8_t* const p_src, const uint32_t bytes)
{
    int nRet;

    UartWaitWriteReady();
    nRet = UartAsyncWrite(p_src, bytes);
    if (nRet == FSP_SUCCESS)
        UartWaitWriteReady();

    return nRet;
}


void UartWaitReadReady()
{
    int nCount = 20000;
    while (g_bUartDataReading && nCount-- > 0)
    {
        R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MICROSECONDS);
    }
}


void UartWaitWriteReady()
{
    int nCount = 20000;
    while (g_bUartDataWriting && nCount-- > 0)
    {
        R_BSP_SoftwareDelay(100U, BSP_DELAY_UNITS_MICROSECONDS);
    }
}


bool UartIsReading(void)
{
    return g_bUartDataReading;
}


bool UartIsWriting(void)
{
    return g_bUartDataWriting;
}


bool UartIsOpened(void)
{
    return (g_pUart != NULL);
}

// TODO: This API has problem, always return -1, don't use it.
int UartGetAvailableReadCount()
{
    fsp_err_t err;
    uart_info_t uart_info;

    if (g_pUart == NULL || g_bUartClosing)
        return 0;

    err = g_pUart->p_api->infoGet(g_pUart->p_ctrl, &uart_info);
    if (err != FSP_SUCCESS)
    {
        DBGTRACE("pUart->p_api->infoGet() err = %d!\n", err);
        return 0;
    }

    return (int)uart_info.read_bytes_max;
}


int UartAbortAyncIO(int nDir)
{
    fsp_err_t err;

    if (g_pUart == NULL || g_bUartClosing)
        return FSP_ERR_NOT_OPEN;

    err = g_pUart->p_api->communicationAbort(g_pUart->p_ctrl, nDir);
    if (err != FSP_SUCCESS)
    {
        DBGTRACE("g_pUart->p_api->communicationAbort() err = %d!\n", err);
        return err;
    }

    if (nDir == UART_DIR_TX || nDir == UART_DIR_RX_TX)
        g_bUartDataWriting = false;
    if (nDir == UART_DIR_RX || nDir == UART_DIR_RX_TX)
        g_bUartDataReading = false;

    return FSP_SUCCESS;
}


int UartAbortAyncRead()
{
    return UartAbortAyncIO(UART_DIR_RX);
}


int UartAbortAyncWrite()
{
    return UartAbortAyncIO(UART_DIR_TX);
}


char g_szUartTemp[256];
int UartTrace(const char *lpszFormat, ...)
{
    int nRet = FSP_ERR_NOT_OPEN;

    if (g_pUart != NULL)
    {
        __VALIST args;

        va_start(args, lpszFormat);
        UartWaitWriteReady();
        int n = vsnprintf(g_szUartTemp, 256, lpszFormat, args);
        if (n >= 0 && n < 256)
        {
            nRet = UartAsyncWrite((uint8_t const *)g_szUartTemp, (uint32_t)n);
            //UartWaitWriteReady();
        }
        else
           nRet = FSP_ERR_INVALID_ARGUMENT;
        va_end(args);
    }

    return nRet;
}


void user_uart_callback(uart_callback_args_t * p_args)
{
    if (g_nUartChannel == (int)p_args->channel)
    {
        switch (p_args->event)
        {
        case UART_EVENT_RX_COMPLETE:
            g_bUartDataReading = false;
            if (g_pfnOnDataReadCompleteCallback)
                g_pfnOnDataReadCompleteCallback();
            break;
        case UART_EVENT_TX_COMPLETE:
            g_bUartDataWriting = false;
            break;
        default:
            break;
        }
    }
}

/*
#include "hal_data.h"
#include "uart_ep.h"
#include "common_utils.h"
fsp_err_t uart_print_user_msg_uart(uint8_t *p_msg)
{
    fsp_err_t err   = FSP_SUCCESS;

    uint32_t msg_len = 0;
    char *p_temp_ptr = (char *)p_msg;

    // Calculate length of message received
    msg_len = ((uint8_t)(strlen(p_temp_ptr)));
    UartSyncWrite(p_msg, msg_len);
    //UartWaitWriteReady();
    //UartAsyncWrite(p_msg, msg_len);
    //UartWaitWriteReady();

    return err;
}

fsp_err_t uart_print_user_msg_org(uint8_t *p_msg)
{
    fsp_err_t err   = FSP_SUCCESS;

    uint32_t msg_len = 0;
    uint32_t local_timeout = (DATA_LENGTH * UINT16_MAX);
    char *p_temp_ptr = (char *)p_msg;

    // Calculate length of message received
    msg_len = ((uint8_t)(strlen(p_temp_ptr)));

    // Reset callback capture variable
    g_bUartDataWriting = true;

    // Writing to terminal
    err = R_SCI_UART_Write (&g_uart4_ctrl, p_msg, msg_len);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT ("\r\n**  R_SCI_UART_Write API Failed  **\r\n");
        g_bUartDataWriting = false;
        return err;
    }

    // Check for event transfer complete
    while (g_bUartDataWriting && (--local_timeout))
    {
    }

    if(0 == local_timeout)
    {
        err = FSP_ERR_TIMEOUT;
    }
    return err;
}

fsp_err_t uart_print_user_msg_uart1(uint8_t *p_msg)
{
    fsp_err_t err   = FSP_SUCCESS;

    uint32_t msg_len = 0;
    uint32_t local_timeout = (DATA_LENGTH * UINT16_MAX);
    char *p_temp_ptr = (char *)p_msg;

    // Calculate length of message received
    msg_len = ((uint8_t)(strlen(p_temp_ptr)));

    UartWaitWriteReady();
    UartAsyncWrite(p_msg, msg_len);

    // Check for event transfer complete
    while (g_bUartDataWriting && (--local_timeout))
    {
    }

    if(0 == local_timeout)
    {
        err = FSP_ERR_TIMEOUT;
    }
    return err;
}
*/
