#include "csv.h"

/* Signal Handlers */
static struct sigaction act;
/* sigset_t and sa_flags only set to shut up valgrind */
static sigset_t vg_shutup = { {0} };

static int _signalsReady = FALSE;


/**
 * Internal Structure
 */
struct csv_internal {
        FILE* file;
        char* buffer;
        char appendBuffer[CSV_BUFFER_FACTOR];
        char delimiter[32];
        char inlineBreak[32];
        uint fieldsAllocated;
        int recordLen;
        int bufferSize;
        uint delimLen;

        /* Statistics */
        uint rows;
        uint inlineBreaks;

        /* Flags */
        int standard;
        int allowStdChange;
        int normal;
        int normalOrg;

        int extraField;
};


/**
 * Internal prototypes
 */
//void csvr_determine_delimiter(const char* header);

char* csvr_get_delim(struct csv_record* rec)
{
        return rec->_internal->delimiter;
}

int csvr_get_allowstdchange(struct csv_record* rec)
{
        return rec->_internal->allowStdChange;
}

void csvr_set_delimiter(struct csv_record* rec, const char* delim)
{
        STRNCPY(rec->_internal->delimiter, delim, 32);
}

void csvr_set_standard(struct csv_record* rec, int i)
{
        rec->_internal->standard = (rec->_internal->standard) ? i : 0;
        rec->_internal->allowStdChange = 0;
}

void csvr_set_normal(struct csv_record* rec, int i)
{
        rec->_internal->normal = i;
}

void csvr_set_internalbreak(struct csv_record* rec, const char* internalBreak)
{
        STRNCPY(rec->_internal->inlineBreak, internalBreak, 32);
}

void csvr_lowerstandard(struct csv_record* rec)
{
        if (!rec->_internal->allowStdChange) {
                switch(rec->_internal->standard) {
                case 1:
                        fprintf(stderr,
                                "Line %d: Qualifier issue.\n",
                                1 + rec->_internal->rows + rec->_internal->inlineBreaks);
                        break;
                case 2:
                        fprintf(stderr,
                                "Line %d: RFC4180 Qualifier issue.\n",
                                1 + rec->_internal->rows + rec->_internal->inlineBreaks);
                        break;
                default:
                        fputs("Unexpected Condition.\n", stderr);
                }

                exit(EXIT_FAILURE);
        }

        switch(rec->_internal->standard) {
        case 1:
                fprintf(stderr,
                        "Line %d: Qualifier issue. Quotes disabled.\n",
                        1 + rec->_internal->rows + rec->_internal->inlineBreaks);
                break;
        case 2:
                fprintf(stderr,
                        "Line %d: Qualifier issue. RFC4180 quotes disabled.\n",
                        1 + rec->_internal->rows + rec->_internal->inlineBreaks);
                break;
        default:
                fputs("Unexpected Condition.\n", stderr);
                exit(EXIT_FAILURE);
        }

        --rec->_internal->standard;
        rec->_internal->rows = 0;
        rec->_internal->inlineBreaks = 0;
        fseek(rec->_internal->file, 0, SEEK_SET);
}

void csvr_increasebuffer(struct csv_record* rec)
{
        static int bufferCoef = 1;
        rec->_internal->bufferSize = ++bufferCoef * CSV_BUFFER_FACTOR;
        REALLOC(rec->_internal->buffer, rec->_internal->bufferSize);
}

void csvr_appendlines(struct csv_record* rec)
{
        int offset = 0;
        do {
                offset = rec->_internal->bufferSize;
                csvr_increasebuffer(rec);
                rec->_internal->recordLen = safegetline(rec->_internal->file, rec->_internal->appendBuffer
                                             ,CSV_BUFFER_FACTOR);
                strcat(rec->_internal->buffer, rec->_internal->appendBuffer);
        } while (rec->_internal->recordLen == -2 && rec->_internal->bufferSize < CSV_MAX_RECORD_SIZE);

        if (rec->_internal->recordLen > 0)
                rec->_internal->recordLen += offset;
}

void csvr_getline(struct csv_record* rec)
{
        rec->_internal->recordLen = safegetline(rec->_internal->file, rec->_internal->buffer, rec->_internal->bufferSize);
        if (rec->_internal->recordLen == -2 && rec->_internal->bufferSize < CSV_MAX_RECORD_SIZE)
                csvr_appendlines(rec);
}

