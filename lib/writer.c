#include "csv.h"

static char csv_tempdir[PATH_MAX-10] = "";

/**
 * Internal Prototypes
 */



/**
 * Internal Structure
 */
struct csv_write_internal {
        //struct csv_record* _record;
        char tempname[PATH_MAX];
        char filename[PATH_MAX];
        char filename_org[PATH_MAX];
        FILE* file;
        char* buffer;

        uint delimLen;
        int bufferSize;
        int recordLen;
};


void csv_writer_reset(struct csv_writer *writer)
{
        if (writer->_internal->filename[0]) {
                STRNCPY(writer->_internal->filename, writer->_internal->filename_org, PATH_MAX);
        }

        EXIT_IF(fclose(writer->_internal->file) == EOF, writer->_internal->tempname);
        writer->_internal->file = NULL;

        writer->_internal->file = fopen(writer->_internal->tempname, "w");
        EXIT_IF(!writer->_internal->file, writer->_internal->tempname);
}

void csv_write_record(struct csv_writer* writer, struct csv_record* rec)
{
        int i = 0;
        unsigned int j = 0;
        int writeIndex = 0;
        int quoteCurrentField = 0;
        uint delimI = 0;
        char c = 0;

        for (i = 0; i < rec->size; ++i) {
                if (i)
                        fputs(writer->delimiter, writer->_internal->file);

                quoteCurrentField = (writer->quotes == QUOTE_ALL);
                writeIndex = 0;
                for (j = 0; j < rec->fields[i].length; ++j) {
                        c = rec->fields[i].begin[j];
                        writer->_internal->buffer[writeIndex++] = c;
                        if (c == '"' && writer->quotes >= QUOTE_RFC4180) {
                                writer->_internal->buffer[writeIndex++] = '"';
                                quoteCurrentField = TRUE;
                        }
                        if (writer->quotes && !quoteCurrentField) {
                                if (strhaschar("\"\n\r", c))
                                        quoteCurrentField = 1;
                                else if (c == writer->delimiter[delimI])
                                        ++delimI;
                                else
                                        delimI = (c == writer->delimiter[0]) ? 1 : 0;

                                if (delimI == writer->_internal->delimLen)
                                        quoteCurrentField = TRUE;
                        }
                }
                writer->_internal->buffer[writeIndex] = '\0';
                if (quoteCurrentField)
                        fprintf(writer->_internal->file, "\"%s\"", writer->_internal->buffer);
                else
                        fputs(writer->_internal->buffer, writer->_internal->file);
        }
        fputs(writer->lineEnding, writer->_internal->file);
}

//void csvw_update_filename(struct csv_writer* writer)
//{
//        char indexStr[12];
//        snprintf(indexStr, 12, "%d", ++writer->_internal->fileIndex);
//
//        char* extension = getext(writer->_internal->filename);
//        char* noExtension = getnoext(writer->filename_org);
//
//        strcpy(writer->_internal->filename, noExtension);
//        strcat(writer->_internal->filename, indexStr);
//        strcat(writer->_internal->filename, extension);
//
//        free(extension);
//        free(noExtension);
//}

void csv_writer_close(struct csv_writer* writer)
{
        if (writer->_internal->file == stdout)
                return;

        EXIT_IF(fclose(writer->_internal->file) == EOF, writer->_internal->tempname);
        writer->_internal->file = NULL;

        if (writer->_internal->filename[0]) {
                int ret = rename(writer->_internal->tempname, writer->_internal->filename);
                EXIT_IF(ret, writer->_internal->tempname);
                //csvw_update_filename();
        } else {
                FILE* dumpFile = fopen(writer->_internal->tempname, "r");
                EXIT_IF(!dumpFile, writer->_internal->tempname);

                char c = '\0';
                while ((c = getc(dumpFile)) != EOF)
                        putchar(c);

                EXIT_IF(fclose(dumpFile) == EOF, writer->_internal->tempname);
                cleanoutputfile();
        }
}

struct csv_writer* csv_new_writer()
{
        struct csv_writer* writer = NULL;

        MALLOC(writer, sizeof(*writer));
        *writer = (struct csv_writer) {
                NULL
                //,""
                ,","
                ,"\n"
                ,QUOTE_RFC4180
        };

        MALLOC(writer->_internal, sizeof(*writer->_internal));
        *writer->_internal = (struct csv_write_internal) {
                ""      /* tempname */
                ,""     /* filename */
                ,""     /* filename_org */
                ,NULL   /* file */
                ,NULL   /* buffer */
                ,1      /* delimLen */
                ,0      /* bufferSize */
                ,0      /* recordLen */
        };

        MALLOC(writer->_internal->buffer, CSV_BUFFER_FACTOR);

        /* Open a temp FILE* for writer->_internal->file.  This will
         * be used for output in case we do a reset.
         */
        if (!strcmp(csv_tempdir, "")) {
                char pwd[PATH_MAX];
                struct statvfs stats;

                EXIT_IF(!getcwd(pwd, sizeof(pwd)-10), "getcwd error");
                statvfs(pwd, &stats);

                /* If the partition we are currently in is low on
                 * available space, we use TMPDIR for temp files.
                 * This can be defined during compilation.
                 */
                if (stats.f_bsize * stats.f_bavail < MIN_SPACE_AVAILABLE)
                        STRNCPY(csv_tempdir, TMPDIR_STR, PATH_MAX - 10);
        }

        STRNCPY(writer->_internal->tempname, csv_tempdir, PATH_MAX - 10);
        strcat(writer->_internal->tempname, "csv_XXXXXX");
        int fd = mkstemp(writer->_internal->tempname);
        writer->_internal->file = fdopen(fd, "w");
        EXIT_IF(!writer->_internal->file, writer->_internal->tempname);

        set_tempoutputfile(writer->_internal->tempname);

        return writer;

}

void csv_writer_open(struct csv_writer* writer, const char* filename)
{
        STRNCPY(writer->_internal->filename, filename, PATH_MAX);
}

void csv_destroy_writer(struct csv_writer* writer)
{
        FREE(writer->_internal->buffer);
        FREE(writer->_internal);
        FREE(writer);
}

