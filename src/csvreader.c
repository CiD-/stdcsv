#include "csvrw.h"

static FILE* csvr_file = NULL;
static char* csvr_fileName = NULL;
static char* csvr_buffer = NULL;
static char* csvr_append = NULL;
static char* csvr_delim = NULL;
static char* csvr_internalBreak = NULL;
static int csvr_recordLen = 0;
static int csvr_bufferSize = CSV_BUFFER_FACTOR;
static unsigned int csvr_delimlen = 0;

/* Statistics */
static unsigned int csvr_rows = 0;
static unsigned int csvr_inLineBreaks = 0;
static int csvr_fieldCount = 0;

/* Flags */
static int csvr_qualifiers = 2;
static int csvr_allowStdChange = 1;
static int csvr_normal = 0;
static int csvr_normal_org = 0;

/* Extra field hack */
static int extraField = 0;

/* Signal Handlers */
static struct sigaction act;
/* sigset_t and sa_flags only set to shut up valgrind */
static sigset_t vg_shutup = { {0} };



/**
 * Internal prototypes
 */
void csvr_getdelimiter(const char* header);

char* csvr_get_filename()
{
        return csvr_fileName;
}

char* csvr_get_delim()
{
        return csvr_delim;
}

int csvr_get_allowstdchange()
{
        return csvr_allowStdChange;
}

void csvr_set_delim(const char* delim)
{
        STRDUP(delim, csvr_delim);
        csvr_delimlen = strlen(csvr_delim);
}

void csvr_set_qualifiers(int i)
{
        csvr_qualifiers = (csvr_qualifiers) ? i : 0;
        csvr_allowStdChange = 0;
}

void csvr_set_normal(int i)
{
        csvr_normal = i;
}

void csvr_set_internalbreak(const char* internalBreak)
{
        STRDUP(internalBreak, csvr_internalBreak);
}

void csvr_lowerstandard()
{
        if (!csvr_allowStdChange) {
                switch(csvr_qualifiers) {
                case 1:
                        fprintf(stderr,
                                "Line %d: Qualifier issue."
                                "Try option --ignore-quotes \n",
                                1 + csvr_rows + csvr_inLineBreaks);
                        break;
                case 2:
                        fprintf(stderr,
                                "Line %d: RFC4180 Qualifier issue."
                                "Try option --no-rfc4180-in\n",
                                1 + csvr_rows + csvr_inLineBreaks);
                        break;
                default:
                        fputs("Unexpected Condition.\n", stderr);
                }

                exit(EXIT_FAILURE);
        }

        switch(csvr_qualifiers) {
        case 1:
                fprintf(stderr,
                        "Line %d: Qualifier issue. Quotes disabled.\n",
                        1 + csvr_rows + csvr_inLineBreaks);
                break;
        case 2:
                fprintf(stderr,
                        "Line %d: Qualifier issue. RFC4180 quotes disabled.\n",
                        1 + csvr_rows + csvr_inLineBreaks);
                break;
        default:
                fputs("Unexpected Condition.\n", stderr);
                exit(EXIT_FAILURE);
        }

        --csvr_qualifiers;
        csvr_rows = 0;
        csvr_inLineBreaks = 0;
        fseek(csvr_file, 0, SEEK_SET);
}

struct csv_field csvr_nextquoted(char** begin)
{
        unsigned int delimI = 0;
        unsigned int quoteI = 0;
        unsigned int i = 1;

        struct csv_field field = {NULL, 0};

        while (1) {
                for (; (*begin)[i] != '\0' && delimI != csvr_delimlen; ++i) {
                        if (quoteI) {
                                if ((*begin)[i] == csvr_delim[delimI]) {
                                        ++delimI;
                                }
                                else if ((*begin)[i] == '"') {
                                        if(CSVR_STD_QUALIFIERS) {
                                                if (delimI)
                                                        return field;
                                                quoteI = 0;
                                                removecharat(*begin, i);
                                        }
                                }
                                else if (CSVR_STD_QUALIFIERS) {
                                        return field;
                                }
                                else {
                                        delimI = 0;
                                        quoteI = 0;
                                }
                        } else {
                                quoteI = ((*begin)[i] == '"') ? 1 : 0;
                        }
                }
                if ((*begin)[i] == '\0' && !quoteI) {
                        int newLineLen = strlen(csvr_internalBreak);
                        int n = csvr_bufferSize - csvr_recordLen - newLineLen;
                        csvr_recordLen = safegetline(csvr_file, csvr_append, n);
                        if (csvr_recordLen == EOF || csvr_recordLen == -2)
                                return field;
                        strcat(csvr_buffer, csvr_internalBreak);
                        strcat(csvr_buffer, csvr_append);
                        ++csvr_inLineBreaks;
                        continue;
                }
                break;
        }

        field.begin = *begin + 1;
        field.size = (delimI == csvr_delimlen) ? i - 2 - csvr_delimlen : i - 2;

