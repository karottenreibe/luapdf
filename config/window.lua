------------------
-- Window class --
------------------
-- Window class table
window = {}

-- List of active windows by window widget
window.bywidget = setmetatable({}, { __mode = "k" })

-- Widget construction aliases
local function entry()    return widget{type="entry"}    end
local function eventbox() return widget{type="eventbox"} end
local function hbox()     return widget{type="hbox"}     end
local function label()    return widget{type="label"}    end
local function notebook() return widget{type="notebook"} end
local function vbox()     return widget{type="vbox"}     end

-- Build and pack window widgets
function window.build()
    -- Create a table for widgets and state variables for a window
    local w = {
        win    = widget{type="window"},
        ebox   = eventbox(),
        layout = vbox(),
        tabs   = notebook(),
        -- Tablist widget
        tablist = lousy.widget.tablist(),
        -- Status bar widgets
        sbar = {
            layout = hbox(),
            ebox   = eventbox(),
            -- Left aligned widgets
            l = {
                layout = hbox(),
                ebox   = eventbox(),
                path   = label(),
            },
            -- Fills space between the left and right aligned widgets
            sep = eventbox(),
            -- Right aligned widgets
            r = {
                layout = hbox(),
                ebox   = eventbox(),
                buf    = label(),
                tabi   = label(),
                scroll = label(),
            },
        },

        -- Vertical menu window widget (completion results, bookmarks, qmarks, ..)
        menu = lousy.widget.menu(),

        -- Input bar widgets
        ibar = {
            layout  = hbox(),
            ebox    = eventbox(),
            prompt  = label(),
            input   = entry(),
        },

        closed_tabs = {}
    }

    -- Assemble window
    w.ebox.child = w.layout
    w.win.child = w.ebox

    -- Pack tablist
    w.layout:pack(w.tablist.widget)

    -- Pack notebook
    w.layout:pack(w.tabs, { expand = true, fill = true })

    -- Pack left-aligned statusbar elements
    local l = w.sbar.l
    l.layout:pack(l.path)
    l.ebox.child = l.layout

    -- Pack right-aligned statusbar elements
    local r = w.sbar.r
    r.layout:pack(r.buf)
    r.layout:pack(r.tabi)
    r.layout:pack(r.scroll)
    r.ebox.child = r.layout

    -- Pack status bar elements
    local s = w.sbar
    s.layout:pack(l.ebox)
    s.layout:pack(s.sep, { expand = true, fill = true })
    s.layout:pack(r.ebox)
    s.ebox.child = s.layout
    w.layout:pack(s.ebox)

    -- Pack menu widget
    w.layout:pack(w.menu.widget)
    w.menu:hide()

    -- Pack input bar
    local i = w.ibar
    i.layout:pack(i.prompt)
    i.layout:pack(i.input, { expand = true, fill = true })
    i.ebox.child = i.layout
    w.layout:pack(i.ebox)

    -- Other settings
    i.input.show_frame = false
    w.tabs.show_tabs = false
    l.path.selectable = true

    -- Allows indexing of window struct by window widget
    window.bywidget[w.win] = w

    return w
end

