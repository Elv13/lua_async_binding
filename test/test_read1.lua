
local lgi  = require     'lgi'
local Gio  = lgi.require 'Gio'
local core = require     'lgi.core'
local GLib = lgi.require 'GLib'

require("libluabridge")

local main_loop = GLib.MainLoop()

local cr = aoi_load_file("/tmp/cat", "123", 3)

request_connect(cr, "request::completed", function(signame)
   print("COMPLETED!!!!", signame)
end)

print("Starting main loop")

main_loop:run()