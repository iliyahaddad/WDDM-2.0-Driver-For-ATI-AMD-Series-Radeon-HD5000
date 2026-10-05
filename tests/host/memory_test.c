#include <stdio.h>
#include "wdk_stub.h"
#include "driver.h"
#include "adapter.h"
#include "ddi.h"
int DbgPrint(const char*f,...){(void)f;return 0;} ULONG g_MadisonDebugLevel=0;
static int f=0; static void ck(int c,const char*m){ if(!c){printf("FAIL %s\n",m);f++;} }
int main(void){
    static MADISON_ADAPTER a; DXGK_QUERYSEGMENTIN in; DXGK_QUERYSEGMENTOUT3 q; DXGK_SEGMENTDESCRIPTOR3 d;
    memset(&a,0,sizeof a); memset(&in,0,sizeof in); memset(&q,0,sizeof q); memset(&d,0,sizeof d);

    ck(MadisonMemoryQuerySegments(NULL,&in,&q)==STATUS_INVALID_PARAMETER,"null adapter");
    ck(MadisonMemoryQuerySegments(&a,NULL,&q)==STATUS_INVALID_PARAMETER,"null input");

    /* no aperture supplied -> zero segments, success */
    q.NbSegment=7;
    ck(MadisonMemoryQuerySegments(&a,&in,&q)==STATUS_SUCCESS && q.NbSegment==0,"no aperture -> 0 segments");

    /* first call (pSegmentDescriptor == NULL) returns only the count */
    in.AgpApertureBase.QuadPart=0xD0000000ull; in.AgpApertureSize.QuadPart=64ull*1024*1024;
    memset(&q,0,sizeof q);
    ck(MadisonMemoryQuerySegments(&a,&in,&q)==STATUS_SUCCESS && q.NbSegment==1 && q.PagingBufferSize==0,"count call");

    /* second call fills the descriptor: Agp is the only flag bit */
    q.pSegmentDescriptor=&d; q.NbSegment=1;
    ck(MadisonMemoryQuerySegments(&a,&in,&q)==STATUS_SUCCESS,"detail call");
    ck(d.Flags.Value==1 && d.Flags.Agp==1,"only Agp flag set");
    ck(d.CpuTranslatedAddress.QuadPart==0xD0000000ll && d.Size==64u*1024*1024 && d.CommitLimit==d.Size,"aperture from input");
    ck(q.PagingBufferSegmentId==1 && q.PagingBufferSize==64*1024,"paging buffer");

    /* too-small output array */
    q.NbSegment=0; ck(MadisonMemoryQuerySegments(&a,&in,&q)==STATUS_BUFFER_TOO_SMALL,"buffer too small");

    /* aperture larger than the GART -> not supported */
    in.AgpApertureSize.QuadPart=(LONGLONG)(MADISON_GART_SIZE+4096); q.NbSegment=1;
    ck(MadisonMemoryQuerySegments(&a,&in,&q)==STATUS_NOT_SUPPORTED,"oversize aperture");

    if(f){printf("%d MEMORY TEST FAILURES\n",f);return 1;}
    printf("ALL MEMORY HOST TESTS PASSED\n"); return 0;
}
