#include "util.h"
#include "csv.h"

/* Signal Handlers */
static struct sigaction act;
/* sigset_t and sa_flags only set to shut up valgrind */
static sigset_t vg_shutup = { {0} };
static int _signalsReady = FALSE;
static char blank = '\0';


/**
 * Internal Structure
 */
struct csv_read_internal {
        struct csv_record* _record;
        FILE* file;
        char* rawBuffer;
        char* buffer;
        size_t bufIdx;
        size_t rawBufferSize;
        size_t bufferSize;
        size_t rawLength;
        uint fieldsAllocated;
        uint delimLen;

        /* Statistics */
        uint rows;
        uint inlineBreaks;

        /* Properties */
        int normalOrg;
};


/**
 * Internal prototypes
 */

/**
 * csv_determine_delimiter chooses between comma,
 * pipe or tab depending on which one is found most.
 * If none of these delimiters are found, use comma.
 * If this->delimiter was set externally, simply
 * update this->_in->delimLen and return.
 */
void csv_determine_delimiter(struct csv_reader*, const char* header);

/**
 * csv_growrecord allocates the initial memory for a
 * char*. It also reallocs an additional fields
 * if fields is already allocated a pointer.
 */
void csv_growrecord(struct csv_reader*);

/**
 * csv_getline is just a wrapper for sgetline
 */
//int csv_getline(struct csv_reader*);

/**
 * csv_parse_none takes a pointer to the beginning of
 * a field. *begin is searched for the next delimiter.
 *
 * Returns:
 *      - char* representing parsed field
 *      - NULL on failure
 */
int csv_parse_none(struct csv_reader*, const char** line);

/**
 * csv_parse_weak behaves the same as csv_parse_none
 * except that it expects the next field to be quoted.
 * *begin is searched for a terminating quote and
 * a delimiter.
 *
 * Returns:
 *      - char* representing parsed field
 *      - NULL on failure
 */
int csv_parse_weak(struct csv_reader*, const char** line);

/**
 * csv_parse_rfc4180 takes a pointer to the beginning of
 * a field. *begin is searched for the next delimiter
 * while we are not within text qualification. This is the
 * default quotes and most flexible/ideal.
 *
 * Returns:
 *      - char* representing parsed field
 *      - NULL on failure
 */
int csv_parse_rfc4180(struct csv_reader*, const char** line);


void csv_lowerstandard(struct csv_reader* this)
{
        if (!this->failsafeMode || this->_in->file == stdin) {
                switch(this->quotes) {
                case QUOTE_ALL:
                case QUOTE_RFC4180:
                        fprintf(stderr,
                                "Line %d: RFC4180 Qualifier issue.\n",
                                1 + this->_in->rows + this->_in->inlineBreaks);
                        break;
                case QUOTE_WEAK:
                        fprintf(stderr,
                                "Line %d: Qualifier issue.\n",
                                1 + this->_in->rows + this->_in->inlineBreaks);
                        break;
                default:
                        fputs("Unexpected Condition.\n", stderr);
                }

                exit(EXIT_FAILURE);
        }

        switch(this->quotes) {
        case QUOTE_ALL:
        case QUOTE_RFC4180:
                fprintf(stderr,
                        "Line %d: Qualifier issue. RFC4180 quotes disabled.\n",
                        1 + this->_in->rows + this->_in->inlineBreaks);
                break;
        case QUOTE_WEAK:
                fprintf(stderr,
                        "Line %d: Qualifier issue. Quotes disabled.\n",
                        1 + this->_in->rows + this->_in->inlineBreaks);
                break;
        default:
                fputs("Unexpected Condition.\n", stderr);
                exit(EXIT_FAILURE);
        }

        --this->quotes;
        this->_in->rows = 0;
        this->_in->inlineBreaks = 0;
        fseek(this->_in->file, 0, SEEK_SET);
}

struct csv_record* csv_get_record(struct csv_reader* this)
{
        int ret = sgetline(this->_in->file,
                           &this->_in->rawBuffer,
                           &this->_in->rawBufferSize,
                           &this->_in->rawLength);


        if (ret == EOF) {
                this->_in->fieldsAllocated = 0;
                this->normal = this->_in->normalOrg;
                return NULL;
        }

        return csv_parse(this, this->_in->buffer);
}