        /* Hack for case when last character is delimiter. */
        if ((*begin)[i] == '\0' && delimI == csvr_delimlen) {
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

        for (; (*begin)[i] != '\0' && delimI != csvr_delimlen; ++i) {
                if ((*begin)[i] == csvr_delim[delimI])
                        ++delimI;
                else if (delimI != 0)
                        delimI = ((*begin)[i] == csvr_delim[0]) ? 1 : 0;
        }

        field.begin = *begin;
        field.size = (delimI == csvr_delimlen) ? i - csvr_delimlen : i;

        /* Hack for case when last character is delimiter. */
        if ((*begin)[i] == '\0' && delimI == csvr_delimlen) {
                extraField = (extraField) ? 0 : 1;
                *begin -= extraField;
        }

        *begin += i;
        return field;
}

void csvr_getdelimiter(const char* header)
{
        char* delims = ",|\t";
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
        csvr_delim = malloc(2);
        csvr_delim[0] = delims[sel];
        csvr_delim[1] = '\0';
        csvr_delimlen = 1;
}

int csvr_open(const char* fileName)
{
        //EXIT_IF(fclose(csvr_file) == EOF, NULL);
        csvr_rows = 0;
        if (fileName) {
                csvr_file = fopen(fileName, "r");
                EXIT_IF(!csvr_file, fileName);
        } else {
                csvr_allowStdChange = 0;
                csvr_file = stdin;
                static int ret = 2;
                --ret;
                return ret;
        }
        return 1;
}

void csvr_getline()
{
        static int bufferCoef = 1;
        int len = 0;

        csvr_recordLen = safegetline(csvr_file, csvr_buffer, csvr_bufferSize);

        while (csvr_recordLen == -2 && csvr_bufferSize < CSV_MAX_RECORD_SIZE) {
                len = csvr_bufferSize;
                csvr_bufferSize = ++bufferCoef * CSV_BUFFER_FACTOR;
                csvr_buffer = realloc(csvr_buffer, csvr_bufferSize);
                EXIT_IF(!csvr_buffer, "Buffer realloc");
                csvr_append = realloc(csvr_append, csvr_bufferSize);
                EXIT_IF(!csvr_buffer, "Append realloc");

                csvr_recordLen = safegetline(csvr_file, csvr_append
                                             ,CSV_BUFFER_FACTOR);
                strcat(csvr_buffer, csvr_append);
        }

        if (csvr_recordLen > 0)
                csvr_recordLen += len;
}

struct csv_field* csvr_getfields(struct csv_field* fields, int* count)
{
        if (!csvr_buffer)
                csvr_init();

        *count = 0;

        csvr_getline();

        /* This can go away if we get rid of CSV_MAX_RECORD_SIZE */
        if (csvr_recordLen == -2) {
                fprintf(stderr,"Buffer overflow on line %d.\n",
                                csvr_rows + csvr_inLineBreaks);
                exit(EXIT_FAILURE);
        }

        if (csvr_recordLen == EOF) {
                csvr_fieldCount = 0;
                csvr_normal = csvr_normal_org;
                csvr_destroy();
                return NULL;
        }

        char* pBuffer = csvr_buffer;

        if (!csvr_delim && !csvr_fieldCount)
                csvr_getdelimiter(pBuffer);

        while(pBuffer[0] != '\0') {
                if (++(*count) > csvr_fieldCount)
                        fields = csvr_growfields(fields);
                if (csvr_qualifiers && pBuffer[0] == '"')
                        fields[*count-1] = csvr_nextquoted(&pBuffer);
                else
                        fields[*count-1] = csvr_nextfield(&pBuffer);

                if (!fields[*count-1].begin) {
                        csvr_lowerstandard();
                        *count = CSV_RESET;
                        return fields;
                }
        }

        if (csvr_normal > 0) {
                /* Append fields if we are short */
                while (csvr_normal > *count) {
                        if (++(*count) > csvr_fieldCount)
                                fields = csvr_growfields(fields);
                        fields[*count-1].size = 0;
                }
                *count = csvr_normal;
        }

        if (csvr_normal == CSVR_NORMAL_OPEN)
                csvr_normal = csvr_fieldCount;

        ++csvr_rows;

        return fields;
}

void csvr_init()
{
        csvr_buffer = malloc(CSV_BUFFER_FACTOR);
        EXIT_IF(!csvr_buffer, "csvr_buffer malloc");

        csvr_append = malloc(CSV_BUFFER_FACTOR);
        EXIT_IF(!csvr_append, "csvr_append malloc");

        if (!csvr_internalBreak)
                STRDUP("\n", csvr_internalBreak);

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

void csvr_destroy()
{
        FREE(csvr_delim);
        FREE(csvr_internalBreak);
        FREE(csvr_buffer);
        FREE(csvr_append);
}
