#ifndef _MADISON_FIRMWARE_H_
#define _MADISON_FIRMWARE_H_
#include <ntddk.h>
struct _MADISON_ADAPTER;
#define MADISON_PFP_UCODE_DWORDS 1120u
#define MADISON_ME_UCODE_DWORDS  1376u
#define MADISON_RLC_UCODE_DWORDS 768u
#define MADISON_PFP_BYTES (MADISON_PFP_UCODE_DWORDS * 4u)
#define MADISON_ME_BYTES  (MADISON_ME_UCODE_DWORDS * 4u)
#define MADISON_RLC_BYTES (MADISON_RLC_UCODE_DWORDS * 4u)
NTSTATUS MadisonFirmwareLoad(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonFirmwareLoadFromDisk(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonFirmwareUploadCp(_Inout_ struct _MADISON_ADAPTER* Adapter);
NTSTATUS MadisonCpStart(_Inout_ struct _MADISON_ADAPTER* Adapter);
VOID MadisonFirmwareUnload(_Inout_ struct _MADISON_ADAPTER* Adapter);
#endif
