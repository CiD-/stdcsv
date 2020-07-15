#include "util.h"
#include "safegetline.h"
#include "csv.h"

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
        int fieldsAllocated;
        uint delimLen;

        /* Statistics */
        uint rows;
        uint embedded_breaks;

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
int csv_append_line(struct csv_reader* this, size_t lineIdx, uint nlCount);

/**
 * Simple CSV parsing. Disregard all quotes.
 */
int csv_parse_none(struct csv_reader*, const char** line, size_t* lineIdx);

/**
 * Parse line respecting quotes. Quotes within
 * the field are not expected to be duplicated.
 * Leading spaces will cause the field to not
 * be treated as qualified.
 */
int csv_parse_weak(struct csv_reader*, const char** line, size_t* lineIdx);

/**
 * Parse line according to RFC-4180 guidelines.
 * More info: https://tools.ietf.org/html/rfc4180
 */
int csv_parse_rfc4180(struct csv_reader*, const char** line, size_t* lineIdx);


struct csv_reader* csv_reader_new()
{
        init_sig();

        struct csv_reader* reader = NULL;

        MALLOC(reader, sizeof(*reader));
        *reader = (struct csv_reader) {
                NULL            /* internal */
                ,""             /* delimiter */
                ,"\n"           /* embedded_break */
                ,QUOTE_RFC4180  /* quotes */
                ,0              /* Normal */
                ,FALSE          /* failsafeMode */
                ,FALSE
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
                ,0     /* embedded_breaks */
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

uint csv_reader_embedded_breaks(struct csv_reader* this)
{
        return this->_in->embedded_breaks;
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
                if (csv_reader_close(this) == CSV_FAIL)
                        csv_perror();
                *rec = NULL;
                return ret;
        }

        *rec = csv_parse(this, this->_in->rawBuffer);

        if ((*rec)->size < 0)
                return (*rec)->size;

        return CSV_GOOD;
}

int csv_lowerstandard(struct csv_reader* this)
{
        if (!this->failsafe_mode || this->_in->file == stdin) {
                switch(this->quotes) {
                case QUOTE_ALL:
                case QUOTE_RFC4180:
                        fprintf(stderr,
                                "Line %d: RFC4180 Qualifier issue.\n",
                                1 + this->_in->rows + this->_in->embedded_breaks);
                        break;
                case QUOTE_WEAK:
                        fprintf(stderr,
                                "Line %d: Qualifier issue.\n",
                                1 + this->_in->rows + this->_in->embedded_breaks);
                        break;
                default:
                        fputs("Unexpected Condition.\n", stderr);
                }

                return CSV_FAIL;
        }

        switch(this->quotes) {
        case QUOTE_ALL:
        case QUOTE_RFC4180:
                fprintf(stderr,
                        "Line %d: Qualifier issue. RFC4180 quotes disabled.\n",
                        1 + this->_in->rows + this->_in->embedded_breaks);
                break;
        case QUOTE_WEAK:
                fprintf(stderr,
                        "Line %d: Qualifier issue. Quotes disabled.\n",
                        1 + this->_in->rows + this->_in->embedded_breaks);
                break;
        default:
                fputs("Unexpected Condition.\n", stderr);
                return CSV_FAIL;
        }

        --this->quotes;
        this->_in->rows = 0;
        this->_in->embedded_breaks = 0;
        fseek(this->_in->file, 0, SEEK_SET);

        return CSV_RESET;
}

void csv_append_empty_field(struct csv_reader* this)
{
        struct csv_record* record = this->_in->_record;
        if (++(record->size) > this->_in->fieldsAllocated)
                csv_growrecord(this);

        if (this->_in->bufIdx >= this->_in->bufferSize)
                increase_buffer(&this->_in->buffer,
                                &this->_in->bufferSize);

        this->_in->buffer[this->_in->bufIdx] = '\0';
        record->fields[record->size-1] = &this->_in->buffer[this->_in->bufIdx];
}

struct csv_record* csv_parse(struct csv_reader* this, const char* line)
{
        struct csv_record* record = this->_in->_record;

        if (!this->_in->delimLen)
                csv_determine_delimiter(this, line);

        if (strlen(line) >= this->_in->bufferSize) {
                increase_buffer_to(&this->_in->buffer,
                                   &this->_in->bufferSize,
                                   strlen(line)+1);
        }

        this->_in->bufIdx = 0;
        size_t lineIdx = 0;

