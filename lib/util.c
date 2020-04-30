#include "csvsignal.h"
#include "util.h"

void increase_buffer(char** buf, size_t* buflen)
{
        *buflen += BUFFER_FACTOR;
        if (*buflen == BUFFER_FACTOR){
                MALLOC(*buf, *buflen);
        } else {
                REALLOC(*buf, *buflen);
        }
}

void increase_buffer_to(char** buf, size_t* buflen, size_t target)
{
        target = BUFFER_FACTOR * (target / BUFFER_FACTOR + 1);
        if (*buflen > target)
                return;

        *buflen = target;
        if (*buflen == BUFFER_FACTOR){
                MALLOC(*buf, *buflen);
        } else {
                REALLOC(*buf, *buflen);
        }
}

long stringtolong10(const char* s)
{
        return stringtolong(s, 10);
}

long stringtolong(const char* s, int base)
{
        char* endPtr = NULL;
        errno = 0;
        long val = strtol(s, &endPtr, base);
        //if (s == endPtr)
        if ((errno == ERANGE && (val == LONG_MIN || val == LONG_MAX))
                        || (errno == EINVAL)
                        || (errno != 0 && val == 0)
                        || (errno == 0 && s && *endPtr != 0))
                perror("strtol");
        else
                return val;

        exit(EXIT_FAILURE);
}

int charcount(const char* s, char c)
{
        int count = 0;
        int i = 0;
        for(; s[i] != '\0'; ++i)
                if (s[i] == c)
                        ++count;

        return count;
}

int strhaschar(const char* s, char c)
{
        int i = 0;
        for (; s[i] != '\0'; ++i)
                if (s[i] == c)
                        return TRUE;

        return FALSE;
}

void removecharat(char* s, int i)
{
        for (; s[i] != '\0'; ++i)
                s[i] = s[i+1];
        s[i] = '\0';
}

char* randstr(char* s, const int len)
{
        static const char alphanum[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";

        int i = 0;

        for (; i < len; ++i)
                s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

        s[i] = '\0';

        return s;
}

/* https://stackoverflow.com/questions/2736753/how-to-remove-extension-from-file-name */
char* getnoext(const char* filename)
{
        char* retstr = NULL;
        char* lastdot = NULL;
        char* lastsep = NULL;

        if (!filename)
                return NULL;
        if (!(retstr = malloc(strlen(filename) + 1)))
                return NULL;

        strncpy(retstr, filename, PATH_MAX);
        lastdot = strrchr(retstr, '.');
        lastsep = ('/' == 0) ? NULL : strrchr(retstr, '/');

        if (lastdot != NULL) {
                if (lastsep != NULL) {
                        if (lastsep < lastdot)
                                *lastdot = '\0';
                } else {
                        *lastdot = '\0';
                }
        }

        return retstr;
}


char* getext(char* filename)
{
        char* base = basename(filename);
        const char *dot = strrchr(base, '.');
        if(dot && dot != base) {
                char* ext = NULL;
                MALLOC(ext, strlen(dot) + 1);
                strcpy(ext, dot);
                return ext;
        }
        return "";
}

