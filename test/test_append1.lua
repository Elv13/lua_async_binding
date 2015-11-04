local lgi  = require     'lgi'
local GLib = lgi.require 'GLib'

local async = require "async"

local main_loop = GLib.MainLoop()

os.execute("rm -rf /tmp/asyncappend")

async.file.write("/tmp/asyncappend", "test1234")
: connect_signal("request::completed", function()

   -- Test if the file content match
   async.file.read("/tmp/asyncappend")
   : connect_signal("request::completed", function(content)

      if content ~= "test1234" then
         print("content mismatch")
         os.execute("rm -rf /tmp/asyncappend")
         os.exit(1)
      end

      -- Append some text to the file
      async.file.append("/tmp/asyncappend", " append")
      : connect_signal("request::completed", function()

         -- Check again if the result match
         async.file.read("/tmp/asyncappend")
         : connect_signal("request::completed", function(content)
            if content == "test1234 append" then
               print("content match")
               os.execute("rm -rf /tmp/asyncappend")
               os.exit(0)
            else
               print("append failed")
               os.execute("rm -rf /tmp/asyncappend")
               os.exit(1)
            end
         end)
      end)
   end)

end)

-- Make sure it doesn't get stuck if the request isn't completed
GLib.timeout_add_seconds(10, 10, function()
   print("Timeout")
   os.exit(2)
end)

print("Starting main loop")

main_loop:run()