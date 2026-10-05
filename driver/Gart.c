#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "hardware.h"
#include "debug.h"

/* Evergreen system-memory PTE flags (Linux radeon: R600_PTE_* / RADEON_GART_PAGE_*).
   NOTE: READ/WRITE are bits 5/6 - the original tree used bits 3/4, which are not permission bits. */
#define MADISON_PTE_VALID       (1ull << 0)
#define MADISON_PTE_SYSTEM      (1ull << 1)
#define MADISON_PTE_SNOOPED     (1ull << 2)
#define MADISON_PTE_READABLE    (1ull << 5)
#define MADISON_PTE_WRITEABLE   (1ull << 6)

static VOID MadisonGartFreeAll(MADISON_ADAPTER* Adapter)
{
    if (Adapter->Gart.TableCpuAddress)
        MmFreeContiguousMemorySpecifyCache(Adapter->Gart.TableCpuAddress, Adapter->Gart.TableBytes, MmNonCached);
    if (Adapter->Gart.DummyPageCpuAddress)
        MmFreeContiguousMemorySpecifyCache(Adapter->Gart.DummyPageCpuAddress, PAGE_SIZE, MmNonCached);
    RtlZeroMemory(&Adapter->Gart, sizeof(Adapter->Gart));
}

NTSTATUS MadisonGartInitialize(MADISON_ADAPTER* Adapter, PHYSICAL_ADDRESS ApertureBase, SIZE_T ApertureBytes)
{
    SIZE_T tableBytes;
    ULONG i;
    ULONGLONG dummyPte;
    PHYSICAL_ADDRESS low, high, boundary;

    if (!Adapter || ApertureBytes < MADISON_GART_PAGE_SIZE) return STATUS_INVALID_PARAMETER;
    /* Never silently truncate the OS-provided aperture: QuerySegment must describe
       exactly the range that the GART can actually map. */
    if ((ULONGLONG)ApertureBytes > MADISON_GART_SIZE) return STATUS_NOT_SUPPORTED;

    low.QuadPart = 0;
    boundary.QuadPart = 0;
    high.QuadPart = (LONGLONG)MADISON_MAX_PHYS_ADDR;

    tableBytes = ((ApertureBytes + MADISON_GART_PAGE_SIZE - 1) / MADISON_GART_PAGE_SIZE) * sizeof(ULONGLONG);
    tableBytes = (tableBytes + PAGE_SIZE - 1) & ~((SIZE_T)PAGE_SIZE - 1);

    RtlZeroMemory(&Adapter->Gart, sizeof(Adapter->Gart));

    Adapter->Gart.DummyPageCpuAddress = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, low, high, boundary, MmNonCached);
    if (!Adapter->Gart.DummyPageCpuAddress) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(Adapter->Gart.DummyPageCpuAddress, PAGE_SIZE);
    Adapter->Gart.DummyPagePhysicalAddress = MmGetPhysicalAddress(Adapter->Gart.DummyPageCpuAddress);

    Adapter->Gart.TableCpuAddress = MmAllocateContiguousMemorySpecifyCache(tableBytes, low, high, boundary, MmNonCached);
    if (!Adapter->Gart.TableCpuAddress) {
        MadisonGartFreeAll(Adapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Adapter->Gart.TablePhysicalAddress = MmGetPhysicalAddress(Adapter->Gart.TableCpuAddress);
    Adapter->Gart.TableBytes = tableBytes;
    Adapter->Gart.ApertureBase = ApertureBase;
    Adapter->Gart.ApertureBytes = ApertureBytes;
    Adapter->Gart.PageCount = (ULONG)(ApertureBytes / MADISON_GART_PAGE_SIZE);
    Adapter->Gart.NextFreePage = 0;
    Adapter->Gart.Ready = FALSE;

    /* Point every PTE at the dummy page so stray GPU accesses never hit random memory. */
    RtlZeroMemory(Adapter->Gart.TableCpuAddress, tableBytes);
    dummyPte = ((ULONGLONG)Adapter->Gart.DummyPagePhysicalAddress.QuadPart & ~0xFFFULL) |
               MADISON_PTE_VALID | MADISON_PTE_SYSTEM | MADISON_PTE_SNOOPED |
               MADISON_PTE_READABLE | MADISON_PTE_WRITEABLE;
    for (i = 0; i < Adapter->Gart.PageCount; ++i) ((ULONGLONG*)Adapter->Gart.TableCpuAddress)[i] = dummyPte;
    KeMemoryBarrier();

    MADISON_INFO("GART table allocated: PA=0x%llX bytes=0x%llX pages=%u",
                 Adapter->Gart.TablePhysicalAddress.QuadPart, (ULONGLONG)tableBytes, Adapter->Gart.PageCount);
    return STATUS_SUCCESS;
}

/* Program the Evergreen L2/L1 TLBs and VM context 0 (mirrors Linux evergreen_pcie_gart_enable). */
NTSTATUS MadisonGartEnableHw(MADISON_ADAPTER* Adapter)
{
    ULONG tlb;
    ULONGLONG start, end;

    if (!Adapter || !Adapter->RegisterBaseVirtual || !Adapter->Gart.TableCpuAddress) return STATUS_INVALID_PARAMETER;

    start = (ULONGLONG)Adapter->Gart.ApertureBase.QuadPart;
    end   = start + Adapter->Gart.ApertureBytes - 1;

    WR_REG32(Adapter, VM_L2_CNTL, ENABLE_L2_CACHE | ENABLE_L2_FRAGMENT_PROCESSING |
                                  ENABLE_L2_PTE_CACHE_LRU_UPDATE_BY_WRITE | EFFECTIVE_L2_QUEUE_SIZE(7));
    WR_REG32(Adapter, VM_L2_CNTL2, 0);
    WR_REG32(Adapter, VM_L2_CNTL3, BANK_SELECT(0) | CACHE_UPDATE_MODE(2));

    tlb = ENABLE_L1_TLB | ENABLE_L1_FRAGMENT_PROCESSING | SYSTEM_ACCESS_MODE_NOT_IN_SYS |
          SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU | EFFECTIVE_L1_TLB_SIZE(5) | EFFECTIVE_L1_QUEUE_SIZE(5);
    WR_REG32(Adapter, MC_VM_MD_L1_TLB0_CNTL, tlb);
    WR_REG32(Adapter, MC_VM_MD_L1_TLB1_CNTL, tlb);
    WR_REG32(Adapter, MC_VM_MD_L1_TLB2_CNTL, tlb);
    WR_REG32(Adapter, MC_VM_MD_L1_TLB3_CNTL, tlb);   /* Juniper/Cypress/Hemlock/Barts only */
    WR_REG32(Adapter, MC_VM_MB_L1_TLB0_CNTL, tlb);
    WR_REG32(Adapter, MC_VM_MB_L1_TLB1_CNTL, tlb);
    WR_REG32(Adapter, MC_VM_MB_L1_TLB2_CNTL, tlb);
    WR_REG32(Adapter, MC_VM_MB_L1_TLB3_CNTL, tlb);

    WR_REG32(Adapter, VM_CONTEXT0_PAGE_TABLE_START_ADDR, (ULONG)(start >> 12));
    WR_REG32(Adapter, VM_CONTEXT0_PAGE_TABLE_END_ADDR,   (ULONG)(end >> 12));
    WR_REG32(Adapter, VM_CONTEXT0_PAGE_TABLE_BASE_ADDR,  (ULONG)((ULONGLONG)Adapter->Gart.TablePhysicalAddress.QuadPart >> 12));
    WR_REG32(Adapter, VM_CONTEXT0_PROTECTION_FAULT_DEFAULT_ADDR,
             (ULONG)((ULONGLONG)Adapter->Gart.DummyPagePhysicalAddress.QuadPart >> 12));
    WR_REG32(Adapter, VM_CONTEXT0_CNTL, VM_CONTEXT0_CNTL__ENABLE_CONTEXT_MASK | PAGE_TABLE_DEPTH(0) |
                                        RANGE_PROTECTION_FAULT_ENABLE_DEFAULT);
    WR_REG32(Adapter, VM_CONTEXT1_CNTL, 0);

    return MadisonGartFlush(Adapter);
}

VOID MadisonGartDisableHw(MADISON_ADAPTER* Adapter)
{
    if (!Adapter || !Adapter->RegisterBaseVirtual) return;
    WR_REG32(Adapter, VM_CONTEXT0_CNTL, 0);
    WR_REG32(Adapter, VM_CONTEXT1_CNTL, 0);
    WR_REG32(Adapter, VM_L2_CNTL, 0);
}

VOID MadisonGartCleanup(MADISON_ADAPTER* Adapter)
{
    if (!Adapter || !Adapter->Gart.TableCpuAddress) return;
    /* The GPU must stop walking the table before it is freed. */
    MadisonGartDisableHw(Adapter);
    MadisonGartFreeAll(Adapter);
}

NTSTATUS MadisonGartMapPhysicalPage(MADISON_ADAPTER* Adapter, ULONG PageIndex, PHYSICAL_ADDRESS PhysicalAddress)
{
    ULONGLONG* table;
    if (!Adapter || !Adapter->Gart.TableCpuAddress || PageIndex >= Adapter->Gart.PageCount) return STATUS_INVALID_PARAMETER;
    if ((ULONGLONG)PhysicalAddress.QuadPart & (MADISON_GART_PAGE_SIZE - 1)) return STATUS_DATATYPE_MISALIGNMENT;
    if ((ULONGLONG)PhysicalAddress.QuadPart > MADISON_MAX_PHYS_ADDR) return STATUS_INVALID_ADDRESS;
    table = (ULONGLONG*)Adapter->Gart.TableCpuAddress;
    table[PageIndex] = ((ULONGLONG)PhysicalAddress.QuadPart & ~0xFFFULL) |
                       MADISON_PTE_VALID | MADISON_PTE_SYSTEM | MADISON_PTE_SNOOPED |
                       MADISON_PTE_READABLE | MADISON_PTE_WRITEABLE;
    KeMemoryBarrier();
    return STATUS_SUCCESS;
}

NTSTATUS MadisonGartFlush(MADISON_ADAPTER* Adapter)
{
    ULONG i;
    ULONG response;
    if (!Adapter || !Adapter->RegisterBaseVirtual) return STATUS_INVALID_PARAMETER;

    /* Linux evergreen_pcie_gart_tlb_flush: HDP flush, then request a TLB invalidate. */
    WR_REG32(Adapter, HDP_MEM_COHERENCY_FLUSH_CNTL, 0x1);
    WR_REG32(Adapter, VM_CONTEXT0_REQUEST_RESPONSE, REQUEST_TYPE(1));
    for (i = 0; i < 1000; ++i) {
        response = (REG32(Adapter, VM_CONTEXT0_REQUEST_RESPONSE) & RESPONSE_TYPE_MASK) >> RESPONSE_TYPE_SHIFT;
        if (response == 2) {
            MADISON_ERROR("GART TLB flush request failed");
            return STATUS_UNSUCCESSFUL;
        }
        if (response != 0) return STATUS_SUCCESS;
        KeStallExecutionProcessor(1);
    }
    return STATUS_IO_TIMEOUT;
}

/* Maps a physically-contiguous, page-aligned kernel buffer at the next free GART pages. */
NTSTATUS MadisonGartMapBuffer(MADISON_ADAPTER* Adapter, PVOID CpuAddress, SIZE_T Bytes, ULONGLONG* GpuAddress, PULONG FirstPage)
{
    ULONG pages, i, first;
    PHYSICAL_ADDRESS pa;

    if (!Adapter || !CpuAddress || !Bytes || !GpuAddress || !FirstPage || !Adapter->Gart.Ready) return STATUS_INVALID_PARAMETER;
    pages = (ULONG)((Bytes + PAGE_SIZE - 1) / PAGE_SIZE);
    first = Adapter->Gart.NextFreePage;
    if (pages > Adapter->Gart.PageCount || first > Adapter->Gart.PageCount - pages) return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < pages; ++i) {
        pa = MmGetPhysicalAddress((PUCHAR)CpuAddress + ((SIZE_T)i * PAGE_SIZE));
        if (!pa.QuadPart || !NT_SUCCESS(MadisonGartMapPhysicalPage(Adapter, first + i, pa))) return STATUS_INSUFFICIENT_RESOURCES;
    }
    Adapter->Gart.NextFreePage = first + pages;
    *FirstPage = first;
    *GpuAddress = (ULONGLONG)Adapter->Gart.ApertureBase.QuadPart + ((ULONGLONG)first * PAGE_SIZE);
    return MadisonGartFlush(Adapter);
}
