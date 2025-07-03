
/*
 * AHT10_Port.h
 *
 * Autor: Amilcar Rincon
 */

#ifndef INC_AHT10_PORT_H_
#define INC_AHT10_PORT_H_
#include "hal_data.h"
#include "AHT10.h"
#include "DbgTrace.h"
void AHT10_I2C_Init_Handler(void);
fsp_err_t AHT10_I2C_Send(uint8_t *pData, uint16_t Size);
fsp_err_t AHT10_I2C_Receive(uint8_t *pData, uint16_t Size);
void AHT10_I2C_Delay(uint32_t ret);
typedef bool bool_t;

#endif /* INC_AHT10_PORT_H_ */
