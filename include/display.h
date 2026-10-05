#ifndef _MADISON_DISPLAY_H_
#define _MADISON_DISPLAY_H_

#include <ntddk.h>
#include <dispmprt.h>

/* One VidPN source and one child (video output). Matches NumberOfVideoPresentSources /
   NumberOfChildren reported from DxgkDdiStartDevice. */
#define MADISON_MAX_VIEWS        1
#define MADISON_MAX_CHILDREN     1

/* Used only when DxgkCbAcquirePostDisplayOwnership gives no usable POST mode. */
#define MADISON_FALLBACK_WIDTH   1024
#define MADISON_FALLBACK_HEIGHT  768

struct _MADISON_ADAPTER;

typedef struct _MADISON_CURRENT_MODE {
    DXGK_DISPLAY_INFORMATION             DispInfo;      /* POST (or fallback) scanout description */
    D3DKMDT_VIDPN_PRESENT_PATH_SCALING   Scaling;
    D3DKMDT_VIDPN_PRESENT_PATH_ROTATION  Rotation;
    UINT                                 SrcModeWidth;
    UINT                                 SrcModeHeight;
    UINT                                 SrcModePitch;
    PHYSICAL_ADDRESS                     PrimaryAddress; /* last SetVidPnSourceAddress */
    ULONG                                PrimarySegmentId;
    struct {
        UINT Committed        : 1;
        UINT SourceNotVisible : 1;
        UINT PostOwned        : 1;   /* DispInfo came from the POST framebuffer */
    } Flags;
} MADISON_CURRENT_MODE;

typedef struct _MADISON_DISPLAY {
    MADISON_CURRENT_MODE CurrentModes[MADISON_MAX_VIEWS];
    DEVICE_POWER_STATE   AdapterPowerState;
    DEVICE_POWER_STATE   MonitorPowerState;
    BOOLEAN              PostOwnershipHeld;
    BOOLEAN              UsingFallbackMode;
} MADISON_DISPLAY;

NTSTATUS MadisonDisplayStart(_Inout_ struct _MADISON_ADAPTER* Adapter);
VOID     MadisonDisplayStop(_Inout_ struct _MADISON_ADAPTER* Adapter);

/* Hardware scanout programming hook. NOT implemented: the display keeps scanning out whatever the
   VBIOS/GOP programmed. Implementing it needs the Evergreen (DCE4) CRTC/GRPH register map. */
NTSTATUS MadisonDisplayProgramScanout(_Inout_ struct _MADISON_ADAPTER* Adapter, _In_ UINT SourceId);

#endif
