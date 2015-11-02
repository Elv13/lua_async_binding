
local lgi  = require     'lgi'
local Gio  = lgi.require 'Gio'
local core = require     'lgi.core'
local GLib = lgi.require 'GLib'

require("libluabridge")

local main_loop = GLib.MainLoop()

local cr = aio_file_write("/tmp/cat", "123", 3)

request_connect(cr, "request::completed", function(signame)
   print("COMPLETED!!!!", signame)
   os.exit(0)
end)



print("Starting main loop")

main_loop:run()