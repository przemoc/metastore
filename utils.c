/*
 * Main functions of the program.
 *
 * Copyright (C) 2007 David Härdeman <david@hardeman.nu>
 *
 *      This program is free software; you can redistribute it and/or modify it
 *      under the terms of the GNU General Public License as published by the
 *      Free Software Foundation version 2 of the License.
 * 
 *      This program is distributed in the hope that it will be useful, but
 *      WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      General Public License for more details.
 * 
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>

#include "utils.h"

/* Controls the verbosity level for msg() */
static int verbosity = 0;

/* Adjusts the verbosity level for msg() */
void
adjust_verbosity(int adj)
{
	verbosity += adj;
}

/*
 * Prints messages to console according to the current verbosity
 * - see utils.h for level defines
 */
int
msg(int level, const char *fmt, ...)
{
	int ret;
	va_list ap;

	if (level > verbosity)
		return 0;

	va_start(ap, fmt);

	if (level < MSG_QUIET)
		ret = vfprintf(stderr, fmt, ap);
	else
		ret = vfprintf(stdout, fmt, ap);

	va_end(ap);
	return ret;
}

/* Malloc which either succeeds or exits */
void *
xmalloc(size_t size)
{
        void *result = malloc(size);
        if (!result) {
                msg(MSG_CRITICAL, "Failed to malloc %zi bytes\n", size);
                exit(EXIT_FAILURE);
        }
        return result;
}

/* Ditto for strdup */
char *
xstrdup(const char *s)
{
	char *result = strdup(s);
	if (!result) {
		msg(MSG_CRITICAL, "Failed to strdup %zi bytes\n", strlen(s));
		exit(EXIT_FAILURE);
	}
	return result;
}

/* Human-readable printout of binary data */
void
binary_print(const char *s, ssize_t len)
{
	ssize_t i;

	for (i = 0; i < len; i++) {
		if (isprint(s[i]))
			msg(MSG_DEBUG, "%c", s[i]);
		else
			msg(MSG_DEBUG, "0x%02X", (int)s[i]);
	}
}

/* Writes data to a file or exits on failure */
void
xfwrite(const void *ptr, size_t size, FILE *stream)
{
	if (fwrite(ptr, size, 1, stream) != 1) {
		perror("fwrite");
		exit(EXIT_FAILURE);
	}
}

/* Writes an int to a file, using len bytes, in bigendian order */
void
write_int(uint64_t value, size_t len, FILE *to)
{
	char buf[len];
	int i;

	for (i = 0; i < len; i++)
		buf[i] = ((value >> (8 * i)) & 0xff);
	xfwrite(buf, len, to);
}

/* Writes a binary string to a file */
void
write_binary_string(const char *string, size_t len, FILE *to)
{
	xfwrite(string, len, to);
}

/* Writes a normal C string to a file */
void
write_string(const char *string, FILE *to)
{
	xfwrite(string, strlen(string) + 1, to);
}

/* Reads an int from a file, using len bytes, in bigendian order */
uint64_t
read_int(char **from, size_t len, const char *max)
{
	uint64_t result = 0;
	int i;

	if (*from + len > max) {
		msg(MSG_CRITICAL,
		    "Attempt to read beyond end of file, corrupt file?\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < len; i++)
		result += (((*from)[i] & 0xff) << (8 * i));
	*from += len;
	return result;
}

/* Reads a binary string from a file */
char *
read_binary_string(char **from, size_t len, const char *max)
{
	char *result;

	if (*from + len > max) {
		msg(MSG_CRITICAL,
		    "Attempt to read beyond end of file, corrupt file?\n");
		exit(EXIT_FAILURE);
	}

	result = xmalloc(len);
	strncpy(result, *from, len);
	*from += len;
	return result;
}

/* Reads a normal C string from a file */
char *
read_string(char **from, const char *max)
{
	return read_binary_string(from, strlen(*from) + 1, max);
}

