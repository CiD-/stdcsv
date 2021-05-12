#ifndef INTERNAL_H
#define INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "util/queue.h"
#include "util/vec.h"

/**
 * Internal Structures
 */

struct csv_record_internal {
	vec* field_data;     /* vec<string> */
	vec* _fields;        /* vec<struct csv_field> */
	unsigned allocated;
};

struct csv_read_internal {
	FILE* file;
	char* linebuf;
	size_t linebufsize;

	size_t len;
	unsigned delimlen;

	/* Statistics */
	unsigned rows;
	unsigned embedded_breaks;

	/* Properties */
	int normorg;
};

struct csv_write_internal {
	char* buffer;
	queue* tmp_node;
	char tempname[PATH_MAX];
	char filename[PATH_MAX];
	char filename_org[PATH_MAX];

	size_t bufferSize;
	unsigned delimLen;
	int recordLen;
};


#endif
