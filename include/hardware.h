#ifndef _MADISON_HARDWARE_H_
#define _MADISON_HARDWARE_H_

#include "driver.h"

// ============================================================================
// Madison / Juniper Hardware Constants
// Source: Linux kernel drivers/gpu/drm/radeon/evergreen.c, evergreend.h, r600d.h
// Ring/GART size macros live in driver.h (single source of truth).
// ============================================================================

#define MADISON_GART_PAGE_SHIFT          12

// CP_RB_CNTL fields (Linux evergreen_cp_resume): BUFSZ = log2(ring_bytes/8),
// BLKSZ = log2(GPU_page_size/8)
#define MADISON_RING_BUFSZ               15   // log2(256 KiB / 8)
#define MADISON_RING_BLKSZ               9    // log2(4096 / 8)

// ----------------------------------------------------------------------------
// PM4 packet encodings (Linux r600d.h / evergreend.h)
//   PACKET0: register offset is a DWORD index (byte offset >> 2)
//   PACKET3: opcode lives in bits [15:8], count (payload DWORDs - 1) in [29:16]
// ----------------------------------------------------------------------------
#define PACKET_TYPE0                     0x00000000u
#define PACKET_TYPE2                     0x80000000u
#define PACKET_TYPE3                     0xC0000000u

#define PACKET0(_reg, _n) \
    (PACKET_TYPE0 | (((ULONG)(_reg) >> 2) & 0xFFFFu) | (((ULONG)(_n) & 0x3FFFu) << 16))
#define PACKET3(_op, _n) \
    (PACKET_TYPE3 | (((ULONG)(_op) & 0xFFu) << 8) | (((ULONG)(_n) & 0x3FFFu) << 16))

/* Legacy names kept for source compatibility. */
#define PACKET0_SET_BASE(_reg, _n)       PACKET0(_reg, _n)
#define PACKET3_SET(_op, _n)             PACKET3(_op, _n)

// Evergreen PM4 opcodes (VERIFY against evergreend.h before hardware use)
#define PACKET3_NOP                      0x10
#define PACKET3_INDIRECT_BUFFER          0x32
#define PACKET3_INDIRECT_BUFFER_CONST    0x33
#define PACKET3_WRITE_DATA               0x37
#define PACKET3_SEMAPHORE                0x39
#define PACKET3_WAIT_REG_MEM             0x3C
#define PACKET3_SURFACE_SYNC             0x43
#define PACKET3_ME_INITIALIZE            0x44
#define PACKET3_EVENT_WRITE              0x46
#define PACKET3_EVENT_WRITE_EOP          0x47
#define PACKET3_SET_CONFIG_REG           0x68
#define PACKET3_SET_CONTEXT_REG          0x69

#define PACKET3_ME_INITIALIZE_DEVICE_ID(_x) ((ULONG)(_x) << 16)

// SURFACE_SYNC CP_COHER_CNTL bits
#define PACKET3_TC_ACTION_ENA            (1u << 23)
#define PACKET3_VC_ACTION_ENA            (1u << 24)
#define PACKET3_SH_ACTION_ENA            (1u << 27)

// EVENT_WRITE_EOP fields
#define EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT_TS  0x14u
#define EVENT_INDEX(_x)                  ((ULONG)(_x) << 8)
#define EVENT_INDEX_5                    EVENT_INDEX(5)
#define DATA_SEL(_x)                     ((ULONG)(_x) << 29)
#define INT_SEL(_x)                      ((ULONG)(_x) << 24)
#define DATA_SEL_1                       DATA_SEL(1)   /* write 32-bit value */
#define INT_SEL_2                        INT_SEL(2)    /* interrupt after data confirmed */

// Evergreen (Juniper) maximum hardware contexts - 1, as sent in ME_INITIALIZE
#define MADISON_ME_MAX_CONTEXT_INDEX     7u

// Number of DWORDs MadisonFenceEmit() produces
#define MADISON_FENCE_EMIT_DWORDS        11u

// IH ring size (must be power of 2)
#define MADISON_IH_RING_SIZE             (4 * 1024)

// Largest physical address the 40-bit GART PTE / MC can reach
#define MADISON_MAX_PHYS_ADDR            0xFFFFFFFFFFull

#define MADISON_MIN(_a, _b)              (((_a) < (_b)) ? (_a) : (_b))

#endif // _MADISON_HARDWARE_H_
