/* Minimal host-side stand-ins so gcc can check names/macros/arity. NOT the real WDK. */
#ifndef WDK_STUB_H
#define WDK_STUB_H
struct _DI; struct _VPI;
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef unsigned char UCHAR,*PUCHAR,BOOLEAN; typedef unsigned short USHORT; typedef unsigned int ULONG,UINT,*PULONG;
typedef uint64_t ULONG64,ULONGLONG; typedef int64_t LONGLONG; typedef long LONG; typedef void VOID,*PVOID,*HANDLE; typedef size_t SIZE_T,PFN_NUMBER;
typedef int NTSTATUS; typedef uint8_t KIRQL; typedef unsigned long KSPIN_LOCK;
typedef wchar_t WCHAR; typedef const WCHAR* PCWSTR;
typedef union { struct{ULONG LowPart; LONG HighPart;}; LONGLONG QuadPart; } PHYSICAL_ADDRESS, LARGE_INTEGER;
#define TRUE 1
#define FALSE 0
#define CONST const
#define NT_SUCCESS(s) ((s)>=0)
#define _In_
#define _Out_
#define _Inout_
#define _Outptr_result_bytebuffer_(x)
#define _In_reads_bytes_(x)
#define _Out_writes_(x)
#define _Inout_
#define IN_CONST_PDEVICE_OBJECT void*
#define OUT_PPVOID void**
#define UNREFERENCED_PARAMETER(x) (void)(x)
#define UNALIGNED
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define MAXULONG64 (~0ULL)
#define MAXULONG 0xFFFFFFFFu
#define STATUS_SUCCESS 0
#define STATUS_INVALID_PARAMETER (-2)
#define STATUS_NOT_SUPPORTED (-3)
#define STATUS_DEVICE_NOT_READY (-4)
#define STATUS_INSUFFICIENT_RESOURCES (-5)
#define STATUS_BUFFER_TOO_SMALL (-6)
#define STATUS_OBJECT_NAME_NOT_FOUND (-7)
#define STATUS_UNSUCCESSFUL (-8)
#define STATUS_DEVICE_DATA_ERROR (-9)
#define STATUS_DATATYPE_MISALIGNMENT (-10)
#define STATUS_INVALID_ADDRESS (-11)
#define STATUS_IO_TIMEOUT (-12)
#define STATUS_DEVICE_BUSY (-13)
#define STATUS_FILE_INVALID (-14)
#define STATUS_END_OF_FILE (-15)
#define STATUS_REVISION_MISMATCH (-16)
#define STATUS_DEVICE_CONFIGURATION_ERROR (-17)
#define POOL_FLAG_NON_PAGED 1
void* ExAllocatePool2(uint64_t f,SIZE_T n,ULONG tag); void ExFreePoolWithTag(void*p,ULONG tag);
typedef enum{MmNonCached,MmCached,MmWriteCombined}MEMORY_CACHING_TYPE;
void* MmAllocateContiguousMemorySpecifyCache(SIZE_T,PHYSICAL_ADDRESS,PHYSICAL_ADDRESS,PHYSICAL_ADDRESS,MEMORY_CACHING_TYPE);
void MmFreeContiguousMemorySpecifyCache(void*,SIZE_T,MEMORY_CACHING_TYPE);
PHYSICAL_ADDRESS MmGetPhysicalAddress(void*);
void KeStallExecutionProcessor(ULONG); void KeMemoryBarrier(void);
void KeInitializeSpinLock(KSPIN_LOCK*); void KeAcquireSpinLock(KSPIN_LOCK*,KIRQL*); void KeReleaseSpinLock(KSPIN_LOCK*,KIRQL);
#define RtlZeroMemory(d,n) memset(d,0,n)
#define RtlCopyMemory(d,s,n) memcpy(d,s,n)
static inline SIZE_T RtlCompareMemory(const void*a,const void*b,SIZE_T n){SIZE_T i=0;while(i<n&&((const char*)a)[i]==((const char*)b)[i])i++;return i;}
ULONG READ_REGISTER_ULONG(PULONG); void WRITE_REGISTER_ULONG(PULONG,ULONG);
int DbgPrint(const char*,...);
#define MmGetMdlPfnArray(m) ((PFN_NUMBER*)(m))
typedef struct{int x;}*PMDL;
typedef struct{USHORT Length;USHORT MaximumLength;WCHAR*Buffer;}UNICODE_STRING,*PUNICODE_STRING;
typedef struct _DRIVER_OBJECT DRIVER_OBJECT,*PDRIVER_OBJECT;
typedef struct{int a;}OBJECT_ATTRIBUTES; typedef struct{ULONG Information;}IO_STATUS_BLOCK; typedef struct{LARGE_INTEGER EndOfFile;}FILE_STANDARD_INFORMATION;
#define OBJ_CASE_INSENSITIVE 1
#define OBJ_KERNEL_HANDLE 2
#define InitializeObjectAttributes(a,b,c,d,e)
void RtlInitUnicodeString(PUNICODE_STRING,PCWSTR);
NTSTATUS ZwCreateFile(HANDLE*,ULONG,OBJECT_ATTRIBUTES*,IO_STATUS_BLOCK*,void*,ULONG,ULONG,ULONG,ULONG,void*,ULONG);
NTSTATUS ZwQueryInformationFile(HANDLE,IO_STATUS_BLOCK*,void*,ULONG,int); NTSTATUS ZwReadFile(HANDLE,void*,void*,void*,IO_STATUS_BLOCK*,void*,ULONG,void*,void*); NTSTATUS ZwClose(HANDLE);
#define GENERIC_READ 1
#define SYNCHRONIZE 2
#define FILE_ATTRIBUTE_NORMAL 0
#define FILE_SHARE_READ 1
#define FILE_OPEN 1
#define FILE_SYNCHRONOUS_IO_NONALERT 1
#define FileStandardInformation 5
/* CM resources */
typedef struct{unsigned char Type; union{struct{PHYSICAL_ADDRESS Start;ULONG Length;}Memory;}u;}CM_PARTIAL_RESOURCE_DESCRIPTOR,*PCM_PARTIAL_RESOURCE_DESCRIPTOR;
#define CmResourceTypeMemory 3
typedef struct{struct{struct{ULONG Count;CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];}PartialResourceList;}List[1];ULONG Count;}CM_RESOURCE_LIST,*PCM_RESOURCE_LIST;
typedef struct{int x;}PCI_COMMON_CONFIG;
/* DXGK subset */
typedef enum{DXGK_WHICHSPACE_ROM=5}DXGK_WHICHSPACE;
typedef struct{PCM_RESOURCE_LIST TranslatedResourceList;PHYSICAL_ADDRESS AgpApertureBase;SIZE_T AgpApertureSize;}DXGK_DEVICE_INFO;
typedef struct{int x;}DXGK_START_INFO,*PDXGK_START_INFO;
typedef struct{int InterruptType;struct{UINT SubmissionFenceId,NodeOrdinal,EngineOrdinal;}DmaCompleted;}DXGKARGCB_NOTIFY_INTERRUPT_DATA;
#define DXGK_INTERRUPT_DMA_COMPLETED 1
typedef BOOLEAN (*PDXGKCB_NOTIFY_INTERRUPT)(HANDLE,DXGKARGCB_NOTIFY_INTERRUPT_DATA*);
typedef struct{HANDLE DeviceHandle;
 NTSTATUS(*DxgkCbReadDeviceSpace)(HANDLE,ULONG,void*,ULONG,ULONG,ULONG*);
 NTSTATUS(*DxgkCbGetDeviceInformation)(HANDLE,DXGK_DEVICE_INFO*);
 NTSTATUS(*DxgkCbMapMemory)(HANDLE,PHYSICAL_ADDRESS,ULONG,BOOLEAN,BOOLEAN,MEMORY_CACHING_TYPE,PVOID*);
 NTSTATUS(*DxgkCbUnmapMemory)(HANDLE,PVOID);
 PDXGKCB_NOTIFY_INTERRUPT DxgkCbNotifyInterrupt;
 NTSTATUS(*DxgkCbAcquirePostDisplayOwnership)(HANDLE,struct _DI*);
 NTSTATUS(*DxgkCbQueryVidPnInterface)(ULONG64,int,const struct _VPI**);
 void(*DxgkCbQueueDpc)(HANDLE); void(*DxgkCbNotifyDpc)(HANDLE);}DXGKRNL_INTERFACE,*PDXGKRNL_INTERFACE;
