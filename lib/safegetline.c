#include "safegetline.h"
#include "misc.h"

#include <string.h>

int _safegetline(FILE *fp, char* buffer, size_t* buflen, size_t* off)
{
	char *end = buffer + *buflen - 1;
	char *dst = buffer + *off;
	int endfound = false;
	int c = 0;

	while (!endfound && dst < end && (c = getc(fp)) != EOF)
	{
		*dst++ = c;
		if (c == '\r') {
			c = getc(fp);
			if (c != '\n' && c != EOF)
				ungetc(c, fp);
			endfound = true;
		}
		if (c == '\n') {
			/* Handles trailing EOL at EOF consistently */
			c = getc(fp);
			if (c != EOF)
				ungetc(c, fp);
			endfound = true;
		}
	}

	if (endfound)
		dst--;

	*dst = '\0';
	*off = dst - buffer;

	/* End of Buffer before End of Line */
	if (dst == end)
		return EOF - 1;

	if (c == EOF && dst == buffer) {
		if (ferror(fp)) {
			perror("getc");
			exit(EXIT_FAILURE);
		}
		return EOF;
	}

	return 0;
}

int _getline_runner(FILE* f, char** buf, size_t* buflen, size_t* len, size_t off)
{
	size_t offset = off;
	int ret = 0;
	do {
		if(offset + 1 >= *buflen)
			increase_buffer(buf, buflen);
		ret = _safegetline(f, *buf, buflen, &offset);
	} while (ret == EOF - 1);

	if (len)
		*len = offset;

	return ret;
}

int sappline(FILE *f, char **buf, size_t* buflen, size_t* len)
{
	if (*len+1 > *buflen) {
		increase_buffer(buf, buflen);
	}
	char* end = *buf + *len;
	*end = '\n';
	++(*len);

	return _getline_runner(f, buf, buflen, len, *len);
}

int sgetline(FILE *f, char **buf, size_t* buflen, size_t* len)
{
	return _getline_runner(f, buf, buflen, len, 0);
}


/**
 * Unlike sgetline sgetline_mmap, will not read a 
 * carriage return only line ending file. I ignored it
 * because I want to go *FAST*.  I don't have any proof
 * that this is any faster, so as it turns out, I'm just
 * *LAZY*
 */

int sappline_mmap(const char* mmap, 
		  char** line, 
		  size_t* bufidx, 
		  size_t* len, 
		  size_t limit)
{
	const char* org_line = *line;
	size_t org_idx = *bufidx;
	int ret = sgetline_mmap(mmap, line, bufidx, len, limit);
	if (ret == EOF) {
		return EOF;
	}
	*line = (char*)org_line;
	*len += (*bufidx - org_idx) + 1;
	return 0;
}

int sgetline_mmap(const char* mmap,
		  char** line,
		  size_t* bufidx,
		  size_t* len,
		  size_t limit)
{
	if (*bufidx == limit) {
		return EOF;
	}
        switch (mmap[*bufidx]) {
	case '\r':
		*bufidx += 2;
		break;
	case '\n':
		*bufidx += 1;
	default:
		;
	}

	*line = (char*) &mmap[*bufidx];
	char* eol = memchr(*line, '\n', limit - *bufidx);

	/* last read */
	if (eol == NULL) {
		*len = limit - *bufidx;
		*bufidx = limit;
		return (*len) ? 0 : EOF;
	}

	if (*(eol-1) == '\r') {
		--eol;
	}

	*len = eol - *line;
	*bufidx = eol - mmap;

	return 0;
}
