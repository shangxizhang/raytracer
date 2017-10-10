#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "raytracer-project2.h"

/*** ran on valgrind. no memory leak.**/
color* color_dup(color *c)
{
  return color_new(c->r, c->g, c->b);
}

//color is shallow copied
hit *hit_new_shallow(double t, color *surf, color *shine, vector3 *surf_norm)
{
  hit *h = (hit*)malloc(sizeof(hit));
  check_malloc("hit_new", h);
  h->t = t;
  h->surface_color = surf;
  h->surface_normal = vector3_new(surf_norm->x, surf_norm->y, surf_norm->z);
  h->shine = color_dup(shine);
  return h;
}

//color is deep copied
hit *hit_new_deep(double t, color *surf, color *shine, vector3 *surf_norm)
{
  hit *h = (hit*)malloc(sizeof(hit));
  check_malloc("hit_new", h);
  h->t = t;
  h->surface_color = color_dup(surf);
  h->surface_normal = vector3_new(surf_norm->x, surf_norm->y, surf_norm->z);
  h->shine = color_dup(shine);
  return h;
}

void hit_free(hit *h)
{ if (h) {
    free(h->surface_color);
    free(h->surface_normal);
    free(h->shine);
    free(h);
  }
}

vector3 *logical_coord(uint image_height, uint image_width,
                       uint pixel_row, uint pixel_col)
{
  //association : x ~ col ~ width
  //              y ~ row ~ height
  double x_init = -1.0, y_init = 1.0;
  double logical_sidelen = 2.0 / image_width;
  if (image_width > image_height)
    y_init *= ((double)image_height / (double)image_width);
  else if (image_width < image_height) {
    x_init *= ((double)image_width / (double)image_height);
    logical_sidelen = 2.0 / image_height;
  }
  double x = x_init + 0.5 * logical_sidelen + (pixel_col - 1) * logical_sidelen;
  double y = y_init - 0.5 * logical_sidelen - (pixel_row - 1) * logical_sidelen;
  return vector3_new(x, y, 0);
}

int hit_sphere(vector3 *v1, vector3 *v2, sphere *s)
{
  vector3 *a = vector3_sub(v1, s->center);
  double b = vector3_dot(a, v2);
  double c = vector3_dot(a, a) - s->radius * s->radius;
  double d = b * b - c;
  double t = - b - sqrt(d);
  free(a);
  return (d > 0 && t > 0) ? 1 : 0;
}

int hit_rect(vector3 *v1, vector3 *v2, rectangle *r)
{
  int result;
  ray3 * rr = ray3_new(v1, v2);
  vector3 *n = vector3_new(0, 0, -1);
  double d = r->upper_left->z;
  double t = -(vector3_dot(v1, n) + d) / vector3_dot(v2, n);
  vector3 *hitpoint = ray3_position(rr, t);
  if (t > 0 && hitpoint->x >= r->upper_left->x &&
      hitpoint->x <= r->upper_left->x + r->w &&
      hitpoint->y >= r->upper_left->y - r->h &&
      hitpoint->y <= r->upper_left->y) {
    result = 1;
  } else {
    result = 0;
  }
  free(rr);
  free(n);
  free(hitpoint);
  return result;
}

int in_shadow(vector3 *loc, light *dl, object_list *objs)
{
  int result = 0;
  vector3 *nudge = vector3_scale(0.0001, dl->direction);
  vector3 *lifted = vector3_add(loc, nudge);
  while (objs != NULL) {
    switch (objs->first.tag) {
    case SPHERE:
      result += hit_sphere(lifted, dl->direction, objs->first.o.s);
      break;
    case RECTANGLE:
      result += hit_rect(lifted, dl->direction, objs->first.o.r);
      break;
    default:
      fprintf(stderr, "bad tag in obj\n");
      exit(1);
    }
    objs = objs->rest;
  }
  free(nudge);
  free(lifted);
  return result;
}

//helper for background functional color, color is malloced
color* bkg_color(ray3 *r, color * (*f)(vector3 *x, vector3 *y))
{
  double n = (-r->origin->z) / r->direction->z;
  vector3 *v = vector3_new(r->direction->x * n,
                           r->direction->y * n, 0);
  color *c = f(r->origin, v);
  free(v);
  return c;
}

