# Juniper firmware inputs

Phase 3 expects the redistributable Radeon firmware files below in this directory and in the installed `System32\drivers\MadisonFirmware` directory:

| File | Exact size | Linux-firmware role |
|---|---:|---|
| `JUNIPER_pfp.bin` | 4480 bytes | PFP microcode |
| `JUNIPER_me.bin` | 5504 bytes | ME/PM4 microcode |
| `JUNIPER_rlc.bin` | 3072 bytes | RLC microcode |

The Linux radeon source defines the Evergreen microcode sizes as 1120, 1376 and 768 DWORDs respectively. The public linux-firmware tree lists these exact Juniper files. The driver verifies the exact byte size before uploading PFP/ME.

Do not substitute Cypress/Redwood/Turks firmware. For licensing/provenance, obtain the files from a redistributable linux-firmware package and retain its license/WHENCE metadata.
