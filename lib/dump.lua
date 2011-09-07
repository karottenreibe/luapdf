----------------------------------------------------------------
-- Page/document dumping                                      --
-- Copyright © 2011 Fabian Streitel (luapdf@rottenrei.be)     --
----------------------------------------------------------------

-- Grab environment we need
local table = table
local string = string
local io = io
local os = os
local unpack = unpack
local type = type
local pairs = pairs
local ipairs = ipairs
local assert = assert
local capi = { luapdf = luapdf }
local lousy = require("lousy")
local util = lousy.util
local add_binds, add_cmds = add_binds, add_cmds
local tonumber = tonumber
local tostring = tostring
local window = window

local dump = function (w, fname, get_text)
    local downdir = string.gsub(fname, "[^/]*$", "")
    local file = a or luapdf.save_file("Save file", w.win, downdir, fname)
    if file then
        local fd = assert(io.open(file, "w"), "failed to open: " .. file)
        local text = get_text()
        assert(fd:write(text), "unable to save text")
        io.close(fd)
        w:notify("Dumped text to: " .. file)
    end
end

local cmd = lousy.bind.cmd
add_cmds({
    cmd("dump", function (w, a)
        local doc = w:get_current()
        if not doc then return w:error("no open document") end
        local fname = doc.path .. '.txt'
        dump(w, fname, function ()
            local text = {}
            for i, p in ipairs(doc.pages) do
                text[i] = p.text
            end
            return table.concat(text, "\n\n")
        end)
    end),

    cmd("dumppage", function (w, a)
        local doc = w:get_current()
        if not doc then return w:error("no open document") end
        local p = w:get_current_page()
        local fname = doc.path .. '.page' .. p .. '.txt'
        dump(w, fname, function ()
            return doc.pages[p].text
        end)
    end),
})

-- vim: et:sw=4:ts=8:sts=4:tw=80
