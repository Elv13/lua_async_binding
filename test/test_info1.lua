local lgi  = require     'lgi'
local GLib = lgi.require 'GLib'

local async = require "async"

local main_loop = GLib.MainLoop()

async.file.info("/etc/fstab", {
   "standard::name"     , --string
   "owner::user"        , --string
   "standard::type"     , --uint32
})
: connect_signal("request::completed", function(info)
   local name  = info["standard::name"].value
   local owner = info["owner::user"   ].value
   local t     = info["standard::type"].value
   print( "Name" , name  )
   print( "Owner", owner )
   print( "Type" , t     )

   if type(name) ~= "string" or type(owner) ~= "string" or type(t) ~= "number" then
      print("Invalid type")
      os.exit(1)
   end

   os.exit(0)
end)

-- Make sure it doesn't get stuck if the request isn't completed
GLib.timeout_add_seconds(10, 10, function()
   print("Timeout")
   os.exit(2)
end)

print("Starting main loop")

main_loop:run()