#include "wsa.h"

#include <cassert>

int main()
{
    constexpr unsigned int legacy_header_size = 37;

    assert(WSA_Delta_Payload_Size(legacy_header_size + 4096, 2048) == 4096);
    assert(WSA_Delta_Payload_Size(legacy_header_size + 307, 2928) == 2928);
    assert(WSA_Delta_Payload_Size(legacy_header_size, 512) == 512);
    assert(WSA_Delta_Payload_Size(0, 0) == 0);
    return 0;
}
