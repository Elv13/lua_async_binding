#include "async.h"

#include <glib-2.0/glib.h>
#include <glib-2.0/glib/gvariant.h>
#include <glib-2.0/gio/gio.h>
#include <cairo/cairo.h>
#include <string.h>
#include <stdarg.h>

#ifdef ENABLE_GTK
 #include <gtk-3.0/gtk/gtk.h>
#endif

#include <functional>

/* Constants */
#define TUPLE_MAX_ARGS 12

#define UNCANCELLABLE NULL
#define NO_CALLBACK NULL

#define CPP_G_ASYNC_CALLBACK(lambda) (void(*)(GObject *f, GAsyncResult *res,\
gpointer)) ([]lambda)

#define CPP_G_SOURCE(lambda) (gboolean(*)(gpointer)) ([]lambda)

#define CPP_G_MONITOR_CALLBACK(lambda) (void(*)(GFileMonitor*, GFile*, GFile*\
, GFileMonitorEvent, gpointer)) ([]lambda)

/* Private prototypes */
void
emit_signal(request_t *r, const char *signal_name, GVariant *args, ...)
G_GNUC_NULL_TERMINATED;
void
emit_signal_o(request_t *r, const char *signal_name, cairo_surface_t **surfaces,
            GVariant *args, ...)
G_GNUC_NULL_TERMINATED;

/**
 * Convert an elipsys into a GVariant dynamic tuple
 * @param args The first elipsys element
 * @param ap   The elipsys ... struct
 * @return A packed GVariant tuple
 */
static GVariant *
pack_vars(GVariant *args, va_list ap)
{
   /* Pack all arguments into a GVariant tuple */
   int i = 0;

   /* Increase TUPLE_MAX_ARGS if a new function with more args is added */
   GVariant* vars[TUPLE_MAX_ARGS];

   for (GVariant *v = args; v; v = va_arg(ap, GVariant*)) {
      if (i == TUPLE_MAX_ARGS) {
         printf("Too many arguments\n");
         break;
      }

      /* Add the variant to the tuple */
      vars[i++] = v;

   }
   va_end(ap);

   vars[i] = NULL;

   /* Pack the tuple */
   return g_variant_new_tuple(vars, i);
}

/**
 * Emit a signal to the Lua request object
 *
 * The Lua side then expand the tuple into the callback arguments
 *
 * This is done to be compatible with how GDbus is supposed to be used,
 * allowing code to be shared between the async API and the GDbus API
 *
 * @param r The C-side request object
 * @param signal_name The name of the signal to be emitted
 * @param args A NULL terminated sequence of GVariants
 */
void
emit_signal(request_t *r, const char *signal_name, GVariant *args, ...)
{
   //TODO make sure the signal is not emitted if no listener are set
//    if ((!r->ohandler) || (!r->listener_count)
//       return;
   //TODO ignore emit after an error, it will be garbage

   /* No arguments */
   if (!args) {
      printf("NO ARG %s\n",signal_name);
   }

   va_list ap;
   va_start(ap, args);

   auto packed = pack_vars(args, ap);

   if (r->ohandler)
      r->ohandler(r, signal_name, NULL, NULL, packed);
}

void
emit_signal_o(request_t *r, const char *signal_name, cairo_surface_t **surfaces, GVariant *args, ...)
{
   if (!args) {
      printf("NO ARG %s\n",signal_name);
   }

   va_list ap;
   va_start(ap, args);

   auto packed = pack_vars(args, ap);

   if (r->ohandler)
      r->ohandler(r, signal_name, surfaces, NULL, packed);
}

/**
 * Helper function to avoid repetitive code
 */
void
emit_signal(request_t *r, const char *signal_name, const char *message)
{
   auto text = g_variant_new_string(message);
   emit_signal(r, signal_name, text, NULL);
}

/**
 * Convert an attributes_list_t into a string
 *
 * @param attributes A linked list of attributes
 * @return A CSV formatted list of attributes (by name)
 *
 * @warning A single buffer is shared, mutex are required for multi thread use
 */
