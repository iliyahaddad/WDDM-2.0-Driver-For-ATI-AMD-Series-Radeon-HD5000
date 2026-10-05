#include "driver.h"
#include "adapter.h"
#include "registers.h"
#include "ddi.h"
#include "debug.h"


// ============================================================================
// MadisonDebugInit
// ============================================================================
VOID
MadisonDebugInit(
    _In_ MADISON_ADAPTER* pAdapter
)
{
    UNREFERENCED_PARAMETER(pAdapter);

    // Initialize debug level from registry or use default
    g_MadisonDebugLevel = MADISON_DEBUG_ALL;

    MADISON_INFO("Debug system initialized");
}

// ============================================================================
// MadisonDebugDumpAdapter
// ============================================================================
VOID
MadisonDebugDumpAdapter(
    _In_ MADISON_ADAPTER* pAdapter
)
{
    if (pAdapter == NULL) {
        MADISON_ERROR("DumpAdapter: NULL adapter");
        return;
    }

    MADISON_INFO("=== Adapter Dump ===");
    MADISON_INFO("  VendorId: 0x%04X", pAdapter->VendorId);
    MADISON_INFO("  DeviceId: 0x%04X", pAdapter->DeviceId);
    MADISON_INFO("  SubsystemVendorId: 0x%04X", pAdapter->SubsystemVendorId);
    MADISON_INFO("  SubsystemDeviceId: 0x%04X", pAdapter->SubsystemDeviceId);
    MADISON_INFO("  RevisionId: 0x%02X", pAdapter->RevisionId);
    MADISON_INFO("  RegisterBase: 0x%llX", pAdapter->RegisterBase.QuadPart);
    MADISON_INFO("  RegisterLength: 0x%X", pAdapter->RegisterLength);
    MADISON_INFO("  Started: %d", pAdapter->Started);
    MADISON_INFO("  Removed: %d", pAdapter->Removed);
    MADISON_INFO("  PhysicalAddressingEnabled: %d", pAdapter->PhysicalAddressingEnabled);
    MADISON_INFO("  PreemptionAware: %d", pAdapter->PreemptionAware);
    MADISON_INFO("  FirmwareLoaded: %d", pAdapter->FirmwareLoaded);
    MADISON_INFO("===================");
}

// ============================================================================
// MadisonDebugDumpRegisters
// ============================================================================
VOID
MadisonDebugDumpRegisters(
    _In_ MADISON_ADAPTER* pAdapter
)
{
    if (pAdapter == NULL || pAdapter->RegisterBaseVirtual == NULL) {
        MADISON_ERROR("DumpRegisters: Invalid adapter");
        return;
    }

    MADISON_INFO("=== Register Dump ===");
    MADISON_INFO("  GRBM_STATUS:    0x%08X", REG32(pAdapter, GRBM_STATUS));
    MADISON_INFO("  GRBM_STATUS2:   0x%08X", REG32(pAdapter, GRBM_STATUS2));
    MADISON_INFO("  GRBM_SOFT_RESET: 0x%08X", REG32(pAdapter, GRBM_SOFT_RESET));
    MADISON_INFO("  CP_ME_CNTL:    0x%08X", REG32(pAdapter, CP_ME_CNTL));
    MADISON_INFO("  CP_ME_STATUS:  0x%08X", REG32(pAdapter, CP_ME_STATUS));
    MADISON_INFO("  CP_RB_BASE:    0x%08X", REG32(pAdapter, CP_RB_BASE));
    MADISON_INFO("  CP_RB_CNTL:    0x%08X", REG32(pAdapter, CP_RB_CNTL));
    MADISON_INFO("  CP_RB_RPTR:    0x%08X", REG32(pAdapter, CP_RB_RPTR));
    MADISON_INFO("  CP_RB_WPTR:    0x%08X", REG32(pAdapter, CP_RB_WPTR));
    MADISON_INFO("  CP_INT_CNTL:   0x%08X", REG32(pAdapter, CP_INT_CNTL));
    MADISON_INFO("  CP_INT_STATUS: 0x%08X", REG32(pAdapter, CP_INT_STATUS));
    MADISON_INFO("  SRBM_STATUS:   0x%08X", REG32(pAdapter, SRBM_STATUS));
    MADISON_INFO("  SRBM_SOFT_RESET: 0x%08X", REG32(pAdapter, SRBM_SOFT_RESET));
    MADISON_INFO("  IH_CNTL:       0x%08X", REG32(pAdapter, IH_CNTL));
    MADISON_INFO("  IH_RB_WPTR:    0x%08X", REG32(pAdapter, IH_RB_WPTR));
    MADISON_INFO("  MC_FB_LOCATION: 0x%08X", REG32(pAdapter, MC_FB_LOCATION));
    MADISON_INFO("  VM_CONTEXT0_CNTL: 0x%08X", REG32(pAdapter, VM_CONTEXT0_CNTL));
    MADISON_INFO("===================");
}
