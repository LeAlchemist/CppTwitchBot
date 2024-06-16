#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <iostream>
#include <sstream>

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

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
    // TODO: Replace this with fmt or something.
    std::cout << str << '\n';
}

void
println(auto&& str)
    requires(requires { str.str(); })
{
    std::cout << fwd(str).str() << '\n';
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

// this reads the chat and send a few responses based on basic text parsing
void
process_chat() {
    auto payload_wrapper = boost::asio::buffer(message_receive);
    stream.read_some(payload_wrapper);

    // `boost::static_string` does not have a `.contains()` member.
    boost::string_view const message(message_receive);

    if (message.contains("tmi.twitch.tv")) {
        println(message_receive);

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

    // Make message.
    write_message("this is a message");

    write_message("this is a second message");

    while (true) {
        process_chat();
    }
}