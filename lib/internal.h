#ifndef INTERNAL_H
#define INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include "util/queue.h"
#include "util/vec.h"
#include "util/stringy.h"

/**
 * Internal Structures
 */

/* [csv_record].fields always points to the data
 * owned by [csv_record]._in->_fields. This way
 * we do not expose our internal vector type.
 */
struct csv_record_internal {
	vec* field_data;     /* vec<string> */
	vec* _fields;        /* vec<struct csv_field> */
	unsigned allocated;
	unsigned org_limit;
};

struct csv_read_internal {
	FILE* file;
	char* linebuf;
	size_t linebuf_alloc;
	size_t linebuf_len; /* what is this? */

	string delim;
	string embedded_break;

	/* Statistics */
	unsigned rows;
	unsigned embedded_breaks;

	/* Properties */
	int normorg;
	_Bool is_mmap;
};

struct csv_write_internal {
	char* buffer;
	queue* tmp_node;
	string* tempname;
	string* filename;
	string* filename_org;

	size_t bufsize;
	unsigned delimlen;
	int reclen;
};


#endif
