#include <stdio.h>
#include <stdlib.h>
#include "wdk_stub.h"
#include "driver.h"
#include "adapter.h"
#include "ddi.h"

/* ---- host stand-ins for kernel services referenced by Vidpn.c ---- */
int DbgPrint(const char*f,...){(void)f;return 0;}
ULONG g_MadisonDebugLevel=0;
NTSTATUS MadisonDisplayProgramScanout(MADISON_ADAPTER*a,UINT s){(void)a;(void)s;return 0;}

/* ---- mock VidPN with leak accounting ---- */
static int pathsHeld, srcSetsHeld, tgtSetsHeld, srcModesHeld, tgtModesHeld, pinnedSrcHeld, pinnedTgtHeld, pathsHeldCommit;
static int cfg_paths=1, cfg_pinSrc=0, cfg_pinTgt=0, addedSrcModes=0, addedTgtModes=0, updated=0, assignedSrc=0, assignedTgt=0;
static D3DKMDT_VIDPN_PRESENT_PATH gPath;
static D3DKMDT_VIDPN_SOURCE_MODE gPinSrc; static D3DKMDT_VIDPN_TARGET_MODE gPinTgt;
static D3DKMDT_VIDPN_SOURCE_MODE gNewSrc; static D3DKMDT_VIDPN_TARGET_MODE gNewTgt;
static int cursor;

static int GetTopology(ULONG64 h,D3DKMDT_HVIDPNTOPOLOGY*t,const DXGK_VIDPNTOPOLOGY_INTERFACE**i);
static int GetNumPaths(D3DKMDT_HVIDPNTOPOLOGY t,SIZE_T*n){*n=cfg_paths;return 0;}
static int GetNumPathsFromSource(D3DKMDT_HVIDPNTOPOLOGY t,UINT s,SIZE_T*n){*n=cfg_paths;return 0;}
static int EnumTargets(D3DKMDT_HVIDPNTOPOLOGY t,UINT s,UINT i,UINT*o){*o=0;return 0;}
static int AcqFirst(D3DKMDT_HVIDPNTOPOLOGY t,const D3DKMDT_VIDPN_PRESENT_PATH**p){ if(!cfg_paths){*p=NULL;return STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET;} cursor=0; pathsHeld++; *p=&gPath; return 0;}
static int AcqNext(D3DKMDT_HVIDPNTOPOLOGY t,const D3DKMDT_VIDPN_PRESENT_PATH*c,const D3DKMDT_VIDPN_PRESENT_PATH**p){ cursor++; if(cursor>=cfg_paths){*p=NULL;return STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET;} pathsHeld++; *p=&gPath; return 0;}
static int AcqPath(D3DKMDT_HVIDPNTOPOLOGY t,UINT s,UINT g,const D3DKMDT_VIDPN_PRESENT_PATH**p){pathsHeldCommit++;*p=&gPath;return 0;}
static int RelPath(D3DKMDT_HVIDPNTOPOLOGY t,const D3DKMDT_VIDPN_PRESENT_PATH*p){ if(pathsHeldCommit>0)pathsHeldCommit--; else pathsHeld--; return 0;}
static int UpdPath(D3DKMDT_HVIDPNTOPOLOGY t,const D3DKMDT_VIDPN_PRESENT_PATH*p){updated++; if(!p->ContentTransformation.ScalingSupport.Identity||p->ContentTransformation.ScalingSupport.Centered) return -1; return 0;}
static DXGK_VIDPNTOPOLOGY_INTERFACE topo={GetNumPaths,GetNumPathsFromSource,EnumTargets,AcqFirst,AcqNext,AcqPath,RelPath,UpdPath};

static int S_Create(D3DKMDT_HVIDPNSOURCEMODESET h,D3DKMDT_VIDPN_SOURCE_MODE**m){srcModesHeld++;*m=&gNewSrc;return 0;}
static int S_Add(D3DKMDT_HVIDPNSOURCEMODESET h,D3DKMDT_VIDPN_SOURCE_MODE*m){srcModesHeld--;addedSrcModes++; if(m->Format.Graphics.PixelFormat!=D3DDDIFMT_A8R8G8B8||m->Format.Graphics.PrimSurfSize.cx!=1280) return -1; return 0;}
static int S_Rel(D3DKMDT_HVIDPNSOURCEMODESET h,const D3DKMDT_VIDPN_SOURCE_MODE*m){ if(m==&gPinSrc)pinnedSrcHeld--; else srcModesHeld--; return 0;}
static int S_Pin(D3DKMDT_HVIDPNSOURCEMODESET h,const D3DKMDT_VIDPN_SOURCE_MODE**m){ if(cfg_pinSrc){pinnedSrcHeld++;*m=&gPinSrc;}else *m=NULL; return 0;}
static DXGK_VIDPNSOURCEMODESET_INTERFACE sIf={S_Create,S_Add,S_Rel,S_Pin};
static int T_Create(D3DKMDT_HVIDPNTARGETMODESET h,D3DKMDT_VIDPN_TARGET_MODE**m){tgtModesHeld++;*m=&gNewTgt;return 0;}
static int T_Add(D3DKMDT_HVIDPNTARGETMODESET h,D3DKMDT_VIDPN_TARGET_MODE*m){tgtModesHeld--;addedTgtModes++;return 0;}
static int T_Rel(D3DKMDT_HVIDPNTARGETMODESET h,const D3DKMDT_VIDPN_TARGET_MODE*m){ if(m==&gPinTgt)pinnedTgtHeld--; else tgtModesHeld--; return 0;}
static int T_Pin(D3DKMDT_HVIDPNTARGETMODESET h,const D3DKMDT_VIDPN_TARGET_MODE**m){ if(cfg_pinTgt){pinnedTgtHeld++;*m=&gPinTgt;}else *m=NULL; return 0;}
static DXGK_VIDPNTARGETMODESET_INTERFACE tIf={T_Create,T_Add,T_Rel,T_Pin};

