#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>

int timer_timeout(gpointer p)
{
   printf("Failed: Timeout\n");

   exit(1);

   return 0;
}

void auto_timeout(unsigned int timeout)
{
   g_timeout_add_seconds(timeout, timer_timeout, NULL);
}

#endif