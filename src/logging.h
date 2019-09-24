#pragma once
#include <fmt/color.h>
#include <fmt/core.h>

namespace eo::log {
template<typename... Args>
void info(Args &&... args)
{
    fmt::print("[{1}]: {0}\n", fmt::format(std::forward<Args>(args)...), fmt::format(fg(fmt::terminal_color::bright_green), "INFO"));
}

template<typename... Args>
void error(Args &&... args)
{
    fmt::print("[{1}]: {0}\n", fmt::format(std::forward<Args>(args)...), fmt::format(fg(fmt::terminal_color::bright_red), "ERROR"));
}
}
