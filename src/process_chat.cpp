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
    // get if user is "broadcaster", "subscriber", or "premium"... info found
    // here https://dev.twitch.tv/docs/irc/tags/
    //
    // information about badge icons can be found here:
    // https://help.twitch.tv/s/article/twitch-chat-badges-guide?language=en_US
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

    // admin badge link:
    // https://static-cdn.jtvnw.net/badges/v1/9ef7e029-4cdf-4d4d-a0d5-e2b3fb2583fe/1
    DECL_BADGE(admin);
    // I might have to rework the bits badge request, since certain amount of
    // bits will change the badge icon
    DECL_BADGE(bits);
    // broadcaster badge link:
    // https://static-cdn.jtvnw.net/badges/v1/5527c58c-fb7d-422d-b71b-f309dcb85cc1/1
    DECL_BADGE(broadcaster);
    // moderator badge link:
    // https://static-cdn.jtvnw.net/badges/v1/3267646d-33f0-4b17-b3df-f923a41db1d0/1
    DECL_BADGE(moderator);
    // subscriber badges are channel specific and might need to be requested by
    // an actual registered twitch bot
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
    // staff badge link:
    // https://static-cdn.jtvnw.net/badges/v1/d97c37bd-a6f5-4c38-8f57-4e4bef88af34/1
    DECL_BADGE(staff);
    // twitch turbo badge link:
    // https://static-cdn.jtvnw.net/badges/v1/bd444ec6-8f34-4bf9-91f4-af1e3428d80f/1
    DECL_BADGE(turbo);
    // premium badge link:
    // https://static-cdn.jtvnw.net/badges/v1/bbbe0db0-a598-423e-86d0-f9fb98ca1933/1
    // as far as I can tell premium is twitch prime
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
        // this doesn't do anything yet, much of the functionality I wanted
        // to add is locked behind registering the app with twitch and getting
        // their cli oAuth key for http requests
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

#ifdef SHOW_RAW_MESSAGE
            println(message_receive);
#endif

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