/* Segment query (member names per learn.microsoft.com: DXGK_QUERYSEGMENTIN has AgpApertureBase/AgpApertureSize(LARGE_INTEGER)/AgpFlags) */
typedef struct{union{struct{UINT Agp:1;UINT Aperture:1;UINT CpuVisible:1;UINT CacheCoherent:1;UINT PitchAlignment:1;UINT PopulatedFromSystemMemory:1;UINT PreservedDuringStandby:1;UINT PreservedDuringHibernate:1;UINT PartiallyPurgeable:1;UINT DirectFlip:1;UINT Use64KBPages:1;UINT Reserved:21;};UINT Value;};}DXGK_SEGMENTFLAGS;
typedef struct{PHYSICAL_ADDRESS AgpApertureBase;LARGE_INTEGER AgpApertureSize;DXGK_SEGMENTFLAGS AgpFlags;}DXGK_QUERYSEGMENTIN;
typedef struct{PHYSICAL_ADDRESS BaseAddress;PHYSICAL_ADDRESS CpuTranslatedAddress;SIZE_T Size;UINT NbOfBanks;SIZE_T*pBankRangeTable;SIZE_T CommitLimit;DXGK_SEGMENTFLAGS Flags;}DXGK_SEGMENTDESCRIPTOR3;
typedef struct{UINT NbSegment;DXGK_SEGMENTDESCRIPTOR3*pSegmentDescriptor;UINT PagingBufferSegmentId;UINT PagingBufferSize;UINT PagingBufferPrivateDataSize;}DXGK_QUERYSEGMENTOUT3;

