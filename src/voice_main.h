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

#ifndef VOICE_MAIN_H
#define VOICE_MAIN_H

/*******************************************************************************
 Include
*******************************************************************************/
#include "hal_data.h"

/*******************************************************************************
 Macro definitions
*******************************************************************************/

// Definition for support function
#define SUPPORT_SPEEX_PLAY                               // To decode Speex compressed audio and play it after getting the result.
//#define SUPPORT_RECOG_TWO_MODEL                          // To recognize two model by two DSpotter instance at the same time, the CPU/RAM/Model_ROM usage will double.
#define SUPPORT_VOICE_TAG                                // To support voice tag function.
#define SUPPORT_UART_DUMP_RECORD                         // Transfer record data through UART(460800 bps) to PC CybSerialRecorder.

#define NOT_SHOW_MULTI_PRONUNCIATION                     // Don't display the command string that include '^' character.


// Definition for recognition flow
#define RECOG_FLOW_NONE          0                       // Record only, no recognition.
#define RECOG_FLOW_NORMAL        1                       // We treat the first group as trigger words and the second group as command words if it exist.


#define RECOG_FLOW               RECOG_FLOW_NORMAL


// Definition for group index
#define GROUP_INDEX_TRIGGER      0                       // The group index of trigger.
#define GROUP_INDEX_COMMAND      1                       // The group index of command.


// Definition for audio record.
#define AUDIO_RECORD_I2S         0
#define AUDIO_RECORD_SPI         1
#define AUDIO_RECORD_AMIC        2

#ifndef AUDIO_RECORD
    #define AUDIO_RECORD             AUDIO_RECORD_I2S
#endif


// Definition for MCU and board type.
#define MCU_BOARD_RA6E1_VOICE    614    // VOICE-RA6E1
#define MCU_BOARD_RA4E1_VOICE    413    // VOICE-RA4E1
#define MCU_BOARD_RA8M1_VOICE    810    // VOICE-RA8M1
#define MCU_BOARD_RA8D1_EK       811    // EK-RA8D1

#if (MCU_BOARD == MCU_BOARD_RA6E1_VOICE)
    #define MCU_BOARD_STRING                        "RA6E1_VOICE"

    /** Data flash */
    #define FLASH_DF_SIZE                           (8192)
    #define FLASH_DF_BLOCK_SIZE                     (64)
    #define FLASH_DF_BLOCK_BASE_ADDRESS             0x08000000U

    /** LED for VOICE-RA6E1 */
    #define LED_R                                   BSP_IO_PORT_05_PIN_00
    #define LED_G                                   BSP_IO_PORT_04_PIN_00
    #define LED_B                                   BSP_IO_PORT_01_PIN_13
    #define OFF                                     (1U)
    #define ON                                      (0U)

    /** ADC0 ADDR0 Address, DMA settings */
    #define VD_PRV_DMA_BUF_MAX                      (2)                             /*!<  DMA Buffer Max */
    #define VD_PRV_DMA_BUF_LEN                      (160)                           /*!<  DMA Size 160 (16KHz) */
    #define VD_PRV_ADC0_ADDR                        (0x40170020)                    /*!<  address of ADC channel 0 */
    #define VD_PRV_ADC1_ADDR                        (0x40170022)                    /*!<  address of ADC channel 1 */
    #define VD_PRV_INT16_MIN                        (-32768)                        /*!<  int16_t MIN */
    #define VD_PRV_INT16_MAX                        (32767)                         /*!<  int16_t MAX */

