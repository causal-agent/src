require("awful")
require("awful.autofocus")
require("awful.rules")
require("beautiful")
require("naughty")
vicious = require("vicious")

home = os.getenv("HOME")
awesome_home = home .. "/.config/awesome"
terminal = "terminal"
editor = "gvim"

modkey = "Mod4"

-- Theming
beautiful.init(awesome_home .. "/themes/current/theme.lua")
naughty.config.default_preset.timeout = 3

-- Layouts and tags
layouts = {
    awful.layout.suit.tile,
    awful.layout.suit.tile.left,
    awful.layout.suit.tile.bottom,
    awful.layout.suit.tile.top,
    awful.layout.suit.max,
    awful.layout.suit.floating
}

tags = {
    names = { 1, 2, 3, 4, 5, 6, 7, 8, 9 },
    layouts = {
        layouts[5],
        layouts[3],
        layouts[1],
        layouts[1],
        layouts[1],
        layouts[1],
        layouts[1],
        layouts[1],
        layouts[6]
    }
}

for s = 1, screen.count() do
    tags[s] = awful.tag(tags.names, s, tags.layouts)
end

-- disable startup-notification globally
local oldspawn = awful.util.spawn
awful.util.spawn = function (s)
  oldspawn(s, false)
end

-- {{{ Error handling
-- Check if awesome encountered an error during startup and fell back to
-- another config (This code will only ever execute for the fallback config)
if awesome.startup_errors then
    naughty.notify({ preset = naughty.config.presets.critical,
                     title = "Oops, there were errors during startup!",
                     text = awesome.startup_errors })
end

-- Handle runtime errors after startup
do
    local in_error = false
    awesome.add_signal("debug::error", function (err)
        -- Make sure we don't go into an endless error loop
        if in_error then return end
        in_error = true

        naughty.notify({ preset = naughty.config.presets.critical,
                         title = "Oops, an error happened!",
                         text = err })
        in_error = false
    end)
end
-- }}}

-- Wibox
function icon(i)
    return image(awesome_home .. "/icons/" .. i .. ".png")
end

taglist = {
    buttons = awful.util.table.join(
                  awful.button({ }, 1, awful.tag.viewonly),
                  awful.button({ modkey }, 1, awful.client.movetotag),
                  awful.button({ }, 3, awful.tag.viewtoggle),
                  awful.button({ modkey }, 3, awful.client.toggletag),
                  awful.button({ }, 4, awful.tag.viewnext),
                  awful.button({ }, 5, awful.tag.viewprev)),
    create = function (s)
        return awful.widget.taglist(s, awful.widget.taglist.label.all, taglist.buttons)
    end
}

wibox = {
    separator = widget({ type = "textbox" }),
    promptbox = awful.widget.prompt({ layout = awful.widget.layout.horizontal.leftright }),
    clock = awful.widget.textclock({ align = "right" }, "%a %b %d, %I:%M %p"),
    systray = widget({ type = "systray" }),
    mpd = widget({ type = "textbox" }),

    clockicon = widget({ type = "imagebox" }),
    mpdicon = widget({ type = "imagebox" }),

    icons = {
        play = icon("play"),
        pause = icon("pause"),
        stop = icon("stop"),
        clock = icon("clock"),
    },
}

wibox.separator.text = " "
wibox.clockicon.image = wibox.icons.clock

vicious.register(wibox.mpd, vicious.widgets.mpd, function (widget, args)
    if args["{state}"] == "Stop" then
        wibox.mpdicon.image = wibox.icons.stop
        return ""
    end
    if args["{state}"] == "Pause" then
        wibox.mpdicon.image = wibox.icons.pause
    else
        wibox.mpdicon.image = wibox.icons.play
    end
    return args["{Artist}"] .. " - " .. args["{Title}"]
end)

