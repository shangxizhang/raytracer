#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "raytracer-project2.h"

ray3 *ray3_new(vector3 *origin, vector3 *direction)
{
  ray3 *r = (ray3*)malloc(sizeof(ray3));
  check_malloc("ray3_new", r);
  r->origin = origin;
  r->direction = direction;
  return r;
}
void ray3_free(ray3 *r)
{
  free(r->origin);
  free(r->direction);
  free(r);
}
vector3 *ray3_position(ray3 *r, double t)
{
  vector3 *t1 = vector3_scale(t, r->direction);
  vector3 *t2 = vector3_add(r->origin, t1);
  free(t1);
  return t2;
}

char* ray_format = "src <%lf,%lf,%lf>, dir <%lf,%lf,%lf>";

char *ray3_tos(ray3 *r)
{
  char buf[256];
  sprintf(buf, ray_format, r->origin->x, r->origin->y, r->origin->z,
          r->direction->x, r->direction->y, r->direction->z);
  return strdup(buf);
}
void ray3_show(FILE *f, ray3 *r)
{
  fprintf(f, ray_format, r->origin->x, r->origin->y, r->origin->z,
          r->direction->x, r->direction->y, r->direction->z);
}
