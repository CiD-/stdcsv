#include <stdlib.h>
#include "/data/user/jason/dev/refactor/lib/safegetline.h"

int main()
{
        FILE* _file = fopen("tests/test1.txt", "r");
        char* buf = NULL;
        size_t buflen = 0;
        size_t linelen = 0;
        int ret = sgetline(_file, &buf, &buflen, &linelen);
        printf("%s = aaa,bbb,ccc\n", buf);
        printf("%d = %d\n",ret, 0);
        printf("%lu = %lu\n", linelen, 11);
        printf("%lu = %lu\n", buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        printf("%d = %d\n", ret, 0);
        printf("%lu = %lu\n", linelen, 0);
        printf("%lu = %lu\n", buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        printf("%d = %d\n", ret, EOF);

        free(buf);
        fclose(_file);
}
