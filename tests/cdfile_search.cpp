#include "cdfile.h"

#include <cassert>
#include <fstream>
#include <string>
#include <sys/stat.h>

int RequiredCD = -2;
bool RunningAsDLL = false;

bool Force_CD_Available(int)
{
    return true;
}

void Prog_End(char const*, bool)
{
}

int main()
{
    const std::string fallback = "/tmp/ratouch-search-fallback";
    const std::string selected = "/tmp/ratouch-search-selected";
    mkdir(fallback.c_str(), 0755);
    mkdir(selected.c_str(), 0755);
    std::ofstream(fallback + "/main.mix") << "fallback";
    std::ofstream(selected + "/main.mix") << "selected";

    CDFileClass::Clear_Search_Drives();
    CDFileClass::Add_Search_Drive(fallback.c_str());
    CDFileClass::Add_Search_Drive(selected.c_str(), true);

    CDFileClass file("MAIN.MIX");
    assert(file.Is_Available());
    assert(file.File_Name() == selected + "/main.mix");
    return 0;
}
