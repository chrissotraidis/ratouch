#include "redalert/function.h"
#include "redalert/vortex.h"
#include "redalert/savegame_version.h"

#include <iostream>

static_assert(Red_Alert_Savegame_Version() == 0x01007E46, "base save ABI changed");
static_assert(Red_Alert_Savegame_Header_Version() == 0x01007E47, "save header changed");
static_assert(Is_Red_Alert_Savegame_Version_Compatible(0x01007E46), "base ABI must load");
static_assert(Is_Red_Alert_Savegame_Version_Compatible(0x01007E47), "Aftermath header must load");
static_assert(!Is_Red_Alert_Savegame_Version_Compatible(0x01007E45), "older unknown ABI must fail");
static_assert(!Is_Red_Alert_Savegame_Version_Compatible(0x01007E48), "newer unknown ABI must fail");

int main()
{
    std::cout << "base=0x" << std::hex << Red_Alert_Savegame_Version() << " header=0x"
              << Red_Alert_Savegame_Header_Version() << '\n';
    return 0;
}