-- Table of functions to call on window creation. Normally used to add signal
-- handlers to the new windows widgets.
window.init_funcs = {
    -- Attach notebook widget signals
    notebook_signals = function (w)
        w.tabs:add_signal("page-added", function (nbook, doc, idx)
            w:update_tab_count(idx)
            w:update_tablist()
        end)
        w.tabs:add_signal("switch-page", function (nbook, doc, idx)
            w:set_mode()
            w:update_tab_count(idx)
            w:update_win_title(doc)
            w:update_path(doc)
            w:update_tablist(idx)
            w:update_buf()
        end)
        w.tabs:add_signal("page-reordered", function (nbook, doc, idx)
            w:update_tab_count()
            w:update_tablist()
        end)
    end,

    last_win_check = function (w)
        w.win:add_signal("destroy", function ()
            -- call the quit function if this was the last window left
            if #luapdf.windows == 0 then luapdf.quit() end
            if w.close_win then w:close_win() end
        end)
    end,

    key_press_match = function (w)
        w.win:add_signal("key-press", function (_, mods, key)
            -- Match & exec a bind
            local success, match = pcall(w.hit, w, mods, key)
            if not success then
                w:error("In bind call: " .. match)
            elseif match then
                return true
            end
        end)
    end,

    tablist_tab_click = function (w)
        w.tablist:add_signal("tab-clicked", function (_, index, mods, button)
            if button == 1 then
                w.tabs:switch(index)
                return true
            elseif button == 2 then
                w:close_tab(w.tabs[index])
                return true
            end
        end)
    end,

    apply_window_theme = function (w)
        local s, i = w.sbar, w.ibar

        -- Set foregrounds
        for wi, v in pairs({
            [s.l.path]   = theme.path_sbar_fg,
            [s.r.buf]    = theme.buf_sbar_fg,
            [s.r.tabi]   = theme.tabi_sbar_fg,
            [s.r.scroll] = theme.scroll_sbar_fg,
            [i.prompt]   = theme.prompt_ibar_fg,
            [i.input]    = theme.input_ibar_fg,
        }) do wi.fg = v end

        -- Set backgrounds
        for wi, v in pairs({
            [s.l.ebox]   = theme.sbar_bg,
            [s.r.ebox]   = theme.sbar_bg,
            [s.sep]      = theme.sbar_bg,
            [s.ebox]     = theme.sbar_bg,
            [i.ebox]     = theme.ibar_bg,
            [i.input]    = theme.input_ibar_bg,
        }) do wi.bg = v end

        -- Set fonts
        for wi, v in pairs({
            [s.l.path]   = theme.path_sbar_font,
            [s.r.buf]    = theme.buf_sbar_font,
            [s.r.tabi]   = theme.tabi_sbar_font,
            [s.r.scroll] = theme.scroll_sbar_font,
            [i.prompt]   = theme.prompt_ibar_font,
            [i.input]    = theme.input_ibar_font,
        }) do wi.font = v end
    end,

    set_default_size = function (w)
        local size = globals.default_window_size or "800x600"
        if string.match(size, "^%d+x%d+$") then
            w.win:set_default_size(string.match(size, "^(%d+)x(%d+)$"))
        else
            warn("E: window.lua: invalid window size: %q", size)
        end
    end,

    set_window_icon = function (w)
        local path = (luapdf.dev_paths and os.exists("./extras/luapdf.png")) or
            os.exists("/usr/share/pixmaps/luapdf.png")
        if path then w.win.icon = path end
    end,
}

