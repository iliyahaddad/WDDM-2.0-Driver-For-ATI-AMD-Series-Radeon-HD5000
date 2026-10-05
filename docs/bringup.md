# Bring-Up Procedure — Madison WDDM 2.0 Physical-Mode Prototype

## Phase 2A: KMD Skeleton

### Objective

Prove that the Madison WDDM 2.0 KMD can load, initialize, and unload without crashing.

### Prerequisites

- Windows 11 x64
- Windows 11 WDK (matching OS build)
- Visual Studio 2022 with C++ kernel-mode driver development tools
- Test system with ATI/AMD Mobility Radeon HD 5730 (Madison, PCI 1002:68C0)
- WinDbg (kernel debugging recommended but not required for Phase 2A)
- Test signing certificate (for development)

### Build Instructions

1. Open `MadisonWddm.sln` in Visual Studio 2022
2. Select configuration: `Debug` or `Release`, `x64`
3. Build → Build Solution (Ctrl+Shift+B)
4. Output: `MadisonWddm.sys` in `x64\Debug` or `x64\Release`

### Deployment

1. Copy `MadisonWddm.sys` to `C:\Windows\System32\drivers\`
2. Create INF file (to be created in later phase)
3. Install driver via Device Manager or `pnputil`
4. Reboot if necessary

### Test Procedure

#### Test 1: Driver Load

**Objective**: Verify the driver loads without BSOD.

**Steps**:
1. Install driver on test system
2. Boot Windows
3. Check Device Manager for "Madison WDDM 2.0" under Display adapters
4. Check System Event Log for driver load events

**Expected**:
- No BSOD
- Driver appears in Device Manager
- No error 43
- Event log shows successful driver load

**Actual**: ___________

**PASS/FAIL**: ___________

#### Test 2: DxgkDdiAddDevice

**Objective**: Verify `DxgkDdiAddDevice` is called and succeeds.

**Steps**:
1. Enable WPP tracing or kernel debug logging
2. Boot with driver loaded
3. Check debug output for "DxgkDdiAddDevice called"
4. Check debug output for "DxgkDdiAddDevice succeeded"

**Expected**:
- Debug output shows AddDevice called
- Debug output shows AddDevice succeeded
- No STATUS_INSUFFICIENT_RESOURCES

**Actual**:
```
[DEBUG] DxgkDdiAddDevice called
[DEBUG] DxgkDdiAddDevice succeeded
```

**PASS/FAIL**: ___________

#### Test 3: DxgkDdiStartDevice

**Objective**: Verify `DxgkDdiStartDevice` is called and succeeds.

**Steps**:
1. Enable debug logging
2. Boot with driver loaded
3. Check debug output for "DxgkDdiStartDevice called"
4. Check debug output for "DxgkDdiStartDevice succeeded"

**Expected**:
- Debug output shows StartDevice called
- Debug output shows StartDevice succeeded
- WDDM version reported as 2.0
- Physical addressing enabled
- PreemptionAware=0

**Actual**:
```
[DEBUG] DxgkDdiStartDevice called
[INFO]  WDDM Version: 2
[INFO]  Physical Addressing: TRUE
[INFO]  PreemptionAware: FALSE
[DEBUG] DxgkDdiStartDevice succeeded
```

**PASS/FAIL**: ___________

#### Test 4: DxgkDdiQueryAdapterInfo

**Objective**: Verify `DxgkDdiQueryAdapterInfo` returns correct capabilities.

**Steps**:
1. Enable debug logging
2. Boot with driver loaded
3. Use `dxdiag` to query adapter info
4. Check debug output for query types

**Expected**:
- DRIVERCAPS: WDDMVersion = 2
- VIDSCHCAPS: PreemptionAware = 0, MultiEngineAware = 0
- VIDMMCAPS: VirtualAddressingSupported = 0, GpuMmuSupported = 0
- GPUENGINETOPOLOGY: 1 engine, type 3D

**Actual**:
```
[DEBUG] DxgkDdiQueryAdapterInfo called, Type=...
[INFO]   Returned VIDSCHCAPS: PreemptionAware=0, MultiEngineAware=0
[INFO]   Returned VIDMMCAPS: VirtualAddressingSupported=0, GpuMmuSupported=0
```

**PASS/FAIL**: ___________

#### Test 5: DxgkDdiStopDevice

**Objective**: Verify `DxgkDdiStopDevice` is called on shutdown.

**Steps**:
1. Enable debug logging
2. Boot with driver loaded
3. Shut down Windows
4. Check debug output for "DxgkDdiStopDevice called"

**Expected**:
- Debug output shows StopDevice called
- No errors

**Actual**: ___________

**PASS/FAIL**: ___________

#### Test 6: DxgkDdiRemoveDevice

**Objective**: Verify `DxgkDdiRemoveDevice` is called and frees resources.

**Steps**:
1. Enable debug logging
2. Boot with driver loaded
3. Shut down Windows
4. Check debug output for "DxgkDdiRemoveDevice called"
5. Check debug output for "DxgkDdiRemoveDevice completed"

**Expected**:
- Debug output shows RemoveDevice called
- Debug output shows RemoveDevice completed
- No memory leaks

**Actual**: ___________

**PASS/FAIL**: ___________

### Debug Output

Enable debug logging via registry or WPP:

```reg
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\MadisonWddm\Parameters]
"DebugLevel"=dword:ffffffff
```

Expected debug output during boot:
```
MadisonKMD: DriverEntry - Madison WDDM 2.0 Physical-Mode Prototype
MadisonKMD: Target: ATI/AMD Mobility Radeon HD 5730 (Madison/Juniper/Evergreen)
MadisonKMD: Mode: WDDM 2.0 Physical Addressing, PreemptionAware=0
MadisonKMD: DriverEntry completed successfully
MadisonKMD: DxgkDdiAddDevice called
MadisonKMD: DxgkDdiAddDevice succeeded
MadisonKMD: DxgkDdiStartDevice called
MadisonKMD:   WDDM Version: 2
MadisonKMD:   Physical Addressing: TRUE
MadisonKMD:   PreemptionAware: FALSE
MadisonKMD:   Engines: 1
MadisonKMD: DxgkDdiStartDevice succeeded
```

### Known Limitations

1. No GPU execution yet (Phase 2B+)
2. No MMIO register writes (read-only in Phase 2A)
3. No interrupt handling
4. No firmware loading
5. No CP/ring initialization
6. No DMA buffer submission
7. No physical patching

### Next Steps

After Phase 2A passes:
- Phase 2B: MMIO mapping and safe register reads
- Phase 2C: Interrupt registration and ISR/DPC
- Phase 2D: Firmware loading
- Phase 2E: CP and ring initialization
- Phase 2F: Fence implementation
- Phase 2G: Reset/TDR path
