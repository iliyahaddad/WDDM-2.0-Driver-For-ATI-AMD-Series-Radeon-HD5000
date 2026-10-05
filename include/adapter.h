#ifndef _MADISON_ADAPTER_H_
#define _MADISON_ADAPTER_H_

#include <ntddk.h>
#include <dispmprt.h>
#include "ring.h"
#include "fence.h"
#include "gart.h"
#include "memory.h"
#include "firmware.h"
#include "atom.h"
#include "display.h"

// Madison adapter context
struct _MADISON_ADAPTER {
    // WDDM/KMD infrastructure
    DXGKRNL_INTERFACE          DxgkInterface;
    PDXGKCB_NOTIFY_INTERRUPT   NotifyInterrupt;
    HANDLE                     DeviceHandle;
    ULONG                      NumberOfVideoPresentSources;
    ULONG                      NumberOfChildren;
    BOOLEAN                    Started;
    BOOLEAN                    Removed;

    // PCI configuration
    PCI_COMMON_CONFIG          PciConfig;
    ULONG                      PciConfigLength;
    PHYSICAL_ADDRESS           RegisterBase;
    PUCHAR                     RegisterBaseVirtual;
    ULONG                      RegisterLength;
    BOOLEAN                    RegisterMapped;

    // Hardware identification
    USHORT                     VendorId;
    USHORT                     DeviceId;
    USHORT                     SubsystemVendorId;
    USHORT                     SubsystemDeviceId;
    UCHAR                      RevisionId;
    USHORT                     SubsystemId;

    // Hardware state
    MADISON_RING*              Ring;
    MADISON_FENCE              Fence;
    MADISON_GART               Gart;
    MADISON_MEMORY             Memory;
    MADISON_DISPLAY            Display;
    KSPIN_LOCK                 SubmissionLock;

    // WDDM capabilities (filled during StartDevice/QueryAdapterInfo)
    DXGK_DRIVERCAPS            DriverCaps;
    DXGK_VIDSCHCAPS            VidSchCaps;
    DXGK_VIDMMCAPS             VidMmCaps;
    DXGK_GPUENGINETOPOLOGY     GpuEngineTopology;
    DXGK_POINTERFLAGS          PointerCaps;
    DXGK_PRESENTATIONCAPS      PresentationCaps;
    DXGK_FLIPCAPS              FlipCaps;

    // Physical addressing mode flags
    BOOLEAN                    PhysicalAddressingEnabled;
    BOOLEAN                    PreemptionAware;

    // Firmware
    BOOLEAN                    FirmwareLoaded;
    PUCHAR                     PfpFirmware;
    ULONG                      PfpFirmwareSize;
    PUCHAR                     MeFirmware;
    ULONG                      MeFirmwareSize;
    PUCHAR                     RlcFirmware;
    ULONG                      RlcFirmwareSize;

    // Debug
    MADISON_ATOM_INFO          AtomInfo;
    ULONG64                    VramBytes;
    BOOLEAN                    CpRunning;
    ULONG                      DebugFlags;
};

// Device context (per-display or per-render device)
typedef struct _MADISON_CONTEXT MADISON_CONTEXT;

struct _MADISON_DEVICE {
    MADISON_ADAPTER*           Adapter;
    HANDLE                     hDevice;
    ULONG                      EngineOrdinal;
    BOOLEAN                    Created;
};

typedef struct _MADISON_CONTEXT {
    MADISON_ADAPTER* Adapter;
    MADISON_DEVICE* Device;
    ULONG NodeOrdinal;
    ULONG EngineAffinity;
    BOOLEAN Created;
} MADISON_CONTEXT;

#endif // _MADISON_ADAPTER_H_
