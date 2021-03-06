#include "../lib/async.h"

#include "common.h"

#include <glib-2.0/glib.h>


void test_handler(void* r, const char* signal_name, cairo_surface_t **sp, gpointer *gp, GVariant* var)
{
   request_t* request = (request_t*) r;

   if (!strcmp(signal_name, "request::completed"))
      exit(0);

   printf("Failed: Something else than request::completed was emited\n");

   exit(1);
}

int main(int argc, char** argv)
{
   GMainLoop* main_loop = g_main_loop_new (NULL, FALSE);

   request_t* r = aio_append_to_file("/tmp/foobar", "test_content", sizeof("test_content"));
   r->ohandler  = test_handler;

   auto_timeout(10);

   g_main_loop_run(main_loop);

   return 1;
}