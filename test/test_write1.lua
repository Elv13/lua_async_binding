local lgi  = require     'lgi'
local GLib = lgi.require 'GLib'

local async = require "async"

local main_loop = GLib.MainLoop()

os.execute("rm -rf /tmp/asyncwrite")

async.file.write("/tmp/asyncwrite", "test1234")
: connect_signal("request::completed", function()

   -- Test if the file content match
   async.file.read("/tmp/asyncwrite")
   : connect_signal("request::completed", function(content)
      if content == "test1234" then
         print("Content match")
         os.execute("rm -rf /tmp/asyncwrite")
         os.exit(0)
      else
         print("content mismatch")
         os.execute("rm -rf /tmp/asyncwrite")
         os.exit(1)
      end
   end)

end)

-- Make sure it doesn't get stuck if the request isn't completed
GLib.timeout_add_seconds(10, 10, function()
   print("Timeout")
   os.exit(2)
end)

print("Starting main loop")

main_loop:run()