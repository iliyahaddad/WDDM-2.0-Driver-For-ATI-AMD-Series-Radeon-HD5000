# Host-side tests (gcc, no WDK)

These compile `driver/Vidpn.c` / `driver/Display.c` against **stub** WDK headers (`wdk_stub.h`) and a mock of
the Dxgkrnl VidPN interface that counts every acquire/release. They check control flow and resource balance
(no leaked paths / mode sets / mode infos on success and error paths), NOT conformance with real Dxgkrnl.
The stubs encode member names from memory of the WDK headers; a real WDK build is the real test.

    gcc -fms-extensions -Wno-multichar -I tests/host -I include -include tests/host/wdk_stub.h \
        -o vidpn_test tests/host/vidpn_mock_test.c driver/Vidpn.c && ./vidpn_test
    gcc -fms-extensions -Wno-multichar -I tests/host -I include -include tests/host/wdk_stub.h \
        -o display_test tests/host/display_test.c driver/Display.c && ./display_test

    gcc -fms-extensions -Wno-multichar -I tests/host -I include -include tests/host/wdk_stub.h \
        -o memory_test tests/host/memory_test.c driver/Memory.c && ./memory_test

`memory_test.c` covers `MadisonMemoryQuerySegments` (count call, detail call, Agp-only flag, too-small buffer, oversize aperture).