//struct csv_field csvr_nextquoted(char** begin)
//{
//        unsigned int delimI = 0;
//        unsigned int onQualifier = 0;
//        unsigned int i = 1;
//
//        struct csv_field field = {NULL, 0};
//
//        while (1) {
//                for (; (*begin)[i] != '\0' && delimI != rec->_internal->delimlen; ++i) {
//                        if (onQualifier) {
//                                if ((*begin)[i] == rec->_internal->delim[delimI]) {
//                                        ++delimI;
//                                }
//                                else if ((*begin)[i] == '"') {
//                                        if(CSVR_STD_QUALIFIERS) {
//                                                if (delimI)
//                                                        return field;
//                                                onQualifier = FALSE;
//                                                removecharat(*begin, i);
//                                        }
//                                }
//                                else if (CSVR_STD_QUALIFIERS) {
//                                        return field;
//                                }
//                                else {
//                                        delimI = 0;
//                                        onQualifier = FALSE;
//                                }
//                        } else {
//                                onQualifier = ((*begin)[i] == '"');
//                        }
//                }
//                if ((*begin)[i] == '\0' && !onQualifier) {
//                        int newLineLen = strlen(rec->_internal->internalBreak);
//                        if (newLineLen + rec->_internal->recordLen >= rec->_internal->bufferSize)
//                                csvr_increasebuffer();
//
//                        strcat(rec->_internal->buffer, rec->_internal->internalBreak);
//                        //int n = rec->_internal->bufferSize - rec->_internal->recordLen - newLineLen;
//                        //rec->_internal->recordLen = safegetline(rec->_internal->file, rec->_internal->appendBuffer, n);
//                        rec->_internal->appendlines();
//                        if (rec->_internal->recordLen == EOF) /** TODO **/
//                                return field;
//                        strcat(rec->_internal->buffer, rec->_internal->appendBuffer);
//                        ++rec->_internal->inlineBreaks;
//                        continue;
//                }
//                break;
//        }
//
//        field.begin = *begin + 1;
//        field.length = (delimI == rec->_internal->delimlen) ? i - 2 - rec->_internal->delimlen : i - 2;
//
//        /* Hack for case when last character is delimiter. */
//        if ((*begin)[i] == '\0' && delimI == rec->_internal->delimlen) {
//                extraField = (extraField) ? 0 : 1;
//                *begin -= extraField;
//        }
//
//        *begin += i;
//
//        return field;
//}

//struct csv_field csvr_nextfield(char** begin)
//{
//        unsigned int delimI = 0;
//        unsigned int i = 0;
//        struct csv_field field = {NULL, 0};
//
//        for (; (*begin)[i] != '\0' && delimI != rec->_internal->delimlen; ++i) {
//                if ((*begin)[i] == rec->_internal->delim[delimI])
//                        ++delimI;
//                else if (delimI != 0)
//                        delimI = ((*begin)[i] == rec->_internal->delim[0]) ? 1 : 0;
//        }
//
//        field.begin = *begin;
//        field.length = (delimI == rec->_internal->delimlen) ? i - rec->_internal->delimlen : i;
//
//        /* Hack for case when last character is delimiter. */
//        if ((*begin)[i] == '\0' && delimI == rec->_internal->delimlen) {
//                extraField = (extraField) ? 0 : 1;
//                *begin -= extraField;
//        }
//
//        *begin += i;
//        return field;
//}

void csvr_determine_delimiter(struct csv_record* rec, const char* header)
{
        const char* delims = ",|\t";
        int i = 0;
        int sel = 0;
        int count = 0;
        int maxCount = 0;
        for (; i < 3; ++i) {
                count = charcount(header, delims[i]);
                if (count > maxCount) {
                        sel = i;
                        maxCount = count;
                }
        }
        //rec->_internal->delim = malloc(2);
        rec->_internal->delimiter[0] = delims[sel];
        rec->_internal->delimiter[1] = '\0';
        rec->_internal->delimLen = 1;
}

