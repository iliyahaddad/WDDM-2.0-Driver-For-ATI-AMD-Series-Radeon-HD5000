#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "ddi.h"
#include "debug.h"

// ============================================================================
// DxgkDdiInterruptRoutine  (runs at DIRQL: no pageable code, no heavy logging)
//
// Evergreen delivers interrupts through the IH ring, which this prototype does not
// implement yet, and the original code polled invented IH_STATUS/CP_INT_STATUS bits.
// Until the IH ring exists, the ISR only claims an interrupt when the CP is running and
// the fence page shows forward progress, which is a condition we can verify ourselves.
// ============================================================================
BOOLEAN
MadisonDdiInterruptRoutine(
    _In_ PVOID pMiniportDeviceContext,
    _In_ ULONG MessageNumber
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;
    ULONG previous;
    ULONG current;

    UNREFERENCED_PARAMETER(MessageNumber);

    if (pAdapter == NULL || !pAdapter->Started || !pAdapter->CpRunning || pAdapter->Fence.Value == NULL) {
        return FALSE;
    }

    previous = pAdapter->Fence.LastCompleted;
    current  = MadisonFenceReadCompleted(pAdapter);
    if (current == previous) {
        return FALSE;                       /* not ours / nothing new */
    }

    if (pAdapter->NotifyInterrupt != NULL) {
        DXGKARGCB_NOTIFY_INTERRUPT_DATA NotifyArgs;
        RtlZeroMemory(&NotifyArgs, sizeof(NotifyArgs));
        NotifyArgs.InterruptType = DXGK_INTERRUPT_DMA_COMPLETED;
        NotifyArgs.DmaCompleted.SubmissionFenceId = current;
        NotifyArgs.DmaCompleted.NodeOrdinal = 0;
        NotifyArgs.DmaCompleted.EngineOrdinal = 0;
        pAdapter->NotifyInterrupt(pAdapter->DeviceHandle, &NotifyArgs);
    }
    if (pAdapter->DxgkInterface.DxgkCbQueueDpc != NULL) {
        pAdapter->DxgkInterface.DxgkCbQueueDpc(pAdapter->DeviceHandle);
    }
    return TRUE;
}

// ============================================================================
// DxgkDdiDpcRoutine
// ============================================================================
VOID
MadisonDdiDpcRoutine(
    _In_ PVOID pMiniportDeviceContext
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)pMiniportDeviceContext;

    if (pAdapter == NULL || !pAdapter->Started) {
        return;
    }

    /* Dxgkrnl's DPC (DxgkCbNotifyDpc) must be called to finish notification processing. */
    if (pAdapter->DxgkInterface.DxgkCbNotifyDpc != NULL) {
        pAdapter->DxgkInterface.DxgkCbNotifyDpc(pAdapter->DeviceHandle);
    }
}