char *
helper_format_attributes(attributes_list_t *attributes)
{
   /* Store the parsed attributes into a buffer */
   static char buffer[4096] = {'\0'};
   int         buffer_pos   = 0     ;

   /* Add each attributes to the buffer delimited by a ',' */
   for (; attributes; attributes = attributes->next) {

      /* In practice, this should happen unless attributes are used more than*/
      /* once or new attributes are invented. In that case, blame the user   */
      if (buffer_pos > 4096) {
         printf("Maximum attributes exeeded\n");
         buffer[buffer_pos - 1] = '\0';
      }

      strcpy(buffer + buffer_pos, attributes->name);

      buffer[buffer_pos + attributes->length] = ',';

      buffer_pos += attributes->length + 1;
   }

   /* Terminate the string */
   buffer[buffer_pos == 0 ? 0 : buffer_pos - 1] = '\0';

   return buffer;
}

/**
 * Extract attributes information out of the file descriptor
 *
 * If a single attributes is requested, this function returns it.
 * If multiple ones are, then they are packed into a map with the
 * attribute name as key and output as value
 *
 * @param file_info The file descriptor
 * @param attributes The attributes to query
 * @return the value packed as a GVariant or an a{sv} array of values
 */
GVariant *
helper_extract_attributes(
   GFileInfo *info,
   attributes_list_t *attributes,
   char **type
)
{
   const auto is_list = attributes->next != NULL;

   static char default_mixed_type[] = "a{sv}";

   /* For now, assume "aav" is the most optimal option for multi formet */
   *type         = is_list ? default_mixed_type : NULL;
   auto is_mixed = FALSE; //TODO use it

   //TODO handle the case where all attrs are the same type


   auto builder = is_list ?
      g_variant_builder_new(g_variant_type_new(*type))
      : NULL;

   for (; attributes; attributes = attributes->next) {

      //TODO parse attributes, this code isn't valid
      auto attr_type = g_file_info_get_attribute_type(
         info, attributes->name
      );

      GVariant *ret = NULL;

      //TODO make sure the mixed type detection run only once per request

#define CHECK_MIXED(var_type) \
      *type = (*type) ?: (char*) var_type; \
      is_mixed = strcmp(*type, var_type);\

#define CREATE_VAR(t, var_type) \
   ret = g_variant_new_##t(\
      g_file_info_get_attribute_##t(info, attributes->name)\
   );\
   CHECK_MIXED(var_type)\
break;
      switch(attr_type) {
         /* Handle the basic types */
         case G_FILE_ATTRIBUTE_TYPE_STRING      : CREATE_VAR( string  , "s")
         case G_FILE_ATTRIBUTE_TYPE_BOOLEAN     : CREATE_VAR( boolean , "b")
         case G_FILE_ATTRIBUTE_TYPE_UINT32      : CREATE_VAR( uint32  , "u")
         case G_FILE_ATTRIBUTE_TYPE_INT32       : CREATE_VAR( int32   , "i")
         case G_FILE_ATTRIBUTE_TYPE_UINT64      : CREATE_VAR( uint64  , "u")
         case G_FILE_ATTRIBUTE_TYPE_INT64       : CREATE_VAR( int64   , "i")

         /* Handle the more complex types */
         case G_FILE_ATTRIBUTE_TYPE_INVALID     :
            ret = g_variant_new_string("");
            break;
         case G_FILE_ATTRIBUTE_TYPE_BYTE_STRING :
            CHECK_MIXED("s")
            ret = g_variant_new_string(
               g_file_info_get_attribute_as_string(
                  info, G_FILE_ATTRIBUTE_STANDARD_NAME
               )
            );
            break;
            break;
         case G_FILE_ATTRIBUTE_TYPE_OBJECT      :
            //g_file_info_get_attribute_data
            if (attributes->is_pixmap) {
#ifdef ENABLE_GTK
            //TODO if the attributes is a pixmap, convert it to cairo_surface_t
#endif
            }
            else {
               /* I guess each relevant types need to be processed manually */
               /*CREATE_VAR(object)*/
            }
            break;
         case G_FILE_ATTRIBUTE_TYPE_STRINGV     :
            /*CREATE_VAR(stringv)*/ //TODO
            break;
      }
#undef CREATE_VAR

      ret = ret ?: g_variant_new_string(
         "" //TODO use binary for this
      );

      if (builder) {
         g_variant_builder_add(
            builder,
            "{sv}",
            attributes->name,
            g_variant_new_variant(ret),
            NULL
         );
      }
      else
         return ret;

   }

   return builder ? g_variant_builder_end(builder) : g_variant_new_variant(
      g_variant_new_string("")
   );
}

