local wibox = require "wibox"
local async = require "async"
local cairo = require( "lgi"         ).cairo

async.file.icon("/etc/fstab", 24, false)
: connect_signal("request::completed", function(icon, icon2)
   print("REALLY COMPLETED", type(icon), icon, "FOO", icon2)
    if icon2 then
      print(cairo.Surface:is_type_of(icon2))
      local pattern = cairo.Pattern.create_for_surface(icon2)
      pattern:set_extend(cairo.Extend.REPEAT)
      w = wibox {x=100, y=100, width=100, height=100, visible=true, bg = pattern, ontop=true}
    end
end)
: connect_signal("request::error", function(err)
    print(err)
end)