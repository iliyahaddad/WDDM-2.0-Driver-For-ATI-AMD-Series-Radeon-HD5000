# Phase 2B — WDK / memory / GART / CP ring / fence / packaging

## Implemented

- Real Visual Studio/WDK x64 project and solution.
- Display-class INF targeting PCI `VEN_1002&DEV_68C0`.
- WDDM 2.0 physical-addressing architecture retained.
- Memory-segment query path using `DXGKQAITYPE_QUERYSEGMENT3`.
- GART table allocation and page-entry writer.
- Evergreen VM context/TLB flush register path.
- Contiguous CP ring allocation and CP register programming preparation.
- Fence writeback page and `DxgkDdiQueryCurrentFence` implementation.
- Interrupt fence read/notification path.
- Hardware execution remains compile-time disabled (`MADISON_ENABLE_HW_EXECUTION=0`).

## Why execution is disabled

The CP ring and GART must not be enabled on real HD 5730 hardware until the exact VRAM/GART aperture discovered from the adapter/ATOM configuration is validated and the required PFP/ME firmware is supplied. The Linux Evergreen driver confirms that startup requires firmware, PCIe GART setup, GPU initialization, writeback/fence setup, and interrupt initialization before the ring is started.

This package therefore provides the implementation boundary without pretending that a guessed aperture or missing firmware is safe.

## UMD

A production UMD is not generated as a fake DLL. Microsoft documents that WDDM graphics requires a paired KMD and UMD, and the UMD must implement the Direct3D UMD DDI contract. The `umd/` directory documents that boundary for the next phase.

## Windows build

Use Visual Studio 2022 + WDK 11. Open `build/MadisonKmd.sln` and build x64. Then use the WDK `Inf2Cat` and `SignTool` tools to generate/sign the catalog. Do not install an unsigned or test-signed driver on a production machine.

## Important WDDM constraint

The aperture segment path is only advertised when the OS supplies a non-zero aperture. The KMD implements `MAP_APERTURE_SEGMENT`/`UNMAP_APERTURE_SEGMENT` by programming its GART table and flushing the Evergreen VM context. Hardware execution remains disabled by default because the CP ring requires validated firmware and a validated GPU-visible address for the ring/writeback memory.
