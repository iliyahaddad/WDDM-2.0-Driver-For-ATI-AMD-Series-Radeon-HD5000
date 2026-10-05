#ifndef _MADISON_REGISTERS_H_
#define _MADISON_REGISTERS_H_

// ============================================================================
// Evergreen / Madison Register Definitions
// Source: Linux kernel drivers/gpu/drm/radeon/evergreen_reg.h, evergreend.h
// ============================================================================

// ----------------------------------------------------------------------------
// GRBM (Graphics Register Bus Master) Block
// ----------------------------------------------------------------------------
#define GRBM_STATUS                     0x8010
#define GRBM_STATUS2                    0x8008
#define GRBM_SOFT_RESET                 0x8020
#define GRBM_CNTL                       0x8000

// GRBM_STATUS bit definitions
#define GRBM_STATUS__GUI_ACTIVE_MASK                     0x80000000u
#define GRBM_STATUS__CB_BUSY_MASK                       0x40000000u
#define GRBM_STATUS__DB_BUSY_MASK                       0x20000000u
#define GRBM_STATUS__CP_BUSY_MASK                       0x10000000u
#define GRBM_STATUS__PA_BUSY_MASK                       0x08000000u
#define GRBM_STATUS__SC_BUSY_MASK                       0x04000000u
#define GRBM_STATUS__SPI_BUSY_MASK                      0x02000000u
#define GRBM_STATUS__SX_BUSY_MASK                       0x01000000u
#define GRBM_STATUS__TA_BUSY_MASK                       0x00800000u
#define GRBM_STATUS__TC_BUSY_MASK                       0x00400000u
#define GRBM_STATUS__VGT_BUSY_MASK                      0x00200000u
#define GRBM_STATUS__IA_BUSY_MASK                       0x00100000u

// GRBM_SOFT_RESET bit definitions
#define GRBM_SOFT_RESET__SOFT_RESET_CP                  (1 << 0)
#define GRBM_SOFT_RESET__SOFT_RESET_CB                  (1 << 1)
#define GRBM_SOFT_RESET__SOFT_RESET_DB                  (1 << 3)
#define GRBM_SOFT_RESET__SOFT_RESET_GDS                 (1 << 4)
#define GRBM_SOFT_RESET__SOFT_RESET_PA                  (1 << 5)
#define GRBM_SOFT_RESET__SOFT_RESET_SH                  (1 << 9)
#define GRBM_SOFT_RESET__SOFT_RESET_SC                  (1 << 6)
#define GRBM_SOFT_RESET__SOFT_RESET_SPI                 (1 << 8)
#define GRBM_SOFT_RESET__SOFT_RESET_SX                  (1 << 10)
#define GRBM_SOFT_RESET__SOFT_RESET_TC                  (1 << 11)
#define GRBM_SOFT_RESET__SOFT_RESET_TA                  (1 << 12)
#define GRBM_SOFT_RESET__SOFT_RESET_VGT                 (1 << 14)
#define GRBM_SOFT_RESET__SOFT_RESET_IA                  (1 << 15)

// ----------------------------------------------------------------------------
// Command Processor (CP) Registers — Evergreen/Juniper exact offsets
// ----------------------------------------------------------------------------
#define CP_ME_CNTL                      0x86D8
#define CP_ME_STATUS                    0x86D4
#define CP_PFP_UCODE_ADDR               0xC150
#define CP_PFP_UCODE_DATA               0xC154
#define CP_ME_RAM_RADDR                 0xC158
#define CP_ME_RAM_WADDR                 0xC15C
#define CP_ME_RAM_DATA                  0xC160
#define CP_RB_BASE                      0xC100
#define CP_RB_CNTL                      0xC104
#define CP_RB_RPTR_WR                   0xC108
#define CP_RB_RPTR_ADDR                 0xC10C
#define CP_RB_RPTR_ADDR_HI              0xC110
#define CP_RB_WPTR                      0xC114
#define CP_RB_WPTR_ADDR                 0xC118
#define CP_RB_WPTR_ADDR_HI              0xC11C
#define CP_RB_RPTR                      0x8700
#define CP_RB_WPTR_DELAY                0x8704
#define CP_DEBUG                        0xC1FC
#define CP_INT_CNTL                     0xC124
#define CP_INT_STATUS                   0xC128
#define CP_ME_HALT                      (1u << 28)
#define CP_PFP_HALT                     (1u << 26)
#define RB_BUFSZ(x)                     ((ULONG)(x) << 0)
#define RB_BLKSZ(x)                     ((ULONG)(x) << 8)
#define RB_NO_UPDATE                    (1u << 27)
#define RB_RPTR_WR_ENA                  (1u << 31)
#define BUF_SWAP_32BIT                  (2u << 16)

