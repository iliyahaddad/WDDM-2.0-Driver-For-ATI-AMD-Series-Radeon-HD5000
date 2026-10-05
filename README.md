# Madison WDDM 2.0 Physical-Mode Prototype

> **Research / experimental project — not a production driver.**
>
> This repository is an experimental Windows WDDM 2.0 kernel-mode display miniport (KMD) research implementation targeting the **ATI/AMD Mobility Radeon HD 5730 (Madison / Juniper / Evergreen, PCI 1002:68C0)**.
>
> **Hardware validation has not been completed.** The project must be treated as kernel-level experimental software. Do not install or test it on a machine that contains data or workloads you cannot afford to lose. Hardware bring-up should be performed only on an isolated test system with Windows test-signing and a kernel debugger configured.

## Project goal

The goal is to investigate how far a legacy Evergreen/TeraScale 2 GPU can be brought toward a **WDDM 2.0 physical-addressing KMD architecture** on modern Windows, without claiming unsupported capabilities such as modern per-process GPU virtual addressing or hardware graphics preemption.

The current architecture intentionally targets:

- WDDM 2.0 physical-addressing mode
- `PreemptionAware = 0`
- No `GpuMmu`
- No `IoMmu`
- No modern per-process GPU virtual addressing
- No hardware graphics preemption

This is a research path, not a claim that Windows 11 will accept, certify, or fully operate such a driver on real hardware.

## Target hardware

| Item | Target |
|---|---|
| GPU | ATI/AMD Mobility Radeon HD 5730 |
| Codename | Madison / Juniper |
| Architecture | Evergreen / TeraScale 2 |
| PCI Vendor | `0x1002` |
| PCI Device | `0x68C0` |
| Driver model under investigation | WDDM 2.0 physical addressing |
| Modern GPUVA | Not used |
| Hardware preemption | Not available / not claimed |

The repository is intentionally scoped around the Madison/Juniper/Evergreen path. Broader HD 5000-family compatibility is not implied by this project.

---

## Development phases

The repository is organized as a staged bring-up rather than as a claim of a finished graphics stack.

### Phase 1 — Research and feasibility

Established the hardware/software feasibility boundary for the HD 5730 and identified the architectural constraints that matter for a WDDM 2.0 physical-mode design.

Key conclusions carried into implementation:

- Madison is an Evergreen / TeraScale 2 GPU.
- The design avoids relying on modern GPUVM/GPUVA features that the hardware does not provide.
- Physical-addressing mode is the selected experimental architecture.
- Linux `radeon` is used as a hardware-behavior reference where appropriate.
- Hardware capability, Windows WDDM requirements, firmware availability, and certification feasibility remain separate questions.

### Phase 2A — KMD skeleton

Built the initial Windows kernel-mode display miniport structure and WDDM DDI surface.

This phase established the driver entry point, adapter/device lifecycle, resource handling, interrupt/reset/display/DDI scaffolding, memory/allocation interfaces, and the initial test/bring-up documentation.

### Phase 2B — Corrected prototype integration

Corrected important WDDM/WDK-facing details and removed unsafe or misleading prototype behavior.

Examples include:

- Correct adapter/device ownership and callback signatures.
- Correct PnP-translated resource handling.
- MMIO mapping through the appropriate Dxgk callback path.
- Correct DDI signatures and topology representation.
- Correct patch-list byte-offset handling.
- Rejection of unsupported physical addresses above the current 32-bit PM4 prototype boundary.
- Avoidance of fake success for unimplemented paging/allocation/preemption paths.
- Correct interrupt completion handling and NULL-check ordering.

See `PHASE-2B-IMPLEMENTATION.md`, `PHASE-2-PROTOTYPE-REPORT.md`, and `FIXES.md`.

### Phase 3 — Hardware bring-up foundations

Phase 3 adds the first controlled Evergreen hardware-facing building blocks:

- Juniper PFP/ME/RLC firmware acquisition and validation.
- ATOM/VBIOS ROM discovery.
- Evergreen VRAM-size discovery.
- Corrected Evergreen CP/ring register definitions and programming.
- Controlled PFP/ME CP activation path.
- CP ring self-test.
- Evergreen-style fence writeback/self-test.
- GART-backed fence addressing.

The Phase 3 hardware path is **disabled by default** through `MADISON_ENABLE_PHASE3_CP=0`.

See `PHASE-3-IMPLEMENTATION.md` and `docs/phase3/`.

### Current phase status

**Phase 3 source implementation / controlled bring-up — experimental.**

The repository contains a substantially more complete research KMD source tree than the earlier skeleton, but it is **not** a production-ready Windows graphics driver and is **not hardware-validated**.

In particular, this repository does not claim:

- successful installation on Windows 11,
- successful execution on an HD 5730,
- a working display scanout/modeset path,
- a complete user-mode graphics driver (UMD),
- WHLK/HLK certification,
- production stability,
- complete Evergreen ASIC initialization,
- complete RLC startup/state handling, or
- compatibility with the entire Radeon HD 5000 family.

---

## Repository structure

