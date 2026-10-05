#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "hardware.h"
#include "ddi.h"
#include "debug.h"


// ============================================================================
// DxgkDdiPatch
// ============================================================================
NTSTATUS
MadisonDdiPatch(
    _In_ CONST HANDLE            hAdapter,
    _In_ CONST DXGKARG_PATCH*    pPatch
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    const D3DDDI_PATCHLOCATIONLIST* patchList;
    ULONG first;
    ULONG count;
    ULONG i;

    if (pAdapter == NULL || pPatch == NULL || pPatch->pDmaBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    /* Paging buffers have no allocation/patch lists. */
    if (pPatch->pAllocationList == NULL || pPatch->AllocationListSize == 0 ||
        pPatch->pPatchLocationList == NULL || pPatch->PatchLocationListSize == 0) {
        return STATUS_SUCCESS;
    }

    if (pPatch->DmaBufferSubmissionStartOffset > pPatch->DmaBufferSubmissionEndOffset ||
        pPatch->DmaBufferSubmissionEndOffset > pPatch->DmaBufferSize) {
        return STATUS_INVALID_PARAMETER;
    }

    first = pPatch->PatchLocationListSubmissionStart;
    count = pPatch->PatchLocationListSubmissionLength;
    if (first > pPatch->PatchLocationListSize ||
        count > (pPatch->PatchLocationListSize - first)) {
        return STATUS_INVALID_PARAMETER;
    }

    patchList = pPatch->pPatchLocationList + first;

    for (i = 0; i < count; ++i) {
        const D3DDDI_PATCHLOCATIONLIST* loc = &patchList[i];
        const DXGK_ALLOCATIONLIST* allocation;
        ULONGLONG address;
        ULONG offset;

        if (loc->AllocationIndex >= pPatch->AllocationListSize) {
            return STATUS_INVALID_PARAMETER;
        }

        /* PatchOffset and AllocationOffset are BYTE offsets, not bit offsets. */
        offset = loc->PatchOffset;
        if (pPatch->DmaBufferSize < sizeof(ULONG) ||
            offset < pPatch->DmaBufferSubmissionStartOffset ||
            offset > pPatch->DmaBufferSubmissionEndOffset - sizeof(ULONG) ||
            pPatch->DmaBufferSubmissionEndOffset < sizeof(ULONG) ||
            offset > pPatch->DmaBufferSize - sizeof(ULONG)) {
            return STATUS_INVALID_PARAMETER;
        }

        allocation = &pPatch->pAllocationList[loc->AllocationIndex];
        if (allocation->PhysicalAddress.QuadPart == 0) {
            return STATUS_INVALID_PARAMETER;
        }

        address = (ULONGLONG)allocation->PhysicalAddress.QuadPart +
                  (ULONGLONG)loc->AllocationOffset;

        /* The current Evergreen prototype emits a 32-bit physical address field.
           Refuse addresses above 4 GiB rather than silently truncating them. */
        if ((address >> 32) != 0) {
            MADISON_ERROR("Patch[%u]: physical address exceeds 32-bit PM4 field: 0x%llX",
                          i, address);
            return STATUS_NOT_SUPPORTED;
        }

        *(UNALIGNED ULONG*)((PUCHAR)pPatch->pDmaBuffer + offset) =
            (ULONG)address;
    }

    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiSubmitCommand
// ============================================================================
NTSTATUS
MadisonDdiSubmitCommand(
    _In_ CONST HANDLE hAdapter,
    _In_ CONST DXGKARG_SUBMITCOMMAND* pSubmitCommand
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    if (!pAdapter || !pSubmitCommand) return STATUS_INVALID_PARAMETER;
#if MADISON_ENABLE_HW_EXECUTION
    if (!pAdapter->Ring || !pAdapter->Ring->Ready) return STATUS_DEVICE_NOT_READY;
    return MadisonRingSubmit(pAdapter,
                             pSubmitCommand->pDmaBuffer,
                             pSubmitCommand->DmaBufferSize,
                             pSubmitCommand->DmaBufferSubmissionStartOffset,
                             pSubmitCommand->DmaBufferSubmissionEndOffset,
                             pSubmitCommand->SubmissionFenceId);
#else
    /* Phase 2B default: build/validate the hardware path but do not start an unverified GPU. */
    UNREFERENCED_PARAMETER(pSubmitCommand);
    return STATUS_DEVICE_NOT_READY;
#endif
}

// ============================================================================
// DxgkDdiBuildPagingBuffer
// ============================================================================
NTSTATUS
MadisonDdiBuildPagingBuffer(
    _In_ CONST HANDLE hAdapter,
    _Inout_ DXGKARG_BUILDPAGINGBUFFER* pBuildPagingBuffer
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    SIZE_T i;
    if (!pAdapter || !pBuildPagingBuffer) return STATUS_INVALID_PARAMETER;

    switch (pBuildPagingBuffer->Operation) {
        case DXGK_OPERATION_MAP_APERTURE_SEGMENT: {
            PMDL mdl = pBuildPagingBuffer->MapApertureSegment.pMdl;
            PFN_NUMBER* pfns;
            if (!MADISON_ENABLE_GART || !pAdapter->Gart.TableCpuAddress || !mdl) return STATUS_NOT_SUPPORTED;
            if (pBuildPagingBuffer->MapApertureSegment.SegmentId != 1) return STATUS_INVALID_PARAMETER;
            if (pBuildPagingBuffer->MapApertureSegment.OffsetInPages + pBuildPagingBuffer->MapApertureSegment.NumberOfPages > pAdapter->Gart.PageCount) return STATUS_INVALID_PARAMETER;
            pfns = MmGetMdlPfnArray(mdl);
            if (!pfns) return STATUS_INVALID_PARAMETER;
            for (i = 0; i < pBuildPagingBuffer->MapApertureSegment.NumberOfPages; ++i) {
                PHYSICAL_ADDRESS pa;
                pa.QuadPart = ((ULONGLONG)pfns[pBuildPagingBuffer->MapApertureSegment.MdlOffset + i]) << PAGE_SHIFT;
                if (!NT_SUCCESS(MadisonGartMapPhysicalPage(pAdapter, (ULONG)(pBuildPagingBuffer->MapApertureSegment.OffsetInPages + i), pa))) return STATUS_INVALID_PARAMETER;
            }
            return MadisonGartFlush(pAdapter);
        }
        case DXGK_OPERATION_UNMAP_APERTURE_SEGMENT: {
            if (!MADISON_ENABLE_GART || !pAdapter->Gart.TableCpuAddress) return STATUS_NOT_SUPPORTED;
            if (pBuildPagingBuffer->UnmapApertureSegment.SegmentId != 1) return STATUS_INVALID_PARAMETER;
            if (pBuildPagingBuffer->UnmapApertureSegment.OffsetInPages + pBuildPagingBuffer->UnmapApertureSegment.NumberOfPages > pAdapter->Gart.PageCount) return STATUS_INVALID_PARAMETER;
            for (i = 0; i < pBuildPagingBuffer->UnmapApertureSegment.NumberOfPages; ++i) {
                if (!NT_SUCCESS(MadisonGartMapPhysicalPage(pAdapter, (ULONG)(pBuildPagingBuffer->UnmapApertureSegment.OffsetInPages + i), pAdapter->Gart.DummyPagePhysicalAddress))) return STATUS_INVALID_PARAMETER;
            }
            return MadisonGartFlush(pAdapter);
        }
        default:
            return STATUS_NOT_SUPPORTED;
    }
}

// ============================================================================
// DxgkDdiQueryCurrentFence
// ============================================================================
NTSTATUS
MadisonDdiQueryCurrentFence(
    _In_ CONST HANDLE hAdapter,
    _Inout_ DXGKARG_QUERYCURRENTFENCE* pCurrentFence
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;
    if (!pAdapter || !pCurrentFence) return STATUS_INVALID_PARAMETER;
    if (pCurrentFence->NodeOrdinal != 0 || pCurrentFence->EngineOrdinal != 0) return STATUS_INVALID_PARAMETER;
    pCurrentFence->CurrentFence = MadisonFenceReadCompleted(pAdapter);
    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiResetEngine
// ============================================================================
NTSTATUS
MadisonDdiResetEngine(
    _In_    CONST HANDLE         hAdapter,
    _Inout_ DXGKARG_RESETENGINE* pResetEngine
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    if (pAdapter == NULL || pResetEngine == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiResetEngine");
        return STATUS_INVALID_PARAMETER;
    }

    MADISON_RESET("DxgkDdiResetEngine called");

    /* Hardware reset sequencing is not implemented yet; never report a reset that did not happen. */
    return STATUS_NOT_SUPPORTED;
}

// ============================================================================
// DxgkDdiPreemptCommand
// ============================================================================
NTSTATUS
MadisonDdiPreemptCommand(
    _In_ CONST HANDLE                    hAdapter,
    _In_ CONST DXGKARG_PREEMPTCOMMAND*   pPreemptCommand
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    if (pAdapter == NULL || pPreemptCommand == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    /* PreemptionAware is deliberately zero for this hardware prototype.
       Never fabricate a preemption-complete notification. */
    return STATUS_NOT_SUPPORTED;
}

// ============================================================================
// DxgkDdiGetNodeMetadata
// ============================================================================
NTSTATUS
MadisonDdiGetNodeMetadata(
    _In_  CONST HANDLE               hAdapter,
    _In_  UINT                       NodeOrdinalAndAdapterIndex,
    _Out_ DXGKARG_GETNODEMETADATA*   pGetNodeMetadata
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    if (pAdapter == NULL || pGetNodeMetadata == NULL ||
        NodeOrdinalAndAdapterIndex >= MADISON_MAX_NODES) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pGetNodeMetadata, sizeof(*pGetNodeMetadata));
    pGetNodeMetadata->EngineType = DXGK_ENGINE_TYPE_3D;
    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiCreateContext
// ============================================================================
NTSTATUS
MadisonDdiCreateContext(
    _In_    CONST HANDLE            hDevice,
    _Inout_ DXGKARG_CREATECONTEXT*  pCreateContext
)
{
    MADISON_DEVICE* pDevice = (MADISON_DEVICE*)hDevice;
    MADISON_CONTEXT* pContext;

    if (pDevice == NULL || pCreateContext == NULL || pDevice->Adapter == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (pCreateContext->NodeOrdinal != 0 || pCreateContext->EngineAffinity != 1) {
        return STATUS_INVALID_PARAMETER;
    }

    pContext = (MADISON_CONTEXT*)ExAllocatePool2(
        POOL_FLAG_NON_PAGED, sizeof(*pContext), MADISON_POOL_TAG);
    if (pContext == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pContext, sizeof(*pContext));
    pContext->Adapter = pDevice->Adapter;
    pContext->Device = pDevice;
    pContext->NodeOrdinal = pCreateContext->NodeOrdinal;
    pContext->EngineAffinity = pCreateContext->EngineAffinity;
    pContext->Created = TRUE;

    RtlZeroMemory(&pCreateContext->ContextInfo, sizeof(pCreateContext->ContextInfo));
    pCreateContext->ContextInfo.DmaBufferSize = MADISON_RING_SIZE;
    pCreateContext->ContextInfo.DmaBufferSegmentSet = 0;
    pCreateContext->ContextInfo.DmaBufferPrivateDataSize = 0;
    pCreateContext->ContextInfo.AllocationListSize = 16;
    pCreateContext->ContextInfo.PatchLocationListSize = 16;
    pCreateContext->hContext = (HANDLE)pContext;

    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiDestroyContext
// ============================================================================
NTSTATUS
MadisonDdiDestroyContext(
    _In_ CONST HANDLE hContext
)
{
    MADISON_CONTEXT* pContext = (MADISON_CONTEXT*)hContext;

    if (pContext == NULL) {
        return STATUS_SUCCESS;
    }

    ExFreePoolWithTag(pContext, MADISON_POOL_TAG);
    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiCreateDevice
// ============================================================================
NTSTATUS
MadisonDdiCreateDevice(
    _In_    CONST HANDLE           hAdapter,
    _Inout_ DXGKARG_CREATEDEVICE*  pCreateDevice
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    MADISON_INFO("DxgkDdiCreateDevice called");

    if (pAdapter == NULL || pCreateDevice == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiCreateDevice");
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate device context
    MADISON_DEVICE* pDevice = (MADISON_DEVICE*)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        sizeof(MADISON_DEVICE),
        MADISON_POOL_TAG
    );

    if (pDevice == NULL) {
        MADISON_ERROR("Failed to allocate device context");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pDevice, sizeof(MADISON_DEVICE));
    pDevice->Adapter = pAdapter;
    pDevice->hDevice = pCreateDevice->hDevice;
    pDevice->EngineOrdinal = 0;
    pDevice->Created = TRUE;

    /* hDevice is an in/out field: return our driver-owned device handle. */
    pCreateDevice->hDevice = (HANDLE)pDevice;

    MADISON_INFO("DxgkDdiCreateDevice completed");
    return STATUS_SUCCESS;
}

// ============================================================================
// DxgkDdiDestroyDevice
// ============================================================================
NTSTATUS
MadisonDdiDestroyDevice(
    _In_ CONST HANDLE   hDevice
)
{
    MADISON_DEVICE* pDevice = (MADISON_DEVICE*)hDevice;

    MADISON_INFO("DxgkDdiDestroyDevice called");

    if (pDevice == NULL) {
        return STATUS_SUCCESS;
    }

    pDevice->Created = FALSE;
    ExFreePoolWithTag(pDevice, MADISON_POOL_TAG);

    MADISON_INFO("DxgkDdiDestroyDevice completed");
    return STATUS_SUCCESS;
}
