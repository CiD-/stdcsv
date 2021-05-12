#include "csv.h"
#include "csvsignal.h"
#include "csverror.h"
#include "safegetline.h"
#include "internal.h"
#include "misc.h"
#include "util/vec.h"
#include "util/stringy.h"
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
void csv_determine_delimiter(struct csv_reader*, const char* header, unsigned char_limit);

/**
 * csv_record_grow allocates space for the fields
 * member (char**) of the csv_record struct
 */
void csv_record_grow(struct csv_record*);

/**
 * csv_append_line is used to retrieve another line
 * of input for an in-line break.
 */
int csv_append_line(struct csv_reader* self, struct csv_record *rec, size_t recidx, unsigned nlCount);

/**
 * Simple CSV parsing. Disregard all quotes.
 */
int csv_parse_none(struct csv_reader*, struct csv_record* rec, const char** line, size_t* recidx, unsigned char_limit);

/**
 * Parse line respecting quotes. Quotes within
 * the field are not expected to be duplicated.
 * Leading spaces will cause the field to not
 * be treated as qualified.
 */
int csv_parse_weak(struct csv_reader*, struct csv_record* rec, const char** line, size_t* recidx, unsigned char_limit);

/**
 * Parse line according to RFC-4180 guidelines.
 * More info: https://tools.ietf.org/html/rfc4180
 */
int csv_parse_rfc4180(struct csv_reader* self, struct csv_record* rec, const char** line, size_t* recidx, unsigned char_limit);


struct csv_reader* csv_reader_new()
{
	init_sig();

	struct csv_reader* reader = NULL;

	reader = malloc_(sizeof(*reader));
	*reader = (struct csv_reader) {
		 NULL           /* internal */
		,""             /* delimiter */
		,"\n"           /* embedded_break */
		,QUOTE_RFC4180  /* quotes */
		,0              /* Normal */
		,false          /* failsafeMode */
		,false          /* trim */
	};

	reader->_in = malloc_(sizeof(*reader->_in));
	*reader->_in = (struct csv_read_internal) {
		 stdin /* file */
		,NULL  /* linebuf */
		,0     /* linebufsize */
		,0     /* len */
		,0     /* delimlen */
		,0     /* rows */
		,0     /* embedded_breaks */
		,0     /* normorg */
	};

	return reader;
}

