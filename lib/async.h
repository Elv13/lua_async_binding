#ifndef LIBASYNC
#define LIBASYNC

#include <glib-2.0/glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tracker struct used to interact with Lua from the GIO callbacks
 */
typedef struct request_t
{
   void (*handler)(struct request_t *request_ptr, const char* signal_name, GVariant* var);
   int listener_count;
   void *user_data;
   //TODO a list of error handler to delete
   //TODO a destructor
} request_t;

typedef enum {
   MON_FILE,
   MON_DIRECTORY
} MonitoringType;

/* Methodes */
request_t * aio_scan_directory(const char *path, const char* attributes                  );
request_t * aio_watch_gfile   (const char* path, MonitoringType t                        );
request_t * aoi_load_file     (const char* path                                          );
request_t * aio_append_to_file(const char* path, const char* content, const unsigned size);
request_t * aio_file_write    (const char* path, const char* content, const unsigned size);

#ifdef ENABLE_GTK
request_t* aio_icon_load(const char** names, int size, char is_symbolic);
#endif


#ifdef __cplusplus
}
#endif

#endif