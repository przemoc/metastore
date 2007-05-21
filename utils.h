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

/* For uint64_t */
#include <stdint.h>
/* For ssize_t */
#include <unistd.h>
/* For FILE */
#include <stdio.h>
/* For struct passwd */
#include <pwd.h>
/* For struct group */
#include <grp.h>

/* Adjusts the verbosity level for msg() */
void adjust_verbosity(int adj);

/* Verbosity levels using stdout */
#define MSG_NORMAL    0
#define MSG_DEBUG     1
#define MSG_QUIET    -1
/* Verbosity levels using stderr */
#define MSG_ERROR    -2
#define MSG_CRITICAL -3

/* Prints messages to console according to the current verbosity */
int msg(int level, const char *fmt, ...);

/* Malloc which either succeeds or exits */
void *xmalloc(size_t size);

/* Ditto for strdup */
char *xstrdup(const char *s);

/* Human-readable printout of binary data */
void binary_print(const char *s, ssize_t len);

/* Writes data to a file or exits on failure */ 
void xfwrite(const void *ptr, size_t size, FILE *stream);

/* Writes an int to a file, using len bytes, in bigendian order */
void write_int(uint64_t value, size_t len, FILE *to);

/* Writes a binary string to a file */
void write_binary_string(const char *string, size_t len, FILE *to);

/* Writes a normal C string to a file */
void write_string(const char *string, FILE *to);

/* Reads an int from a file, using len bytes, in bigendian order */
uint64_t read_int(char **from, size_t len, const char *max);

/* Reads a binary string from a file */
char *read_binary_string(char **from, size_t len, const char *max);

/* Reads a normal C string from a file */
char *read_string(char **from, const char *max);

/* Caching version of getgrnam */
struct group *xgetgrnam(const char *name);

/* Caching version of getgrgid */
struct group *xgetgrgid(gid_t gid);

/* Caching version of getpwnam */
struct passwd *xgetpwnam(const char *name);

/* Caching version of getpwuid */
struct passwd *xgetpwuid(uid_t uid);

