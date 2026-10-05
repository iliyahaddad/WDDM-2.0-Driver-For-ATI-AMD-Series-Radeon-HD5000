#ifndef _MADISON_DDI_H_
#define _MADISON_DDI_H_

#include "driver.h"

/* Prototypes for every DDI routine implemented in driver\*.c.
   Keeping them in one header lets DriverEntry.c see them (the original tree had none). */

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD     MadisonDriverUnload;

NTSTATUS MadisonDdiAddDevice(_In_ IN_CONST_PDEVICE_OBJECT PhysicalDeviceObject, _Out_ OUT_PPVOID MiniportDeviceContext);
NTSTATUS MadisonDdiStartDevice(_In_ PVOID MiniportDeviceContext, _In_ PDXGK_START_INFO DxgkStartInfo, _In_ PDXGKRNL_INTERFACE DxgkInterface, _Out_ PULONG NumberOfVideoPresentSources, _Out_ PULONG NumberOfChildren);
NTSTATUS MadisonDdiStopDevice(_In_ PVOID MiniportDeviceContext);
NTSTATUS MadisonDdiRemoveDevice(_In_ PVOID MiniportDeviceContext);
NTSTATUS MadisonDdiQueryAdapterInfo(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_QUERYADAPTERINFO* pQueryAdapterInfo);

BOOLEAN  MadisonDdiInterruptRoutine(_In_ PVOID MiniportDeviceContext, _In_ ULONG MessageNumber);
VOID     MadisonDdiDpcRoutine(_In_ PVOID MiniportDeviceContext);

NTSTATUS MadisonDdiPatch(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_PATCH* pPatch);
NTSTATUS MadisonDdiSubmitCommand(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_SUBMITCOMMAND* pSubmitCommand);
NTSTATUS MadisonDdiBuildPagingBuffer(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_BUILDPAGINGBUFFER* pBuildPagingBuffer);
NTSTATUS MadisonDdiQueryCurrentFence(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_QUERYCURRENTFENCE* pCurrentFence);
NTSTATUS MadisonDdiResetEngine(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_RESETENGINE* pResetEngine);
NTSTATUS MadisonDdiPreemptCommand(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_PREEMPTCOMMAND* pPreemptCommand);
NTSTATUS MadisonDdiGetNodeMetadata(_In_ CONST HANDLE hAdapter, _In_ UINT NodeOrdinal, _Out_ DXGKARG_GETNODEMETADATA* pGetNodeMetadata);

NTSTATUS MadisonDdiCreateDevice(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_CREATEDEVICE* pCreateDevice);
NTSTATUS MadisonDdiDestroyDevice(_In_ CONST HANDLE hDevice);
NTSTATUS MadisonDdiCreateContext(_In_ CONST HANDLE hDevice, _Inout_ DXGKARG_CREATECONTEXT* pCreateContext);
NTSTATUS MadisonDdiDestroyContext(_In_ CONST HANDLE hContext);

NTSTATUS MadisonDdiRender(_In_ CONST HANDLE hContext, _Inout_ DXGKARG_RENDER* pRender);
NTSTATUS MadisonDdiPresent(_In_ CONST HANDLE hContext, _Inout_ DXGKARG_PRESENT* pPresent);

NTSTATUS MadisonDdiCreateAllocation(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_CREATEALLOCATION* pCreateAllocation);
NTSTATUS MadisonDdiDestroyAllocation(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_DESTROYALLOCATION* pDestroyAllocation);
NTSTATUS MadisonDdiDescribeAllocation(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_DESCRIBEALLOCATION* pDescribeAllocation);
NTSTATUS MadisonDdiGetStandardAllocationDriverData(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA* pData);
NTSTATUS MadisonDdiOpenAllocation(_In_ CONST HANDLE hDevice, _In_ CONST DXGKARG_OPENALLOCATION* pOpenAllocation);
NTSTATUS MadisonDdiCloseAllocation(_In_ CONST HANDLE hDevice, _In_ CONST DXGKARG_CLOSEALLOCATION* pCloseAllocation);

NTSTATUS MadisonDdiCollectDbgInfo(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_COLLECTDBGINFO* pCollectDbgInfo);