/**
 * If the attributes are not defined, act as the "ls" command and return file
 * name without path
 *
 * @return A default attribute list
 */
attributes_list_t *
helper_default_attributes()
{
   static int length = strlen( G_FILE_ATTRIBUTE_STANDARD_NAME );

   attributes_list_t *node = (attributes_list_t *) malloc(
      sizeof(attributes_list_t)
   );

   node->is_pixmap = FALSE;
   node->length    = length;
   node->next      = NULL;
   node->name      = (char *) malloc( (length + 1) * sizeof(char) );


   strcpy(node->name, G_FILE_ATTRIBUTE_STANDARD_NAME);

   return node;
}

/**
 * List elements in a directory
 * @param path The directory path
 * @param attributes, a comma separated list of arguments
 *
 * @see https://developer.gnome.org/gio/stable/gio-GFileAttribute.html
 * @see https://developer.gnome.org/gio/stable/GFileInfo.html
 *
 * @todo Add a new argument (optional) argument for icon/thumb size
 * @todo Add a new argument to check if icon/thumb need to be converted
 * @todo free the attributes memory
 */
request_t *
aio_scan_directory(const char *path, attributes_list_t *attributes)
{

   auto f = g_file_new_for_path(path);
   auto r = new request_t {NULL, 0, NULL};

   const auto attr = attributes ?: helper_default_attributes();

   typedef struct {
      request_t         *request   ;
      attributes_list_t *attributes;
   } scan_dir_data_t;

   auto data = new scan_dir_data_t { r, attr };

   g_file_enumerate_children_async(f,
      helper_format_attributes(attr), /* File attributes                      */
      G_FILE_QUERY_INFO_NONE        , /* Flags                                */
      G_PRIORITY_DEFAULT            , /* Priority                             */
      UNCANCELLABLE                 , /* Cancellable                          */
      CPP_G_ASYNC_CALLBACK((GObject *obj, GAsyncResult *res, gpointer usr_dt) {
         auto content = g_file_enumerate_children_finish(
            (GFile*)obj, res, NULL
         );

         auto data = (scan_dir_data_t*) usr_dt;

         g_file_enumerator_next_files_async(content,
            99999             , /* Maximum number of file HACK */
            G_PRIORITY_DEFAULT, /* Priority                    */
            UNCANCELLABLE     , /* Cancellable                 */
            CPP_G_ASYNC_CALLBACK(
               (GObject *obj, GAsyncResult *res, gpointer user_data) {

                  auto all_files = g_file_enumerator_next_files_finish(
                     (GFileEnumerator *) obj, res, NULL
                  );

                  auto data = (scan_dir_data_t*) user_data;

                  GVariantBuilder *builder = NULL;

                  for (auto l = all_files; l; l = l->next) {

                     auto file_info = (GFileInfo*) l->data;

                     char* type = NULL;

                     /* Get the attributes */
                     auto var = helper_extract_attributes(
                        file_info       ,
                        data->attributes,
                        &type
                     );

                     /* Now that the final type is known, create the builder */
                     if (!builder) {
                        char buffer2[32];
                        sprintf(buffer2, "a%s\0", type);

                        auto var_type = g_variant_type_new(buffer2);

                        builder = g_variant_builder_new(var_type);
                     }

                     g_variant_builder_add_value(builder, var);

                     emit_signal(data->request, "request::folder", var, NULL);

                     g_object_unref(file_info);

                  }


                  auto folder_list = g_variant_builder_end (builder);

                  /*DEBUG ONLY*/
                  auto test_arg = g_variant_new_string("cyborg bobcat");

                  emit_signal(data->request, "request::completed",
                     folder_list ,
                     test_arg    , //TODO remove
                     NULL
                  );

                  g_variant_builder_unref(builder);

                  g_list_free(all_files);
                  g_file_enumerator_close_async((GFileEnumerator *) obj,
                     G_PRIORITY_DEFAULT, /* Priority    */
                     UNCANCELLABLE     , /* Cancellable */
                     NO_CALLBACK       , /* Callback    */
                     data                /* Data        */
                  );
               }
            ),
            data
         );

         //TODO emit error if !content
      }),
      data /* Data */
   );

   g_object_unref(f);

   return r;
}

