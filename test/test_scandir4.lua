local lgi  = require     'lgi'
local GLib = lgi.require 'GLib'

local async = require "async"

local main_loop = GLib.MainLoop()

-- Test if the number of folder matches
local counter = 0

-- This script test if file attributes with interger

async.directory.scan("/etc",{
   "standard::type"     , --uint32
})
: connect_signal("request::completed", function(folder_list, extra)
   local counter2 = 0

   for k,v in folder_list:ipairs() do
      print("TYPE",v)
      --TODO check types
      --TODO check if the thumbnail exist if is_valie == true
      counter2 = counter2 + 1
   end

   os.exit(counter2 == counter and 0 or 1)
end)
: connect_signal("request::folder", function(var)
   local v = var
   counter = counter + 1
end)

-- Make sure it doesn't get stuck if the request isn't completed
GLib.timeout_add_seconds(10, 10, function()
   print("Timeout")
   os.exit(2)
end)

print("Starting main loop")

main_loop:run()