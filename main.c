#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "utils.h"
#include "raytracer-project2.h"

/* some convenience constructors for objects, etc. */

surface surf_const(double r, double g, double b)
{
  surface s;
  s.tag = CONSTANT;
  s.c.k = color_new(r, g, b);
  return s;
}

surface surf_fn(color * (*f)(vector3*, vector3*))
{
  surface s;
  s.tag = FUNCTION;
  s.c.f = f;
  return s;
}

/* create a container object for a sphere */
object *obj_sph(sphere *s)
{
  if (!s) {
    fprintf(stderr, "obj_sph given NULL\n");
    exit(1);
  }
  object *o = (object*)malloc(sizeof(object));
  check_malloc("obj_sph", o);
  o->tag = SPHERE;
  o->o.s = s;
  return o;
}

/* create a container object for a rectangle */
object *obj_rect(rectangle *r)
{
  if (!r) {
    fprintf(stderr, "obj_rect given NULL\n");
    exit(1);
  }
  object *o = (object*)malloc(sizeof(object));
  check_malloc("obj_rect", o);
  o->tag = RECTANGLE;
  o->o.r = r;
  return o;
}

/* private internal sphere constructor that leaves color slot uninitialized */
sphere *sph(double cx, double cy, double cz, double r, double sr, double sg, double \
            sb)
{
  sphere *s = (sphere*)malloc(sizeof(sphere));
  check_malloc("sph", s);
  s->center = vector3_new(cx, cy, cz);
  if (r < 0) {
    fprintf(stderr, "sph: r<0 (r=%lf)\n", r);
    exit(1);
  }
  s->radius = r;
  s->shine = color_new(sr, sg, sb);
  return s;
}

/* solid-color sphere constructor */
object *sphere_new(double cx, double cy, double cz,
                   double r,
                   double cr, double cg, double cb,
                   double sr, double sg, double sb)
{
  sphere *s = sph(cx, cy, cz, r, sr, sg, sb);
  s->surf   = surf_const(cr, cg, cb);
  return obj_sph(s);
}

/* private internal rectangle constructor that leaves color slot uninitialized */
rectangle *rect(double ulx, double uly, double ulz,
                double w, double h,
                double sr, double sg, double sb)
{
  rectangle *r = (rectangle*)malloc(sizeof(rectangle));
  check_malloc("rect", r);
  r->upper_left = vector3_new(ulx, uly, ulz);
  if (w < 0) {
    fprintf(stderr, "rectangle_new: negative width (%lf)\n", w);
    exit(1);
  }
  r->w = w;
  if (h < 0) {
    fprintf(stderr, "rectangle_new: negative height (%lf)\n", h);
    exit(1);
  }
  r->h = h;
  r->shine = color_new(sr, sg, sb);
  return r;
}

/* solid-color rectangle constructor */
object *rectangle_new(double ulx, double uly, double ulz,
                      double w, double h,
                      double cr, double cg, double cb,
                      double sr, double sg, double sb)
{
  rectangle *r = rect(ulx, uly, ulz, w, h, sr, sg, sb);
  r->surf = surf_const(cr, cg, cb);
  return obj_rect(r);
}

/* shallow-copy object list cons */
object_list *cons(object *o, object_list *os)
{
  object_list *l = (object_list*)malloc(sizeof(object_list));
  check_malloc("cons", l);
  l->first = *o;
  l->rest  = os;
  return l;
}

/* (mostly) shallow-copy scene constructor */
scene *scene_new(color *bg, color *amb, light *dl, object_list *objs)
{
  if (!bg || !amb || !dl) {
    fprintf(stderr, "scene_new: unexpected NULL\n");
    exit(1);
  }
  scene *sc = (scene*)malloc(sizeof(scene));
  check_malloc("scene_new", sc);
  sc->bg.tag = CONSTANT;
  sc->bg.c.k = bg;
  sc->amb_light = amb;
  sc->dir_light = dl;
  sc->objects = objs;
  return sc;
}

/* dl_new: new directional light */
/* note: direction vector need not be a unit vector, it is normalized here */
light *dl_new(double x, double y, double z, double r, double g, double b)
{
  light *dl = (light*)malloc(sizeof(light));
  check_malloc("dl_new", dl);
  dl->direction = vector3_new(x, y, z);
  vector3_normify(dl->direction);
  dl->color = color_new(r, g, b);
  return dl;
}

