#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "csv.h"
#include "csvsignal.h"
#include "csverror.h"
#include "safegetline.h"
#include "internal.h"
#include "misc.h"
#include "util/vec.h"
#include "util/stringy.h"
#include "util/stringview.h"
#include "util/util.h"

/**
 * Internal prototypes
 */

/**
 * csv_determine_delimiter chooses between comma, pipe, semi-colon,
 * colon or tab depending on which  one is found most. If none of
 * these delimiters are found, use comma. If self->delimiter was set
 * externally, simply update self->_in->delimlen and return.
 */
void csv_determine_delimiter(struct csv_reader*, const char* header, unsigned byte_limit);

/**
 * csv_record_grow allocates space for the fields
 * member (char**) of the csv_record struct
 */
void csv_record_grow(struct csv_record*);

/**
 * csv_append_line is used to retrieve another line
 * of input for an in-line break.
 */
int csv_append_line(struct csv_reader* self, struct csv_record*);

/**
 * Simple CSV parsing. Disregard all quotes.
 */
int csv_parse_none(struct csv_reader*,
                   struct csv_record*,
                   const char** line,
                   size_t* recidx,
                   unsigned* byte_limit);

/**
 * Parse line respecting quotes. Quotes within
 * the field are not expected to be duplicated.
 * Leading spaces will cause the field to not
 * be treated as qualified.
 */
int csv_parse_weak(struct csv_reader*,
                   struct csv_record*,
                   const char** line,
                   size_t* recidx,
                   unsigned* byte_limit);

/**
 * Parse line according to RFC-4180 guidelines.
 * More info: https://tools.ietf.org/html/rfc4180
 */
int csv_parse_rfc4180(struct csv_reader* self,
                      struct csv_record*,
                      const char** line,
                      size_t* recidx,
                      unsigned* byte_limit);

struct csv_reader* csv_reader_new()
{
	struct csv_reader* reader = malloc_(sizeof(*reader));
	return csv_reader_construct(reader);
}

struct csv_reader* csv_reader_construct(struct csv_reader* reader)
{
	init_sig();
	*reader = (struct csv_reader) {
	        NULL,          /* internal */
	        0,             /* offset */
	        QUOTE_RFC4180, /* quotes */
	        0,             /* Normal */
	        false,         /* failsafe_mode */
	        false          /* trim */
	};

	reader->_in = malloc_(sizeof(*reader->_in));
	*reader->_in = (struct csv_read_internal) {
	        stdin, /* file */
	        {0},   /* delim */
	        {0},   /* weak_delim */
	        {0},   /* embedded_breaks */
	        NULL,  /* mmap_ptr */
	        0,     /* offset */
	        0,     /* file_size */
	        0,     /* fd */
	        0,     /* rows */
	        0,     /* embedded_breaks */
	        0,     /* normorg */
	        false  /* is_mmap */
	};

	string_construct(&reader->_in->delim);
	string_construct(&reader->_in->weak_delim);
	string_construct_from_char_ptr(&reader->_in->embedded_break, "\n");

	return reader;
}

void csv_reader_free(struct csv_reader* self)
{
	csv_reader_destroy(self);
	free_(self);
}

void csv_reader_destroy(struct csv_reader* self)
{
	string_destroy(&self->_in->delim);
	string_destroy(&self->_in->weak_delim);
	string_destroy(&self->_in->embedded_break);
	free_(self->_in);
}

/**
 * Simple accessors
 */
unsigned csv_reader_row_count(struct csv_reader* self)
{
	return self->_in->rows;
}

unsigned csv_reader_embedded_breaks(struct csv_reader* self)
{
	return self->_in->embedded_breaks;
}

const char* csv_reader_get_delim(struct csv_reader* self)
{
	return string_c_str(&self->_in->delim);
}

size_t csv_reader_get_file_size(struct csv_reader* self)
{
	if (self->_in->is_mmap || (self->_in->file && self->_in->file != stdin)) {
		return 0;
	}

	return self->_in->file_size;
}

/**
 * Mutators
 */
void csv_reader_set_delim(struct csv_reader* self, const char* delim)
{
	string_strcpy(&self->_in->delim, delim);
	string_sprintf(&self->_in->weak_delim, "\"%s", delim);
}

