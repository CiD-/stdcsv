#include "util.h"
#include "csv.h"

//const struct csv_record blank_record = {NULL, 0, 0};

//const char* csv_get_end(struct csv_field *s)
//{
//        return s->begin + s->length;
//}
//
//char* csv_field_copy(struct csv_field *s)
//{
//        char* newString = NULL;
//        MALLOC(newString, s->length + 1);
//        csv_get_string(s, newString);
//        return newString;
//}

//int csv_get_string(struct csv_field *s, char* buffer)
//{
//        char* dest = strncpy(buffer, s->begin, s->length);
//        if(!dest)
//                return -1;
//        buffer[s->length] = '\0';
//        return 0;
//}

//void csv_destroyrecord(struct csv_record *record)
//{
//        FREE(record->fields);
//        //record->size = 0;
//}

//struct csv_field** csv_init()
//{
//        struct csv_field** fields = malloc(sizeof(struct csv_field*));
//        EXIT_IF(!fields, "csv_init malloc");
//
//        *fields = malloc(sizeof(struct csv_field));
//        EXIT_IF (!(*fields), "csv_field malloc")
//
//        fields[0]->begin = NULL;
//        fields[0]->size = 0;
//
//        return fields;
//}
