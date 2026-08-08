#ifndef RATOUCH_TEST_DIRECTORY_H
#define RATOUCH_TEST_DIRECTORY_H

#include <string>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

inline void Ratouch_Test_Create_Directory(const std::string& path)
{
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

#endif