#elif (MCU_BOARD == MCU_BOARD_RA4E1_VOICE)
    #define MCU_BOARD_STRING                        "RA4E1_VOICE"

    /** Data flash */
    #define FLASH_DF_SIZE                           (8192)
    #define FLASH_DF_BLOCK_SIZE                     (64)
    #define FLASH_DF_BLOCK_BASE_ADDRESS             0x08000000U

    /** LED for VOICE-RA4E1 */
    #define LED_R                                   BSP_IO_PORT_05_PIN_00
    #define LED_G                                   BSP_IO_PORT_02_PIN_13
    #define LED_B                                   BSP_IO_PORT_02_PIN_12
    #define OFF                                     (1U)
    #define ON                                      (0U)

    /** ADC0 ADDR0 Address, DMA settings */
    #define VD_PRV_DMA_BUF_MAX                      (2)                             /*!<  DMA Buffer Max */
    #define VD_PRV_DMA_BUF_LEN                      (160)                           /*!<  DMA Size 160 (16KHz) */
    #define VD_PRV_ADC0_ADDR                        (0x40170020)                    /*!<  address of ADC channel 0 */
    #define VD_PRV_ADC1_ADDR                        (0x40170022)                    /*!<  address of ADC channel 1 */
    #define VD_PRV_INT16_MIN                        (-32768)                        /*!<  int16_t MIN */
    #define VD_PRV_INT16_MAX                        (32767)                         /*!<  int16_t MAX */

#elif (MCU_BOARD == MCU_BOARD_RA8D1_EK)
    #define MCU_BOARD_STRING                        "RA8D1_EK"

    /** Data flash */
    #define FLASH_DF_SIZE                           (12288)
    #define FLASH_DF_BLOCK_SIZE                     (64)
    #define FLASH_DF_BLOCK_BASE_ADDRESS             0x27000000U

    /** LED for EK-RA8D1 */
    #define LED_R                                   BSP_IO_PORT_01_PIN_07
    #define LED_G                                   BSP_IO_PORT_04_PIN_14
    #define LED_B                                   BSP_IO_PORT_06_PIN_00
    #define OFF                                     (0U)
    #define ON                                      (1U)

#elif (MCU_BOARD == MCU_BOARD_RA8M1_VOICE)
	#define D_CACHE_ENABLE

    #define MCU_BOARD_STRING                        "RA8M1_VOICE"

    /** Data flash */
    #define FLASH_DF_SIZE                           (12288)
    #define FLASH_DF_BLOCK_SIZE                     (64)
    #define FLASH_DF_BLOCK_BASE_ADDRESS             0x27000000U

    /** LED for VOICE-RA8M1 */
    #define LED_R                                   BSP_IO_PORT_04_PIN_04
    #define LED_G                                   BSP_IO_PORT_04_PIN_05
    #define LED_B                                   BSP_IO_PORT_04_PIN_06
    #define OFF                                     (1U)
    #define ON                                      (0U)

    /** ADC0 ADDR0 Address, DMA settings */
    #define VD_PRV_DMA_BUF_MAX                      (2)                             /*!<  DMA Buffer Max */
    #define VD_PRV_DMA_BUF_LEN                      (160)                           /*!<  DMA Size 160 (16KHz) */
    #define VD_PRV_ADC0_ADDR                        (0x40332020)                    /*!<  address of ADC channel 0 (AN000) */
    #define VD_PRV_ADC1_ADDR                        (0x40332022)                    /*!<  address of ADC channel 1 (AN001) */
    #define VD_PRV_INT16_MIN                        (-32768)                        /*!<  int16_t MIN */
    #define VD_PRV_INT16_MAX                        (32767)                         /*!<  int16_t MAX */

#ifdef D_CACHE_ENABLE
	/* if the SSI receive buffer is not put in the DTCM (non-cacheable),
	 * we need to do cache invalidate before start the read transfer */
	#define ALIGN_BASE2_CEIL(nSize, nAlign)  ( ((nSize) + ((nAlign) - 1)) & ~((nAlign) - 1) )
#endif
#endif

/*******************************************************************************
 Exported global variables
*******************************************************************************/

/*******************************************************************************
 Exported global functions
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void voice_main(void);
void ToggleLED(bsp_io_port_pin_t oLED);
void TurnOnLED(bsp_io_port_pin_t oLED);
void TurnOffLED(bsp_io_port_pin_t oLED);

#ifdef __cplusplus
}
#endif

#endif /* VOICE_MAIN_H */

/*******************************************************************************
 End Of File
*******************************************************************************/
