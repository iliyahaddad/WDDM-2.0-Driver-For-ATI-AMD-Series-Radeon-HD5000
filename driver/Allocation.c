#include "driver.h"
#include "adapter.h"
#include "ddi.h"
#include "debug.h"


// ============================================================================
// DxgkDdiOpenAllocation
// ============================================================================
NTSTATUS
MadisonDdiOpenAllocation(
    _In_ CONST HANDLE                    hDevice,
    _In_ CONST DXGKARG_OPENALLOCATION*   pOpenAllocation
)
{
    MADISON_DEVICE* pDevice = (MADISON_DEVICE*)hDevice;

    MADISON_DEBUG(MADISON_DEBUG_CP, "DxgkDdiOpenAllocation called");

    if (pDevice == NULL || pOpenAllocation == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiOpenAllocation");
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_NOT_SUPPORTED;
}

// ============================================================================
// DxgkDdiCloseAllocation
// ============================================================================
NTSTATUS
MadisonDdiCloseAllocation(
    _In_ CONST HANDLE                    hDevice,
    _In_ CONST DXGKARG_CLOSEALLOCATION*  pCloseAllocation
)
{
    MADISON_DEVICE* pDevice = (MADISON_DEVICE*)hDevice;

    if (pDevice == NULL || pCloseAllocation == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_NOT_SUPPORTED;
}

// ============================================================================
// DxgkDdiDescribeAllocation
// ============================================================================
NTSTATUS
MadisonDdiDescribeAllocation(
    _In_    CONST HANDLE                     hAdapter,
    _Inout_ DXGKARG_DESCRIBEALLOCATION*      pDescribeAllocation
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    MADISON_DEBUG(MADISON_DEBUG_CP, "DxgkDdiDescribeAllocation called");

    if (pAdapter == NULL || pDescribeAllocation == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiDescribeAllocation");
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_NOT_SUPPORTED;
}

// ============================================================================
// DxgkDdiGetStandardAllocationDriverData
// ============================================================================
NTSTATUS
MadisonDdiGetStandardAllocationDriverData(
    _In_    CONST HANDLE                                 hAdapter,
    _Inout_ DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA*     pGetStandardAllocationDriverData
)
{
    MADISON_ADAPTER* pAdapter = (MADISON_ADAPTER*)hAdapter;

    MADISON_DEBUG(MADISON_DEBUG_CP, "DxgkDdiGetStandardAllocationDriverData called");

    if (pAdapter == NULL || pGetStandardAllocationDriverData == NULL) {
        MADISON_ERROR("Invalid parameters to DxgkDdiGetStandardAllocationDriverData");
        return STATUS_INVALID_PARAMETER;
    }

    // In Phase 2A, this is a stub
    MADISON_WARN("DxgkDdiGetStandardAllocationDriverData is a stub in Phase 2A");

    return STATUS_NOT_SUPPORTED;
}

// ============================================================================
// DxgkDdiCreateAllocation / DxgkDdiDestroyAllocation  (not implemented; never fake success)
// ============================================================================
NTSTATUS
MadisonDdiCreateAllocation(
    _In_    CONST HANDLE                hAdapter,
    _Inout_ DXGKARG_CREATEALLOCATION*   pCreateAllocation
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pCreateAllocation);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
MadisonDdiDestroyAllocation(
    _In_ CONST HANDLE                       hAdapter,
    _In_ CONST DXGKARG_DESTROYALLOCATION*   pDestroyAllocation
)
{
    UNREFERENCED_PARAMETER(hAdapter);
    UNREFERENCED_PARAMETER(pDestroyAllocation);
    return STATUS_NOT_SUPPORTED;
}
