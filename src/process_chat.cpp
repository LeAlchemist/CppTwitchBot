#include "process_chat.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <fmt/color.h>
#include <iostream>
#include <sstream>
#include <string>

#include "append_message.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/utility/string_view_fwd.hpp"
#include "connect_to_twitch.hpp"
#include "println.hpp"

using boost::bad_lexical_cast;
using boost::lexical_cast;

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
    std::getline(std::cin, console);
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
            // get the username
            std::string display_name = "display-name=";
            std::size_t chat_user_start = message.find(display_name);
            assert(chat_user_start != message.npos);
            std::size_t chat_user_end =
                message.find(';', chat_user_start + display_name.size());
            std::string chat_user(
                message.data() + chat_user_start + display_name.size(),
                chat_user_end - (chat_user_start + display_name.size()));

            // make user name lower case
            std::string chat_user_lower = chat_user;
            std::transform(chat_user.begin(), chat_user.end(),
                           chat_user_lower.begin(), ::tolower);

            // get username color
            // twitch/irc uses a hex code value here
            std::string display_name_color = "color=#";
            std::size_t user_color_start = message.find(display_name_color);
            assert(user_color_start != message.npos);
            std::size_t user_color_end =
                message.find(';', user_color_start + display_name_color.size());
            std::string user_color(
                message.data() + user_color_start + display_name_color.size(),
                user_color_end -
                    (user_color_start + display_name_color.size()));
            int hex;
            // if user doesn't have an assigned color
            if (user_color.empty()) {
                std::string temp_color = "ccff00";
                auto _ = std::from_chars(temp_color.data(),
                                         temp_color.data() + 6, hex, 16);
            }
            auto _ = std::from_chars(user_color.data(), user_color.data() + 6,
                                     hex, 16);

            // get if user is subbed
            // checking to see if "subscriber=" is true or false
            std::string subscriber = "subscriber=";
            std::size_t subscriber_start = message.find(subscriber);
            assert(subscriber_start != message.npos);
            std::size_t subscriber_end =
                message.find(';', subscriber_start + subscriber.size());
            std::string subbed(
                message.data() + subscriber_start + subscriber.size(),
                subscriber_end - (subscriber_start + subscriber.size()));
            [[maybe_unused]]
            bool issubbed = lexical_cast<bool>(subbed);

            // get if user is broadcaster
            // this could also be modified for other terms such as "subscriber",
            // "premium"... found here https://dev.twitch.tv/docs/irc/tags/
            std::string badges = "badges=";
            std::size_t badges_start = message.find(badges);
            assert(badges_start != message.npos);
            std::size_t badges_end =
                message.find(';', badges_start + badges.size());
            std::string badges_type(
                message.data() + badges_start + badges.size(),
                badges_end - (badges_start + badges.size()));
            // this checks to see if "badges=" contains a term for "broadcaster"
            [[maybe_unused]]
            bool isbroadcaster =
                lexical_cast<bool>(badges_type.contains("broadcaster"));

            // get if user is a moderator
            // checking to see if "mod=" is true or false
            std::string moderator = "mod=";
            std::size_t moderator_start = message.find(moderator);
            assert(moderator_start != message.npos);
            std::size_t moderator_end =
                message.find(';', moderator_start + moderator.size());
            std::string mod(
                message.data() + moderator_start + moderator.size(),
                moderator_end - (moderator_start + moderator.size()));
            [[maybe_unused]]
            bool ismod = lexical_cast<bool>(subbed);

            // parse out chat message
            std::string chat_start_user =
                "#" + std::string(twitch_channel) + " :";
            std::size_t chat_start = message.find(chat_start_user);
            std::string chat_msg(
                message.data() + chat_start + chat_start_user.size(), 512);

            // plain text message
            std::string user_colored = fmt::format(
                "{}", fmt::styled(chat_user, fmt::fg(fmt::rgb(hex))));
            println(user_colored + ": " + chat_msg);
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