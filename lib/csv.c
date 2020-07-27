#include "csverror.h"
#include "util.h"
#include "csv.h"

void csv_perror()
{
        err_printall();
}

void csv_perror_exit()
{
        csv_perror();
        exit(EXIT_FAILURE);
}

void csv_record_free(struct csv_record* rec)
{
        int i = 0;
        for (; i < rec->size; ++i)
                FREE(rec->fields[i]);
        FREE(rec->fields);
        FREE(rec);
}

struct csv_record* csv_record_export(struct csv_record* rec)
{
        struct csv_record* new_rec = NULL;
        MALLOC(new_rec, sizeof(*new_rec));

        *new_rec = (struct csv_record) {
                NULL,
                rec->size
        };

        MALLOC(new_rec->fields, rec->size * sizeof(char*));

        int i = 0;
        for (; i < rec->size; ++i)
                new_rec->fields[i] = strdup(rec->fields[i]);

        return new_rec;
}
