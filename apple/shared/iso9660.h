#ifndef RATOUCH_ISO9660_H
#define RATOUCH_ISO9660_H

#include <cstdint>
#include <string>
#include <vector>

struct RatouchIsoFile
{
    std::string Name;
    uint32_t Sector;
    uint32_t Size;
};

bool Ratouch_List_ISO_MIX_Files(const std::string& iso_path,
                                std::vector<RatouchIsoFile>& files,
                                std::string& error);
bool Ratouch_Extract_ISO_MIX_Files(const std::string& iso_path,
                                   const std::string& destination,
                                   std::vector<std::string>& extracted,
                                   std::string& error);

#endif
