#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "raytracer-project2.h"

color *color_new(double r, double g, double b)
{
  color *c = (color*)malloc(sizeof(color));
  check_malloc("color_new", c);
  c->r = r;
  c->g = g;
  c->b = b;
  return c;
}

char *color_format = "(r=%lf,g=%lf,b=%lf)";

char *color_tos(color *c)
{
  char buf[256];
  sprintf(buf, color_format, c->r, c->g, c->b);
  return strdup(buf);
}

void color_show(FILE *f, color *c)
{
  fprintf(f, color_format, c->r, c->g, c->b);
}

color *color_add(color *c1, color *c2)
{
  double r0 = c1->r + c2->r;
  double g0 = c1->g + c2->g;
  double b0 = c1->b + c2->b;
  return color_new(r0 > 1 ? 1 : r0, g0 > 1 ? 1 : g0, b0 > 1 ? 1 : b0);
}
color *color_modulate(color *c1, color *c2)
{
  return color_new(c1->r * c2->r, c1->g * c2->g, c1->b * c2->b);
}
color *color_scale(double scalar, color *c)
{
  double r0 = c->r * scalar;
  double g0 = c->g * scalar;
  double b0 = c->b * scalar;
  return color_new(r0 > 1 ? 1 : r0, g0 > 1 ? 1 : g0, b0 > 1 ? 1 : b0);
}
