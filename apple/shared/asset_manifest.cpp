#include "asset_manifest.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace
{
std::string Uppercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}
}

const std::vector<RatouchKnownAsset>& Ratouch_Steam_2229840_Manifest()
{
    static const std::vector<RatouchKnownAsset> manifest = {
        {"EXPAND.MIX", 458242, "e144753593161f867a26428f901a09de5cb8a71bab6e690dd9ca727e76d4f724", "EXPAND.MIX"},
        {"EXPAND2.MIX", 469922, "e379b23ce6c7af9d4f7469e10b788210124cb92e8b6e3978b9569802edfdfc9a", "EXPAND2.MIX"},
        {"HIRES1.MIX", 90264, "48c407f80f1fdbc86ac2689c00339927b2127a758871e998c9942b7a7d93e07d", "HIRES1.MIX"},
        {"LORES1.MIX", 57076, "5b83e8d731fc78041647f19adbb82f4e71cc09b837d2492b8f2a0520e11de641", "LORES1.MIX"},
        {"MAIN1.MIX", 454605294, "512beab10095f2422498f16ce468fca613bf6bec2a6257bbc18a1d01691d1482", "allied/MAIN.MIX"},
        {"MAIN2.MIX", 500577414, "cbcddf7fc75b2924728a2698307b97605557528b99e5f175d3d5de8f6c96de3c", "soviet/MAIN.MIX"},
        {"MAIN3.MIX", 236034607, "f747654b0ff09584086460d1b74ba400c2cc4dd9d467e27ca38c99443c45dae2", "counterstrike/MAIN.MIX"},
        {"MAIN4.MIX", 270673111, "1bc68a3509a762730a7bfedca132413a5d2ab3a883da8dbb4233b32366baa72f", "aftermath/MAIN.MIX"},
        {"REDALERT.MIX", 25046328, "ad5ad68a08d1d6bb073324e91beb02543deeb9e1b8dca922f3cef768dda07b53", "REDALERT.MIX"},
        {"WOLAPI.MIX", 211145, "aba7915e3c5c24ef95d55dccca402ee3bde1e6bdcc927e6398831328a63981a3", "WOLAPI.MIX"},
    };
    return manifest;
}

const RatouchKnownAsset* Ratouch_Match_Steam_2229840_Asset(const RatouchAssetIdentity& asset)
{
    const std::string name = Uppercase(asset.Name);
    const std::string hash = Uppercase(asset.SHA256);
    for (const RatouchKnownAsset& known : Ratouch_Steam_2229840_Manifest()) {
        if (name == known.Name && asset.Size == known.Size && hash == Uppercase(known.SHA256)) {
            return &known;
        }
    }
    return nullptr;
}

bool Ratouch_Resolve_Asset_Target(const RatouchAssetIdentity& asset,
                                  std::string& relative_path,
                                  std::string& error)
{
    relative_path.clear();
    error.clear();
    const std::string name = Uppercase(asset.Name);
    if (const RatouchKnownAsset* known = Ratouch_Match_Steam_2229840_Asset(asset)) {
        relative_path = known->RelativePath;
        return true;
    }

    for (const RatouchKnownAsset& known : Ratouch_Steam_2229840_Manifest()) {
        if (name == known.Name) {
            error = name + " does not match the known Steam 2229840 size and SHA-256.";
            return false;
        }
    }

    relative_path = name;
    return true;
}

bool Ratouch_Is_Complete_Steam_2229840(const std::vector<RatouchAssetIdentity>& assets)
{
    std::set<std::string> matches;
    for (const RatouchAssetIdentity& asset : assets) {
        if (const RatouchKnownAsset* known = Ratouch_Match_Steam_2229840_Asset(asset)) {
            matches.insert(known->Name);
        }
    }
    return matches.size() == Ratouch_Steam_2229840_Manifest().size();
}
