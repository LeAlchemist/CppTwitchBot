add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")
add_requires("boost")
add_requires("fmt", {
	configs = { header_only = true, toolchain = "llvm", runtimes = "c++_shared" },
	system = false,
})
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
if true then
	set_showmenu(true)
	set_default(true)
	add_defines("DECL_BADGE_FEATURE")
end

option("DONT_SHOW_OAUTH")
if true then
	set_showmenu(true)
	set_default(true)
	add_defines("DONT_SHOW_OAUTH")
end

-- Build the Twitch bot.
target("exe")
if true then
	set_kind("binary")
	set_warnings("allextra")
	set_toolchains("llvm")
	set_runtimes("c++_shared")
	-- Experimental features are required for `jthread`.
	add_cxxflags("-fexperimental-library", { force = true })

	add_packages("boost")
	add_packages("fmt")
	add_options("oauth")
	add_options("twitch_channel")
	add_options("bot_username")
	add_options("DECL_BADGE_FEATURE")
	add_options("DONT_SHOW_OAUTH")

	add_defines('OAUTH="' .. tostring(get_config("oauth")) .. '"')
	add_defines('CHANNEL="' .. tostring(get_config("twitch_channel")) .. '"')
	add_defines('BOTNAME="' .. tostring(get_config("bot_username")) .. '"')

	if is_mode("release") then
		add_cxxflags("-fmerge-all-constants", { tools = "clang" })
		add_cxxflags("-fmerge-constants", { tools = "gcc" })
	end

	add_files("src/*.cpp")
end
