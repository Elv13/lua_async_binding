local lgi  = require     'lgi'
local core = require     'lgi.core'
local GLib = lgi.require 'GLib'

require("libluabridge")

local function connect_signal(request, signal_name, callback, cancelled_callback)

   request_connect(request._native, signal_name, function(signal_name, var)
      assert(type(var) == 'userdata')
      callback(unpack(core.record.new(GLib.Variant, var, true).value))
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

local mt = nil

-- The dispatcher work by drilling the dispatch table using metatable
-- callbacks until the function is found. The (native) function call
-- is wrapper around a lua object proxying the connection using an
-- OOP API rather than the flat one used by the native module

local function dispatcher(t, key)
   return setmetatable({_dispatch = t._dispatch[key]}, mt)
end

mt = {__index = dispatcher, __call = run}

return setmetatable({_dispatch = dispatch}, mt)