#ifndef CSV_H
#define CSV_H

#ifndef TMPDIR
#define TMPDIR  /tmp/
#endif

#define STRING_VALUE(arg) #arg
#define TMPDIR_STR STRING_VALUE(TMPDIR)

#define MIN_SPACE_AVAILABLE     5000000000

#define CSV_RESET               -1
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
#include <errno.h>

#include "util.h"
#include "safegetline.h"

///**
// * CSV Structures
// */
//
///* Structure containing an individual field */
//struct csv_field {
//        const char* begin;
//        size_t length;
//};
//
///* Forward declaration of internal structures */
//struct csv_read_internal;
//struct csv_write_internal;
//
///* Structure containing dynamic array of fields */
//struct csv_record {
//        char** fields;
//        int size;
//};
//
//struct csv_reader {
//        struct csv_read_internal* _internal;
//        char delimiter[32];
//        char inlineBreak[32];
//        int quotes;
//        int normal;
//        int failsafeMode;
//};
//
//struct csv_writer {
//        struct csv_write_internal* _internal;
//        char delimiter[32];
//        char lineEnding[3];
//        int quotes;
//};
//
//extern const struct csv_record blank_record;
//
///**
// * CSV Field
// */
//
///**
// * Return char* to end of field
// */
////const char* csv_get_end(struct csv_field* s);
//
///**
// * Provide dynamically allocated char*
// * copy of the field
// */
////char* csv_field_copy(struct csv_field* s);
//
///**
// * Assigns buffer to a copy of the data within
// * the field. This char* should already be allocated.
// * If strncpy fails, return -1
// */
////int csv_get_string(struct csv_field* s, char* buffer);
//
//
//
//
///**
// * CSV Reader
// */
//
///**
// * Buffers for reading and appending are allocated here.
// */
//struct csv_reader* csv_reader_new();
//
///**
// * Relese allocated heap resources
// */
//void csv_reader_free(struct csv_reader*);
//
///**
// * Open a csv file for reading.  This file will close itself
// * when reading has compeleted.
// */
//void csv_reader_open(struct csv_reader*, const char* fileName);
//
///**
// * csv_get_record takes an array of pointers to csv_field's.
// * The variable fieldCount is set within this function to the
// * number of fields in the current csv line.  If this function
// * fails, fieldCount will be set to -1.
// *
// * Returns:
// *      - an array of pointers to struct csv_field which
// *        represents the parsed fields from the next line.
// *      - NULL if EOF
// */
//struct csv_record* csv_get_record(struct csv_reader*);
//
///**
// *
// */
//struct csv_record* csv_parse(struct csv_reader*, char* line);
//
//
//
//
///**
// * CSV Writer
// */
//
///**
// * Allocate resources for a writer
// */
//struct csv_writer* csv_writer_new();
//
///**
// * Relese allocated heap resources
// */
//void csv_writer_free(struct csv_writer*);
//
///**
// * Open a file for writing csv conents
// */
//void csv_writer_open(struct csv_writer*, const char* fileName);
//
///**
// *
// */
//void csv_writer_close(struct csv_writer*);
//
///**
// * Loop through array of struct csv_field, and print the line to csvw_file.
// */
//void csv_write_record(struct csv_writer*, struct csv_record*);
//
///**
// *
// */
//void csv_writer_reset(struct csv_writer*);


#endif