```text
Madison WDDM 2.0 Prototype/
├── driver/                  # Kernel-mode driver implementation
├── include/                 # Driver headers and hardware definitions
├── build/                   # Visual Studio solution/project
├── package/                 # Driver INF/package definition
├── firmware/                # Firmware documentation / acquisition notes
├── tools/                   # Controlled bring-up/helper scripts
├── scripts/                 # Build and package-test helpers
├── tests/                   # Host-side tests and bring-up checklist
├── umd/                     # UMD status/documentation boundary
├── docs/                    # Architecture, hardware and validation docs
│   └── phase3/              # Phase 3 source audit and bring-up documentation
├── PHASE-2-PROTOTYPE-REPORT.md
├── PHASE-2B-IMPLEMENTATION.md
├── PHASE-3-IMPLEMENTATION.md
├── FIXES.md
├── THIRD-PARTY-NOTICES.md
├── MANIFEST-SHA256.txt
└── LICENSE
```

The existing source tree, file names, build files, scripts, tests, and documentation layout are intentionally preserved.

## Build

### Prerequisites

- Windows 11 x64 test/development machine
- Visual Studio 2022
- Windows Driver Kit (WDK) matching the intended Windows build
- C++ kernel-mode driver development components
- An isolated test machine for any driver installation
- WinDbg/kernel debugging for hardware bring-up

### Build steps

1. Open `build\MadisonKmd.sln` in Visual Studio 2022.
2. Select `x64` and the desired `Debug` or `Release` configuration.
3. Build the solution.
4. Alternatively, review/use `scripts\build.ps1` for the repository's scripted build path.
5. Review the resulting build/package output before any installation attempt.

A successful source build is **not** equivalent to successful hardware validation.

## Hardware bring-up safety

Phase 3 contains hardware-facing code. The CP bring-up gate is deliberately disabled by default:

```c
#define MADISON_ENABLE_PHASE3_CP 0
```

Do not enable it on production systems. If real hardware testing is undertaken, use the documented sequence in `PHASE-3-IMPLEMENTATION.md` and `docs/phase3/bringup-order.md`, obtain the required Juniper firmware from an appropriate redistributable source, use test signing on an isolated machine, and capture kernel debugger output.

The RLC image is currently acquired/validated/retained but is **not claimed to be started by the KMD**. This boundary is intentional until the required Evergreen RLC initialization/state handling is independently verified.

## Documentation map

- `PHASE-2-PROTOTYPE-REPORT.md` — Phase 2 prototype report
- `PHASE-2B-IMPLEMENTATION.md` — Phase 2B implementation notes
- `PHASE-3-IMPLEMENTATION.md` — Phase 3 hardware-facing implementation and safety boundary
- `FIXES.md` — corrective changes made during review
- `docs/architecture.md` — driver architecture
- `docs/hardware-mapping.md` — hardware/register mapping reference
- `docs/wddm-ddi-matrix.md` — WDDM DDI coverage
- `docs/bringup.md` — bring-up procedure
- `docs/known-limitations.md` — current known limitations
- `docs/prototype-status.md` — current prototype status
- `docs/phase3/source-audit.md` — Phase 3 source audit
- `docs/phase3/bringup-order.md` — controlled bring-up order
- `tests/bringup-checklist.md` — hardware/test checklist
- `firmware/README.md` — firmware handling notes
- `umd/README.md` — user-mode driver boundary/status

## What this project is / is not

### This project is

- A research implementation.
- A learning and reverse-engineering exercise around WDDM/KMD and Evergreen hardware.
- A controlled attempt to reproduce selected hardware initialization and command/fence mechanisms using Windows driver interfaces.
- A repository for documenting what is known, implemented, unimplemented, and still experimentally unknown.

### This project is not

- A certified AMD driver.
- An AMD-provided product.
- A replacement for AMD's historical Windows driver.
- A production-ready Windows graphics stack.
- A guarantee that Windows 11 supports this architecture on the target hardware.
- A guarantee of WHLK/HLK compliance.
- A complete Direct3D user-mode driver.

## Validation status

At the current revision, the strongest evidence is source-level implementation and static/documentary review. A real WDK build, signed installation, and execution trace on an ATI/AMD Mobility Radeon HD 5730 are still required before hardware capability claims can be upgraded.

The project therefore deliberately distinguishes between:

**implemented in source** → **build-validated** → **installed** → **hardware-executed** → **stable** → **certified**.

These are separate milestones and should not be conflated.

## Third-party code

Some display/VidPN/child/power logic is adapted from Microsoft's Windows-driver-samples KMDOD sample. See `THIRD-PARTY-NOTICES.md` for the attribution boundary and licensing notice. Third-party material remains subject to its applicable license.

## License

Original project code is released under the MIT License unless a file or third-party notice states otherwise. See `LICENSE` and `THIRD-PARTY-NOTICES.md`.

## Disclaimer

This software is provided for research and educational purposes. Kernel-mode and GPU hardware experimentation can cause crashes, hangs, data loss, or hardware/firmware recovery problems. Use appropriate backups, an isolated test environment, test signing, and kernel debugging. The authors make no claim of fitness for production use, certification, or compatibility with any particular Windows release or GPU beyond the explicitly documented research target.
