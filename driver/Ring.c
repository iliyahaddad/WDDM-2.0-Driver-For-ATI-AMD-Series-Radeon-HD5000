#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "hardware.h"
#include "debug.h"

static ULONG MadisonRingFreeDwords(const MADISON_RING* Ring)
{
    ULONG used = (Ring->WritePtr >= Ring->ReadPtr)
                     ? (Ring->WritePtr - Ring->ReadPtr)
                     : (Ring->Dwords - Ring->ReadPtr + Ring->WritePtr);
    return Ring->Dwords - used - 1;
}

NTSTATUS MadisonRingInitialize(MADISON_ADAPTER* Adapter)
{
    PHYSICAL_ADDRESS low, high, boundary;
    NTSTATUS st;

    if (!Adapter || !Adapter->RegisterBaseVirtual || !Adapter->Gart.Ready || !Adapter->Gart.ApertureBase.QuadPart)
        return STATUS_DEVICE_NOT_READY;

    low.QuadPart = 0;
    boundary.QuadPart = 0;
    high.QuadPart = (LONGLONG)MADISON_MAX_PHYS_ADDR;

    Adapter->Ring = (MADISON_RING*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(MADISON_RING), MADISON_POOL_TAG);
    if (!Adapter->Ring) return STATUS_INSUFFICIENT_RESOURCES;

    Adapter->Ring->Bytes = MADISON_RING_SIZE;
    Adapter->Ring->Dwords = MADISON_RING_SIZE / 4;
    KeInitializeSpinLock(&Adapter->Ring->Lock);

    Adapter->Ring->CpuAddress = MmAllocateContiguousMemorySpecifyCache(Adapter->Ring->Bytes, low, high, boundary, MmCached);
    if (!Adapter->Ring->CpuAddress) {
        ExFreePoolWithTag(Adapter->Ring, MADISON_POOL_TAG);
        Adapter->Ring = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(Adapter->Ring->CpuAddress, Adapter->Ring->Bytes);
    Adapter->Ring->PhysicalAddress = MmGetPhysicalAddress(Adapter->Ring->CpuAddress);

    st = MadisonGartMapBuffer(Adapter, Adapter->Ring->CpuAddress, Adapter->Ring->Bytes,
                              &Adapter->Ring->GpuAddress, &Adapter->Ring->GartPageIndex);
    if (!NT_SUCCESS(st)) {
        /* Original code leaked the ring buffer and the MADISON_RING on this path. */
        MadisonRingCleanup(Adapter);
        return st;
    }
    Adapter->Ring->ReadPtr = 0;
    Adapter->Ring->WritePtr = 0;
    Adapter->Ring->Ready = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS MadisonRingStart(MADISON_ADAPTER* Adapter)
{
    if (!Adapter || !Adapter->Ring || !Adapter->Ring->GpuAddress) return STATUS_INVALID_PARAMETER;
    return MadisonCpStart(Adapter);
}

VOID MadisonRingCleanup(MADISON_ADAPTER* Adapter)
{
    if (!Adapter || !Adapter->Ring) return;
    /* Halt the CP before the memory it reads is released. */
    if (Adapter->RegisterBaseVirtual && Adapter->CpRunning) WR_REG32(Adapter, CP_ME_CNTL, CP_ME_HALT | CP_PFP_HALT);
    if (Adapter->Ring->CpuAddress)
        MmFreeContiguousMemorySpecifyCache(Adapter->Ring->CpuAddress, Adapter->Ring->Bytes, MmCached);
    ExFreePoolWithTag(Adapter->Ring, MADISON_POOL_TAG);
    Adapter->Ring = NULL;
    Adapter->CpRunning = FALSE;
}

/* Copies a DMA-buffer range into the ring followed by a fence. Prototype path only:
   a production driver would emit INDIRECT_BUFFER instead of copying the command stream. */
NTSTATUS MadisonRingSubmit(MADISON_ADAPTER* Adapter, const VOID* DmaBuffer, ULONG DmaSize,
                           ULONG SubmissionStart, ULONG SubmissionEnd, UINT SubmissionFenceId)
{
    KIRQL oldIrql;
    ULONG bytes, dwords, first, second, i, fenceCount = 0;
    ULONG fenceWords[MADISON_FENCE_EMIT_DWORDS];   /* original used [8] while Emit writes 11: stack overflow */
    const UCHAR* src;
    UCHAR* dst;
    MADISON_RING* ring;

    if (!Adapter || !Adapter->Ring || !Adapter->Ring->Ready || !Adapter->CpRunning || !DmaBuffer) return STATUS_DEVICE_NOT_READY;
    if (SubmissionStart > SubmissionEnd || SubmissionEnd > DmaSize) return STATUS_INVALID_PARAMETER;
    bytes = SubmissionEnd - SubmissionStart;
    if (!bytes || (bytes & 3) || bytes > MADISON_MAX_DMA_COPY) return STATUS_INVALID_PARAMETER;

    ring = Adapter->Ring;
    dwords = bytes / 4;
    src = (const UCHAR*)DmaBuffer + SubmissionStart;
    dst = (UCHAR*)ring->CpuAddress;

    KeAcquireSpinLock(&ring->Lock, &oldIrql);

    /* Read pointer and free-space check must happen under the lock. */
    ring->ReadPtr = REG32(Adapter, CP_RB_RPTR);
    if (dwords + MADISON_FENCE_EMIT_DWORDS + 1 > MadisonRingFreeDwords(ring)) {
        KeReleaseSpinLock(&ring->Lock, oldIrql);
        return STATUS_DEVICE_BUSY;
    }

    first = MADISON_MIN(bytes, (ring->Dwords - ring->WritePtr) * 4);
    RtlCopyMemory(dst + (SIZE_T)ring->WritePtr * 4, src, first);
    second = bytes - first;
    if (second) RtlCopyMemory(dst, src + first, second);
    ring->WritePtr = (ring->WritePtr + dwords) % ring->Dwords;

    if (!NT_SUCCESS(MadisonFenceEmit(Adapter, SubmissionFenceId, &fenceCount, fenceWords))) {
        /* Do not publish a command stream that has no completion fence. Roll back the
           producer pointer; the caller may safely retry the submission. */
        ring->WritePtr = (ring->WritePtr + ring->Dwords - dwords) % ring->Dwords;
        KeReleaseSpinLock(&ring->Lock, oldIrql);
        return STATUS_UNSUCCESSFUL;
    }
    for (i = 0; i < fenceCount; i++) {
        ((ULONG*)dst)[ring->WritePtr] = fenceWords[i];
        ring->WritePtr = (ring->WritePtr + 1) % ring->Dwords;
    }

    KeMemoryBarrier();
    WR_REG32(Adapter, CP_RB_WPTR, ring->WritePtr);
    KeReleaseSpinLock(&ring->Lock, oldIrql);
    return STATUS_SUCCESS;
}

NTSTATUS MadisonRingSelfTest(MADISON_ADAPTER* Adapter)
{
    ULONG i, wp;
    ULONG* ring;

    if (!Adapter || !Adapter->Ring || !Adapter->CpRunning) return STATUS_DEVICE_NOT_READY;

    KIRQL oldIrql;
    KeAcquireSpinLock(&Adapter->Ring->Lock, &oldIrql);

    /* Seed the scratch register with a different value so a stale 0xA55A5AA5 cannot pass. */
    WR_REG32(Adapter, SCRATCH_REG0, 0xCAFEDEADu);

    ring = (ULONG*)Adapter->Ring->CpuAddress;
    wp = Adapter->Ring->WritePtr;
    ring[wp] = PACKET0(SCRATCH_REG0, 0);   /* register offset is encoded as a DWORD index */
    ring[(wp + 1) % Adapter->Ring->Dwords] = 0xA55A5AA5u;
    KeMemoryBarrier();
    Adapter->Ring->WritePtr = (wp + 2) % Adapter->Ring->Dwords;
    WR_REG32(Adapter, CP_RB_WPTR, Adapter->Ring->WritePtr);

    for (i = 0; i < 500000; i++) {
        if (REG32(Adapter, SCRATCH_REG0) == 0xA55A5AA5u) {
            KeReleaseSpinLock(&Adapter->Ring->Lock, oldIrql);
            return STATUS_SUCCESS;
        }
        KeStallExecutionProcessor(1);
    }
    KeReleaseSpinLock(&Adapter->Ring->Lock, oldIrql);
    return STATUS_IO_TIMEOUT;
}
