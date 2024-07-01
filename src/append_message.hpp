#pragma once
#include <sstream>

#include "macros.hpp"

inline std::stringstream message_sent;

void
append_message(std::stringstream& message_sent, auto&&... arguments) {
    (message_sent << ... << fwd(arguments)) << '\n';
}