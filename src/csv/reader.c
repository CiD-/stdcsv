#include "csv.h"

//static FILE* csvr_file = NULL;
//static char* csvr_buffer = NULL;
//static char csvr_append[CSV_BUFFER_FACTOR];
//static char csvr_delim[32] = "\0";
//static char csvr_internalBreak[32] = "\n";
//static int csvr_recordLen = 0;
//static int csvr_bufferSize = CSV_BUFFER_FACTOR;
//static unsigned int csvr_delimlen = 0;
//
///* Statistics */
//static unsigned int csvr_rows = 0;
//static unsigned int csvr_inLineBreaks = 0;
//
///* Flags */
//static int csvr_qualifiers = 2;
//static int csvr_allowStdChange = 1;
//static int csvr_normal = 0;
//static int csvr_normal_org = 0;
//
///* Extra field hack */
//static int extraField = 0;

/* Signal Handlers */
static struct sigaction act;
/* sigset_t and sa_flags only set to shut up valgrind */
static sigset_t vg_shutup = { {0} };


/**
 * Internal Structure
 */
struct csv_internal {
        FILE* file;
        char* buffer;
        char appendBuffer[CSV_BUFFER_FACTOR];
        char delimiter[32];
        char inlineBreak[32];
        int recordLen;
        int bufferSize;
        uint delimLen;

        /* Statistics */
        uint rows;
        uint inlineBreaks;

        /* Flags */
        int qualifiers;
        int allowStdChange;
        int normal;
        int normalOrg;

        int extraField;
};


/**
 * Internal prototypes
 */
void csvr_getdelimiter(const char* header);

char* csvr_get_delim()
{
        return rec->internal->delim;
}

int csvr_get_allowstdchange()
{
        return rec->internal->allowStdChange;
}

void csvr_set_delim(const char* delim)
{
        STRNCPY(rec->internal->delim, delim, 32);
}

void csvr_set_qualifiers(int i)
{
        rec->internal->qualifiers = (rec->internal->qualifiers) ? i : 0;
        rec->internal->allowStdChange = 0;
}

void csvr_set_normal(int i)
{
        rec->internal->normal = i;
}

void csvr_set_internalbreak(const char* internalBreak)
{
        STRNCPY(rec->internal->internalBreak, internalBreak, 32);
}

void csvr_lowerstandard()
{
        if (!rec->internal->allowStdChange) {
                switch(rec->internal->qualifiers) {
                case 1:
                        fprintf(stderr,
                                "Line %d: Qualifier issue.\n",
                                1 + rec->internal->rows + rec->internal->inLineBreaks);
                        break;
                case 2:
                        fprintf(stderr,
                                "Line %d: RFC4180 Qualifier issue.\n",
                                1 + rec->internal->rows + rec->internal->inLineBreaks);
                        break;
                default:
                        fputs("Unexpected Condition.\n", stderr);
                }

                exit(EXIT_FAILURE);
        }

        switch(rec->internal->qualifiers) {
        case 1:
                fprintf(stderr,
                        "Line %d: Qualifier issue. Quotes disabled.\n",
                        1 + rec->internal->rows + rec->internal->inLineBreaks);
                break;
        case 2:
                fprintf(stderr,
                        "Line %d: Qualifier issue. RFC4180 quotes disabled.\n",
                        1 + rec->internal->rows + rec->internal->inLineBreaks);
                break;
        default:
                fputs("Unexpected Condition.\n", stderr);
                exit(EXIT_FAILURE);
        }

        --rec->internal->qualifiers;
        rec->internal->rows = 0;
        rec->internal->inLineBreaks = 0;
        fseek(rec->internal->file, 0, SEEK_SET);
}

void csvr_increasebuffer()
{
        static int bufferCoef = 1;
        rec->internal->bufferSize = ++bufferCoef * CSV_BUFFER_FACTOR;
        REALLOC(rec->internal->buffer, rec->internal->bufferSize);
}

void csvr_appendlines()
{
        int offset = 0;
        do {
                offset = rec->internal->bufferSize;
                rec->internal->increasebuffer();
                rec->internal->recordLen = safegetline(rec->internal->file, rec->internal->append
                                             ,CSV_BUFFER_FACTOR);
                strcat(rec->internal->buffer, rec->internal->append);
        } while (rec->internal->recordLen == -2 && rec->internal->bufferSize < CSV_MAX_RECORD_SIZE);

        if (rec->internal->recordLen > 0)
                rec->internal->recordLen += offset;
}

void csvr_getline()
{
        rec->internal->recordLen = safegetline(rec->internal->file, rec->internal->buffer, rec->internal->bufferSize);
        if (rec->internal->recordLen == -2 && rec->internal->bufferSize < CSV_MAX_RECORD_SIZE)
                rec->internal->appendlines();
}

struct csv_field csvr_nextquoted(char** begin)
{
        unsigned int delimI = 0;
        unsigned int onQualifier = 0;
        unsigned int i = 1;

        struct csv_field field = {NULL, 0};

