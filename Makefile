.PHONY : clean

raytracer : raytracer-project2.h vector3.c main.c utils.[hc] color.c ray3.c logic.c
	clang -g -Wall -lm -o raytracer utils.c vector3.c color.c ray3.c logic.c main.c

clean :
	rm -rf raytracer raytracer.dSYM

