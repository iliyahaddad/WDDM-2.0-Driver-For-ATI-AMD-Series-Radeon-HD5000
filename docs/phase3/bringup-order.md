# Phase 3 hardware bring-up order

1. Driver loads with CP disabled.
2. Confirm VBIOS/ATOM log and CONFIG_MEMSIZE value.
3. Confirm GART aperture is non-zero and GART flush succeeds.
4. Copy Juniper firmware into the driver package.
5. Enable Phase 3 CP flag.
6. Confirm PFP/ME exact-size validation.
7. Confirm CP microcode upload completes.
8. Confirm `CP_RB_RPTR` advances after ME_INITIALIZE.
9. Ring self-test must change `SCRATCH_REG0` to `0xA55A5AA5`.
10. Fence self-test must write `0x13572468` to the mapped fence page.
11. Only after all 1–10 succeed should WDDM DMA submissions be enabled.
12. Only after stable DMA/fence operation should the real UMD work begin.
