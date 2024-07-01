#pragma once
#include <boost/beast/core.hpp>

inline constinit boost::static_string<2'048> message_receive(2'048, '\0');

void process_chat();
void console_message();