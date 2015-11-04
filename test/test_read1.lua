local lgi  = require     'lgi'
local GLib = lgi.require 'GLib'

local async = require "async"

local main_loop = GLib.MainLoop()

async.file.read("/etc/fstab")
: connect_signal("request::completed", function(content)
   print(content)
   os.exit(0)
end)

-- Make sure it doesn't get stuck if the request isn't completed
GLib.timeout_add_seconds(10, 10, function()
   print("Timeout")
   os.exit(2)
end)

print("Starting main loop")

main_loop:run()