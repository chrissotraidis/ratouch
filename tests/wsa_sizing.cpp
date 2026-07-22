#include "wsa.h"

#include <cassert>

int main()
{
    constexpr unsigned int legacy_header_size = 37;

    assert(WSA_Delta_Payload_Size(legacy_header_size + 4096, 2048) == 4096);
    assert(WSA_Delta_Payload_Size(legacy_header_size + 307, 2928) == 2928);
    assert(WSA_Delta_Payload_Size(legacy_header_size, 512) == 512);
    assert(WSA_Delta_Payload_Size(0, 0) == 0);

    assert(WSA_Delta_Range_Is_Valid(128, 640, 1024, 2048));
    assert(!WSA_Delta_Range_Is_Valid(0, 640, 1024, 2048));
    assert(!WSA_Delta_Range_Is_Valid(640, 128, 1024, 2048));
    assert(!WSA_Delta_Range_Is_Valid(128, 1200, 1024, 2048));
    assert(!WSA_Delta_Range_Is_Valid(128, 640, 1024, 512));
    return 0;
}
