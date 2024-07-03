add_rules("mode.debug", "mode.release")
add_requires("boost")
add_requires("fmt")
-- add_requires("bzip2")
-- add_requires("zlib")
set_languages("gnuxxlatest")

-- TODO: Enforce these being set.
-- TODO: Put these options in a Twitch group.

option("oauth_path")
set_showmenu(true)
set_description(
    "oauth_path: A path to the file which contains an oauthorization key on its first line.")

option("twitch_channel")
set_showmenu(true)
set_description("twitch_channel: A channel name for this bot to connect to.")

option("bot_username")
set_showmenu(true)
set_description("bot_username: A username for messages sent by this bot.")

-- Build the Twitch bot.
target("exe")
oauth = ""
channel_name = ""
bot_username = ""

on_config(function()
    function oauth_exists(file_path)
        local file = io.open(file_path)
        if file then
            file:close()
        else
            print(
                "`" .. file_path ..
                "` was not found! This file must contain your Twitch authorization key.")
        end
        return file ~= nil
    end

    function oauth_from(file_path)
        if not oauth_exists(file_path) then return {} end
        for line in io.lines(file_path) do
            -- Read out the first line.
            return line
        end
    end

    -- Ensure `oauth_path` is set.
    assert(get_config("oauth_path"), "`oauth_path` was not set!")
    print("Found `oauth_path`: " .. tostring(get_config("oauth_path")))
    oauth = oauth_from(get_config("oauth_path"))

    -- Ensure `channel_name` is set.
    if (not get_config("channel_name")) then
        print(
            'WARNING! `channel_name` was not set! Defaulting to "Le_Alchemist".')
        channel_name = "Le_Alchemist"
    else
        print("Found `channel_name`: " .. tostring(get_config("channel_name")))
        channel_name = tostring(get_config("channel_name"))
    end

    -- Ensure `bot_username` is set.
    if (not get_config("bot_username")) then
        print('WARNING! `bot_username` was not set! Defaulting to "Lily".')
        bot_username = "Lily"
    else
        print("Found `bot_username`: " .. tostring(get_config("bot_username")))
        bot_username = tostring(get_config("bot_username"))
    end
end)

add_defines('OAUTH="' .. oauth .. '"')
add_defines('CHANNEL="' .. channel_name .. '"')
add_defines('BOTNAME="' .. bot_username .. '"')

set_kind("binary")
set_warnings("all", "extra")
add_packages("boost")
add_packages("fmt")
add_options("oauth")
add_options("twitch_channel")
add_options("bot_username")

if is_mode("release") then
    add_cxxflags("-fmerge-all-constants", { tools = "clang" })
    add_cxxflags("-fexperimental-library", { tools = "clang", force = true })
    add_cxxflags("-fmerge-constants", { tools = "gcc" })
end

add_files("src/*.cpp")