        record->size = 0;
        int ret = 0;
        while(line[lineIdx] != '\0') {
                csv_append_empty_field(this);

                int quotes = this->quotes;
                if (this->quotes && line[lineIdx] != '"')
                        quotes = QUOTE_NONE;
                switch(quotes) {
                case QUOTE_ALL:
                        /* Not seeing a point to implementing this for reading. */
                case QUOTE_RFC4180:
                        ret = csv_parse_rfc4180(this, &line, &lineIdx);
                        break;
                case QUOTE_WEAK:
                        ret = csv_parse_weak(this, &line, &lineIdx);
                        break;
                case QUOTE_NONE:
                        ret = csv_parse_none(this, &line, &lineIdx);
                        break;
                }

                if (ret == CSV_RESET) {
                        ret = csv_lowerstandard(this);
                        record->size = ret;
                        csv_reader_reset(this);
                        return record;
                }
        }

        if (this->normal > 0) {
                /* Append fields if we are short */
                while (this->normal > record->size)
                        csv_append_empty_field(this);
                record->size = this->normal;
        }

        if (this->normal == CSV_NORMAL_OPEN)
                this->normal = this->_in->fieldsAllocated;

        ++this->_in->rows;

        return record;
}

int csv_append_line(struct csv_reader* this, size_t lineIdx, uint nlCount)
{
        int ret = sappline(this->_in->file,
                           &this->_in->rawBuffer,
                           &this->_in->rawBufferSize,
                           &this->_in->rawLength);

        uint nlLen = strlen(this->embedded_break);
        uint delta = this->_in->rawLength - lineIdx + (nlLen+1) * nlCount;
        if (delta >= this->_in->bufferSize - this->_in->bufIdx) {
                increase_buffer_to(&this->_in->buffer,
                                   &this->_in->bufferSize,
                                   this->_in->bufferSize + delta);

                /* Move all pointers from _record->fields */
                struct csv_record* rec = this->_in->_record;
                int i = rec->size - 1;
                for (; i >= 0; --i) {
                        int distance = rec->fields[i] - rec->fields[0];
                        rec->fields[i] = this->_in->buffer + distance;
                }
        }

        strcpy(&this->_in->buffer[this->_in->bufIdx], this->embedded_break);
        this->_in->bufIdx += nlLen;

        return ret;
}

int csv_parse_rfc4180(struct csv_reader* this, const char** line, size_t* lineIdx)
{
        uint delimIdx = 0;
        uint lastDelimIdx = 0;
        uint qualified = FALSE;
        uint lastWasQuote = FALSE;
        uint trailingSpace = 0;
        uint nlCount = 0;
        int skip = FALSE;
        int appendField = FALSE;
        int firstChar = TRUE;

        while (TRUE) {
                for (; (*line)[*lineIdx] != '\0' && delimIdx != this->_in->delimLen; ++(*lineIdx)) {
                        skip = FALSE;
                        if (qualified) {
                                qualified = ((*line)[*lineIdx] != '"');
                                if (!qualified) {
                                        skip = TRUE;
                                        lastWasQuote = TRUE;
                                }
                        } else {
                                if ((*line)[*lineIdx] == this->delimiter[delimIdx])
                                        ++delimIdx;
                                else
                                        delimIdx = ((*line)[*lineIdx] == this->delimiter[0]) ? 1 : 0;

                                if ( (qualified = ((*line)[*lineIdx] == '"') ) ) {
                                        if (!lastWasQuote)
                                                skip = TRUE;
                                }
                                lastWasQuote = FALSE;
                        }

                        if (!skip) {
                                if (!firstChar || !this->trim || !isspace((*line)[*lineIdx])) {
                                        this->_in->buffer[this->_in->bufIdx++] = (*line)[*lineIdx];

                                        firstChar = FALSE;
                                        if (this->trim && isspace((*line)[*lineIdx]))
                                                ++trailingSpace;
                                        else if (delimIdx == lastDelimIdx) 
                                                trailingSpace = 0;
                                }
                        }

                        if ((*line)[*lineIdx] == '\n')
                                ++this->_in->embedded_breaks;

                        lastDelimIdx = delimIdx;
                }
                /** In-line break found **/
                if ((*line)[*lineIdx] == '\0' && qualified) {
                        if (++nlCount > CSV_MAX_NEWLINES ||
                            csv_append_line(this, *lineIdx, nlCount) == EOF) {
                                return CSV_RESET;
                        }
                        *line = this->_in->rawBuffer;
                        continue;
                }

                break;
        }

        if (delimIdx == this->_in->delimLen) {
                /* Handle Trailing delimiter */
                if (!((*line)[*lineIdx]))
                        appendField = TRUE;
                this->_in->bufIdx -= this->_in->delimLen;
        }

        this->_in->bufIdx -= trailingSpace;
        this->_in->buffer[this->_in->bufIdx++] = '\0';

        if (appendField)
                csv_append_empty_field(this);

        this->_in->embedded_breaks += nlCount;
        return 0;
}

int csv_parse_weak(struct csv_reader* this, const char** line, size_t* lineIdx)
{
        uint delimIdx = 0;
        uint lastDelimIdx = 0;
        uint onQuote = FALSE;
        uint trailingSpace = 0;
        int nlCount = 0;
        int appendField = FALSE;
        int firstChar = TRUE;

        ++(*lineIdx);

        while (TRUE) {
                for (; (*line)[*lineIdx] != '\0' && delimIdx != this->_in->delimLen; ++(*lineIdx)) {
                        if (onQuote && (*line)[*lineIdx] == this->delimiter[delimIdx]) {
                                ++delimIdx;
                        } else {
                                delimIdx = 0;
                                /* Handle trailing space after quote */
                                //if (onQuote && isspace((*line)[*lineIdx])) {
                                //        ++trailingSpace;
                                //} else {
                                //        trailingSpace = 0;
                                        onQuote = ((*line)[*lineIdx] == '"');
                                //}
                        }
                        if (!firstChar || !this->trim || !isspace((*line)[*lineIdx])) {
                                this->_in->buffer[this->_in->bufIdx++] = (*line)[*lineIdx];

                                firstChar = FALSE;
                                if (this->trim && isspace((*line)[*lineIdx]))
                                        ++trailingSpace;
                                else if (onQuote + delimIdx == lastDelimIdx) 
                                        trailingSpace = 0;
                        }

                        if ((*line)[*lineIdx] == '\n')
                                ++this->_in->embedded_breaks;

                        lastDelimIdx = delimIdx + onQuote;
                }
                if ((*line)[*lineIdx] == '\0' && !onQuote) {
                        if (++nlCount > CSV_MAX_NEWLINES ||
                            csv_append_line(this, *lineIdx, nlCount) == EOF) {
                                return CSV_RESET;
                        }
                        *line = this->_in->rawBuffer;
                        continue;
                }

                break;
        }

        if (delimIdx == this->_in->delimLen) {
                /* Handle Trailing delimiter */
                if (!((*line)[*lineIdx]))
                        appendField = TRUE;
                this->_in->bufIdx -= this->_in->delimLen;
        }
        this->_in->bufIdx -= trailingSpace + 1;
        this->_in->buffer[this->_in->bufIdx++] = '\0';

        if (appendField)
                csv_append_empty_field(this);

        this->_in->embedded_breaks += nlCount;
        return 0;
}

int csv_parse_none(struct csv_reader* this, const char** line, size_t* lineIdx)
{
        uint delimIdx = 0;
        uint lastDelimIdx = 0;
        uint trailingSpace = 0;
        int appendField = FALSE;
        int firstChar = TRUE;

        for (; (*line)[*lineIdx] != '\0' && delimIdx != this->_in->delimLen; ++(*lineIdx)) {
                if ((*line)[*lineIdx] == this->delimiter[delimIdx])
                        ++delimIdx;
                else if (delimIdx != 0)
                        delimIdx = ((*line)[*lineIdx] == this->delimiter[0]) ? 1 : 0;

                if (!firstChar || !this->trim || !isspace((*line)[*lineIdx])) {
                        this->_in->buffer[this->_in->bufIdx++] = (*line)[*lineIdx];

                        firstChar = FALSE;
                        if (this->trim && isspace((*line)[*lineIdx]))
                                ++trailingSpace;
                        else if (delimIdx == lastDelimIdx) 
                                trailingSpace = 0;
                }


                if ((*line)[*lineIdx] == '\n')
                        ++this->_in->embedded_breaks;

                lastDelimIdx = delimIdx;
        }

        if (delimIdx == this->_in->delimLen) {
                /* Handle Trailing delimiter */
                if (!((*line)[*lineIdx]))
                        appendField = TRUE;
                this->_in->bufIdx -= this->_in->delimLen;
        }
        
        this->_in->bufIdx -= trailingSpace;
        this->_in->buffer[this->_in->bufIdx++] = '\0';

        if (appendField)
                csv_append_empty_field(this);

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

int csv_reader_open(struct csv_reader* this, const char* fileName)
{
        this->_in->file = fopen(fileName, "r");
        FAIL_IF(!this->_in->file, fileName);
        return 0;
}

int csv_reader_close(struct csv_reader* this)
{
        if (this->_in->file && this->_in->file != stdin) {
                int ret = fclose(this->_in->file);
                FAIL_IF(ret, "fclose");
        }
        return 0;
}

int csv_reader_reset(struct csv_reader* this)
{
        this->normal = this->_in->normalOrg;
        this->delimiter[0] = '\0';
        this->_in->delimLen = 0;
        this->_in->embedded_breaks = 0;
        this->_in->rows = 0;
        this->_in->buffer[0] = '\0';
        if (this->_in->file && this->_in->file != stdin) {
                int ret = fseek(this->_in->file, 0, SEEK_SET);
                FAIL_IF(ret, "fseek");
        }
        return 0;
}
