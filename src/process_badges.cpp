#include "process_chat.hpp"

#include <cassert>
#include <cstddef>
#include <string>
#include "boost/lexical_cast.hpp"
#include "fmt/core.h"

using boost::bad_lexical_cast;
using boost::lexical_cast;

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