typedef struct{int MultiEngineAware,PreemptionAware,NoDmaPatching,CancelCommandAware;}DXGK_VIDSCHCAPS;
typedef struct{int VirtualAddressingSupported,GpuMmuSupported,IoMmuSupported;}DXGK_VIDMMCAPS;
typedef struct{int NbAsymetricProcessingNodes;}DXGK_GPUENGINETOPOLOGY;
typedef struct{int a;}DXGK_POINTERFLAGS,DXGK_PRESENTATIONCAPS,DXGK_FLIPCAPS;
typedef struct{int WDDMVersion; DXGK_VIDSCHCAPS SchedulingCaps; DXGK_VIDMMCAPS MemoryManagementCaps; DXGK_POINTERFLAGS PointerCaps; DXGK_PRESENTATIONCAPS PresentationCaps; DXGK_FLIPCAPS FlipCaps; DXGK_GPUENGINETOPOLOGY GpuEngineTopology;}DXGK_DRIVERCAPS;
#define DXGKDDI_WDDMv2 0x2000
#define DXGKDDI_INTERFACE_VERSION_WDDM2_0 0x5023

#define __forceinline inline
#define __in
#define __out
#define __inout
typedef NTSTATUS DRIVER_INITIALIZE(PDRIVER_OBJECT,PUNICODE_STRING); typedef VOID DRIVER_UNLOAD(PDRIVER_OBJECT);
typedef struct{int a;}DXGKARG_QUERYADAPTERINFO_DUMMY;
typedef struct{int Type; void* pInputData; ULONG InputDataSize; void* pOutputData; ULONG OutputDataSize;}DXGKARG_QUERYADAPTERINFO;
#define DXGKQAITYPE_DRIVERCAPS 0
#define DXGKQAITYPE_VIDSCHCAPS 1
#define DXGKQAITYPE_VIDMMCAPS 2
#define DXGKQAITYPE_GPUENGINETOPOLOGY 3
#define DXGKQAITYPE_QUERYSEGMENT3 4
typedef struct{ULONG DmaBufferSubmissionStartOffset,DmaBufferSubmissionEndOffset,DmaBufferSize,AllocationListSize,PatchLocationListSize,PatchLocationListSubmissionStart,PatchLocationListSubmissionLength;void*pDmaBuffer;struct _AL*pAllocationList;struct _PL*pPatchLocationList;}DXGKARG_PATCH;
typedef struct _AL{PHYSICAL_ADDRESS PhysicalAddress;}DXGK_ALLOCATIONLIST; typedef struct _PL{ULONG AllocationIndex,PatchOffset,AllocationOffset;}D3DDDI_PATCHLOCATIONLIST;
typedef struct{void*pDmaBuffer;ULONG DmaBufferSize,DmaBufferSubmissionStartOffset,DmaBufferSubmissionEndOffset,SubmissionFenceId;}DXGKARG_SUBMITCOMMAND;
enum{DXGK_OPERATION_MAP_APERTURE_SEGMENT,DXGK_OPERATION_UNMAP_APERTURE_SEGMENT};
typedef struct{int Operation;struct{PMDL pMdl;ULONG SegmentId,OffsetInPages,NumberOfPages,MdlOffset;}MapApertureSegment;struct{ULONG SegmentId,OffsetInPages,NumberOfPages;}UnmapApertureSegment;}DXGKARG_BUILDPAGINGBUFFER;
typedef struct{ULONG CurrentFence,NodeOrdinal,EngineOrdinal;}DXGKARG_QUERYCURRENTFENCE;
typedef struct{int a;}DXGKARG_RESETENGINE,DXGKARG_PREEMPTCOMMAND,DXGKARG_RENDER,DXGKARG_PRESENT,DXGKARG_CREATEALLOCATION,DXGKARG_DESTROYALLOCATION,DXGKARG_DESCRIBEALLOCATION,DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA,DXGKARG_OPENALLOCATION,DXGKARG_CLOSEALLOCATION,DXGKARG_COLLECTDBGINFO;
typedef struct{int EngineType;}DXGKARG_GETNODEMETADATA; enum{DXGK_ENGINE_TYPE_3D=1};
typedef struct{HANDLE hDevice;}DXGKARG_CREATEDEVICE;
typedef struct{ULONG NodeOrdinal,EngineAffinity;HANDLE hContext;struct{ULONG DmaBufferSize,DmaBufferSegmentSet,DmaBufferPrivateDataSize,AllocationListSize,PatchLocationListSize;}ContextInfo;}DXGKARG_CREATECONTEXT;
#define DXGKDDI_QUERYSEGMENT3_STUB 0
typedef struct{ULONG HasStuff;}DXGK_DUMMY;
typedef struct{void*DxgkDdiAddDevice,*DxgkDdiStartDevice,*DxgkDdiStopDevice,*DxgkDdiRemoveDevice,*DxgkDdiUnload,*DxgkDdiQueryAdapterInfo,*DxgkDdiInterruptRoutine,*DxgkDdiDpcRoutine,*DxgkDdiPatch,*DxgkDdiSubmitCommand,*DxgkDdiBuildPagingBuffer,*DxgkDdiQueryCurrentFence,*DxgkDdiResetEngine,*DxgkDdiPreemptCommand,*DxgkDdiGetNodeMetadata,*DxgkDdiCreateDevice,*DxgkDdiDestroyDevice,*DxgkDdiCreateContext,*DxgkDdiDestroyContext,*DxgkDdiRender,*DxgkDdiPresent,*DxgkDdiCreateAllocation,*DxgkDdiDestroyAllocation,*DxgkDdiDescribeAllocation,*DxgkDdiGetStandardAllocationDriverData,*DxgkDdiOpenAllocation,*DxgkDdiCloseAllocation,*DxgkDdiCollectDbgInfo,
*DxgkDdiQueryChildRelations,*DxgkDdiQueryChildStatus,*DxgkDdiQueryDeviceDescriptor,*DxgkDdiSetPowerState,*DxgkDdiNotifyAcpiEvent,*DxgkDdiDispatchIoRequest,*DxgkDdiResetDevice,*DxgkDdiQueryInterface,*DxgkDdiControlEtwLogging,*DxgkDdiResetFromTimeout,*DxgkDdiRestartFromTimeout,*DxgkDdiStopDeviceAndReleasePostDisplayOwnership,*DxgkDdiSetPointerPosition,*DxgkDdiSetPointerShape,*DxgkDdiSetPalette,*DxgkDdiEscape,*DxgkDdiGetScanLine,*DxgkDdiControlInterrupt,*DxgkDdiIsSupportedVidPn,*DxgkDdiRecommendFunctionalVidPn,*DxgkDdiRecommendVidPnTopology,*DxgkDdiRecommendMonitorModes,*DxgkDdiEnumVidPnCofuncModality,*DxgkDdiSetVidPnSourceAddress,*DxgkDdiSetVidPnSourceVisibility,*DxgkDdiCommitVidPn,*DxgkDdiUpdateActiveVidPnPresentPath,*DxgkDdiQueryVidPnHWCapability;ULONG Version;}DRIVER_INITIALIZATION_DATA;
NTSTATUS DxgkInitialize(PDRIVER_OBJECT,PUNICODE_STRING,DRIVER_INITIALIZATION_DATA*);

