-- Global variables for luapdf
globals = {
    homepage            = "http://luapdf.org/",
 -- homepage            = "http://github.com/mason-larobina/luapdf",
    scroll_step         = 40,
    zoom_step           = 0.1,
    max_cmd_history     = 100,
    max_srch_history    = 100,
 -- http_proxy          = "http://example.com:3128",
    default_window_size = "800x600",

 -- Disables loading of hostnames from /etc/hosts (for large host files)
 -- load_etc_hosts      = false,
 -- Disables checking if a filepath exists in search_open function
 -- check_filepath      = false,
}

-- Make useragent
local arch = string.match(({luapdf.spawn_sync("uname -sm")})[2], "([^\n]*)")
local lkv  = string.format("luapdf/%s", luapdf.version)
local wkv  = string.format("WebKitGTK+/%d.%d.%d", luapdf.webkit_major_version, luapdf.webkit_minor_version, luapdf.webkit_micro_version)
local awkv = string.format("AppleWebKit/%s.%s+", luapdf.webkit_user_agent_major_version, luapdf.webkit_user_agent_minor_version)
globals.useragent = string.format("Mozilla/5.0 (%s) %s %s %s", arch, awkv, wkv, lkv)

-- Search common locations for a ca file which is used for ssl connection validation.
local ca_files = {
    -- $XDG_DATA_HOME/luapdf/ca-certificates.crt
    luapdf.data_dir .. "/ca-certificates.crt",
    "/etc/certs/ca-certificates.crt",
    "/etc/ssl/certs/ca-certificates.crt",
}
-- Use the first ca-file found
for _, ca_file in ipairs(ca_files) do
    if os.exists(ca_file) then
        soup.set_property("ssl-ca-file", ca_file)
        break
    end
end

-- Change to stop navigation sites with invalid or expired ssl certificates
soup.set_property("ssl-strict", false)

-- Set cookie acceptance policy
cookie_policy = { always = 0, never = 1, no_third_party = 2 }
soup.set_property("accept-policy", cookie_policy.always)

-- List of search engines. Each item must contain a single %s which is
-- replaced by URI encoded search terms. All other occurances of the percent
-- character (%) may need to be escaped by placing another % before or after
-- it to avoid collisions with lua's string.format characters.
-- See: http://www.lua.org/manual/5.1/manual.html#pdf-string.format
search_engines = {
    luapdf      = "http://luapdf.org/search/index/luapdf?q=%s",
    google      = "http://google.com/search?q=%s",
    duckduckgo  = "http://duckduckgo.com/?q=%s",
    wikipedia   = "http://en.wikipedia.org/wiki/Special:Search?search=%s",
    debbugs     = "http://bugs.debian.org/%s",
    imdb        = "http://imdb.com/find?s=all&q=%s",
    sourceforge = "http://sf.net/search/?words=%s",
}

-- Set google as fallback search engine
search_engines.default = search_engines.google
-- Use this instead to disable auto-searching
--search_engines.default = "%s"

-- Per-domain webview properties
-- See http://webkitgtk.org/reference/webkitgtk-WebKitWebSettings.html
domain_props = { --[[
    ["all"] = {
        ["enable-scripts"]          = false,
        ["enable-plugins"]          = false,
        ["enable-private-browsing"] = false,
        ["user-stylesheet-uri"]     = "",
    },
    ["youtube.com"] = {
        ["enable-scripts"] = true,
        ["enable-plugins"] = true,
    },
    ["bbs.archlinux.org"] = {
        ["user-stylesheet-uri"]     = "file://" .. luapdf.data_dir .. "/styles/dark.css",
        ["enable-private-browsing"] = true,
    }, ]]
}

-- vim: et:sw=4:ts=8:sts=4:tw=80
