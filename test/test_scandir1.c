#include "../lib/async.h"

#include "common.h"

#include <glib-2.0/glib.h>


void test_handler(void* r, const char* signal_name, GVariant* var)
{
   request_t* request = (request_t*) r;

   if (strcmp(signal_name, "request::completed") && strcmp(signal_name, "request::folder")) {
      printf("Failed: Something else than request::completed was emitted: %s\n",signal_name);
      exit(1);
   }


   if (!strcmp(signal_name, "request::completed"))
      exit(0);
}

int main(int argc, char** argv)
{
   GMainLoop* main_loop = g_main_loop_new (NULL, FALSE);

   request_t* r = aio_scan_directory("/", NULL);
   r->handler = test_handler;

   auto_timeout(10);

   g_main_loop_run(main_loop);

   return 1;
}