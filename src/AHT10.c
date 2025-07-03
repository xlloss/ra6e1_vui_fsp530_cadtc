/*
 * AHT10.c
 *
 * Autor: Amilcar Rincon
 */

#include "AHT10.h"
#include "AHT10_port.h"

uint8_t buffer_read[6];
uint8_t buffer_transmission[3];
const uint32_t timeout = 100;
uint16_t size;
uint16_t size_read;
int temperatura = -50;
int humedad = -50;

/**
 * @brief AHT10  Funcion de inicializacion
 * @param None
 * @retval None
 */
void AHT10_Init(void)
{
  fsp_err_t err;

  AHT10_I2C_Init_Handler();
  buffer_transmission[0] = AHT10_INIT_CMD;
  buffer_transmission[1] = 0x08;
  buffer_transmission[2] = AHT10_DATA_NOP;
  size = sizeof(buffer_transmission) / sizeof(uint8_t);
  err = AHT10_I2C_Send(buffer_transmission, size);
  AHT10_I2C_Delay(50);
}

/**
 * @brief AHT10  Funcion de lectura de temperatura
 * @param None
 * @retval float temperatura
 */
int AHT10_Read_Temp(void)
{
  int tmp1, tmp2;
  buffer_transmission[0] = AHT10_START_MEASURMENT_CMD;
  buffer_transmission[1] = AHT10_DATA_MEASURMENT_CMD;
  buffer_transmission[2] = AHT10_DATA_NOP;
  size = sizeof(buffer_transmission) / sizeof(uint8_t);
  AHT10_I2C_Send(buffer_transmission, size);
  AHT10_I2C_Delay(200);

  size_read = sizeof(buffer_read) / sizeof(uint8_t);
  AHT10_I2C_Receive(buffer_read, size_read);
  /* 20-bit raw temperature data */
  tmp1 = ((buffer_read[3] & 0x0F) << 16) | (buffer_read[4] << 8) | buffer_read[5];
  tmp1 = tmp1 * 200;
  tmp2 = tmp1 / 1048576;
  temperatura = tmp2 - 50;

  return temperatura;
}
/**
 * @brief AHT10  Funcion lectura de humedad
 * @param None
 * @retval int humedad
 */
int AHT10_Read_Hum(void)
{
  buffer_transmission[0] = AHT10_START_MEASURMENT_CMD;
  buffer_transmission[1] = AHT10_DATA_MEASURMENT_CMD;
  buffer_transmission[2] = AHT10_DATA_NOP;
  AHT10_I2C_Send(buffer_transmission, 3);
  AHT10_I2C_Delay(10);
  size = sizeof(buffer_read) / sizeof(uint8_t);
  AHT10_I2C_Receive(buffer_read, size);
  // 20-bit raw temperature data
  return humedad = ((((buffer_read[1] << 16) |
                    (buffer_read[2] << 8) |
                    buffer_read[3]) >> 4) * 100 / 1048576);
}

void AHT10_Reset(void)
{
  fsp_err_t err;

  AHT10_I2C_Init_Handler();
  buffer_transmission[0] = AHT10_SOFT_RESET_CMD;
  size = 1;
  err = AHT10_I2C_Send(buffer_transmission, size);
  if (err != FSP_SUCCESS)
      DBG_UART_TRACE("I2C Send fail\r\n");

  AHT10_I2C_Delay(50);
}
