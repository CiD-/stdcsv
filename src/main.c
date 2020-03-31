#include <stdlib.h>
#include "safegetline.h"

int main()
{
        FILE* _file = fopen("tests/test1.txt", "r");
        char* buf = NULL;
        size_t buflen = 0;
        size_t linelen = 0;
        int ret = sgetline(_file, &buf, &buflen, &linelen);
        printf("%s = 123,456,789\n", buf);
        printf("%d = %d\n",ret, 0);
        printf("%lu = %u\n", linelen, 11);
        printf("%lu = %u\n", buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        printf("%d = %d\n", ret, 0);
        printf("%lu = %u\n", linelen, 0);
        printf("%lu = %u\n", buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        printf("%d = %d\n", ret, EOF);

        printf("errno: %d\n", errno);

        free(buf);
        fclose(_file);
}
