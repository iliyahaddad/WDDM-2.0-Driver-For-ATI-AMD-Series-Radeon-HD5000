# WDDM DDI Matrix (after the KMDOD-based display work)

What `driver/DriverEntry.c` registers, and how real each DDI is.
"Hardware" means actual GPU/display programming exists; "policy" means correct negotiation with Dxgkrnl only.

| Group | DDI | Source | State |
|-------|-----|--------|-------|
| Lifetime | AddDevice, StartDevice, StopDevice, RemoveDevice, Unload, QueryAdapterInfo | Adapter.c | implemented |
| Interrupts | InterruptRoutine, DpcRoutine | Interrupt.c | fence-progress only; no IH ring |
| Scheduling / VidMm | Patch, SubmitCommand, BuildPagingBuffer, QueryCurrentFence, PreemptCommand, GetNodeMetadata | Device.c | hardware path gated by `MADISON_ENABLE_PHASE3_CP` |
| Reset | ResetEngine, ResetFromTimeout | Device.c / Display.c | returns STATUS_NOT_SUPPORTED (no HW reset) |
| | RestartFromTimeout, ResetDevice | Display.c | no-op |
| Device / context | Create/Destroy Device, Create/Destroy Context | Device.c | minimal |
| Render / present | Render, Present | Command.c | not supported |
| Allocations | Create/Destroy/Describe/GetStandardAllocationDriverData/Open/Close | Allocation.c | not supported |
| Child / monitor | QueryChildRelations, QueryChildStatus | Display.c (from KMDOD) | policy: 1 child, always "connected" |
| | QueryDeviceDescriptor | Display.c | no EDID -> STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED |
| Power / PnP | SetPowerState | Display.c (from KMDOD) | state tracking only, no HW save/restore |
| | NotifyAcpiEvent, DispatchIoRequest, QueryInterface, ControlEtwLogging | Display.c | trivial |
| | StopDeviceAndReleasePostDisplayOwnership | Display.c | returns POST info, no blackout |
| VidPN | IsSupportedVidPn, RecommendFunctionalVidPn, RecommendVidPnTopology, RecommendMonitorModes, EnumVidPnCofuncModality, CommitVidPn, UpdateActiveVidPnPresentPath, SetVidPnSourceVisibility, QueryVidPnHWCapability | Vidpn.c (from KMDOD) | policy: ONE mode (POST/fallback), identity scaling + rotation, A8R8G8B8 |
| | SetVidPnSourceAddress | Vidpn.c | **records the address only** (scanout hook is empty) |
| Display misc | SetPointerPosition/Shape, SetPalette, Escape, GetScanLine, ControlInterrupt | Display.c | not supported (honest NOT_SUPPORTED) |
| Debug | CollectDbgInfo | Display.c | not supported |

## Not registered
AcquireSwizzlingRange, ReleaseSwizzlingRange, StopCapture, CreateOverlay and other optional/legacy DDIs.
Which of these `DxgkInitialize` still insists on for a WDDM 2.0 full driver must be confirmed with the real WDK.

## What "loads" would still not mean
- The screen is not driven by this driver: `MadisonDisplayProgramScanout()` writes no registers, so the
  display keeps whatever the firmware (GOP/VBIOS) scans out. No modeset, no flip, no cursor, no vblank.
- No EDID, no hot-plug, no panel power sequencing, no GPU state save across D3.
- A full-graphics driver also needs a user-mode driver for D3D/DWM.
