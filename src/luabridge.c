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
   char                *signal_name; /* The signal name                    */
   int                  reference  ; /* The lua stack index of the cllback */
   struct connection_t *next       ; /* The next connection                */
} connection_t;

/* A request C <-> Lua binder */
typedef struct {
   connection_t *first_connection; /* The chained list first element        */
   connection_t *last_connection ; /* The chained list last element         */
   lua_State    *L               ; /* The lua context used for the request  */
   gboolean      is_completed    ; /* If request::completed has been emited */
} lua_request_t;

void
helper_check_completion(lua_request_t *lrequest, const char *signal_name)
{
   /* If the request if completed, assume it is ready for GC */
   if (!strcmp(signal_name, "request::completed"))
      lrequest->is_completed = TRUE;
}

void
lua_request_handler_object(
   struct request_t *req        ,
   const char       *signal_name,
   cairo_surface_t **surfaces   ,
   gpointer         *objects    ,
   GVariant         *var
)
{
   lua_request_t *lua_request = (lua_request_t*) req->user_data;
   connection_t  *conn        = lua_request->first_connection;
   unsigned       counter     = 0;

   for (;conn ; conn = (connection_t*) conn->next) {

      if (!strcmp(signal_name, conn->signal_name)) { //TODO stop using char* as signal

         /* Restore the callback to the top of the stack */
         lua_rawgeti(lua_request->L, LUA_REGISTRYINDEX, conn->reference);

         /* Push the signal name for debug purpose */
         lua_pushstring(lua_request->L, signal_name);

         /* Push the arguments to the stack */
         lua_pushlightuserdata(lua_request->L, var);

         /* Push all surfaces */
         for (; surfaces && *surfaces; ++counter && surfaces++)
            lua_pushlightuserdata(lua_request->L, *surfaces);

         //TODO pack all surfaces and objects into arrays

         /* Push a gobjects */
         for (; objects && *objects; ++counter && objects++)
            lua_pushlightuserdata(lua_request->L, *objects);

         /* Call the callback */
         lua_call(lua_request->L, counter + 2,0); //TODO handle retvals

      }

   }

   helper_check_completion(lua_request, signal_name);
}

void
init_request(lua_State *L, request_t *request)
{
   lua_request_t *lua_request = (lua_request_t*) malloc(sizeof(lua_request_t));

   lua_request->first_connection = NULL ;
   lua_request->last_connection  = NULL ;
   lua_request->L                = L    ;
   lua_request->is_completed     = FALSE;

   request->user_data = lua_request;
   request->ohandler  = lua_request_handler_object;
}