int csvr_get_record(struct csv_record* rec)
{
        uint fieldIndex = 0;

        csvr_getline(rec);

        /* This can go away if we get rid of CSV_MAX_RECORD_SIZE */
        if (rec->_internal->recordLen == -2) {
                fprintf(stderr,"Buffer overflow on line %d.\n",
                                rec->_internal->rows + rec->_internal->inlineBreaks);
                exit(EXIT_FAILURE);
        }

        if (rec->_internal->recordLen == EOF) {
                rec->_internal->fieldsAllocated = 0;
                rec->_internal->normal = rec->_internal->normalOrg;
                csvr_destroy(rec);
                return FALSE;
        }

        char* pBuffer = rec->_internal->buffer;

        if (!*rec->_internal->delimiter && !rec->_internal->fieldsAllocated)
                csvr_determine_delimiter(rec, pBuffer);

        while(pBuffer[0] != '\0') {
                if (++(fieldIndex) > rec->_internal->fieldsAllocated)
                        csvr_growrecord(rec);
                if (rec->_internal->standard && pBuffer[0] == '"')
                        rec->fields[fieldIndex-1] = csvr_nextquoted(&pBuffer);
                else
                        rec->fields[fieldIndex-1] = csvr_nextfield(&pBuffer);

                if (!rec->fields[fieldIndex-1].begin) {
                        csvr_lowerstandard(rec);
                        rec->size = CSV_RESET;
                        return TRUE;
                }
        }

        if (rec->_internal->normal > 0) {
                /* Append fields if we are short */
                while (rec->_internal->normal > fieldIndex) {
                        if (++(fieldIndex) > rec->_internal->fieldsAllocated)
                                csvr_growrecord(rec);
                        rec->fields[fieldIndex-1].length = 0;
                }
                fieldIndex = rec->_internal->normal;
        }

        if (rec->_internal->normal == CSVR_NORMAL_OPEN)
                rec->_internal->normal = rec->_internal->fieldsAllocated;

        ++rec->_internal->rows;

        rec->size = fieldIndex;

        return TRUE;
}

int csvr_open(struct csv_record* rec, const char* fileName)
{
        csvr_init(rec);
        rec->_internal->rows = 0;
        if (fileName) {
                rec->_internal->file = fopen(fileName, "r");
                EXIT_IF(!rec->_internal->file, fileName);
        } else {
                rec->_internal->allowStdChange = 0;
                rec->_internal->file = stdin;
                static int ret = 2;
                --ret;
                return ret;
        }
        return 1;
}

void csvr_init(struct csv_record* rec)
{
        MALLOC(rec, sizeof(*rec));
        *rec = (struct csv_record) {
                NULL    /* fields */
                ,0      /* size */
                ,NULL   /* _internal */
        };

        MALLOC(rec->_internal, sizeof(*rec->_internal));
        *rec->_internal = (struct csv_internal) {
                NULL         /* file */
                ,NULL        /* buffer */
                ,""          /* appendBuffer */
                ,"\0"        /* delimiter */
                ,"\n"        /* inlineBreak */
                ,0           /* fieldsAllocated; */
                ,0           /* recordLen; */
                ,0           /* bufferSize; */
                ,1           /* delimLen; */
                ,0           /* rows; */
                ,0           /* inlineBreaks; */
                ,STD_RFC4180 /* standard; */
                ,FALSE       /* allowStdChange; */
                ,0           /* normal; */
                ,0           /* normalOrg; */
                ,0           /* extraField; */
        };

        MALLOC(rec->_internal->buffer, CSV_BUFFER_FACTOR);

        if (!_signalsReady) {
                /** Attach signal handlers **/
                act.sa_mask = vg_shutup;
                act.sa_flags = 0;
                act.sa_handler = cleanexit;
                sigaction(SIGINT, &act, NULL);
                sigaction(SIGQUIT, &act, NULL);
                sigaction(SIGTERM, &act, NULL);
                sigaction(SIGHUP, &act, NULL);

                _signalsReady = TRUE;
        }
}

void csvr_destroy(struct csv_record* rec)
{
        FREE(rec->_internal->buffer);
        FREE(rec->_internal);
        FREE(rec);
}

void csvr_growrecord(struct csv_record* rec)
{
        ++rec->_internal->fieldsAllocated;
        uint arraySize = rec->_internal->fieldsAllocated * sizeof(struct csv_field);
        if (rec->_internal->fieldsAllocated > 1) {
                REALLOC(rec->fields, arraySize);
        } else {
                MALLOC(rec->fields, arraySize);
        }
}

