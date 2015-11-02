
local lgi  = require     'lgi'
local core = require     'lgi.core'
local GLib = lgi.require 'GLib'

require("libluabridge")

--This function work around a bug/limitation is LGI < 0.9.1 where GVariant
-- cannot be loaded using the usual LGI lightuserdata->lua constructs. The
-- issue has been reported and fixed upstream
local function called_from_C(userdata)
   local variant = core.record.new(GLib.Variant, userdata)

   return variant
end

local function connect_signal(request, signal_name, callback, cancelled_callback)

   request_connect(request._native, signal_name, function(sn, var)
      assert(type(var) == 'userdata')
      callback(sn,called_from_C(var))
   end)

   --TODO connect to the CAPI cancelled method

   -- Return the request to enable daisy-chaining
   return request
end

local dispatch = {
   directory = {
      scan           = aio_scan_directory,
      watch          = aio_watch_gfile   ,
   },
   file = {
      read           = aoi_load_file     ,
      append         = aio_append_to_file,
      write          = aio_file_write    ,
   },
}

local function run(self, ...)
   return {
      connect_signal = connect_signal,
      _native        = self._dispatch(...)
   }
end

local function dispatcher(t, key)
   print("Dispatch request to", key)

   return setmetatable({_dispatch = t._dispatch[key]}, {__index = dispatcher, __call = run})
end

return setmetatable({_dispatch = dispatch}, {__index = dispatcher, __call = run})