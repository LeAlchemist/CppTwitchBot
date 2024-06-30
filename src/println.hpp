#pragma once
#include <chrono>
#include <fmt/core.h>

#include "macros.hpp"

using system_clock = std::chrono::system_clock;

inline void
println(std::string_view str) {
    auto const now = system_clock::now();
    std::time_t const t_c = system_clock::to_time_t(now);

    fmt::print("{}{}\n", std::ctime(&t_c), str);
}

void
println(auto&& str)
    requires(requires { str.str(); })
{
    auto const now = system_clock::now();
    std::time_t const t_c = system_clock::to_time_t(now);

    fmt::print("{}{}\n", std::ctime(&t_c), fwd(str).str());
}