struct csv_record* csv_parse(struct csv_reader* this, const char* line)
{
        struct csv_record* record = this->_in->_record;
        if (!this->_in->fieldsAllocated && !this->_in->delimLen)
                csv_determine_delimiter(this, line);

        if (strlen(line) >= this->_in->bufferSize) {
                increase_buffer_to(&this->_in->buffer,
                                   &this->_in->bufferSize,
                                   strlen(line));
        }

        this->_in->bufIdx = 0;

        uint fieldIndex = 0;
        int ret = 0;
        while(line[0] != '\0') {
                if (fieldIndex)
                        line += this->_in->delimLen;
                if (++(fieldIndex) > this->_in->fieldsAllocated)
                        csv_growrecord(this);

                record->fields[fieldIndex-1] = &this->_in->buffer[this->_in->bufIdx];

                switch(this->quotes) {
                case QUOTE_ALL:
                        /* Not seeing a point to implementing this for reading. */
                case QUOTE_RFC4180:
                        ret = csv_parse_rfc4180(this, &line);
                        break;
                case QUOTE_WEAK:
                        if (this->quotes && line[0] == '"') { /** Implicit Fallthrough **/
                                ret = csv_parse_weak(this, &line);
                                break;
                        }
                case QUOTE_NONE:
                        ret = csv_parse_none(this, &line);
                        break;
                }

                if (ret == EOF) {
                        /** lowerstandard will exit outside of failsafe mode **/
                        csv_lowerstandard(this);
                        record->size = CSV_RESET;
                        return record;
                }
        }

        if (this->normal > 0) {
                /* Append fields if we are short */
                while ((uint) this->normal > fieldIndex) {
                        if (++(fieldIndex) > this->_in->fieldsAllocated)
                                csv_growrecord(this);
                        record->fields[fieldIndex-1] = &blank;
                }
                fieldIndex = this->normal;
        }

        if (this->normal == CSV_NORMAL_OPEN)
                this->normal = this->_in->fieldsAllocated;

        ++this->_in->rows;
        record->size = fieldIndex;

        return record;
}

int csv_append_line(struct csv_reader* this, uint nlCount)
{
        int ret = sappline(this->_in->file,
                           &this->_in->rawBuffer,
                           &this->_in->rawBufferSize,
                           &this->_in->rawLength);
        ++this->_in->inlineBreaks;

        uint nlLen = strlen(this->inlineBreak);
        if (this->_in->rawLength + nlLen * nlCount >= this->_in->bufferSize) {
               increase_buffer_to(&this->_in->buffer,
                                  &this->_in->bufferSize,
                                  this->_in->rawLength + nlLen * nlCount);
        }

        strcat(this->_in->buffer, this->inlineBreak);
        this->_in->bufIdx += nlLen;

        return ret;
}

int csv_parse_rfc4180(struct csv_reader* this, const char** line)
{
        uint delimI = 0;
        uint qualified = FALSE;
        uint lastWasQuote = FALSE;
        int skip = FALSE;
        uint nlCount = 0;

        while (TRUE) {
                for (; **line != '\0' && delimI != this->_in->delimLen; ++(*line)) {
                        skip = FALSE;
                        if (qualified) {
                                qualified = (**line != '"');
                                if (!qualified) {
                                        skip = TRUE;
                                        lastWasQuote = TRUE;
                                }
                        } else {
                                if (**line == this->delimiter[delimI])
                                        ++delimI;
                                else
                                        delimI = (**line == this->delimiter[0]) ? 1 : 0;

                                if ( (qualified = (**line == '"') ) ) {
                                        if (!lastWasQuote)
                                                skip = TRUE;
                                }
                                lastWasQuote = FALSE;
                        }

                        if (!skip)
                                this->_in->buffer[this->_in->bufIdx++] = **line;
                }
                /** In-line break found **/
                if (**line == '\0' && qualified) {
                        if (++nlCount > CSV_MAX_NEWLINES ||
                            csv_append_line(this, nlCount) == EOF) {
                                return EOF;
                        }
                        continue;
                }

                if (!**line)
                        this->_in->bufIdx -= this->_in->delimLen;
                this->_in->buffer[this->_in->bufIdx++] = '\0';

                break;
        }
        return 0;
}