        while (1) {
                for (; (*begin)[i] != '\0' && delimI != rec->internal->delimlen; ++i) {
                        if (onQualifier) {
                                if ((*begin)[i] == rec->internal->delim[delimI]) {
                                        ++delimI;
                                }
                                else if ((*begin)[i] == '"') {
                                        if(CSVR_STD_QUALIFIERS) {
                                                if (delimI)
                                                        return field;
                                                onQualifier = FALSE;
                                                removecharat(*begin, i);
                                        }
                                }
                                else if (CSVR_STD_QUALIFIERS) {
                                        return field;
                                }
                                else {
                                        delimI = 0;
                                        onQualifier = FALSE;
                                }
                        } else {
                                onQualifier = ((*begin)[i] == '"');
                        }
                }
                if ((*begin)[i] == '\0' && !onQualifier) {
                        int newLineLen = strlen(rec->internal->internalBreak);
                        if (newLineLen + rec->internal->recordLen >= rec->internal->bufferSize)
                                csvr_increasebuffer();

                        strcat(rec->internal->buffer, rec->internal->internalBreak);
                        //int n = rec->internal->bufferSize - rec->internal->recordLen - newLineLen;
                        //rec->internal->recordLen = safegetline(rec->internal->file, rec->internal->append, n);
                        rec->internal->appendlines();
                        if (rec->internal->recordLen == EOF) /** TODO **/
                                return field;
                        strcat(rec->internal->buffer, rec->internal->append);
                        ++rec->internal->inLineBreaks;
                        continue;
                }
                break;
        }

        field.begin = *begin + 1;
        field.length = (delimI == rec->internal->delimlen) ? i - 2 - rec->internal->delimlen : i - 2;

        /* Hack for case when last character is delimiter. */
        if ((*begin)[i] == '\0' && delimI == rec->internal->delimlen) {
                extraField = (extraField) ? 0 : 1;
                *begin -= extraField;
        }

        *begin += i;

        return field;
}

struct csv_field csvr_nextfield(char** begin)
{
        unsigned int delimI = 0;
        unsigned int i = 0;
        struct csv_field field = {NULL, 0};

        for (; (*begin)[i] != '\0' && delimI != rec->internal->delimlen; ++i) {
                if ((*begin)[i] == rec->internal->delim[delimI])
                        ++delimI;
                else if (delimI != 0)
                        delimI = ((*begin)[i] == rec->internal->delim[0]) ? 1 : 0;
        }

        field.begin = *begin;
        field.length = (delimI == rec->internal->delimlen) ? i - rec->internal->delimlen : i;

        /* Hack for case when last character is delimiter. */
        if ((*begin)[i] == '\0' && delimI == rec->internal->delimlen) {
                extraField = (extraField) ? 0 : 1;
                *begin -= extraField;
        }

        *begin += i;
        return field;
}

void csvr_getdelimiter(const char* header)
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
        //rec->internal->delim = malloc(2);
        rec->internal->delim[0] = delims[sel];
        rec->internal->delim[1] = '\0';
        rec->internal->delimlen = 1;
}

int csvr_get_record(struct csv_record* rec)
{
        int fieldIndex = 0;

        rec->internal->getline();

        /* This can go away if we get rid of CSV_MAX_RECORD_SIZE */
        if (rec->internal->recordLen == -2) {
                fprintf(stderr,"Buffer overflow on line %d.\n",
                                rec->internal->rows + rec->internal->inLineBreaks);
                exit(EXIT_FAILURE);
        }

        if (rec->internal->recordLen == EOF) {
                rec->_allocated_size = 0;
                rec->internal->normal = rec->internal->normal_org;
                csvr_destroy();
                return FALSE;
        }

        char* pBuffer = rec->internal->buffer;

        if (!*rec->internal->delim && !rec->_allocated_size)
                csvr_getdelimiter(pBuffer);

        while(pBuffer[0] != '\0') {
                if (++(fieldIndex) > rec->_allocated_size)
                        csvr_growrecord(rec);
                if (rec->internal->qualifiers && pBuffer[0] == '"')
                        rec->fields[fieldIndex-1] = csvr_nextquoted(&pBuffer);
                else
                        rec->fields[fieldIndex-1] = csvr_nextfield(&pBuffer);

                if (!rec->fields[fieldIndex-1].begin) {
                        csvr_lowerstandard();
                        rec->size = CSV_RESET;
                        return TRUE;
                }
        }

        if (rec->internal->normal > 0) {
                /* Append fields if we are short */
                while (rec->internal->normal > fieldIndex) {
                        if (++(fieldIndex) > rec->_allocated_size)
                                csvr_growrecord(rec);
                        rec->fields[fieldIndex-1].length = 0;
                }
                fieldIndex = rec->internal->normal;
        }

        if (rec->internal->normal == CSVR_NORMAL_OPEN)
                rec->internal->normal = rec->_allocated_size;

        ++rec->internal->rows;

        rec->size = fieldIndex;

        return TRUE;
}

int csvr_open(const char* fileName)
{
        //EXIT_IF(fclose(rec->internal->file) == EOF, NULL);
        rec->internal->rows = 0;
        if (fileName) {
                rec->internal->file = fopen(fileName, "r");
                EXIT_IF(!rec->internal->file, fileName);
        } else {
                rec->internal->allowStdChange = 0;
                rec->internal->file = stdin;
                static int ret = 2;
                --ret;
                return ret;
        }
        return 1;
}

void csvr_init(const char* fileName)
{
        if (fileName)
                rec->internal->open(fileName);

        MALLOC(rec->internal->buffer, CSV_BUFFER_FACTOR);

        /** Attach signal handlers **/
        act.sa_mask = vg_shutup;
        act.sa_flags = 0;
        act.sa_handler = cleanexit;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGQUIT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        sigaction(SIGHUP, &act, NULL);
        //sigaction(SIGKILL, &act, NULL);
}

void csvr_destroy(struct csvr_record* rec)
{
        FREE(rec->internal->buffer);
}

void csvr_growrecord(struct csv_record* rec)
{
        ++rec->_internal->allocatedSize;
        uint arraySize = rec->_allocated_size * sizeof(struct csv_field);
        if (rec->_allocated_size > 1) {
                REALLOC(rec->fields, arraySize);
        } else {
                MALLOC(rec->fields, arraySize);
        }
}

