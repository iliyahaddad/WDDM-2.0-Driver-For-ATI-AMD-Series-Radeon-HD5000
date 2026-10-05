#include "driver.h"
#include "adapter.h"
#include "ddi.h"
#include "debug.h"


// ============================================================================
// DxgkDdiPresent
// ============================================================================
NTSTATUS
MadisonDdiPresent(
    _In_    CONST HANDLE           hContext,
    _Inout_ DXGKARG_PRESENT*       pPresent
)
{
    MADISON_CONTEXT* pContext = (MADISON_CONTEXT*)hContext;

    MADISON_INFO("DxgkDdiPresent called");

    if (pContext == NULL || pPresent == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiPresent");
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_NOT_SUPPORTED;
}

// ============================================================================
// DxgkDdiRender
// ============================================================================
NTSTATUS
MadisonDdiRender(
    _In_    CONST HANDLE           hContext,
    _Inout_ DXGKARG_RENDER*        pRender
)
{
    /* CreateContext returns a MADISON_CONTEXT as hContext (original code cast it to MADISON_DEVICE). */
    MADISON_CONTEXT* pContext = (MADISON_CONTEXT*)hContext;

    MADISON_INFO("DxgkDdiRender called");

    if (pContext == NULL || pRender == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiRender");
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_NOT_SUPPORTED;
}
