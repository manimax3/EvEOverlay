#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <string_view>

namespace eo {
using int32  = std::int32_t;
using uint   = unsigned int;
using uint32 = std::uint32_t;
using int8   = std::int8_t;
using uint8  = std::uint8_t;
using byte   = uint8;

namespace math = glm;

std::string get_cwd();
std::string get_exe_dir();

constexpr const char *data_folder        = "data/";
constexpr const char *settings_file      = "settings.json";
inline const auto     settings_file_path = get_exe_dir() + data_folder + settings_file;
}