static int AcqSrcSet(ULONG64 v,UINT id,D3DKMDT_HVIDPNSOURCEMODESET*h,const DXGK_VIDPNSOURCEMODESET_INTERFACE**i){srcSetsHeld++;*h=11;*i=&sIf;return 0;}
static int RelSrcSet(ULONG64 v,D3DKMDT_HVIDPNSOURCEMODESET h){srcSetsHeld--;return 0;}
static int NewSrcSet(ULONG64 v,UINT id,D3DKMDT_HVIDPNSOURCEMODESET*h,const DXGK_VIDPNSOURCEMODESET_INTERFACE**i){srcSetsHeld++;*h=12;*i=&sIf;return 0;}
static int AsgSrcSet(ULONG64 v,UINT id,D3DKMDT_HVIDPNSOURCEMODESET h){srcSetsHeld--;assignedSrc++;return 0;}
static int AcqTgtSet(ULONG64 v,UINT id,D3DKMDT_HVIDPNTARGETMODESET*h,const DXGK_VIDPNTARGETMODESET_INTERFACE**i){tgtSetsHeld++;*h=21;*i=&tIf;return 0;}
static int RelTgtSet(ULONG64 v,D3DKMDT_HVIDPNTARGETMODESET h){tgtSetsHeld--;return 0;}
static int NewTgtSet(ULONG64 v,UINT id,D3DKMDT_HVIDPNTARGETMODESET*h,const DXGK_VIDPNTARGETMODESET_INTERFACE**i){tgtSetsHeld++;*h=22;*i=&tIf;return 0;}
static int AsgTgtSet(ULONG64 v,UINT id,D3DKMDT_HVIDPNTARGETMODESET h){tgtSetsHeld--;assignedTgt++;return 0;}
static DXGK_VIDPN_INTERFACE vIf={GetTopology,AcqSrcSet,RelSrcSet,NewSrcSet,AsgSrcSet,AcqTgtSet,RelTgtSet,NewTgtSet,AsgTgtSet};
static int GetTopology(ULONG64 h,D3DKMDT_HVIDPNTOPOLOGY*t,const DXGK_VIDPNTOPOLOGY_INTERFACE**i){*t=1;*i=&topo;return 0;}
static NTSTATUS QueryVidPnIf(ULONG64 h,int ver,const DXGK_VIDPN_INTERFACE**i){*i=&vIf;return 0;}
static NTSTATUS Acquire(HANDLE h,DXGK_DISPLAY_INFORMATION*d){d->Width=1280;d->Height=720;d->Pitch=5120;d->ColorFormat=D3DDDIFMT_A8R8G8B8;d->PhysicAddress.QuadPart=0xE0000000;return 0;}

static int fails=0;
#define CHECK(c,m) do{ if(!(c)){printf("  FAIL: %s\n",m);fails++;} }while(0)
static void reset(int paths,int pinS,int pinT){pathsHeld=srcSetsHeld=tgtSetsHeld=srcModesHeld=tgtModesHeld=pinnedSrcHeld=pinnedTgtHeld=pathsHeldCommit=0;addedSrcModes=addedTgtModes=updated=assignedSrc=assignedTgt=0;cfg_paths=paths;cfg_pinSrc=pinS;cfg_pinTgt=pinT;
 memset(&gPath,0,sizeof gPath);gPath.ContentTransformation.Scaling=D3DKMDT_VPPS_UNPINNED;gPath.ContentTransformation.Rotation=D3DKMDT_VPPR_UNPINNED;}
static void balanced(const char*n){ printf("  %s: paths=%d srcSets=%d tgtSets=%d srcModes=%d tgtModes=%d pinS=%d pinT=%d\n",n,pathsHeld+pathsHeldCommit,srcSetsHeld,tgtSetsHeld,srcModesHeld,tgtModesHeld,pinnedSrcHeld,pinnedTgtHeld);
 CHECK(pathsHeld+pathsHeldCommit==0&&srcSetsHeld==0&&tgtSetsHeld==0&&srcModesHeld==0&&tgtModesHeld==0&&pinnedSrcHeld==0&&pinnedTgtHeld==0,"leak: something acquired was not released"); }

