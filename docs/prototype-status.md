# Prototype Status

## Current revision

This repository is a **Phase 3 research prototype** of a Windows WDDM 2.0 kernel-mode display miniport targeting the ATI/AMD Mobility Radeon HD 5730 (`PCI 1002:68C0`, Madison/Juniper/Evergreen).

The project has progressed beyond the original KMD skeleton and now contains hardware-facing research components for firmware discovery/loading, ATOM/VBIOS discovery, VRAM discovery, Evergreen CP/ring handling, GART-backed addressing and fence bring-up.

**It remains experimental and is not a production-ready or certified graphics driver.**

## What has been corrected / implemented

### WDDM/KMD foundation

- Correct `DxgkDdiAddDevice` callback signature and adapter-context ownership.
- Uses `DxgkCbGetDeviceInformation` and PnP-translated resources during `DxgkDdiStartDevice`.
- Maps assigned MMIO resources with `DxgkCbMapMemory` and releases them with `DxgkCbUnmapMemory`.
- Removes invalid `extern "C"` wrappers from C sources.
- Removes unsafe `RtlZeroMemory(DRIVER_OBJECT)` behavior.
- Corrects `DxgkDdiGetNodeMetadata` to the WDK signature and returns 3D-engine metadata.
- Corrects GPU topology representation to `NbAsymetricProcessingNodes`.
- Corrects `DXGK_POINTERFLAGS` handling by leaving unsupported pointer capabilities zeroed.
- Corrects patch-list processing so `PatchOffset` and `AllocationOffset` are treated as byte offsets and only the submission range is processed.
- Rejects physical addresses above the current 32-bit PM4 prototype boundary rather than silently truncating them.
- Avoids fake success from unimplemented paging/allocation/preemption paths.
- Corrects interrupt completion notification handling.
- Corrects NULL-check ordering in QueryAdapterInfo and CollectDbgInfo.

### Phase 3 hardware-facing work

- Juniper PFP/ME/RLC firmware acquisition and size validation.
- Evergreen firmware byte-order conversion and CP microcode upload path.
- ATOM/VBIOS ROM discovery through the display-miniport ROM callback.
- Evergreen `CONFIG_MEMSIZE` VRAM discovery.
- Evergreen CP/ring register corrections.
- Controlled CP activation behind `MADISON_ENABLE_PHASE3_CP`.
- Minimal CP ring self-test.
- Evergreen-style fence writeback/self-test using the configured GART path.

## What is not yet proven

The following are deliberately **not** claimed as complete or validated:

- real WDK/MSVC build success for the current revision,
- signed installation on Windows 11,
- successful first-start on a real HD 5730,
- stable PFP/ME execution on physical hardware,
- complete RLC startup/state handling,
- complete GPU reset/recovery behavior,
- complete memory residency/paging implementation,
- complete hardware display modeset/scanout,
- complete Direct3D UMD,
- WHLK/HLK certification,
- production stability or performance.

## Safety gate

The CP hardware path is disabled by default:

```c
#define MADISON_ENABLE_PHASE3_CP 0
```

This is intentional. Any real-hardware bring-up should follow `PHASE-3-IMPLEMENTATION.md` and `docs/phase3/bringup-order.md` on an isolated test machine with test signing and kernel debugging.

## Historical phase reports

The repository retains the Phase 2 and Phase 2B documents as historical implementation records. They should be read in chronological context rather than as a description of every capability in the current tree.

- Phase 2A: KMD skeleton / DDI foundation
- Phase 2B: corrected prototype integration and safety fixes
- Phase 3: hardware-facing firmware/CP/ring/fence bring-up foundations

## Evidence standard

Future status updates should explicitly distinguish:

`implemented in source` → `built with real WDK` → `installed` → `executed on HD 5730` → `repeatably stable` → `HLK/WHLK validated`.

This evidence chain is part of the project's engineering discipline and prevents source-level implementation from being mistaken for hardware validation.
