#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "ddi.h"
#include "debug.h"
#include "memory.h"
#include "ring.h"
#include "fence.h"
#include "gart.h"
#include "firmware.h"


// ============================================================================
// DxgkDdiAddDevice
// ============================================================================
NTSTATUS
MadisonDdiAddDevice(
    _In_  IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ OUT_PPVOID              MiniportDeviceContext
)
{
    MADISON_ADAPTER* pAdapter;

    if (PhysicalDeviceObject == NULL || MiniportDeviceContext == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    *MiniportDeviceContext = NULL;

    pAdapter = (MADISON_ADAPTER*)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        sizeof(MADISON_ADAPTER),
        MADISON_POOL_TAG
    );

    if (pAdapter == NULL) {
        MADISON_ERROR("Failed to allocate adapter context");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAdapter, sizeof(*pAdapter));
    pAdapter->VendorId = MADISON_PCI_VENDOR_ID;
    pAdapter->DeviceId = MADISON_PCI_DEVICE_ID;

    *MiniportDeviceContext = pAdapter;

    MADISON_INFO("DxgkDdiAddDevice succeeded, PDO=%p", PhysicalDeviceObject);
    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiRemoveDevice
// ============================================================================
NTSTATUS
MadisonDdiRemoveDevice(
    _In_ PVOID pMiniportDeviceContext
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;

    if (pAdapter == NULL) {
        return STATUS_SUCCESS;
    }

    pAdapter->Started = FALSE;

    MadisonRingCleanup(pAdapter);
    MadisonFenceCleanup(pAdapter);
    MadisonGartCleanup(pAdapter);
    MadisonMemoryCleanup(pAdapter);
    MadisonFirmwareUnload(pAdapter);
    MadisonDisplayStop(pAdapter);

    if (pAdapter->RegisterMapped &&
        pAdapter->RegisterBaseVirtual != NULL &&
        pAdapter->DxgkInterface.DxgkCbUnmapMemory != NULL &&
        pAdapter->DeviceHandle != NULL) {
        (VOID)pAdapter->DxgkInterface.DxgkCbUnmapMemory(
            pAdapter->DeviceHandle,
            pAdapter->RegisterBaseVirtual
        );
    }

    pAdapter->RegisterMapped = FALSE;
    pAdapter->RegisterBaseVirtual = NULL;
    pAdapter->RegisterLength = 0;
    pAdapter->RegisterBase.QuadPart = 0;

    ExFreePoolWithTag(pAdapter, MADISON_POOL_TAG);
    return STATUS_SUCCESS;
}

// ============================================================================
// StartDevice rollback helper
// ============================================================================
static VOID MadisonStartDeviceRollback(_Inout_ MADISON_ADAPTER* pAdapter, _In_ BOOLEAN DisplayWasStarted)
{
    if (!pAdapter) return;
    pAdapter->Started = FALSE;
    if (DisplayWasStarted) MadisonDisplayStop(pAdapter);
    MadisonRingCleanup(pAdapter);
    MadisonFenceCleanup(pAdapter);
    MadisonFirmwareUnload(pAdapter);
    MadisonGartCleanup(pAdapter);
    MadisonMemoryCleanup(pAdapter);
    if (pAdapter->RegisterMapped && pAdapter->DxgkInterface.DxgkCbUnmapMemory && pAdapter->RegisterBaseVirtual) {
        (VOID)pAdapter->DxgkInterface.DxgkCbUnmapMemory(pAdapter->DeviceHandle, pAdapter->RegisterBaseVirtual);
    }
    pAdapter->RegisterMapped = FALSE;
    pAdapter->RegisterBaseVirtual = NULL;
    pAdapter->RegisterLength = 0;
    pAdapter->RegisterBase.QuadPart = 0;
}

// ============================================================================
// DxgkDdiStartDevice
// ============================================================================
NTSTATUS
MadisonDdiStartDevice(
    _In_    PVOID              pMiniportDeviceContext,
    _In_    PDXGK_START_INFO   pDxgkStartInfo,
    _In_    PDXGKRNL_INTERFACE pDxgkInterface,
    _Out_   PULONG             pNumberOfVideoPresentSources,
    _Out_   PULONG             pNumberOfChildren
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;
    DXGK_DEVICE_INFO deviceInfo;
    PCM_RESOURCE_LIST resourceList;
    NTSTATUS status;
    ULONG i;

    UNREFERENCED_PARAMETER(pDxgkStartInfo);

    if (pAdapter == NULL || pDxgkInterface == NULL ||
        pNumberOfVideoPresentSources == NULL || pNumberOfChildren == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(&pAdapter->DxgkInterface, pDxgkInterface, sizeof(*pDxgkInterface));
    pAdapter->DeviceHandle = pDxgkInterface->DeviceHandle;
    pAdapter->NotifyInterrupt = pDxgkInterface->DxgkCbNotifyInterrupt;

    if (pAdapter->DeviceHandle == NULL ||
        pDxgkInterface->DxgkCbGetDeviceInformation == NULL ||
        pDxgkInterface->DxgkCbMapMemory == NULL ||
        pDxgkInterface->DxgkCbUnmapMemory == NULL) {
        return STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS(MadisonMemoryInitialize(pAdapter))) {   /* zeroes pAdapter->Memory: must precede BAR bookkeeping */
        MADISON_ERROR("Memory subsystem initialization failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(&deviceInfo, sizeof(deviceInfo));
    status = pDxgkInterface->DxgkCbGetDeviceInformation(
        pAdapter->DeviceHandle, &deviceInfo);
    if (!NT_SUCCESS(status)) {
        MADISON_ERROR("DxgkCbGetDeviceInformation failed: 0x%08X", status);
        return status;
    }

    resourceList = deviceInfo.TranslatedResourceList;
    if (resourceList == NULL || resourceList->Count == 0) {
        return STATUS_DEVICE_NOT_READY;
    }

    /* Use only PnP-translated resources; never invent or probe BARs.
       The original code mapped the FIRST memory resource, which on Evergreen is BAR0 (the VRAM
       aperture, hundreds of MB), not the register BAR. The register BAR (BAR2) is the smallest
       memory resource of at least 64 KB, and the framebuffer BAR is the largest one. */
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR regDesc = NULL;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR fbDesc = NULL;
        PVOID mapped = NULL;   /* DxgkCbMapMemory takes PVOID*; RegisterBaseVirtual is PUCHAR */

        for (i = 0; i < resourceList->List[0].PartialResourceList.Count; ++i) {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR r =
                &resourceList->List[0].PartialResourceList.PartialDescriptors[i];

            if (r->Type != CmResourceTypeMemory || r->u.Memory.Length == 0) {
                continue;
            }
            if (r->u.Memory.Length >= MADISON_MIN_REGISTER_BAR &&
                r->u.Memory.Length <= MADISON_MAX_REGISTER_BAR &&
                (regDesc == NULL || r->u.Memory.Length < regDesc->u.Memory.Length)) {
                regDesc = r;
            }
            if (fbDesc == NULL || r->u.Memory.Length > fbDesc->u.Memory.Length) {
                fbDesc = r;
            }
        }

        if (regDesc == NULL) {
            MADISON_ERROR("No register-sized (64KB-2MB) MMIO resource was assigned");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        if (fbDesc != NULL && fbDesc != regDesc) {
            pAdapter->Memory.FrameBufferBase = fbDesc->u.Memory.Start;
            pAdapter->Memory.FrameBufferBytes = fbDesc->u.Memory.Length;
        }

        pAdapter->RegisterBase = regDesc->u.Memory.Start;
        pAdapter->RegisterLength = regDesc->u.Memory.Length;

        status = pDxgkInterface->DxgkCbMapMemory(
            pAdapter->DeviceHandle,
            pAdapter->RegisterBase,
            pAdapter->RegisterLength,
            FALSE,
            FALSE,
            MmNonCached,
            &mapped
        );
        pAdapter->RegisterBaseVirtual = (PUCHAR)mapped;

        if (!NT_SUCCESS(status) || pAdapter->RegisterBaseVirtual == NULL) {
            pAdapter->RegisterBaseVirtual = NULL;
            pAdapter->RegisterLength = 0;
            pAdapter->RegisterBase.QuadPart = 0;
            return NT_SUCCESS(status) ? STATUS_INSUFFICIENT_RESOURCES : status;
        }

        pAdapter->RegisterMapped = TRUE;
    }

    if (!pAdapter->RegisterMapped) {
        MADISON_ERROR("No translated MMIO resource was available");
        return STATUS_DEVICE_NOT_READY;
    }

    pAdapter->Memory.GpuApertureBase = deviceInfo.AgpApertureBase;
    pAdapter->Memory.GpuApertureBytes = deviceInfo.AgpApertureSize;
    (VOID)MadisonDetectVram(pAdapter, &pAdapter->VramBytes);
    (VOID)MadisonAtomReadAndParse(pAdapter, &pAdapter->AtomInfo);
    MADISON_INFO("Detected VRAM=%llu bytes, AGP/GART aperture base=0x%llX size=0x%llX", pAdapter->VramBytes, deviceInfo.AgpApertureBase.QuadPart, deviceInfo.AgpApertureSize);
    pAdapter->Memory.HasFrameBuffer = (pAdapter->Memory.FrameBufferBytes != 0);
    pAdapter->Memory.HasAperture = (deviceInfo.AgpApertureSize != 0);
    if (pAdapter->Memory.HasAperture && MADISON_ENABLE_GART) {
        status = MadisonGartInitialize(pAdapter, deviceInfo.AgpApertureBase, deviceInfo.AgpApertureSize);
        if (!NT_SUCCESS(status)) { MadisonStartDeviceRollback(pAdapter, FALSE); return status; }
    }
    if (pAdapter->Memory.HasAperture && MADISON_ENABLE_GART) {
        status = MadisonGartEnableHw(pAdapter);    /* L2/L1 TLB + VM context 0 + TLB flush */
        if (!NT_SUCCESS(status)) {
            MadisonStartDeviceRollback(pAdapter, FALSE);
            return status;
        }
        pAdapter->Gart.Ready = TRUE;
    }
    KeInitializeSpinLock(&pAdapter->SubmissionLock);

    pAdapter->NumberOfVideoPresentSources = MADISON_MAX_VIEWS;
    pAdapter->NumberOfChildren = MADISON_MAX_CHILDREN;
    *pNumberOfVideoPresentSources = MADISON_MAX_VIEWS;
    *pNumberOfChildren = MADISON_MAX_CHILDREN;

    status = MadisonDisplayStart(pAdapter);
    if (!NT_SUCCESS(status)) {
        /* DxgkDdiStopDevice is not guaranteed after a failed StartDevice. */
        MadisonStartDeviceRollback(pAdapter, FALSE);
        return status;
    }

    RtlZeroMemory(&pAdapter->DriverCaps, sizeof(pAdapter->DriverCaps));
    RtlZeroMemory(&pAdapter->VidSchCaps, sizeof(pAdapter->VidSchCaps));
    RtlZeroMemory(&pAdapter->VidMmCaps, sizeof(pAdapter->VidMmCaps));
    RtlZeroMemory(&pAdapter->GpuEngineTopology, sizeof(pAdapter->GpuEngineTopology));
    RtlZeroMemory(&pAdapter->PointerCaps, sizeof(pAdapter->PointerCaps));
    RtlZeroMemory(&pAdapter->PresentationCaps, sizeof(pAdapter->PresentationCaps));
    RtlZeroMemory(&pAdapter->FlipCaps, sizeof(pAdapter->FlipCaps));

    pAdapter->DriverCaps.WDDMVersion = MADISON_WDDM_VERSION;
    pAdapter->PhysicalAddressingEnabled = TRUE;
    pAdapter->PreemptionAware = FALSE;

    /* WDDM 2.x physical-addressing path: no GPUVA/MMU advertisement. */
    pAdapter->VidSchCaps.MultiEngineAware = FALSE;
    pAdapter->VidSchCaps.PreemptionAware = FALSE;
    pAdapter->VidSchCaps.NoDmaPatching = FALSE;
    pAdapter->VidSchCaps.CancelCommandAware = FALSE;

    pAdapter->GpuEngineTopology.NbAsymetricProcessingNodes = 1;

    /* Mirror the capabilities into DXGK_DRIVERCAPS where dxgkrnl consumes them. */
    pAdapter->DriverCaps.SchedulingCaps = pAdapter->VidSchCaps;
    pAdapter->DriverCaps.MemoryManagementCaps = pAdapter->VidMmCaps;
    pAdapter->DriverCaps.PointerCaps = pAdapter->PointerCaps;
    pAdapter->DriverCaps.PresentationCaps = pAdapter->PresentationCaps;
    pAdapter->DriverCaps.FlipCaps = pAdapter->FlipCaps;
    pAdapter->DriverCaps.GpuEngineTopology = pAdapter->GpuEngineTopology;

#if MADISON_ENABLE_PHASE3_CP
    if (!pAdapter->Gart.Ready || !pAdapter->Gart.ApertureBase.QuadPart) { MadisonStartDeviceRollback(pAdapter, FALSE); return STATUS_DEVICE_NOT_READY; }
    status = MadisonRingInitialize(pAdapter);
    if (!NT_SUCCESS(status)) { MadisonStartDeviceRollback(pAdapter, FALSE); return status; }
    status = MadisonFenceInitialize(pAdapter);
    if (!NT_SUCCESS(status)) { MadisonStartDeviceRollback(pAdapter, FALSE); return status; }
    status = MadisonFirmwareLoad(pAdapter);
    if (!NT_SUCCESS(status)) { MadisonStartDeviceRollback(pAdapter, FALSE); return status; }
    status = MadisonRingStart(pAdapter);
    if (!NT_SUCCESS(status)) { MadisonStartDeviceRollback(pAdapter, FALSE); return status; }
    status = MadisonRingSelfTest(pAdapter);
    if (!NT_SUCCESS(status)) { MADISON_ERROR("CP ring self-test failed: 0x%08X", status); MadisonStartDeviceRollback(pAdapter, FALSE); return status; }
    status = MadisonFenceSelfTest(pAdapter);
    if (!NT_SUCCESS(status)) { MADISON_ERROR("Fence self-test failed: 0x%08X", status); MadisonStartDeviceRollback(pAdapter, FALSE); return status; }
#endif

    pAdapter->Started = TRUE;

    MADISON_INFO("DxgkDdiStartDevice succeeded: MMIO=%p length=0x%X",
                 pAdapter->RegisterBaseVirtual, pAdapter->RegisterLength);
    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiStopDevice
// ============================================================================
NTSTATUS
MadisonDdiStopDevice(
    _In_ PVOID pMiniportDeviceContext
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;

    MADISON_INFO("DxgkDdiStopDevice called");

    if (pAdapter == NULL) {
        return STATUS_SUCCESS;
    }

    pAdapter->Started = FALSE;
    MadisonDisplayStop(pAdapter);
#if MADISON_ENABLE_PHASE3_CP
    /* Halt the CP first, then release the fence/firmware. */
    MadisonRingCleanup(pAdapter);
    MadisonFenceCleanup(pAdapter);
    MadisonFirmwareUnload(pAdapter);
#endif
    /* GART/VM is owned by StartDevice and must be torn down here as well; otherwise
       a subsequent StartDevice would overwrite the GART bookkeeping and leak the old table. */
    MadisonGartCleanup(pAdapter);
    MadisonMemoryCleanup(pAdapter);
    MADISON_INFO("DxgkDdiStopDevice completed");
    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiQueryAdapterInfo
// ============================================================================
NTSTATUS
MadisonDdiQueryAdapterInfo(
    _In_    CONST HANDLE                     hAdapter,
    _In_    CONST DXGKARG_QUERYADAPTERINFO*  pQueryAdapterInfo
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    if (pAdapter == NULL || pQueryAdapterInfo == NULL || pQueryAdapterInfo->pOutputData == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkQueryAdapterInfo");
        return STATUS_INVALID_PARAMETER;
    }

    MADISON_INFO("DxgkDdiQueryAdapterInfo called, Type=%d", pQueryAdapterInfo->Type);

    switch (pQueryAdapterInfo->Type) {
        case DXGKQAITYPE_DRIVERCAPS: {
            if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_DRIVERCAPS)) {
                MADISON_ERROR("Output buffer too small for DRIVERCAPS");
                return STATUS_BUFFER_TOO_SMALL;
            }

            DXGK_DRIVERCAPS* pCaps = (DXGK_DRIVERCAPS*)pQueryAdapterInfo->pOutputData;
            RtlCopyMemory(pCaps, &pAdapter->DriverCaps, sizeof(DXGK_DRIVERCAPS));
            MADISON_INFO("  Returned DRIVERCAPS");
            break;
        }

        case DXGKQAITYPE_VIDSCHCAPS: {
            if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_VIDSCHCAPS)) {
                MADISON_ERROR("Output buffer too small for VIDSCHCAPS");
                return STATUS_BUFFER_TOO_SMALL;
            }

            DXGK_VIDSCHCAPS* pCaps = (DXGK_VIDSCHCAPS*)pQueryAdapterInfo->pOutputData;
            RtlCopyMemory(pCaps, &pAdapter->VidSchCaps, sizeof(DXGK_VIDSCHCAPS));
            MADISON_INFO("  Returned VIDSCHCAPS: PreemptionAware=%d, MultiEngineAware=%d",
                         pCaps->PreemptionAware, pCaps->MultiEngineAware);
            break;
        }

        case DXGKQAITYPE_VIDMMCAPS: {
            if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_VIDMMCAPS)) {
                MADISON_ERROR("Output buffer too small for VIDMMCAPS");
                return STATUS_BUFFER_TOO_SMALL;
            }

            DXGK_VIDMMCAPS* pCaps = (DXGK_VIDMMCAPS*)pQueryAdapterInfo->pOutputData;
            RtlCopyMemory(pCaps, &pAdapter->VidMmCaps, sizeof(DXGK_VIDMMCAPS));
            MADISON_INFO("  Returned VIDMMCAPS: VirtualAddressingSupported=%d, GpuMmuSupported=%d, IoMmuSupported=%d",
                         pCaps->VirtualAddressingSupported, pCaps->GpuMmuSupported, pCaps->IoMmuSupported);
            break;
        }

        case DXGKQAITYPE_GPUENGINETOPOLOGY: {
            if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_GPUENGINETOPOLOGY)) {
                MADISON_ERROR("Output buffer too small for GPUENGINETOPOLOGY");
                return STATUS_BUFFER_TOO_SMALL;
            }

            DXGK_GPUENGINETOPOLOGY* pTopo = (DXGK_GPUENGINETOPOLOGY*)pQueryAdapterInfo->pOutputData;
            RtlCopyMemory(pTopo, &pAdapter->GpuEngineTopology, sizeof(DXGK_GPUENGINETOPOLOGY));
            MADISON_INFO("  Returned GPUENGINETOPOLOGY: %d processing nodes", pTopo->NbAsymetricProcessingNodes);
            break;
        }

        case DXGKQAITYPE_QUERYSEGMENT3: {
            if (pQueryAdapterInfo->OutputDataSize < sizeof(DXGK_QUERYSEGMENTOUT3)) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            return MadisonMemoryQuerySegments(pAdapter,
                (const DXGK_QUERYSEGMENTIN*)pQueryAdapterInfo->pInputData,
                (DXGK_QUERYSEGMENTOUT3*)pQueryAdapterInfo->pOutputData);
        }

        default:
            MADISON_WARN("  Unsupported query type: %d", pQueryAdapterInfo->Type);
            return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}
