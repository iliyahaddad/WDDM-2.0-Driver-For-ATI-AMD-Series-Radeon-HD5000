#ifndef _MADISON_FENCE_H_
#define _MADISON_FENCE_H_
#include <ntddk.h>
struct _MADISON_ADAPTER;
typedef struct _MADISON_FENCE {
    PVOID CpuAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONGLONG GpuAddress;
    ULONG GartPageIndex;
    volatile ULONG* Value;
    ULONG LastSubmitted;
    ULONG LastCompleted;
} MADISON_FENCE;
NTSTATUS MadisonFenceInitialize(_Inout_ struct _MADISON_ADAPTER* Adapter);
VOID MadisonFenceCleanup(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonFenceEmit(_Inout_ struct _MADISON_ADAPTER* Adapter,
                          _In_ UINT FenceId,
                          _Out_ ULONG* DwordCount,
                          _Out_writes_(11) ULONG* Dwords);
NTSTATUS MadisonFenceSelfTest(_Inout_ struct _MADISON_ADAPTER* Adapter);
ULONG MadisonFenceReadCompleted(_In_ struct _MADISON_ADAPTER* Adapter);
#endif