int main(void){
  static MADISON_ADAPTER A; NTSTATUS st; DXGKARG_ENUMVIDPNCOFUNCMODALITY e; DXGKARG_COMMITVIDPN c; DXGKARG_ISSUPPORTEDVIDPN is;
  memset(&A,0,sizeof A); A.DxgkInterface.DxgkCbQueryVidPnInterface=QueryVidPnIf; A.DxgkInterface.DxgkCbAcquirePostDisplayOwnership=Acquire;
  /* MadisonDisplayStart lives in Display.c; do the same thing by hand here */
  Acquire(NULL,&A.Display.CurrentModes[0].DispInfo);
  memset(&e,0,sizeof e); e.hConstrainingVidPn=1; e.EnumPivotType=D3DKMDT_EPT_SCALING;  /* pivot elsewhere */
  e.EnumPivotType=99;

  printf("Enum: no pins\n"); reset(1,0,0); st=MadisonDdiEnumVidPnCofuncModality(&A,&e);
  CHECK(st==0,"status"); CHECK(addedSrcModes==1&&addedTgtModes==1,"exactly one POST source+target mode"); CHECK(assignedSrc==1&&assignedTgt==1,"sets assigned"); CHECK(updated==1,"path support updated"); balanced("no-pins");

  printf("Enum: pinned source and target\n"); reset(1,1,1); st=MadisonDdiEnumVidPnCofuncModality(&A,&e);
  CHECK(st==0,"status"); CHECK(addedSrcModes==0&&addedTgtModes==0,"no modes added when pinned"); balanced("pinned");

  printf("Enum: empty topology\n"); reset(0,0,0); st=MadisonDdiEnumVidPnCofuncModality(&A,&e);
  CHECK(st==0,"status"); balanced("empty");

  printf("Enum: two paths\n"); reset(2,0,0); st=MadisonDdiEnumVidPnCofuncModality(&A,&e);
  CHECK(st==0,"status"); CHECK(addedSrcModes==2,"two iterations"); balanced("two-paths");

  printf("Commit: pinned 1280x720 A8R8G8B8\n"); reset(1,1,0);
  gPinSrc.Type=D3DKMDT_RMT_GRAPHICS; gPinSrc.Format.Graphics.PixelFormat=D3DDDIFMT_A8R8G8B8; gPinSrc.Format.Graphics.ColorBasis=D3DKMDT_CB_SCRGB; gPinSrc.Format.Graphics.PixelValueAccessMode=D3DKMDT_PVAM_DIRECT; gPinSrc.Format.Graphics.PrimSurfSize.cx=1280; gPinSrc.Format.Graphics.PrimSurfSize.cy=720; gPinSrc.Format.Graphics.Stride=5120;
  gPath.ContentTransformation.Scaling=D3DKMDT_VPPS_IDENTITY; gPath.ContentTransformation.Rotation=D3DKMDT_VPPR_IDENTITY; gPath.VidPnTargetColorBasis=D3DKMDT_CB_SCRGB; gPath.GammaRamp.Type=D3DDDI_GAMMARAMP_DEFAULT;
  memset(&c,0,sizeof c); c.hFunctionalVidPn=1; c.AffectedVidPnSourceId=0;
  st=MadisonDdiCommitVidPn(&A,&c); CHECK(st==0,"commit status"); CHECK(A.Display.CurrentModes[0].Flags.Committed==1,"committed flag"); balanced("commit");

  printf("Commit: unsupported rotation is rejected and still balanced\n"); reset(1,1,0); gPath.ContentTransformation.Scaling=D3DKMDT_VPPS_IDENTITY; gPath.ContentTransformation.Rotation=D3DKMDT_VPPR_ROTATE90; gPath.VidPnTargetColorBasis=D3DKMDT_CB_SCRGB; gPath.GammaRamp.Type=D3DDDI_GAMMARAMP_DEFAULT;
  st=MadisonDdiCommitVidPn(&A,&c); CHECK(st==STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED,"rotate90 rejected"); balanced("commit-reject");

  printf("Commit: zero paths\n"); reset(0,0,0); st=MadisonDdiCommitVidPn(&A,&c); CHECK(st==0,"status"); balanced("commit-empty");

  printf("IsSupportedVidPn\n"); reset(1,0,0); is.hDesiredVidPn=1; st=MadisonDdiIsSupportedVidPn(&A,&is); CHECK(st==0&&is.IsVidPnSupported,"supported"); is.hDesiredVidPn=0; MadisonDdiIsSupportedVidPn(&A,&is); CHECK(is.IsVidPnSupported,"null vidpn supported");
  printf(fails?"\n%d FAILURES\n":"\nALL VIDPN HOST TESTS PASSED\n",fails); return fails;
}
