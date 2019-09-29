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

#pragma once
#include <cstdint>
#include <string>
#include <string_view>

#include <boost/core/ignore_unused.hpp>

namespace eo {
using int32  = std::int32_t;
using uint   = unsigned int;
using uint32 = std::uint32_t;
using int8   = std::int8_t;
using uint8  = std::uint8_t;
using byte   = uint8;

std::string get_cwd();
std::string get_exe_dir();

constexpr const char *data_folder        = "data/";
constexpr const char *settings_file      = "settings.json";
inline const auto     settings_file_path = get_exe_dir() + data_folder + settings_file;

template<typename F>
struct scope_exit {
    scope_exit(F f)
        : f_(f)
    {
    }
    ~scope_exit() { f_(); }

private:
    F f_;
};
}
