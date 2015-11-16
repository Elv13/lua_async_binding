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
 * List elements in a directory
 * @param path The directory path
 * @param attributes, a comma separated list of arguments
 *
 * @see https://developer.gnome.org/gio/stable/gio-GFileAttribute.html
 */
request_t *
aio_scan_directory(const char *path, const char *attributes)
{
   auto f = g_file_new_for_path(path);
   auto r = new request_t {NULL, 0};

   const auto attr = attributes ?: G_FILE_ATTRIBUTE_STANDARD_NAME;

   g_file_enumerate_children_async(f,
      attr                          , /* File attributes                      */
      G_FILE_QUERY_INFO_NONE        , /* Flags                                */
      G_PRIORITY_DEFAULT            , /* Priority                             */
      UNCANCELLABLE                 , /* Cancellable                          */
      CPP_G_ASYNC_CALLBACK((GObject *obj, GAsyncResult *res, gpointer usr_dt) {
         auto content = g_file_enumerate_children_finish(
            (GFile*)obj, res, NULL
         );

         auto r = (request_t*) usr_dt;

         g_file_enumerator_next_files_async(content,
            99999             , /* Maximum number of file HACK */
            G_PRIORITY_DEFAULT, /* Priority                    */
            UNCANCELLABLE     , /* Cancellable                 */
            CPP_G_ASYNC_CALLBACK(
               (GObject *obj, GAsyncResult *res, gpointer user_data) {
                  auto all_files = g_file_enumerator_next_files_finish(
                     (GFileEnumerator *) obj, res, NULL
                  );

                  auto r = (request_t*) user_data;

                  static auto string_array = g_variant_type_new("as");

                  auto builder = g_variant_builder_new(string_array);

                  for (auto l = all_files; l; l = l->next) {

                     auto file_info = (GFileInfo*) l->data;

                     //TODO parse attributes, this code isn't valid
                     auto attr_type = g_file_info_get_attribute_type(
                        file_info, G_FILE_ATTRIBUTE_STANDARD_NAME
                     );

                     //TODO if more than 1 attribute, pack them into 
                     //a string-GVariant dictionary

                     /* Not all attributes are strings */
                     if (attr_type == G_FILE_ATTRIBUTE_TYPE_OBJECT)
                        printf("IS OBJECT\n"); //TODO do something about it
                     else if (attr_type == G_FILE_ATTRIBUTE_TYPE_STRING
                        || attr_type == G_FILE_ATTRIBUTE_TYPE_BYTE_STRING) {

                        auto name = g_variant_new_string(
                           g_file_info_get_attribute_as_string(
                              file_info, G_FILE_ATTRIBUTE_STANDARD_NAME
                           )
                        );

                        emit_signal(r, "request::folder", name, NULL);

                        //TODO use "a{av}" if there is multiple attributes
                        g_variant_builder_add_value(builder, name);
                     }
                     // do something with l->data
                     g_object_unref(file_info);
                  }

                  auto folder_list = g_variant_builder_end (builder);

                  /*DEBUG ONLY*/
                  auto test_arg = g_variant_new_string("cyborg bobcat");

                  emit_signal(r, "request::completed", folder_list, test_arg,  NULL);

                  g_variant_builder_unref(builder);

                  g_list_free(all_files);
                  g_file_enumerator_close_async((GFileEnumerator *) obj,
                     G_PRIORITY_DEFAULT, /* Priority    */
                     UNCANCELLABLE     , /* Cancellable */
                     NO_CALLBACK       , /* Callback    */
                     r                   /* Data        */
                  );
               }
            ),
            r
         );

         //TODO emit error if !content
      }),
      r /* Data */
   );

   g_object_unref(f);

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
   auto r = new request_t {NULL, 0};

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

      printf("GET CALLED\n");

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
   auto r    = new request_t {NULL, 0};

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
   auto r = new request_t {NULL, 0};

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
   auto r    = new request_t {NULL, 0};
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
   auto r    = new request_t {NULL, 0};
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
   auto r = new request_t {NULL, 0};

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