# Bring-Up Checklist — Madison WDDM 2.0 Physical-Mode Prototype

## Phase 2A: KMD Skeleton

### Pre-Flight Checks

- [ ] Windows 11 x64 installed on test system
- [ ] Windows 11 WDK installed (matching OS build)
- [ ] Visual Studio 2022 with C++ kernel-mode tools installed
- [ ] Test system has ATI/AMD Mobility Radeon HD 5730 (Madison, PCI 1002:68C0)
- [ ] Legacy AMD driver uninstalled (use Microsoft Basic Display Driver)
- [ ] Test signing certificate available
- [ ] WinDbg installed and configured for kernel debugging (optional but recommended)

### Build Checks

- [ ] `MadisonWddm.sln` opens without errors
- [ ] Solution builds successfully in x64 Debug
- [ ] `MadisonWddm.sys` is produced in `x64\Debug\`
- [ ] No compiler warnings related to missing includes or type mismatches
- [ ] No linker errors

### Deployment Checks

- [ ] `MadisonWddm.sys` copied to `C:\Windows\System32\drivers\`
- [ ] INF file created and signed (to be created in later phase)
- [ ] Driver installed without errors
- [ ] No error 43 in Device Manager
- [ ] Driver appears under "Display adapters" in Device Manager

### Runtime Checks

#### 1. Driver Load
- [ ] System boots without BSOD
- [ ] System Event Log shows driver load
- [ ] No unexpected reboots

#### 2. DxgkDdiAddDevice
- [ ] Debug output shows "DxgkDdiAddDevice called"
- [ ] Debug output shows "DxgkDdiAddDevice succeeded"
- [ ] No STATUS_INSUFFICIENT_RESOURCES

#### 3. DxgkDdiStartDevice
- [ ] Debug output shows "DxgkDdiStartDevice called"
- [ ] Debug output shows "DxgkDdiStartDevice succeeded"
- [ ] WDDM Version reported as 2
- [ ] Physical Addressing: TRUE
- [ ] PreemptionAware: FALSE
- [ ] Number of engines: 1

#### 4. DxgkDdiQueryAdapterInfo
- [ ] `dxdiag` shows WDDM 2.0 driver
- [ ] VIDSCHCAPS: PreemptionAware=0
- [ ] VIDMMCAPS: VirtualAddressingSupported=0
- [ ] GPUENGINETOPOLOGY: 1 engine, 3D type

#### 5. Device Creation
- [ ] `DxgkDdiCreateDevice` called and succeeds
- [ ] `DxgkDdiCreateContext` called and succeeds

#### 6. Clean Shutdown
- [ ] `DxgkDdiStopDevice` called on shutdown
- [ ] `DxgkDdiRemoveDevice` called on shutdown
- [ ] No memory leaks reported
- [ ] System shuts down cleanly

### Evidence Collection

For each test, collect:
- [ ] Debug output logs
- [ ] Event Viewer logs
- [ ] dxdiag output
- [ ] Device Manager screenshot
- [ ] WinDbg capture (if kernel debugging enabled)

### Success Criteria

Phase 2A is successful if ALL of the following are true:
- [ ] Driver loads without BSOD
- [ ] `DxgkDdiAddDevice` succeeds
- [ ] `DxgkDdiStartDevice` succeeds
- [ ] `DxgkDdiQueryAdapterInfo` returns correct caps
- [ ] Driver unloads cleanly

### Failure Criteria

Phase 2A has failed if ANY of the following occur:
- [ ] BSOD during driver load
- [ ] Error 43 in Device Manager
- [ ] `DxgkDdiAddDevice` returns error
- [ ] `DxgkDdiStartDevice` returns error
- [ ] System becomes unbootable

### Rollback Procedure

If the system becomes unstable:
1. Boot into Safe Mode
2. Use `pnputil` to remove the driver:
   ```
   pnputil /delete-driver MadisonWddm.inf /uninstall
   ```
3. Delete `MadisonWddm.sys` from `C:\Windows\System32\drivers\`
4. Reboot

### Known Issues

1. **No INF file yet**: Phase 2A does not include a production INF. Driver must be loaded via test signing or WinDbg.
2. **No WHLK testing**: Phase 2A is not WHLK-certified.
3. **No GPU execution**: Phase 2A does not initialize the GPU or execute commands.

### Next Phase Gate

Do NOT proceed to Phase 2B until ALL Phase 2A success criteria are met and documented.
