#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void todo(char *function_name)
{
  fprintf(stderr,"todo: %s (halting)\n",function_name);
  exit(1);
}

void check_malloc(char *function_name, void *h)
{
  if (h==NULL) {
    fprintf(stderr,"%s: malloc failed\n",function_name);
    exit(1);
  }
}