request_t *
aio_file_info(const char *path, attributes_list_t *attributes)
{
   auto f = g_file_new_for_path(path);
   auto r = new request_t {NULL, 0, NULL};

   const auto attr = attributes ?: helper_default_attributes();

   typedef struct {
      request_t         *request   ;
      attributes_list_t *attributes;
   } file_info_data_t;

   auto data = new file_info_data_t { r, attr };

   g_file_query_info_async (f,
      helper_format_attributes(attr),
      G_FILE_QUERY_INFO_NONE        ,
      G_PRIORITY_DEFAULT            ,
      UNCANCELLABLE                 ,
      CPP_G_ASYNC_CALLBACK(
         (GObject *obj, GAsyncResult *res, gpointer user_data) {
            auto info = g_file_query_info_finish(
               (GFile *) obj, res, NULL
            );

            auto data = (file_info_data_t*) user_data;

            char* type = NULL;

            /* Get the attributes */
            auto var = helper_extract_attributes(
               info            ,
               data->attributes,
               &type
            );

            emit_signal(data->request, "request::completed", var, NULL);
         }
      ),
      data);

   return r;
}

/**
 * Watch for changes in a file or directory
 *
 * @param path The file or directory path
 * @param if it is a file or a directory
 *
 * @todo auto detect parameter 2
 *
 * @note This function is unreliable, it may not work on all platforms and may
 * require GVFS to be running.
 */
request_t *
aio_watch_gfile(const char *path, MonitoringType t)
{
   auto r = new request_t {NULL, 0, NULL};

   auto file = g_file_new_for_path(path);

   auto func = t == MonitoringType::MON_FILE ?
               g_file_monitor_file     :
               g_file_monitor_directory;

   auto monitor = func(file,
      G_FILE_MONITOR_NONE , /* Flags       */
      UNCANCELLABLE       , /* Cancellable */
      NULL                  /* Error       */
   );

   //TODO use template / operator overload to catch the
   //error setting and send it to the request

   //TODO use an hash to map monitors to requests

   //TODO, get the changed file for MonitoringType::DIRECTORY

   //TODO doesn't work at all
   static auto lambda = CPP_G_MONITOR_CALLBACK((
      GFileMonitor *mon     ,
      GFile *file           ,
      GFile *other          ,
      GFileMonitorEvent type,
      gpointer user_data
   ) {
      auto r = (request_t*) user_data;

      switch (type) {
         case G_FILE_MONITOR_EVENT_CHANGED           :
            emit_signal(r, "request::changed"     , NULL, NULL);
            break;
         case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT :
            emit_signal(r, "request::changed_hint", NULL, NULL);
            break;
         case G_FILE_MONITOR_EVENT_DELETED           :
            emit_signal(r, "request::deleted"     , NULL, NULL);
            break;
         case G_FILE_MONITOR_EVENT_CREATED           :
            emit_signal(r, "request::created"     , NULL, NULL);
            break;
         case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED :
            emit_signal(r, "request::attributes"  , NULL, NULL);
            break;
         case G_FILE_MONITOR_EVENT_PRE_UNMOUNT       :
            emit_signal(r, "request::pre_umount"  , NULL, NULL);
            break;
         case G_FILE_MONITOR_EVENT_UNMOUNTED         :
            emit_signal(r, "request::umount"      , NULL, NULL);
            break;
         case G_FILE_MONITOR_EVENT_MOVED             :
            emit_signal(r, "request::moved"       , NULL, NULL);
            break;
      }
   });
   g_signal_connect(monitor, "changed", G_CALLBACK(lambda) , r);

   g_object_unref(monitor);
   g_object_unref(file);

   return r;
}

