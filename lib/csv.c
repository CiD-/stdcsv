#include "csverror.h"
#include "misc.h"
#include "csv.h"
#include "internal.h"
#include "util/stringy.h"
#include "util/util.h"

void csv_perror()
{
	err_printall();
}

void csv_perror_exit()
{
	csv_perror();
	exit(EXIT_FAILURE);
}

struct csv_record* csv_record_new()
{
	struct csv_record* new_rec = NULL;
	new_rec = malloc_(sizeof(*new_rec));
	return csv_record_construct(new_rec);
}

struct csv_record* csv_record_construct(struct csv_record* self)
{
	*self = (struct csv_record) {
		NULL,   /* _in */
		NULL,   /* fields */
		0,      /* rec */
		0,      /* reclen */
		0,      /* size */
	};

	self->_in = malloc_(sizeof(*self->_in));
	*self->_in = (struct csv_record_internal) {
		new_t_(vec, string),           /* field_data */
		new_t_(vec, struct csv_field), /* _fields */
		0,                             /* rec_alloc */
		0,                             /* allocated */
	};

	return self;
}

void csv_record_free(struct csv_record* self)
{
	csv_record_destroy(self);
	free_(self);
}

void csv_record_destroy(struct csv_record* self)
{
	delete_ (vec, self->_in->_fields);
	int i = 0;
	for (; i < self->size; ++i)
		string_destroy(&self->_in->field_data[i]);
	delete_ (vec, self->_in->field_data);
	free_(self->_in);
}

struct csv_field* csv_record_release_data(struct csv_record* self)
{
	//delete_ (vec, self->_in->_fields);
	free_(self->_in->_fields);
	//int i = 0;
	//for (; i < self->size; ++i)
	//	string_destroy(&self->_in->field_data[i]);
	delete_ (vec, self->_in->field_data);
	free_(self->_in);
	free_(self);

	return self->fields;
}

struct csv_record* csv_record_clone(const struct csv_record* src)
{
	struct csv_record* dest = NULL;
	dest = malloc_(sizeof(*dest));

	*dest = (struct csv_record) {
		NULL,           /* _in */
		NULL,           /* fields */
		src->rec,       /* rec */
		src->reclen,    /* reclen */
		src->size,      /* size */
	};

	dest->_in = malloc_(sizeof(*dest->_in));
	*dest->_in = (struct csv_record_internal) {
		new_t_(vec, string),           /* field_data */
		new_t_(vec, struct csv_field), /* _fields */
		0,                             /* rec_alloc */
		src->size,                     /* allocated */
	};

	vec_resize(dest->_in->_fields, src->size);
	int i = 0;
	for (; i < src->size; ++i) {
		string* s_src = vec_at(src->_in->field_data, i);
		string* s_dest = vec_at(dest->_in->field_data, i);
		string_construct_from_string(s_dest, s_src);
	}

	vec_extend(dest->_in->_fields, src->_in->_fields);
	dest->fields = vec_begin(dest->_in->_fields);

	return dest;
}
