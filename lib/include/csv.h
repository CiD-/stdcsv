#ifndef CSV_H
#define CSV_H

#ifdef __cplusplus
extern "C" {
#endif

#define CSV_GOOD                0
#define CSV_FAIL                -5
#define CSV_RESET               -100
#define CSV_NORMAL_OPEN         -2
#define CSV_BUFFER_FACTOR       128
#define CSV_MAX_NEWLINES        40

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

enum quote_style {
	QUOTE_NONE = 0,
	QUOTE_WEAK,
	QUOTE_RFC4180,
	QUOTE_ALL,
};

/**
 * CSV Structures
 */

/* Forward declaration of internal structures */
struct csv_record_internal;
struct csv_read_internal;
struct csv_write_internal;

struct csv_field {
	const char* data;
	unsigned len;
};

/* Structure containing dynamic array of fields */
struct csv_record {
	struct csv_record_internal* _in;
	struct csv_field* fields;
	const char* rec;
	unsigned reclen;
	int size;
};

/**
 * These structs are meant to function more like
 * classes. All the available members are used to
 * change their behavior.
 */
struct csv_reader {
	struct csv_read_internal* _in;
	enum quote_style quotes;
	int normal;
	_Bool failsafe_mode;
	_Bool trim;
};

struct csv_writer {
	struct csv_write_internal* _in;
	enum quote_style quotes;
};

/**
 * CSV Global
 */
void csv_perror();
void csv_perror_exit();

/**
 */
struct csv_record* csv_record_new();
struct csv_record* csv_record_construct(struct csv_record*);

/**
 */
void csv_record_free(struct csv_record* rec);
void csv_record_destroy(struct csv_record* self);
/**
 * free, but field pointers are still valid
 * user is now responsible for freeing the
 * array of struct csv_field AS WELL AS
 * each individual data field:
 *
 * int n = rec->size;
 * struct csv_field* fields = csv_release_data(rec);
 *
 * .... DO STUFF ....
 *
 * for (int i = 0; i < n; ++i) {
 *         free(fields[i].data);
 * }
 * free(fields);
 */
struct csv_field* csv_record_release_data(struct csv_record* rec);

/**
 * Clone the record and its data
 */
struct csv_record* csv_record_clone(const struct csv_record*);

/**
 * CSV Reader
 */

/**
 * Allocate all necessary memory here. Set up
 * signal handling here.
 */
struct csv_reader* csv_reader_new();
struct csv_reader* csv_reader_construct(struct csv_reader*);

/**
 * Relese allocated heap resources
 */
void csv_reader_free(struct csv_reader*);

/** Accessors **/
unsigned csv_reader_row_count(struct csv_reader*);
unsigned csv_reader_embedded_breaks(struct csv_reader*);

/** Mutators **/
void csv_reader_set_delim(struct csv_reader*, const char*);
void csv_reader_set_embedded_break(struct csv_reader*, const char*);

/**
 * Main accessing function for reading data.
 */
int csv_get_record(struct csv_reader*, struct csv_record*);
int csv_nget_record(struct csv_reader*, struct csv_record*, unsigned char_limit);
int csv_get_record_to(struct csv_reader*, struct csv_record*, unsigned field_limit);
int csv_nget_record_to(struct csv_reader*, struct csv_record*, unsigned char_limit, unsigned field_limit);

/**
 * Reset statistics. If their is an associated file
 * to the reader, seek to the beginning of it.
 */
int csv_reader_reset(struct csv_reader*);

/**
 * Open a csv file for reading.  This file will close itself
 * when reading reaches the end of the file.
 */
int csv_reader_open(struct csv_reader*, const char* fileName);

/**
 * This function is available but is called internally
 * when the end of the file has been reached.
 */
int csv_reader_close(struct csv_reader*);

/**
 * Parse an individual const char*. This function is used
 * internally by csv_get_record, and is only meant to handle
 * a single record. This will not treat new lines as a
 * record separator.
 */
int csv_parse(struct csv_reader*, struct csv_record*, const char*);
int csv_nparse(struct csv_reader*, struct csv_record*, const char*, unsigned char_limit);
int csv_parse_to(struct csv_reader*, struct csv_record*, const char*, unsigned field_limit);
int csv_nparse_to(struct csv_reader*, struct csv_record*, const char*, unsigned char_limit, unsigned field_limit);




/**
 * CSV Writer
 */

/**
 * Allocate resources for a writer
 */
struct csv_writer* csv_writer_new();
struct csv_writer* csv_writer_construct(struct csv_writer*);

/**
 * Relese allocated heap resources
 */
void csv_writer_free(struct csv_writer*);
void csv_writer_destroy(struct csv_writer*);

/**
 * return TRUE if writer has open file
 */
int csv_writer_isopen(struct csv_writer*);

/**
 * Open a file for writing csv conents
 */
int csv_writer_open(struct csv_writer*, const char* fileName);

/**
 * Open a temp FILE* that will be used for
 * output in case we do a reset.
 */
int csv_writer_mktmp(struct csv_writer*);

/**
 * If we are writing to a file, close and re-open
 * the file for writing.
 */
int csv_writer_reset(struct csv_writer*);

/**
 * csv_writer is always writing to a temp file
 * if we are not writing to stdout. Here we
 * close and rename the temp file to the desired
 * file name. If there is no output file, dump
 * the temp file to stdout
 */
int csv_writer_close(struct csv_writer*);

/**
 * Write a single record into the FILE* stored
 * in the writer struct
 */
void csv_write_record(struct csv_writer*, struct csv_record*);

/**
 * Write a single field from a record.  This function
 * will not print write delimiters or line endings.
 */
void csv_write_field(struct csv_writer*, const struct csv_field* field);

#ifdef __cplusplus
}
#endif

#endif /* CSV_H */
