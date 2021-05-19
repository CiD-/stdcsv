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
	return csv_writer_construct(self);
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
	string_construct_from_char_ptr(&self->_in->delim, ",");
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
	tmp_remove_node(self->_in->tmp_node);
	delete_ (string, self->_in->filename);
	delete_ (string, self->_in->filename_org);
	string_destroy(&self->_in->buffer);
	string_destroy(&self->_in->delim);
	string_destroy(&self->_in->rec_terminator);
	free_(self->_in);
}

void _write_field_manually(struct csv_writer* self, const struct csv_field* field)
{
	unsigned i = 0;
	fputc('"', self->_in->file);
	for (; i < field->len; ++i) {
		fputc(field->data[i], self->_in->file);
		if (field->data[i] == '"') {
			fputc('"', self->_in->file);
		}
	}
	fputc('"', self->_in->file);
}

void csv_write_field(struct csv_writer* self, const struct csv_field* field)
{
	_Bool quote_current_field = false;
	const char* c = field->data;

	if (memchr(field->data, '"', field->len)) {
		quote_current_field = true;
		if (self->quotes >= QUOTE_RFC4180) {
			_write_field_manually(self, field);
			return; 
		}
	}
	if (quote_current_field
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

	fputs(string_c_str(&self->_in->rec_terminator), self->_in->file);
}

int csv_writer_reset(struct csv_writer* self)
{
	csvfail_if_(self->_in->file == stdout, "Cannot reset stdout");
	csvfail_if_(!self->_in->file, "No file to reset");
	csvfail_if_(fclose(self->_in->file) == EOF,
		    string_c_str(self->_in->tempname));
	//self->_in->file = NULL;
	self->_in->file = fopen(string_c_str(self->_in->tempname), "w");
	csvfail_if_(!self->_in->file, string_c_str(self->_in->tempname));

	return 0;
}

int csv_writer_mktmp(struct csv_writer* self)
{
	char* filename_cp = strdup(string_c_str(self->_in->filename));

	char* targetdir = dirname(filename_cp);
	self->_in->tempname = string_from_char_ptr(targetdir);
	string_strcat(self->_in->tempname, "/csv_XXXXXX");

	int fd = mkstemp(self->_in->tempname->data);
	self->_in->file = fdopen(fd, "w");
	csvfail_if_(!self->_in->file, string_c_str(self->_in->tempname));

	self->_in->tmp_node = tmp_push(self->_in->tempname);

	return 0;
}

int csv_writer_isopen(struct csv_writer* self)
{
	return (self->_in->file && self->_in->file != stdout);
}

void csv_writer_set_delim(struct csv_writer* self, const char* delim)
{
	string_strcpy(&self->_in->delim, delim);
}

void csv_writer_set_line_ending(struct csv_writer* self, const char* ending)
{
	string_strcpy(&self->_in->rec_terminator, ending);
}

int csv_writer_open(struct csv_writer* self, const char* filename)
{
	int ret = 0;
	if (!csv_writer_isopen(self))
		ret = csv_writer_mktmp(self);
	self->_in->filename = string_from_char_ptr(filename);
	return ret;
}

int csv_writer_close(struct csv_writer* self)
{
	if (self->_in->file == stdout)
		return 0;

	csvfail_if_(fclose(self->_in->file) == EOF,
			   string_c_str(self->_in->tempname));
	self->_in->file = NULL;

	const char* file = string_c_str(self->_in->filename);
	const char* tmp = string_c_str(self->_in->tempname);

	if (!string_empty(self->_in->filename)) {
		csvfail_if_(rename(tmp, file), tmp);
		csvfail_if_(chmod(file, 0666), file);
		tmp_remove_node(self->_in->tmp_node);
	} else {
		FILE* dump_file = fopen(tmp, "r");
		csvfail_if_(!dump_file, tmp);

		char c = '\0';
		while ((c = getc(dump_file)) != EOF)
			putchar(c);

		csvfail_if_(fclose(dump_file) == EOF, tmp);
		tmp_remove_file(self->_in->tmp_node->data);
		tmp_remove_node(self->_in->tmp_node);
	}
	return 0;
}