for s = 1, screen.count() do
    wibox[s] = awful.wibox({ position = "top", screen = s, height = 16 })
    wibox[s].widgets = {
        {
            taglist.create(s),
            wibox.promptbox,
            layout = awful.widget.layout.horizontal.leftright
        },
        s == 1 and wibox.systray or nil,
        wibox.clock,
        wibox.clockicon,
        wibox.separator,
        wibox.mpd,
        wibox.mpdicon,
        layout = awful.widget.layout.horizontal.rightleft
    }
end

-- Bindings
root.buttons(awful.util.table.join(
    awful.button({ }, 4, awful.tag.viewnext),
    awful.button({ }, 5, awful.tag.viewprev)))

globalkeys = awful.util.table.join(
    awful.key({ modkey }, "Left", awful.tag.viewprev),
    awful.key({ modkey }, "Right", awful.tag.viewnext),

    -- Client focus
    awful.key({ modkey }, "j", function ()
        awful.client.focus.byidx(1)
        if client.focus then client.focus:raise() end
    end),
    awful.key({ modkey }, "k", function ()
        awful.client.focus.byidx(-1)
        if client.focus then client.focus:raise() end
    end),

    -- Client movement
    awful.key({ modkey, "Shift" }, "j", function () awful.client.swap.byidx(1) end),
    awful.key({ modkey, "Shift" }, "k", function () awful.client.swap.byidx(-1) end),
    awful.key({ modkey, "Control" }, "j", function () awful.screen.focus_relative(1) end),
    awful.key({ modkey, "Control" }, "k", function () awful.screen.focus_relative(-1) end),
    awful.key({ modkey }, "u", awful.client.urgent.jumpto),

    -- Tag modification
    awful.key({ modkey }, "l", function () awful.tag.incmwfact(0.05) end),
    awful.key({ modkey }, "h", function () awful.tag.incmwfact(-0.05) end),
    awful.key({ modkey, "Shift" }, "h", function () awful.tag.incnmaster(1) end),
    awful.key({ modkey, "Shift" }, "l", function () awful.tag.incnmaster(-1) end),
    awful.key({ modkey, "Control" }, "h", function () awful.tag.incncol(1) end),
    awful.key({ modkey, "Control" }, "l", function () awful.tag.incncol(-1) end),
    awful.key({ modkey }, "space", function () awful.layout.inc(layouts, 1) end),
    awful.key({ modkey, "Shift" }, "space", function () awful.layout.inc(layouts, -1) end),

    -- Prompts
    awful.key({ modkey }, "r", function () wibox.promptbox:run() end),
    awful.key({ modkey }, "x", function ()
        awful.prompt.run({ prompt = "Lua: " },
            wibox.promptbox.widget,
            awful.util.eval, nil,
            awful.util.getdir("cache") .. "/history_eval")
        end),

    awful.key({ }, "Print", function () awful.util.spawn(home .. "/bin/scrot-upload") end),
    awful.key({ modkey }, "Print", function () awful.util.spawn(home .. "/bin/scrot-upload -b -s") end),
    awful.key({ }, "XF86AudioPlay", function () awful.util.spawn("mpc toggle") end),
    awful.key({ }, "XF86AudioStop", function () awful.util.spawn("mpc stop") end),
    awful.key({ }, "XF86AudioNext", function () awful.util.spawn("mpc next") end),
    awful.key({ }, "XF86AudioPrev", function () awful.util.spawn("mpc prev") end),

    awful.key({ modkey }, "Return", function () awful.util.spawn(terminal) end),
    awful.key({ modkey, "Control" }, "r", awesome.restart),
    awful.key({ modkey, "Shift" }, "q", awesome.quit))

clientkeys = awful.util.table.join(
    awful.key({ modkey, "Shift" }, "c", function (c) c:kill() end),

    awful.key({ modkey }, "f", function (c) c.fullscreen = not c.fullscreen end),
    awful.key({ modkey, "Control" }, "space", awful.client.floating.toggle),
    awful.key({ modkey }, "t", function (c) c.ontop = not c.ontop end),
    awful.key({ modkey, "Shift" }, "r", function (c) c:redraw() end),

    awful.key({ modkey, "Control" }, "Return", function (c) c:swap(awful.client.getmaster()) end),
    awful.key({ modkey }, "o", awful.client.movetoscreen),

    -- This is where I got too lazy to rewrite the default
    awful.key({ modkey,           }, "m",
        function (c)
            c.maximized_horizontal = not c.maximized_horizontal
            c.maximized_vertical   = not c.maximized_vertical
        end)
)

