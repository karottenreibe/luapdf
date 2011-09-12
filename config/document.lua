----------------------------
-- Poppler Document class --
----------------------------

-- Document class table
document = {}

-- Table of functions which are called on new document widgets.
document.init_funcs = {
    -- Update history indicator
    hist_update = function (doc, w)
        doc:add_signal("load-status", function (doc, status)
            if w:is_current(doc) then
                w:update_hist(doc)
            end
        end)
    end,

    -- Update tab titles
    tablist_update = function (doc, w)
        doc:add_signal("load-status", function (doc, status)
            if status == "provisional" or status == "finished" or status == "failed" then
                w:update_tablist()
            end
        end)
    end,

    -- Update scroll widget
    scroll_update = function (doc, w)
        doc:add_signal("expose", function (doc)
            if w:is_current(doc) then
                w:update_scroll(doc)
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
        doc:add_signal("button-release", function (doc, mods, button, context)
            (w.search_state or {}).marker = nil
            if w:hit(mods, button, { context = context }) then
                return true
            end
        end)
    end,

    layouting = function (doc, w)
        doc:add_signal("layout", function (doc)
            local width = 0
            local height = 0
            local spacing = 10

            for _, p in ipairs(doc.pages) do
                p.y = height
                if (p.width > width) then width = p.width end
                height = height + p.height + spacing
            end

            for _, p in ipairs(doc.pages) do
                p.x = (width - p.width) / 2
            end

            if height > 0 then height = height - spacing end

            return width, height
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

    -- Zoom functions
    zoom_in = function (doc, w, step)
        step = step or globals.zoom_step or 0.1
        doc.zoom = doc.zoom + step
    end,

    zoom_out = function (doc, w, step)
        step = step or globals.zoom_step or 0.1
        doc.zoom = math.max(0.01, doc.zoom - step)
    end,

    zoom_set = function (doc, w, level)
        doc.zoom = level or 1.0
    end,

    get_current_page = function (doc, w)
        local s = doc.scroll
        local mid = {
            x = s.x + (s.xpage_size / 2),
            y = s.y + (s.ypage_size / 2),
        }
        for idx, page in ipairs(doc.pages) do
            local l = page.x
            local r = page.x + page.width
            local t = page.y
            local b = page.y + page.height
            if l <= mid.x and r >= mid.x and t <= mid.y and b >= mid.y then
                return idx
            end
        end
        return 1
    end,

    scroll_to_dest = function (doc, w, dest)
        local p = doc.pages[dest.page]
        if not p then return end
        local x = p.x + dest.x
        local y = p.y + dest.y
        document.methods.scroll(doc, w, { x = x, y = y })
    end,

    print = function (doc, w, p)
        doc:print(p)
        w:notify("print successful")
    end,
}

document.scroll_parse_funcs = {
    -- Abs "100px"
    ["^(%d+)px$"] = function (_, _, px, _) return px end,

    -- Rel "+/-100px"
    ["^([-+]%d+)px$"] = function (s, axis, px, _) return s[axis] + px end,

    -- Abs "10%"
    ["^(%d+)%%$"] = function (s, axis, pc, _)
        return math.ceil(s[axis.."max"] * (pc / 100))
    end,

    -- Rel "+/-10%"
    ["^([-+]%d+)%%$"] = function (s, axis, pc, _)
        return s[axis] + math.ceil(s[axis.."max"] * (pc / 100))
    end,

    -- Abs "10p" (pages)
    ["^(%d+%.?%d*)p$"] = function (s, axis, p, _)
        return math.ceil(s[axis.."page_size"] * p)
    end,

    -- Rel "+10p" (pages)
    ["^([-+]%d+%.?%d*)p$"] = function (s, axis, p, _)
        return s[axis] + math.ceil(s[axis.."page_size"] * p)
    end,

    -- Abs "10th" (page)
    ["^(%d+)th$"] = function (s, _, p, doc)
        return (doc.pages[p] or {}).y or s.ymax
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
                if n then scroll[axis] = func(scroll, axis, tonumber(n), doc) end
            end
        end
    end
end

function document.new(w, path, password)
    local d = widget{type = "document"}
    d.path = path
    d.password = password

    -- Call document init functions
    for k, func in pairs(document.init_funcs) do
        func(d, w)
    end

    d:load()
    return d
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
