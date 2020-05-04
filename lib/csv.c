#include "csverror.h"
#include "csv.h"

void csv_perror()
{
        err_printall();
}

void csv_perror_exit()
{
        csv_perror();
        exit(EXIT_FAILURE);
}
