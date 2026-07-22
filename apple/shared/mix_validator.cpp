#include "mix_validator.h"

#include <cstdint>
#include <fstream>
#include <vector>

namespace
{
uint16_t Read16(const unsigned char* bytes)
{
    return static_cast<uint16_t>(bytes[0]) | static_cast<uint16_t>(bytes[1] << 8);
}

uint32_t Read32(const unsigned char* bytes)
{
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8)
           | (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

bool Validate_Classic(std::ifstream& stream, uint64_t fileSize, uint64_t headerOffset, uint64_t checksumBytes)
{
    unsigned char header[6];
    stream.clear();
    stream.seekg(static_cast<std::streamoff>(headerOffset), std::ios::beg);
    stream.read(reinterpret_cast<char*>(header), sizeof(header));
    if (stream.gcount() != sizeof(header)) {
        return false;
    }

    const uint16_t count = Read16(header);
    const uint32_t dataSize = Read32(header + 2);
    if (count == 0 || dataSize == 0) {
        return false;
    }
    const uint64_t dataOffset = headerOffset + sizeof(header) + static_cast<uint64_t>(count) * 12;
    if (dataOffset > fileSize || dataSize > fileSize - dataOffset || checksumBytes > fileSize - dataOffset - dataSize) {
        return false;
    }

    std::vector<unsigned char> index(static_cast<size_t>(count) * 12);
    stream.read(reinterpret_cast<char*>(index.data()), static_cast<std::streamsize>(index.size()));
    if (static_cast<size_t>(stream.gcount()) != index.size()) {
        return false;
    }
    for (size_t offset = 0; offset < index.size(); offset += 12) {
        const uint32_t fileOffset = Read32(index.data() + offset + 4);
        const uint32_t entrySize = Read32(index.data() + offset + 8);
        if (fileOffset > dataSize || entrySize > dataSize - fileOffset) {
            return false;
        }
    }
    return true;
}
}

bool Ratouch_Validate_MIX(const std::string& path, std::string& error)
{
    error.clear();
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        error = "The MIX file could not be opened.";
        return false;
    }
    const std::streamoff end = stream.tellg();
    if (end < 6) {
        error = "The MIX file is truncated.";
        return false;
    }
    const uint64_t fileSize = static_cast<uint64_t>(end);
    stream.seekg(0, std::ios::beg);

    if (Validate_Classic(stream, fileSize, 0, 0)) {
        return true;
    }

    unsigned char flagBytes[4];
    stream.clear();
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(flagBytes), sizeof(flagBytes));
    const uint32_t flags = Read32(flagBytes);
    const bool checksum = (flags & 0x00010000) != 0;
    const bool encrypted = (flags & 0x00020000) != 0;
    if ((flags & ~0x00030000U) != 0 || (!checksum && !encrypted)) {
        error = "The file does not have a recognized MIX header.";
        return false;
    }

    if (encrypted) {
        if (fileSize < 4 + 80 + 8 + (checksum ? 20 : 0)) {
            error = "The encrypted MIX header is truncated.";
            return false;
        }
        return true;
    }
    if (!Validate_Classic(stream, fileSize, 4, checksum ? 20 : 0)) {
        error = "The MIX index points outside the file.";
        return false;
    }
    return true;
}
