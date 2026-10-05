# Architecture — Madison WDDM 2.0 Physical-Mode Prototype

## Target Architecture

```
Windows 11
    ↓
Dxgkrnl (Dxgkrnl.sys)
    ↓
VidMm (Video Memory Manager)
    ↓
VidSch (GPU Scheduler)
    ↓
MadisonKMD.sys (Our WDDM 2.0 KMD)
    ↓
Madison Hardware (PCI 1002:68C0)
```

## Design Principles

1. **Physical Addressing Mode**: The GPU engine accesses memory through physical addresses, not GPU virtual addresses. This is explicitly supported by WDDM 2.0.

2. **PreemptionAware = 0**: The driver uses the Windows 7 preemption model. The OS will not issue preemption requests before TDR.

3. **Allocation Lists + Patch Location Lists**: VidMm provides allocation lists with physical addresses. The KMD's `DxgkDdiPatch` inserts these addresses into DMA buffers.

4. **No GpuMmu / No IoMmu**: The driver does not opt into GPU virtual memory management. VidMm manages residency via the device residency requirement list.

5. **Hardware Reference**: Linux Radeon Evergreen driver (`drivers/gpu/drm/radeon/evergreen.c`) is used as a hardware reference, not copied verbatim.

## WDDM Layer Responsibilities

### Dxgkrnl (OS-provided)
- Adapter enumeration and management
- Device creation/destruction
- DMA buffer management
- Paging buffer management
- GPU scheduling (VidSch)
- Memory management (VidMm)
- TDR detection and recovery initiation
- Interrupt registration

### VidMm (OS-provided)
- Segment management
- Allocation creation/destruction
- Residency management (device residency requirement list)
- Paging operations
- Physical address assignment

### VidSch (OS-provided)
- Context scheduling
- DMA buffer submission ordering
- Preemption requests (when PreemptionAware=1)
- TDR timeout detection

### MadisonKMD (Our implementation)
- Hardware detection and identification
- MMIO mapping
- Firmware loading
- CP initialization and ring management
- Command buffer translation to DMA packets
- Physical address patching (`DxgkDdiPatch`)
- DMA buffer submission (`DxgkDdiSubmitCommand`)
- Paging buffer construction (`DxgkDdiBuildPagingBuffer`)
- Interrupt handling (`DxgkDdiInterruptRoutine` / `DxgkDdiDpcRoutine`)
- GPU reset (`DxgkDdiResetEngine`)
- Hardware-specific capabilities reporting

## Addressing Model

### Physical Addressing Mode

```
UMD generates command buffer
    ↓
Command buffer contains placeholders for physical addresses
    ↓
VidMm creates AllocationList with physical addresses
    ↓
VidMm creates PatchLocationList
    ↓
Dxgkrnl calls DxgkDdiPatch
    ↓
KMD inserts physical addresses into DMA buffer
    ↓
Dxgkrnl calls DxgkDdiSubmitCommand
    ↓
KMD submits DMA buffer to CP ring
    ↓
GPU executes with physical addresses
```

### Key Implication

In physical mode:
- Allocations referenced by the rendering engine must be physically contiguous when in VRAM
- Allocations in system memory must be mapped into the aperture segment
- The `AccessedPhysically` flag must be set on such allocations
- Patch Location Lists are required for every DMA buffer submission

## Memory Model

### Segments

1. **Memory Segment (VRAM)**
   - Local GPU memory
   - Contiguous physical addresses
   - Fastest access

2. **Aperture Segment (System Memory)**
   - System memory mapped into GPU address space via GART
   - Slower access than VRAM
   - Used when VRAM is exhausted

3. **Implicit System Memory Segment (SegmentId=0)**
   - System memory pages
   - Used for DMA buffers and paging operations

### GART (Graphics Address Remapping Table)

- Single-level page table (Evergreen does not support multi-level GPUVM)
- Maps system memory pages into GPU address space
- Controlled via `VM_CONTEXT0` registers
- TLB invalidation via `VM_CONTEXT0_REQUEST_RESPONSE`

## Interrupt Model

### WDDM Contract

- `DxgkDdiInterruptRoutine` runs at DIRQL
- Must acknowledge the interrupt before returning
- Must call `DxgkCbNotifyInterrupt` to notify VidSch
- `DxgkDdiDpcRoutine` runs at DISPATCH_LEVEL for deferred work

### Evergreen IH (Interrupt Handler)

- IH ring buffer records interrupt events
- CP completion generates IH interrupt
- Driver reads `IH_STATUS` to determine source
- Driver writes `IH_STATUS` to acknowledge

## Firmware Model

### Required Firmware (Juniper/Madison)

1. **PFP (Pre-Fetch Processor)**: `radeon/JUNIPER_pfp.bin`
   - Controls CP prefetch and decode
   - Loaded at CP initialization

2. **ME (MicroEngine)**: `radeon/JUNIPER_me.bin`
   - Main command processor engine
   - Loaded at CP initialization

3. **RLC (Run-Length Control)**: `radeon/JUNIPER_rlc.bin`
   - Ring-level control (if required)
   - Loaded at CP initialization

### Loading Mechanism

- Linux uses `request_firmware()` from `/lib/firmware/radeon/`
- Windows prototype will use driver-embedded resources or `MmLoadSystemImage`
- Firmware is uploaded to scratch registers or CP memory space

## Preemption Model

### Windows 7 Model (PreemptionAware=0)

- No mid-DMA preemption
- OS may reset GPU on timeout (TDR) without preemption
- Simpler KMD implementation
- Risk of repeated resets on long-running shaders

### Implementation

- `DxgkDdiPreemptCommand` exists in DDI table but may never be called
- Implement as safe stub that notifies scheduler
- No hardware preemption support required

## Reset Model

### Linux Reference: evergreen_gpu_soft_reset()

1. Check `GRBM_STATUS` — if GUI not active, skip reset
2. Halt CP: `CP_ME_CNTL = CP_ME_HALT | CP_PFP_HALT`
3. Wait for MC idle
4. Assert `GRBM_SOFT_RESET` with relevant block bits
5. Wait 50us
6. De-assert `GRBM_SOFT_RESET`
7. Wait 50us
8. Resume MC

### Windows Mapping

- `DxgkDdiResetEngine` — per-engine reset
- `DxgkDdiResetFromTimeout` — full adapter reset from TDR
- Both will use the same underlying hardware reset sequence

## Phase 2A Scope

Phase 2A implements ONLY:
- Driver skeleton (load/unload)
- MMIO mapping
- Safe register reads
- DDI function table
- Capability reporting

Phase 2A does NOT implement:
- Firmware loading
- CP initialization
- Ring buffer
- Command submission
- Interrupts
- Fences
- Patching
- Reset
