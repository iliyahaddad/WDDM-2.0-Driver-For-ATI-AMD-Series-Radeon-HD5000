# UMD integration boundary

A full Direct3D 10/11 UMD is intentionally **not fabricated** in this phase. A real UMD must implement the Microsoft D3D10/D3D11 UMD DDI tables and command validation contract and must match the KMD allocation/render/present protocol.

The KMD package is now buildable as a real WDK project and the INF/package boundary is present. The next UMD phase should be based on the WDK's D3D UMD interfaces (`d3d10umddi.h`, `d3d11umddi.h`) and the KMD's actual allocation and render protocol.

Do not register a DLL that only exports `DllMain` as a production UMD: Dxgkrnl/Direct3D will expect the correct `OpenAdapter*` entry point and function tables.