/* ---- display / VidPN stubs ---- */
#define STATUS_NOT_IMPLEMENTED (-30)
#define STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED (-31)
#define STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE (-32)
#define STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET (-33)
#define STATUS_GRAPHICS_GAMMA_RAMP_NOT_SUPPORTED (-34)
#define STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED (-35)
#define STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE_MODE (-36)
#define STATUS_GRAPHICS_NO_RECOMMENDED_FUNCTIONAL_VIDPN (-37)
#define STATUS_GRAPHICS_SOURCE_NOT_IN_TOPOLOGY (-38)
#define STATUS_GRAPHICS_MODE_ALREADY_IN_MODESET (-39)
#define STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET 0x401E0000
#define DISPLAY_ADAPTER_HW_ID 0xFFFFFFFFu
#define DXGK_VIDPN_INTERFACE_VERSION_V1 1
typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID, D3DDDI_VIDEO_PRESENT_TARGET_ID;
#define D3DDDI_ID_UNINITIALIZED ((UINT)~0u)
#define D3DDDI_ID_ALL ((UINT)~1u)
typedef enum{D3DDDIFMT_A8R8G8B8=21,D3DDDIFMT_X8R8G8B8=22}D3DDDIFORMAT;
typedef ULONG64 D3DKMDT_HVIDPN,D3DKMDT_HVIDPNTOPOLOGY,D3DKMDT_HVIDPNSOURCEMODESET,D3DKMDT_HVIDPNTARGETMODESET,D3DKMDT_HMONITORSOURCEMODESET;
typedef enum{D3DKMDT_VPPS_UNINITIALIZED,D3DKMDT_VPPS_IDENTITY,D3DKMDT_VPPS_CENTERED,D3DKMDT_VPPS_NOTSPECIFIED,D3DKMDT_VPPS_UNPINNED}D3DKMDT_VIDPN_PRESENT_PATH_SCALING;
typedef enum{D3DKMDT_VPPR_UNINITIALIZED,D3DKMDT_VPPR_IDENTITY,D3DKMDT_VPPR_ROTATE90,D3DKMDT_VPPR_NOTSPECIFIED,D3DKMDT_VPPR_UNPINNED}D3DKMDT_VIDPN_PRESENT_PATH_ROTATION;
typedef struct{UINT Identity:1,Centered:1;}D3DKMDT_VIDPN_PRESENT_PATH_SCALING_SUPPORT;
typedef struct{UINT Identity:1,Rotate90:1,Rotate180:1,Rotate270:1,Offset0:1;}D3DKMDT_VIDPN_PRESENT_PATH_ROTATION_SUPPORT;
enum{D3DDDI_GAMMARAMP_DEFAULT=1}; enum{D3DKMDT_CB_UNINITIALIZED,D3DKMDT_CB_SCRGB,D3DKMDT_CB_SRGB};
enum{D3DKMDT_RMT_GRAPHICS=1}; enum{D3DKMDT_PVAM_DIRECT=1};
typedef struct{UINT VidPnSourceId,VidPnTargetId; struct{int Type;}GammaRamp; int VidPnTargetColorBasis;
 struct{D3DKMDT_VIDPN_PRESENT_PATH_SCALING Scaling;D3DKMDT_VIDPN_PRESENT_PATH_SCALING_SUPPORT ScalingSupport;D3DKMDT_VIDPN_PRESENT_PATH_ROTATION Rotation;D3DKMDT_VIDPN_PRESENT_PATH_ROTATION_SUPPORT RotationSupport;}ContentTransformation;}D3DKMDT_VIDPN_PRESENT_PATH;
