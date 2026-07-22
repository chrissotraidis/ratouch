#include "asset_manifest.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    std::vector<RatouchAssetIdentity> complete;
    for (const RatouchKnownAsset& known : Ratouch_Steam_2229840_Manifest()) {
        RatouchAssetIdentity asset = {known.Name, known.Size, known.SHA256};
        std::string target;
        std::string error;
        assert(Ratouch_Resolve_Asset_Target(asset, target, error));
        assert(target == known.RelativePath);
        assert(error.empty());
        complete.push_back(asset);
    }
    assert(Ratouch_Is_Complete_Steam_2229840(complete));

    RatouchAssetIdentity wrongMain = complete[4];
    ++wrongMain.Size;
    std::string target;
    std::string error;
    assert(!Ratouch_Resolve_Asset_Target(wrongMain, target, error));
    assert(!error.empty());

    const RatouchAssetIdentity unknown = {"custom.mix", 123, "abc"};
    assert(Ratouch_Resolve_Asset_Target(unknown, target, error));
    assert(target == "CUSTOM.MIX");

    complete.pop_back();
    assert(!Ratouch_Is_Complete_Steam_2229840(complete));
    return 0;
}
