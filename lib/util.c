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

/**
 * Signal Handlers
 * sigset_t and sa_flags only set to shut up valgrind
 */
static struct sigaction act;
static sigset_t vg_shutup = { {0} };
static int _signals_ready = FALSE;
static struct charnode* _tmp_file_head = NULL;

void init_sig()
{
        if (_signals_ready)
                return;
        /** Attach signal handlers **/
        act.sa_mask = vg_shutup;
        act.sa_flags = 0;
        act.sa_handler = cleanexit;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGQUIT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        sigaction(SIGHUP, &act, NULL);

        _signals_ready = TRUE;
}


void cleanexit()
{
        removealltmp();
        exit(EXIT_FAILURE);
}

struct charnode* addtmp(const char* tmp_file)
{
        struct charnode* newnode = NULL;
        MALLOC(newnode, sizeof(*newnode));
        *newnode = (struct charnode) {
                tmp_file
                ,_tmp_file_head
                ,NULL
        };

        if (_tmp_file_head)
                _tmp_file_head->next = newnode;
        _tmp_file_head = newnode;

        return newnode;
}

void removetmpnode(struct charnode* node)
{
        if (!node)
                return;

        if (node->prev)
                node->prev->next = node->next;
        if (node->next)
                node->next->prev = node->prev;
        else
                _tmp_file_head = node->prev;

        FREE(node);
}

void removetmp(struct charnode* node)
{
        if (!node)
                return;

        if (remove(node->data))
                perror(node->data);

        removetmpnode(node);
}

void removealltmp()
{
        while (_tmp_file_head)
                removetmp(_tmp_file_head);
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
                char* ext = malloc(strlen(dot) + 1);
                EXIT_IF(!ext, "malloc");
                strcpy(ext, dot);
                return ext;
        }
        return "";
}

