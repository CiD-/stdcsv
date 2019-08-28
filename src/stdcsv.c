#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "util.h"
#include "csvrw.h"

static const char* helpString =
"\nUsage: stdcsv [vhniqQxXS] [-N field_count] [-dD delimiter]"
"\n       [-r new_line_replacement] [-o outputfile] input_file\n"
"\n-c|--concatenate       Concatenate all input files together. Assuming a"
"\n                       header, the first row only prints for first file."
"\n-C|--concatenate-all   Same as -c except we assume there is no header."
"\n                       All rows are printed for all files."
"\n-d|--in-delimiter X    Specify an input delimiter."
"\n                       Default delimiters: comma, pipe, tab"
"\n-D|--out-delimiter X   Specify an output delimiter."
"\n                       By default, the input delimiter is used."
"\n-h|--help              Print this help menu."
"\n-i|--in-place          Files edited in place. This will not work for stdin."
"\n-M|--cr                Output will have Macintosh line endings."
"\n-n|--normalize         Output field count will match header."
"\n-N|--num-fields X      Specify number of output fields (Implies -n)"
"\n-o|--out-file X        Specify an output file. Default is stdout."
"\n                       Note: This implies -c concatenation"
"\n-q|--suppress-quotes   Do not quote fields in the output."
"\n-Q|--ignore-quotes     Ignore input quotes. All quotes will be in fields."
"\n-r|--no-embedded-nl    Remove embedded new lines."
"\n-R|--embedded-nl-sub X Specify a string to replace embedded new lines."
//"\n-v|--verbose           More detailed output."
"\n-W|--crlf              Output will have Windows line endings."
"\n-X|--no-rfc4180-out    Embedded quotes won't be duplicated in the output."
"\n-x|--no-rfc4180-in     Embedded quotes won't be duplicated in the input."
"\n                       More info: https://www.ietf.org/rfc/rfc4180.txt\n"
"\nRFC4180 is not an industry standard, so many csv files will not follow"
"\nthe rules proposed by it. If no input quote options are specified, stdcsv"
"\nattempt to parse the input following the RFC4180 rules. If a violation is"
"\nfound, parsing will restart from the beginning of the file (not stdin) with"
"\nRFC4180 rules disabled. If more quoting violations are found, input quotes"
"\nwill be turned off entirely. Appropriate warnings will be printed in these"
"\ncases. If input is coming from stdin, the appropriate rules must be"
"\nspecified in the options. Invalid input from stdin cannot be retrieved.\n";


void parseargs(char c)
{
        switch (c) {
        case 'c':
                csvr_set_cat(CSVR_CAT);
                break;
        case 'C':
                csvr_set_cat(CSVR_CAT_ALL);
                break;
        //case 'v': /* verbose */
        //        verbose = 1;
        //        break;
        case 'h': /* help */
                puts(helpString);
                exit(EXIT_SUCCESS);
        case 'n': /* normalize */
                csvr_set_normal(CSVR_NORMAL_OPEN);
                break;
        case 'N': { /* number-of-fields */
                long val = stringtolong10(optarg);
                if (val < 1) {
                        fputs("Invalid number of columns", stderr);
                        exit(EXIT_FAILURE);
                }
                csvr_set_normal(val);
        }
                break;
        case 'i': /* in-place-edit */
                csvw_set_inplaceedit(1);
                break;
        case 'q': /* suppress-qualifiers */
                csvw_set_qualifiers(0);
                break;
        case 'Q': /* ignore-qualifiers */
                csvr_set_qualifiers(0);
                break;
        case 'X': /* disable-std-quotes */
                csvr_set_qualifiers(1);
                break;
        case 'x': /* disable-std-quotes */
                csvw_set_qualifiers(1);
                break;
        case 'D': /* output-delimiter */
                csvw_set_delim(optarg);
                break;
        case 'd': /* input-delimiter */
                csvr_set_delim(optarg);
                break;
        case 'r': /* no-embedded-nl */
                csvr_set_internalbreak("");
                break;
        case 'R': /* replace-newlines */
                csvr_set_internalbreak(optarg);
                break;
        case 'o': /* output-file */
                csvw_set_filename(optarg);
                break;
        case 'W': /* windows-line-ending */
                csvw_set_lineending("\r\n");
                break;
        case 'M': /* mac-line-ending */
                csvw_set_lineending("\r");
                break;
        case '?': /* Should never get here... */
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
                //{"verbose", no_argument, 0, 'v'},
                {"help", no_argument, 0, 'h'},
                {"normalize", no_argument, 0, 'n'},
                {"num-fields", required_argument, 0, 'N'},
                {"in-place-edit", no_argument, 0, 'i'},
                {"suppress-quotes", no_argument, 0, 'q'},
                {"ignore-quotes", no_argument, 0, 'Q'},
                {"no-rfc4180-out", no_argument, 0, 'x'},
                {"no-rfc4180-in", no_argument, 0, 'X'},
                {"out-delimiter", required_argument, 0, 'D'},
                {"in-delimiter", required_argument, 0, 'd'},
                {"no-embedded-nl", no_argument, 0, 'r'},
                {"embedded-nl-sub", required_argument, 0, 'R'},
                {"output-file", required_argument, 0, 'o'},
                {"concatenate", no_argument, 0, 'c'},
                {"concatenate-all", no_argument, 0, 'C'},
                {"crlf", no_argument, 0, 'W'},
                {"cr", no_argument, 0, 'M' },
                {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        while ( (c = getopt_long (argc, argv, "rhniqQxXcCN:D:d:R:o:W",
                                  long_options, &option_index)) != -1)
                parseargs(c);

        while (optind < argc)
                csvr_add_file(argv[optind++]);

        struct csv_field* fields = NULL;
        int fieldCount = 0;

        while (csvr_open_next()) {
                csvw_open();
                while ( (fields = csvr_getfields(fields, &fieldCount)) )
                        csvw_writeline_d(fields, fieldCount, csvr_get_delim());
                csvw_close();
        }

        return 0;
}
