local lgi  = require     'lgi'
local GLib = lgi.require 'GLib'

local async = require "async"

local main_loop = GLib.MainLoop()

async.file.read("/etc/fstab")
: connect_signal("request::completed", function(content)
   print(content)
   os.exit(0)
end)

print("Starting main loop")

main_loop:run()