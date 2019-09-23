#include "util.h"
#include <array>
#include <boost/predef.h>
#include <cstring>

#if BOOST_OS_LINUX
#include <unistd.h>
#endif

std::string eo::get_cwd()
{
#if BOOST_OS_LINUX
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
#if BOOST_OS_LINUX
    std::array<char, 512> data;
    std::memset(data.data(), 0, 512);
    readlink("/proc/self/exe", data.data(), 512);
    std::string path(data.data());
    const auto  ep = path.substr(0, path.find_last_of('/') + 1);
    return ep;
#else
    static_assert(false, "This OS is currently no supported");
#endif
}
