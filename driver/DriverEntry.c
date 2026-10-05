#include "driver.h"
#include "adapter.h"
#include "ddi.h"
#include "debug.h"

// Global debug level (errors/warnings/info by default; ALL is far too noisy for a kernel driver)
ULONG g_MadisonDebugLevel = MADISON_DEBUG_ERROR | MADISON_DEBUG_WARN | MADISON_DEBUG_INFO;

// ============================================================================
// DriverEntry
// ============================================================================
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  pDriverObject,
    _In_ PUNICODE_STRING pRegistryPath
)
{
    NTSTATUS Status;
    DRIVER_INITIALIZATION_DATA InitData;

    RtlZeroMemory(&InitData, sizeof(InitData));

    MADISON_INFO("DriverEntry - Madison WDDM 2.0 Physical-Mode Prototype");
    MADISON_INFO("Target: ATI/AMD Mobility Radeon HD 5730 (Madison/Juniper/Evergreen)");
    MADISON_INFO("Mode: WDDM 2.0 Physical Addressing, PreemptionAware=0");

    InitData.Version = MADISON_WDDM_INTERFACE_VERSION;

    // PnP / adapter lifetime
    InitData.DxgkDdiAddDevice         = MadisonDdiAddDevice;
    InitData.DxgkDdiStartDevice       = MadisonDdiStartDevice;
    InitData.DxgkDdiStopDevice        = MadisonDdiStopDevice;
    InitData.DxgkDdiRemoveDevice      = MadisonDdiRemoveDevice;
    InitData.DxgkDdiUnload            = MadisonDriverUnload;
    InitData.DxgkDdiQueryAdapterInfo  = MadisonDdiQueryAdapterInfo;

    // Interrupts
    InitData.DxgkDdiInterruptRoutine  = MadisonDdiInterruptRoutine;
    InitData.DxgkDdiDpcRoutine        = MadisonDdiDpcRoutine;

    // Scheduling / memory management
    InitData.DxgkDdiPatch             = MadisonDdiPatch;
    InitData.DxgkDdiSubmitCommand     = MadisonDdiSubmitCommand;
    InitData.DxgkDdiBuildPagingBuffer = MadisonDdiBuildPagingBuffer;
    InitData.DxgkDdiQueryCurrentFence = MadisonDdiQueryCurrentFence;
    InitData.DxgkDdiResetEngine       = MadisonDdiResetEngine;
    InitData.DxgkDdiPreemptCommand    = MadisonDdiPreemptCommand;
    InitData.DxgkDdiGetNodeMetadata   = MadisonDdiGetNodeMetadata;

    // Devices / contexts
    InitData.DxgkDdiCreateDevice      = MadisonDdiCreateDevice;
    InitData.DxgkDdiDestroyDevice     = MadisonDdiDestroyDevice;
    InitData.DxgkDdiCreateContext     = MadisonDdiCreateContext;
    InitData.DxgkDdiDestroyContext    = MadisonDdiDestroyContext;
    InitData.DxgkDdiRender            = MadisonDdiRender;
    InitData.DxgkDdiPresent           = MadisonDdiPresent;

    // Allocations
    InitData.DxgkDdiCreateAllocation              = MadisonDdiCreateAllocation;
    InitData.DxgkDdiDestroyAllocation             = MadisonDdiDestroyAllocation;
    InitData.DxgkDdiDescribeAllocation            = MadisonDdiDescribeAllocation;
    InitData.DxgkDdiGetStandardAllocationDriverData = MadisonDdiGetStandardAllocationDriverData;
    InitData.DxgkDdiOpenAllocation                = MadisonDdiOpenAllocation;
    InitData.DxgkDdiCloseAllocation               = MadisonDdiCloseAllocation;

    // Debug
    InitData.DxgkDdiCollectDbgInfo    = MadisonDdiCollectDbgInfo;

    // Child devices / monitor / power / PnP (required by Dxgkrnl for any display miniport)
    InitData.DxgkDdiQueryChildRelations   = MadisonDdiQueryChildRelations;
    InitData.DxgkDdiQueryChildStatus      = MadisonDdiQueryChildStatus;
    InitData.DxgkDdiQueryDeviceDescriptor = MadisonDdiQueryDeviceDescriptor;
    InitData.DxgkDdiSetPowerState         = MadisonDdiSetPowerState;
    InitData.DxgkDdiNotifyAcpiEvent       = MadisonDdiNotifyAcpiEvent;
    InitData.DxgkDdiDispatchIoRequest     = MadisonDdiDispatchIoRequest;
    InitData.DxgkDdiResetDevice           = MadisonDdiResetDevice;
    InitData.DxgkDdiQueryInterface        = MadisonDdiQueryInterface;
    InitData.DxgkDdiControlEtwLogging     = MadisonDdiControlEtwLogging;
    InitData.DxgkDdiResetFromTimeout      = MadisonDdiResetFromTimeout;
    InitData.DxgkDdiRestartFromTimeout    = MadisonDdiRestartFromTimeout;
    InitData.DxgkDdiStopDeviceAndReleasePostDisplayOwnership = MadisonDdiStopDeviceAndReleasePostDisplayOwnership;

    // Pointer / palette / misc display
    InitData.DxgkDdiSetPointerPosition    = MadisonDdiSetPointerPosition;
    InitData.DxgkDdiSetPointerShape       = MadisonDdiSetPointerShape;
    InitData.DxgkDdiSetPalette            = MadisonDdiSetPalette;
    InitData.DxgkDdiEscape                = MadisonDdiEscape;
    InitData.DxgkDdiGetScanLine           = MadisonDdiGetScanLine;
    InitData.DxgkDdiControlInterrupt      = MadisonDdiControlInterrupt;

    // VidPN / display mode management
    InitData.DxgkDdiIsSupportedVidPn          = MadisonDdiIsSupportedVidPn;
    InitData.DxgkDdiRecommendFunctionalVidPn  = MadisonDdiRecommendFunctionalVidPn;
    InitData.DxgkDdiRecommendVidPnTopology    = MadisonDdiRecommendVidPnTopology;
    InitData.DxgkDdiRecommendMonitorModes     = MadisonDdiRecommendMonitorModes;
    InitData.DxgkDdiEnumVidPnCofuncModality   = MadisonDdiEnumVidPnCofuncModality;
    InitData.DxgkDdiSetVidPnSourceAddress     = MadisonDdiSetVidPnSourceAddress;
    InitData.DxgkDdiSetVidPnSourceVisibility  = MadisonDdiSetVidPnSourceVisibility;
    InitData.DxgkDdiCommitVidPn               = MadisonDdiCommitVidPn;
    InitData.DxgkDdiUpdateActiveVidPnPresentPath = MadisonDdiUpdateActiveVidPnPresentPath;
    InitData.DxgkDdiQueryVidPnHWCapability    = MadisonDdiQueryVidPnHWCapability;

    /* Still NOT registered (not covered by the Microsoft KMDOD sample, which is display-only):
       AcquireSwizzlingRange/ReleaseSwizzlingRange, StopCapture, CreateOverlay and other optional/legacy DDIs.
       Whether DxgkInitialize requires any of them for WDDM 2.0 must be confirmed with the real WDK. */

    Status = DxgkInitialize(pDriverObject, pRegistryPath, &InitData);
    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("DxgkInitialize failed: 0x%08X", Status);
        return Status;
    }

    MADISON_INFO("DriverEntry completed successfully");
    return STATUS_SUCCESS;
}

// ============================================================================
// Unload
// ============================================================================
VOID
MadisonDriverUnload(
    _In_ PDRIVER_OBJECT pDriverObject
)
{
    UNREFERENCED_PARAMETER(pDriverObject);
    MADISON_INFO("Driver unload");
}
