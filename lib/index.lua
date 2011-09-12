----------------------------------------------------------------
-- Document index menu                                        --
-- @author Fabian Streitel (luapdf@rottenrei.be)              --
----------------------------------------------------------------

-- Add index command
local cmd = lousy.bind.cmd
add_cmds({
    cmd("index",                function (w) w:set_mode("index") end),
})

-- Add mode to display the index in an interactive menu
new_mode("index", {
    enter = function (w)
        local rows = {{ "Index", title = true }}
        local build_menu
        build_menu = function (t, indent)
            for _, action in pairs(t) do
                table.insert(rows, { indent .. action.title, dest = action.destination })
                build_menu(action.children, indent .. "  ")
            end
        end
        build_menu(w:get_current().index, "  ")
        w.menu:build(rows)
        w:notify("Use j/k to move, t tabopen, w winopen.", false)
    end,

    leave = function (w)
        w.menu:hide()
    end,
})

-- Add additional binds to quickmarks menu mode
local key = lousy.bind.key
add_binds("index", lousy.util.table.join({

    -- Open quickmark
    key({}, "Return", function (w)
        local row = w.menu:get()
        if row and row.dest then
            document.methods.scroll_to_dest(w:get_current(), w, row.dest)
        end
    end),

    -- Open quickmark in new tab
    key({}, "t", function (w)
        local row = w.menu:get()
        if row and row.dest then
            local doc = w:new_tab(w:get_current().path, {switch = false})
            document.methods.scroll_to_dest(doc, w, row.dest)
        end
    end),

    -- Open quickmark in new window
    key({}, "w", function (w)
        local row = w.menu:get()
        w:set_mode()
        if row and row.dest then
            local new_win = window.new({w:get_current().path})
            document.methods.scroll_to_dest(new_win:get_current(), new_win, row.dest)
        end
    end),

    -- Exit menu
    key({}, "q", function (w) w:set_mode() end),

}, menu_binds))

-- vim: et:sw=4:ts=8:sts=4:tw=80
