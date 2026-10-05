#include "driver.h"
#include "adapter.h"
#include "debug.h"

NTSTATUS MadisonMemoryInitialize(MADISON_ADAPTER* Adapter)
{
    if (!Adapter) return STATUS_INVALID_PARAMETER;
    RtlZeroMemory(&Adapter->Memory, sizeof(Adapter->Memory));
    /* The exact VRAM aperture must be obtained from the hardware/ATOM tables; never guess it. */
    Adapter->Memory.HasFrameBuffer = FALSE;
    Adapter->Memory.HasAperture = FALSE;
    return STATUS_SUCCESS;
}

VOID MadisonMemoryCleanup(MADISON_ADAPTER* Adapter)
{
    if (Adapter) RtlZeroMemory(&Adapter->Memory, sizeof(Adapter->Memory));
}

NTSTATUS MadisonMemoryQuerySegments(MADISON_ADAPTER* Adapter, const DXGK_QUERYSEGMENTIN* Input, DXGK_QUERYSEGMENTOUT3* Query)
{
    PHYSICAL_ADDRESS apertureBase;
    SIZE_T apertureBytes;

    if (!Adapter || !Input || !Query) return STATUS_INVALID_PARAMETER;
    UNREFERENCED_PARAMETER(Adapter);

    /* Microsoft requires the first QuerySegment3 call to return only NbSegment.
       The aperture supplied in DXGK_QUERYSEGMENTIN is authoritative for this query. */
    apertureBase = Input->AgpApertureBase;
    apertureBytes = (SIZE_T)Input->AgpApertureSize.QuadPart;
    if (!apertureBase.QuadPart || !apertureBytes) {
        Query->NbSegment = 0;
        Query->PagingBufferSegmentId = 0;
        Query->PagingBufferSize = 0;
        Query->PagingBufferPrivateDataSize = 0;
        return STATUS_SUCCESS;
    }
    if ((ULONGLONG)apertureBytes > MADISON_GART_SIZE) return STATUS_NOT_SUPPORTED;

    if (Query->pSegmentDescriptor == NULL) {
        Query->NbSegment = 1;
        return STATUS_SUCCESS;
    }
    if (Query->NbSegment < 1) return STATUS_BUFFER_TOO_SMALL;

    RtlZeroMemory(Query->pSegmentDescriptor, sizeof(DXGK_SEGMENTDESCRIPTOR3));
    Query->NbSegment = 1;
    Query->PagingBufferSegmentId = 1;
    Query->PagingBufferSize = 64 * 1024;
    Query->PagingBufferPrivateDataSize = 0;

    /* This is an AGP-type aperture. Microsoft explicitly requires Agp to be the
       only DXGK_SEGMENTFLAGS bit for this segment; do not combine it with
       Aperture/CpuVisible/CacheCoherent. */
    Query->pSegmentDescriptor[0].Flags.Value = 0;
    Query->pSegmentDescriptor[0].Flags.Agp = 1;
    Query->pSegmentDescriptor[0].BaseAddress.QuadPart = 0;
    Query->pSegmentDescriptor[0].CpuTranslatedAddress.QuadPart = apertureBase.QuadPart;
    Query->pSegmentDescriptor[0].Size = apertureBytes;
    Query->pSegmentDescriptor[0].CommitLimit = apertureBytes;
    Query->pSegmentDescriptor[0].NbOfBanks = 1;
    Query->pSegmentDescriptor[0].pBankRangeTable = NULL;
    return STATUS_SUCCESS;
}
