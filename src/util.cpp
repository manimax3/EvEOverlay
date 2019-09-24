#include "util.h"
#include <array>
#include <cstring>

#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif

std::string eo::get_cwd()
{
#ifdef __linux__
    std::array<char, 512> data;
    std::memset(data.data(), 0, 512);
    getcwd(data.data(), 512);
    return data.data();
#else
    static_assert(false, "This OS is currently no supported");
#endif
}

std::string eo::get_exe_dir()
{
#ifdef __linux__
    std::array<char, 512> data;
    std::memset(data.data(), 0, 512);
    readlink("/proc/self/exe", data.data(), 512);
    std::string path(data.data());
    const auto  ep = path.substr(0, path.find_last_of('/') + 1);
    mkdir(ep.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return ep;
#else
    static_assert(false, "This OS is currently no supported");
#endif
}
