LUA BRIDGE TO GLIB ASYNCHRONOUS
===============================

This module expose some GLib asynchronous methods to be used in Lua. It exists
to work around obscure bugs found in the current LGI binding and use some
asynchronous method not exposed by GObject-introspection.

## Build dependencies

 * LGI 0.7+ (runtime only)
 * Cairo
 * GLib2
 * GDK (build time only)
 * GTK 3+ (build time only)

## Installation

To install this module
cd into the directory
mkdir build
cmake -DCMAKE_INSTALL_PREFIX=$HOME
make install

If another prefix is used, the module will be installed as a shared lua module.

## Usage example

First, require it:

```lua
    local async = require "gears.async"
```

Various examples can be found in the `test` directory

### Print the cotent of /etc/fstab to stdout
```lua
    async.file.read("/etc/fstab")
    : connect_signal("request::completed", function(content)
        print(content)
    end)
```

### Create an awesome wibox with an icon as background

```lua
    async.icon.load("applications-internet", 22, false)
    : connect_signal("request::completed", function(icon, icon2)
        if icon2 then
          print(cairo.Surface:is_type_of(icon2))
          local pattern = cairo.Pattern.create_for_surface(icon2)
          pattern:set_extend(cairo.Extend.REPEAT)
          w = wibox {x=100, y=100, width=100, height=100, visible=true, bg = pattern, ontop=true}
        end
    end)
    : connect_signal("request::error", function(err)
        print(err)
    end)
```

## TODO

 * A lua module path issue that still need to be fixed to avoid symlinks
 * Various TODO in the code
 * Support async sockets and pipes
 * Support async files and images download
 * File COPY
 * Fix the file watched
 * All DBus async methods
 * Support extra file attributes like icons
 * More advanced command execution methods (merge Awesome spawn.c code?)