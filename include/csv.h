#ifndef CSV_RW
#define CSV_RW

#ifndef TMPDIR
#define TMPDIR  /tmp/
#endif

#define STRING_VALUE(arg) #arg
#define TMPDIR_STR STRING_VALUE(TMPDIR)

#define MIN_SPACE_AVAILABLE     5000000000

#define CSV_DONE                -4
#define CSV_DUMP                -3
#define CSV_SKIP_LINE           -2
#define CSV_ERROR_QUOTES        -1
#define CSV_BUFFER_FACTOR       128
#define CSV_MAX_FIELD_SIZE      10000
#define CSV_MAX_RECORD_SIZE     50000

#define CSV_NORMAL_OPEN         -2

#define QUOTE_ALL         3
#define QUOTE_RFC4180     2
#define QUOTE_WEAK        1
#define QUOTE_NONE        0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include "util.h"

/** Incomplete definition in main **/
/*
#define csv_destroy(csv_struct)                 \
{                                               \
        FREE(csv_struct->_internal->buffer);    \
        FREE(csv_struct->_internal);            \
        FREE(csv_struct);                       \
}
*/

/* Structure containing an individual field */
struct csv_field {
        const char* begin;
        size_t length;
};

/* Forward declaration of internal structures */
struct csv_read_internal;
struct csv_write_internal;

/* Structure containing dynamic array of fields */
struct csv_record {
        struct csv_field* fields;
        int size;
};

struct csv_reader {
        struct csv_read_internal* _internal;
        char delimiter[32];
        char inlineBreak[32];
        int quotes;
        int normal;
        int failsafeMode;
};

struct csv_writer {
        struct csv_write_internal* _internal;
        //char filename[PATH_MAX];
        char delimiter[32];
        char lineEnding[3];
        int quotes;
};

extern const struct csv_record blank_record;

/** csv_field **/

/**
 * Return char* to end of field
 */
const char* csv_get_end(struct csv_field* s);

/**
 * Provide dynamically allocated char*
 * copy of the field
 */
char* csv_newstring(struct csv_field* s);

/**
 * Assigns buffer to a copy of the data within
 * the field. This char* should already be allocated.
 * If strncpy fails, return -1
 */
int csv_get_string(struct csv_field* s, char* buffer);

/**
 * Initializes and returns a dynamic array of
 * pointers to struct csv_field
 */
//struct csv_field** csv_init();


/** csv_reader **/

/**
 * Methods
 */


/**
 * csv_get_record takes an array of pointers to csv_field's.
 * The variable fieldCount is set within this function to the
 * number of fields in the current csv line.  If this function
 * fails, fieldCount will be set to -1.
 *
 * Returns:
 *      - an array of pointers to struct csv_field which
 *        represents the parsed fields from the next line.
 *      - NULL if EOF
 */
struct csv_record* csv_get_record(struct csv_reader*);

/**
 *
 */
void csv_reader_open(struct csv_reader*, const char* fileName);

/**
 * Buffers for reading and appending are allocated here.
 */
struct csv_reader* csv_new_reader();

/**
 * Free all heap memory.
 */
//void csv_destroy(struct csv_reader*);

/**
 *
 */
struct csv_record* csv_parse(struct csv_reader*, char* line);


/** csv_writer **/

/**
 *
 */
void csv_writer_open(struct csv_writer*, const char* fileName);


/**
 * csvw_init opens a temp file for writing if input is from a file.
 * The location of the temp file is determined by available space on
 * the current partition. If that amount of space is less than
 * MIN_SPACE_AVAILABLE, then we use the TMPDIR (defined during
 * compilation). If TMPDIR is not defined during compilation, use /tmp.
 */
struct csv_writer* csv_new_writer();

/**
 * Relese allocated heap resources
 */
void csv_destroy_reader(struct csv_reader*);
void csv_destroy_writer(struct csv_writer*);

/**
 * Dump temp file to destination file/stdout
 */
void csv_writer_close(struct csv_writer*);

/**
 * Loop through array of struct csv_field, and print the line to csvw_file.
 */
void csv_write_record(struct csv_writer*, struct csv_record*);

/**
 *
 */
void csv_writer_reset(struct csv_writer*);


#endif
