#ifndef _MADISON_GART_H_
#define _MADISON_GART_H_
#include <ntddk.h>
struct _MADISON_ADAPTER;
typedef struct _MADISON_GART {
    PVOID TableCpuAddress;
    PHYSICAL_ADDRESS TablePhysicalAddress;
    SIZE_T TableBytes;
    PHYSICAL_ADDRESS ApertureBase;
    SIZE_T ApertureBytes;
    ULONG PageCount;
    ULONG NextFreePage;
    PVOID DummyPageCpuAddress;
    PHYSICAL_ADDRESS DummyPagePhysicalAddress;
    BOOLEAN Ready;
} MADISON_GART;
NTSTATUS MadisonGartInitialize(_Inout_ struct _MADISON_ADAPTER* Adapter,
                               _In_ PHYSICAL_ADDRESS ApertureBase,
                               _In_ SIZE_T ApertureBytes);
VOID MadisonGartCleanup(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonGartMapPhysicalPage(_Inout_ struct _MADISON_ADAPTER* Adapter,
                                     _In_ ULONG PageIndex,
                                     _In_ PHYSICAL_ADDRESS PhysicalAddress);
NTSTATUS MadisonGartFlush(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonGartEnableHw(_Inout_ struct _MADISON_ADAPTER* Adapter);
VOID MadisonGartDisableHw(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonGartMapBuffer(_Inout_ struct _MADISON_ADAPTER* Adapter, _In_ PVOID CpuAddress, _In_ SIZE_T Bytes, _Out_ ULONGLONG* GpuAddress, _Out_ PULONG FirstPage);
#endif
