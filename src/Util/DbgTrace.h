#ifndef _DBG_TRACE_H
#define _DBG_TRACE_H

#if defined(SEGGER_LOG)
#include "common_utils.h"
#endif

#if defined(UART_LOG)
#include "UartMgr.h"
#endif

#if defined(UART_LOG) || defined(SEGGER_LOG)
    #define DBG_UART_TRACE    DbgUartTrace
#else
    #define DBG_UART_TRACE    1 ? (void)0 : EmptyTrace
#endif


#if defined(UART_LOG)
    #define UART_TRACE        UartTrace
#else
    #define UART_TRACE        1 ? (void)0 : EmptyTrace
#endif


#if defined(SEGGER_LOG)
    #define DBGTRACE          APP_PRINT
#else
    #define DBGTRACE          1 ? (void)0 : EmptyTrace
#endif

#ifdef __cplusplus
extern "C" {
#endif

//void DbgTrace(const char *lpszFormat, ...);
void DbgUartTrace(const char *lpszFormat, ...);

inline void EmptyTrace(const char *lpszFormat, ...) {(void)(lpszFormat);}

#ifdef __cplusplus
}
#endif

#endif //_DBG_TRACE_H
