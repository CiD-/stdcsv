#include "safegetline.h"

int _safegetline(FILE *fp, char* buffer, size_t* buflen, size_t* off)
{
        char *end = buffer + *buflen - 1;
        char *dst = buffer + *off;
        int c = 0;

        while ((c = getc(fp)) != EOF  && c != '\n' && dst < end)
        {
                if (c == '\r') {
                        c = getc(fp);
                        if (c != '\n' && c != EOF)
                                ungetc(c, fp);
                        *off = dst - buffer + 1;
                        break;
                }
                *dst++ = c;
        }
        *dst = '\0';

        if (!*off) {
                /* End of Buffer before End of Line */
                if (dst == end && c != '\n')
                        return EOF - 1;
                *off = dst - buffer;
        }

        if (c == EOF && dst == buffer)
                return EOF;

        return 0;
}


int sappline(FILE *f, char **buf, size_t* buflen, size_t* len, size_t off)
{
        size_t offset = off;
        int ret = 0;
        do {
                if(offset >= *buflen)
                        increase_buffer(buf, buflen);
                ret = _safegetline(f, *buf, buflen, &offset);
        } while (ret == EOF - 1);

        if (len)
                *len = offset;

        return ret;
}

int sgetline(FILE *fp, char **buf, size_t* buflen, size_t* linelen)
{
        return sappline(fp, buf, buflen, linelen, 0);
}
