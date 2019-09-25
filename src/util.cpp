// Copyright 2019 Maximilian Schiller
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
