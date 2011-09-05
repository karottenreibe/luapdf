-----------------------------------------------------------------------
-- luapdf configuration file, more information at http://luapdf.org/ --
-----------------------------------------------------------------------

if unique then
    unique.new("org.luapdf")
    -- Check for a running luapdf instance
    if unique.is_running() then
        if uris[1] then
            for _, uri in ipairs(uris) do
                unique.send_message("tabopen " .. uri)
            end
        else
            unique.send_message("winopen")
        end
        luapdf.quit()
    end
end

-- Load library of useful functions for luapdf
require "lousy"

-- Small util functions to print output (info prints only when luapdf.verbose is true)
function warn(...) io.stderr:write(string.format(...) .. "\n") end
function info(...) if luapdf.verbose then io.stderr:write(string.format(...) .. "\n") end end

-- Load user's global config
-- ("$XDG_CONFIG_HOME/luapdf/globals.lua" or "/etc/xdg/luapdf/globals.lua")
require "globals"

-- Load user's theme
-- ("$XDG_CONFIG_HOME/luapdf/theme.lua" or "/etc/xdg/luapdf/theme.lua")
lousy.theme.init(lousy.util.find_config("theme.lua"))
theme = assert(lousy.theme.get(), "failed to load theme")

-- Load user's window class
-- ("$XDG_CONFIG_HOME/luapdf/window.lua" or "/etc/xdg/luapdf/window.lua")
require "window"

-- Load user's document class
-- ("$XDG_CONFIG_HOME/luapdf/document.lua" or "/etc/xdg/luapdf/document.lua")
require "document"

-- Load user's mode configuration
-- ("$XDG_CONFIG_HOME/luapdf/modes.lua" or "/etc/xdg/luapdf/modes.lua")
require "modes"

-- Load user's keybindings
-- ("$XDG_CONFIG_HOME/luapdf/binds.lua" or "/etc/xdg/luapdf/binds.lua")
require "binds"

----------------------------------
-- Optional user script loading --
----------------------------------

-- Add document index support
require "index"

-- Add quickmarks support & manager
require "quickmarks"

-- Add command to list closed tabs & bind to open closed tabs
require "undoclose"

-- Add command to list tab history items
require "tabhistory"

-- Add bookmarks support
require "bookmarks"

-- Add command history
require "cmdhist"

-- Add search mode & binds
require "search"

-- Add ordering of new tabs
require "taborder"

-- Add command completion
require "completion"

-----------------------------
-- End user script loading --
-----------------------------

-- Restore last saved session
local w = (session and session.restore())
if w then
    for i, uri in ipairs(uris) do
        w:new_tab(uri, {switch = (i == 1)})
    end
else
    -- Or open new window
    window.new(uris)
end

-------------------------------------------
-- Open URIs from other luapdf instances --
-------------------------------------------

if unique then
    unique.add_signal("message", function (msg, screen)
        local cmd, arg = string.match(msg, "^(%S+)%s*(.*)")
        local w = lousy.util.table.values(window.bywidget)[1]
        if cmd == "tabopen" then
            w:new_tab(arg)
        elseif cmd == "winopen" then
            w = window.new((arg ~= "") and { arg } or {})
        end
        w.win:set_screen(screen)
    end)
end

-- vim: et:sw=4:ts=8:sts=4:tw=80
