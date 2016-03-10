/**
  * Source: https://github.com/git/git/blob/master/compat/win32mmap.c
  * License: https://github.com/git/git/blob/master/COPYING (GPL v2)
  */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <io.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <windows.h>

#include "mmap.h"

static void vreportf(const char *prefix, const char *err, va_list params)
{
	fflush(stderr);
	fputs(prefix, stderr);
	vfprintf(stderr, err, params);
	fputc('\n', stderr);
}

static void die(const char *err, ...)
{
	va_list params;

	va_start(params, err);
	vreportf("fatal: ", err, params);
	va_end(params);
	exit(128);
}

static void warning(const char *warn, ...)
{
	va_list params;

	va_start(params, warn);
	vreportf("warning: ", warn, params);
	va_end(params);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"

static inline size_t xsize_t(off_t len)
{
	if (len > (size_t) len)
		die("Cannot handle files this big");
	return (size_t)len;
}

#pragma GCC diagnostic pop

void *mmap(void *start, size_t length, __attribute__((unused)) int prot, int flags, int fd, off_t offset)
{
	HANDLE hmap;
	void *temp;
	off_t len;
	struct stat st;
	uint64_t o = offset;
	uint32_t l = o & 0xFFFFFFFF;
	uint32_t h = (o >> 32) & 0xFFFFFFFF;

	if (!fstat(fd, &st))
		len = st.st_size;
	else
		die("mmap: could not determine filesize");

	if ((off_t) (length + offset) > len)
		length = xsize_t(len - offset);

	if (!(flags & MAP_PRIVATE))
		die("Invalid usage of mmap");

	hmap = CreateFileMapping((HANDLE)_get_osfhandle(fd), NULL,
		PAGE_WRITECOPY, 0, 0, NULL);

	if (!hmap)
		return MAP_FAILED;

	temp = MapViewOfFileEx(hmap, FILE_MAP_COPY, h, l, length, start);

	if (!CloseHandle(hmap))
		warning("unable to close file mapping handle");

	return temp ? temp : MAP_FAILED;
}

int munmap(void *start, __attribute__((unused)) size_t length)
{
	return !UnmapViewOfFile(start);
}
