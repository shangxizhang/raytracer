#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "raytracer-project2.h"

vector3 *vector3_new(double x, double y, double z)
{
  vector3 *v = (vector3*)malloc(sizeof(vector3));
  check_malloc("vector3_new", v);
  v->x = x;
  v->y = y;
  v->z = z;
  return v;
}

vector3 *vector3_add(vector3 *v1, vector3 *v2)
{
  return vector3_new(v1->x + v2->x, v1->y + v2->y, v1->z + v2->z);
}

vector3 *vector3_sub(vector3 *v1, vector3 *v2)
{
  return vector3_new(v1->x - v2->x, v1->y - v2->y, v1->z - v2->z);
}

vector3 *vector3_negate(vector3 *v)
{
  return vector3_new(-(v->x), -(v->y), -(v->z));
}

vector3 *vector3_scale(double scalar, vector3 *v)
{
  return vector3_new(scalar * (v->x), scalar * (v->y), scalar * (v->z));
}

double vector3_dot(vector3 *v1, vector3 *v2)
{
  return (v1->x) * (v2->x) + (v1->y) * (v2->y) + (v1->z) * (v2->z);
}

double vector3_magnitude(vector3 *v)
{
  return sqrt((v->x) * (v->x) + (v->y) * (v->y) + (v->z) * (v->z));
}

vector3 *vector3_normalize(vector3 *v)
{
  double norm = vector3_magnitude(v);
  return vector3_new((v->x) / norm, (v->y) / norm, (v->z) / norm);
}

char *vector_format = "<%lf,%lf,%lf>";

char *vector3_tos(vector3 *v)
{
  char buf[256]; /* way more than enough, but chars are cheap */
  sprintf(buf, vector_format, v->x, v->y, v->z);
  return strdup(buf);
}

void vector3_show(FILE *f, vector3 *v)
{
  fprintf(f, vector_format, v->x, v->y, v->z);
}

void vector3_normify(vector3 *v)
{
  double norm = vector3_magnitude(v);
  v->x /= norm;
  v->y /= norm;
  v->z /= norm;
}
