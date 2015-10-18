
local lgi  = require     'lgi'
local Gio  = lgi.require 'Gio'
local core = require     'lgi.core'
local GLib = lgi.require 'GLib'

require("libluabridge")

local main_loop = GLib.MainLoop()

local cr = aio_scan_directory("/", "")

request_connect(cr, "request::completed", function(signame)
   print("COMPLETED!!!!", signame)
end)


request_connect(cr, "request::folder", function(signame)
   print("FOLDER!!!!", signame)
end)

print("Starting main loop")

main_loop:run()