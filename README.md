LUA BRIDGE TO GLIB ASYNCHRONOUS
===============================

This module expose some GLib asynchronous methods to be used in Lua. It exists
to work around obscure bugs found in the current LGI binding and use some
asynchronous method not exposed by GObject-introspection.

The goals is to provide an implementation of all common I/O or compute related
task that a window manager / desktop shell may require.

The library also aim to be fully unit tested and easy to integrate into
continuous integration systems. The C test are to detect some memory leaks
and make it easier to run with address sanitizer.

## Build dependencies

 * A CC and CCX compatible compiler (build time only)
 * Lua or Luajit with support for Lua 5.1 or 5.2 or 5.3 (runtime only)
 * LGI 0.7+ (runtime only)
 * Cairo
 * GLib2
 * Gio
 * GLib 2.0+
 * GDK (build time only)
 * GTK 3+ (build time only)
 * GVFS (optional, runtime only)

## Installation

To install this module
cd into the directory
mkdir build
cmake -DCMAKE_INSTALL_PREFIX=$HOME
make install

If another prefix is used, the module will be installed as a shared lua module.

## Methods

TODO list already implemented async methods

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

Also note that connections can be daisy-chained

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

### Iteration 1

Port Elv13 "fd_async" module from Lua to C (DONE)

### Iteration 2

Iteration 2 goal is to have a feature complete implementation ready to be
integrated into Awesome.

 * A lua module path issue that still need to be fixed to avoid symlinks
 * Various TODO in the code
 * Support async sockets and pipes
 * Support async files and images download
 * File COPY
 * File compare
 * File attributes (WIP)
 * File thumbnails (WIP)
 * Fix the file watched
 * All DBus async methods
 * Scan-dir recursive
 * Support extra file attributes like icons
 * More advanced command execution methods (merge Awesome spawn.c code?)
 * Helper methods to save and load lua tables to/from disk
 * Helper methods to parse JSON (in a thread) and load it into Lua
 * Helper methods to parse INI (in a thread) and load it into lua
 * Helper methods to parse XML (in a thread) and load it into lua
 * Write a proper error handler
 * Notify Lua or type errors
 * Fix the memory leaks (WIP)
 * Handle more glib errors and notify Lua about them
 * If no handler is connected to the error signal, create lua errors
 * "Compact syntax" async.file.read "/tmp/foo" : on_completed function(...) end
 * Find a way to add luadoc for the "virtual" auto generated/wrapped methods
 * Add unit tests for error case
 * Add unit test for memory leaks
 * Merge the github.com/elv13/wirefu module user API

### Iteration 3

Iteration 3 goal is to implement additional XDG standards and common protocols
on top of the async API.

 * Add XDG appmenu server support
 * The new standardized systray protocol used by KDE and Unity?
 * Merge the XDG menu support from Elv13 config + integrate Awesome menubar as fallback code when GTK is missing
 * Add NetworkManager client (optional runtime extension)
 * Add XDG notification server (optional runtime extension)
 * Add UPower client (optional runtime extension)
 * Add UDisk client (optional runtime extension)
 * Add MDP client (optional runtime extension)
 * Add logind client (optional runtime extension)
 * Add systemd client (optional runtime extension)
 * PulseAudio mixer client (optional runtime extension)
 * Some kind of weather service (optional runtime extension)
 * The "open document" API if the startard is ready (I haven't looked in a long time)
 * Compton DBus API? (optional runtime extension)
