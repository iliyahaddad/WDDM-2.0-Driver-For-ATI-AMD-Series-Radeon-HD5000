/*
 * Vidpn.c - VidPN / display-mode-management DDIs.
 *
 * Adapted from the Microsoft KMDOD (kernel-mode display-only) sample,
 * Windows-driver-samples/video/KMDOD/bdd_dmm.cxx, Copyright (c) Microsoft Corporation,
 * published under the MIT license (see THIRD-PARTY-NOTICES.md). Translated from C++ to C.
 *
 * Differences from KMDOD, all because this driver cannot do software blit/scale/rotate and has no
 * mode-setting code yet:
 *   - only the POST (or fallback) mode is offered, not the sample's table of smaller modes;
 *   - scaling support = identity only (KMDOD also offers centered);
 *   - rotation support = identity only (KMDOD offers 90 degrees in software);
 *   - the framebuffer is never mapped or blitted; MadisonDisplayProgramScanout() is the (empty) hook.
 * In the sample's rotation block the pivot test reads "!= D3DKMDT_EPT_ROTATION", which contradicts
 * its own comment; "==" is used here.
 */
#include "driver.h"
#include "adapter.h"
#include "ddi.h"
#include "debug.h"

static const D3DDDIFORMAT gMadisonPixelFormats[] = { D3DDDIFMT_A8R8G8B8 };
#define MADISON_PIXEL_FORMAT_COUNT  (sizeof(gMadisonPixelFormats) / sizeof(gMadisonPixelFormats[0]))