typedef struct{struct{int cx,cy;}PrimSurfSize,VisibleRegionSize;UINT Stride;D3DDDIFORMAT PixelFormat;int ColorBasis,PixelValueAccessMode;}D3DKMDT_GRAPHICS_RENDERING_FORMAT;
typedef struct{int Type;union{D3DKMDT_GRAPHICS_RENDERING_FORMAT Graphics;}Format;}D3DKMDT_VIDPN_SOURCE_MODE;
typedef struct{ULONG Numerator,Denominator;}D3DDDI_RATIONAL;
typedef struct{int VideoStandard;struct{int cx,cy;}TotalSize,ActiveSize;D3DDDI_RATIONAL VSyncFreq,HSyncFreq;ULONG PixelRate;int ScanLineOrdering;}D3DKMDT_VIDEO_SIGNAL_INFO;
typedef struct{D3DKMDT_VIDEO_SIGNAL_INFO VideoSignalInfo;int Preference;}D3DKMDT_VIDPN_TARGET_MODE;
typedef struct{D3DKMDT_VIDEO_SIGNAL_INFO VideoSignalInfo;int ColorBasis;struct{UINT FirstChannel,SecondChannel,ThirdChannel,FourthChannel;}ColorCoeffDynamicRanges;int Origin,Preference;}D3DKMDT_MONITOR_SOURCE_MODE;
enum{D3DKMDT_VSS_OTHER,D3DKMDT_VSS_VESA_DMT}; enum{D3DDDI_VSSLO_PROGRESSIVE=1}; enum{D3DKMDT_MCO_DRIVER=2}; enum{D3DKMDT_MP_PREFERRED=2};
#define D3DKMDT_FREQUENCY_NOTSPECIFIED 0
enum{D3DKMDT_EPT_VIDPNSOURCE,D3DKMDT_EPT_VIDPNTARGET,D3DKMDT_EPT_SCALING,D3DKMDT_EPT_ROTATION};
typedef struct{ULONG64 hDesiredVidPn;BOOLEAN IsVidPnSupported;}DXGKARG_ISSUPPORTEDVIDPN;
typedef struct{int a;}DXGKARG_RECOMMENDFUNCTIONALVIDPN,DXGKARG_RECOMMENDVIDPNTOPOLOGY;
typedef struct _MSMS{int (*pfnCreateNewModeInfo)(ULONG64,D3DKMDT_MONITOR_SOURCE_MODE**);int(*pfnAddMode)(ULONG64,D3DKMDT_MONITOR_SOURCE_MODE*);int(*pfnReleaseModeInfo)(ULONG64,D3DKMDT_MONITOR_SOURCE_MODE*);}DXGK_MONITORSOURCEMODESET_INTERFACE;
typedef struct{D3DKMDT_HMONITORSOURCEMODESET hMonitorSourceModeSet;const DXGK_MONITORSOURCEMODESET_INTERFACE*pMonitorSourceModeSetInterface;}DXGKARG_RECOMMENDMONITORMODES;
typedef struct{ULONG64 hConstrainingVidPn;int EnumPivotType;struct{UINT VidPnSourceId,VidPnTargetId;}EnumPivot;}DXGKARG_ENUMVIDPNCOFUNCMODALITY;
typedef struct{UINT VidPnSourceId;PHYSICAL_ADDRESS PrimaryAddress;}DXGKARG_SETVIDPNSOURCEADDRESS;
typedef struct{UINT VidPnSourceId;BOOLEAN Visible;}DXGKARG_SETVIDPNSOURCEVISIBILITY;
typedef struct{ULONG64 hFunctionalVidPn;UINT AffectedVidPnSourceId;struct{UINT PathPoweredOff:1;}Flags;}DXGKARG_COMMITVIDPN;
typedef struct{D3DKMDT_VIDPN_PRESENT_PATH VidPnPresentPathInfo;}DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH;
typedef struct{UINT SourceId,TargetId;struct{UINT DriverRotation:1,DriverScaling:1,DriverCloning:1,DriverColorConvert:1,DriverLinkedAdapaterOutput:1,DriverRemoteDisplay:1;}VidPnHWCaps;}DXGKARG_QUERYVIDPNHWCAPABILITY;
typedef struct _SMS{int(*pfnCreateNewModeInfo)(D3DKMDT_HVIDPNSOURCEMODESET,D3DKMDT_VIDPN_SOURCE_MODE**);int(*pfnAddMode)(D3DKMDT_HVIDPNSOURCEMODESET,D3DKMDT_VIDPN_SOURCE_MODE*);int(*pfnReleaseModeInfo)(D3DKMDT_HVIDPNSOURCEMODESET,const D3DKMDT_VIDPN_SOURCE_MODE*);int(*pfnAcquirePinnedModeInfo)(D3DKMDT_HVIDPNSOURCEMODESET,const D3DKMDT_VIDPN_SOURCE_MODE**);}DXGK_VIDPNSOURCEMODESET_INTERFACE;
typedef struct _TMS{int(*pfnCreateNewModeInfo)(D3DKMDT_HVIDPNTARGETMODESET,D3DKMDT_VIDPN_TARGET_MODE**);int(*pfnAddMode)(D3DKMDT_HVIDPNTARGETMODESET,D3DKMDT_VIDPN_TARGET_MODE*);int(*pfnReleaseModeInfo)(D3DKMDT_HVIDPNTARGETMODESET,const D3DKMDT_VIDPN_TARGET_MODE*);int(*pfnAcquirePinnedModeInfo)(D3DKMDT_HVIDPNTARGETMODESET,const D3DKMDT_VIDPN_TARGET_MODE**);}DXGK_VIDPNTARGETMODESET_INTERFACE;
typedef struct _TI{int(*pfnGetNumPaths)(D3DKMDT_HVIDPNTOPOLOGY,SIZE_T*);int(*pfnGetNumPathsFromSource)(D3DKMDT_HVIDPNTOPOLOGY,UINT,SIZE_T*);int(*pfnEnumPathTargetsFromSource)(D3DKMDT_HVIDPNTOPOLOGY,UINT,UINT,UINT*);
 int(*pfnAcquireFirstPathInfo)(D3DKMDT_HVIDPNTOPOLOGY,const D3DKMDT_VIDPN_PRESENT_PATH**);int(*pfnAcquireNextPathInfo)(D3DKMDT_HVIDPNTOPOLOGY,const D3DKMDT_VIDPN_PRESENT_PATH*,const D3DKMDT_VIDPN_PRESENT_PATH**);int(*pfnAcquirePathInfo)(D3DKMDT_HVIDPNTOPOLOGY,UINT,UINT,const D3DKMDT_VIDPN_PRESENT_PATH**);int(*pfnReleasePathInfo)(D3DKMDT_HVIDPNTOPOLOGY,const D3DKMDT_VIDPN_PRESENT_PATH*);int(*pfnUpdatePathSupportInfo)(D3DKMDT_HVIDPNTOPOLOGY,const D3DKMDT_VIDPN_PRESENT_PATH*);}DXGK_VIDPNTOPOLOGY_INTERFACE;
