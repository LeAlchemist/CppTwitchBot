#pragma once
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

// The io_context is required for all I/O
inline net::io_context ioc;
// These objects perform our I/O
inline tcp::resolver resolver(ioc);
inline beast::tcp_stream stream(ioc);

inline constexpr std::string_view host = "irc.chat.twitch.tv";
inline constexpr std::string_view port = "6667";
inline constexpr std::string_view oauth = "oauth:" OAUTH;
inline constexpr std::string_view bot_username = BOTNAME;
inline constexpr std::string_view twitch_channel = CHANNEL;

void connect_to_twitch();