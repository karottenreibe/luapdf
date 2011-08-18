--- luapdf core API
-- @author Mason Larobina &lt;mason.larobina&lt;AT&gt;gmail.com&gt;
-- @author Paweł Zuzelski &lt;pawelz&lt;AT&gt;pld-linux.org&gt;
-- @copyright 2010 Mason Larobina, Paweł Zuzelski
module("luapdf")

--- Luakit global table
-- @field config_dir config directory path (default: XDG_CONFIG_HOME)
-- @field data_dir data directory path (default: XDG_DATA_HOME)
-- @field cache_dir cache directory path (default: XDG_CACHE_HOME)
-- @field verbose verbosity (boolean value)
-- @field install_path luapdf installation path (read only property)
-- @field version luapdf version (read only property)
-- @class table
-- @name luapdf

--- All active window widgets
-- @class table
-- @name windows

--- Quit luapdf
-- @param -
-- @name quit
-- @class function

--- Get selection
-- @param clipboard X clipboard name ('primary', 'secondary' or 'clipboard')
-- @return A string with the selection (clipboard) content.
-- @name get_selection
-- @class function

--- Set selection
-- @param text UTF-8 string to be copied to clipboard
-- @param clipboard X clipboard name ('primary', 'secondary' or 'clipboard')
-- @name set_selection
-- @class function

--- Spawn process asynchronously
-- @param cmd Command to execute. It is parsed with simple shell-like parser.
-- @name spawn
-- @class function

--- Spawn process synchronously
-- @param cmd Command to execute. It is parsed with simple shell-like parser.
-- @return An exit status of the command.
-- @return A string containig data printed on stdout.
-- @return A string containig data printed on stderr.
-- @name spawn_sync
-- @class function

--- Get xdg-userdir directory
-- @param dir Type of directory ('DESKTOP', 'DOCUMENTS', 'DOWNLOAD', 'MUSIC', 'PITCURES', 'PUBLIC_SHARE', 'TEMPLATES', 'VIDEOS').
-- @return A path of xdg special directory.
-- @name get_special_dir
-- @class function

