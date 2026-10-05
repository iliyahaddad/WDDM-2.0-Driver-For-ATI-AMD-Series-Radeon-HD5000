# Phase 3 — Juniper firmware, ATOM/VRAM discovery, CP/ring/fence bring-up

## What is now implemented

1. **Juniper firmware acquisition and loader**
   - Reads `JUNIPER_pfp.bin`, `JUNIPER_me.bin`, and `JUNIPER_rlc.bin` from `System32\drivers\MadisonFirmware`.
   - Enforces the exact Evergreen sizes: 4480 / 5504 / 3072 bytes.
   - Converts firmware DWORDs from the big-endian representation used by the Linux radeon loader before writing the CP microcode RAM.
   - Upload sequence follows the Evergreen radeon CP microcode path.

2. **ATOM/VBIOS discovery**
   - Uses `DxgkCbReadDeviceSpace(..., DXGK_WHICHSPACE_ROM, ...)`, the supported display-miniport callback for expansion ROM access.
   - Validates `0x55AA`, reads the ATOM header pointer at `0x48`, and records the ATOM marker/version when present.

3. **VRAM discovery**
   - Reads Evergreen `CONFIG_MEMSIZE` and converts the reported MB value to bytes.
   - No hard-coded 1 GB/2 GB assumption is used.

4. **Juniper/Evergreen CP register corrections**
   - `CP_RB_BASE=0xC100`, `CP_RB_CNTL=0xC104`.
   - `CP_PFP_UCODE_ADDR/DATA=0xC150/0xC154`.
   - `CP_ME_RAM_RADDR/WADDR/DATA=0xC158/0xC15C/0xC160`.
   - `CP_ME_HALT` and `CP_PFP_HALT` use the Evergreen bit positions 28 and 26.
   - Ring control uses the Linux Evergreen `RB_BLKSZ/RB_BUFSZ/RB_RPTR_WR_ENA` model.

5. **CP activation path**
   - Firmware upload -> ring programming -> `ME_INITIALIZE` -> release PFP/ME -> ring test.
   - The CP path is behind `MADISON_ENABLE_PHASE3_CP`.

6. **Ring self-test**
   - Submits a minimal PM4 `PACKET0` write to `SCRATCH_REG0` and polls the register.

7. **Fence self-test**
   - Uses an Evergreen-style `SURFACE_SYNC` + `EVENT_WRITE_EOP` fence writeback sequence.
   - The fence page is mapped into the configured GART aperture instead of incorrectly using a CPU physical address as a GPU address.

## Safety gate

`MADISON_ENABLE_PHASE3_CP` is **0 by default**. This is intentional. The project is not hardware-validated in this environment and a wrong ring/GART/firmware sequence can hang a legacy GPU or cause a Windows TDR.

To test on real hardware:

1. Obtain the three Juniper firmware files from a redistributable linux-firmware package.
2. Put them in `firmware\`.
3. Build the driver with WDK/VS2022.
4. Enable test signing on the isolated test machine.
5. Enable `MADISON_ENABLE_PHASE3_CP=1` using `tools\enable-phase3-hardware.ps1`.
6. Install the signed/test-signed package.
7. Capture WinDbg kernel logs on first start.

## Important limitation

This phase does **not** claim a production-ready graphics driver. A Windows WDK/MSVC build and real HD 5730 execution test could not be performed in the current Linux build environment. The next evidence required is a real WDK build log plus hardware traces from the target HD 5730.

## Explicit boundary

The RLC image is currently acquired, size-checked, retained, and reported, but **not started by the KMD**.
The CP self-test intentionally depends only on the PFP/ME path. RLC activation requires the additional
Evergreen RLC state/save-restore setup and should be implemented as a separate hardware-tested step rather than
inventing register programming.

The Phase-3 CP path is therefore a controlled bring-up implementation, not a claim that the complete Evergreen
ASIC startup sequence has been reproduced.
