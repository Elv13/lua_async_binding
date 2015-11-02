#define LUA_LIB
#include "lua.h"
#include "lualib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/async.h"

#define LUA_BRIDGE_NAME "luabridge"

/* A signal connection */
typedef struct {
   char                *signal_name;
   int                 reference   ;
   struct connection_t *next       ;
} connection_t;

/* A request C <-> Lua binder */
typedef struct {
   connection_t *first_connection;
   connection_t *last_connection ;
   lua_State    *L               ;
   gboolean     is_completed     ;

} lua_request_t;

void
lua_request_handler(request_t* req, const char* signal_name, GVariant* var)
{
   lua_request_t *lua_request = (lua_request_t*) req->user_data;
   connection_t  *conn        = lua_request->first_connection;

   while (conn) {

      if (!strcmp(signal_name, conn->signal_name)) {

         /* Restore the callback to the top of the stack */
         lua_rawgeti(lua_request->L, LUA_REGISTRYINDEX, conn->reference);

         /* Push the arguments to the stack */
         lua_pushstring(lua_request->L, signal_name);

         lua_pushlightuserdata(lua_request->L, var);

         /* Call the callback */
         lua_call(lua_request->L,2,0); //TODO handle retvals

      }

      conn = (connection_t*) conn->next;
   }

   /* If the request if completed, assume it is ready for GC */
   if (!strcmp(signal_name, "request::completed"))
      lua_request->is_completed = TRUE;

}

void
init_request(lua_State *L, request_t *request)
{
   lua_request_t* lua_request = (lua_request_t*) malloc(sizeof(lua_request_t));

   lua_request->first_connection = NULL ;
   lua_request->last_connection  = NULL ;
   lua_request->L                = L    ;
   lua_request->is_completed     = FALSE;

   request->user_data = lua_request;
   request->handler   = lua_request_handler;
}

void
destroy_request(const request_t* request)
{
   lua_request_t *lua_request = (lua_request_t*) request->user_data;
   connection_t  *conn        = lua_request->first_connection;
   connection_t  * last_conn  = NULL;

   while (conn) {

      if (last_conn)
         free(last_conn);

      /* Allow the function to be garbage collected */
      luaL_unref(lua_request->L, LUA_REGISTRYINDEX, conn->reference);

      free(conn->signal_name);

      last_conn = conn;
   }

   if (last_conn)
      free(last_conn);

   free(lua_request);
}

static int
lua_aio_scan_directory(lua_State *L)
{
   const char* path       = lua_tostring(L, -2);
   const char* attributes = lua_tostring(L, -1);
   lua_pop(L, 2);

   request_t *r = aio_scan_directory(path, attributes);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aio_watch_gfile(lua_State *L)
{
   const char *path = lua_tostring(L, -2);
   const int   type = lua_tonumber(L, -1);
   lua_pop(L, 2);

   request_t *r = aio_watch_gfile(path, type);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aoi_load_file(lua_State *L)
{
   const char *path = lua_tostring(L, -1);
   lua_pop(L, 1);

   request_t *r = aoi_load_file(path);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aio_append_to_file(lua_State *L)
{
   const char* path       = lua_tostring(L, -3);
   const char* content    = lua_tostring(L, -2);
   const int   size       = lua_tonumber(L, -1);
   lua_pop(L, 3);

   request_t *r = aio_append_to_file(path, content, size);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aio_file_write(lua_State *L)
{
   const char* path       = lua_tostring(L, -3);
   const char* content    = lua_tostring(L, -2);
   const int   size       = lua_tonumber(L, -1);
   lua_pop(L, 3);

   request_t *r = aio_file_write(path, content, size);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_request_connect(lua_State *L)
{
   if (! (
      lua_isuserdata(L, -3) &&
      lua_isstring  (L, -2) &&
      lua_type      (L, -1) == LUA_TFUNCTION
   )) {
      printf("Invalid arguments");
      return 0;
   }

   /* Get the values from the stack */
   const request_t *r      = (request_t*) lua_touserdata(L, -3);
   const char      *signal = lua_tostring               (L, -2);

   /* Remove the function from the stack and add it to the registry */
   const int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

   /* Remove the other args, they are no longer needed */
   lua_pop(L, 2);

   /* Create the binder */
   lua_request_t* lr  = (lua_request_t*) r->user_data;
   connection_t* conn = (connection_t*) malloc(sizeof(connection_t));
   conn->next         = NULL;
   conn->reference    = callback_ref;
   conn->signal_name  = (char*) malloc(strlen(signal) * sizeof(char));

   /* Copy the signal name to avoid user after free */
   strcpy(conn->signal_name, signal);

   /* Updated the connection linked list */
   if (!lr->first_connection)
      lr->first_connection = conn;
   else
      lr->last_connection->next = (struct connection_t*) conn;

   lr->last_connection = conn;

   return 0;
}

static int
lua_request_collect(lua_State* L)
{
   if (!lua_isuserdata(L, -1))
      return;

   const request_t *r = (request_t*) lua_touserdata(L, -1);

   /* Only delete the request if it has been completed as it is
    * still referenced by G_CALLBACK
    */
   if (((lua_request_t*) r->user_data)->is_completed) {

      destroy_request(r);

      free((request_t*) r);
   }

   //TODO if the request is in progress, set a flag to delete it when it's done
   return 0;
}

LUALIB_API int
luaopen_libluabridge(lua_State *L)
{
   /* Async I/O methods */
   lua_register(L, "aio_scan_directory" , lua_aio_scan_directory );
   lua_register(L, "aio_watch_gfile"    , lua_aio_watch_gfile    );
   lua_register(L, "aoi_load_file"      , lua_aoi_load_file      );
   lua_register(L, "aio_append_to_file" , lua_aio_append_to_file );
   lua_register(L, "aio_file_write"     , lua_aio_file_write     );

   /* Request helper methods */
   lua_register(L, "request_connect"    , lua_request_connect    );
}