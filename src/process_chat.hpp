#pragma once
#include <boost/beast/core.hpp>

inline constinit boost::static_string<2'048> message_receive(2'048, '\0');
 // `boost::static_string` does not have a `.contains()` member.
inline boost::string_view const message(message_receive);

void process_chat();
void console_message();