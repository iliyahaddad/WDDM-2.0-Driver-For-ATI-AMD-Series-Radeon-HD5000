# Hardware-Mapping — Madison WDDM 2.0 Physical-Mode Prototype

## Linux-to-Windows Hardware Mapping

This document maps Linux Radeon Evergreen hardware operations to Windows KMD equivalents.

## Source: Linux Radeon Driver

Primary reference: `drivers/gpu/drm/radeon/evergreen.c`

Key functions:
- `evergreen_init()` — ASIC initialization
- `evergreen_startup()` — Start GPU after init
- `evergreen_cp_init()` — Command processor init
- `evergreen_cp_start()` — Start CP
- `evergreen_cp_resume()` — Resume CP after suspend
- `evergreen_cp_fini()` — CP cleanup
- `evergreen_gpu_soft_reset()` — GPU soft reset
- `evergreen_pcie_gart_enable()` — GART enable
- `evergreen_mc_init()` — Memory controller init
- `evergreen_scratch_init()` — Scratch register init
- `evergreen_irq_set()` — IRQ enable/disable
- `evergreen_irq_process()` — IRQ handler
- `radeon_ring_init()` — Ring buffer init
- `r600_ring_test()` — Ring test
- `radeon_fence_emit()` — Emit fence
- `radeon_fence_wait()` — Wait for fence

## Hardware Operation → Windows KMD Equivalent

### 1. PCI Enumeration

**Linux**: `pci_get_device(PCI_VENDOR_ID_ATI, PCI_DEVICE_ID_ATI_JUNIPER, ...)`

**Windows**: `DxgkDdiAddDevice` is called by Dxgkrnl when the PCI device is enumerated.

**KMD Action**: In `DxgkDdiAddDevice`, allocate adapter context and store the device handle.

### 2. MMIO Mapping

**Linux**: `pci_iomap(pdev, 2, 0)` — maps BAR 2 (register space)

**Windows**: In `DxgkDdiStartDevice`, call `MmMapIoSpace` with the physical address and length from the PCI config space.

**KMD Action**:
```c
pAdapter->RegisterBase = PhysicalAddress;
pAdapter->RegisterLength = Length;
pAdapter->RegisterBaseVirtual = MmMapIoSpace(PhysicalAddress, Length, MmNonCached);
```

### 3. ASIC Identification

**Linux**: Checks `pdev->device` and `pdev->subsystem_device` to identify Juniper/Madison.

**Windows**: Read from PCI config space in `DxgkDdiStartDevice` or `DxgkDdiQueryAdapterInfo`.

**KMD Action**: Store VendorId, DeviceId, SubsystemVendorId, SubsystemDeviceId in adapter context.

### 4. Register Access

**Linux**: `WREG32(reg, val)` / `RREG32(reg)` — 32-bit MMIO access

**Windows**: `WRITE_REGISTER_ULONG` / `READ_REGISTER_ULONG`

**KMD Action**: Use macros defined in `registers.h`:
```c
#define REG32(_adapter, _reg) READ_REGISTER_ULONG((PULONG)((_adapter)->RegisterBaseVirtual + (_reg)))
#define WR_REG32(_adapter, _reg, _value) WRITE_REGISTER_ULONG((PULONG)((_adapter)->RegisterBaseVirtual + (_reg)), (_value))
```

### 5. GRBM_STATUS Read

**Linux**: `RREG32(GRBM_STATUS)` — used to check GPU busy/idle state

**Windows**: Same register read via `REG32()` macro.

**KMD Action**: In Phase 2A, read and log via debug output. In later phases, use for reset/hang detection.

### 6. GART Initialization

**Linux**: `evergreen_pcie_gart_enable()` — programs VM_CONTEXT0 registers, allocates GART table, enables TLB.

**Windows**: In `DxgkDdiStartDevice` or later, allocate GART memory and program:
- `VM_CONTEXT0_PAGE_TABLE_START_ADDR`
- `VM_CONTEXT0_PAGE_TABLE_END_ADDR`
- `VM_CONTEXT0_PAGE_TABLE_BASE_ADDR`
- `VM_CONTEXT0_CNTL` (with `ENABLE_CONTEXT | PAGE_TABLE_DEPTH(0)`)

**KMD Action**: Phase 2B+ will implement this.

### 7. CP Initialization

**Linux**: `evergreen_cp_init()` — loads firmware, initializes CP registers, starts CP.

**Windows**: In Phase 2D+:
1. Load PFP/ME firmware (via `DxgkDdiStartDevice` or firmware loading mechanism)
2. Upload firmware to scratch registers or CP memory
3. Program `CP_RB_BASE`, `CP_RB_CNTL`, `CP_RB_RPTR`, `CP_RB_WPTR`
4. Clear `CP_ME_CNTL` halt bits

**KMD Action**: Phase 2D+.

### 8. Ring Initialization

**Linux**: `radeon_ring_init()` — allocates ring buffer, programs `CP_RB_BASE`, `CP_RB_CNTL`.

**Windows**: In Phase 2E+:
1. Allocate ring buffer memory (contiguous, non-paged)
2. Get GPU physical address (via GART or direct VRAM)
3. Program `CP_RB_BASE` with GPU address
4. Program `CP_RB_CNTL` with size and enable
5. Set `CP_RB_RPTR` = 0, `CP_RB_WPTR` = 0

**KMD Action**: Phase 2E+.

### 9. Firmware Loading

**Linux**: `request_firmware(&fw, "radeon/JUNIPER_pfp.bin", ...)` — loads from `/lib/firmware/radeon/`