-- Compute the maximum number of digit we need, limited to 9
keynumber = 0
for s = 1, screen.count() do
   keynumber = math.min(9, math.max(#tags[s], keynumber));
end

-- Bind all key numbers to tags.
-- Be careful: we use keycodes to make it works on any keyboard layout.
-- This should map on the top row of your keyboard, usually 1 to 9.
for i = 1, keynumber do
    globalkeys = awful.util.table.join(globalkeys,
        awful.key({ modkey }, "#" .. i + 9,
                  function ()
                        local screen = mouse.screen
                        if tags[screen][i] then
                            awful.tag.viewonly(tags[screen][i])
                        end
                  end),
        awful.key({ modkey, "Control" }, "#" .. i + 9,
                  function ()
                      local screen = mouse.screen
                      if tags[screen][i] then
                          awful.tag.viewtoggle(tags[screen][i])
                      end
                  end),
        awful.key({ modkey, "Shift" }, "#" .. i + 9,
                  function ()
                      if client.focus and tags[client.focus.screen][i] then
                          awful.client.movetotag(tags[client.focus.screen][i])
                      end
                  end),
        awful.key({ modkey, "Control", "Shift" }, "#" .. i + 9,
                  function ()
                      if client.focus and tags[client.focus.screen][i] then
                          awful.client.toggletag(tags[client.focus.screen][i])
                      end
                  end))
end

clientbuttons = awful.util.table.join(
    awful.button({ }, 1, function (c) client.focus = c; c:raise() end),
    awful.button({ modkey }, 1, awful.mouse.client.move),
    awful.button({ modkey }, 3, awful.mouse.client.resize))

-- Set keys
root.keys(globalkeys)
-- }}}

-- {{{ Rules
awful.rules.rules = {
    -- All clients will match this rule.
    { rule = { },
      properties = { border_width = beautiful.border_width,
                     border_color = beautiful.border_normal,
                     focus = true,
                     keys = clientkeys,
                     buttons = clientbuttons } },
    { rule = { class = "MPlayer" },
      properties = { floating = true } },
    { rule = { class = "pinentry" },
      properties = { floating = true } },
    { rule = { class = "gimp" },
      properties = { floating = true } },
    { rule = { class = "qemu" },
      properties = { floating = true } },
    { rule = { class = "feh" },
      properties = { floating = true } },
    -- Set Firefox to always map on tags number 2 of screen 1.
    -- { rule = { class = "Firefox" },
    --   properties = { tag = tags[1][2] } },
}
-- }}}

-- {{{ Signals
-- Signal function to execute when a new client appears.
client.add_signal("manage", function (c, startup)
    -- Add a titlebar
    -- awful.titlebar.add(c, { modkey = modkey })

    -- Enable sloppy focus
    c:add_signal("mouse::enter", function(c)
        if awful.layout.get(c.screen) ~= awful.layout.suit.magnifier
            and awful.client.focus.filter(c) then
            client.focus = c
        end
    end)

    if not startup then
        -- Set the windows at the slave,
        -- i.e. put it at the end of others instead of setting it master.
        -- awful.client.setslave(c)

        -- Put windows in a smart way, only if they does not set an initial position.
        if not c.size_hints.user_position and not c.size_hints.program_position then
            awful.placement.no_overlap(c)
            awful.placement.no_offscreen(c)
        end
    end
end)

client.add_signal("focus", function(c) c.border_color = beautiful.border_focus end)
client.add_signal("unfocus", function(c) c.border_color = beautiful.border_normal end)
-- }}}

awful.util.spawn(awesome_home .. "/autostart.sh")
