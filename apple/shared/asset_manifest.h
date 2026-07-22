#ifndef RATOUCH_ASSET_MANIFEST_H
#define RATOUCH_ASSET_MANIFEST_H

#include <cstdint>
#include <string>
#include <vector>

struct RatouchAssetIdentity
{
    std::string Name;
    uint64_t Size;
    std::string SHA256;
};

struct RatouchKnownAsset
{
    const char* Name;
    uint64_t Size;
    const char* SHA256;
    const char* RelativePath;
};

const std::vector<RatouchKnownAsset>& Ratouch_Steam_2229840_Manifest();
const RatouchKnownAsset* Ratouch_Match_Steam_2229840_Asset(const RatouchAssetIdentity& asset);
bool Ratouch_Resolve_Asset_Target(const RatouchAssetIdentity& asset,
                                  std::string& relative_path,
                                  std::string& error);
bool Ratouch_Is_Complete_Steam_2229840(const std::vector<RatouchAssetIdentity>& assets);

#endif