/**
 * Read the complete content of a file into memory
 * @param path The file path
 */
request_t *
aoi_load_file(const char *path)
{
   auto file = g_file_new_for_path(path);
   auto r    = new request_t {NULL, 0, NULL};

   g_file_load_contents_async(file,
      UNCANCELLABLE,
      CPP_G_ASYNC_CALLBACK(
         (GObject *obj, GAsyncResult *res, gpointer user_data) {
            auto  r         = (request_t*) user_data;
            char  **content = new char*;
            gsize *size     = new gsize;

            auto success = g_file_load_contents_finish(
               (GFile*)obj, res, content, size, NULL, NULL
            );

            auto name = g_variant_new_string(
               *content //TODO use binary for this
            );

            emit_signal(r, "request::completed", name, NULL);

            delete *content;
            delete size;
         }
      ),
      r /* Data */
   );

   g_object_unref(file);

   return r;
}

/**
 * Write to an output stream
 *
 * Helper function to mutualize the handling of file writing and socket handling
 *
 * @param stream An open output stream
 * @param content The content to append to the stream
 * @param size The content size
 */
request_t *
aio_stream_write(
   GOutputStream *stream ,
   const char    *content,
   unsigned      size
)
{
   auto r = new request_t {NULL, 0, NULL};

   g_output_stream_write_async((GOutputStream*)stream,
      content           ,
      size              ,
      G_PRIORITY_DEFAULT,
      UNCANCELLABLE     ,
      CPP_G_ASYNC_CALLBACK(
         (GObject *obj, GAsyncResult *res, gpointer user_data) {
            auto r = (request_t*) user_data;

            g_output_stream_write_finish((GOutputStream*) obj, res, NULL);

            emit_signal(r, "request::completed", NULL, NULL);
         }
      ),
      r /* Data */
   );

   return r;
}

/**
 * Append content into a new or existing file
 *
 * @param path The file path
 * @param content The content to append
 * @param size The content size
 */
request_t *
aio_append_to_file(
   const char     *path   ,
   const char     *content,
   const unsigned size
)
{
   auto r    = new request_t {NULL, 0, NULL};
   auto file = g_file_new_for_path(path);

   /* Captured lambdas cannot be converted to G_CALLBACK */
   typedef struct {
      const char *content;
      gsize      size    ;
      request_t  *request;
   } aoi_append_t;
   auto string = new aoi_append_t { content, size, r };

   /* Setup and create the write stream */
   g_file_append_to_async(file,
      G_FILE_CREATE_NONE,
      G_PRIORITY_DEFAULT,
      UNCANCELLABLE     ,
      CPP_G_ASYNC_CALLBACK(
         (GObject *obj, GAsyncResult *res, gpointer user_data) {
            auto stream = g_file_append_to_finish((GFile*) obj, res, NULL);

            aoi_append_t *string = (aoi_append_t*) user_data;

            /* Write the content into the stream */
            aio_stream_write(
               (GOutputStream*) stream, string->content, string->size
            );

            emit_signal(string->request, "request::completed", NULL, NULL);

            delete string;
            g_object_unref(stream);
         }
      ),
      string /* Data */
   );

   return r;
}

/**
 * Write or replace the content of a file
 * @param path The file path
 * @param content The file content
 * @param size The content size
 */
