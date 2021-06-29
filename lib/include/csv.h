#ifndef CSV_H
#define CSV_H

#ifdef __cplusplus
extern "C" {
#endif

#define CSV_GOOD          0
#define CSV_FAIL          -5
#define CSV_RESET         -100
#define CSV_NORMAL_OPEN   -2
#define CSV_BUFFER_FACTOR 128
#define CSV_MAX_NEWLINES  40

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
	char* rec;
	size_t reclen;
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

void csv_reader_free(struct csv_reader*);
void csv_reader_destroy(struct csv_reader* self);

/** Accessors **/
unsigned csv_reader_row_count(struct csv_reader*);
unsigned csv_reader_embedded_breaks(struct csv_reader*);
const char* csv_reader_get_delim(struct csv_reader*);
size_t csv_reader_get_file_size(struct csv_reader*);

/** Mutators **/
void csv_reader_set_delim(struct csv_reader*, const char*);
void csv_reader_set_embedded_break(struct csv_reader*, const char*);

/**
 * Main accessing function for reading data.
 */
int csv_get_record(struct csv_reader*, struct csv_record*);
int csv_get_record_to(struct csv_reader*, struct csv_record*, unsigned field_limit);

/**
 * Reset statistics. If their is an associated file
 * to the reader, seek to the beginning of it.
 */
int csv_reader_reset(struct csv_reader*);

/**
 * Open a csv file for reading.
 */
int csv_reader_open(struct csv_reader*, const char* file_name);

/**
 * Open a csv file for reading and mmap it.
 * By default, the map is advised `MADV_SEQUENTAIL'
 */
int csv_reader_open_mmap(struct csv_reader*, const char* file_name);

/**
 *  For mmap only: pass through to madvise on whole file
 *  returns CSV_FAIL if out of range
 */
int csv_reader_madvise(struct csv_reader*, const int advice);

/**
 * Seek to a certain offset in the file
 * returns CSV_FAIL if out of range or seek fails
 */
int csv_reader_seek(struct csv_reader*, size_t offset);
/**
 * For mmap only: goto a specific location via address
 */
int csv_reader_goto(struct csv_reader*, const char*);
/**
 * This function is available but is called internally
 * when the end of the file has been reached.
 */
int csv_reader_close(struct csv_reader*);

/**
 * Parse an individual const char*. This function is used
 * internally by csv_get_record, and is only meant to handle
 * a single record. This will not treat new lines as a
 * record separator, nor increment embedded break count.
 */
int csv_parse(struct csv_reader*, struct csv_record*, const char*);
int csv_nparse(struct csv_reader*, struct csv_record*, const char*, unsigned char_limit);
int csv_parse_to(struct csv_reader*,
                 struct csv_record*,
                 const char*,
                 unsigned field_limit);
int csv_nparse_to(struct csv_reader*,
                  struct csv_record*,
                  const char*,
                  unsigned char_limit,
                  unsigned field_limit);

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
 * return true if writer has open file
 */
int csv_writer_isopen(struct csv_writer*);

/**
 * access delimiter as const char*
 */
const char* csv_writer_get_delim(struct csv_writer*);
struct csv_field csv_writer_get_delim_field(struct csv_writer*);

/**
 * access line ending as const char*
 */
const char* csv_writer_get_terminator(struct csv_writer*);
struct csv_field csv_writer_get_terminator_field(struct csv_writer*);

/**
 * access FILE* for writer
 */
FILE* csv_writer_get_file(struct csv_writer*);

/**
 * retrieve file name to be written to
 */
const char* csv_writer_get_filename(struct csv_writer*);

/**
 * Retrieve file name, and remove it from
 * the writer. Caller now owns the allocated
 * memory for the file name.
 */
char* csv_writer_detach_filename(struct csv_writer*);

/**
 * Forcefully change the output FILE*. Useful for
 * redirecting temporarily
 */
void csv_writer_set_file(struct csv_writer*, FILE*);

/**
 * Set output delimiter
 */
void csv_writer_set_delim(struct csv_writer*, const char*);

/**
 * set output record terminator
 */
void csv_writer_set_line_ending(struct csv_writer*, const char*);

/**
 * Open a file for writing csv conents
 */
int csv_writer_open(struct csv_writer*, const char*);

/**
 * All writing is done to temp files that are
 * then renamed. The renaming occurs when you
 * call csv_writer_close Therefore, the
 * following is legal:
 *
 * csv_writer_open(writer, "foo.txt");
 * csv_write_record(writer, record);
 * csv_writer_set_filename(writer, "bar.txt");
 * csv_writer_close(writer);
 *
 * The data from record will be stored in
 * bar.txt and foo.txt does not exist.
 */
void csv_writer_set_filename(struct csv_writer*, const char*);

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
int csv_write_record(struct csv_writer*, struct csv_record*);

/**
 * Write a single field from a record.  This function
 * will not print write delimiters or line endings.
 */
int csv_write_field(struct csv_writer*, const struct csv_field* field);

#ifdef __cplusplus
}
#endif

#endif /* CSV_H */
