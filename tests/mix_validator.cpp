#include "mix_validator.h"

#include <cassert>
#include <fstream>
#include <string>
#include <vector>

namespace
{
void Write16(std::vector<unsigned char>& bytes, size_t offset, uint16_t value)
{
    bytes[offset] = value & 0xFF;
    bytes[offset + 1] = (value >> 8) & 0xFF;
}

void Write32(std::vector<unsigned char>& bytes, size_t offset, uint32_t value)
{
    bytes[offset] = value & 0xFF;
    bytes[offset + 1] = (value >> 8) & 0xFF;
    bytes[offset + 2] = (value >> 16) & 0xFF;
    bytes[offset + 3] = (value >> 24) & 0xFF;
}

void Write(const char* path, const std::vector<unsigned char>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
}

int main()
{
    std::string error;
    std::vector<unsigned char> classic(30, 0);
    Write16(classic, 0, 1);
    Write32(classic, 2, 12);
    Write32(classic, 6, 0x12345678);
    Write32(classic, 10, 0);
    Write32(classic, 14, 12);
    Write("/tmp/ratouch-valid.mix", classic);
    assert(Ratouch_Validate_MIX("/tmp/ratouch-valid.mix", error));

    classic.resize(20);
    Write("/tmp/ratouch-truncated.mix", classic);
    assert(!Ratouch_Validate_MIX("/tmp/ratouch-truncated.mix", error));

    std::vector<unsigned char> encrypted(112, 0);
    Write32(encrypted, 0, 0x00020000);
    Write("/tmp/ratouch-encrypted.mix", encrypted);
    assert(Ratouch_Validate_MIX("/tmp/ratouch-encrypted.mix", error));

    Write("/tmp/ratouch-junk.mix", std::vector<unsigned char>(64, 0x55));
    assert(!Ratouch_Validate_MIX("/tmp/ratouch-junk.mix", error));
    return 0;
}