void csv_reader_set_embedded_break(struct csv_reader* self, const char* embedded_break)
{
	string_strcpy(&self->_in->embedded_break, embedded_break);
}

void csv_record_grow(struct csv_record* self)
{
	string* s = vec_add_one(self->_in->field_data);
	string_construct(s);
	++self->_in->field_alloc;

	struct csv_field field = {s->data, s->size};
	vec_push_back(self->_in->_fields, &field);
	self->fields = vec_begin(self->_in->_fields);
}

int csv_get_record(struct csv_reader* self, struct csv_record* rec)
{
	return csv_get_record_to(self, rec, UINT_MAX);
}

int csv_get_record_to(struct csv_reader* self,
                      struct csv_record* rec,
                      unsigned field_limit)
{
	int ret = 0;
	if (self->_in->is_mmap) {
		/* sgetline allocates memory into rec, and sgetline_mmap
		 * references an mmaped file. If a record was used on
		 * sgetline first, it will have allocated memory in 
		 * rec->rec.  Free that first. */
		if (rec->_in->rec_alloc > 0) {
			rec->_in->rec_alloc = 0;
			free_(rec->rec);
		}
		ret = sgetline_mmap(self->_in->mmap_ptr,
		                    &rec->rec,
		                    &self->_in->offset,
		                    &rec->reclen,
		                    self->_in->file_size);
		self->offset = self->_in->offset;
	} else {
		ret = sgetline(self->_in->file,
		               &rec->rec,
		               &rec->_in->rec_alloc,
		               &rec->reclen);
		if (self->_in->file != stdin) {
			self->offset = ftello(self->_in->file);
		}
	}

	if (ret == EOF) {
		self->normal = self->_in->normorg;
		return ret;
	}

	return csv_nparse_to(self, rec, rec->rec, rec->reclen, field_limit);
}

int csv_lowerstandard(struct csv_reader* self)
{
	if (!self->failsafe_mode || (self->_in->file == stdin && !self->_in->is_mmap)) {
		switch (self->quotes) {
		case QUOTE_ALL:
		case QUOTE_RFC4180:
			fprintf(stderr,
			        "Line %d: RFC4180 Qualifier issue.\n",
			        1 + self->_in->rows + self->_in->embedded_breaks);
			break;
		case QUOTE_WEAK:
			fprintf(stderr,
			        "Line %d: Qualifier issue.\n",
			        1 + self->_in->rows + self->_in->embedded_breaks);
			break;
		default:
			fputs("Unexpected Condition.\n", stderr);
		}

		return CSV_FAIL;
	}

	switch (self->quotes) {
	case QUOTE_ALL:
	case QUOTE_RFC4180:
		fprintf(stderr,
		        "Line %d: Qualifier issue. RFC4180 quotes disabled.\n",
		        1 + self->_in->rows + self->_in->embedded_breaks);
		break;
	case QUOTE_WEAK:
		fprintf(stderr,
		        "Line %d: Qualifier issue. Quotes disabled.\n",
		        1 + self->_in->rows + self->_in->embedded_breaks);
		break;
	default:
		fputs("Unexpected Condition.\n", stderr);
		return CSV_FAIL;
	}

	--self->quotes;
	//self->_in->rows = 0;
	//self->_in->embedded_breaks = 0;
	//fseek(self->_in->file, 0, SEEK_SET);
	csv_reader_reset(self);

	return CSV_RESET;
}

void csv_append_empty_field(struct csv_record* self)
{
	if (++(self->size) > (int)self->_in->field_alloc)
		csv_record_grow(self);
}

int csv_parse(struct csv_reader* self, struct csv_record* rec, const char* line)
{
	return csv_nparse_to(self, rec, line, UINT_MAX, UINT_MAX);
}

int csv_nparse(struct csv_reader* self,
               struct csv_record* rec,
               const char* line,
               unsigned byte_limit)
{
	return csv_nparse_to(self, rec, line, byte_limit, UINT_MAX);
}

int csv_parse_to(struct csv_reader* self,
                 struct csv_record* rec,
                 const char* line,
                 unsigned field_limit)
{
	return csv_nparse_to(self, rec, line, UINT_MAX, field_limit);
}

