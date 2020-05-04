#ifndef UTIL_H
#define UTIL_H

#include <libgen.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "charnode.h"
#include "csvsignal.h"
#include "csverror.h"

#define TRUE  1
#define FALSE 0
#define BUFFER_FACTOR 128


/**
 * malloc wrapper that does error checking
 */
#define MALLOC(dest, size) {            \
        dest = malloc(size);            \
        if (!dest) {                    \
                perror("malloc");       \
                cleanexit();            \
        }                               \
}

/**
 * realloc wrapper that does error checking
 */
#define REALLOC(dest, size) {                   \
        void* new_dest_ = realloc(dest, size);  \
        if (!dest) {                            \
                perror("realloc");              \
                cleanexit();                    \
        }                                       \
        dest = new_dest_;                       \
        new_dest_ = NULL;                       \
}

/**
 * Wrapper macro for non-posix function strdup
 * strdup allocates memory for us. If dest is already
 * allocated memory, free it first.
 */
#define STRDUP(src, dest)  {                                            \
        if (dest)                                                       \
                free(dest);                                             \
        dest = strdup(src);                                             \
        if (!dest) {                                                    \
                fprintf(stderr, "strdup failed on string %s.\n", src);  \
                cleanexit();                                            \
        }                                                               \
}

/**
 * strncpy but guaranteed to end with '\0'
 */
#define STRNCPY(dest, src, n) {         \
        strncpy(dest, src, n-1);        \
        dest[n-1] = '\0';               \
}


/**
 * Free pointer if not NULL and set to NULL
 */
#define FREE(ptr) {                 \
        if (ptr) {                  \
                free((void*)ptr);   \
                ptr = NULL;         \
        }                           \
}




/** **/
void increase_buffer(char**, size_t*);
void increase_buffer_to(char**, size_t*, size_t);



/**
 * This function is a wrapper for stringtolong below.
 * with an assumed base of 10.
 *
 * Returns:
 *      - parsed base 10 long int
 */
long stringtolong10(const char* s);

/**
 * This function is a wrapper for the standard strtol
 * function that also handles all errors internally.
 *
 * Returns:
 *      - parsed long int
 */
long stringtolong(const char* s, int base);

/**
 * charcount simply counts the occurences of char c
 * in the string s.
 *
 * Returns:
 *      - Number of occurences.
 */
int charcount(const char* s, char c);

/**
 * strhaschar checks whether a char c exists within string s.
 *
 * Returns:
 *      - 1 if yes
 *      - 0 if no
 */
int strhaschar(const char* s, char c);

/**
 * removecharat shifts all characters left 1 at the
 * provided index i in order to essentially remove that
 * character from the string.
 */
void removecharat(char* s, int i);

/**
 * randstr generates a random string of length n
 * out of all alpha-numeric characters.
 *
 * Returns:
 *      - char* of random characters
 */
char* randstr(char* s, const int n);


/**
 * getnoext returns a filename with no extension
 * The returned char* will be allocated on heap
 * and must be free'd!
 */
char* getnoext(const char* filename);

/**
 * getext returns the extension from a provided filename
 * The returned char* will be allocated on the heap
 * and must be free'd!
 */
char* getext(char* filename);


#endif