void csv_reader_free(struct csv_reader* self)
{
	free_(self->_in->linebuf);
	free_(self->_in);
	free_(self);
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

void csv_record_grow(struct csv_record* self)
{
        string* s = vec_add_one(self->_in->field_data);
	string_construct(s);
	++self->_in->allocated;
	
	struct csv_field field = {
		 s->data
		,s->size
	};
	vec_push_back(self->_in->_fields, &field);
	self->fields = vec_begin(self->_in->_fields);
}

int csv_get_record(struct csv_reader* self, struct csv_record* rec)
{
	return csv_nget_record(self, rec, UINT_MAX);
}

int csv_nget_record(struct csv_reader* self, struct csv_record* rec, unsigned char_limit)
{
	return csv_nget_record_to(self, rec, char_limit, UINT_MAX);
}

int csv_get_record_to(struct csv_reader* self, struct csv_record* rec, unsigned field_limit)
{
	return csv_nget_record_to(self, rec, UINT_MAX, field_limit);
}

int csv_nget_record_to(struct csv_reader* self, struct csv_record* rec, unsigned char_limit, unsigned field_limit)
{
	int ret = sgetline(self->_in->file,
			   &self->_in->linebuf,
			   &self->_in->linebufsize,
			   &self->_in->len);

	if (ret == EOF) {
		self->normal = self->_in->normorg;
		if (csv_reader_close(self) == CSV_FAIL)
			csv_perror();
		return ret;
	}

	return csv_nparse_to(self, rec, self->_in->linebuf, char_limit, field_limit);
}

int csv_lowerstandard(struct csv_reader* self)
{
	if (!self->failsafe_mode || self->_in->file == stdin) {
		switch(self->quotes) {
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

	switch(self->quotes) {
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
	self->_in->rows = 0;
	self->_in->embedded_breaks = 0;
	fseek(self->_in->file, 0, SEEK_SET);

	return CSV_RESET;
}

void csv_append_empty_field(struct csv_record* self)
{
	if (++(self->size) > self->_in->allocated)
		csv_record_grow(self);
}

int csv_parse(struct csv_reader *self, struct csv_record *rec, const char* line)
{
	return csv_nparse_to(self, rec, line, UINT_MAX, UINT_MAX);
}

int csv_nparse(struct csv_reader *self, struct csv_record *rec, const char* line, unsigned char_limit)
{
	return csv_nparse_to(self, rec, line, char_limit, UINT_MAX);
}

int csv_parse_to(struct csv_reader *self, struct csv_record *rec, const char* line, unsigned field_limit)
{
	return csv_nparse_to(self, rec, line, UINT_MAX, field_limit);
}

int csv_nparse_to(struct csv_reader *self, struct csv_record *rec, const char* line, unsigned char_limit, unsigned field_limit)
{
	if (!self->_in->delimlen)
		csv_determine_delimiter(self, line, char_limit);

	unsigned line_len = (char_limit == UINT_MAX) ? strlen(line) : char_limit;

	rec->size = 0;
	rec->rec = NULL;
	rec->reclen = 0;
	size_t recidx = 0;
	int ret = 0;

	while(line[recidx] != '\0' && recidx < char_limit && rec->size < field_limit) {
		csv_append_empty_field(rec);

		int quotes = self->quotes;
		if (self->quotes != QUOTE_NONE && line[recidx] != '"')
			quotes = QUOTE_NONE;

		switch(quotes) {
		case QUOTE_ALL:
			/* Not seeing a point to implementing self for reading. */
		case QUOTE_RFC4180:
			ret = csv_parse_rfc4180(self, rec, &line, &recidx, char_limit);
			break;
		case QUOTE_WEAK:
			ret = csv_parse_weak(self, rec, &line, &recidx, char_limit);
			break;
		case QUOTE_NONE:
			ret = csv_parse_none(self, rec, &line, &recidx, char_limit);
			break;
		}

		if (ret == CSV_RESET) {
			ret = csv_lowerstandard(self);
			csv_reader_reset(self);
			return ret;
		}
	}

	rec->rec = line;
	rec->reclen = line_len;

	if (self->normal > 0) {
		/* Append fields if we are short */
		while (self->normal > rec->size)
			csv_append_empty_field(rec);
		rec->size = self->normal;
	}

	if (self->normal == CSV_NORMAL_OPEN)
		self->normal = rec->_in->allocated;

	++self->_in->rows;



	return 0;
}

int csv_append_line(struct csv_reader* self, struct csv_record *rec, size_t recidx, unsigned nlCount)
{
	int ret = sappline(self->_in->file,
			   &self->_in->linebuf,
			   &self->_in->linebufsize,
			   &self->_in->len);

	unsigned nlLen = strlen(self->embedded_break);
	unsigned delta = self->_in->len - recidx + (nlLen+1) * nlCount;
	if (delta >= rec->_in->bufsize - rec->_in->bufidx) {
		increase_buffer_to(&rec->_in->buffer,
				   &rec->_in->bufsize,
				    rec->_in->bufsize + delta);

		/* Move all pointers from _record->fields */
		int i = rec->size - 1;
		for (; i >= 0; --i) {
			int distance = rec->fields[i] - rec->fields[0];
			rec->fields[i] = rec->_in->buffer + distance;
		}
	}

	strcpy(&rec->_in->buffer[rec->_in->bufidx], self->embedded_break);
	rec->_in->bufidx += nlLen;

	return ret;
}

int csv_parse_rfc4180(struct csv_reader* self, struct csv_record* rec, const char** line, size_t* recidx, unsigned char_limit)
{
	unsigned delimIdx = 0;
	unsigned lastDelimIdx = 0;
	unsigned qualified = false;
	unsigned lastWasQuote = false;
	unsigned trailingSpace = 0;
	unsigned nlCount = 0;
	int skip = false;
	int appendField = false;
	int firstChar = true;

	while (true) {
		for (; *recidx < char_limit && (*line)[*recidx] != '\0' && delimIdx != self->_in->delimlen; ++(*recidx)) {
			skip = false;
			if (qualified) {
				qualified = ((*line)[*recidx] != '"');
				if (!qualified) {
					skip = true;
					lastWasQuote = true;
				}
			} else {
				if ((*line)[*recidx] == self->delimiter[delimIdx])
					++delimIdx;
				else
					delimIdx = ((*line)[*recidx] == self->delimiter[0]) ? 1 : 0;

				if ( (qualified = ((*line)[*recidx] == '"') ) ) {
					if (!lastWasQuote)
						skip = true;
				}
				lastWasQuote = false;
			}

			if (!skip) {
				if (!firstChar || !self->trim || !isspace((*line)[*recidx])) {
					rec->_in->buffer[rec->_in->bufidx++] = (*line)[*recidx];

					firstChar = false;
					if (self->trim && isspace((*line)[*recidx]))
						++trailingSpace;
					else if (delimIdx == lastDelimIdx)
						trailingSpace = 0;
				}
			}

			if ((*line)[*recidx] == '\n')
				++self->_in->embedded_breaks;

			lastDelimIdx = delimIdx;
		}
		/** In-line break found **/
		if ((*line)[*recidx] == '\0' && qualified) {
			if (++nlCount > CSV_MAX_NEWLINES ||
			    csv_append_line(self, rec, *recidx, nlCount) == EOF) {
				return CSV_RESET;
			}
			*line = self->_in->linebuf;
			continue;
		}

		break;
	}

	if (delimIdx == self->_in->delimlen) {
		/* Handle Trailing delimiter */
		if (!((*line)[*recidx]) || *recidx == char_limit)
			appendField = true;
		rec->_in->bufidx -= self->_in->delimlen;
	}

	rec->_in->bufidx -= trailingSpace;
	rec->_in->buffer[rec->_in->bufidx++] = '\0';

	if (appendField)
		csv_append_empty_field(rec);

	self->_in->embedded_breaks += nlCount;
	return CSV_GOOD;
}

int csv_parse_weak(struct csv_reader* self, struct csv_record* rec, const char** line, size_t* recidx, unsigned char_limit)
{
	unsigned delimIdx = 0;
	unsigned lastDelimIdx = 0;
	unsigned onQuote = false;
	unsigned trailingSpace = 0;
	int nlCount = 0;
	int appendField = false;
	int firstChar = true;

	++(*recidx);

	while (true) {
		for (; *recidx < char_limit && (*line)[*recidx] != '\0' && delimIdx != self->_in->delimlen; ++(*recidx)) {
			if (onQuote && (*line)[*recidx] == self->delimiter[delimIdx]) {
				++delimIdx;
			} else {
				delimIdx = 0;
				/* Handle trailing space after quote */
				//if (onQuote && isspace((*line)[*recidx])) {
				//        ++trailingSpace;
				//} else {
				//        trailingSpace = 0;
					onQuote = ((*line)[*recidx] == '"');
				//}
			}
			if (!firstChar || !self->trim || !isspace((*line)[*recidx])) {
				rec->_in->buffer[rec->_in->bufidx++] = (*line)[*recidx];

				firstChar = false;
				if (self->trim && isspace((*line)[*recidx]))
					++trailingSpace;
				else if (onQuote + delimIdx == lastDelimIdx)
					trailingSpace = 0;
			}

			if ((*line)[*recidx] == '\n')
				++self->_in->embedded_breaks;

			lastDelimIdx = delimIdx + onQuote;
		}
		if ((*line)[*recidx] == '\0' && !onQuote) {
			if (++nlCount > CSV_MAX_NEWLINES ||
			    csv_append_line(self, rec, *recidx, nlCount) == EOF) {
				return CSV_RESET;
			}
			*line = self->_in->linebuf;
			continue;
		}

		break;
	}

	if (delimIdx == self->_in->delimlen) {
		/* Handle Trailing delimiter */
		if (!((*line)[*recidx]) || *recidx == char_limit)
			appendField = true;
		rec->_in->bufidx -= self->_in->delimlen;
	}
	rec->_in->bufidx -= trailingSpace + 1;
	rec->_in->buffer[rec->_in->bufidx++] = '\0';

	if (appendField)
		csv_append_empty_field(rec);

	self->_in->embedded_breaks += nlCount;
	return CSV_GOOD;
}

int csv_parse_none(struct csv_reader* self, struct csv_record* rec, const char** line, size_t* recidx, unsigned char_limit)
{
	unsigned delimIdx = 0;
	unsigned lastDelimIdx = 0;
	unsigned trailingSpace = 0;
	int appendField = false;
	int firstChar = true;

	for (; *recidx < char_limit && (*line)[*recidx] != '\0' && delimIdx != self->_in->delimlen; ++(*recidx)) {
		if ((*line)[*recidx] == self->delimiter[delimIdx])
			++delimIdx;
		else if (delimIdx != 0)
			delimIdx = ((*line)[*recidx] == self->delimiter[0]) ? 1 : 0;

		if (!firstChar || !self->trim || !isspace((*line)[*recidx])) {
			rec->_in->buffer[rec->_in->bufidx++] = (*line)[*recidx];

			firstChar = false;
			if (self->trim && isspace((*line)[*recidx]))
				++trailingSpace;
			else if (delimIdx == lastDelimIdx)
				trailingSpace = 0;
		}


		if ((*line)[*recidx] == '\n')
			++self->_in->embedded_breaks;

		lastDelimIdx = delimIdx;
	}

	if (delimIdx == self->_in->delimlen) {
		/* Handle Trailing delimiter */
		if (!((*line)[*recidx]) || *recidx == char_limit)
			appendField = true;
		rec->_in->bufidx -= self->_in->delimlen;
	}

	rec->_in->bufidx -= trailingSpace;
	rec->_in->buffer[rec->_in->bufidx++] = '\0';

	if (appendField)
		csv_append_empty_field(rec);

	return CSV_GOOD;
}

void csv_determine_delimiter(struct csv_reader* self, const char* header, unsigned char_limit)
{
	unsigned delimlen = strlen(self->delimiter);
	if (delimlen) {
		self->_in->delimlen = delimlen;
		return;
	}

	const char* delims = ",|\t;:";
	unsigned i = 0;
	int sel = 0;
	int count = 0;
	int maxCount = 0;
	for (; i < strlen(delims); ++i) {
		count = charncount(header, delims[i], char_limit);
		if (count > maxCount) {
			sel = i;
			maxCount = count;
		}
	}

	self->delimiter[0] = delims[sel];
	self->delimiter[1] = '\0';
	self->_in->delimlen = 1;
}

int csv_reader_open(struct csv_reader* self, const char* fileName)
{
	self->_in->file = fopen(fileName, "r");
	csvfail_if_(!self->_in->file, fileName);
	return CSV_GOOD;
}

int csv_reader_close(struct csv_reader* self)
{
	if (self->_in->file && self->_in->file != stdin) {
		int ret = fclose(self->_in->file);
		csvfail_if_(ret, "fclose");
	}
	return CSV_GOOD;
}

int csv_reader_reset(struct csv_reader* self)
{
	self->normal = self->_in->normorg;
	self->delimiter[0] = '\0';
	self->_in->delimlen = 0;
	self->_in->embedded_breaks = 0;
	self->_in->rows = 0;
	//self->_in->linebuf[0] = '\0';
	if (self->_in->file && self->_in->file != stdin) {
		int ret = fseek(self->_in->file, 0, SEEK_SET);
		csvfail_if_(ret, "fseek");
	}
	return CSV_GOOD;
}