/* shallow copy environment constructor */
environment *environment_new(double z, uint w, uint h, scene *sc)
{
  environment *e = (environment*)malloc(sizeof(environment));
  check_malloc("environment_new", e);
  e->camera_z = z;
  e->image_width = w;
  e->image_height = h;
  e->scene = sc;
  return e;
}

/* *** destructors *** */

void surf_free(surface *surf)
{
  switch (surf->tag)
  {
  case CONSTANT:
    free(surf->c.k);
    break;
  case FUNCTION:
    break;
  }
}

void sphere_free(sphere *s)
{
  free(s->center);
  surf_free(&s->surf);
  free(s->shine);
  free(s);
}

void rect_free(rectangle *r)
{
  free(r->upper_left);
  surf_free(&r->surf);
  free(r->shine);
  free(r);
}

void object_free(object *o) {
  switch (o->tag) {
  case SPHERE:
    sphere_free(o->o.s);
    break;
  case RECTANGLE:
    rect_free(o->o.r);
    break;
  }
}

void ol_free(object_list *ol)
{
  if (ol == NULL) return;
  else {
    ol_free(ol->rest);
    object_free(&ol->first);
    free(ol);
  }
}

void light_free(light *l)
{
  free(l->direction);
  free(l->color);
  free(l);
}

void scene_free(scene *sc)
{
  surf_free(&sc->bg);
  free(sc->amb_light);
  light_free(sc->dir_light);
  ol_free(sc->objects);
  free(sc);
}

void env_free(environment *e)
{
  scene_free(e->scene);
  free(e);
}

/* *** rendering functions *** */

void render_ppm(FILE *f, environment *e) {
  fprintf(f, "P3\n");
  fprintf(f, "%d %d\n", e->image_width, e->image_height);
  fprintf(f, "255\n");
  uint w = e->image_width;
  uint h = e->image_height;
  for (uint i = 1; i <= h; i++) {
    for (uint j = 1; j <= w; j++) {
      vector3 *cam = vector3_new(0, 0, e->camera_z);
      vector3 *coord = logical_coord(h, w, i, j);
      vector3 *diff = vector3_sub(coord, cam);
      vector3_normify(diff);
      ray3 *r = ray3_new(cam, diff);
      color *c = trace_ray(r, e->scene);
      fprintf(f, "%d %d %d\n",
              (int)(c->r * 255), (int)(c->g * 255), (int)(c->b * 255));
      free(c);
      free(cam);
      free(coord);
      free(diff);
      free(r);
    }
  }
}

int is_pre(char* test, char* str) {
  int len = strlen(test);
  for (int i = 0; i < len; i++) {
    if (test[i] != str[i]) {
      return 0;
    }
  }
  return 1;
}

environment *read_env()
{
  char buf[512];
  double a[11];
  color *dummy = color_new(0, 0, 0);
  light *dummylight = dl_new(0, 0, 0, 0, 0, 0);
  scene *sc = scene_new(dummy, dummy, dummylight, NULL);
  environment *env = environment_new(0, 0, 0, sc);
  while (fgets(buf, 512, stdin) != NULL) {
    if (is_pre("ENV", buf)) {
      sscanf(buf, "ENV %lf %lf %lf", &a[0], &a[1], &a[2]);
      env->camera_z = a[0];
      env->image_width = (unsigned int)a[1];
      env->image_height = (unsigned int)a[2];
    } else if (is_pre("BG", buf)) {
      sscanf(buf, "BG %lf %lf %lf", &a[0], &a[1], &a[2]);
      sc->bg.tag = CONSTANT;
      sc->bg.c.k = color_new(a[0], a[1], a[2]);
    } else if (is_pre("AMB", buf)) {
      sscanf(buf, "AMB %lf %lf %lf", &a[0], &a[1], &a[2]);
      sc->amb_light = color_new(a[0], a[1], a[2]);
    } else if (is_pre("DL", buf)) {
      sscanf(buf, "DL %lf %lf %lf %lf %lf %lf", &a[0], &a[1],
             &a[2], &a[3], &a[4], &a[5]);
      sc->dir_light = dl_new(a[0], a[1], a[2], a[3], a[4], a[5]);
    } else if (is_pre("SPHERE", buf)) {
      sscanf(buf, "SPHERE %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
             &a[0], &a[1], &a[2], &a[3], &a[4],
             &a[5], &a[6], &a[7], &a[8], &a[9]);
      object *sp = sphere_new(a[0], a[1], a[2], a[3], a[4],
                              a[5], a[6], a[7], a[8], a[9]);
      sc->objects = cons(sp, sc->objects);
    } else if (is_pre("RECTANGLE", buf)) {
      sscanf(buf, "RECTANGLE %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
             &a[0], &a[1], &a[2], &a[3], &a[4],
             &a[5], &a[6], &a[7], &a[8], &a[9], &a[10]);
      object *rect = rectangle_new(a[0], a[1], a[2], a[3], a[4],
                                   a[5], a[6], a[7], a[8], a[9], a[10]);
      sc->objects = cons(rect, sc->objects);
    } else {
      fprintf(stderr, "skipping \"%s\"\n", buf);
    }
  }
  free(dummy);
  light_free(dummylight);
  return env;
}

