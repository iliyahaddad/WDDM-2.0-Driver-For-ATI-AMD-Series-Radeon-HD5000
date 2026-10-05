#ifndef _MADISON_DEBUG_H_
#define _MADISON_DEBUG_H_

#include "driver.h"

// Debug levels
#define MADISON_DEBUG_ERROR             0x00000001u
#define MADISON_DEBUG_WARN              0x00000002u
#define MADISON_DEBUG_INFO              0x00000004u
#define MADISON_DEBUG_MMIO              0x00000010u
#define MADISON_DEBUG_INTERRUPT         0x00000020u
#define MADISON_DEBUG_FIRMWARE          0x00000040u
#define MADISON_DEBUG_CP                0x00000080u
#define MADISON_DEBUG_RING              0x00000100u
#define MADISON_DEBUG_FENCE             0x00000200u
#define MADISON_DEBUG_RESET             0x00000400u
#define MADISON_DEBUG_ALL               0xFFFFFFFFu

extern ULONG g_MadisonDebugLevel;

#define MADISON_DEBUG(_level, _fmt, ...)                         \
    do {                                                         \
        if ((g_MadisonDebugLevel & (_level)) != 0) {             \
            DbgPrint("MadisonKMD: " _fmt "\n", ##__VA_ARGS__);   \
        }                                                        \
    } while (0)

#define MADISON_ERROR(_fmt, ...)    MADISON_DEBUG(MADISON_DEBUG_ERROR, "ERROR: " _fmt, ##__VA_ARGS__)
#define MADISON_WARN(_fmt, ...)     MADISON_DEBUG(MADISON_DEBUG_WARN,  "WARN: " _fmt, ##__VA_ARGS__)
#define MADISON_INFO(_fmt, ...)     MADISON_DEBUG(MADISON_DEBUG_INFO,  "INFO: " _fmt, ##__VA_ARGS__)
#define MADISON_MMIO(_fmt, ...)     MADISON_DEBUG(MADISON_DEBUG_MMIO,  "MMIO: " _fmt, ##__VA_ARGS__)
#define MADISON_LOG_IRQ(_fmt, ...)      MADISON_DEBUG(MADISON_DEBUG_INTERRUPT, "IRQ: " _fmt, ##__VA_ARGS__)
#define MADISON_FW(_fmt, ...)       MADISON_DEBUG(MADISON_DEBUG_FIRMWARE, "FW: " _fmt, ##__VA_ARGS__)
#define MADISON_CP(_fmt, ...)       MADISON_DEBUG(MADISON_DEBUG_CP,  "CP: " _fmt, ##__VA_ARGS__)
#define MADISON_LOG_RING(_fmt, ...)     MADISON_DEBUG(MADISON_DEBUG_RING, "RING: " _fmt, ##__VA_ARGS__)
#define MADISON_LOG_FENCE(_fmt, ...)    MADISON_DEBUG(MADISON_DEBUG_FENCE, "FENCE: " _fmt, ##__VA_ARGS__)
#define MADISON_RESET(_fmt, ...)    MADISON_DEBUG(MADISON_DEBUG_RESET, "RESET: " _fmt, ##__VA_ARGS__)

#endif // _MADISON_DEBUG_H_
