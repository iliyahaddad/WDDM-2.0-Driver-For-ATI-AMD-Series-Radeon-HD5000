/*
 * Display.c - child devices, monitor, power and display-side DDIs.
 *
 * Adapted from the Microsoft KMDOD (kernel-mode display-only) sample,
 * Windows-driver-samples/video/KMDOD/bdd.cxx, Copyright (c) Microsoft Corporation,
 * which is published under the MIT license (see THIRD-PARTY-NOTICES.md).
 * Translated from C++ to C and changed where a full graphics driver differs from KMDOD.
 */
#include "driver.h"
#include "adapter.h"
#include "ddi.h"
#include "debug.h"

#define MADISON_DISPLAY_ADAPTER_HW_ID  DISPLAY_ADAPTER_HW_ID

// ============================================================================
// Start / stop of the display part
// ============================================================================
NTSTATUS
MadisonDisplayStart(
    _Inout_ MADISON_ADAPTER* Adapter
)
{
    MADISON_DISPLAY* d = &Adapter->Display;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    RtlZeroMemory(d, sizeof(*d));
    d->AdapterPowerState = PowerDeviceD0;
    d->MonitorPowerState = PowerDeviceD0;
    d->CurrentModes[0].DispInfo.TargetId = D3DDDI_ID_UNINITIALIZED;
    d->CurrentModes[0].Scaling  = D3DKMDT_VPPS_IDENTITY;
    d->CurrentModes[0].Rotation = D3DKMDT_VPPR_IDENTITY;

    /* Like KMDOD: the POST framebuffer tells us the mode the firmware left running. WDDM 1.2+ API. */
    if (Adapter->DxgkInterface.DxgkCbAcquirePostDisplayOwnership != NULL) {
        Status = Adapter->DxgkInterface.DxgkCbAcquirePostDisplayOwnership(
                     Adapter->DeviceHandle, &d->CurrentModes[0].DispInfo);
    }

    if (NT_SUCCESS(Status) && d->CurrentModes[0].DispInfo.Width != 0 && d->CurrentModes[0].DispInfo.Height != 0) {
        d->PostOwnershipHeld = TRUE;
        d->CurrentModes[0].Flags.PostOwned = 1;
        MADISON_INFO("POST display: %ux%u pitch=%u format=%u",
                     d->CurrentModes[0].DispInfo.Width, d->CurrentModes[0].DispInfo.Height,
                     d->CurrentModes[0].DispInfo.Pitch, (UINT)d->CurrentModes[0].DispInfo.ColorFormat);
    } else {
        /* KMDOD refuses to start here. A full driver may legitimately start without a POST framebuffer,
           so advertise a fixed 32bpp mode instead. It is NOT backed by real scanout programming. */
        MADISON_WARN("No POST display information (status 0x%08X); using %ux%u fallback mode",
                     Status, MADISON_FALLBACK_WIDTH, MADISON_FALLBACK_HEIGHT);
        RtlZeroMemory(&d->CurrentModes[0].DispInfo, sizeof(d->CurrentModes[0].DispInfo));
        d->CurrentModes[0].DispInfo.TargetId    = D3DDDI_ID_UNINITIALIZED;
        d->CurrentModes[0].DispInfo.Width       = MADISON_FALLBACK_WIDTH;
        d->CurrentModes[0].DispInfo.Height      = MADISON_FALLBACK_HEIGHT;
        d->CurrentModes[0].DispInfo.Pitch       = MADISON_FALLBACK_WIDTH * 4;
        d->CurrentModes[0].DispInfo.ColorFormat = D3DDDIFMT_A8R8G8B8;
        d->UsingFallbackMode = TRUE;
    }

    if (d->CurrentModes[0].DispInfo.ColorFormat != D3DDDIFMT_A8R8G8B8 &&
        d->CurrentModes[0].DispInfo.ColorFormat != D3DDDIFMT_X8R8G8B8) {
        MADISON_WARN("POST color format %u is not 32bpp; only A8R8G8B8 is advertised to VidPN",
                     (UINT)d->CurrentModes[0].DispInfo.ColorFormat);
    }
    return STATUS_SUCCESS;
}

