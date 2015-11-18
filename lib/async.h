#ifndef LIBASYNC
#define LIBASYNC

#include <glib-2.0/glib.h>
#include <cairo/cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tracker struct used to interact with Lua from the GIO callbacks
 */
typedef struct request_t
{

   /* Alternative syntax used to push raw cairo_surface_t and gpointer */
   void (*ohandler)(
      struct request_t *request_ptr, /* Request object                        */
      const char       *signal_name, /* The signal name                       */ //TODO
      cairo_surface_t **surfaces   , /* A NULL terminated list of surfaces    */
      gpointer         *objects    , /* A NULL terminated list of GObjects    */
      GVariant         *var          /* A packed GVariant tuple with all args */
   );

   int listener_count;
   void *user_data;
   //TODO a list of error handler to delete
   //TODO a destructor
} request_t;

/**
 * A linked list node of Gio file attributes name. This will be used to
 * convert it back into something Lua can handle
 */
typedef struct attributes_list_t
{
   char                     *name     ; /* The attribute name (with ::)     */
   short                     length   ; /* Attribute name length            */
   short                     height   ; /* The pixmap height (if required)  */
   short                     width    ; /* The pixmap width  (if required)  */
   gboolean                  is_pixmap; /* If the attribute is a GDK pixmap */
   struct attributes_list_t *next     ; /* The next attribute               */
} attributes_list_t;

typedef enum {
   MON_FILE,
   MON_DIRECTORY
} MonitoringType;

/* Methodes */
request_t *
aio_scan_directory(const char *path, attributes_list_t *attributes           );

request_t *
aio_file_info     (const char *path, attributes_list_t *attributes           );

request_t *
aio_watch_gfile   (const char *path, MonitoringType t                        );

request_t *
aoi_load_file     (const char *path                                          );

request_t *
aio_append_to_file(const char *path, const char *content, const unsigned size);

request_t *
aio_file_write    (const char *path, const char *content, const unsigned size);

/* This is not an installed .h, so using #ifdef is fine */
#ifdef ENABLE_GTK
request_t *
aio_icon_load(const char **names, int size, char is_symbolic);
#endif


#ifdef __cplusplus
}
#endif

#endif