int csv_nparse_to(struct csv_reader* self,
                  struct csv_record* rec,
                  const char* line,
                  unsigned byte_limit,
                  unsigned field_limit)
{
	/* Does libcsv not own `line'? */
	if (byte_limit == UINT_MAX) {
		byte_limit = strlen(line);
		rec->reclen = byte_limit;
		/* I'm not making any promises... */
		rec->rec = (char*)line; /* meh */
	}

	if (string_empty(&self->_in->delim))
		csv_determine_delimiter(self, line, byte_limit);

	rec->size = 0;
	//rec->rec = NULL;
	//rec->reclen = 0;
	size_t recidx = 0;
	int ret = 0;

	while (recidx < byte_limit && (unsigned)rec->size < field_limit) {
		if (rec->size > 0) {
			recidx += self->_in->delim.size;
		}
		csv_append_empty_field(rec);

		int quotes = self->quotes;
		if (quotes != QUOTE_NONE && line[recidx] != '"') {
			quotes = QUOTE_NONE;
		}

		switch (quotes) {
		case QUOTE_ALL:
			/* Not seeing a point to implementing this for reading. */
		case QUOTE_RFC4180:
			ret = csv_parse_rfc4180(self, rec, &line, &recidx, &byte_limit);
			break;
		case QUOTE_WEAK:
			ret = csv_parse_weak(self, rec, &line, &recidx, &byte_limit);
			break;
		case QUOTE_NONE:
			ret = csv_parse_none(self, rec, &line, &recidx, &byte_limit);
			break;
		}

		if (ret == CSV_RESET) {
			ret = csv_lowerstandard(self);
			csv_reader_reset(self);
			return ret;
		}
	}

	if (self->normal > 0) {
		/* Append fields if we are short */
		while (self->normal > rec->size)
			csv_append_empty_field(rec);
		rec->size = self->normal;
	}

	if (self->normal == CSV_NORMAL_OPEN)
		self->normal = rec->_in->field_alloc;

	++self->_in->rows;
	return 0;
}

int csv_append_line(struct csv_reader* self, struct csv_record* rec)
{
	int ret = 0;

	const char* old_rec = rec->rec;
	if (self->_in->is_mmap) {
		ret = sappline_mmap(self->_in->mmap_ptr,
		                    &rec->rec,
		                    &self->_in->offset,
		                    &rec->reclen,
		                    self->_in->file_size);
	} else {
		ret = sappline(self->_in->file,
		               &rec->rec,
		               &rec->_in->rec_alloc,
		               &rec->reclen);
	}

	if (old_rec == rec->rec) {
		return ret;
	}

	/* If we have reached this spot, there was a realloc
	 * that moved the location of the record.  Any fields
	 * that were pointing to the record are now invalid
	 * and must be fixed.
	 */
	unsigned i = 0;
	for (; i < rec->_in->_fields->size; ++i) {
		struct csv_field* field = vec_at(rec->_in->_fields, i);
		string* field_data = vec_at(rec->_in->field_data, i);
		if (field->data == field_data->data) {
			continue;
		}
		size_t offset = field->data - old_rec;
		field->data = rec->rec + offset;
	}

	return ret;
}

