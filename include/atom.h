#ifndef _MADISON_ATOM_H_
#define _MADISON_ATOM_H_
#include <ntddk.h>
struct _MADISON_ADAPTER;
typedef struct _MADISON_ATOM_INFO {
    BOOLEAN RomValid;
    BOOLEAN AtomBios;
    UCHAR RomRevision;
    USHORT AtomHeaderOffset;
    USHORT AtomMajor;
    USHORT AtomMinor;
    ULONG ImageLength;
    ULONG VramMegabytes;  /* CONFIG_MEMSIZE in MB (a ULONG cannot hold >4 GiB in bytes) */
} MADISON_ATOM_INFO;
NTSTATUS MadisonAtomReadAndParse(_Inout_ struct _MADISON_ADAPTER* Adapter, _Out_ MADISON_ATOM_INFO* Info);
NTSTATUS MadisonDetectVram(_Inout_ struct _MADISON_ADAPTER* Adapter, _Out_ ULONG64* VramBytes);
#endif