/* *** functional colors *** */

color *sphere_color_fn1(vector3 *c, vector3 *hp)
{
  double r = sin((hp->x + hp->y + hp->z) * 16);
  double d = r / 2.0 + 0.5;
  return color_new(d / 2.0, d / 1.5, d);
}

color *sphere_color_fn2(vector3 *c, vector3 *hp)
{
  double r = cos((hp->x + hp->y * hp->z) * 2);
  double d = r / 2.0 + 0.5;
  return color_new(1.0, d / 1.5, d / 1.1);
}

color *sunset(vector3 *ro, vector3 *vp)
{
  double grad = (1.0 - -vp->y) / 2.0;
  return color_new((1.0 - grad) / 1.5, 0.0, grad / 2.0);
}

object *sphere_new_fn(double cx, double cy, double cz,
                      double r,
                      color * (*f)(vector3*, vector3*),
                      double sr, double sg, double sb)
{
  sphere *s = sph(cx, cy, cz, r, sr, sg, sb);
  s->surf   = surf_fn(f);
  return obj_sph(s);
}

scene *scene_new_fn(color * (*f)(vector3*, vector3*),
                    color *amb, light *dl, object_list *objs)
{
  if (!f || !amb || !dl) {
    fprintf(stderr, "scene_new: unexpected NULL\n");
    exit(1);
  }
  scene *sc = (scene*)malloc(sizeof(scene));
  check_malloc("scene_new_fn", sc);
  sc->bg = surf_fn(f);
  sc->amb_light = amb;
  sc->dir_light = dl;
  sc->objects = objs;
  return sc;
}

/* *** main program *** */

int main(int argc, char *argv[])
{
  if (argc == 2 && !strcmp(argv[1], "1")) {
    /* n.b. WHITE sphere (so you can tell this apart from other similar scenes) */
    // object *sphere0    = sphere_new(1, 0, 3, 0.6, 1, 1, 1, 0, 0, 0);
    // object *rectangle0 = rectangle_new(1, 1.3, 4, 1, 2.5, 0, 0, 1, 0, 0, 0);
    // object_list *objs0 = cons(sphere0, cons(rectangle0, NULL));
    // scene *scene0      = scene_new(color_new(0.8, 0.8, 0.8),
    //                                color_new(0.2, 0.2, 0.2),
    //                                dl_new(-1, 1, -1, 1, 1, 1),
    //                                objs0);
    // environment *env0  = environment_new(-3.3, 600, 400, scene0);
    // render_ppm(stdout, env0);
    // free(sphere0);
    // free(rectangle0);
    // env_free(env0);

    // /****functional colored env****/
     object *sphere1 = sphere_new_fn(-0.6, 0.2, 13.0, 1.1,
                                    sphere_color_fn1, 0.8, 0.8, 0.8);
     object *sphere2 = sphere_new_fn(1.4, -0.15, 16.0, 1.1,
                                  sphere_color_fn2, 0.8, 0.8, 0.8);
     object_list *objs1 =  cons(sphere2, cons(sphere1,  NULL));
     scene *scene1      = scene_new_fn(sunset,
                                        color_new(0.2, 0.2, 0.2),
                                        dl_new(-1, 1, -1, 1, 1, 1),
                                        objs1);
     environment *env1  = environment_new(-3.3, 800, 240, scene1);
     render_ppm(stdout, env1);
     free(sphere1);
     free(sphere2);
     env_free(env1);
  } else if (argc == 1) {
    environment *e = read_env();
    render_ppm(stdout, e);
    env_free(e);
  }
  return 0;
}
