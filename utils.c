#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#include "utils.h"
#include "metastore.h"

void *
xmalloc(size_t size)
{
        void *result = malloc(size);
        if (!result) {
                fprintf(stderr, "Failed to malloc %zi bytes\n", size);
                exit(EXIT_FAILURE);
        }
        return result;
}

char *
xstrdup(const char *s)
{
	char *result = strdup(s);
	if (!result) {
		fprintf(stderr, "Failed to strdup %zi bytes\n", strlen(s));
		exit(EXIT_FAILURE);
	}
	return result;
}

void
binary_print(const char *s, ssize_t len)
{
	ssize_t i;

	for (i = 0; i < len; i++) {
		if (isprint(s[i]))
			printf("%c", s[i]);
		else
			printf("0x%02X", (int)s[i]);
	}
}

void
xfwrite(const void *ptr, size_t size, FILE *stream)
{
	if (fwrite(ptr, size, 1, stream) != 1) {
		perror("fwrite");
		exit(EXIT_FAILURE);
	}
}

void
write_int(uint64_t value, size_t len, FILE *to)
{
	char buf[len];
	int i;

	for (i = 0; i < len; i++)
		buf[i] = ((value >> (8 * i)) & 0xff);
	xfwrite(buf, len, to);
}

void
write_binary_string(const char *string, size_t len, FILE *to)
{
	xfwrite(string, len, to);
}

void
write_string(const char *string, FILE *to)
{
	xfwrite(string, strlen(string) + 1, to);
}

uint64_t
read_int(char **from, size_t len, const char *max)
{
	uint64_t result = 0;
	int i;

	if (*from + len > max) {
		fprintf(stderr, "Attempt to read beyond end of file, corrupt file?\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < len; i++)
		result += (((*from)[i] & 0xff) << (8 * i));
	*from += len;
	return result;
}

char *
read_binary_string(char **from, size_t len, const char *max)
{
	char *result;

	if (*from + len > max) {
		fprintf(stderr, "Attempt to read beyond end of file, corrupt file?\n");
		exit(EXIT_FAILURE);
	}

	result = xmalloc(len);
	strncpy(result, *from, len);
	*from += len;
	return result;
}

char *
read_string(char **from, const char *max)
{
	return read_binary_string(from, strlen(*from) + 1, max);
}

