#include <stdio.h>
#include "wdk_stub.h"
#include "driver.h"
#include "adapter.h"
#include "ddi.h"
int DbgPrint(const char*f,...){(void)f;return 0;} ULONG g_MadisonDebugLevel=0;
NTSTATUS MadisonResetEngine(MADISON_ADAPTER*a,ULONG e,BOOLEAN t){return STATUS_NOT_SUPPORTED;}
NTSTATUS MadisonDdiStopDevice(PVOID c){ MadisonDisplayStop((MADISON_ADAPTER*)c); return 0; }
static NTSTATUS AcqFail(HANDLE h,DXGK_DISPLAY_INFORMATION*d){return STATUS_UNSUCCESSFUL;}
static NTSTATUS AcqOk(HANDLE h,DXGK_DISPLAY_INFORMATION*d){d->Width=1920;d->Height=1080;d->Pitch=7680;d->ColorFormat=D3DDDIFMT_X8R8G8B8;return 0;}
static int f=0; static void ck(int c,const char*m){ if(!c){printf("FAIL %s\n",m);f++;} }
int main(void){
  static MADISON_ADAPTER A; DXGK_CHILD_DESCRIPTOR ch[2]; DXGK_CHILD_STATUS cs; DXGK_DISPLAY_INFORMATION di;
  memset(&A,0,sizeof A); memset(ch,0xAA,sizeof ch);
  A.DxgkInterface.DxgkCbAcquirePostDisplayOwnership=AcqFail;
  ck(MadisonDisplayStart(&A)==0,"start w/o POST info must not fail");
  ck(A.Display.UsingFallbackMode && A.Display.CurrentModes[0].DispInfo.Width==MADISON_FALLBACK_WIDTH,"fallback mode");
  A.DxgkInterface.DxgkCbAcquirePostDisplayOwnership=AcqOk; MadisonDisplayStart(&A);
  ck(A.Display.PostOwnershipHeld && A.Display.CurrentModes[0].DispInfo.Width==1920,"POST mode taken");
  memset(ch,0,sizeof ch); ck(MadisonDdiQueryChildRelations(&A,ch,sizeof ch)==0,"child rel");
  ck(ch[0].ChildDeviceType==TypeVideoOutput && ch[0].ChildUid==0,"child 0 filled"); ck(ch[1].ChildDeviceType==0,"last descriptor stays zeroed");
  ck(MadisonDdiQueryChildRelations(&A,ch,sizeof(ch[0]))==0,"size for 0 children ok");
  cs.Type=StatusConnection; cs.ChildUid=0; A.Started=TRUE; ck(MadisonDdiQueryChildStatus(&A,&cs,0)==0&&cs.HotPlug.Connected,"connected");
  cs.ChildUid=5; ck(MadisonDdiQueryChildStatus(&A,&cs,0)!=0,"bad uid rejected");
  ck(MadisonDdiQueryDeviceDescriptor(&A,0,NULL)==STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED,"no EDID");
  A.Display.AdapterPowerState=PowerDeviceD3; MadisonDdiSetPowerState(&A,DISPLAY_ADAPTER_HW_ID,PowerDeviceD0,0);
  ck(A.Display.CurrentModes[0].Flags.SourceNotVisible==1,"sources invisible after D3->D0");
  ck(MadisonDdiSetPowerState(&A,9,PowerDeviceD0,0)!=0,"bad hw uid rejected");
  ck(MadisonDdiStopDeviceAndReleasePostDisplayOwnership(&A,0,&di)==0 && di.Width==1920,"release returns POST info");
  printf(f?"%d FAILURES\n":"ALL DISPLAY HOST TESTS PASSED\n",f); return f; }
