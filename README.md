# Luapdf

luapdf is a fast, light and simple to use micro-pdf-reader framework exensible
by Lua using the Poppler PDF rendering library and the GTK+ toolkit.

## Dont Panic!

You don't have to be a developer to use luapdf on a daily basis. If you are
familiar with zathura or similar PDF readers you will find luapdf behaves
similarly out of the box.

## Requirements

 * gtk2
 * Lua (5.1)
 * lfs (lua file system)
 * libpoppler
 * libcairo
 * libunique
 * help2man

## Compiling

To compile the stock luapdf run:

    make

To link against LuaJIT (if you have LuaJIT installed) run:

    make USE_LUAJIT=1

To build without libunique (which uses dbus) run:

    make USE_UNIQUE=0

To build with a custom compiler run:

    make CC=clang

Note to packagers: you may wish to build luapdf with:

    make DEVELOPMENT_PATHS=0

To prevent luapdf searching in relative paths (`./config` & `./lib`) for
user configs.

The `USE_LUAJIT=1`, `USE_UNIQUE=0`, `PREFIX=/path`, `DEVELOPMENT_PATHS=0`,
`CC=clang` build options do not conflict. You can use whichever you desire.

## Installing

To install luapdf run:

    sudo make install

The luapdf binary will be installed at:

    /usr/local/bin/luapdf

And configs to:

    /etc/xdg/luapdf/

And the luapdf libraries to:

    /usr/local/share/luapdf/lib/

To change the install prefix you will need to re-compile luapdf (after a
`make clean`) with the following option:

    make PREFIX=/usr
    sudo make PREFIX=/usr install

## Use Luapdf

Just run:

    luapdf [PATHS..]

Or to see the full list of luapdf launch options run:

    luapdf -h

## Configuration

The configuration options are endless, the entire reader is constructed by
the config files present in `/etc/xdg/luapdf`

There are several files of interest:

 * rc.lua       -- is the main config file which dictates which and in what
                   order different parts of the reader are loaded.
 * binds.lua    -- defines every action the reader takes when you press a
                   button or combination of buttons (even mouse buttons,
                   direction key, etc) and the reader commands (I.e.
                   `:quit`, `:restart`, `:open`, `:lua <code>`, etc).
 * theme.lua    -- change fonts and colours used by the interface widgets.
 * window.lua   -- is responsible for building the luapdf reader window and
                   defining several helper methods (I.e. `w:new_tab(path)`,
                   `w:close_tab()`, `w:close_win()`, etc).
 * document.lua -- is a wrapper around the document widget object and is
                   responsible for watching document signals (I.e.
                   "key-press", etc). This file
                   also provides several window methods which operate on the
                   current document tab (I.e. `w:reload()`,
                   `w:back()`, `w:forward()`).
 * modes.lua    -- manages the modal aspect of the reader and the actions
                   that occur when switching modes.
 * globals.lua  -- change global options like scroll/zoom step, default
                   window size, useragent, search engines, etc.

Just copy the files you wish to change (and the rc.lua) into
`$XDG_CONFIG_HOME/luapdf` (defaults to `~/.config/luapdf/`) and luapdf will
use those files when you next launch it.

## Uninstall

To delete luapdf from your system run:

    sudo make uninstall

If you installed with a custom prefix remember to add the identical prefix
here also, example:

    sudo make PREFIX=/usr uninstall

## Reporting Bugs

Please use the bug tracker at:

  http://github.com/karottenreibe/luapdf/issues

## Community

TODO
