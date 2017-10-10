#ifndef _UTILS_H_
#define _UTILS_H_

/* use this file to specify general-purpose utilities */

/* todo: print a message to stderr and exit(1) */
void todo(char *function_name);

/* check_malloc: if pointer is NULL, print a message to stderr and exit(1) */
void check_malloc(char *function_name, void *h);

#endif /* _UTILS_H_ */
