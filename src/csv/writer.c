#include "csv.h"

static char csvw_fileName[PATH_MAX] = "";
static char csvw_fileName_org[PATH_MAX] = "";
static char csvw_tempname[PATH_MAX] = "";
static char csvw_tempdir[PATH_MAX-10] = "";
static FILE* csvw_file = NULL;
static int csvw_fileIndex = 1;
static char csvw_buffer[CSV_MAX_FIELD_SIZE];
static char csvw_delim[32] = "";
static int csvw_delimlen = 0;
static char csvw_lineEnding[3] = "\n";

/* Flags */
static int csvw_qualifiers = 2;
static int csvw_inPlaceEdit = 0;

/**
 *
 */
void csvw_reset();

void csvw_set_delim(const char* delim)
{
        STRNCPY(csvw_delim, delim, 32);
        csvw_delimlen = strlen(csvw_delim);
}

void csvw_set_qualifiers(int i)
{
        csvw_qualifiers = (csvw_qualifiers) ? i : 0;
}

void csvw_set_lineending(const char* lineEnding)
{
        csvw_lineEnding[0] = lineEnding[0];
        csvw_lineEnding[1] = lineEnding[1];
        csvw_lineEnding[2] = lineEnding[2];
}

void csvw_set_filename(const char* filename)
{
        STRNCPY(csvw_fileName_org, filename, PATH_MAX);
        STRNCPY(csvw_fileName, filename, PATH_MAX);
}

void csvw_set_inplaceedit(int ipe)
{
        csvw_inPlaceEdit = ipe;
}

void csvw_reset()
{
        if (csvw_fileName[0]) {
                csvw_fileIndex = 1;
                STRNCPY(csvw_fileName, csvw_fileName_org, PATH_MAX);
        }

        EXIT_IF(fclose(csvw_file) == EOF, csvw_tempname);
        csvw_file = NULL;

        csvw_file = fopen(csvw_tempname, "w");
        EXIT_IF(!csvw_file, csvw_tempname);
}

void csvw_writeline_d(struct csv_record* rec, char* delim)
{
        if (!*csvw_delim && delim)
                csvw_set_delim(delim);

        csvw_writeline(rec);
}

void csvw_writeline(struct csv_record* rec)
{
        int i = 0;
        unsigned int j = 0;
        int writer = 0;
        int quotes = 0;
        int delimI = 0;
        char c = 0;
        for (i = 0; i < rec->size; ++i) {
                quotes = 0;
                writer = 0;
                /* TODO - up front size check. */
                for (j = 0; j < rec->fields[i].length; ++j) {
                        c = rec->fields[i].begin[j];
                        csvw_buffer[writer++] = c;
                        if (c == '"' && CSVW_STD_QUALIFIERS) {
                                csvw_buffer[writer++] = '"';
                                quotes = 1;
                        }
                        if (csvw_qualifiers && !quotes) {
                                if (strhaschar("\"\n\r", c))
                                        quotes = 1;
                                else if (c == csvw_delim[delimI])
                                        ++delimI;
                                else if (c == csvw_delim[0])
                                        delimI = 1;
                                else
                                        delimI = 0;

                                if (delimI == csvw_delimlen)
                                        quotes = 1;
                        }
                }
                csvw_buffer[writer] = '\0';
                if (quotes)
                        fprintf(csvw_file, "\"%s\"", csvw_buffer);
                else
                        fputs(csvw_buffer, csvw_file);

                if (i != rec->size - 1)
                        fputs(csvw_delim, csvw_file);
        }
        fputs(csvw_lineEnding, csvw_file);
}

void csvw_update_filename()
{
        char indexStr[12];
        snprintf(indexStr, 12, "%d", ++csvw_fileIndex);

        char* extension = getext(csvw_fileName);
        char* noExtension = getnoext(csvw_fileName_org);

        strcpy(csvw_fileName, noExtension);
        strcat(csvw_fileName, indexStr);
        strcat(csvw_fileName, extension);

        free(extension);
        free(noExtension);
}

void csvw_close()
{
        if (!csvr_get_allowstdchange())
                return;

        EXIT_IF(fclose(csvw_file) == EOF, csvw_tempname);
        csvw_file = NULL;

        if (csvw_fileName[0]) {
                int ret = rename(csvw_tempname, csvw_fileName);
                EXIT_IF(ret, csvw_tempname);
                csvw_update_filename();
        } else {
                FILE* dumpFile = fopen(csvw_tempname, "r");
                EXIT_IF(!dumpFile, csvw_tempname);

                char c = '\0';
                while ((c = getc(dumpFile)) != EOF)
                        putchar(c);

                EXIT_IF(fclose(dumpFile) == EOF, csvw_tempname);
                cleanoutputfile();
        }
}

void csvw_init()
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
                STRNCPY(csvw_tempdir, TMPDIR_STR, PATH_MAX);

        //if (!csvw_buffer) {
        //        MALLOC(csvw_buffer, CSV_MAX_FIELD_SIZE);
        //}
}

void csvw_open()
{
        csvw_init();

        if (csvr_get_allowstdchange()) {
                STRNCPY(csvw_tempname, csvw_tempdir, PATH_MAX - 10);
                strcat(csvw_tempname, "csv_XXXXXX");
                int fd = mkstemp(csvw_tempname);
                set_tempoutputfile(csvw_tempname);
                csvw_file = fdopen(fd, "w");
                EXIT_IF(!csvw_file, csvw_tempname);
        } else {
                csvw_file = stdout;
        }
}

//void csvw_destroy()
//{
//        //FREE(csvw_delim);
//        //FREE(csvw_buffer);
//}


