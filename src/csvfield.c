#include "csvrw.h"

const char* csv_get_end(struct csv_field s)
{
        return s.begin + s.size;
}

char* csv_newstring(struct csv_field s)
{
        char* newString = malloc(s.size + 1);
        csv_getstring(s, newString);
        return newString;
}

int csv_getstring(struct csv_field s, char* buffer)
{
        char* dest = strncpy(buffer, s.begin, s.size);
        if(!dest)
                return -1;
        buffer[s.size] = '\0';
        return 0;
}

void csv_growrecord(struct csv_record *record)
{
        ++record->size;
        uint arraySize = record->size * sizeof(struct csv_field);
        if (record->size > 1)
                record->fields = realloc(record->fields, arraySize);
        else
                record->fields = malloc(arraySize);

        EXIT_IF(!record->fields, "fields allocation");
}

void csv_destroyrecord(struct csv_record *record)
{
        FREE(record);
        record->size = 0;
}

/*
struct csv_field** csv_init()
{
        struct csv_field** fields = malloc(sizeof(struct csv_field*));
        EXIT_IF(!fields, "csv_init malloc");

        *fields = malloc(sizeof(struct csv_field));
        EXIT_IF (!(*fields), "csv_field malloc")

        fields[0]->begin = NULL;
        fields[0]->size = 0;

        return fields;
}
*/
