#ifndef _MADISON_RING_H_
#define _MADISON_RING_H_
#include <ntddk.h>
struct _MADISON_ADAPTER;
typedef struct _MADISON_RING {
    PVOID CpuAddress;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONGLONG GpuAddress;
    ULONG GartPageIndex;
    ULONG Bytes;
    ULONG Dwords;
    ULONG WritePtr;
    ULONG ReadPtr;
    BOOLEAN Ready;
    KSPIN_LOCK Lock;
} MADISON_RING;
NTSTATUS MadisonRingInitialize(_Inout_ struct _MADISON_ADAPTER* Adapter);
VOID MadisonRingCleanup(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonRingStart(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonRingSelfTest(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonRingSubmit(_Inout_ struct _MADISON_ADAPTER* Adapter,
                           _In_reads_bytes_(DmaSize) const VOID* DmaBuffer,
                           _In_ ULONG DmaSize,
                           _In_ ULONG SubmissionStart,
                           _In_ ULONG SubmissionEnd,
                           _In_ UINT SubmissionFenceId);
#endif