//this function does not change values from h
color *light_color(scene * s, ray3 * r, hit * h)
{
  if (h == NULL) {
    switch (s->bg.tag) {
    case CONSTANT:
      return color_dup(s->bg.c.k);
    case FUNCTION:
      return bkg_color(r, s->bg.c.f);
    default:
      fprintf(stderr, "bad tag\n");
      exit(1);
    }
  }
  color *k = h->surface_color;
  color *d;
  color *surf_color;
  vector3 *n = h->surface_normal;
  vector3 *l = s->dir_light->direction;
  vector3 *loc = ray3_position(r, h->t);
  double nl = vector3_dot(n, l);
  if (in_shadow(loc, s->dir_light, s->objects)) {
    free(loc);
    return color_modulate(k, s->amb_light);
  } else {
    free(loc);
    color *tmp1 = color_scale(fmax(nl, 0), s->dir_light->color);
    color *tmp2 = color_add(s->amb_light, tmp1);
    surf_color = color_modulate(k, tmp2);
    free(tmp1);
    free(tmp2);
  }
  if (nl <= 0) {
    d = color_new(0, 0, 0);
  } else {
    vector3 *tmp = vector3_scale(2 * nl, n);
    vector3 *refl = vector3_sub(tmp, l);
    free(tmp);
    vector3 *v = vector3_negate(r->direction);
    double m = fmax(0, vector3_dot(refl, v));
    free(refl);
    free(v);
    d = color_scale(pow(m, 6), h->shine);
  }
  color *result = color_add(surf_color, d);
  free(surf_color);
  free(d);
  return result;
}


hit *intersect_sphere(ray3 * r, sphere * s)
{
  if (r == NULL || s == NULL) {
    fprintf(stderr, "null pointer\n");
    exit(1);
  }
  vector3 *a = vector3_sub(r->origin, s->center);
  double b = vector3_dot(a, r->direction);
  double c = vector3_dot(a, a) - s->radius * s->radius;
  free(a);
  double d = b * b - c;
  double t = - b - sqrt(d);
  if (d > 0 && t > 0) {
    vector3 *hitpoint = ray3_position(r, t);
    vector3 *norm = vector3_sub(hitpoint, s->center);
    vector3_normify(norm);
    hit *h = NULL;
    switch (s->surf.tag) {
    case CONSTANT:
    {
      h = hit_new_deep(t, s->surf.c.k, s->shine, norm);
      free(norm);
      free(hitpoint);
      return h;
    }
    case FUNCTION:
    {
      h = hit_new_shallow(t, (s->surf.c.f)(s->center, hitpoint),
                          s->shine, norm);

      free(norm);
      free(hitpoint);
      return h;
    }
    default:
      fprintf(stderr, "bad tag\n");
      exit(1);
    }
  } else if (d > 0 && t <= 0) {
    return NULL;
  } else  {
    return NULL;
  }
}

hit *intersect_rect(ray3 * r, rectangle * rect)
{
  if (r == NULL || rect == NULL) {
    fprintf(stderr, "null pointer\n");
    exit(1);
  }
  vector3 *n = vector3_new(0, 0, -1);
  double d = rect->upper_left->z;
  double t = -(vector3_dot(r->origin, n) + d) / vector3_dot(r->direction, n);
  vector3 *hitpoint = ray3_position(r, t);
  if (t > 0 && hitpoint->x >= rect->upper_left->x &&
      hitpoint->x <= rect->upper_left->x + rect->w &&
      hitpoint->y >= rect->upper_left->y - rect->h &&
      hitpoint->y <= rect->upper_left->y) {
    hit *h = NULL;
    switch (rect->surf.tag) {
    case CONSTANT:
    {
      h = hit_new_deep(t, rect->surf.c.k, rect->shine, n);
      free(hitpoint);
      free(n);
      return h;
    }
    case FUNCTION:
    {
      h = hit_new_shallow(t, (rect->surf.c.f)(rect->upper_left, hitpoint),
                          rect->shine, n);

      free(hitpoint);
      free(n);
      return h;
    }
    default :
      fprintf(stderr, "bad tag\n");
      exit(1);
    }
  } else {
    free(hitpoint);
    free(n);
    return NULL;
  }
}

hit *intersect(ray3 * r, object * obj)
{
  if (r == NULL || obj == NULL) {
    fprintf(stderr, "null pointer\n");
    exit(1);
  }
  switch (obj->tag) {
  case SPHERE:
    return intersect_sphere(r, obj->o.s);
  case RECTANGLE:
    return intersect_rect(r, obj->o.r);
  default:
    fprintf(stderr, "bad tag in obj\n");
    exit(1);
  }
}

color *trace_ray(ray3 * r, scene * s)
{
  if (r == NULL || s == NULL) {
    fprintf(stderr, "null pointer\n");
    exit(1);
  }
  hit *closest = NULL;
  object_list *ol = s->objects;
  while (ol != NULL) {
    hit *h = intersect(r, &(ol->first));
    if (h != NULL) {
      if (closest == NULL) {
        closest = h;
      } else if (closest->t > h->t) {
        hit_free(closest);
        closest = h;
      } else {
        hit_free(h);
      }
    }
    ol = ol->rest;
  }
  //background
  if (closest == NULL) {
    return light_color(s, r, NULL);
  } else {
    color *c = light_color(s, r, closest);
    hit_free(closest);
    return c;
  }
}
