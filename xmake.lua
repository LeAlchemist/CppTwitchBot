add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")
add_requires("boost")
add_requires("fmt")
set_languages("gnuxxlatest")

-- TODO: Enforce these being set.
-- TODO: Put these options in a Twitch group.

option("oauth")
set_showmenu(true)

option("twitch_channel")
set_showmenu(true)

option("bot_username")
set_showmenu(true)

option("DECL_BADGE_FEATURE")
set_showmenu(true)
set_default(true)
add_defines("DECL_BADGE_FEATURE")

-- Build the Twitch bot.
target("exe")


set_kind("binary")
set_warnings("all", "extra")
add_packages("boost")
add_packages("fmt")
add_options("oauth")
add_options("twitch_channel")
add_options("bot_username")
add_options("DECL_BADGE_FEATURE")

add_defines('OAUTH="' .. tostring(get_config("oauth")) .. '"')
add_defines('CHANNEL="' .. tostring(get_config("twitch_channel")) .. '"')
add_defines('BOTNAME="' .. tostring(get_config("bot_username")) .. '"')

if is_mode("release") then
    add_cxxflags("-fmerge-all-constants", { tools = "clang" })
    add_cxxflags("-fexperimental-library", { tools = "clang", force = true })
    add_cxxflags("-fmerge-constants", { tools = "gcc" })
end

add_files("src/*.cpp")
