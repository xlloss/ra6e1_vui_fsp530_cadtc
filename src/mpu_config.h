/*
 * mpu_config.h
 *
 *  Created on: Mar 8, 2023
 *      Author: a5123412
 */

#ifndef MPU_CONFIG_H_
#define MPU_CONFIG_H_

#include "hal_data.h"

#define ENABLE_MPU_CONFIG_FOR_EDMAC					0

#define ENABLE_MPU_CONFIG_FOR_QSPI					1

#define ENABLE_MPU_CONFIG_FOR_SRAM					1

#if ENABLE_MPU_CONFIG_FOR_EDMAC
void mpu_config_edmac(void);
#endif

#if ENABLE_MPU_CONFIG_FOR_QSPI
void mpu_config_quadspi(void);
#endif

#if ENABLE_MPU_CONFIG_FOR_SRAM
void mpu_config_sram_area(void);
#endif

#endif /* MPU_CONFIG_H_ */
