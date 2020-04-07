#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "util.h"
#include "csv.h"

static const char* helpString =
"\nUsage: stdcsv [vhniqQxXS] [-N field_count] [-dD delimiter]"
"\n       [-r new_line_replacement] [-o outputfile] input_file"
"\n"
//"\n-c|--concat               Concatenate all input files together. Assuming a"
//"\n                          header, the first row only prints for first file."
//"\n-C|--concat-all           Same as -c except we assume there is no header."
//"\n                          All rows are printed for all files."
"\n-d|--in-delimiter arg     Specify an input delimiter."
"\n                          Default delimiters: comma, pipe, tab"
"\n-D|--out-delimiter arg    Specify an output delimiter."
"\n                          By default, the input delimiter is used."
"\n-f|--failsafe             Use failsafe mode (more info below)."
"\n-h|--help                 Print this help menu."
"\n-i|--in-place             Files edited in place. This will not work for stdin."
"\n-M|--cr                   Output will have Macintosh line endings."
"\n-n|--normalize            Output field count will match header."
"\n-N|--num-fields arg       Specify number of output fields (Implies -n)"
"\n-o|--output-file arg      Specify an output file. Default is stdout."
"\n                          Note: This implies concatenation"
"\n-Q|--out-quotes arg       Specify quoting rule set for output."
"\n-q|--in-quotes arg        Specify quoting rule set for input."
"\n                          Options: NONE, WEAK, RFC4180, ALL (details below)"
"\n-r|--no-embedded-nl       Remove embedded new lines."
"\n-R|--replace-newline arg  Specify a string to replace embedded new lines."
//"\n-v|--verbose              More detailed output."
"\n-W|--crlf                 Output will have Windows line endings."
"\n"
"\nFAILSAFE MODE"
"\nFailsafe mode allows us to loop through the different CSV rule sets"
"\nlooking for violations along the way.  If a violation is discovered,"
"\nfile reading is restarted from the beginning of the file."
"\nNOTE: Failsafe mode will be turned off when reading from stdin."
"\n"
"\nQUOTING RULES"
"\n  NONE - Assume no text qualification."
"\n  WEAK - Allow embedded quotes without duplication. No extra white space."
"\n  RFC4180 (default) - The most flexible \"standard\" for delimited files."
"\n  ALL - Quote every field. This option has no effect on input."
"\n"
"\n        More info: https://www.ietf.org/rfc/rfc4180.txt\n"
;

typedef struct csv_reader csv_reader;
typedef struct csv_writer csv_writer;
typedef struct csv_record csv_record;


