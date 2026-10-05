# Review fixes (PHASE3 -> PHASE3-reviewed)

Verification done: every `driver/*.c` was syntax-checked with gcc against *stub* headers
(checks identifiers, macros, arity, types I defined). It was NOT built with the real WDK and was
NOT run on hardware. Items marked VERIFY come from knowledge of Linux `evergreend.h`/`r600d.h`,
not from a source tree available during the review - compare them with your copy.

## Would not compile
- `DriverEntry.c` used `MadisonDdi*` functions that had no prototypes anywhere -> new `include/ddi.h`.
- `Display.c` referenced undefined `pCollectDbgInfo`; removed it and 4 unregistered stubs with guessed signatures.
- `Interrupt.c` used `DXGKARGCB_NOTIFY_INTERRUPT` (not a type; correct: `..._DATA`) and undefined `CP_INT_STATUS__CP_RB_INT_MASK`.
- `Fence.c`/`Firmware.c` used `PACKET3_SURFACE_SYNC`, `PACKET3_EVENT_WRITE_EOP`, `PACKET3_ME_INITIALIZE_DEVICE_ID`, `DATA_SEL_1`, ... that were never defined.
- `hardware.h` redefined `MADISON_GART_SIZE/PAGE_SIZE/RING_SIZE` differently from `driver.h`.
- Duplicate `typedef`s of `MADISON_ADAPTER`/`MADISON_DEVICE`/`MADISON_RING`; logging macros `MADISON_RING/FENCE/IRQ` collided with type names -> renamed `MADISON_LOG_*`.
- `.gitignore` ignored `*.vcxproj`, `*.sln`, `*.inf` (the project could never be committed).
- `Adapter.c` passed `PUCHAR*` where `PVOID*` is required (`DxgkCbMapMemory`).

## Wrong behaviour on real hardware
- PM4 encoding: PACKET3 opcode was placed in bits 7:0 (must be 15:8); PACKET0 register was not `>>2`
  (the ring self-test wrote a wrong register). Wrong opcodes fixed: NOP 0x10, ME_INITIALIZE 0x44 (VERIFY).
- `CpStart`: no `RB_NO_UPDATE` and `CP_RB_RPTR_ADDR` left 0 -> CP would write the read pointer to bus address 0;
  `RB_RPTR_WR_ENA` was never cleared; `ME_INITIALIZE` max-context was 0x7F (Juniper: 7).
- MMIO mapping picked the FIRST memory BAR (VRAM aperture on Evergreen) instead of the register BAR.
- GART PTE read/write bits were 3/4 (must be 5/6); unused PTEs were 0 instead of the dummy page; flush
  polled the wrong response bits and skipped the HDP flush; L2 cache was never enabled (VERIFY).
- Register offsets that contradicted Linux: VM_CONTEXT0_* / VM_CONTEXT0_REQUEST_RESPONSE, MC_VM_MX/MB L1 TLB,
  CP_INT_*, SCRATCH_UMSK/ADDR, IH_* (VERIFY all). L1-TLB/VM-context bit fields rewritten (VERIFY).
- ATOM parse looked for the "ATOM" signature at the header start; it is at offset +4. VRAM size field
  was a `ULONG` byte count (overflows >=4 GiB) -> now MB.
- Teardown order: fence page was freed while the CP was still running; GART table was freed without
  disabling VM context 0. Ring init leaked on GART-map failure.
- `MadisonRingSubmit`: `fenceWords[8]` but the emitter writes 11 DWORDs (stack overflow); free-space check
  and RPTR read were done outside the lock.
- `Render`/`Present` cast the *context* handle to `MADISON_DEVICE`; `DescribeAllocation` takes the adapter handle.
- Patch: the 4-byte write is now required to fit inside the submission range. `ResetEngine` had a wrong signature.
- Interrupt path polled invented IH_STATUS/CP_INT bits at DIRQL; now it only claims an interrupt when the
  fence value advanced, and calls `DxgkCbNotifyDpc` from the DPC.
- `DxgkDdiQuerySegment` no longer reports an empty aperture segment.
- Default debug level `ALL` -> error/warn/info.

## Packaging / scripts / docs
- `package-test.ps1` looked in the wrong output directory and ignored firmware; `enable-phase3-hardware.ps1`
  now fails if the define is not found; README/`hardware-mapping.md` corrected; manifest regenerated.

## Still NOT done (cannot be fixed by review - needs WDK + hardware)
- Dxgkrnl will reject the DDI table: mandatory DDIs are missing (VidPN set, QueryChildRelations/Status,
  QueryDeviceDescriptor, SetPowerState, ResetDevice, SetVidPnSourceAddress, ...).
- The GART base is deliberately derived from `DXGK_DEVICE_INFO.AgpApertureBase`; Microsoft documents this as the
  physical base of the AGP aperture, and WDDM ignores BaseAddress for an AGP aperture segment. The driver now
  documents this distinction explicitly; it must still be confirmed against the target machine's translated aperture.
- RLC firmware is loaded and size-checked but never uploaded; IH ring and real interrupts are not implemented.
- GART/L2/TLB registers are programmed at start even with `MADISON_ENABLE_PHASE3_CP=0`.
- `DXGK_*` member names (e.g. `VidMmCaps`, `DXGK_SEGMENTDESCRIPTOR3`) were not checked against the WDK headers.

## Phase 3 review pass (2026-10-02)

