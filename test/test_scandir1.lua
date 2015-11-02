
local lgi  = require     'lgi'
local GLib = lgi.require 'GLib'

local async = require "async"

local main_loop = GLib.MainLoop()

-- Test if the number of folder matches
local counter = 0

async.directory.scan("/","")
: connect_signal("request::completed", function(signame, var)

   local counter2 = 0

   for k,v in var:ipairs() do
      print("MEH",k,v)
      counter2 = counter2 + 1
   end

   os.exit(counter2 == counter and 0 or 1)
end)
: connect_signal("request::folder", function(signame, var)
   local v = var
   counter = counter + 1
end)

print("Starting main loop")

main_loop:run()