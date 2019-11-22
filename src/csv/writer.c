#include "csv.h"

//static char csvw_fileName[PATH_MAX] = "";
//static char csvw_fileName_org[PATH_MAX] = "";
//static char csvw_tempname[PATH_MAX] = "";
//static char csvw_tempdir[PATH_MAX-10] = "";
//static FILE* csvw_file = NULL;
//static char csvw_buffer[CSV_MAX_FIELD_SIZE];
//static char csvw_delim[32] = "";
//static int csvw_delimlen = 0;
//static char csvw_lineEnding[3] = "\n";
//
///* Flags */
//static int csvw_qualifiers = 2;
//static int csvw_inPlaceEdit = 0;

void csv_writer_reset()
{
        if (writer->_internal->fileName[0]) {
                STRNCPY(writer->_internal->fileName, writer->_internal->fileName_org, PATH_MAX);
        }

        EXIT_IF(fclose(writer->_internal->file) == EOF, writer->_internal->tempname);
        writer->_internal->file = NULL;

        writer->_internal->file = fopen(writer->_internal->tempname, "w");
        EXIT_IF(!writer->_internal->file, writer->_internal->tempname);
}

void csvw_writeline(struct csv_writer* writer)
{
        int i = 0;
        unsigned int j = 0;
        int writeIndex = 0;
        int quotes = 0;
        int delimI = 0;
        char c = 0;
        for (i = 0; i < writer->size; ++i) {
                quotes = 0;
                writeIndex = 0;
                /* TODO - up front size check. */
                for (j = 0; j < writer->fields[i].length; ++j) {
                        c = writer->fields[i].begin[j];
                        writer->_internal->buffer[writeIndex++] = c;
                        if (c == '"' && writer->_internal->STD_QUALIFIERS) {
                                writer->_internal->buffer[writeIndex++] = '"';
                                quotes = 1;
                        }
                        if (writer->_internal->qualifiers && !quotes) {
                                if (strhaschar("\"\n\r", c))
                                        quotes = 1;
                                else if (c == writer->_internal->delim[delimI])
                                        ++delimI;
                                else if (c == writer->_internal->delim[0])
                                        delimI = 1;
                                else
                                        delimI = 0;

                                if (delimI == writer->_internal->delimlen)
                                        quotes = 1;
                        }
                }
                writer->_internal->buffer[writeIndex] = '\0';
                if (quotes)
                        fprintf(writer->_internal->file, "\"%s\"", writer->_internal->buffer);
                else
                        fputs(writer->_internal->buffer, writer->_internal->file);

                if (i != writer->size - 1)
                        fputs(writer->_internal->delim, writer->_internal->file);
        }
        fputs(writer->_internal->lineEnding, writer->_internal->file);
}

void csvw_update_filename()
{
        char indexStr[12];
        snprintf(indexStr, 12, "%d", ++writer->_internal->fileIndex);

        char* extension = getext(writer->_internal->fileName);
        char* noExtension = getnoext(writer->_internal->fileName_org);

        strcpy(writer->_internal->fileName, noExtension);
        strcat(writer->_internal->fileName, indexStr);
        strcat(writer->_internal->fileName, extension);

        free(extension);
        free(noExtension);
}

void csv_writer_close(struct csv_writer* writer)
{
        //if (!csvr_get_allowstdchange())
        //        return;

        EXIT_IF(fclose(writer->_internal->file) == EOF, writer->_internal->tempname);
        writer->_internal->file = NULL;

        if (writer->_internal->fileName[0]) {
                int ret = rename(writer->_internal->tempname, writer->_internal->fileName);
                EXIT_IF(ret, writer->_internal->tempname);
                writer->_internal->update_filename();
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

struct csv_writer* new_csv_writer()
{
        char pwd[PATH_MAX];
        struct statvfs stats;

        EXIT_IF(!getcwd(pwd, sizeof(pwd)-10), "getcwd error");
        statvfs(pwd, &stats);

        /**
         * If the partition we are currently in is low on
         * available space, we use TMPDIR for temp files.
         * This can be defined during compilation.
         */
        if (stats.f_bsize * stats.f_bavail < MIN_SPACE_AVAILABLE)
                STRNCPY(writer->_internal->tempdir, TMPDIR_STR, PATH_MAX);

        //if (!writer->_internal->buffer) {
        //        MALLOC(writer->_internal->buffer, CSV_MAX_FIELD_SIZE);
        //}
}

void csv_writer_open()
{
        //csvw_init();

        if (csvr_get_allowstdchange()) {
                STRNCPY(writer->_internal->tempname, writer->_internal->tempdir, PATH_MAX - 10);
                strcat(writer->_internal->tempname, "csv_XXXXXX");
                int fd = mkstemp(writer->_internal->tempname);
                set_tempoutputfile(writer->_internal->tempname);
                writer->_internal->file = fdopen(fd, "w");
                EXIT_IF(!writer->_internal->file, writer->_internal->tempname);
        } else {
                writer->_internal->file = stdout;
        }
}

//void writer->_internal->destroy()
//{
//        //FREE(writer->_internal->delim);
//        //FREE(writer->_internal->buffer);
//}