- CP activation now uses a single gate: `MADISON_ENABLE_PHASE3_CP`; `MADISON_ENABLE_HW_EXECUTION`
  derives from it, so enabling Phase 3 cannot accidentally leave `DxgkDdiSubmitCommand` disabled.
- CP bring-up now performs the same Evergreen CP/graphics soft-reset sequence used by Linux before ring programming.
- Evergreen IH register offsets were corrected (`IH_RB_CNTL=0x3E00`, `IH_RB_BASE=0x3E04`).
- Ring and fence self-tests are serialized with the ring spin lock.
- CP/Ring/GART constants that were previously marked VERIFY were cross-checked against Linux Evergreen headers.
- The project still intentionally does not claim a complete production WDDM adapter: the full mandatory
  display/VidPN DDI surface and a real UMD remain outside this phase.
- No Windows WDK/MSVC build or physical HD 5730 execution test was available during this review.

## Final pass
- `MADISON_ENABLE_HW_EXECUTION` was still an independent `0` in driver.h although the notes above say it
  derives from `MADISON_ENABLE_PHASE3_CP`; now it really does.
- `docs/wddm-ddi-matrix.md` rewritten to match DriverEntry and list the missing DDI groups.
- Open notes: self-tests hold the ring spin lock during a busy-wait of up to 0.5 s, and the GRBM soft reset
  uses `KeStallExecutionProcessor(15000)`; prefer `KeDelayExecutionThread` in the PASSIVE-level start path.

## Display / VidPN work (adapted from Microsoft KMDOD)
Source consulted: Windows-driver-samples/video/KMDOD (bdd.cxx, bdd_dmm.cxx) read via the GitHub web pages;
bdd_ddi.cxx and the raw files were not readable, so DDI signatures in `include/ddi.h` for the wrapper layer are
from the DDI documentation/memory, not copied.
- New: `driver/Vidpn.c` (IsSupportedVidPn, Recommend*, EnumVidPnCofuncModality, CommitVidPn,
  UpdateActiveVidPnPresentPath, SetVidPnSourceAddress/Visibility, QueryVidPnHWCapability).
- New in `driver/Display.c`: QueryChildRelations/Status, QueryDeviceDescriptor, SetPowerState, NotifyAcpiEvent,
  DispatchIoRequest, ResetDevice, QueryInterface, ControlEtwLogging, Reset/RestartFromTimeout,
  StopDeviceAndReleasePostDisplayOwnership, pointer/palette/escape/scanline/interrupt-control stubs.
- `MadisonDisplayStart` takes the POST framebuffer description (`DxgkCbAcquirePostDisplayOwnership`); unlike KMDOD
  it falls back to a 1024x768 A8R8G8B8 mode instead of failing to start.
- Deliberate differences from KMDOD: only the POST mode, identity scaling/rotation (no software blit), no framebuffer mapping.
  The sample's rotation pivot test uses `!=` against its own comment; `==` is used.
- `tests/host/`: gcc tests with a mock VidPN that count acquire/release (paths, mode sets, mode infos) and
  check success, pinned, empty-topology, two-path and rejected-commit flows. They pass; they prove internal
  consistency only.
- Known gap: `SetVidPnSourceAddress` accepts and records but does not program scanout.

## Phase 3 r6 final audit (2026-10-04)

- `DXGK_QUERYSEGMENTOUT3` now consumes the authoritative `DXGK_QUERYSEGMENTIN` supplied by Dxgkrnl for both the count and detail calls; it no longer substitutes adapter-start bookkeeping for the OS-provided aperture.
- AGP aperture segment flags were corrected to set **only** `DXGK_SEGMENTFLAGS.Agp`. Microsoft explicitly rejects combining `Agp` with `Aperture`, `CpuVisible`, `CacheCoherent`, or other segment flags.
- GART no longer silently truncates an OS-provided aperture larger than the implemented table. It returns `STATUS_NOT_SUPPORTED` instead of advertising a larger range than it can map.
- `DxgkDdiStopDevice` now tears down GART/VM state and memory bookkeeping, preventing a Start/Stop/Start cycle from leaking or reusing stale GART state.
- `StartDevice` failure paths now roll back MMIO/GART/Ring/Fence/Firmware ownership instead of relying on a later StopDevice callback.
- `MadisonRingSubmit` now propagates a fence-emission failure and does not publish a command stream without a completion fence.
- No claim of real WDK/MSVC compilation or physical HD 5730 execution has been added; those remain hardware/Windows-host validation steps.

## r6 follow-up: host-test stubs repaired
- r6 made `Memory.c`/`Adapter.c` use `DXGK_QUERYSEGMENTIN` and `DXGKARG_QUERYADAPTERINFO::pInputData`, but `tests/host/wdk_stub.h`
  did not define them, so every host test and the gcc syntax check failed at `include/memory.h:17`.
- `wdk_stub.h` now defines `DXGK_SEGMENTFLAGS`, `DXGK_QUERYSEGMENTIN` (AgpApertureBase / AgpApertureSize as LARGE_INTEGER / AgpFlags,
  per the Microsoft DDI reference), full `DXGK_SEGMENTDESCRIPTOR3` / `DXGK_QUERYSEGMENTOUT3`, and `pInputData`/`InputDataSize` in
  `DXGKARG_QUERYADAPTERINFO`. No driver source changed.
- New `tests/host/memory_test.c` for `MadisonMemoryQuerySegments`.
- Result: vidpn_test, display_test, memory_test pass; every `driver/*.c` passes `gcc -fsyntax-only` against the stubs.
  Still NOT built with the real WDK and NOT run on hardware.