int csv_parse_weak(struct csv_reader* this, const char** line)
{
        uint delimI = 0;
        uint onQuote = FALSE;
        int nlCount = 0;

        while (TRUE) {
                for (; **line != '\0' && delimI != this->_in->delimLen; ++(*line)) {
                        if (onQuote && **line == this->delimiter[delimI]) {
                                ++delimI;
                        } else {
                                delimI = 0;
                                onQuote = (**line == '"');
                        }
                        this->_in->buffer[this->_in->bufIdx++] = **line;
                }
                if (**line == '\0' && !onQuote) {
                        if (++nlCount > CSV_MAX_NEWLINES || 
                            csv_append_line(this, nlCount) == EOF) {
                                return EOF;
                        }
                        continue;
                }

                if (!**line)
                        this->_in->bufIdx -= this->_in->delimLen - 1;
                this->_in->buffer[this->_in->bufIdx++] = '\0';

                break;
        }

        return 0;
}

int csv_parse_none(struct csv_reader* this, const char** line)
{
        uint delimI = 0;

        for (; **line != '\0' && delimI != this->_in->delimLen; ++(*line)) {
                if (**line == this->delimiter[delimI])
                        ++delimI;
                else if (delimI != 0)
                        delimI = (**line == this->delimiter[0]) ? 1 : 0;

                this->_in->buffer[this->_in->bufIdx++] = **line;
        }

        if (!**line)
                this->_in->bufIdx -= this->_in->delimLen;
        this->_in->buffer[this->_in->bufIdx++] = '\0';

        return 0;
}

void csv_determine_delimiter(struct csv_reader* this, const char* header)
{
        uint delimLen = strlen(this->delimiter);
        if (delimLen) {
                this->_in->delimLen = delimLen;
                return;
        }

        const char* delims = ",|\t;:";
        int i = 0;
        int sel = 0;
        int count = 0;
        int maxCount = 0;
        for (; i < strlen(delims); ++i) {
                count = charcount(header, delims[i]);
                if (count > maxCount) {
                        sel = i;
                        maxCount = count;
                }
        }

        this->delimiter[0] = delims[sel];
        this->delimiter[1] = '\0';
        this->_in->delimLen = 1;
}

void csv_reader_open(struct csv_reader* this, const char* fileName)
{
        if (fileName) {
                this->_in->file = fopen(fileName, "r");
                EXIT_IF(!this->_in->file, fileName);
        } else {
                this->_in->file = stdin;
        }
}

struct csv_reader* csv_reader_new()
{
        struct csv_reader* reader = NULL;

        MALLOC(reader, sizeof(*reader));
        *reader = (struct csv_reader) {
                NULL            /* internal */
                ,""             /* delimiter */
                ,"\n"           /* inlineBreak */
                ,QUOTE_RFC4180  /* quotes */
                ,0              /* Normal */
                ,FALSE          /* failsafeMode */
        };

        MALLOC(reader->_in, sizeof(*reader->_in));
        *reader->_in = (struct csv_read_internal) {
                NULL   /* internal record */
                ,stdin /* file */
                ,NULL  /* rawBuffer */
                ,NULL  /* buffer */
                ,0     /* bufIdx */
                ,0     /* rawBufferSize */
                ,0     /* bufferSize */
                ,0     /* rawLength */
                ,0     /* fieldsAllocated; */
                ,0     /* delimLen */
                ,0     /* rows */
                ,0     /* inlineBreaks */
                ,0     /* normalOrg */
        };

        increase_buffer(&reader->_in->buffer, &reader->_in->bufferSize);
        MALLOC(reader->_in->_record, sizeof(struct csv_record));

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

        //csv_growrecord(reader);

        return reader;
}

void csv_growrecord(struct csv_reader* this)
{
        ++this->_in->fieldsAllocated;
        uint arraySize = this->_in->fieldsAllocated * sizeof(char*);
        if (this->_in->fieldsAllocated > 1) {
                REALLOC(this->_in->_record->fields, arraySize);
        } else {
                MALLOC(this->_in->_record->fields, arraySize);
        }
}

void csv_reader_free(struct csv_reader* this)
{
        FREE(this->_in->buffer);
        FREE(this->_in->_record);
        FREE(this->_in);
        FREE(this);
}

