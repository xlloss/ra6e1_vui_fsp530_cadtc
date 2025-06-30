#ifndef __UART_MGR_H
#define __UART_MGR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PfnOnDataReadCompleteCallback)(void);

int UartOpen(const void *pUart, int nUartChannel, int nBaudRate);
int UartClose(void);
int UartAsyncRead(uint8_t* const p_dest, const uint32_t bytes, PfnOnDataReadCompleteCallback pfnOnDataReadCompleteCallback);
int UartAsyncWrite(const uint8_t* const p_src, const uint32_t bytes);
int UartSyncRead(uint8_t* const p_dest, const uint32_t bytes);
int UartSyncWrite(const uint8_t* const p_src, const uint32_t bytes);
void UartWaitReadReady(void);
void UartWaitWriteReady(void);
bool UartIsReading(void);
bool UartIsWriting(void);
bool UartIsOpened(void);
int UartGetAvailableReadCount();
int UartAbortAyncRead();
int UartAbortAyncWrite();

int UartTrace(const char *lpszFormat, ...);

#ifdef __cplusplus
}
#endif

#endif
