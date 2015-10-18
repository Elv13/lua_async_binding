#include "../lib/async.h"

#include "common.h"

#include <unistd.h>

#include <glib-2.0/glib.h>

gboolean was_created  = FALSE;
gboolean was_deleted  = FALSE;
gboolean was_modified = FALSE;

int create_file(gpointer p)
{
   system("echo test > /home/lepagee/test_watch1_create.txt");

   return 0;
}

int modify_file(gpointer p)
{
   system("echo ' other_test' >> /home/lepagee/test_watch1_create.txt");

   return 0;
}

int delete_file(gpointer p)
{
   system("rm /home/lepagee/test_watch1_create.txt");

   return 0;
}

void test_handler(void* r, const char* signal_name, GVariant* var)
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

   request_t* r = aio_watch_gfile("/home/lepagee/", MON_DIRECTORY);
   r->handler = test_handler;

   auto_timeout(10);

   g_timeout_add_seconds(1, create_file, NULL);
   g_timeout_add_seconds(2, modify_file, NULL);
   g_timeout_add_seconds(3, delete_file, NULL);

   g_main_loop_run(main_loop);

   return 1;
}