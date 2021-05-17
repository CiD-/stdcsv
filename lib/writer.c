#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "csv.h"
#include <libgen.h>
#include "csvsignal.h"
#include "csverror.h"
#include "internal.h"
#include "misc.h"
#include "util/util.h"


struct csv_writer* csv_writer_new()
{
	struct csv_writer* self = malloc_(sizeof(*self));
	return self;

}
struct csv_writer* csv_writer_construct(struct csv_writer* self)
{
	init_sig();

	*self = (struct csv_writer) {
		 NULL          /* _in */
		,QUOTE_RFC4180 /* quotes */
	};

	self->_in = malloc_(sizeof(*self->_in));
	*self->_in = (struct csv_write_internal) {
		 stdout /* file */
		,NULL   /* tmp_node */
		,NULL   /* tempname */
		,NULL   /* filename */
		,NULL   /* filename_org */
		,{ 0 }  /* buffer */
		,{ 0 }  /* delim */
		,{ 0 }  /* rec_terminator */
		,0      /* record_len */
	};

	string_construct(&self->_in->buffer);
	string_construct(&self->_in->delim);
	string_construct_from_char_ptr(&self->_in->rec_terminator, "\n");

	return self;
}

void csv_writer_free(struct csv_writer* self)
{
	csv_writer_destroy(self);
	free_(self);
}

void csv_writer_destroy(struct csv_writer* self)
{
	delete_ (string, self->_in->tempname);
	delete_ (string, self->_in->filename);
	delete_ (string, self->_in->filename_org);
	string_destroy(&self->_in->buffer);
	string_destroy(&self->_in->delim);
	string_destroy(&self->_in->rec_terminator);
	free_(self->_in);
}

void csv_write_field(struct csv_writer* self, const struct csv_field* field)
{
	_Bool quote_current_field = false;
	const char* c = field->data;

	if (memchr(field->data, '"', field->len)
	 || memchr(field->data, '\r', field->len)
	 || memchr(field->data, '\n', field->len)
	 || memmem(field->data,
		   field->len,
		   self->_in->delim.data,
		   self->_in->delim.size)) {
		fprintf(self->_in->file, "\"%.*s\"", field->len, field->data);
		return;
	}

	fprintf(self->_in->file, "%.*s", field->len, field->data);
}

void csv_write_record(struct csv_writer* self, struct csv_record* rec)
{
	int i = 0;
	for (; i < rec->size; ++i) {
		if (i)
			fputs(string_c_str(&self->_in->delim), self->_in->file);
		csv_write_field(self, &rec->fields[i]);
	}

	fputs(self->_in->rec_terminator, self->_in->file);
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
		FILE* dump_file = fopen(self->_in->tempname, "r");
		csvfail_if_(!dump_file, self->_in->tempname);

		char c = '\0';
		while ((c = getc(dump_file)) != EOF)
			putchar(c);

		csvfail_if_(fclose(dump_file) == EOF, self->_in->tempname);
		tmp_remove_file(self->_in->tmp_node->data);
		tmp_remove_node(self->_in->tmp_node);
	}

	return 0;
}

