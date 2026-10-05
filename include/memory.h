#ifndef _MADISON_MEMORY_H_
#define _MADISON_MEMORY_H_
#include <ntddk.h>
#include <d3dkmddi.h>
struct _MADISON_ADAPTER;
typedef struct _MADISON_MEMORY {
    PHYSICAL_ADDRESS FrameBufferBase;
    SIZE_T FrameBufferBytes;
    PHYSICAL_ADDRESS GpuApertureBase;
    SIZE_T GpuApertureBytes;
    BOOLEAN HasFrameBuffer;
    BOOLEAN HasAperture;
} MADISON_MEMORY;
NTSTATUS MadisonMemoryInitialize(_Inout_ struct _MADISON_ADAPTER* Adapter);
VOID MadisonMemoryCleanup(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonMemoryQuerySegments(_Inout_ struct _MADISON_ADAPTER* Adapter,
                                    _In_ const DXGK_QUERYSEGMENTIN* Input,
                                    _Inout_ DXGK_QUERYSEGMENTOUT3* Query);
#endif
