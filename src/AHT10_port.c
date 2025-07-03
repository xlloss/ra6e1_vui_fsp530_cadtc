/*
 * AHT10_Port.c
 *
 * Author: Amilcar Rincon
 */
#include "AHT10_port.h"

#define RESET_VALUE 0
static volatile i2c_master_event_t g_master_event = (i2c_master_event_t)RESET_VALUE;

void AHT10_I2C_Init_Handler(void)
{
    fsp_err_t err = FSP_SUCCESS;

    err = R_SCI_I2C_Open(&g_i2c3_ctrl, &g_i2c3_cfg);
    assert(FSP_SUCCESS == err);
}

fsp_err_t AHT10_I2C_Send(uint8_t *pData, uint16_t Size)
{
    fsp_err_t write_err = FSP_SUCCESS;

    write_err = R_SCI_I2C_Write(&g_i2c3_ctrl, pData, Size, false);
    if (FSP_SUCCESS != write_err)
        DBG_UART_TRACE("\r\nI2C Write FAILED\r\n");

    return write_err;
}

fsp_err_t AHT10_I2C_Receive(uint8_t *pData, uint16_t Size)
{
    fsp_err_t read_err = FSP_SUCCESS;
    int timeout = 100;

    read_err = R_SCI_I2C_Read(&g_i2c3_ctrl, pData, Size, false);
    if (FSP_SUCCESS != read_err)
        DBG_UART_TRACE("I2C Read FAILED\r\n");

    while (I2C_MASTER_EVENT_RX_COMPLETE != g_master_event)
    {
        timeout--;
        if (timeout <= 0)
        {
            DBG_UART_TRACE("I2C Read timeout\r\n");
            read_err = FSP_ERR_HARDWARE_TIMEOUT;
            break;
        }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }

    return read_err;
}

void AHT10_I2C_Delay(uint32_t ret)
{
    R_BSP_SoftwareDelay(ret, BSP_DELAY_UNITS_MILLISECONDS);
}

void sci_i2c_master_callback(i2c_master_callback_args_t * p_args)
{
    if (NULL != p_args)
        g_master_event = p_args->event;

    if (I2C_MASTER_EVENT_ABORTED == g_master_event)
        DBG_UART_TRACE ("I2C ABORTED I2C-M\r\n");

}