void
destroy_request(const request_t *request)
{
   lua_request_t *lua_request = (lua_request_t*) request->user_data;
   connection_t  *conn        = lua_request->first_connection;
   connection_t  *last_conn   = NULL;

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

attributes_list_t *
helper_parse_attributes(lua_State *L, int index)
{
   /* Store the parsed attributes into a linked list */
   attributes_list_t *first_node = NULL;
   attributes_list_t *last_node  = NULL;

   /* Iterate the attributes (if any) and convert them to CSV */
   if ( ! lua_isnil(L, -1) ) {

      lua_pushnil(L); /* Move the table to -2 */

      while(lua_next(L, -2)) {
         if(lua_isstring(L, -1)) {

            const char *i      = lua_tostring(L, -1);
            const int   length = strlen(i);

            attributes_list_t *node = (attributes_list_t *) malloc(
               sizeof(attributes_list_t)
            );

            node->length = strlen(i);
            node->next   = NULL;
            node->name   = (char *) malloc((length + 1) * sizeof(char));

            /* Check if the attribute need further request processing */
            node->is_pixmap = (!strcmp(i, "standard::symbolic-icon")) ||
               (!strcmp(i, "standard::icon"));
            //TODO handle width/height

            /* Copy the name into the buffer as Lua own the memory */
            strcpy(node->name, i);

            /* Update the linked list */
            if ( first_node ) {
               last_node->next = node;
               last_node       = node;
            }
            else
               last_node       = first_node = node;

         }
         lua_pop(L, 1);
      }

      lua_pop(L, 1);
   }

   return first_node;
}

static int
lua_aio_scan_directory(lua_State *L)
{
   if (!(
      lua_isstring  (L, -2) &&
      ( lua_type(L, -1) == LUA_TTABLE || lua_isnil(L, -1) )
      //TODO handle if attributes are a single string without ','
   )) {
      printf("Invalid arguments"); //TODO use luaL_typerror
      return 0;
   }

   /* Discards extra arguments */
   lua_settop(L, 2); //TODO add for all other methods
   //TODO handle if the arg is missing

   const char *path = lua_tostring(L, -2);

   attributes_list_t *first_node = helper_parse_attributes(L, -1);

   lua_pop(L, 2);

   request_t *r = aio_scan_directory(path, first_node);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aio_file_info(lua_State *L)
{
   if (!(
      lua_isstring  (L, -2) &&
      ( lua_type(L, -1) == LUA_TTABLE || lua_isnil(L, -1) )
      //TODO handle if attributes are a single string without ','
   )) {
      printf("Invalid arguments"); //TODO use luaL_typerror
      return 0;
   }

   /* Discards extra arguments */
   lua_settop(L, 2); //TODO add for all other methods
   //TODO handle if the arg is missing

   const char *path      = lua_tostring(L, -2);

   attributes_list_t *first_node = helper_parse_attributes(L, -1);

   lua_pop(L, 2);

   request_t *r = aio_file_info(path, first_node);
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
lua_aio_load_file(lua_State *L)
{
   const char *path = lua_tostring(L, -1);
   lua_pop(L, 1);

   request_t *r = aio_load_file(path);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aio_append_to_file(lua_State *L)
{
   const char *path       = lua_tostring(L, -2);
   const char *content    = lua_tostring(L, -1);
   lua_pop(L, 2);

   request_t *r = aio_append_to_file(path, content, strlen(content));
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aio_file_write(lua_State *L)
{
   const char *path       = lua_tostring(L, -2);
   const char *content    = lua_tostring(L, -1);
   lua_pop(L, 2);

   request_t *r = aio_file_write(path, content, strlen(content));
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

#ifdef ENABLE_GTK

static int
lua_aio_load_icon(lua_State *L)
{
   const char *name     = lua_tostring (L, -3);
   const int   size     = lua_tonumber (L, -2);
   const int   symbolic = lua_toboolean(L, -1);

   lua_pop(L, 3);

   const char *names[2] = {NULL, NULL};

   names[0] = name;

   request_t *r = aio_icon_load(names, size, symbolic);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

static int
lua_aio_file_icon(lua_State *L)
{
   const char *path     = lua_tostring (L, -3);
   const int   size     = lua_tonumber (L, -2);
   const int   symbolic = lua_toboolean(L, -1);

   lua_pop(L, 3);

   request_t *r = aio_file_icon(path, size, symbolic);
   init_request(L, r);

   lua_pushlightuserdata(L, r);

   return 1;
}

#endif

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
   lua_request_t *lr  = (lua_request_t *) r->user_data;
   connection_t *conn = (connection_t  *) malloc(sizeof(connection_t));
   conn->next         = NULL;
   conn->reference    = callback_ref;
   conn->signal_name  = (char*) malloc((strlen(signal) +1) * sizeof(char));

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
lua_request_collect(lua_State *L)
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

#ifdef ENABLE_GTK
luaopen_gears_async_libluabridge_gtk(lua_State *L)
#else
luaopen_gears_async_libluabridge(lua_State *L)
#endif
{
   /* Async I/O methods */
   lua_register(L, "aio_scan_directory" , lua_aio_scan_directory );
   lua_register(L, "aio_file_info"      , lua_aio_file_info      );
   lua_register(L, "aio_watch_gfile"    , lua_aio_watch_gfile    );
   lua_register(L, "aio_load_file"      , lua_aio_load_file      );
   lua_register(L, "aio_append_to_file" , lua_aio_append_to_file );
   lua_register(L, "aio_file_write"     , lua_aio_file_write     );

#ifdef ENABLE_GTK
   lua_register(L, "aio_load_icon"      , lua_aio_load_icon      );
   lua_register(L, "aio_file_icon"      , lua_aio_file_icon      );
#endif

   /* Request helper methods */
   lua_register(L, "request_connect"    , lua_request_connect    );

   return 0;
}