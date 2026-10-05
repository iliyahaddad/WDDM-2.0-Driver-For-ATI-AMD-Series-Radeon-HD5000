#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "hardware.h"
#include "firmware.h"
#include "debug.h"

static NTSTATUS MadisonReadFile(_In_ PCWSTR Path, _Outptr_result_bytebuffer_(*Size) PUCHAR* Buffer, _Out_ PULONG Size)
{
    UNICODE_STRING name; OBJECT_ATTRIBUTES oa; IO_STATUS_BLOCK iosb; FILE_STANDARD_INFORMATION fsi;
    HANDLE h = NULL; NTSTATUS st; PUCHAR b = NULL; ULONG got = 0;
    if (!Buffer || !Size) return STATUS_INVALID_PARAMETER;
    *Buffer = NULL; *Size = 0;
    RtlInitUnicodeString(&name, Path);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    st = ZwCreateFile(&h, GENERIC_READ | SYNCHRONIZE, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL,
                      FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    if (!NT_SUCCESS(st)) return st;
    st = ZwQueryInformationFile(h, &iosb, &fsi, sizeof(fsi), FileStandardInformation);
    if (!NT_SUCCESS(st) || fsi.EndOfFile.QuadPart <= 0 || fsi.EndOfFile.QuadPart > 64 * 1024) { ZwClose(h); return STATUS_FILE_INVALID; }
    b = (PUCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, (SIZE_T)fsi.EndOfFile.QuadPart, MADISON_POOL_TAG);
    if (!b) { ZwClose(h); return STATUS_INSUFFICIENT_RESOURCES; }
    st = ZwReadFile(h, NULL, NULL, NULL, &iosb, b, (ULONG)fsi.EndOfFile.QuadPart, NULL, NULL);
    if (NT_SUCCESS(st)) got = (ULONG)iosb.Information;
    ZwClose(h);
    if (!NT_SUCCESS(st) || got != (ULONG)fsi.EndOfFile.QuadPart) { ExFreePoolWithTag(b, MADISON_POOL_TAG); return NT_SUCCESS(st) ? STATUS_END_OF_FILE : st; }
    *Buffer = b; *Size = got; return STATUS_SUCCESS;
}

static NTSTATUS MadisonLoadOne(_Inout_ PUCHAR* Dst, _Inout_ PULONG DstSize, _In_ PCWSTR Path, _In_ ULONG ExactSize)
{
    PUCHAR b = NULL; ULONG size = 0; NTSTATUS st = MadisonReadFile(Path, &b, &size);
    if (!NT_SUCCESS(st)) return st;
    if (size != ExactSize) { ExFreePoolWithTag(b, MADISON_POOL_TAG); return STATUS_REVISION_MISMATCH; }
    *Dst = b; *DstSize = size; return STATUS_SUCCESS;
}

NTSTATUS MadisonFirmwareLoadFromDisk(MADISON_ADAPTER* Adapter)
{
    NTSTATUS st;
    if (!Adapter) return STATUS_INVALID_PARAMETER;
    if (Adapter->FirmwareLoaded) return STATUS_SUCCESS;
    st = MadisonLoadOne(&Adapter->PfpFirmware, &Adapter->PfpFirmwareSize, L"\\SystemRoot\\System32\\drivers\\MadisonFirmware\\JUNIPER_pfp.bin", MADISON_PFP_BYTES);
    if (!NT_SUCCESS(st)) return st;
    st = MadisonLoadOne(&Adapter->MeFirmware, &Adapter->MeFirmwareSize, L"\\SystemRoot\\System32\\drivers\\MadisonFirmware\\JUNIPER_me.bin", MADISON_ME_BYTES);
    if (!NT_SUCCESS(st)) { MadisonFirmwareUnload(Adapter); return st; }
    st = MadisonLoadOne(&Adapter->RlcFirmware, &Adapter->RlcFirmwareSize, L"\\SystemRoot\\System32\\drivers\\MadisonFirmware\\JUNIPER_rlc.bin", MADISON_RLC_BYTES);
    if (!NT_SUCCESS(st)) { MadisonFirmwareUnload(Adapter); return st; }
    Adapter->FirmwareLoaded = TRUE;
    MADISON_FW("Juniper firmware loaded: PFP=%lu ME=%lu RLC=%lu", Adapter->PfpFirmwareSize, Adapter->MeFirmwareSize, Adapter->RlcFirmwareSize);
    return STATUS_SUCCESS;
}

static __forceinline ULONG Be32(_In_reads_bytes_(4) const UCHAR* p)
{ return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | (ULONG)p[3]; }

NTSTATUS MadisonFirmwareUploadCp(MADISON_ADAPTER* Adapter)
{
    ULONG i;
    if (!Adapter || !Adapter->FirmwareLoaded || !Adapter->RegisterBaseVirtual) return STATUS_INVALID_PARAMETER;
    /* Exact Evergreen sequence used by radeon: stop CP, program PFP then ME RAM. */
    WR_REG32(Adapter, CP_ME_CNTL, CP_ME_HALT | CP_PFP_HALT);
    WR_REG32(Adapter, CP_RB_CNTL, RB_NO_UPDATE | RB_BLKSZ(15) | RB_BUFSZ(3));
    WR_REG32(Adapter, CP_PFP_UCODE_ADDR, 0);
    for (i = 0; i < MADISON_PFP_UCODE_DWORDS; ++i) WR_REG32(Adapter, CP_PFP_UCODE_DATA, Be32(Adapter->PfpFirmware + i * 4));
    WR_REG32(Adapter, CP_PFP_UCODE_ADDR, 0);
    WR_REG32(Adapter, CP_ME_RAM_WADDR, 0);
    for (i = 0; i < MADISON_ME_UCODE_DWORDS; ++i) WR_REG32(Adapter, CP_ME_RAM_DATA, Be32(Adapter->MeFirmware + i * 4));
    WR_REG32(Adapter, CP_PFP_UCODE_ADDR, 0);
    WR_REG32(Adapter, CP_ME_RAM_WADDR, 0);
    WR_REG32(Adapter, CP_ME_RAM_RADDR, 0);
    return STATUS_SUCCESS;
}

NTSTATUS MadisonCpStart(MADISON_ADAPTER* Adapter)
{
    ULONG rbCtl;
    ULONG* r;
    if (!Adapter || !Adapter->Ring || !Adapter->Ring->GpuAddress || !Adapter->RegisterBaseVirtual) return STATUS_INVALID_PARAMETER;

    /* Match Linux evergreen_cp_resume(): reset CP + dependent graphics blocks before
       programming the ring. This is intentionally only reached when Phase-3 hardware
       execution is explicitly enabled. */
    WR_REG32(Adapter, GRBM_SOFT_RESET,
             GRBM_SOFT_RESET__SOFT_RESET_CP |
             GRBM_SOFT_RESET__SOFT_RESET_PA |
             GRBM_SOFT_RESET__SOFT_RESET_SH |
             GRBM_SOFT_RESET__SOFT_RESET_VGT |
             GRBM_SOFT_RESET__SOFT_RESET_SPI |
             GRBM_SOFT_RESET__SOFT_RESET_SX);
    (VOID)REG32(Adapter, GRBM_SOFT_RESET);
    KeStallExecutionProcessor(15000);
    WR_REG32(Adapter, GRBM_SOFT_RESET, 0);
    (VOID)REG32(Adapter, GRBM_SOFT_RESET);

    /* 4-KB GPU page => BLKSZ 9, 256-KB ring => BUFSZ 15. RB_NO_UPDATE is mandatory here: no read-pointer
       writeback buffer exists, so without it the CP would write RPTR to CP_RB_RPTR_ADDR (= bus address 0). */
    rbCtl = RB_BLKSZ(MADISON_RING_BLKSZ) | RB_BUFSZ(MADISON_RING_BUFSZ) | RB_NO_UPDATE;
    WR_REG32(Adapter, CP_RB_CNTL, rbCtl | RB_RPTR_WR_ENA);
    WR_REG32(Adapter, CP_RB_WPTR_DELAY, 0);
    WR_REG32(Adapter, CP_RB_RPTR_WR, 0);
    WR_REG32(Adapter, CP_RB_WPTR, 0);
    WR_REG32(Adapter, CP_RB_RPTR_ADDR, 0);
    WR_REG32(Adapter, CP_RB_RPTR_ADDR_HI, 0);
    WR_REG32(Adapter, CP_RB_CNTL, rbCtl);               /* drop RPTR_WR_ENA again (Linux evergreen_cp_resume) */
    WR_REG32(Adapter, CP_RB_BASE, (ULONG)(Adapter->Ring->GpuAddress >> 8));
    WR_REG32(Adapter, CP_DEBUG, (1u << 27) | (1u << 28));

    /* Evergreen startup submits ME_INITIALIZE before releasing the PFP/ME halt. */
    r = (ULONG*)Adapter->Ring->CpuAddress;
    r[0] = PACKET3(PACKET3_ME_INITIALIZE, 5);
    r[1] = 0x1;
    r[2] = 0x0;
    r[3] = MADISON_ME_MAX_CONTEXT_INDEX;                /* was 0x7F; Juniper has 8 HW contexts */
    r[4] = PACKET3_ME_INITIALIZE_DEVICE_ID(1);
    r[5] = 0x0;
    r[6] = 0x0;
    KeMemoryBarrier();
    WR_REG32(Adapter, CP_RB_WPTR, 7);
    Adapter->Ring->WritePtr = 7;

    WR_REG32(Adapter, CP_ME_CNTL, 0xFFu);               /* release PFP + ME */
    KeStallExecutionProcessor(10);

    Adapter->Ring->ReadPtr = REG32(Adapter, CP_RB_RPTR);
    Adapter->CpRunning = TRUE;
    Adapter->Ring->Ready = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS MadisonFirmwareLoad(MADISON_ADAPTER* Adapter)
{
    NTSTATUS st = MadisonFirmwareLoadFromDisk(Adapter);
    if (!NT_SUCCESS(st)) { MADISON_FW("Firmware load failed: 0x%08X", st); return st; }
    st = MadisonFirmwareUploadCp(Adapter);
    if (!NT_SUCCESS(st)) return st;
    return STATUS_SUCCESS;
}

VOID MadisonFirmwareUnload(MADISON_ADAPTER* Adapter)
{
    if (!Adapter) return;
    if (Adapter->PfpFirmware) ExFreePoolWithTag(Adapter->PfpFirmware, MADISON_POOL_TAG);
    if (Adapter->MeFirmware) ExFreePoolWithTag(Adapter->MeFirmware, MADISON_POOL_TAG);
    if (Adapter->RlcFirmware) ExFreePoolWithTag(Adapter->RlcFirmware, MADISON_POOL_TAG);
    Adapter->PfpFirmware = Adapter->MeFirmware = Adapter->RlcFirmware = NULL;
    Adapter->PfpFirmwareSize = Adapter->MeFirmwareSize = Adapter->RlcFirmwareSize = 0;
    Adapter->FirmwareLoaded = FALSE;
}
