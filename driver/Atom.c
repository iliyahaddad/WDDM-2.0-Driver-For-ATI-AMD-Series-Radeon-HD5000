#include "driver.h"
#include "adapter.h"
#include "atom.h"
#include "registers.h"
#include "debug.h"

static USHORT U16(const UCHAR* p) { return (USHORT)(p[0] | ((USHORT)p[1] << 8)); }

NTSTATUS MadisonDetectVram(MADISON_ADAPTER* Adapter, ULONG64* VramBytes)
{
    ULONG mb;
    if (!Adapter || !VramBytes || !Adapter->RegisterBaseVirtual) return STATUS_INVALID_PARAMETER;
    mb = REG32(Adapter, CONFIG_MEMSIZE);              /* Evergreen reports MB */
    if (!mb || mb > 16384) return STATUS_DEVICE_DATA_ERROR;
    *VramBytes = (ULONG64)mb * 1024ULL * 1024ULL;
    return STATUS_SUCCESS;
}

NTSTATUS MadisonAtomReadAndParse(MADISON_ADAPTER* Adapter, MADISON_ATOM_INFO* Info)
{
    UCHAR hdr[0x60];
    UCHAR atom[8];
    ULONG got = 0;
    ULONG off;
    NTSTATUS st;
    ULONG64 vram = 0;

    if (!Adapter || !Info) return STATUS_INVALID_PARAMETER;
    RtlZeroMemory(Info, sizeof(*Info));
    if (!Adapter->DxgkInterface.DxgkCbReadDeviceSpace) return STATUS_NOT_SUPPORTED;

    st = Adapter->DxgkInterface.DxgkCbReadDeviceSpace(Adapter->DeviceHandle, DXGK_WHICHSPACE_ROM,
                                                      hdr, 0, sizeof(hdr), &got);
    if (!NT_SUCCESS(st) || got < 0x4A || hdr[0] != 0x55 || hdr[1] != 0xAA) return STATUS_OBJECT_NAME_NOT_FOUND;

    Info->RomValid = TRUE;
    Info->RomRevision = hdr[3];
    Info->ImageLength = (ULONG)hdr[2] * 512;

    off = U16(hdr + 0x48);                            /* pointer to the ATOM ROM header */
    Info->AtomHeaderOffset = (USHORT)off;
    if (off != 0) {
        got = 0;
        if (off + sizeof(atom) <= sizeof(hdr)) {
            RtlCopyMemory(atom, hdr + off, sizeof(atom));
            got = sizeof(atom);
            st = STATUS_SUCCESS;
        } else {
            st = Adapter->DxgkInterface.DxgkCbReadDeviceSpace(Adapter->DeviceHandle, DXGK_WHICHSPACE_ROM,
                                                              atom, off, sizeof(atom), &got);
        }
        /* ATOM_ROM_HEADER: {u16 size, u8 fmt_rev, u8 content_rev, char signature[4] = "ATOM", ...} */
        if (NT_SUCCESS(st) && got >= sizeof(atom)) {
            Info->AtomBios = (RtlCompareMemory(atom + 4, "ATOM", 4) == 4) ||
                             (RtlCompareMemory(atom + 4, "MOTA", 4) == 4);
            if (Info->AtomBios) {
                Info->AtomMajor = atom[2];
                Info->AtomMinor = atom[3];
            }
        }
    }

    if (NT_SUCCESS(MadisonDetectVram(Adapter, &vram))) Info->VramMegabytes = (ULONG)(vram >> 20);

    MADISON_INFO("VBIOS: valid=%d ATOM=%d header=0x%04X image=%lu bytes, VRAM=%lu MB",
                 Info->RomValid, Info->AtomBios, Info->AtomHeaderOffset, Info->ImageLength, Info->VramMegabytes);
    return STATUS_SUCCESS;
}