request_t *
aio_file_write(
   const char     *path   ,
   const char     *content,
   const unsigned size
)
{
   auto r    = new request_t {NULL, 0, NULL};
   auto file = g_file_new_for_path(path);

   g_file_replace_contents_async(file,
      content           , /* content       */
      size              , /* size          */
      NULL              , /* etag          */
      FALSE             , /* make a backup */
      G_FILE_CREATE_NONE, /* Flags         */
      UNCANCELLABLE     , /* Cancellable   */
      CPP_G_ASYNC_CALLBACK(
         (GObject *obj, GAsyncResult *res, gpointer user_data) {
            if (auto success = g_file_replace_contents_finish(
               (GFile*)obj, res, NULL /*etag*/, NULL)
            ) {
               auto r = (request_t*) user_data;
               emit_signal(r, "request::completed", NULL, NULL);
            }
         }
      ),
      r                   /* Data          */
   );
   g_object_unref(file);

   return r;
}

#ifdef ENABLE_GTK

/**
 * Load an icon using GTK theme engine, the first name found is returned
 *
 * @param names An array of valid application name or an XDG compliant icon name
 * @param size the icon size (size * size)
 * @param symbolic Newer GTK icon theme support colorable monochrome icons
 *
 * @warning This method only work when the GDK screens are available
 *
 * @todo add the other symbolic params to change colors
 * @todo support extra search paths
 */
request_t *
aio_icon_load(const char **names, int size, char is_symbolic)
{
   auto r = new request_t {NULL, 0, NULL};

   //TODO it is possible to get a theme per screen (why???)
   static GtkIconTheme *theme = gtk_icon_theme_get_default();

   if (!theme) {
      //TODO use glib idle or the receiver wont get the signal
      emit_signal(r, "request::error", "Load the default icon theme failed");
      return r;
   }

   const auto info = gtk_icon_theme_choose_icon(theme, names, size,
                                             (GtkIconLookupFlags) 0 /*Flags*/);

   if (!info) {
      //TODO use glib idle or the receiver wont get the signal
      emit_signal(r, "request::error", "The icon wasn't found");
      return r;
   }

//TODO they have different signature, mutualization wont be that easy
//    auto afunc = symbolic ?
//                 gtk_icon_info_load_symbolic_async  :
//                 gtk_icon_info_load_async           ;

   /* Information to complete the request */
   typedef struct {
      gboolean   symbolic ;
      request_t  *request ;
      int        size     ;
   } request_info_t;

   auto ri = new request_info_t {is_symbolic, r, size};
   gtk_icon_info_load_icon_async(info,
      UNCANCELLABLE,
      CPP_G_ASYNC_CALLBACK(
         (GObject *obj, GAsyncResult *res, gpointer user_data) {

            auto ri = (request_info_t*) user_data;

            auto ffunc = //ri->symbolic ?
                           //gtk_icon_info_load_symbolic_finish :
                           gtk_icon_info_load_icon_finish     ;

            auto pixbuf = ffunc((GtkIconInfo*) obj, res, NULL);

            if (!pixbuf) {
               emit_signal(ri->request, "request::error", "Failed to obtain pixbuf");
               return;
            }

            /* Convert to a cairo surface */
            auto sur = cairo_image_surface_create(
               CAIRO_FORMAT_ARGB32,
               ri->size           ,
               ri->size
            );

            auto cr = cairo_create(sur);
            gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
            cairo_paint(cr);

            //TODO find a way to wrap that into a GVariant
            static cairo_surface_t* surfaces[2];
            surfaces[1] = NULL;
            surfaces[0] = sur;
            emit_signal_o(ri->request, "request::completed", surfaces, NULL, NULL);
         }
      ),
      ri /* Data */
   );

   return r;
}

#endif /* ENABLE_GTK */

//    const char* names[] = {"list-add", NULL};
//    aio_icon_load(names, 24);


//TODO undefine all macros
//TODO fix all memory leaks