/* Child / monitor / power / PnP (adapted from the Microsoft KMDOD sample) */
NTSTATUS MadisonDdiQueryChildRelations(_In_ PVOID MiniportDeviceContext, _Inout_ DXGK_CHILD_DESCRIPTOR* pChildRelations, _In_ ULONG ChildRelationsSize);
NTSTATUS MadisonDdiQueryChildStatus(_In_ PVOID MiniportDeviceContext, _Inout_ DXGK_CHILD_STATUS* pChildStatus, _In_ BOOLEAN NonDestructiveOnly);
NTSTATUS MadisonDdiQueryDeviceDescriptor(_In_ PVOID MiniportDeviceContext, _In_ ULONG ChildUid, _Inout_ DXGK_DEVICE_DESCRIPTOR* pDeviceDescriptor);
NTSTATUS MadisonDdiSetPowerState(_In_ PVOID MiniportDeviceContext, _In_ ULONG HardwareUid, _In_ DEVICE_POWER_STATE DevicePowerState, _In_ POWER_ACTION ActionType);
NTSTATUS MadisonDdiNotifyAcpiEvent(_In_ PVOID MiniportDeviceContext, _In_ DXGK_EVENT_TYPE EventType, _In_ ULONG Event, _In_ PVOID Argument, _Out_ PULONG AcpiFlags);
NTSTATUS MadisonDdiDispatchIoRequest(_In_ PVOID MiniportDeviceContext, _In_ ULONG VidPnSourceId, _In_ VIDEO_REQUEST_PACKET* pVideoRequestPacket);
VOID     MadisonDdiResetDevice(_In_ PVOID MiniportDeviceContext);
NTSTATUS MadisonDdiQueryInterface(_In_ PVOID MiniportDeviceContext, _In_ PQUERY_INTERFACE QueryInterface);
VOID     MadisonDdiControlEtwLogging(_In_ BOOLEAN Enable, _In_ ULONG Flags, _In_ UCHAR Level);
NTSTATUS MadisonDdiResetFromTimeout(_In_ CONST HANDLE hAdapter);
NTSTATUS MadisonDdiRestartFromTimeout(_In_ CONST HANDLE hAdapter);
NTSTATUS MadisonDdiStopDeviceAndReleasePostDisplayOwnership(_In_ PVOID MiniportDeviceContext, _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId, _Out_ PDXGK_DISPLAY_INFORMATION pDisplayInfo);

/* Pointer / palette / misc display */
NTSTATUS MadisonDdiSetPointerPosition(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_SETPOINTERPOSITION* pSetPointerPosition);
NTSTATUS MadisonDdiSetPointerShape(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_SETPOINTERSHAPE* pSetPointerShape);
NTSTATUS MadisonDdiSetPalette(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_SETPALETTE* pSetPalette);
NTSTATUS MadisonDdiEscape(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_ESCAPE* pEscape);
NTSTATUS MadisonDdiGetScanLine(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_GETSCANLINE* pGetScanLine);
NTSTATUS MadisonDdiControlInterrupt(_In_ CONST HANDLE hAdapter, _In_ CONST DXGK_INTERRUPT_TYPE InterruptType, _In_ BOOLEAN EnableInterrupt);

/* VidPN (Vidpn.c, adapted from the Microsoft KMDOD sample) */
NTSTATUS MadisonDdiIsSupportedVidPn(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_ISSUPPORTEDVIDPN* pIsSupportedVidPn);
NTSTATUS MadisonDdiRecommendFunctionalVidPn(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_RECOMMENDFUNCTIONALVIDPN* CONST pRecommendFunctionalVidPn);
NTSTATUS MadisonDdiRecommendVidPnTopology(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_RECOMMENDVIDPNTOPOLOGY* CONST pRecommendVidPnTopology);
NTSTATUS MadisonDdiRecommendMonitorModes(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_RECOMMENDMONITORMODES* CONST pRecommendMonitorModes);
NTSTATUS MadisonDdiEnumVidPnCofuncModality(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_ENUMVIDPNCOFUNCMODALITY* CONST pEnumCofuncModality);
NTSTATUS MadisonDdiSetVidPnSourceAddress(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_SETVIDPNSOURCEADDRESS* pSetVidPnSourceAddress);
NTSTATUS MadisonDdiSetVidPnSourceVisibility(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_SETVIDPNSOURCEVISIBILITY* pSetVidPnSourceVisibility);
NTSTATUS MadisonDdiCommitVidPn(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_COMMITVIDPN* CONST pCommitVidPn);
NTSTATUS MadisonDdiUpdateActiveVidPnPresentPath(_In_ CONST HANDLE hAdapter, _In_ CONST DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH* CONST pUpdateActiveVidPnPresentPath);
NTSTATUS MadisonDdiQueryVidPnHWCapability(_In_ CONST HANDLE hAdapter, _Inout_ DXGKARG_QUERYVIDPNHWCAPABILITY* pVidPnHWCaps);

/* Internal helpers */
NTSTATUS MadisonResetEngine(_In_ MADISON_ADAPTER* pAdapter, _In_ ULONG EngineType, _In_ BOOLEAN bFromTimeout);
VOID     MadisonDebugInit(_In_ MADISON_ADAPTER* pAdapter);
VOID     MadisonDebugDumpAdapter(_In_ MADISON_ADAPTER* pAdapter);
VOID     MadisonDebugDumpRegisters(_In_ MADISON_ADAPTER* pAdapter);

#endif // _MADISON_DDI_H_
