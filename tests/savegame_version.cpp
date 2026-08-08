#include <iostream>

#include "redalert/function.h"
#include "redalert/vortex.h"
#include "redalert/savegame_version.h"

#ifdef __APPLE__
static_assert(Red_Alert_Savegame_Version() == 0x01007E46, "base save ABI changed");
static_assert(Red_Alert_Savegame_Header_Version() == 0x01007E47, "save header changed");
#endif
static_assert(Is_Red_Alert_Savegame_Version_Compatible(Red_Alert_Savegame_Version()), "base ABI must load");
static_assert(Is_Red_Alert_Savegame_Version_Compatible(Red_Alert_Savegame_Header_Version()),
              "save header must load");
static_assert(!Is_Red_Alert_Savegame_Version_Compatible(Red_Alert_Savegame_Version() - 1),
              "older unknown ABI must fail");
static_assert(!Is_Red_Alert_Savegame_Version_Compatible(Red_Alert_Savegame_Header_Version() + 1),
              "newer unknown ABI must fail");

int main()
{
    std::cout << "base=0x" << std::hex << Red_Alert_Savegame_Version() << " header=0x"
              << Red_Alert_Savegame_Header_Version() << '\n';
    return 0;
}
