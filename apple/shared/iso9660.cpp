#include "iso9660.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace
{
const uint32_t SectorSize = 2048;
const uint32_t MaximumDirectorySize = 64 * 1024 * 1024;

int Create_Directory(const std::string& path)
{
#ifdef _WIN32
    return _mkdir(path.c_str());
#else
    return mkdir(path.c_str(), 0755);
#endif
}

uint32_t Read32(const unsigned char* bytes)
{
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8)
           | (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

std::string Clean_Name(const unsigned char* bytes, size_t length)
{
    std::string name(reinterpret_cast<const char*>(bytes), length);
    const size_t version = name.find(';');
    if (version != std::string::npos) {
        name.erase(version);
    }
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char value) {
        return static_cast<char>(std::toupper(value));
    });
    return name;
}

bool Is_Mix(const std::string& name)
{
    return name.size() > 4 && name.compare(name.size() - 4, 4, ".MIX") == 0;
}

bool Read_At(std::ifstream& stream, uint64_t offset, void* destination, size_t length)
{
    stream.clear();
    stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    stream.read(static_cast<char*>(destination), static_cast<std::streamsize>(length));
    return stream.good() || static_cast<size_t>(stream.gcount()) == length;
}

bool Read_Directory(std::ifstream& stream,
                    uint32_t sector,
                    uint32_t size,
                    unsigned depth,
                    std::set<uint32_t>& visited,
                    std::vector<RatouchIsoFile>& files,
                    std::string& error)
{
    if (depth > 16 || size > MaximumDirectorySize || !visited.insert(sector).second) {
        return true;
    }
    std::vector<unsigned char> data(size);
    if (!Read_At(stream, static_cast<uint64_t>(sector) * SectorSize, data.data(), data.size())) {
        error = "Could not read an ISO directory.";
        return false;
    }

    size_t offset = 0;
    while (offset < data.size()) {
        const size_t recordLength = data[offset];
        if (recordLength == 0) {
            offset = ((offset / SectorSize) + 1) * SectorSize;
            continue;
        }
        if (recordLength < 34 || offset + recordLength > data.size()) {
            error = "The ISO contains a malformed directory record.";
            return false;
        }
        const unsigned char* record = data.data() + offset;
        const uint32_t entrySector = Read32(record + 2);
        const uint32_t entrySize = Read32(record + 10);
        const bool directory = (record[25] & 0x02) != 0;
        const size_t nameLength = record[32];
        if (33 + nameLength > recordLength) {
            error = "The ISO contains a malformed file name.";
            return false;
        }
        const bool dotEntry = nameLength == 1 && (record[33] == 0 || record[33] == 1);
        if (!dotEntry) {
            const std::string name = Clean_Name(record + 33, nameLength);
            if (directory) {
                if (!Read_Directory(stream, entrySector, entrySize, depth + 1, visited, files, error)) {
                    return false;
                }
            } else if (Is_Mix(name)) {
                files.push_back({name, entrySector, entrySize});
            }
        }
        offset += recordLength;
    }
    return true;
}
}

bool Ratouch_List_ISO_MIX_Files(const std::string& iso_path,
                                std::vector<RatouchIsoFile>& files,
                                std::string& error)
{
    files.clear();
    error.clear();
    std::ifstream stream(iso_path, std::ios::binary);
    if (!stream) {
        error = "The ISO could not be opened.";
        return false;
    }

    std::vector<unsigned char> descriptor(SectorSize);
    uint32_t rootSector = 0;
    uint32_t rootSize = 0;
    for (uint32_t sector = 16; sector < 80; ++sector) {
        if (!Read_At(stream, static_cast<uint64_t>(sector) * SectorSize, descriptor.data(), descriptor.size())) {
            break;
        }
        if (std::memcmp(descriptor.data() + 1, "CD001", 5) != 0) {
            continue;
        }
        if (descriptor[0] == 1) {
            const unsigned char* root = descriptor.data() + 156;
            if (root[0] < 34) {
                error = "The ISO primary volume has no usable root directory.";
                return false;
            }
            rootSector = Read32(root + 2);
            rootSize = Read32(root + 10);
            break;
        }
        if (descriptor[0] == 255) {
            break;
        }
    }
    if (rootSector == 0 || rootSize == 0) {
        error = "This is not a supported ISO9660 image.";
        return false;
    }

    std::set<uint32_t> visited;
    if (!Read_Directory(stream, rootSector, rootSize, 0, visited, files, error)) {
        return false;
    }
    if (files.empty()) {
        error = "No MIX files were found in the ISO.";
        return false;
    }
    return true;
}

bool Ratouch_Extract_ISO_MIX_Files(const std::string& iso_path,
                                   const std::string& destination,
                                   std::vector<std::string>& extracted,
                                   std::string& error)
{
    std::vector<RatouchIsoFile> files;
    if (!Ratouch_List_ISO_MIX_Files(iso_path, files, error)) {
        return false;
    }
    if (Create_Directory(destination) != 0 && errno != EEXIST) {
        error = "The import staging directory could not be created.";
        return false;
    }

    std::ifstream input(iso_path, std::ios::binary);
    std::vector<char> buffer(1024 * 1024);
    extracted.clear();
    for (const RatouchIsoFile& file : files) {
        const std::string temporary = destination + "/." + file.Name + ".tmp";
        const std::string target = destination + "/" + file.Name;
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            error = "A staged MIX file could not be created.";
            return false;
        }
        input.clear();
        input.seekg(static_cast<std::streamoff>(static_cast<uint64_t>(file.Sector) * SectorSize), std::ios::beg);
        uint32_t remaining = file.Size;
        while (remaining > 0) {
            const size_t chunk = std::min<size_t>(buffer.size(), remaining);
            input.read(buffer.data(), static_cast<std::streamsize>(chunk));
            if (static_cast<size_t>(input.gcount()) != chunk) {
                error = "A MIX file is truncated inside the ISO.";
                return false;
            }
            output.write(buffer.data(), static_cast<std::streamsize>(chunk));
            if (!output) {
                error = "A staged MIX file could not be written.";
                return false;
            }
            remaining -= static_cast<uint32_t>(chunk);
        }
        output.close();
        std::remove(target.c_str());
        if (std::rename(temporary.c_str(), target.c_str()) != 0) {
            error = "A staged MIX file could not be finalized.";
            return false;
        }
        extracted.push_back(file.Name);
    }
    return true;
}
