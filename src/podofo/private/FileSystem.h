#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#if defined(__GNUC__) && !defined(__llvm__) && __GNUC__ < 8
// gcc < 8 put std::filesystem in experimental namespace
// Also needs -lstdc++fs to link before gcc 9
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#endif // FILESYSTEM_H
