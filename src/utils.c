/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Main functions of the program.
 *
 * Copyright (C) 2007-2008 David Härdeman <david@hardeman.nu>
 * Copyright (C) 2015-2018 Przemyslaw Pawelczyk <przemoc@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; only version 2 of the License is applicable.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

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
                msg(MSG_CRITICAL, "Failed to malloc %zu bytes\n", size);
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
		msg(MSG_CRITICAL, "Failed to strdup %zu bytes\n", strlen(s));
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
	if (size && fwrite(ptr, size, 1, stream) != 1) {
		msg(MSG_CRITICAL, "Failed to write to file: %s\n",
		    strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/* Writes an int to a file, using len bytes, in little-endian order */
void
write_int(uint64_t value, size_t len, FILE *to)
{
	char buf[sizeof(value)];
	size_t i;

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

/* Reads an int from a file, using len bytes, in little-endian order */
uint64_t
read_int(char **from, size_t len, const char *max)
{
	uint64_t result = 0;
	size_t i;

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
	memcpy(result, *from, len);
	*from += len;
	return result;
}

/* Reads a normal C string from a file */
char *
read_string(char **from, const char *max)
{
	return read_binary_string(from, strlen(*from) + 1, max);
}

/* For group caching */
static struct group *gtable = NULL;

/* Initial setup of the gid table */
static void
create_group_table(void)
{
	struct group *tmp;
	int count, index;

	for (count = 0; getgrent(); count++) /* Do nothing */;

	gtable = xmalloc(sizeof(struct group) * (count + 1));
	memset(gtable, 0, sizeof(struct group) * (count + 1));
	setgrent();

	for (index = 0; (tmp = getgrent()) && index < count; index++) {
		gtable[index].gr_gid = tmp->gr_gid;
		gtable[index].gr_name = xstrdup(tmp->gr_name);
	}

	endgrent();
}

/* Caching version of getgrnam */
struct group *
xgetgrnam(const char *name)
{
	int i;

	if (!gtable)
		create_group_table();

	for (i = 0; gtable[i].gr_name; i++) {
		if (!strcmp(name, gtable[i].gr_name))
			return &(gtable[i]);
	}

	return NULL;
}

/* Caching version of getgrgid */
struct group *
xgetgrgid(gid_t gid)
{
	int i;

	if (!gtable)
		create_group_table();

	for (i = 0; gtable[i].gr_name; i++) {
		if (gtable[i].gr_gid == gid)
			return &(gtable[i]);
	}

	return NULL;
}

/* For user caching */
static struct passwd *ptable = NULL;

/* Initial setup of the passwd table */
static void
create_passwd_table(void)
{
	struct passwd *tmp;
	int count, index;

	for (count = 0; getpwent(); count++) /* Do nothing */;

	ptable = xmalloc(sizeof(struct passwd) * (count + 1));
	memset(ptable, 0, sizeof(struct passwd) * (count + 1));
	setpwent();

	for (index = 0; (tmp = getpwent()) && index < count; index++) {
		ptable[index].pw_uid = tmp->pw_uid;
		ptable[index].pw_name = xstrdup(tmp->pw_name);
	}

	endpwent();
}

/* Caching version of getpwnam */
struct passwd *
xgetpwnam(const char *name)
{
	int i;

	if (!ptable)
		create_passwd_table();

	for (i = 0; ptable[i].pw_name; i++) {
		if (!strcmp(name, ptable[i].pw_name))
			return &(ptable[i]);
	}

	return NULL;
}

/* Caching version of getpwuid */
struct passwd *
xgetpwuid(uid_t uid)
{
	int i;

	if (!ptable)
		create_passwd_table();

	for (i = 0; ptable[i].pw_name; i++) {
		if (ptable[i].pw_uid == uid)
			return &(ptable[i]);
	}

	return NULL;
}
