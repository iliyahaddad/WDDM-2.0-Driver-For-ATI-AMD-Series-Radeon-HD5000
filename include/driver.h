#ifndef _MADISON_DRIVER_H_
#define _MADISON_DRIVER_H_

#include <ntddk.h>
#include <dispmprt.h>
#include <d3dkmddi.h>
#include <d3dkmdt.h>

#define MADISON_WDDM_INTERFACE_VERSION DXGKDDI_INTERFACE_VERSION_WDDM2_0
#define MADISON_WDDM_VERSION           DXGKDDI_WDDMv2
#define MADISON_POOL_TAG               'maKW'
#define MADISON_PCI_VENDOR_ID          0x1002
#define MADISON_PCI_DEVICE_ID          0x68C0
#define MADISON_RING_SIZE              (256 * 1024)
#define MADISON_MAX_ENGINES            1
#define MADISON_MAX_NODES              1
#define MADISON_MIN_REGISTER_BAR       (64u * 1024u)        /* Evergreen register BAR is 64-128 KB */
#define MADISON_MAX_REGISTER_BAR       (2u * 1024u * 1024u)

/* Hardware execution is deliberately opt-in until GART/firmware validation is complete. */
#define MADISON_ENABLE_PHASE3_CP      0   /* tools\enable-phase3-hardware.ps1 flips this (single gate) */
#define MADISON_ENABLE_HW_EXECUTION   MADISON_ENABLE_PHASE3_CP   /* derived: SubmitCommand follows the CP gate */
#define MADISON_ENABLE_GART           1
#define MADISON_GART_SIZE             (256ull * 1024ull * 1024ull)
#define MADISON_GART_PAGE_SIZE        4096ull
#define MADISON_MAX_DMA_COPY          (64u * 1024u)

typedef struct _MADISON_ADAPTER MADISON_ADAPTER;
typedef struct _MADISON_DEVICE  MADISON_DEVICE;

#include "registers.h"
#include "ring.h"
#include "fence.h"
#include "gart.h"
#include "memory.h"
#include "adapter.h"

#endif
