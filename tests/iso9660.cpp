#include "iso9660.h"
#include "test_directory.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

namespace
{
const size_t Sector = 2048;

void Write32(unsigned char* destination, uint32_t value)
{
    destination[0] = value & 0xFF;
    destination[1] = (value >> 8) & 0xFF;
    destination[2] = (value >> 16) & 0xFF;
    destination[3] = (value >> 24) & 0xFF;
}

void Write16(unsigned char* destination, uint16_t value)
{
    destination[0] = value & 0xFF;
    destination[1] = (value >> 8) & 0xFF;
}

size_t Directory_Record(unsigned char* destination,
                        uint32_t sector,
                        uint32_t size,
                        unsigned char flags,
                        const unsigned char* name,
                        size_t nameLength)
{
    const size_t length = 33 + nameLength + (nameLength % 2 == 0 ? 1 : 0);
    std::memset(destination, 0, length);
    destination[0] = static_cast<unsigned char>(length);
    Write32(destination + 2, sector);
    Write32(destination + 10, size);
    destination[25] = flags;
    destination[32] = static_cast<unsigned char>(nameLength);
    std::memcpy(destination + 33, name, nameLength);
    return length;
}
}

int main()
{
    const char* isoPath = "ratouch-iso9660-test.iso";
    const char* destination = "ratouch-iso9660-test-out";
    Ratouch_Test_Create_Directory(destination);
    std::remove("ratouch-iso9660-test-out/REDALERT.MIX");

    std::vector<unsigned char> image(24 * Sector, 0);
    unsigned char* primary = image.data() + 16 * Sector;
    primary[0] = 1;
    std::memcpy(primary + 1, "CD001", 5);
    primary[6] = 1;
    const unsigned char dot = 0;
    Directory_Record(primary + 156, 20, Sector, 2, &dot, 1);

    unsigned char* terminator = image.data() + 17 * Sector;
    terminator[0] = 255;
    std::memcpy(terminator + 1, "CD001", 5);
    terminator[6] = 1;

    unsigned char* directory = image.data() + 20 * Sector;
    size_t offset = Directory_Record(directory, 20, Sector, 2, &dot, 1);
    const unsigned char parent = 1;
    offset += Directory_Record(directory + offset, 20, Sector, 2, &parent, 1);
    const char mixName[] = "REDALERT.MIX;1";
    Directory_Record(directory + offset, 21, 30, 0, reinterpret_cast<const unsigned char*>(mixName), sizeof(mixName) - 1);
    unsigned char* mix = image.data() + 21 * Sector;
    Write16(mix, 1);
    Write32(mix + 2, 12);
    Write32(mix + 6, 0x12345678);
    Write32(mix + 10, 0);
    Write32(mix + 14, 12);
    std::memcpy(mix + 18, "MIX-CONTENT!", 12);

    std::ofstream iso(isoPath, std::ios::binary | std::ios::trunc);
    iso.write(reinterpret_cast<const char*>(image.data()), image.size());
    iso.close();

    std::vector<RatouchIsoFile> files;
    std::string error;
    assert(Ratouch_List_ISO_MIX_Files(isoPath, files, error));
    assert(files.size() == 1);
    assert(files[0].Name == "REDALERT.MIX");

    std::vector<std::string> extracted;
    assert(Ratouch_Extract_ISO_MIX_Files(isoPath, destination, extracted, error));
    assert(extracted.size() == 1);
    std::ifstream result("ratouch-iso9660-test-out/REDALERT.MIX", std::ios::binary);
    std::string contents((std::istreambuf_iterator<char>(result)), std::istreambuf_iterator<char>());
    assert(contents.size() == 30);
    assert(contents.substr(18) == "MIX-CONTENT!");
    return 0;
}