**Windows**: Use `MmLoadSystemImage` or similar mechanism to load firmware from driver package.

**KMD Action**: Phase 2D will implement firmware loading.

### 10. Interrupt Initialization

**Linux**: `evergreen_irq_set()` — enables IH, programs `IH_BASE`, `IH_RB_BASE`, `IH_CNTL`, etc.

**Windows**: In `DxgkDdiStartDevice` or later, call `DxgkCbEnableInterrupt` with the interrupt vector.

**KMD Action**: Phase 2C will implement interrupt registration and `DxgkDdiInterruptRoutine`.

### 11. Fence Emission

**Linux**: `radeon_fence_emit()` — emits fence packet into ring buffer, returns fence sequence number.

**Windows**: In Phase 2F+:
1. Emit PM4 fence packet into ring buffer
2. Increment fence sequence number
3. Return fence ID to scheduler

**KMD Action**: Phase 2F+.

### 12. GPU Reset

**Linux**: `evergreen_gpu_soft_reset()` — halts CP, asserts GRBM_SOFT_RESET, waits, de-asserts, resumes MC.

**Windows**: Map to `DxgkDdiResetEngine` or `DxgkDdiResetFromTimeout`.

**KMD Action**: Phase 2G+ will implement reset.

## Register Mapping

| Linux Register Name | Windows Offset | Width | Purpose |
|---------------------|----------------|-------|---------|
| GRBM_STATUS | 0x8010 | 32 | GPU status |
| GRBM_SOFT_RESET | 0x8020 | 32 | Soft reset control |
| CP_ME_CNTL | 0x86D8 | 32 | CP ME control |
| CP_RB_BASE | 0xC100 | 32 | Ring buffer base |
| CP_RB_CNTL | 0xC104 | 32 | Ring buffer control |
| CP_RB_RPTR | 0x8700 | 32 | Ring read pointer |
| CP_RB_WPTR | 0xC114 | 32 | Ring write pointer |
| CP_INT_CNTL | 0xC124 | 32 | CP interrupt control |
| CP_INT_STATUS | 0xC128 | 32 | CP interrupt status |
| SRBM_STATUS | 0xE50 | 32 | SRBM status |
| SRBM_SOFT_RESET | 0x0E60 | 32 | SRBM soft reset |
| IH_CNTL | 0x3E18 | 32 | IH control |
| IH_RB_BASE | 0x3E00 | 32 | IH ring base |
| IH_RB_CNTL | 0x3E04 | 32 | IH ring control |
| IH_RB_RPTR | 0x3E08 | 32 | IH ring read pointer |
| IH_RB_WPTR | 0x3E0C | 32 | IH ring write pointer |
| MC_FB_LOCATION | 0x2024 | 32 | Framebuffer location |
| VM_CONTEXT0_PAGE_TABLE_START_ADDR | 0x155C | 32 | VM context 0 start |
| VM_CONTEXT0_PAGE_TABLE_END_ADDR | 0x157C | 32 | VM context 0 end |
| VM_CONTEXT0_PAGE_TABLE_BASE_ADDR | 0x153C | 32 | VM context 0 base |
| VM_CONTEXT0_CNTL | 0x1410 | 32 | VM context 0 control |
| VM_CONTEXT0_PROTECTION_FAULT_DEFAULT_ADDR | 0x1518 | 32 | Protection fault default addr |
| MC_VM_MX_L1_TLB_CNTL | 0x2064 | 32 | L1 TLB control |
| MC_VM_MD_L1_TLB0_CNTL | 0x2654 | 32 | MD L1 TLB 0 control |
| MC_VM_MD_L1_TLB1_CNTL | 0x2658 | 32 | MD L1 TLB 1 control |
| MC_VM_MD_L1_TLB2_CNTL | 0x265C | 32 | MD L1 TLB 2 control |
| MC_VM_MD_L1_TLB3_CNTL | 0x2698 | 32 | MD L1 TLB 3 control (Juniper) |
| SCRATCH_REG0 | 0x8500 | 32 | Scratch register 0 |
| SCRATCH_REG1 | 0x8504 | 32 | Scratch register 1 |
| SCRATCH_REG2 | 0x8508 | 32 | Scratch register 2 |
| SCRATCH_REG3 | 0x850C | 32 | Scratch register 3 |
| SCRATCH_REG4 | 0x8510 | 32 | Scratch register 4 |
| SCRATCH_REG5 | 0x8514 | 32 | Scratch register 5 |
| SCRATCH_REG6 | 0x8518 | 32 | Scratch register 6 |
| SCRATCH_REG7 | 0x851C | 32 | Scratch register 7 |

## Known Differences: Linux vs Windows

1. **Memory allocation**: Linux uses `dma_alloc_coherent` / `kmalloc`. Windows uses `MmAllocateContiguousMemory` / `ExAllocatePool2`.

2. **Firmware loading**: Linux uses `request_firmware()`. Windows uses `MmLoadSystemImage` or embedded resources.

3. **Interrupt registration**: Linux uses `request_irq()`. Windows uses `DxgkCbEnableInterrupt`.

4. **Register access**: Linux uses `WREG32`/`RREG32` macros. Windows uses `WRITE_REGISTER_ULONG`/`READ_REGISTER_ULONG`.

5. **Synchronization**: Linux uses mutexes/spinlocks. Windows uses `KeAcquireSpinLock` / `ExAcquireResourceExclusiveLite`.

6. **Ring buffer**: Linux uses `radeon_ring_lock`/`radeon_ring_unlock`. Windows will use similar lock-free or spinlock-protected ring management.
