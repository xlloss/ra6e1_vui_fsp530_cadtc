#ifndef __FLASH_MGR_H_
#define __FLASH_MGR_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * For official version of Cyberon library, it will use the this flash API check license.
 * In configuration.xml, please:
 *   1. Add r_flash_hp component.
 *   2. Add Flash Driver stack, named "g_flash0" and set callback as "flash0_bgo_callback".
 */

// Property API for data flash.
int FlashGetDataSize(void);
int FlashGetBlockSize(void);
void* FlashGetBaseAddress(void);
void* FlashGetBlockAddress(int nBlockIndex);

// Operation API for data flash.
int FlashOpen(void);
int FlashClose(void);
bool FlashIsOpened(void);
int FlashErase(void *lpFlashBlockAddress, int nNumBlocks);
int FlashBlankCheck(void *lpFlashAddress, int nNumBytes);
int FlashWrite(void *lpFlashAddress, const uint8_t* lpData, int nDataSize);

#ifdef __cplusplus
}
#endif

#endif