void parseargs(char c, csv_reader* reader, csv_writer* writer)
{
        switch (c) {
        case 'c':
                fputs("Not yet implemented\n", stderr);
                exit(EXIT_FAILURE);
        case 'C':
                fputs("Not yet implemented\n", stderr);
                exit(EXIT_FAILURE);
        case 'f':
                reader->failsafeMode = TRUE;
                csv_open_temp(writer);
                break;
        case 'h': /* help */
                puts(helpString);
                exit(EXIT_SUCCESS);
        case 'n': /* normalize */
                reader->normal = CSV_NORMAL_OPEN;
                break;
        case 'N': { /* number-of-fields */
                long val = stringtolong10(optarg);
                if (val < 1) {
                        fputs("Invalid number of columns.\n", stderr);
                        exit(EXIT_FAILURE);
                }
                reader->normal = val;
        }
                break;
        case 'i': /* in-place-edit */
                //csvw_set_inplaceedit(1);
                break;
        case 'Q': /* out-quotes */
                if(!strcasecmp(optarg, "ALL"))
                        writer->quotes = QUOTE_ALL;
                else if (!strcasecmp(optarg, "WEAK"))
                        writer->quotes = QUOTE_WEAK;
                else if (!strcasecmp(optarg, "NONE"))
                        writer->quotes = QUOTE_NONE;
                else if (!strcasecmp(optarg, "RFC4180"))
                        writer->quotes = QUOTE_RFC4180;
                else {
                        fprintf(stderr
                                ,"Invalid quote option: %s\n"
                                 "Options: NONE, WEAK, RFC4180, ALL\n"
                                , optarg);
                        exit(EXIT_FAILURE);
                }
                break;
        case 'q': /* in-quotes */
                if(!strcasecmp(optarg, "ALL"))
                        reader->quotes = QUOTE_ALL;
                else if (!strcasecmp(optarg, "WEAK"))
                        reader->quotes = QUOTE_WEAK;
                else if (!strcasecmp(optarg, "NONE"))
                        reader->quotes = QUOTE_NONE;
                else if (!strcasecmp(optarg, "RFC4180"))
                        reader->quotes = QUOTE_RFC4180;
                else {
                        fprintf(stderr
                                ,"Invalid quote option: %s\n"
                                 "Options: NONE, WEAK, RFC4180, ALL\n"
                                , optarg);
                        exit(EXIT_FAILURE);
                }
                break;
        case 'D': /* output-delimiter */
                STRNCPY(writer->delimiter, optarg, 32);
                break;
        case 'd': /* input-delimiter */
                STRNCPY(reader->delimiter, optarg, 32);
                break;
        case 'r': /* no-embedded-nl */
                STRNCPY(reader->inlineBreak, "", 32);
                break;
        case 'R': /* replace-newlines */
                STRNCPY(reader->inlineBreak, optarg, 32);
                break;
        case 'o': /* output-file */
                //STRNCPY(writer->filename, optarg, PATH_MAX);
                csv_writer_open(writer, optarg);
                break;
        case 'W': /* windows-line-ending */
                STRNCPY(writer->lineEnding, "\r\n", 3);
                break;
        case 'M': /* mac-line-ending */
                STRNCPY(writer->lineEnding, "\r", 3);
                break;
        case '?': /* Should never get here... */
                exit(EXIT_FAILURE);
                break;
        default:
                abort();
        }
}


int main (int argc, char **argv)
{
        int c = 0;

        static struct option long_options[] =
        {
                /* long option, (no) arg, 0, short option */
                {"help", no_argument, 0, 'h'},
                {"normalize", no_argument, 0, 'n'},
                {"num-fields", required_argument, 0, 'N'},
                {"failsafe", no_argument, 0, 'f'},
                {"in-place-edit", no_argument, 0, 'i'},
                {"out-quotes", required_argument, 0, 'Q'},
                {"in-quotes", required_argument, 0, 'q'},
                {"out-delimiter", required_argument, 0, 'd'},
                {"in-delimiter", required_argument, 0, 'D'},
                {"no-embedded-nl", no_argument, 0, 'r'},
                {"embedded-nl-sub", required_argument, 0, 'R'},
                {"output-file", required_argument, 0, 'o'},
                {"concat", no_argument, 0, 'c'},
                {"concat-all", no_argument, 0, 'C'},
                {"crlf", no_argument, 0, 'W'},
                {"cr", no_argument, 0, 'M' },
                {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        csv_reader* reader = csv_reader_new();
        csv_writer* writer = csv_writer_new();
        csv_record* record = NULL;

        while ( (c = getopt_long (argc, argv, "cCfhMnirWd:D:N:o:Q:q:R:",
                                  long_options, &option_index)) != -1)
                parseargs(c, reader, writer);

        int ret = 0;

        do {
                /** If a file was provided, open it for reading **/
                if (optind != argc && ret != CSV_RESET)
                        csv_reader_open(reader, argv[optind]);

                while ((ret = csv_get_record(reader, &record)) == CSV_GOOD)
                        csv_write_record(writer, record);

                if (ret == CSV_RESET) {
                        csv_writer_reset(writer);
                } else {
                        csv_writer_close(writer);
                        ++optind;
                }

        } while (optind < argc || ret == CSV_RESET);

        csv_reader_free(reader);
        csv_writer_free(writer);

        return 0;
}