int csv_parse_rfc4180(struct csv_reader* self,
                      struct csv_record* rec,
                      const char** line,
                      size_t* recidx,
                      unsigned* byte_limit)
{
	string* field_data = vec_at(rec->_in->field_data, rec->size - 1);
	string_clear(field_data);

	unsigned trailing_space = 0;
	unsigned nl_count = 0;
	_Bool keep = true;
	_Bool first_char = true;
	_Bool last_was_quote = false;
	_Bool qualified = true;
	_Bool delim_skip = false;

	const char* begin = &(*line)[++(*recidx)];
	const char* ptr = begin;
	const char* end = NULL;
	const char* rec_end = &(*line)[*byte_limit];

	for (;;) {
		for (; qualified && end != rec_end; ptr = end + self->_in->delim.size) {
			end = memmem(ptr,
			             *byte_limit - *recidx,
			             self->_in->delim.data,
			             self->_in->delim.size);

			if (!delim_skip && ptr != begin) {
				string_append(field_data, &self->_in->delim);
			}

			delim_skip = (end == NULL);
			if (delim_skip) {
				end = rec_end;
			}
			vec_reserve(field_data, end - begin);

			const char* it = ptr;
			for (; it < end; ++it) {
				keep = true;
				if (qualified) {
					qualified = (*it != '"');
					if (!qualified) {
						keep = false;
						last_was_quote = true;
					}
				} else {
					qualified = (*it == '"');
					if (qualified && !last_was_quote)
						keep = false;
					last_was_quote = false;
				}
				if (!keep || (first_char && self->trim && isspace(*it))) {
					continue;
				}
				string_push_back(field_data, *it);

				first_char = false;
				if (self->trim && isspace(*it))
					++trailing_space;
				else
					trailing_space = 0;
			}
		}

		if (!qualified)
			break;

		/** In-line break found **/
		if (qualified && !rec->_in->rec_alloc && !self->_in->is_mmap) {
			return CSV_RESET;
		}

		if (++nl_count > CSV_MAX_NEWLINES) {
			return CSV_RESET;
		}
		int ret = csv_append_line(self, rec);
		string_append(field_data, &self->_in->embedded_break);
		if (ret == EOF) {
			return CSV_RESET;
		}
		*line = rec->rec;
		begin = &(*line)[*recidx];
		ptr = &(*line)[*byte_limit + 1];
		*byte_limit = rec->reclen;
		rec_end = &(*line)[*byte_limit];
	}

	if (trailing_space) {
		string_resize(field_data, field_data->size - trailing_space);
	}
	struct csv_field* field = vec_at(rec->_in->_fields, rec->size - 1);
	field->data = field_data->data;
	field->len = field_data->size;

	*recidx += (end - begin);

	self->_in->embedded_breaks += nl_count;
	return CSV_GOOD;
}

int csv_parse_weak(struct csv_reader* self,
                   struct csv_record* rec,
                   const char** line,
                   size_t* recidx,
                   unsigned* byte_limit)
{
	string* field_data = vec_at(rec->_in->field_data, rec->size - 1);
	string_clear(field_data);
	unsigned trailing_space = 0;
	unsigned nl_count = 0;
	_Bool first_char = true;

	const char* begin = &(*line)[++(*recidx)];
	const char* end = memmem(begin,
	                         *byte_limit - *recidx,
	                         self->_in->weak_delim.data,
	                         self->_in->weak_delim.size);

	while (end == NULL) {
		end = &(*line)[*byte_limit];
		/* Quote before EOL */
		if (*(end - 1) == '"' && begin != end) {
			--end;
			break;
		}

		/* Cannot reset if we don't own source */
		if (!rec->_in->rec_alloc && !self->_in->is_mmap) {
			return CSV_RESET;
		}

		/* Looks fucky */
		if (++nl_count > CSV_MAX_NEWLINES) {
			return CSV_RESET;
		}
		int ret = csv_append_line(self, rec);

		/* Hit EOF before finishing record */
		if (ret == EOF) {
			return CSV_RESET;
		}
		*line = rec->rec;
		begin = &(*line)[*recidx];
		unsigned old_limit = *byte_limit;
		*byte_limit = rec->reclen;
		end = memmem(*line + old_limit,
		             *byte_limit - old_limit,
		             self->_in->weak_delim.data,
		             self->_in->weak_delim.size);
	}

	vec_reserve(field_data, end - begin);

	const char* it = begin;
	for (; it < end; ++it) {
		if (first_char && self->trim && isspace(*it)) {
			continue;
		}
		string_push_back(field_data, *it);

		first_char = false;
		if (self->trim && isspace(*it))
			++trailing_space;
		else
			trailing_space = 0;
	}

	if (trailing_space) {
		string_resize(field_data, field_data->size - trailing_space);
	}
	struct csv_field* field = vec_at(rec->_in->_fields, rec->size - 1);
	field->data = field_data->data;
	field->len = field_data->size;

	/* + 1 because we treated " as part of delimiter */
	*recidx += (end - begin) + 1;

	self->_in->embedded_breaks += nl_count;
	return CSV_GOOD;
}