// ----------------------------------------------------------------------------
// Validation helpers
// ----------------------------------------------------------------------------
static NTSTATUS
MadisonIsVidPnPathFieldsValid(_In_ CONST D3DKMDT_VIDPN_PRESENT_PATH* pPath)
{
    if (pPath->VidPnSourceId >= MADISON_MAX_VIEWS) {
        MADISON_ERROR("VidPnSourceId %u too high", (UINT)pPath->VidPnSourceId);
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE;
    }
    if (pPath->VidPnTargetId >= MADISON_MAX_CHILDREN) {
        MADISON_ERROR("VidPnTargetId %u too high", (UINT)pPath->VidPnTargetId);
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET;
    }
    if (pPath->GammaRamp.Type != D3DDDI_GAMMARAMP_DEFAULT) {
        MADISON_ERROR("Path contains a gamma ramp (%u)", (UINT)pPath->GammaRamp.Type);
        return STATUS_GRAPHICS_GAMMA_RAMP_NOT_SUPPORTED;
    }
    if (pPath->ContentTransformation.Scaling != D3DKMDT_VPPS_IDENTITY &&
        pPath->ContentTransformation.Scaling != D3DKMDT_VPPS_NOTSPECIFIED &&
        pPath->ContentTransformation.Scaling != D3DKMDT_VPPS_UNINITIALIZED) {
        MADISON_ERROR("Unsupported scaling %u", (UINT)pPath->ContentTransformation.Scaling);
        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }
    if (pPath->ContentTransformation.Rotation != D3DKMDT_VPPR_IDENTITY &&
        pPath->ContentTransformation.Rotation != D3DKMDT_VPPR_NOTSPECIFIED &&
        pPath->ContentTransformation.Rotation != D3DKMDT_VPPR_UNINITIALIZED) {
        MADISON_ERROR("Unsupported rotation %u", (UINT)pPath->ContentTransformation.Rotation);
        return STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED;
    }
    if (pPath->VidPnTargetColorBasis != D3DKMDT_CB_SCRGB &&
        pPath->VidPnTargetColorBasis != D3DKMDT_CB_UNINITIALIZED) {
        MADISON_ERROR("Unsupported color basis %u", (UINT)pPath->VidPnTargetColorBasis);
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
MadisonIsVidPnSourceModeFieldsValid(_In_ CONST D3DKMDT_VIDPN_SOURCE_MODE* pSourceMode)
{
    UINT i;

    if (pSourceMode->Type != D3DKMDT_RMT_GRAPHICS) {
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }
    if (pSourceMode->Format.Graphics.ColorBasis != D3DKMDT_CB_SCRGB &&
        pSourceMode->Format.Graphics.ColorBasis != D3DKMDT_CB_UNINITIALIZED) {
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }
    if (pSourceMode->Format.Graphics.PixelValueAccessMode != D3DKMDT_PVAM_DIRECT) {
        return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
    }
    for (i = 0; i < MADISON_PIXEL_FORMAT_COUNT; ++i) {
        if (pSourceMode->Format.Graphics.PixelFormat == gMadisonPixelFormats[i]) {
            return STATUS_SUCCESS;
        }
    }
    MADISON_ERROR("Unknown pixel format %u", (UINT)pSourceMode->Format.Graphics.PixelFormat);
    return STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE;
}

// ----------------------------------------------------------------------------
// Mode population helpers
// ----------------------------------------------------------------------------
static NTSTATUS
MadisonAddSingleSourceMode(
    _In_ MADISON_ADAPTER*                       pAdapter,
    _In_ CONST DXGK_VIDPNSOURCEMODESET_INTERFACE* pSourceModeSetInterface,
    _In_ D3DKMDT_HVIDPNSOURCEMODESET            hSourceModeSet,
    _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID         SourceId
)
{
    UINT i;
    CONST DXGK_DISPLAY_INFORMATION* pDisp = &pAdapter->Display.CurrentModes[SourceId].DispInfo;

    for (i = 0; i < MADISON_PIXEL_FORMAT_COUNT; ++i) {
        D3DKMDT_VIDPN_SOURCE_MODE* pMode = NULL;
        NTSTATUS Status = pSourceModeSetInterface->pfnCreateNewModeInfo(hSourceModeSet, &pMode);
        if (!NT_SUCCESS(Status)) {
            MADISON_ERROR("pfnCreateNewModeInfo(source) failed: 0x%08X", Status);
            return Status;
        }

        pMode->Type = D3DKMDT_RMT_GRAPHICS;
        pMode->Format.Graphics.PrimSurfSize.cx = pDisp->Width;
        pMode->Format.Graphics.PrimSurfSize.cy = pDisp->Height;
        pMode->Format.Graphics.VisibleRegionSize = pMode->Format.Graphics.PrimSurfSize;
        pMode->Format.Graphics.Stride = pDisp->Pitch;
        pMode->Format.Graphics.PixelFormat = gMadisonPixelFormats[i];
        pMode->Format.Graphics.ColorBasis = D3DKMDT_CB_SCRGB;
        pMode->Format.Graphics.PixelValueAccessMode = D3DKMDT_PVAM_DIRECT;

        Status = pSourceModeSetInterface->pfnAddMode(hSourceModeSet, pMode);
        if (!NT_SUCCESS(Status)) {
            /* The mode was not consumed; release it. */
            (VOID)pSourceModeSetInterface->pfnReleaseModeInfo(hSourceModeSet, pMode);
            if (Status != STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET) {
                MADISON_ERROR("pfnAddMode(source) failed: 0x%08X", Status);
                return Status;
            }
        }
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
MadisonAddSingleTargetMode(
    _In_ MADISON_ADAPTER*                       pAdapter,
    _In_ CONST DXGK_VIDPNTARGETMODESET_INTERFACE* pTargetModeSetInterface,
    _In_ D3DKMDT_HVIDPNTARGETMODESET            hTargetModeSet,
    _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID         SourceId
)
{
    D3DKMDT_VIDPN_TARGET_MODE* pMode = NULL;
    CONST DXGK_DISPLAY_INFORMATION* pDisp = &pAdapter->Display.CurrentModes[SourceId].DispInfo;
    NTSTATUS Status = pTargetModeSetInterface->pfnCreateNewModeInfo(hTargetModeSet, &pMode);

    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("pfnCreateNewModeInfo(target) failed: 0x%08X", Status);
        return Status;
    }

    pMode->VideoSignalInfo.VideoStandard = D3DKMDT_VSS_OTHER;
    pMode->VideoSignalInfo.TotalSize.cx = pDisp->Width;
    pMode->VideoSignalInfo.TotalSize.cy = pDisp->Height;
    pMode->VideoSignalInfo.ActiveSize = pMode->VideoSignalInfo.TotalSize;
    pMode->VideoSignalInfo.VSyncFreq.Numerator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.VSyncFreq.Denominator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.HSyncFreq.Numerator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.HSyncFreq.Denominator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.PixelRate = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.ScanLineOrdering = D3DDDI_VSSLO_PROGRESSIVE;
    pMode->Preference = D3DKMDT_MP_PREFERRED;     /* the only supported target mode */

    Status = pTargetModeSetInterface->pfnAddMode(hTargetModeSet, pMode);
    if (!NT_SUCCESS(Status)) {
        (VOID)pTargetModeSetInterface->pfnReleaseModeInfo(hTargetModeSet, pMode);
        if (Status != STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET) {
            MADISON_ERROR("pfnAddMode(target) failed: 0x%08X", Status);
            return Status;
        }
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
MadisonAddSingleMonitorMode(
    _In_ MADISON_ADAPTER*                          pAdapter,
    _In_ CONST DXGKARG_RECOMMENDMONITORMODES* CONST pRecommendMonitorModes
)
{
    D3DKMDT_MONITOR_SOURCE_MODE* pMode = NULL;
    CONST DXGK_DISPLAY_INFORMATION* pDisp = &pAdapter->Display.CurrentModes[0].DispInfo;
    NTSTATUS Status = pRecommendMonitorModes->pMonitorSourceModeSetInterface->pfnCreateNewModeInfo(
                          pRecommendMonitorModes->hMonitorSourceModeSet, &pMode);

    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("pfnCreateNewModeInfo(monitor) failed: 0x%08X", Status);
        return Status;
    }

    /* No EDID: describe the one mode we can actually show. */
    pMode->VideoSignalInfo.VideoStandard = D3DKMDT_VSS_OTHER;
    pMode->VideoSignalInfo.TotalSize.cx = pDisp->Width;
    pMode->VideoSignalInfo.TotalSize.cy = pDisp->Height;
    pMode->VideoSignalInfo.ActiveSize = pMode->VideoSignalInfo.TotalSize;
    pMode->VideoSignalInfo.VSyncFreq.Numerator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.VSyncFreq.Denominator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.HSyncFreq.Numerator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.HSyncFreq.Denominator = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.PixelRate = D3DKMDT_FREQUENCY_NOTSPECIFIED;
    pMode->VideoSignalInfo.ScanLineOrdering = D3DDDI_VSSLO_PROGRESSIVE;
    pMode->ColorBasis = D3DKMDT_CB_SRGB;
    pMode->ColorCoeffDynamicRanges.FirstChannel = 8;
    pMode->ColorCoeffDynamicRanges.SecondChannel = 8;
    pMode->ColorCoeffDynamicRanges.ThirdChannel = 8;
    pMode->ColorCoeffDynamicRanges.FourthChannel = 8;
    pMode->Origin = D3DKMDT_MCO_DRIVER;
    pMode->Preference = D3DKMDT_MP_PREFERRED;

    Status = pRecommendMonitorModes->pMonitorSourceModeSetInterface->pfnAddMode(
                 pRecommendMonitorModes->hMonitorSourceModeSet, pMode);
    if (!NT_SUCCESS(Status)) {
        (VOID)pRecommendMonitorModes->pMonitorSourceModeSetInterface->pfnReleaseModeInfo(
                  pRecommendMonitorModes->hMonitorSourceModeSet, pMode);
        if (Status != STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET) {
            MADISON_ERROR("pfnAddMode(monitor) failed: 0x%08X", Status);
            return Status;
        }
    }
    return STATUS_SUCCESS;
}

// ----------------------------------------------------------------------------
// DxgkDdiIsSupportedVidPn
// ----------------------------------------------------------------------------
NTSTATUS
MadisonDdiIsSupportedVidPn(
    _In_    CONST HANDLE             hAdapter,
    _Inout_ DXGKARG_ISSUPPORTEDVIDPN* pIsSupportedVidPn
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    CONST DXGK_VIDPN_INTERFACE* pVidPnInterface = NULL;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE* pTopologyInterface = NULL;
    D3DKMDT_HVIDPNTOPOLOGY hTopology = 0;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID SourceId;
    NTSTATUS Status;

    if (pAdapter == NULL || pIsSupportedVidPn == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (pIsSupportedVidPn->hDesiredVidPn == 0) {
        pIsSupportedVidPn->IsVidPnSupported = TRUE;      /* a null VidPN is supported */
        return STATUS_SUCCESS;
    }

    pIsSupportedVidPn->IsVidPnSupported = FALSE;         /* until shown otherwise */

    Status = pAdapter->DxgkInterface.DxgkCbQueryVidPnInterface(
                 pIsSupportedVidPn->hDesiredVidPn, DXGK_VIDPN_INTERFACE_VERSION_V1, &pVidPnInterface);
    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("DxgkCbQueryVidPnInterface failed: 0x%08X", Status);
        return Status;
    }

    Status = pVidPnInterface->pfnGetTopology(pIsSupportedVidPn->hDesiredVidPn, &hTopology, &pTopologyInterface);
    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("pfnGetTopology failed: 0x%08X", Status);
        return Status;
    }

    /* Each source may not have more paths than there are targets. (The sample notes it should also
       check pinned modes and path parameters; EnumVidPnCofuncModality/CommitVidPn validate those.) */
    for (SourceId = 0; SourceId < MADISON_MAX_VIEWS; ++SourceId) {
        SIZE_T NumPathsFromSource = 0;
        Status = pTopologyInterface->pfnGetNumPathsFromSource(hTopology, SourceId, &NumPathsFromSource);
        if (Status == STATUS_GRAPHICS_SOURCE_NOT_IN_TOPOLOGY) {
            continue;
        }
        if (!NT_SUCCESS(Status)) {
            MADISON_ERROR("pfnGetNumPathsFromSource failed: 0x%08X", Status);
            return Status;
        }
        if (NumPathsFromSource > MADISON_MAX_CHILDREN) {
            return STATUS_SUCCESS;                        /* not supported (default already FALSE) */
        }
    }

    pIsSupportedVidPn->IsVidPnSupported = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
MadisonDdiRecommendFunctionalVidPn(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_RECOMMENDFUNCTIONALVIDPN* CONST pRecommendFunctionalVidPn
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pRecommendFunctionalVidPn);
    return STATUS_GRAPHICS_NO_RECOMMENDED_FUNCTIONAL_VIDPN;
}

NTSTATUS
MadisonDdiRecommendVidPnTopology(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_RECOMMENDVIDPNTOPOLOGY* CONST pRecommendVidPnTopology
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pRecommendVidPnTopology);
    return STATUS_GRAPHICS_NO_RECOMMENDED_FUNCTIONAL_VIDPN;
}

NTSTATUS
MadisonDdiRecommendMonitorModes(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_RECOMMENDMONITORMODES* CONST pRecommendMonitorModes
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    if (pAdapter == NULL || pRecommendMonitorModes == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    /* No EDID: Dxgkrnl pre-fills default monitor modes, so the mode we really need is added here. */
    return MadisonAddSingleMonitorMode(pAdapter, pRecommendMonitorModes);
}

// ----------------------------------------------------------------------------
// DxgkDdiEnumVidPnCofuncModality
// ----------------------------------------------------------------------------
NTSTATUS
MadisonDdiEnumVidPnCofuncModality(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_ENUMVIDPNCOFUNCMODALITY* CONST pEnum
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    D3DKMDT_HVIDPNTOPOLOGY hTopology = 0;
    D3DKMDT_HVIDPNSOURCEMODESET hSourceModeSet = 0;
    D3DKMDT_HVIDPNTARGETMODESET hTargetModeSet = 0;
    CONST DXGK_VIDPN_INTERFACE* pVidPnInterface = NULL;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE* pTopologyInterface = NULL;
    CONST DXGK_VIDPNSOURCEMODESET_INTERFACE* pSourceModeSetInterface = NULL;
    CONST DXGK_VIDPNTARGETMODESET_INTERFACE* pTargetModeSetInterface = NULL;
    CONST D3DKMDT_VIDPN_PRESENT_PATH* pPath = NULL;
    CONST D3DKMDT_VIDPN_PRESENT_PATH* pPathTemp = NULL;
    CONST D3DKMDT_VIDPN_SOURCE_MODE* pPinnedSourceMode = NULL;
    CONST D3DKMDT_VIDPN_TARGET_MODE* pPinnedTargetMode = NULL;
    NTSTATUS Status;
    NTSTATUS TempStatus;

    if (pAdapter == NULL || pEnum == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = pAdapter->DxgkInterface.DxgkCbQueryVidPnInterface(
                 pEnum->hConstrainingVidPn, DXGK_VIDPN_INTERFACE_VERSION_V1, &pVidPnInterface);
    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("DxgkCbQueryVidPnInterface failed: 0x%08X", Status);
        return Status;
    }

    Status = pVidPnInterface->pfnGetTopology(pEnum->hConstrainingVidPn, &hTopology, &pTopologyInterface);
    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("pfnGetTopology failed: 0x%08X", Status);
        return Status;
    }

    Status = pTopologyInterface->pfnAcquireFirstPathInfo(hTopology, &pPath);
    if (!NT_SUCCESS(Status)) {
        MADISON_ERROR("pfnAcquireFirstPathInfo failed: 0x%08X", Status);
        return Status;
    }

    if (Status == STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET) {
        pPath = NULL;                                       /* empty topology: nothing is held */
    }

    while (Status != STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET) {
        D3DKMDT_VIDPN_PRESENT_PATH LocalPath;
        BOOLEAN SupportFieldsModified = FALSE;

        /* ---- SOURCE MODES ---- */
        Status = pVidPnInterface->pfnAcquireSourceModeSet(pEnum->hConstrainingVidPn, pPath->VidPnSourceId,
                                                          &hSourceModeSet, &pSourceModeSetInterface);
        if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquireSourceModeSet failed: 0x%08X", Status); break; }

        /* The pinned mode is needed when the source (or target) is not the pivot. */
        Status = pSourceModeSetInterface->pfnAcquirePinnedModeInfo(hSourceModeSet, &pPinnedSourceMode);
        if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquirePinnedModeInfo(source) failed: 0x%08X", Status); break; }

        if (!(pEnum->EnumPivotType == D3DKMDT_EPT_VIDPNSOURCE &&
              pEnum->EnumPivot.VidPnSourceId == pPath->VidPnSourceId)) {
            if (pPinnedSourceMode == NULL) {
                /* No pinned source: replace the set with one holding all supported modes. */
                Status = pVidPnInterface->pfnReleaseSourceModeSet(pEnum->hConstrainingVidPn, hSourceModeSet);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnReleaseSourceModeSet failed: 0x%08X", Status); break; }
                hSourceModeSet = 0;

                Status = pVidPnInterface->pfnCreateNewSourceModeSet(pEnum->hConstrainingVidPn, pPath->VidPnSourceId,
                                                                    &hSourceModeSet, &pSourceModeSetInterface);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnCreateNewSourceModeSet failed: 0x%08X", Status); break; }

                Status = MadisonAddSingleSourceMode(pAdapter, pSourceModeSetInterface, hSourceModeSet, pPath->VidPnSourceId);
                if (!NT_SUCCESS(Status)) { break; }

                Status = pVidPnInterface->pfnAssignSourceModeSet(pEnum->hConstrainingVidPn, pPath->VidPnSourceId, hSourceModeSet);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAssignSourceModeSet failed: 0x%08X", Status); break; }
                hSourceModeSet = 0;   /* assigned == released */
            }
        }

        /* ---- TARGET MODES ---- */
        if (!(pEnum->EnumPivotType == D3DKMDT_EPT_VIDPNTARGET &&
              pEnum->EnumPivot.VidPnTargetId == pPath->VidPnTargetId)) {
            Status = pVidPnInterface->pfnAcquireTargetModeSet(pEnum->hConstrainingVidPn, pPath->VidPnTargetId,
                                                              &hTargetModeSet, &pTargetModeSetInterface);
            if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquireTargetModeSet failed: 0x%08X", Status); break; }

            Status = pTargetModeSetInterface->pfnAcquirePinnedModeInfo(hTargetModeSet, &pPinnedTargetMode);
            if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquirePinnedModeInfo(target) failed: 0x%08X", Status); break; }

            if (pPinnedTargetMode == NULL) {
                Status = pVidPnInterface->pfnReleaseTargetModeSet(pEnum->hConstrainingVidPn, hTargetModeSet);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnReleaseTargetModeSet failed: 0x%08X", Status); break; }
                hTargetModeSet = 0;

                Status = pVidPnInterface->pfnCreateNewTargetModeSet(pEnum->hConstrainingVidPn, pPath->VidPnTargetId,
                                                                    &hTargetModeSet, &pTargetModeSetInterface);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnCreateNewTargetModeSet failed: 0x%08X", Status); break; }

                Status = MadisonAddSingleTargetMode(pAdapter, pTargetModeSetInterface, hTargetModeSet, pPath->VidPnSourceId);
                if (!NT_SUCCESS(Status)) { break; }

                Status = pVidPnInterface->pfnAssignTargetModeSet(pEnum->hConstrainingVidPn, pPath->VidPnTargetId, hTargetModeSet);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAssignTargetModeSet failed: 0x%08X", Status); break; }
                hTargetModeSet = 0;
            } else {
                /* Pinned target: nothing to add, release it and the set. */
                Status = pTargetModeSetInterface->pfnReleaseModeInfo(hTargetModeSet, pPinnedTargetMode);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnReleaseModeInfo(target) failed: 0x%08X", Status); break; }
                pPinnedTargetMode = NULL;

                Status = pVidPnInterface->pfnReleaseTargetModeSet(pEnum->hConstrainingVidPn, hTargetModeSet);
                if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnReleaseTargetModeSet failed: 0x%08X", Status); break; }
                hTargetModeSet = 0;
            }
        }

        /* Release the pinned source mode and, if still held, the source mode set. */
        if (pPinnedSourceMode != NULL) {
            Status = pSourceModeSetInterface->pfnReleaseModeInfo(hSourceModeSet, pPinnedSourceMode);
            if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnReleaseModeInfo(source) failed: 0x%08X", Status); break; }
            pPinnedSourceMode = NULL;
        }
        if (hSourceModeSet != 0) {
            Status = pVidPnInterface->pfnReleaseSourceModeSet(pEnum->hConstrainingVidPn, hSourceModeSet);
            if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnReleaseSourceModeSet failed: 0x%08X", Status); break; }
            hSourceModeSet = 0;
        }

        /* ---- PATH SUPPORT FIELDS (the acquired path is const, so edit a copy) ---- */
        LocalPath = *pPath;

        /* SCALING: identity only */
        if (!(pEnum->EnumPivotType == D3DKMDT_EPT_SCALING &&
              pEnum->EnumPivot.VidPnSourceId == pPath->VidPnSourceId &&
              pEnum->EnumPivot.VidPnTargetId == pPath->VidPnTargetId)) {
            if (pPath->ContentTransformation.Scaling == D3DKMDT_VPPS_UNPINNED) {
                RtlZeroMemory(&LocalPath.ContentTransformation.ScalingSupport,
                              sizeof(D3DKMDT_VIDPN_PRESENT_PATH_SCALING_SUPPORT));
                LocalPath.ContentTransformation.ScalingSupport.Identity = 1;
                SupportFieldsModified = TRUE;
            }
        }

        /* ROTATION: identity only */
        if (!(pEnum->EnumPivotType == D3DKMDT_EPT_ROTATION &&
              pEnum->EnumPivot.VidPnSourceId == pPath->VidPnSourceId &&
              pEnum->EnumPivot.VidPnTargetId == pPath->VidPnTargetId)) {
            if (pPath->ContentTransformation.Rotation == D3DKMDT_VPPR_UNPINNED) {
                LocalPath.ContentTransformation.RotationSupport.Identity = 1;
                LocalPath.ContentTransformation.RotationSupport.Rotate90 = 0;
                LocalPath.ContentTransformation.RotationSupport.Rotate180 = 0;
                LocalPath.ContentTransformation.RotationSupport.Rotate270 = 0;
                LocalPath.ContentTransformation.RotationSupport.Offset0 = 1;   /* no clone => no path-independent rotation */
                SupportFieldsModified = TRUE;
            }
        }

        if (SupportFieldsModified) {
            Status = pTopologyInterface->pfnUpdatePathSupportInfo(hTopology, &LocalPath);
            if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnUpdatePathSupportInfo failed: 0x%08X", Status); break; }
        }

        /* ---- next path; STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET ends the loop ---- */
        pPathTemp = pPath;
        Status = pTopologyInterface->pfnAcquireNextPathInfo(hTopology, pPathTemp, &pPath);
        if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquireNextPathInfo failed: 0x%08X", Status); break; }
        if (Status == STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET) {
            pPath = NULL;                                   /* do not rely on the callee clearing it */
        }

        TempStatus = pTopologyInterface->pfnReleasePathInfo(hTopology, pPathTemp);
        if (!NT_SUCCESS(TempStatus)) {
            MADISON_ERROR("pfnReleasePathInfo failed: 0x%08X", TempStatus);
            Status = TempStatus;
            break;
        }
        pPathTemp = NULL;
    }

    if (Status == STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET) {
        Status = STATUS_SUCCESS;
    }

    /* Release anything still held after an early exit. */
    if (pSourceModeSetInterface != NULL && pPinnedSourceMode != NULL) {
        (VOID)pSourceModeSetInterface->pfnReleaseModeInfo(hSourceModeSet, pPinnedSourceMode);
    }
    if (pTargetModeSetInterface != NULL && pPinnedTargetMode != NULL) {
        (VOID)pTargetModeSetInterface->pfnReleaseModeInfo(hTargetModeSet, pPinnedTargetMode);
    }
    if (pPath != NULL) {
        (VOID)pTopologyInterface->pfnReleasePathInfo(hTopology, pPath);
    }
    if (pPathTemp != NULL) {
        (VOID)pTopologyInterface->pfnReleasePathInfo(hTopology, pPathTemp);
    }
    if (hSourceModeSet != 0) {
        (VOID)pVidPnInterface->pfnReleaseSourceModeSet(pEnum->hConstrainingVidPn, hSourceModeSet);
    }
    if (hTargetModeSet != 0) {
        (VOID)pVidPnInterface->pfnReleaseTargetModeSet(pEnum->hConstrainingVidPn, hTargetModeSet);
    }
    return Status;
}

// ----------------------------------------------------------------------------
// DxgkDdiSetVidPnSourceAddress / Visibility
// ----------------------------------------------------------------------------
NTSTATUS
MadisonDdiSetVidPnSourceAddress(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_SETVIDPNSOURCEADDRESS* pSetVidPnSourceAddress
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    MADISON_CURRENT_MODE* pMode;

    if (pAdapter == NULL || pSetVidPnSourceAddress == NULL ||
        pSetVidPnSourceAddress->VidPnSourceId >= MADISON_MAX_VIEWS) {
        return STATUS_INVALID_PARAMETER;
    }

    pMode = &pAdapter->Display.CurrentModes[pSetVidPnSourceAddress->VidPnSourceId];
    pMode->PrimaryAddress = pSetVidPnSourceAddress->PrimaryAddress;

    /* The address is only recorded; MadisonDisplayProgramScanout() does not touch hardware yet,
       so the visible surface does not change. Documented limitation. */
    return MadisonDisplayProgramScanout(pAdapter, pSetVidPnSourceAddress->VidPnSourceId);
}

NTSTATUS
MadisonDdiSetVidPnSourceVisibility(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_SETVIDPNSOURCEVISIBILITY* pSetVidPnSourceVisibility
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    UINT start, end, id;

    if (pAdapter == NULL || pSetVidPnSourceVisibility == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    if (pSetVidPnSourceVisibility->VidPnSourceId >= MADISON_MAX_VIEWS &&
        pSetVidPnSourceVisibility->VidPnSourceId != D3DDDI_ID_ALL) {
        return STATUS_INVALID_PARAMETER;
    }

    start = (pSetVidPnSourceVisibility->VidPnSourceId == D3DDDI_ID_ALL) ? 0 : pSetVidPnSourceVisibility->VidPnSourceId;
    end   = (pSetVidPnSourceVisibility->VidPnSourceId == D3DDDI_ID_ALL) ? MADISON_MAX_VIEWS : pSetVidPnSourceVisibility->VidPnSourceId + 1;

    for (id = start; id < end; ++id) {
        pAdapter->Display.CurrentModes[id].Flags.SourceNotVisible = !pSetVidPnSourceVisibility->Visible;
        /* TODO: enable/disable the CRTC/GRPH layer; KMDOD blacks out its framebuffer here. */
    }
    return STATUS_SUCCESS;
}

// ----------------------------------------------------------------------------
// DxgkDdiCommitVidPn
// ----------------------------------------------------------------------------
static NTSTATUS
MadisonSetSourceModeAndPath(
    _In_ MADISON_ADAPTER*                    pAdapter,
    _In_ CONST D3DKMDT_VIDPN_SOURCE_MODE*    pSourceMode,
    _In_ CONST D3DKMDT_VIDPN_PRESENT_PATH*   pPath
)
{
    MADISON_CURRENT_MODE* pCur = &pAdapter->Display.CurrentModes[pPath->VidPnSourceId];

    pCur->Scaling       = pPath->ContentTransformation.Scaling;
    pCur->Rotation      = pPath->ContentTransformation.Rotation;
    pCur->SrcModeWidth  = pSourceMode->Format.Graphics.PrimSurfSize.cx;
    pCur->SrcModeHeight = pSourceMode->Format.Graphics.PrimSurfSize.cy;
    pCur->SrcModePitch  = pSourceMode->Format.Graphics.Stride;
    pCur->Flags.Committed = 1;

    if (pCur->SrcModeWidth != pCur->DispInfo.Width || pCur->SrcModeHeight != pCur->DispInfo.Height) {
        MADISON_WARN("Committed mode %ux%u differs from the scanout mode %ux%u (no mode-setting yet)",
                     pCur->SrcModeWidth, pCur->SrcModeHeight, pCur->DispInfo.Width, pCur->DispInfo.Height);
    }
    return MadisonDisplayProgramScanout(pAdapter, pPath->VidPnSourceId);
}

NTSTATUS
MadisonDdiCommitVidPn(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_COMMITVIDPN* CONST pCommitVidPn
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    NTSTATUS Status;
    SIZE_T NumPaths = 0;
    SIZE_T NumPathsFromSource = 0;
    SIZE_T PathIndex;
    D3DKMDT_HVIDPNTOPOLOGY hTopology = 0;
    D3DKMDT_HVIDPNSOURCEMODESET hSourceModeSet = 0;
    CONST DXGK_VIDPN_INTERFACE* pVidPnInterface = NULL;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE* pTopologyInterface = NULL;
    CONST DXGK_VIDPNSOURCEMODESET_INTERFACE* pSourceModeSetInterface = NULL;
    CONST D3DKMDT_VIDPN_PRESENT_PATH* pPath = NULL;
    CONST D3DKMDT_VIDPN_SOURCE_MODE* pPinnedSourceMode = NULL;

    if (pAdapter == NULL || pCommitVidPn == NULL || pCommitVidPn->AffectedVidPnSourceId >= MADISON_MAX_VIEWS) {
        return STATUS_INVALID_PARAMETER;
    }

    /* A mode change notification while the monitor is powered off is ignored. */
    if (pCommitVidPn->Flags.PathPoweredOff) {
        return STATUS_SUCCESS;
    }

    Status = pAdapter->DxgkInterface.DxgkCbQueryVidPnInterface(
                 pCommitVidPn->hFunctionalVidPn, DXGK_VIDPN_INTERFACE_VERSION_V1, &pVidPnInterface);
    if (!NT_SUCCESS(Status)) { MADISON_ERROR("DxgkCbQueryVidPnInterface failed: 0x%08X", Status); goto Exit; }

    Status = pVidPnInterface->pfnGetTopology(pCommitVidPn->hFunctionalVidPn, &hTopology, &pTopologyInterface);
    if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnGetTopology failed: 0x%08X", Status); goto Exit; }

    Status = pTopologyInterface->pfnGetNumPaths(hTopology, &NumPaths);
    if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnGetNumPaths failed: 0x%08X", Status); goto Exit; }

    /* Whatever was committed on this source before is replaced or cleared. */
    pAdapter->Display.CurrentModes[pCommitVidPn->AffectedVidPnSourceId].Flags.Committed = 0;

    if (NumPaths == 0) {
        Status = STATUS_SUCCESS;                  /* nothing to pin: the source was just cleared */
        goto Exit;
    }

    Status = pVidPnInterface->pfnAcquireSourceModeSet(pCommitVidPn->hFunctionalVidPn,
                 pCommitVidPn->AffectedVidPnSourceId, &hSourceModeSet, &pSourceModeSetInterface);
    if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquireSourceModeSet failed: 0x%08X", Status); goto Exit; }

    Status = pSourceModeSetInterface->pfnAcquirePinnedModeInfo(hSourceModeSet, &pPinnedSourceMode);
    if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquirePinnedModeInfo failed: 0x%08X", Status); goto Exit; }

    if (pPinnedSourceMode == NULL) {
        Status = STATUS_SUCCESS;                  /* no mode to pin on this source */
        goto Exit;
    }

    Status = MadisonIsVidPnSourceModeFieldsValid(pPinnedSourceMode);
    if (!NT_SUCCESS(Status)) { goto Exit; }

    Status = pTopologyInterface->pfnGetNumPathsFromSource(hTopology, pCommitVidPn->AffectedVidPnSourceId, &NumPathsFromSource);
    if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnGetNumPathsFromSource failed: 0x%08X", Status); goto Exit; }

    for (PathIndex = 0; PathIndex < NumPathsFromSource; ++PathIndex) {
        D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId = D3DDDI_ID_UNINITIALIZED;

        Status = pTopologyInterface->pfnEnumPathTargetsFromSource(hTopology,
                     pCommitVidPn->AffectedVidPnSourceId, (D3DDDI_VIDEO_PRESENT_TARGET_ID)PathIndex, &TargetId);
        if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnEnumPathTargetsFromSource failed: 0x%08X", Status); goto Exit; }

        Status = pTopologyInterface->pfnAcquirePathInfo(hTopology, pCommitVidPn->AffectedVidPnSourceId, TargetId, &pPath);
        if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnAcquirePathInfo failed: 0x%08X", Status); goto Exit; }

        Status = MadisonIsVidPnPathFieldsValid(pPath);
        if (!NT_SUCCESS(Status)) { goto Exit; }

        Status = MadisonSetSourceModeAndPath(pAdapter, pPinnedSourceMode, pPath);
        if (!NT_SUCCESS(Status)) { goto Exit; }

        Status = pTopologyInterface->pfnReleasePathInfo(hTopology, pPath);
        if (!NT_SUCCESS(Status)) { MADISON_ERROR("pfnReleasePathInfo failed: 0x%08X", Status); goto Exit; }
        pPath = NULL;
    }

Exit:
    if (pSourceModeSetInterface != NULL && hSourceModeSet != 0 && pPinnedSourceMode != NULL) {
        (VOID)pSourceModeSetInterface->pfnReleaseModeInfo(hSourceModeSet, pPinnedSourceMode);
    }
    if (pVidPnInterface != NULL && hSourceModeSet != 0) {
        (VOID)pVidPnInterface->pfnReleaseSourceModeSet(pCommitVidPn->hFunctionalVidPn, hSourceModeSet);
    }
    if (pTopologyInterface != NULL && hTopology != 0 && pPath != NULL) {
        (VOID)pTopologyInterface->pfnReleasePathInfo(hTopology, pPath);
    }
    return Status;
}

NTSTATUS
MadisonDdiUpdateActiveVidPnPresentPath(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH* CONST pUpdate
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    NTSTATUS Status;

    if (pAdapter == NULL || pUpdate == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = MadisonIsVidPnPathFieldsValid(&pUpdate->VidPnPresentPathInfo);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    pAdapter->Display.CurrentModes[pUpdate->VidPnPresentPathInfo.VidPnSourceId].Rotation =
        pUpdate->VidPnPresentPathInfo.ContentTransformation.Rotation;
    return STATUS_SUCCESS;
}

NTSTATUS
MadisonDdiQueryVidPnHWCapability(
    _In_    CONST HANDLE                    hAdapter,
    _Inout_ DXGKARG_QUERYVIDPNHWCAPABILITY* pVidPnHWCaps
)
{
    UNREFERENCED_PARAMETER(hAdapter);

    if (pVidPnHWCaps == NULL ||
        pVidPnHWCaps->SourceId >= MADISON_MAX_VIEWS || pVidPnHWCaps->TargetId >= MADISON_MAX_CHILDREN) {
        return STATUS_INVALID_PARAMETER;
    }

    /* Unlike KMDOD there is no software rotation or colour conversion behind these flags. */
    pVidPnHWCaps->VidPnHWCaps.DriverRotation = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverScaling = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverCloning = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverColorConvert = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverLinkedAdapaterOutput = 0;
    pVidPnHWCaps->VidPnHWCaps.DriverRemoteDisplay = 0;
    return STATUS_SUCCESS;
}
