# Phase 3 Source Audit

The hardware-facing portions of Phase 3 were cross-checked against:

1. Microsoft `DXGK_DEVICE_INFO` and `DXGKCB_READ_DEVICE_SPACE` documentation.
2. Microsoft WDDM memory-segment / aperture documentation.
3. Microsoft Windows-driver-samples KMDOD project for WDK project structure and display-driver architecture.
4. Linux Radeon Evergreen/Juniper sources (`evergreend.h`, `r600d.h`, `evergreen.c`, `r600.c`).
5. Linux `linux-firmware` Juniper firmware names and byte sizes.

Important conclusions:

- `DXGK_DEVICE_INFO.AgpApertureBase` is documented by Microsoft as the physical base of the AGP aperture.
- WDDM ignores `BaseAddress` for an AGP-type aperture segment and uses the actual physical aperture address.
- Juniper has a fourth MD L1 TLB register (`MC_VM_MD_L1_TLB3_CNTL = 0x2698`).
- Evergreen CP ring programming follows the Linux `evergreen_cp_resume()` model: reset dependent graphics blocks,
  program ring size, initialize pointers, set `CP_RB_BASE`, then issue `ME_INITIALIZE` and release the CP.
- Evergreen PTE permissions use bits 5 and 6 for readable/writeable, with valid/system/snooped in bits 0/1/2.
- `CONFIG_MEMSIZE` is `0x5428` and reports VRAM size in MB on this family.
- `SCRATCH_REG0` is `0x8500`.
- `EVENT_WRITE_EOP` uses a 4-DWORD payload after the header and writes the fence value through DATA_SEL/INT_SEL.

This is a source audit, not a hardware validation report. A real WDK build and target-HD-5730 WinDbg trace remain mandatory
before enabling CP execution on the target machine.

## Phase 3 r6 Microsoft segment-contract correction

Microsoft's `DXGK_QUERYSEGMENTOUT3` contract requires the driver to use the `DXGK_QUERYSEGMENTIN` supplied in `DXGKARG_QUERYADAPTERINFO` for the AGP aperture information. The driver now consumes `AgpApertureBase`, `AgpApertureSize`, and treats zero-size/zero-base input as no AGP aperture. For an AGP-type aperture segment, Microsoft requires the `DXGK_SEGMENTFLAGS.Agp` bit to be the only segment flag; the implementation now follows that rule rather than setting `Aperture/CpuVisible/CacheCoherent` together. See Microsoft documentation for `DXGK_QUERYSEGMENTIN`, `DXGK_QUERYSEGMENTOUT3`, and `DXGK_SEGMENTFLAGS`.
