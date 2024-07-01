#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <cassert>
#include <fmt/color.h>

#include <thread>

#include "connect_to_twitch.hpp"
#include "process_chat.hpp"



auto
main() -> int {
    connect_to_twitch();

    std::jthread input([&] {
        while (true) {
            console_message();
        }
    });

    while (true) {
        process_chat();
    }
}