int csv_parse_none(struct csv_reader* self,
                   struct csv_record* rec,
                   const char** line,
                   size_t* recidx,
                   unsigned* byte_limit)
{
	unsigned trailing_space = 0;
	_Bool first_char = true;

	const char* begin = &(*line)[*recidx];
	const char* end = memmem(begin,
	                         *byte_limit - *recidx,
	                         self->_in->delim.data,
	                         self->_in->delim.size);
	if (end == NULL) {
		end = &(*line)[*byte_limit];
	}

	const char* it = begin;
	for (; it != end; ++it) {
		if (first_char && self->trim && isspace(*it)) {
			++begin;
			++(*recidx);
			continue;
		}

		first_char = false;
		if (self->trim && isspace(*it))
			++trailing_space;
		else
			trailing_space = 0;
	}

	struct csv_field* field = vec_at(rec->_in->_fields, rec->size - 1);
	field->data = begin;
	field->len = end - begin - trailing_space;

	*recidx += (end - begin);

	return CSV_GOOD;
}

void csv_determine_delimiter(struct csv_reader* self,
                             const char* header,
                             unsigned byte_limit)
{
	if (!string_empty(&self->_in->delim)) {
		return;
	}

	const char* delims = ",|\t;:";
	unsigned i = 0;
	int sel = 0;
	int count = 0;
	int max_count = 0;
	for (; i < strlen(delims); ++i) {
		count = charncount(header, delims[i], byte_limit);
		if (count > max_count) {
			sel = i;
			max_count = count;
		}
	}

	char delim[2];
	delim[0] = delims[sel];
	delim[1] = '\0';

	csv_reader_set_delim(self, delim);
}

int csv_reader_open(struct csv_reader* self, const char* file_name)
{
	self->_in->file = fopen(file_name, "r");
	csvfail_if_(!self->_in->file, file_name);

	/* populate size */
	self->_in->fd = fileno(self->_in->file);
	struct stat sb;
	csvfail_if_(fstat(self->_in->fd, &sb) == -1, file_name);
	self->_in->file_size = sb.st_size;

	return CSV_GOOD;
}

int csv_reader_open_mmap(struct csv_reader* self, const char* file_name)
{
	self->_in->fd = open(file_name, O_RDONLY);
	csvfail_if_(self->_in->fd == -1, file_name);

	struct stat sb;
	csvfail_if_(fstat(self->_in->fd, &sb) == -1, file_name);

	if (sb.st_size != 0) {
		self->_in->mmap_ptr =
		        mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, self->_in->fd, 0);
		csvfail_if_(self->_in->mmap_ptr == MAP_FAILED, "mmap");
		csv_reader_madvise(self, MADV_SEQUENTIAL);
	}

	self->_in->file_size = sb.st_size;
	self->_in->is_mmap = true;

	return CSV_GOOD;
}

int csv_reader_madvise(struct csv_reader* self, int advice)
{
	csvfail_if_(!self->_in->is_mmap, "Cannot advise unless mmap");
	madvise(self->_in->mmap_ptr, self->_in->file_size, advice);
	return CSV_GOOD;
}

int csv_reader_seek(struct csv_reader* self, size_t offset)
{
	csvfail_if_(offset > self->_in->file_size, "offset out of range");

	self->offset = offset;

	if (self->_in->is_mmap) {
		self->_in->offset = offset;
		return CSV_GOOD;
	}

	csvfail_if_(!self->_in->file, "<null> file");
	csvfail_if_(self->_in->file == stdin, "cannot seek stdin");
	int ret = fseek(self->_in->file, offset, SEEK_SET);
	csvfail_if_(ret, "fseek");

	return CSV_GOOD;
}

int csv_reader_goto(struct csv_reader* self, const char* location)
{
	csvfail_if_(!self->_in->is_mmap, "function `goto' only for mmap");
	return csv_reader_seek(self, location - self->_in->mmap_ptr);
}

int csv_reader_close(struct csv_reader* self)
{
	if (self->_in->is_mmap) {
		self->_in->is_mmap = false;
		csvfail_if_(munmap(self->_in->mmap_ptr, self->_in->file_size), "munmap");
		csvfail_if_(close(self->_in->fd), "close");
	} else if (self->_in->file && self->_in->file != stdin) {
		csvfail_if_(fclose(self->_in->file), "fclose");
	}
	return CSV_GOOD;
}

int csv_reader_reset(struct csv_reader* self)
{
	self->_in->rows = 0;
	self->_in->embedded_breaks = 0;
	self->normal = self->_in->normorg;

	return csv_reader_seek(self, 0);
}