VOID
MadisonDisplayStop(
    _Inout_ MADISON_ADAPTER* Adapter
)
{
    /* There is no explicit "release" callback: ownership ends when the device stops. */
    RtlZeroMemory(&Adapter->Display, sizeof(Adapter->Display));
}

NTSTATUS
MadisonDisplayProgramScanout(
    _Inout_ MADISON_ADAPTER* Adapter,
    _In_    UINT             SourceId
)
{
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(SourceId);
    /* Intentionally empty: no CRTC/GRPH programming exists yet, so the screen keeps showing whatever the
       firmware programmed. Hook point for Evergreen DCE4 scanout-address/mode programming. */
    return STATUS_SUCCESS;
}

// ============================================================================
// Child devices
// ============================================================================
NTSTATUS
MadisonDdiQueryChildRelations(
    _In_    PVOID                   pMiniportDeviceContext,
    _Inout_ DXGK_CHILD_DESCRIPTOR*  pChildRelations,
    _In_    ULONG                   ChildRelationsSize
)
{
    ULONG count, i;

    UNREFERENCED_PARAMETER(pMiniportDeviceContext);

    if (pChildRelations == NULL || ChildRelationsSize < sizeof(DXGK_CHILD_DESCRIPTOR)) {
        return STATUS_INVALID_PARAMETER;
    }

    /* The last DXGK_CHILD_DESCRIPTOR in the array must stay zeroed, so it is not counted. */
    count = (ChildRelationsSize / sizeof(DXGK_CHILD_DESCRIPTOR)) - 1;
    if (count > MADISON_MAX_CHILDREN) {
        count = MADISON_MAX_CHILDREN;
    }

    for (i = 0; i < count; ++i) {
        pChildRelations[i].ChildDeviceType = TypeVideoOutput;
        pChildRelations[i].ChildCapabilities.HpdAwareness = HpdAwarenessInterruptible;
        pChildRelations[i].ChildCapabilities.Type.VideoOutput.InterfaceTechnology = D3DKMDT_VOT_OTHER;
        pChildRelations[i].ChildCapabilities.Type.VideoOutput.MonitorOrientationAwareness = D3DKMDT_MOA_NONE;
        pChildRelations[i].ChildCapabilities.Type.VideoOutput.SupportsSdtvModes = FALSE;
        pChildRelations[i].AcpiUid = 0;     /* TODO: real ACPI _ADR of the panel, if any */
        pChildRelations[i].ChildUid = i;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MadisonDdiQueryChildStatus(
    _In_    PVOID             pMiniportDeviceContext,
    _Inout_ DXGK_CHILD_STATUS* pChildStatus,
    _In_    BOOLEAN           NonDestructiveOnly
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;
    UNREFERENCED_PARAMETER(NonDestructiveOnly);

    if (pAdapter == NULL || pChildStatus == NULL || pChildStatus->ChildUid >= MADISON_MAX_CHILDREN) {
        return STATUS_INVALID_PARAMETER;
    }

    switch (pChildStatus->Type) {
    case StatusConnection:
        /* No hot-plug detection exists: report connected while the adapter is running. */
        pChildStatus->HotPlug.Connected = pAdapter->Started;
        return STATUS_SUCCESS;

    case StatusRotation:
        /* D3DKMDT_MOA_NONE was reported, so this should never be queried. */
        return STATUS_INVALID_PARAMETER;

    default:
        MADISON_WARN("Unknown child status type %u", (UINT)pChildStatus->Type);
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
MadisonDdiQueryDeviceDescriptor(
    _In_    PVOID                   pMiniportDeviceContext,
    _In_    ULONG                   ChildUid,
    _Inout_ DXGK_DEVICE_DESCRIPTOR* pDeviceDescriptor
)
{
    UNREFERENCED_PARAMETER(pMiniportDeviceContext);
    UNREFERENCED_PARAMETER(pDeviceDescriptor);

    if (ChildUid >= MADISON_MAX_CHILDREN) {
        return STATUS_INVALID_PARAMETER;
    }
    /* EDID (DDC/I2C or VBIOS panel info) is not implemented. Dxgkrnl then falls back to a default
       monitor and asks for modes through DxgkDdiRecommendMonitorModes. */
    return STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED;
}

// ============================================================================
// Power / PnP / misc
// ============================================================================
NTSTATUS
MadisonDdiSetPowerState(
    _In_ PVOID              pMiniportDeviceContext,
    _In_ ULONG              HardwareUid,
    _In_ DEVICE_POWER_STATE DevicePowerState,
    _In_ POWER_ACTION       ActionType
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;
    UNREFERENCED_PARAMETER(ActionType);

    if (pAdapter == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (HardwareUid == MADISON_DISPLAY_ADAPTER_HW_ID) {
        if (DevicePowerState == PowerDeviceD0 && pAdapter->Display.AdapterPowerState == PowerDeviceD3) {
            /* Returning from D3: every source is defined to be invisible again. */
            UINT i;
            for (i = 0; i < MADISON_MAX_VIEWS; ++i) {
                pAdapter->Display.CurrentModes[i].Flags.SourceNotVisible = 1;
            }
        }
        /* State tracking only: GPU/CP/GART state is neither saved nor restored across D3 yet. */
        if (DevicePowerState != PowerDeviceD0 && pAdapter->CpRunning) {
            MADISON_WARN("Adapter leaving D0 with CP running: hardware state is not saved");
        }
        pAdapter->Display.AdapterPowerState = DevicePowerState;
        return STATUS_SUCCESS;
    }

    if (HardwareUid >= MADISON_MAX_CHILDREN) {
        return STATUS_INVALID_PARAMETER;
    }
    /* TODO: power the panel/monitor up or down (DPMS). */
    pAdapter->Display.MonitorPowerState = DevicePowerState;
    return STATUS_SUCCESS;
}

NTSTATUS
MadisonDdiNotifyAcpiEvent(
    _In_  PVOID           pMiniportDeviceContext,
    _In_  DXGK_EVENT_TYPE EventType,
    _In_  ULONG           Event,
    _In_  PVOID           Argument,
    _Out_ PULONG          AcpiFlags
)
{
    UNREFERENCED_PARAMETER(pMiniportDeviceContext);
    UNREFERENCED_PARAMETER(EventType);
    UNREFERENCED_PARAMETER(Event);
    UNREFERENCED_PARAMETER(Argument);
    if (AcpiFlags != NULL) {
        *AcpiFlags = 0;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MadisonDdiDispatchIoRequest(
    _In_ PVOID                pMiniportDeviceContext,
    _In_ ULONG                VidPnSourceId,
    _In_ VIDEO_REQUEST_PACKET* pVideoRequestPacket
)
{
    UNREFERENCED_PARAMETER(pMiniportDeviceContext);
    UNREFERENCED_PARAMETER(VidPnSourceId);
    UNREFERENCED_PARAMETER(pVideoRequestPacket);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
MadisonDdiResetDevice(
    _In_ PVOID pMiniportDeviceContext
)
{
    UNREFERENCED_PARAMETER(pMiniportDeviceContext);
    /* Called at bugcheck/hibernate time; nothing to restore (VBIOS owns the display). */
}

NTSTATUS
MadisonDdiQueryInterface(
    _In_ PVOID           pMiniportDeviceContext,
    _In_ PQUERY_INTERFACE QueryInterface
)
{
    UNREFERENCED_PARAMETER(pMiniportDeviceContext);
    UNREFERENCED_PARAMETER(QueryInterface);
    return STATUS_NOT_SUPPORTED;
}

VOID
MadisonDdiControlEtwLogging(
    _In_ BOOLEAN Enable,
    _In_ ULONG   Flags,
    _In_ UCHAR   Level
)
{
    UNREFERENCED_PARAMETER(Enable);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Level);
}

NTSTATUS
MadisonDdiResetFromTimeout(
    _In_ CONST HANDLE hAdapter
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    if (pAdapter == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    /* No working hardware reset yet: report that truthfully instead of pretending the GPU recovered. */
    return MadisonResetEngine(pAdapter, 0, TRUE);
}

NTSTATUS
MadisonDdiRestartFromTimeout(
    _In_ CONST HANDLE hAdapter
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    return STATUS_SUCCESS;
}

NTSTATUS
MadisonDdiStopDeviceAndReleasePostDisplayOwnership(
    _In_  PVOID                          pMiniportDeviceContext,
    _In_  D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId,
    _Out_ PDXGK_DISPLAY_INFORMATION      pDisplayInfo
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;
    UNREFERENCED_PARAMETER(TargetId);

    if (pAdapter == NULL || pDisplayInfo == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    /* Hand the POST mode description back so the next driver (e.g. Basic Display) can take over.
       KMDOD also blacks out the screen here; this driver has no mapped framebuffer to clear. */
    *pDisplayInfo = pAdapter->Display.CurrentModes[0].DispInfo;
    return MadisonDdiStopDevice(pAdapter);
}

// ============================================================================
// Pointer, palette, escape, scanline, interrupt control
// ============================================================================
NTSTATUS
MadisonDdiSetPointerPosition(
    _In_ CONST HANDLE                       hAdapter,
    _In_ CONST DXGKARG_SETPOINTERPOSITION*  pSetPointerPosition
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    if (pSetPointerPosition == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    /* No hardware cursor is advertised (PointerCaps == 0); only "hide" may legitimately arrive. */
    return pSetPointerPosition->Flags.Visible ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

NTSTATUS
MadisonDdiSetPointerShape(
    _In_ CONST HANDLE                   hAdapter,
    _In_ CONST DXGKARG_SETPOINTERSHAPE* pSetPointerShape
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pSetPointerShape);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
MadisonDdiSetPalette(
    _In_ CONST HANDLE               hAdapter,
    _In_ CONST DXGKARG_SETPALETTE*  pSetPalette
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pSetPalette);
    return STATUS_NOT_SUPPORTED;      /* no palettized modes are advertised */
}

NTSTATUS
MadisonDdiEscape(
    _In_ CONST HANDLE            hAdapter,
    _In_ CONST DXGKARG_ESCAPE*   pEscape
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pEscape);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
MadisonDdiGetScanLine(
    _In_    CONST HANDLE          hAdapter,
    _Inout_ DXGKARG_GETSCANLINE*  pGetScanLine
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pGetScanLine);
    return STATUS_NOT_SUPPORTED;      /* needs the Evergreen CRTC status register */
}

NTSTATUS
MadisonDdiControlInterrupt(
    _In_ CONST HANDLE             hAdapter,
    _In_ CONST DXGK_INTERRUPT_TYPE InterruptType,
    _In_ BOOLEAN                  EnableInterrupt
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(InterruptType);
    UNREFERENCED_PARAMETER(EnableInterrupt);
    return STATUS_NOT_SUPPORTED;      /* IH ring / vblank interrupts are not implemented */
}

// ============================================================================
// DxgkDdiCollectDbgInfo
// ============================================================================
NTSTATUS
MadisonDdiCollectDbgInfo(
    _In_ CONST HANDLE                    hAdapter,
    _In_ CONST DXGKARG_COLLECTDBGINFO*   pCollectDbgInfo
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    if (pAdapter == NULL || pCollectDbgInfo == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiCollectDbgInfo");
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_NOT_SUPPORTED;
}