typedef struct _VPI{int(*pfnGetTopology)(ULONG64,D3DKMDT_HVIDPNTOPOLOGY*,const DXGK_VIDPNTOPOLOGY_INTERFACE**);
 int(*pfnAcquireSourceModeSet)(ULONG64,UINT,D3DKMDT_HVIDPNSOURCEMODESET*,const DXGK_VIDPNSOURCEMODESET_INTERFACE**);int(*pfnReleaseSourceModeSet)(ULONG64,D3DKMDT_HVIDPNSOURCEMODESET);int(*pfnCreateNewSourceModeSet)(ULONG64,UINT,D3DKMDT_HVIDPNSOURCEMODESET*,const DXGK_VIDPNSOURCEMODESET_INTERFACE**);int(*pfnAssignSourceModeSet)(ULONG64,UINT,D3DKMDT_HVIDPNSOURCEMODESET);
 int(*pfnAcquireTargetModeSet)(ULONG64,UINT,D3DKMDT_HVIDPNTARGETMODESET*,const DXGK_VIDPNTARGETMODESET_INTERFACE**);int(*pfnReleaseTargetModeSet)(ULONG64,D3DKMDT_HVIDPNTARGETMODESET);int(*pfnCreateNewTargetModeSet)(ULONG64,UINT,D3DKMDT_HVIDPNTARGETMODESET*,const DXGK_VIDPNTARGETMODESET_INTERFACE**);int(*pfnAssignTargetModeSet)(ULONG64,UINT,D3DKMDT_HVIDPNTARGETMODESET);}DXGK_VIDPN_INTERFACE;
