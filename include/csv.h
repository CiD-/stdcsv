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
#define CSV_RESET               -1
#define CSV_BUFFER_FACTOR       128
#define CSV_MAX_FIELD_SIZE      10000
#define CSV_MAX_RECORD_SIZE     50000

#define CSVR_STD_QUALIFIERS     csvr_qualifiers == 2
#define CSVR_NORMAL_OPEN        -2

#define CSVW_STD_QUALIFIERS     csvw_qualifiers == 2

#define STD_ALL         3
#define STD_RFC4180     2
#define STD_WEAK        1
#define STD_NONE        0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include "util.h"

/* Structure containing an individual field */
struct csv_field {
        const char* begin;
        size_t length;
};

/* Forward declaration of internal structure */
struct csv_internal;

/* Structure containing dynamic array of fields */
struct csv_record {
        struct csv_field* fields;
        int size;
        struct csv_internal* _internal;
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
 * Accessors
 */
char* csvr_get_delim(struct csv_record*);
int csvr_get_allowstdchange(struct csv_record*);

/**
 * Mutators
 */
void csvr_set_delimiter(struct csv_record*, const char* delim);
void csvr_set_standard(struct csv_record*, int standard);
void csvr_set_normal(struct csv_record*, int normal);
void csvr_set_internalbreak(struct csv_record*, const char* internalBreak);
void csvw_set_inplaceedit(struct csv_record*, int i);
void csvw_set_standard(struct csv_record*, int i);
void csvw_set_lineending(struct csv_record*, const char* lineEnding);
void csvw_set_filename(struct csv_record*, const char* filename);

/**
 * Methods
 */

/**
 * csvr_nextfield takes a pointer to the beginning of a field.
 * *begin is searched for the next delimiter.
 *
 * Returns:
 *      - struct csv_field representing parsed field
 *      - NULL on failure
 */
struct csv_field csvr_nextfield(char** begin);

/**
 * csvr_nextquoted behaves the same as csvr_nextfield
 * except that it expects the next field to be quoted.
 * *begin is searched for a terminating quote and
 * a delimiter.
 *
 * Returns:
 *      - struct csv_field representing parsed field
 *      - NULL on failure
 */
struct csv_field csvr_nextquoted(char** begin);

/**
 * csv_getfields takes an array of pointers to csv_field's.
 * The variable fieldCount is set within this function to the
 * number of fields in the current csv line.  If this function
 * fails, fieldCount will be set to -1.
 *
 * Returns:
 *      - an array of pointers to struct csv_field which
 *        represents the parsed fields from the next line.
 *      - NULL if EOF
 */
int csvr_get_record(struct csv_record *rec);

/**
 *
 */
int csvr_open(struct csv_record*, const char* fileName);

/**
 * Buffers for reading and appending are allocated here.
 */
void csvr_init(struct csv_record*);

/**
 * Free all heap memory.
 */
void csvr_destroy(struct csv_record* record);

/**
 * csvr_growrecord allocates the initial memory for a
 * struct csv_field. It also reallocs an additional fields
 * if fields is already allocated a pointer.
 */
void csvr_growrecord(struct csv_record* record);






/** csv_writer **/

/**
 *
 */
void csvw_open();


/**
 * csvw_init opens a temp file for writing if input is from a file.
 * The location of the temp file is determined by available space on
 * the current partition. If that amount of space is less than
 * MIN_SPACE_AVAILABLE, then we use the TMPDIR (defined during
 * compilation). If TMPDIR is not defined during compilation, use /tmp.
 */
void csvw_init();

/**
 * Relese allocated heap resources
 */
//void csvw_destroy();

/**
 * Dump temp file to destination file/stdout
 */
void csvw_close();

/**
 * wrapper for csvw_writeline with extra field for delimiter. if csvw_delim
 * is not defined, we set it equal to the provided delim.
 */
void csvw_writeline_d(struct csv_record* rec, char* delim);

/**
 * Loop through array of struct csv_field, and print the line to csvw_file.
 */
void csvw_writeline(struct csv_record* rec);

#endif