// ----------------------------------------------------------------------------
// SRBM (System Register Bus Master) Block
// ----------------------------------------------------------------------------
#define SRBM_STATUS                     0xE50
#define SRBM_SOFT_RESET                 0x0E60

// SRBM_SOFT_RESET bit definitions
#define SRBM_SOFT_RESET__SOFT_RESET_BIF                  (1 << 1)
#define SRBM_SOFT_RESET__SOFT_RESET_DC                   (1 << 5)
#define SRBM_SOFT_RESET__SOFT_RESET_GRBM                 (1 << 8)
#define SRBM_SOFT_RESET__SOFT_RESET_HDP                  (1 << 9)
#define SRBM_SOFT_RESET__SOFT_RESET_IH                   (1 << 10)
#define SRBM_SOFT_RESET__SOFT_RESET_MC                   (1 << 11)
#define SRBM_SOFT_RESET__SOFT_RESET_ROM                  (1 << 14)
#define SRBM_SOFT_RESET__SOFT_RESET_SEM                  (1 << 15)
#define SRBM_SOFT_RESET__SOFT_RESET_VMC                  (1 << 17)
#define SRBM_SOFT_RESET__SOFT_RESET_DMA                  (1 << 20)

// ----------------------------------------------------------------------------
// MC_VM (Memory Controller / Virtual Memory) Registers
// ----------------------------------------------------------------------------
#define MC_VM_MX_L1_TLB_CNTL            0x2064
#define MC_VM_MD_L1_TLB0_CNTL           0x2654
#define MC_VM_MD_L1_TLB1_CNTL           0x2658
#define MC_VM_MD_L1_TLB2_CNTL           0x265C
#define MC_VM_MD_L1_TLB3_CNTL           0x2698  // Juniper-specific
#define MC_VM_MB_L1_TLB0_CNTL           0x2234
#define MC_VM_MB_L1_TLB1_CNTL           0x2238
#define MC_VM_MB_L1_TLB2_CNTL           0x223C
#define MC_VM_MB_L1_TLB3_CNTL           0x2240

#define VM_CONTEXT0_PAGE_TABLE_START_ADDR 0x155C
#define VM_CONTEXT0_PAGE_TABLE_END_ADDR   0x157C
#define VM_CONTEXT0_PAGE_TABLE_BASE_ADDR  0x153C
#define VM_CONTEXT0_CNTL                  0x1410
#define VM_CONTEXT0_PROTECTION_FAULT_DEFAULT_ADDR 0x1518

#define VM_CONTEXT1_CNTL                  0x1414
#define VM_CONTEXT0_REQUEST_RESPONSE     0x1470
#define HDP_MEM_COHERENCY_FLUSH_CNTL     0x5480

// VM_CONTEXT0_CNTL bit definitions (Linux evergreend.h)
#define VM_CONTEXT0_CNTL__ENABLE_CONTEXT_MASK            0x00000001u
#define PAGE_TABLE_DEPTH(x)                              (((x) & 3u) << 1)
#define RANGE_PROTECTION_FAULT_ENABLE_DEFAULT            (1u << 4)

// L1 TLB control bits (Linux evergreend.h)
#define ENABLE_L1_TLB                                    (1u << 0)
#define ENABLE_L1_FRAGMENT_PROCESSING                    (1u << 1)
#define SYSTEM_ACCESS_MODE_NOT_IN_SYS                    (3u << 3)
#define SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU        (0u << 5)
#define EFFECTIVE_L1_TLB_SIZE(x)                         ((ULONG)(x) << 15)
#define EFFECTIVE_L1_QUEUE_SIZE(x)                       ((ULONG)(x) << 18)

