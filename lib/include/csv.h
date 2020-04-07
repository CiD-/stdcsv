#ifndef CSV_H
#define CSV_H

#ifndef TMPDIR
#define TMPDIR  /tmp/
#endif

#define STRING_VALUE(arg) #arg
#define TMPDIR_STR STRING_VALUE(TMPDIR)

#define CSV_GOOD                0
#define CSV_RESET               -100
#define CSV_NORMAL_OPEN         -2
#define CSV_BUFFER_FACTOR       128
#define CSV_MAX_NEWLINES        40

#define QUOTE_ALL         3
#define QUOTE_RFC4180     2
#define QUOTE_WEAK        1
#define QUOTE_NONE        0

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>

#include "util.h"
#include "safegetline.h"

/**
 * CSV Structures
 */

/* Forward declaration of internal structures */
struct csv_read_internal;
struct csv_write_internal;

/* Structure containing dynamic array of fields */
struct csv_record {
        char** fields;
        int size;
};

struct csv_reader {
        struct csv_read_internal* _in;
        char delimiter[32];
        char inlineBreak[32];
        int quotes;
        int normal;
        int failsafeMode;
};

struct csv_writer {
        struct csv_write_internal* _in;
        char delimiter[32];
        char lineEnding[3];
        int quotes;
};

extern const struct csv_record blank_record;

/**
 * CSV Reader
 */

/**
 * Allocate all necessary memory here. Set up
 * signal handling here.
 */
struct csv_reader* csv_reader_new();

/**
 * Relese allocated heap resources
 */
void csv_reader_free(struct csv_reader*);

/** Accessors **/
uint csv_reader_row_count(struct csv_reader*);
uint csv_reader_inline_breaks(struct csv_reader*);

/**
 * Main accessing function for reading data.
 */
int csv_get_record(struct csv_reader*, struct csv_record**);

/**
 * Reset statistics. If their is an associated file
 * to the reader, seek to the beginning of it.
 */
void csv_reader_reset(struct csv_reader*);

/**
 * Open a csv file for reading.  This file will close itself
 * when reading reaches the end of the file.
 */
void csv_reader_open(struct csv_reader*, const char* fileName);

/**
 * This function is available but is called internally
 * when the end of the file has been reached.
 */
void csv_reader_close(struct csv_reader*);

/**
 * Parse an individual const char*. This function is used
 * internally by csv_get_record, and is only meant to handle
 * a single record. This will not treat new lines as a
 * record separator.
 */
struct csv_record* csv_parse(struct csv_reader*, const char* line);




/**
 * CSV Writer
 */

/**
 * Allocate resources for a writer
 */
struct csv_writer* csv_writer_new();

/**
 * Relese allocated heap resources
 */
void csv_writer_free(struct csv_writer*);

/**
 * Open a file for writing csv conents
 */
void csv_writer_open(struct csv_writer*, const char* fileName);

/**
 *
 */
void csv_writer_reset(struct csv_writer*);

/**
 *
 */
void csv_writer_close(struct csv_writer*);

/**
 * Loop through array of struct csv_field, and print the line to csvw_file.
 */
void csv_write_record(struct csv_writer*, struct csv_record*);


#endif
