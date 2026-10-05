# Known Limitations — Madison WDDM 2.0 Physical-Mode Prototype

This document describes the **current Phase 3 state**. It intentionally supersedes the older Phase 2A limitation wording; historical Phase 2 limitations are retained in the phase reports rather than being presented as the current implementation state.

## Validation status

The repository is a research/prototype implementation. The following evidence levels must be kept separate:

1. source implementation,
2. successful real-WDK build,
3. signed package installation,
4. execution on a real HD 5730,
5. repeatable stability,
6. certification/WHLK validation.

The current repository does not claim completion of levels 2–6 unless a future revision explicitly records that evidence.

## Current implementation boundaries

### Hardware bring-up

Phase 3 contains hardware-facing MMIO, firmware, CP/ring, GART and fence code. The CP path is gated by `MADISON_ENABLE_PHASE3_CP=0` and is disabled by default.

No claim is made that the complete Evergreen initialization sequence is reproduced.

### Firmware

- PFP, ME and RLC firmware acquisition/validation is implemented for the documented Juniper path.
- The RLC image is acquired, size-checked and retained/reported.
- RLC startup/state-save/restore handling is **not** claimed as complete.
- Firmware redistribution remains subject to the licensing/source terms of the firmware provider; see `firmware/README.md`.

### Command processor / ring

- The Phase 3 CP path is controlled and experimental.
- Ring programming and a minimal CP self-test are implemented in source.
- A successful source implementation is not evidence that the sequence will execute safely on every Madison board or BIOS configuration.
- GPU hangs/TDRs remain possible during hardware experimentation.

### Fence

- An Evergreen-style fence writeback path and self-test are present.
- The fence page is intended to be represented through the configured GART path rather than treating a CPU physical address as a GPU address.
- Hardware confirmation is still required.

### Memory management / physical addressing

The design intentionally targets physical-addressing mode rather than modern GPU virtual addressing.

Known architectural constraints include:

- no modern per-process GPUVA,
- no `GpuMmu` path,
- no `IoMmu` path,
- physical-addressing allocation/patch-list requirements,
- limitations associated with legacy Evergreen memory-management capabilities.

### Preemption / reset

- Hardware graphics preemption is not claimed.
- `PreemptionAware=0` is part of the selected architecture.
- Long-running or wedged workloads may result in TDR/reset behavior.
- Reset/recovery paths remain experimental and require hardware validation.

### Display

The repository contains WDDM display/VidPN DDI scaffolding, but this should not be interpreted as a complete hardware modeset/scanout implementation.

The project does **not** claim a working physical display pipeline on the HD 5730.

### User-mode driver

A complete production Direct3D UMD is not included. The `umd/` directory documents this boundary.

Therefore, the presence of KMD DDI structures must not be interpreted as a complete Windows graphics stack.

### Certification

- No WHLK/HLK certification is claimed.
- Physical-mode WDDM 2.0 behavior on current Windows releases remains an empirical question.
- Compatibility and certification feasibility must be established with real Microsoft WDK/HLK testing rather than inferred from source alone.

## Hardware scope

The primary research target is:

- ATI/AMD Mobility Radeon HD 5730
- PCI `1002:68C0`
- Madison / Juniper / Evergreen
- TeraScale 2 / VLIW5

Support for other Radeon HD 5000 models is not automatically implied.

## Open validation questions

1. Will a correctly built and signed physical-mode WDDM 2.0 KMD load on the intended Windows 11 configuration?
2. Which WDDM/OS validation requirements will prevent or permit this legacy architecture?
3. Can the controlled PFP/ME CP path execute reliably on the target HD 5730?
4. What additional Evergreen initialization is required for stable RLC operation?
5. Can a usable display scanout path be established without claiming unsupported hardware features?
6. What additional KMD/UMD work is required for meaningful Direct3D functionality?
7. What, if anything, can be validated through WHLK/HLK?

## Risk areas

- Kernel crashes and system hangs during bring-up.
- GPU TDR/reset loops.
- Firmware/version mismatches.
- BIOS-specific behavior.
- Incorrect MMIO/register programming.
- Incomplete memory/paging integration.
- Missing UMD functionality.
- Windows version/WDK compatibility.
- Certification constraints.

These are expected research risks, not claims of resolved functionality.
