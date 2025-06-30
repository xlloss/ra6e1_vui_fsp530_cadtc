#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "bsp_api.h"
#include "DbgTrace.h"
#include "UartMgr.h"

#if defined(SEGGER_LOG)
#include "SEGGER_RTT.h"
#endif

extern char g_szUartTemp[256];

void DbgUartTrace(const char *lpszFormat, ...)
{
    __VALIST args;

    va_start(args, lpszFormat);

#if defined(SEGGER_LOG)
    SEGGER_RTT_vprintf(0, lpszFormat, &args);
#endif

#if defined(UART_LOG)
    if (UartIsOpened())
    {
        UartWaitWriteReady();
        int n = vsnprintf(g_szUartTemp, 256, lpszFormat, args);
        if (n >= 256)
        {
            n = 256;
            g_szUartTemp[253] = '\r';
            g_szUartTemp[254] = '\n';
            g_szUartTemp[255] = 0;
        }

        if (n >= 0)
        {
            UartAsyncWrite((const uint8_t*)g_szUartTemp, (uint32_t)n);
            //UartWaitWriteReady();
        }
    }
#endif
    va_end(args);
}

