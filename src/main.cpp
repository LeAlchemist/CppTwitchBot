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
