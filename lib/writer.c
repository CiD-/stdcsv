#include "csv.h"
#include <libgen.h>
#include "csvsignal.h"
#include "csverror.h"
#include "internal.h"
#include "misc.h"
#include "util/util.h"


struct csv_writer* csv_writer_new()
{
	init_sig();

	struct csv_writer* self = NULL;

	self = malloc_(sizeof(*self));
	*self = (struct csv_writer) {
		 NULL
		,stdout
		,","
		,"\n"
		,QUOTE_RFC4180
	};

	self->_in = malloc_(sizeof(*self->_in));
	*self->_in = (struct csv_write_internal) {
		 NULL   /* buffer */
		,NULL   /* tmp_node */
		,""     /* tempname */
		,""     /* filename */
		,""     /* filename_org */
		,0      /* bufferSize */
		,1      /* delimLen */
		,0      /* recordLen */
	};

	increase_buffer(&self->_in->buffer, &self->_in->bufferSize);

	return self;

}

void csv_writer_free(struct csv_writer* self)
{
	free_(self->_in->buffer);
	free_(self->_in);
	free_(self);
}

void csv_nwrite_field(struct csv_writer* self, const char* field, unsigned char_limit)
{
	unsigned writeIndex = 0;
	int quoteCurrentField = 0;
	unsigned delimIdx = 0;
	const char* c = NULL;

	quoteCurrentField = (self->quotes == QUOTE_ALL);
	writeIndex = 0;
	for (c = field; *c && c - field < char_limit; ++c) {
		if (writeIndex + 3 > self->_in->bufferSize)
			increase_buffer(&self->_in->buffer, &self->_in->bufferSize);
		self->_in->buffer[writeIndex++] = *c;
		if (*c == '"' && self->quotes >= QUOTE_RFC4180) {
			self->_in->buffer[writeIndex++] = '"';
			quoteCurrentField = true;
		}
		if (self->quotes && !quoteCurrentField) {
			if (strhaschar("\"\n\r", *c))
				quoteCurrentField = 1;
			else if (*c == self->delimiter[delimIdx])
				++delimIdx;
			else
				delimIdx = (*c == self->delimiter[0]) ? 1 : 0;

			if (delimIdx == self->_in->delimLen)
				quoteCurrentField = true;
		}
	}
	self->_in->buffer[writeIndex] = '\0';
	if (quoteCurrentField)
		fprintf(self->file, "\"%s\"", self->_in->buffer);
	else
		fputs(self->_in->buffer, self->file);
}

void csv_write_field(struct csv_writer* self, const char* field)
{
	csv_nwrite_field(self, field, UINT_MAX);
}

void csv_write_record(struct csv_writer* self, struct csv_record* rec)
{
	int i = 0;
	for (; i < rec->size; ++i) {
		if (i)
			fputs(self->delimiter, self->file);
		csv_write_field(self, rec->fields[i]);
	}

	fputs(self->line_terminator, self->file);
}

int csv_writer_reset(struct csv_writer* self)
{
	csvfail_if_(self->file == stdout, "Cannot reset stdout");
	csvfail_if_(!self->file, "No file to reset");
	csvfail_if_(fclose(self->file) == EOF, self->_in->tempname);
	//self->file = NULL;
	self->file = fopen(self->_in->tempname, "w");
	csvfail_if_(!self->file, self->_in->tempname);

	return 0;
}

int csv_writer_mktmp(struct csv_writer* self)
{
	char filename_cp[PATH_MAX] = "";
	strncpy_(filename_cp, self->_in->filename, PATH_MAX);

	char* targetdir = dirname(filename_cp);
	strncpy_(self->_in->tempname, targetdir, PATH_MAX);
	strcat(self->_in->tempname, "/csv_XXXXXX");

	int fd = mkstemp(self->_in->tempname);
	self->file = fdopen(fd, "w");
	csvfail_if_(!self->file, self->_in->tempname);

	self->_in->tmp_node = tmp_push(self->_in->tempname);

	return 0;
}

int csv_writer_isopen(struct csv_writer* self)
{
	if (self->file && self->file != stdout)
		return true;
	return false;
}

int csv_writer_open(struct csv_writer* self, const char* filename)
{
	int ret = 0;
	if(!csv_writer_isopen(self))
		ret = csv_writer_mktmp(self);
	strncpy_(self->_in->filename, filename, PATH_MAX);
	return ret;
}

int csv_writer_close(struct csv_writer* self)
{
	if (self->file == stdout)
		return 0;

	csvfail_if_(fclose(self->file) == EOF, self->_in->tempname);
	self->file = NULL;

	if (self->_in->filename[0] != '\0') {
		int ret = rename(self->_in->tempname, self->_in->filename);
		csvfail_if_(ret, self->_in->tempname);
		ret = chmod(self->_in->filename, 0666);
		csvfail_if_(ret, self->_in->filename);
		tmp_remove_node(self->_in->tmp_node);
	} else {
		FILE* dumpFile = fopen(self->_in->tempname, "r");
		csvfail_if_(!dumpFile, self->_in->tempname);

		char c = '\0';
		while ((c = getc(dumpFile)) != EOF)
			putchar(c);

		csvfail_if_(fclose(dumpFile) == EOF, self->_in->tempname);
		tmp_remove_file(self->_in->tmp_node->data);
		tmp_remove_node(self->_in->tmp_node);
	}

	return 0;
}