-- Helper functions which operate on the window widgets or structure.
window.methods = {
    -- Return the widget in the currently active tab
    get_current = function (w)       return w.tabs[w.tabs:current()] end,
    -- Check if given widget is the widget in the currently active tab
    is_current  = function (w, wi)   return w.tabs:indexof(wi) == w.tabs:current() end,

    get_tab_title = function (w, doc)
        if not doc then doc = w:get_current() end
        return doc.title or "(Untitled)"
    end,

    -- Wrapper around the bind plugin's hit method
    hit = function (w, mods, key, opts)
        local opts = lousy.util.table.join(opts or {}, {
            enable_buffer = w:is_mode("normal"),
            buffer = w.buffer,
        })

        local caught, newbuf = lousy.bind.hit(w, w.binds, mods, key, opts)
        if w.win then -- Check binding didn't cause window to exit
            w.buffer = newbuf
            w:update_buf()
        end
        return caught
    end,

    -- Wrapper around the bind plugin's match_cmd method
    match_cmd = function (w, buffer)
        return lousy.bind.match_cmd(w, get_mode("command").binds, buffer)
    end,

    -- enter command or characters into command line
    enter_cmd = function (w, cmd, opts)
        w:set_mode("command")
        w:set_input(cmd, opts)
    end,

    -- insert a string into the command line at the current cursor position
    insert_cmd = function (w, str)
        if not str then return end
        local i = w.ibar.input
        local text = i.text
        local pos = i.position
        local left, right = string.sub(text, 1, pos), string.sub(text, pos+1)
        i.text = left .. str .. right
        i.position = pos + #str
    end,

    -- Emulates pressing the Return key in input field
    activate = function (w)
        w.ibar.input:emit_signal("activate")
    end,

    del_word = function (w)
        local i = w.ibar.input
        local text = i.text
        local pos = i.position
        if text and #text > 1 and pos > 1 then
            local left, right = string.sub(text, 2, pos), string.sub(text, pos+1)
            if not string.find(left, "%s") then
                left = ""
            elseif string.find(left, "%w+%s*$") then
                left = string.sub(left, 0, string.find(left, "%w+%s*$") - 1)
            elseif string.find(left, "%W+%s*$") then
                left = string.sub(left, 0, string.find(left, "%W+%s*$") - 1)
            end
            i.text =  string.sub(text, 1, 1) .. left .. right
            i.position = #left + 1
        end
    end,

    del_line = function (w)
        local i = w.ibar.input
        if not string.match(i.text, "^[:/?]$") then
            i.text = string.sub(i.text, 1, 1)
            i.position = -1
        end
    end,

    del_backward_char = function (w)
        local i = w.ibar.input
        local text = i.text
        local pos = i.position

        if pos > 1 then
            i.text = string.sub(text, 0, pos - 1) .. string.sub(text, pos + 1)
            i.position = pos - 1
        end
    end,

    del_forward_char = function (w)
        local i = w.ibar.input
        local text = i.text
        local pos = i.position

        i.text = string.sub(text, 0, pos) .. string.sub(text, pos + 2)
        i.position = pos
    end,

    beg_line = function (w)
        local i = w.ibar.input
        i.position = 1
    end,

    end_line = function (w)
        local i = w.ibar.input
        i.position = -1
    end,

    forward_char = function (w)
        local i = w.ibar.input
        i.position = i.position + 1
    end,

    backward_char = function (w)
        local i = w.ibar.input
        local pos = i.position
        if pos > 1 then
            i.position = pos - 1
        end
    end,

    forward_word = function (w)
        local i = w.ibar.input
        local text = i.text
        local pos = i.position
        if text and #text > 1 then
            local right = string.sub(text, pos+1)
            if string.find(right, "%w+") then
                local _, move = string.find(right, "%w+")
                i.position = pos + move
            end
        end
    end,

    backward_word = function (w)
        local i = w.ibar.input
        local text = i.text
        local pos = i.position
        if text and #text > 1 and pos > 1 then
            local left = string.reverse(string.sub(text, 2, pos))
            if string.find(left, "%w+") then
                local _, move = string.find(left, "%w+")
                i.position = pos - move
            end
        end
    end,

    -- Shows a notification until the next keypress of the user.
    notify = function (w, msg, set_mode)
        if set_mode ~= false then w:set_mode() end
        w:set_prompt(msg, { fg = theme.notif_fg, bg = theme.notif_bg })
    end,

    warning = function (w, msg, set_mode)
        if set_mode ~= false then w:set_mode() end
        w:set_prompt(msg, { fg = theme.warning_fg, bg = theme.warning_bg })
    end,

    error = function (w, msg, set_mode)
        if set_mode ~= false then w:set_mode() end
        w:set_prompt("Error: "..msg, { fg = theme.error_fg, bg = theme.error_bg })
    end,

    -- Set and display the prompt
    set_prompt = function (w, text, opts)
        local prompt, ebox, opts = w.ibar.prompt, w.ibar.ebox, opts or {}
        prompt:hide()
        -- Set theme
        fg, bg = opts.fg or theme.ibar_fg, opts.bg or theme.ibar_bg
        if prompt.fg ~= fg then prompt.fg = fg end
        if ebox.bg ~= bg then ebox.bg = bg end
        -- Set text or remain hidden
        if text then
            prompt.text = lousy.util.escape(text)
            prompt:show()
        end
    end,

    -- Set display and focus the input bar
    set_input = function (w, text, opts)
        local input, opts = w.ibar.input, opts or {}
        input:hide()
        -- Set theme
        fg, bg = opts.fg or theme.ibar_fg, opts.bg or theme.ibar_bg
        if input.fg ~= fg then input.fg = fg end
        if input.bg ~= bg then input.bg = bg end
        -- Set text or remain hidden
        if text then
            input.text = ""
            input:show()
            input:focus()
            input.text = text
            input.position = opts.pos or -1
        end
    end,

    -- GUI content update functions
    update_tab_count = function (w, i, t)
        w.sbar.r.tabi.text = string.format("[%d/%d]", i or w.tabs:current(), t or w.tabs:count())
    end,

    update_win_title = function (w, doc)
        if not doc then doc = w:get_current() end
        local path = doc.path
        local title = (doc.title or "(Untitled)") .. " - " .. (path or "luapdf")
        local max = globals.max_title_len or 80
        if #title > max then title = string.sub(title, 1, max) .. "..." end
        w.win.title = title
    end,

    update_path = function (w, doc, path, link)
        local u, escape = w.sbar.l.path, lousy.util.escape
        if link then
            u.text = "Link: " .. escape(link)
        else
            if not doc then doc = w:get_current() end
            u.text = escape((path or (doc and doc.path) or "(no pdf)"))
        end
    end,

    update_scroll = function (w, doc)
        if not doc then doc = w:get_current() end
        local label = w.sbar.r.scroll
        if doc then
            local scroll = doc.scroll
            local y, max, text = scroll.y, scroll.ymax
            if     max == 0   then text = "All"
            elseif y   == 0   then text = "Top"
            elseif y   == max then text = "Bot"
            else text = string.format("%2d%%", (y / max) * 100)
            end
            if label.text ~= text then label.text = text end
            label:show()
        else
            label:hide()
        end
    end,

    update_buf = function (w)
        local buf = w.sbar.r.buf
        if w.buffer then
            buf.text = lousy.util.escape(string.format(" %-3s", w.buffer))
            buf:show()
        else
            buf:hide()
        end
    end,

    update_binds = function (w, mode)
        -- Generate the list of active key & buffer binds for this mode
        w.binds = lousy.util.table.join((get_mode(mode) or {}).binds or {}, get_mode('all').binds or {})
        -- Clear & hide buffer
        w.buffer = nil
        w:update_buf()
    end,

    update_tablist = function (w, current)
        local current = current or w.tabs:current()
        local fg, bg, nfg, snfg = theme.tab_fg, theme.tab_bg, theme.tab_ntheme, theme.selected_ntheme
        local escape, get_title = lousy.util.escape, w.get_tab_title
        local tabs, tfmt = {}, ' <span foreground="%s">%s</span> %s'

        for i, doc in ipairs(w.tabs.children) do
            -- Get tab number theme
            local ntheme = nfg
            tabs[i] = {
                title = string.format(tfmt, ntheme or fg, i, escape(get_title(w, doc))),
                fg = (current == i and theme.tab_selected_fg) or fg,
                bg = (current == i and theme.tab_selected_bg) or bg,
            }
        end

        if #tabs < 2 then tabs, current = {}, 0 end
        w.tablist:update(tabs, current)
    end,

    new_tab = function (w, path, switch, order)
        local doc
        -- Make new page widget
        if not doc then
            doc = document.new(w, path)
            local p = 1
            local e = eventbox()
            local i = doc.pages[p].widget
            e.child = i
            e.bg = "#fff"
            doc.current = {
                page = p,
                image = i,
                ebox = e,
            }
            -- Get tab order function
            if not order and taborder then
                order = (switch == false and taborder.default_bg)
                    or taborder.default
            end
            pos = w.tabs:insert((order and order(w, doc)) or -1, e)
            if switch ~= false then w.tabs:switch(pos) end
        end
        -- Update statusbar widgets
        w:update_tab_count()
        w:update_tablist()
        return doc
    end,

    -- close the current tab
    close_tab = function (w, doc, blank_last)
        doc = doc or w:get_current()
        -- Do nothing if no doc open
        if not doc then return end
        -- And relative location
        local index = w.tabs:indexof(doc)
        if index ~= 1 then tab.after = w.tabs[index-1] end
        table.insert(w.closed_tabs, tab)
        doc:destroy()
        w:update_tab_count()
        w:update_tablist()
    end,

    close_win = function (w, force)
        -- Ask plugins if it's OK to close last window
        if not force and (#luapdf.windows == 1) then
            local emsg = luapdf.emit_signal("can-close", w)
            if emsg then
                assert(type(emsg) == "string", "invalid exit error message")
                w:error(string.format("Can't close luapdf: %s (force close "
                    .. "with :q! or :wq!)", emsg))
                return false
            end
        end

        w:emit_signal("close")

        -- Close all tabs
        while w.tabs:count() ~= 0 do
            w:close_tab(nil, false)
        end

        -- Destroy tablist
        w.tablist:destroy()

        -- Remove from window index
        window.bywidget[w.win] = nil

        -- Clear window struct
        w = setmetatable(w, {})

        -- Recursively remove widgets from window
        local children = lousy.util.recursive_remove(w.win)
        -- Destroy all widgets
        for i, c in ipairs(lousy.util.table.join(children, {w.win})) do
            if c.hide then c:hide() end
            c:destroy()
        end

        -- Remove all window table vars
        for k, _ in pairs(w) do w[k] = nil end

        -- Quit if closed last window
        if #luapdf.windows == 0 then luapdf.quit() end
    end,

    -- Navigate current doc or open new tab
    -- TODO: giving a doc here won't work
    navigate = function (w, path, doc)
        if not doc then doc = w:get_current() end
        if doc then
            doc.path = path
        else
            return w:new_tab(path)
        end
    end,

    -- Save, restart luapdf and reload session.
    restart = function (w)
        -- Generate luapdf launch command.
        local args = {({string.gsub(luapdf.execpath, " ", "\\ ")})[1]}
        if luapdf.verbose then table.insert(args, "-v") end
        -- Relaunch without libunique bindings?
        if luapdf.nounique then table.insert(args, "-U") end

        -- Get new config path
        local conf
        if luapdf.confpath ~= "/etc/xdg/luapdf/rc.lua" and os.exists(luapdf.confpath) then
            conf = luapdf.confpath
            table.insert(args, string.format("-c %q", conf))
        end

        -- Check config has valid syntax
        local cmd = table.concat(args, " ")
        if luapdf.spawn_sync(cmd .. " -k") ~= 0 then
            return w:error("Cannot restart, syntax error in configuration file"..((conf and ": "..conf) or "."))
        end

        -- Save session.
        local wins = {}
        for _, w in pairs(window.bywidget) do table.insert(wins, w) end
        session.save(wins)

        -- Replace current process with new luapdf instance.
        luapdf.exec(cmd)
    end,

    -- Tab traversing functions
    next_tab = function (w, n)
        w.tabs:switch((((n or 1) + w.tabs:current() -1) % w.tabs:count()) + 1)
    end,

    prev_tab = function (w, n)
        w.tabs:switch(((w.tabs:current() - (n or 1) -1) % w.tabs:count()) + 1)
    end,

    goto_tab = function (w, n)
        if n and (n == -1 or n > 0) then
            return w.tabs:switch((n <= w.tabs:count() and n) or -1)
        end
    end,
}

-- Ordered list of class index functions. Other classes are able
-- to add their own index functions to this list.
window.indexes = {
    -- Find function in window.methods first
    function (w, k) return window.methods[k] end
}

-- Create new window
function window.new(paths)
    local w = window.build()

    -- Set window metatable
    setmetatable(w, {
        __index = function (_, k)
            -- Check widget structure first
            local v = rawget(w, k)
            if v then return v end
            -- Call each window index function
            for _, index in ipairs(window.indexes) do
                v = index(w, k)
                if v then return v end
            end
        end,
    })

    -- Setup window widget for signals
    lousy.signal.setup(w)

    -- Call window init functions
    for _, func in pairs(window.init_funcs) do
        func(w)
    end

    -- Populate notebook with tabs
    for _, path in ipairs(paths or {}) do
        w:new_tab(path, false)
    end

    -- Make sure something is loaded
    if w.tabs:count() == 0 then
        -- TODO: show something
    end

    -- Set initial mode
    w:set_mode()

    -- Show window
    w.win:show()

    return w
end

-- vim: et:sw=4:ts=8:sts=4:tw=80
