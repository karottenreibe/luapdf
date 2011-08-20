----------------------------
-- Poppler Document class --
----------------------------

-- preserve PopplerDocument wrapper class locally
local clib = {
    document = document,
}

-- Document class table
document = {}

-- Table of functions which are called on new document widgets.
document.init_funcs = {
    -- Update history indicator
    hist_update = function (doc, w)
        doc:add_signal("load-status", function (v, status)
            if w:is_current(v) then
                w:update_hist(v)
            end
        end)
    end,

    -- Update tab titles
    tablist_update = function (doc, w)
        doc:add_signal("load-status", function (v, status)
            if status == "provisional" or status == "finished" or status == "failed" then
                w:update_tablist()
            end
        end)
    end,

    -- Update scroll widget
    scroll_update = function (doc, w)
        doc:add_signal("expose", function (v)
            if w:is_current(v) then
                w:update_scroll(v)
            end
        end)
    end,

    -- Catch keys in non-passthrough modes
    mode_key_filter = function (doc, w)
        doc:add_signal("key-press", function ()
            if not w.mode.passthrough then
                return true
            end
        end)
    end,

    -- Try to match a button event to a user's button binding else let the
    -- press hit the document.
    button_bind_match = function (doc, w)
        doc:add_signal("button-release", function (v, mods, button, context)
            (w.search_state or {}).marker = nil
            if w:hit(mods, button, { context = context }) then
                return true
            end
        end)
    end,
}

-- These methods are present when you index a window instance and no window
-- method is found in `window.methods`. The window then checks if there is an
-- active document and calls the following methods with the given doc instance
-- as the first argument. All methods must take `doc` & `w` as the first two
-- arguments.
document.methods = {
    -- Property functions
    get = function (doc, w, k)
        return doc:get_property(k)
    end,

    set = function (doc, w, k, v)
        doc:set_property(k, v)
    end,
}

document.scroll_parse_funcs = {
    -- Abs "100px"
    ["^(%d+)px$"] = function (_, _, px) return px end,

    -- Rel "+/-100px"
    ["^([-+]%d+)px$"] = function (s, axis, px) return s[axis] + px end,

    -- Abs "10%"
    ["^(%d+)%%$"] = function (s, axis, pc)
        return math.ceil(s[axis.."max"] * (pc / 100))
    end,

    -- Rel "+/-10%"
    ["^([-+]%d+)%%$"] = function (s, axis, pc)
        return s[axis] + math.ceil(s[axis.."max"] * (pc / 100))
    end,

    -- Abs "10p" (pages)
    ["^(%d+%.?%d*)p$"] = function (s, axis, p)
        return math.ceil(s[axis.."page_size"] * p)
    end,

    -- Rel "+10p" (pages)
    ["^([-+]%d+%.?%d*)p$"] = function (s, axis, p)
        return s[axis] + math.ceil(s[axis.."page_size"] * p)
    end,
}

function document.methods.scroll(doc, w, new)
    local scroll = doc.scroll
    for axis, val in pairs{ x = new.x, y = new.y } do
        if type(val) == "number" then
            scroll[axis] = val
        else
            for pat, func in pairs(document.scroll_parse_funcs) do
                local n = string.match(val, pat)
                if n then scroll[axis] = func(scroll, axis, tonumber(n)) end
            end
        end
    end
end

function document.new(w, uri, password)
    local doc = clib.document{uri = uri, password = password}

    doc.show_scrollbars = false

    -- Call document init functions
    for k, func in pairs(document.init_funcs) do
        func(doc, w)
    end
    return doc
end

-- Insert document method lookup on window structure
table.insert(window.indexes, 1, function (w, k)
    -- Get current document
    local doc = w.tabs[w.tabs:current()]
    if not doc then return end
    -- Lookup document method
    local func = document.methods[k]
    if not func then return end
    -- Return document method wrapper function
    return function (_, ...) return func(doc, w, ...) end
end)

-- vim: et:sw=4:ts=8:sts=4:tw=80