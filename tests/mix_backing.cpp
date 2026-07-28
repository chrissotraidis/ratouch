#include "ccfile.h"
#include "crc.h"
#include "mixfile.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

int RequiredCD = -2;
bool RunningAsDLL = false;

bool Force_CD_Available(int)
{
    return true;
}

void Prog_End(char const*, bool)
{
}

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

std::vector<unsigned char> BuildMix(const char* entry_name, const std::vector<unsigned char>& payload)
{
    std::vector<unsigned char> mix(18 + payload.size(), 0);
    Write16(mix, 0, 1);
    Write32(mix, 2, payload.size());
    Write32(mix, 6, Calculate_CRC(entry_name, std::strlen(entry_name)));
    Write32(mix, 10, 0);
    Write32(mix, 14, payload.size());
    std::copy(payload.begin(), payload.end(), mix.begin() + 18);
    return mix;
}

void Write(const std::string& path, const std::vector<unsigned char>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
}

int main()
{
    const std::string first = "/tmp/ratouch-mix-backing-first";
    const std::string second = "/tmp/ratouch-mix-backing-second";
    mkdir(first.c_str(), 0755);
    mkdir(second.c_str(), 0755);

    const std::vector<unsigned char> first_track = {'f', 'i', 'r', 's', 't'};
    const std::vector<unsigned char> second_track = {'o', 't', 'h', 'e', 'r'};
    Write(first + "/main.mix", BuildMix("SCORES.MIX", BuildMix("TRACK.AUD", first_track)));
    Write(second + "/main.mix", BuildMix("SCORES.MIX", BuildMix("TRACK.AUD", second_track)));

    CDFileClass::Clear_Search_Drives();
    CDFileClass::Add_Search_Drive(first.c_str(), true);
    auto* first_main = new MixFileClass<CCFileClass>("MAIN.MIX");
    auto* first_scores = new MixFileClass<CCFileClass>("SCORES.MIX");
    delete first_main;

    CDFileClass::Clear_Search_Drives();
    CDFileClass::Add_Search_Drive(second.c_str(), true);
    auto* second_main = new MixFileClass<CCFileClass>("MAIN.MIX");

    CCFileClass track("TRACK.AUD");
    assert(track.Open(READ));
    std::vector<unsigned char> actual(first_track.size());
    assert(track.Read(actual.data(), actual.size()) == actual.size());
    assert(actual == first_track);

    delete first_scores;
    delete second_main;
    return 0;
}