// VM_CONTEXT0_REQUEST_RESPONSE
#define REQUEST_TYPE(x)                                  (((ULONG)(x) & 0xFu) << 0)
#define RESPONSE_TYPE_MASK                               0x000000F0u
#define RESPONSE_TYPE_SHIFT                              4

// ----------------------------------------------------------------------------
// IH (Interrupt Handler) Registers - Evergreen interrupts are delivered via the
// IH ring (not directly through CP_INT_STATUS). The IH ring is NOT implemented yet.
// ----------------------------------------------------------------------------
#define IH_RB_BASE                       0x3E04
#define IH_RB_CNTL                       0x3E00
#define IH_RB_RPTR                       0x3E08
#define IH_RB_WPTR                       0x3E0C
#define IH_CNTL                          0x3E18

// ----------------------------------------------------------------------------
// DMA Registers (common on r6xx/r7xx/evergreen/ni)
// ----------------------------------------------------------------------------
#define DMA_RB_CNTL                      0xD000
#define DMA_RB_BASE                      0xD004
#define DMA_RB_RPTR                      0xD008
#define DMA_RB_WPTR                      0xD00C

// ----------------------------------------------------------------------------
// Aperture / Framebuffer
// ----------------------------------------------------------------------------
#define MC_FB_LOCATION                   0x2024
#define MC_AGP_LOCATION                  0x2028
#define CONFIG_MEMSIZE                   0x5428
#define MC_SHARED_CHMAP                  0x2004
#define MC_ARB_RAMCFG                   0x2760
#define CHANSIZE_MASK                   0x00000100u
#define CHANSIZE_OVERRIDE               (1u << 11)
#define NOOFCHAN_SHIFT                  12
#define NOOFCHAN_MASK                   0x00003000u
#define VM_L2_CNTL                       0x1400
#define VM_L2_CNTL2                      0x1404
#define VM_L2_CNTL3                      0x1408
#define ENABLE_L2_CACHE                  (1u << 0)
#define ENABLE_L2_FRAGMENT_PROCESSING   (1u << 1)
#define ENABLE_L2_PTE_CACHE_LRU_UPDATE_BY_WRITE (1u << 9)
#define EFFECTIVE_L2_QUEUE_SIZE(x)      (((x) & 7u) << 14)
#define BANK_SELECT(x)                  ((x) << 0)
#define CACHE_UPDATE_MODE(x)            ((x) << 6)
#define MC_VM_SYSTEM_APERTURE_LOW_ADDR  0x2034
#define MC_VM_SYSTEM_APERTURE_HIGH_ADDR 0x2038
#define MC_VM_SYSTEM_APERTURE_DEFAULT_ADDR 0x203C
#define MC_VM_FB_OFFSET                 0x2068
#define MC_VM_FB_LOCATION               0x2024
#define VGA_HDP_CONTROL                 0x328
#define VGA_MEMORY_DISABLE              (1u << 4)
#define HDP_NONSURFACE_BASE            0x2C04
#define HDP_NONSURFACE_INFO            0x2C08
#define HDP_NONSURFACE_SIZE            0x2C0C

// ----------------------------------------------------------------------------
// Scratch Registers
// ----------------------------------------------------------------------------
#define SCRATCH_REG0                     0x8500
#define SCRATCH_REG1                     0x8504
#define SCRATCH_REG2                     0x8508
#define SCRATCH_REG3                     0x850C
#define SCRATCH_REG4                     0x8510
#define SCRATCH_REG5                     0x8514
#define SCRATCH_REG6                     0x8518
#define SCRATCH_REG7                     0x851C
#define SCRATCH_UMSK                     0x8540
#define SCRATCH_ADDR                     0x8544

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------
#define REG32(_adapter, _reg) \
    READ_REGISTER_ULONG((PULONG)((_adapter)->RegisterBaseVirtual + (_reg)))

#define WR_REG32(_adapter, _reg, _value) \
    WRITE_REGISTER_ULONG((PULONG)((_adapter)->RegisterBaseVirtual + (_reg)), (_value))

#endif // _MADISON_REGISTERS_H_
