#include "util.h"
#include "csv.h"

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
 * csv_determine_delimiter chooses between comma, pipe, semi-colon,
 * colon or tab depending on which  one is found most. If none of
 * these delimiters are found, use comma. If this->delimiter was set
 * externally, simply update this->_in->delimLen and return.
 */
void csv_determine_delimiter(struct csv_reader*, const char* header);

/**
 * csv_growrecord allocates space for the fields
 * member (char**) of the csv_record struct
 */
void csv_growrecord(struct csv_reader*);

/**
 * csv_append_line is used to retrieve another line
 * of input for an in-line break.
 */
int csv_append_line(struct csv_reader* this, uint nlCount);

/**
 * Simple CSV parsing. Disregard all quotes.
 */
int csv_parse_none(struct csv_reader*, const char** line);

/**
 * Parse line respecting quotes. Quotes within
 * the field are not expected to be duplicated.
 * Leading spaces will cause the field to not
 * be treated as qualified.
 */
int csv_parse_weak(struct csv_reader*, const char** line);

/**
 * Parse line according to RFC-4180 guidelines.
 * More info: https://tools.ietf.org/html/rfc4180
 */
int csv_parse_rfc4180(struct csv_reader*, const char** line);


struct csv_reader* csv_reader_new()
{
        init_sig();

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

        return reader;
}

void csv_reader_free(struct csv_reader* this)
{
        FREE(this->_in->buffer);
        FREE(this->_in->rawBuffer);
        FREE(this->_in->_record->fields);
        FREE(this->_in->_record);
        FREE(this->_in);
        FREE(this);
}

/**
 * Simple accessors
 */
uint csv_reader_row_count(struct csv_reader* this)
{
        return this->_in->rows;
}

uint csv_reader_inline_breaks(struct csv_reader* this)
{
        return this->_in->inlineBreaks;
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

int csv_get_record(struct csv_reader* this, struct csv_record** rec)
{
        int ret = sgetline(this->_in->file,
                           &this->_in->rawBuffer,
                           &this->_in->rawBufferSize,
                           &this->_in->rawLength);

        if (ret == EOF) {
                this->_in->fieldsAllocated = 0;
                this->normal = this->_in->normalOrg;
                csv_reader_close(this);
                *rec = NULL;
                return ret;
        }

        *rec = csv_parse(this, this->_in->rawBuffer);

        if ((*rec)->size < 0)
                return (*rec)->size;

        return CSV_GOOD;
}

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

struct csv_record* csv_parse(struct csv_reader* this, const char* line)
{
        struct csv_record* record = this->_in->_record;

        if (!this->_in->delimLen)
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

                if (ret == CSV_RESET) {
                        csv_lowerstandard(this);
                        record->size = CSV_RESET;
                        csv_reader_reset(this);
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

        uint nlLen = strlen(this->inlineBreak);
        if (this->_in->rawLength + nlLen * nlCount >= this->_in->bufferSize) {
               increase_buffer_to(&this->_in->buffer,
                                  &this->_in->bufferSize,
                                  this->_in->rawLength + nlLen * nlCount);
        }

        strcpy(&this->_in->buffer[this->_in->bufIdx], this->inlineBreak);
        this->_in->bufIdx += nlLen;

        return ret;
}

int csv_parse_rfc4180(struct csv_reader* this, const char** line)
{
        uint delimIdx = 0;
        uint qualified = FALSE;
        uint lastWasQuote = FALSE;
        int skip = FALSE;
        uint nlCount = 0;

        while (TRUE) {
                for (; **line != '\0' && delimIdx != this->_in->delimLen; ++(*line)) {
                        skip = FALSE;
                        if (qualified) {
                                qualified = (**line != '"');
                                if (!qualified) {
                                        skip = TRUE;
                                        lastWasQuote = TRUE;
                                }
                        } else {
                                if (**line == this->delimiter[delimIdx])
                                        ++delimIdx;
                                else
                                        delimIdx = (**line == this->delimiter[0]) ? 1 : 0;

                                if ( (qualified = (**line == '"') ) ) {
                                        if (!lastWasQuote)
                                                skip = TRUE;
                                }
                                lastWasQuote = FALSE;
                        }

                        if (!skip)
                                this->_in->buffer[this->_in->bufIdx++] = **line;

                        if (**line == '\n')
                                ++this->_in->inlineBreaks;
                }
                /** In-line break found **/
                if (**line == '\0' && qualified) {
                        if (++nlCount > CSV_MAX_NEWLINES ||
                            csv_append_line(this, nlCount) == EOF) {
                                return CSV_RESET;
                        }
                        continue;
                }

                if (**line)
                        this->_in->bufIdx -= this->_in->delimLen;
                this->_in->buffer[this->_in->bufIdx++] = '\0';

                break;
        }
        this->_in->inlineBreaks += nlCount;
        return 0;
}

int csv_parse_weak(struct csv_reader* this, const char** line)
{
        uint delimIdx = 0;
        uint onQuote = FALSE;
        int nlCount = 0;
        uint trailing = 0;

        ++(*line);

        while (TRUE) {
                for (; **line != '\0' && delimIdx != this->_in->delimLen; ++(*line)) {
                        if (onQuote && **line == this->delimiter[delimIdx]) {
                                ++delimIdx;
                        } else {
                                delimIdx = 0;
                                /* Handle trailing space after quote */
                                if (onQuote && isspace(**line)) {
                                        ++trailing;
                                } else {
                                        trailing = 0;
                                        onQuote = (**line == '"');
                                }
                        }
                        this->_in->buffer[this->_in->bufIdx++] = **line;

                        if (**line == '\n')
                                ++this->_in->inlineBreaks;
                }
                if (**line == '\0' && !onQuote) {
                        if (++nlCount > CSV_MAX_NEWLINES ||
                            csv_append_line(this, nlCount) == EOF) {
                                return CSV_RESET;
                        }
                        continue;
                }

                if (**line)
                        this->_in->bufIdx -= this->_in->delimLen;
                this->_in->bufIdx -= trailing + 1;
                this->_in->buffer[this->_in->bufIdx++] = '\0';

                break;
        }

        this->_in->inlineBreaks += nlCount;
        return 0;
}

int csv_parse_none(struct csv_reader* this, const char** line)
{
        uint delimIdx = 0;

        for (; **line != '\0' && delimIdx != this->_in->delimLen; ++(*line)) {
                if (**line == this->delimiter[delimIdx])
                        ++delimIdx;
                else if (delimIdx != 0)
                        delimIdx = (**line == this->delimiter[0]) ? 1 : 0;

                this->_in->buffer[this->_in->bufIdx++] = **line;

                if (**line == '\n')
                        ++this->_in->inlineBreaks;
        }

        if (**line)
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
        uint i = 0;
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
        this->_in->file = fopen(fileName, "r");
        EXIT_IF(!this->_in->file, fileName);
}

void csv_reader_close(struct csv_reader* this)
{
        if (this->_in->file && this->_in->file != stdin) {
                int ret = fclose(this->_in->file);
                EXIT_IF(ret, "fclose");
        }
}

void csv_reader_reset(struct csv_reader* this)
{
        this->normal = this->_in->normalOrg;
        this->delimiter[0] = '\0';
        this->_in->delimLen = 0;
        this->_in->inlineBreaks = 0;
        this->_in->rows = 0;
        this->_in->buffer[0] = '\0';
        if (this->_in->file && this->_in->file != stdin) {
                int ret = fseek(this->_in->file, 0, SEEK_SET);
                EXIT_IF(ret, "fseek");
        }
}
