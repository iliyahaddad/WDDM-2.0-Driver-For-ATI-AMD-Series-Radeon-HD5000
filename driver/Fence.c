#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "hardware.h"
#include "debug.h"

NTSTATUS MadisonFenceInitialize(MADISON_ADAPTER* Adapter)
{
    PHYSICAL_ADDRESS low, high, boundary;
    NTSTATUS st;

    if (!Adapter || !Adapter->Gart.Ready) return STATUS_DEVICE_NOT_READY;

    low.QuadPart = 0;
    boundary.QuadPart = 0;
    high.QuadPart = (LONGLONG)MADISON_MAX_PHYS_ADDR;

    /* Cached + snooped PTE (same model as Linux coherent DMA); write-combined memory
       is a poor choice for a page the CPU polls. */
    Adapter->Fence.CpuAddress = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, low, high, boundary, MmCached);
    if (!Adapter->Fence.CpuAddress) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(Adapter->Fence.CpuAddress, PAGE_SIZE);
    Adapter->Fence.PhysicalAddress = MmGetPhysicalAddress(Adapter->Fence.CpuAddress);

    st = MadisonGartMapBuffer(Adapter, Adapter->Fence.CpuAddress, PAGE_SIZE,
                              &Adapter->Fence.GpuAddress, &Adapter->Fence.GartPageIndex);
    if (!NT_SUCCESS(st)) {
        MadisonFenceCleanup(Adapter);
        return st;
    }

    Adapter->Fence.Value = (volatile ULONG*)Adapter->Fence.CpuAddress;
    Adapter->Fence.LastSubmitted = 0;
    Adapter->Fence.LastCompleted = 0;
    return STATUS_SUCCESS;
}

VOID MadisonFenceCleanup(MADISON_ADAPTER* Adapter)
{
    if (!Adapter || !Adapter->Fence.CpuAddress) return;
    MmFreeContiguousMemorySpecifyCache(Adapter->Fence.CpuAddress, PAGE_SIZE, MmCached);
    RtlZeroMemory(&Adapter->Fence, sizeof(Adapter->Fence));
}

/* Emits exactly MADISON_FENCE_EMIT_DWORDS (11) DWORDs:
     SURFACE_SYNC (1 header + 4 payload) + EVENT_WRITE_EOP (1 header + 5 payload). */
NTSTATUS MadisonFenceEmit(MADISON_ADAPTER* Adapter, UINT FenceId, ULONG* DwordCount, ULONG* Dwords)
{
    ULONGLONG a;
    if (!Adapter || !DwordCount || !Dwords || !Adapter->Fence.Value || !Adapter->Fence.GpuAddress) return STATUS_INVALID_PARAMETER;
    a = Adapter->Fence.GpuAddress;

    Dwords[0]  = PACKET3(PACKET3_SURFACE_SYNC, 3);
    Dwords[1]  = PACKET3_TC_ACTION_ENA | PACKET3_VC_ACTION_ENA | PACKET3_SH_ACTION_ENA;
    Dwords[2]  = 0xFFFFFFFFu;     /* CP_COHER_SIZE */
    Dwords[3]  = 0;               /* CP_COHER_BASE */
    Dwords[4]  = 10;              /* poll interval */

    Dwords[5]  = PACKET3(PACKET3_EVENT_WRITE_EOP, 4);
    Dwords[6]  = EVENT_TYPE_CACHE_FLUSH_AND_INV_EVENT_TS | EVENT_INDEX_5;
    Dwords[7]  = (ULONG)(a & 0xFFFFFFFCu);
    Dwords[8]  = (ULONG)((a >> 32) & 0xFFu) | DATA_SEL_1 | INT_SEL_2;
    Dwords[9]  = FenceId;
    Dwords[10] = 0;

    *DwordCount = MADISON_FENCE_EMIT_DWORDS;
    Adapter->Fence.LastSubmitted = FenceId;
    return STATUS_SUCCESS;
}

ULONG MadisonFenceReadCompleted(MADISON_ADAPTER* Adapter)
{
    ULONG v;
    if (!Adapter || !Adapter->Fence.Value) return 0;
    v = *Adapter->Fence.Value;
    Adapter->Fence.LastCompleted = v;
    return v;
}

NTSTATUS MadisonFenceSelfTest(MADISON_ADAPTER* Adapter)
{
    ULONG words[MADISON_FENCE_EMIT_DWORDS];
    ULONG count = 0, i, wp;
    ULONG* ring;

    if (!Adapter || !Adapter->Ring || !Adapter->CpRunning || !Adapter->Fence.Value) return STATUS_DEVICE_NOT_READY;

    KIRQL oldIrql;
    KeAcquireSpinLock(&Adapter->Ring->Lock, &oldIrql);

    *(volatile ULONG*)Adapter->Fence.CpuAddress = 0;
    if (!NT_SUCCESS(MadisonFenceEmit(Adapter, 0x13572468u, &count, words))) {
        KeReleaseSpinLock(&Adapter->Ring->Lock, oldIrql);
        return STATUS_UNSUCCESSFUL;
    }

    ring = (ULONG*)Adapter->Ring->CpuAddress;
    wp = Adapter->Ring->WritePtr;
    for (i = 0; i < count; ++i) {                          /* word-by-word so the copy wraps correctly */
        ring[wp] = words[i];
        wp = (wp + 1) % Adapter->Ring->Dwords;
    }
    KeMemoryBarrier();
    Adapter->Ring->WritePtr = wp;
    WR_REG32(Adapter, CP_RB_WPTR, wp);

    for (i = 0; i < 500000; i++) {
        if (MadisonFenceReadCompleted(Adapter) == 0x13572468u) {
            KeReleaseSpinLock(&Adapter->Ring->Lock, oldIrql);
            return STATUS_SUCCESS;
        }
        KeStallExecutionProcessor(1);
    }
    KeReleaseSpinLock(&Adapter->Ring->Lock, oldIrql);
    return STATUS_IO_TIMEOUT;
}
