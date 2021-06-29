#ifndef SAFEGETLINE_H
#define SAFEGETLINE_H

#include <stdio.h>

/**
 * safegetline gets the next line from the provided
 * FILE* and sets up to a max of buflen characters to buffer.
 * This is a safer alternative to gnu getline in that it will
 * safely pull line endings from other operating systems.
 *
 * Returns:
 *      - The number of characters in the buffer
 *      - -2 if buflen is reached before new line is found.
 *      - EOF if Failed read
 */
int sappline(FILE*, char** buf, size_t* buflen, size_t* linelen);
int sgetline(FILE*, char** buf, size_t* buflen, size_t* linelen);

int sappline_mmap(
        const char* mmap, char** line, size_t* bufidx, size_t* len, size_t limit);
int sgetline_mmap(
        const char* mmap, char** line, size_t* bufidx, size_t* len, size_t limit);

#endif
