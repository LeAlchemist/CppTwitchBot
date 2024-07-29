#include "process_chat.hpp"

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
#include "fmt/core.h"
#include "println.hpp"

using boost::bad_lexical_cast;
using boost::lexical_cast;

// `boost::static_string` does not have a `.contains()` member.
boost::string_view const message(message_receive);

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

struct message_username {
    std::string chat_user;
    int hex_value;
};

auto
message_username() -> message_username {
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
    std::transform(chat_user.begin(), chat_user.end(), chat_user_lower.begin(),
                   ::tolower);

    // get username color
    // twitch/irc uses a hex code value here
    std::string display_name_color = "color=#";
    std::size_t user_color_start = message.find(display_name_color);
    assert(user_color_start != message.npos);
    std::size_t user_color_end =
        message.find(';', user_color_start + display_name_color.size());
    std::string user_color(
        message.data() + user_color_start + display_name_color.size(),
        user_color_end - (user_color_start + display_name_color.size()));
    int hex_value;
    // if user doesn't have an assigned color
    if (user_color.empty()) {
        std::string temp_color = "ccff00";
        auto _ = std::from_chars(temp_color.data(), temp_color.data() + 6,
                                 hex_value, 16);
    }
    auto _ = std::from_chars(user_color.data(), user_color.data() + 6,
                             hex_value, 16);

    return {chat_user, hex_value};
}

void
message_badges() {
    // get if user is broadcaster
    // this could also be modified for other terms such as "subscriber",
    // "premium"... found here https://dev.twitch.tv/docs/irc/tags/
    std::string badges = "badges=";
    std::size_t badges_start = message.find(badges);
    assert(badges_start != message.npos);
    std::size_t badges_end = message.find(';', badges_start + badges.size());
    std::string badges_type(message.data() + badges_start + badges.size(),
                            badges_end - (badges_start + badges.size()));

    auto lambda = [&](std::string const& name) -> std::string {
        std::size_t start = badges_type.find(name);
        assert(start != badges_type.npos);

        std::size_t end = badges_type.find(',', start + name.size());
        assert(end != badges_type.npos);

        end = std::min(end, badges_type.size());

        return {badges_type.data() + start + name.size(),
                end - (start + name.size())};
    };

#define DECL_BADGE(name)                                              \
    [[maybe_unused]]                                                  \
    bool is_##name = lexical_cast<bool>(badges_type.contains(#name)); \
    std::optional<std::string> name##_tier = std::nullopt;            \
    if (is_##name) {                                                  \
        name##_tier = lambda(#name "/");                              \
    }

    DECL_BADGE(admin);
    DECL_BADGE(bits);
    DECL_BADGE(broadcaster);
    DECL_BADGE(moderator);
    DECL_BADGE(subscriber);
    if (is_subscriber) {
        std::string badge_info = "@badge-info=";
        std::string subscriber_info = "subscriber/";
        std::size_t badge_subscriber_start =
            message.find(subscriber_info, badge_info.size());
        assert(badge_subscriber_start != message.npos);
        std::size_t badge_subscriber_end =
            message.find(';', badge_subscriber_start + subscriber_info.size());
        std::string subscriber_lifetime(
            message.data() + badge_subscriber_start + subscriber_info.size(),
            badge_subscriber_end -
                (badge_subscriber_start + subscriber_info.size()));
    }
    DECL_BADGE(staff);
    DECL_BADGE(turbo);
    DECL_BADGE(premium);

#undef DECL_BADGE
}

void
message_privmsg() {
    if (message.contains("tmi.twitch.tv PRIVMSG")) {
        auto [username, hex] = message_username();

        // parse out chat message
        std::string chat_start_user = "#" + std::string(twitch_channel) + " :";
        std::size_t chat_start = message.find(chat_start_user);
        std::string chat_msg(
            message.data() + chat_start + chat_start_user.size(), 512);

        // plain text message
        std::string user_colored =
            fmt::format("{}", fmt::styled(username, fmt::fg(fmt::rgb(hex))));

#ifdef DECL_BADGE_FEATURE
        message_badges();
#endif

        println(user_colored + ": " + chat_msg);
    }
}

void
message_welcome() {
    constexpr char const twitch_channel_pound[] = "#" CHANNEL;

    if (message.contains("JOIN")) {
        std::size_t welcome_user_start = message.find(':');
        std::size_t welcome_user_end = message.find('!', welcome_user_start);

        std::string welcome_user(message.data() + welcome_user_start + 1,
                                 welcome_user_end - (welcome_user_start + 1));

        std::size_t welcome_channel_start = message.find(twitch_channel_pound);
        std::size_t welcome_channel_end = std::size(twitch_channel_pound);

        message_receive.replace(
            welcome_user_start,
            welcome_channel_start + welcome_channel_end - welcome_user_start,
            welcome_user + " JOINED");
    }

    if (message.contains("PART")) {
        std::size_t welcome_user_start = message.find(':');
        std::size_t welcome_user_end = message.find('!', welcome_user_start);

        std::string welcome_user(message.data() + welcome_user_start + 1,
                                 welcome_user_end - (welcome_user_start + 1));

        std::size_t welcome_channel_start = message.find(twitch_channel_pound);
        std::size_t welcome_channel_end = std::size(twitch_channel_pound);

        message_receive.replace(
            welcome_user_start,
            welcome_channel_start + welcome_channel_end - welcome_user_start,
            welcome_user + " LEFT");
    }
}

void
message_keep_alive() {
    // this is to respond to the "keep alive" check
    if (message.contains("PING :tmi.twitch.tv")) {
        append_message(message_sent, "PONG :tmi.twitch.tv");

        stream.write_some(boost::asio::buffer(message_sent.str()));
        println(message_sent);
        message_sent.str("");  // Empty the string buffer
    }
}

// this reads the chat and send a few responses based on basic text parsing
void
process_chat() {
    auto payload_wrapper = boost::asio::buffer(message_receive);
    stream.read_some(payload_wrapper);

    if (message.contains("tmi.twitch.tv")) {
        if (message.contains("PRIVMSG")) {
            message_privmsg();
        } else {
            // these will be terminal only
            message_welcome();
            println(message_receive);
            message_keep_alive();
        }
    }

    // this clears the read buffer and resets size
    std::memset(message_receive.data(), '\0', message_receive.size());
}
