#include "connect_to_twitch.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <cstddef>
#include <sstream>
#include <string_view>

#include "append_message.hpp"
#include "println.hpp"

namespace net = boost::asio;  // from <boost/asio.hpp>
using tcp = net::ip::tcp;     // from <boost/asio/ip/tcp.hpp>

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

// this is to hide the oauth in the terminal when the bot is started
#ifdef DONT_SHOW_OAUTH
    std::string censored_message = message_sent.str();
    constexpr std::string_view pass_oauth = "PASS oauth:";
    std::size_t oauth_start = censored_message.find(pass_oauth);
    std::size_t oauth_end = censored_message.find('\n', oauth_start);
    censored_message.replace(oauth_start + pass_oauth.length(),
                             oauth_end - (oauth_start + pass_oauth.length()),
                             " OAUTH NOT VISIBLE");
    println(censored_message);
#else
    println(message_sent);
#endif

    message_sent.str("");  // Empty the string buffer.
}