typedef struct _DI{UINT Width,Height,Pitch;D3DDDIFORMAT ColorFormat;PHYSICAL_ADDRESS PhysicAddress;UINT TargetId;}DXGK_DISPLAY_INFORMATION,*PDXGK_DISPLAY_INFORMATION;
typedef enum{PowerDeviceD0=1,PowerDeviceD3=4}DEVICE_POWER_STATE; typedef int POWER_ACTION; typedef int DXGK_EVENT_TYPE; typedef int DXGK_INTERRUPT_TYPE; typedef struct{int a;}*PQUERY_INTERFACE; typedef struct{int a;}VIDEO_REQUEST_PACKET;
enum{TypeVideoOutput=1}; enum{HpdAwarenessInterruptible=3}; enum{D3DKMDT_VOT_OTHER=255}; enum{D3DKMDT_MOA_NONE=0}; enum{StatusConnection=1,StatusRotation=2};
typedef struct{int ChildDeviceType;struct{int HpdAwareness;struct{struct{int InterfaceTechnology,MonitorOrientationAwareness;BOOLEAN SupportsSdtvModes;}VideoOutput;}Type;}ChildCapabilities;ULONG AcpiUid,ChildUid;}DXGK_CHILD_DESCRIPTOR;
typedef struct{int Type;ULONG ChildUid;struct{BOOLEAN Connected;}HotPlug;}DXGK_CHILD_STATUS; typedef struct{int a;}DXGK_DEVICE_DESCRIPTOR;
typedef struct{struct{UINT Visible:1;}Flags;}DXGKARG_SETPOINTERPOSITION; typedef struct{int a;}DXGKARG_SETPOINTERSHAPE,DXGKARG_SETPALETTE,DXGKARG_ESCAPE,DXGKARG_GETSCANLINE;
#endif
