#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include "boost/utility/string_view_fwd.hpp"

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>
using system_clock = std::chrono::system_clock;

constexpr std::string_view host = "irc.chat.twitch.tv";
constexpr std::string_view port = "6667";
constexpr std::string_view oauth = "oauth:" OAUTH;
constexpr std::string_view bot_username = BOTNAME;
constexpr std::string_view twitch_channel = CHANNEL;

#define fwd(x) ::std::forward<decltype(x)>(x)

inline std::stringstream message_sent;
inline constinit boost::static_string<2'048> message_receive(2'048, '\0');

// The io_context is required for all I/O
inline net::io_context ioc;
// These objects perform our I/O
inline tcp::resolver resolver(ioc);
inline beast::tcp_stream stream(ioc);

void
println(std::string_view str) {
    auto const now = system_clock::now();
    std::time_t const t_c = system_clock::to_time_t(now);
    // TODO: Replace this with fmt or something.
    std::cout << std::ctime(&t_c) << str << '\n';
}

void
println(auto&& str)
    requires(requires { str.str(); })
{
    auto const now = system_clock::now();
    std::time_t const t_c = system_clock::to_time_t(now);

    std::cout << std::ctime(&t_c) << fwd(str).str() << '\n';
}

void
append_message(std::stringstream& message_sent, auto&&... arguments) {
    (message_sent << ... << fwd(arguments)) << '\n';
}

// This is to write a PRIVMSG to the chat
void
write_message(boost::string_view message) {
    append_message(message_sent, "PRIVMSG #", twitch_channel, " :", message);

    stream.write_some(boost::asio::buffer(message_sent.str()));
    println(message_sent);
    message_sent.str("");  // Empty the string buffer
}

void
console_message() {
    std::string console;

    getline(std::cin, console);
    write_message(console);
}

// this reads the chat and send a few responses based on basic text parsing
void
process_chat() {
    auto payload_wrapper = boost::asio::buffer(message_receive);
    stream.read_some(payload_wrapper);

    // `boost::static_string` does not have a `.contains()` member.
    boost::string_view const message(message_receive);

    if (message.contains("tmi.twitch.tv")) {
        if (message.contains("tmi.twitch.tv PRIVMSG")) {
            // TODO: Parse message info here
            // get the username
            std::string display_name = "display-name=";
            int chat_user_pos = message.find(display_name);
            assert(chat_user_pos != message.npos);
            int slash_pos =
                message.find(';', chat_user_pos + display_name.size());
            std::string chat_user(
                message.data() + chat_user_pos + display_name.size(),
                slash_pos - (chat_user_pos + display_name.size()));

            // make user name lower case
            std::string chat_user_lower = chat_user;
            std::transform(chat_user.begin(), chat_user.end(),
                           chat_user_lower.begin(), ::tolower);

            // parse out chat message
            std::string chat_start_user =
                "#" + std::string(twitch_channel) + " :";
            int chat_start = message.find(chat_start_user);
            std::string chat_msg(
                message.data() + chat_start + chat_start_user.size(), 512);

            // plain text message
            println(chat_user + ": " + chat_msg);
        } else {
            println(message_receive);
        }

        // this is to respond to the "keep alive" check
        if (message.contains("PING :tmi.twitch.tv")) {
            append_message(message_sent, "PONG :tmi.twitch.tv");

            stream.write_some(boost::asio::buffer(message_sent.str()));
            println(message_sent);
            message_sent.str("");  // Empty the string buffer
        }

        // this clears the read buffer and resets size
        std::memset(message_receive.data(), '\0', message_receive.size());
    }
}

void
connect_to_twitch() {
    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    auto _ = stream.connect(results);

    append_message(message_sent, "PASS ", oauth);
    append_message(message_sent, "NICK ", bot_username);
    append_message(message_sent, "USER ", bot_username, "8 * :", bot_username);
    append_message(message_sent, "JOIN #", twitch_channel);
    append_message(message_sent, "CAP REQ :twitch.tv/tags");
    append_message(message_sent, "CAP REQ :twitch.tv/commands");
    append_message(message_sent, "CAP REQ :twitch.tv/membership");

    stream.write_some(boost::asio::buffer(message_sent.str()));
    println(message_sent);
    message_sent.str("");  // Empty the string buffer.
}

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
