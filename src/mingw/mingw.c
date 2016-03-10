/**
  * Source: https://github.com/git/git/blob/master/compat/mingw.c
  * License: https://github.com/git/git/blob/master/COPYING (GPL v2)
  */

#include "mingw.h"

#include <windows.h>

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "win32.h"

/**
 * Converts UTF-8 encoded string to UTF-16LE.
 *
 * To support repositories with legacy-encoded file names, invalid UTF-8 bytes
 * 0xa0 - 0xff are converted to corresponding printable Unicode chars \u00a0 -
 * \u00ff, and invalid UTF-8 bytes 0x80 - 0x9f (which would make non-printable
 * Unicode) are converted to hex-code.
 *
 * Lead-bytes not followed by an appropriate number of trail-bytes, over-long
 * encodings and 4-byte encodings > \u10ffff are detected as invalid UTF-8.
 *
 * Maximum space requirement for the target buffer is two wide chars per UTF-8
 * char (((strlen(utf) * 2) + 1) [* sizeof(wchar_t)]).
 *
 * The maximum space is needed only if the entire input string consists of
 * invalid UTF-8 bytes in range 0x80-0x9f, as per the following table:
 *
 *               |                   | UTF-8 | UTF-16 |
 *   Code point  |  UTF-8 sequence   | bytes | words  | ratio
 * --------------+-------------------+-------+--------+-------
 * 000000-00007f | 0-7f              |   1   |   1    |  1
 * 000080-0007ff | c2-df + 80-bf     |   2   |   1    |  0.5
 * 000800-00ffff | e0-ef + 2 * 80-bf |   3   |   1    |  0.33
 * 010000-10ffff | f0-f4 + 3 * 80-bf |   4   |  2 (a) |  0.5
 * invalid       | 80-9f             |   1   |  2 (b) |  2
 * invalid       | a0-ff             |   1   |   1    |  1
 *
 * (a) encoded as UTF-16 surrogate pair
 * (b) encoded as two hex digits
 *
 * Note that, while the UTF-8 encoding scheme can be extended to 5-byte, 6-byte
 * or even indefinite-byte sequences, the largest valid code point \u10ffff
 * encodes as only 4 UTF-8 bytes.
 *
 * Parameters:
 * wcs: wide char target buffer
 * utf: string to convert
 * wcslen: size of target buffer (in wchar_t's)
 * utflen: size of string to convert, or -1 if 0-terminated
 *
 * Returns:
 * length of converted string (_wcslen(wcs)), or -1 on failure
 *
 * Errors:
 * EINVAL: one of the input parameters is invalid (e.g. NULL)
 * ERANGE: the output buffer is too small
 */
int xutftowcsn(wchar_t *wcs, const char *utf, size_t wcslen, int utflen);

/**
 * Simplified variant of xutftowcsn, assumes input string is \0-terminated.
 */
static inline int xutftowcs(wchar_t *wcs, const char *utf, size_t wcslen)
{
	return xutftowcsn(wcs, utf, wcslen, -1);
}

/**
 * Simplified file system specific variant of xutftowcsn, assumes output
 * buffer size is MAX_PATH wide chars and input string is \0-terminated,
 * fails with ENAMETOOLONG if input string is too long.
 */
static inline int xutftowcs_path(wchar_t *wcs, const char *utf)
{
	int result = xutftowcsn(wcs, utf, MAX_PATH, -1);
	if (result < 0 && errno == ERANGE)
		errno = ENAMETOOLONG;
	return result;
}

/*
 * The unit of FILETIME is 100-nanoseconds since January 1, 1601, UTC.
 * Returns the 100-nanoseconds ("hekto nanoseconds") since the epoch.
 */
static inline long long filetime_to_hnsec(const FILETIME *ft)
{
	long long winTime = ((long long)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
	/* Windows to Unix Epoch conversion */
	return winTime - 116444736000000000LL;
}

static inline time_t filetime_to_time_t(const FILETIME *ft)
{
	return (time_t)(filetime_to_hnsec(ft) / 10000000);
}

static inline int readlink(
	__attribute__((unused)) const char *path,
	__attribute__((unused)) char *buf,
	__attribute__((unused)) size_t bufsiz)
{
	errno = ENOSYS; return -1;
}

/* We keep the do_lstat code in a separate function to avoid recursion.
 * When a path ends with a slash, the stat will fail with ENOENT. In
 * this case, we strip the trailing slashes and stat again.
 *
 * If follow is true then act like stat() and report on the link
 * target. Otherwise report on the link itself.
 */
static int do_lstat(int follow, const char *file_name, struct stat *buf)
{
	WIN32_FILE_ATTRIBUTE_DATA fdata;
	wchar_t wfilename[MAX_PATH];
	if (xutftowcs_path(wfilename, file_name) < 0)
		return -1;

	if (GetFileAttributesExW(wfilename, GetFileExInfoStandard, &fdata)) {
		buf->st_ino = 0;
		buf->st_gid = 0;
		buf->st_uid = 0;
		buf->st_nlink = 1;
		buf->st_mode = file_attr_to_st_mode(fdata.dwFileAttributes);
		buf->st_size = fdata.nFileSizeLow |
			(((off_t)fdata.nFileSizeHigh)<<32);
		buf->st_dev = buf->st_rdev = 0; /* not used by Git */
		buf->st_atime = filetime_to_time_t(&(fdata.ftLastAccessTime));
		buf->st_mtime = filetime_to_time_t(&(fdata.ftLastWriteTime));
		buf->st_ctime = filetime_to_time_t(&(fdata.ftCreationTime));
		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			WIN32_FIND_DATAW findbuf;
			HANDLE handle = FindFirstFileW(wfilename, &findbuf);
			if (handle != INVALID_HANDLE_VALUE) {
				if ((findbuf.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
						(findbuf.dwReserved0 == IO_REPARSE_TAG_SYMLINK)) {
					if (follow) {
						char buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
						buf->st_size = readlink(file_name, buffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
					} else {
						buf->st_mode = S_IFLNK;
					}
					buf->st_mode |= S_IREAD;
					if (!(findbuf.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
						buf->st_mode |= S_IWRITE;
				}
				FindClose(handle);
			}
		}
		return 0;
	}
	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
	case ERROR_SHARING_BUFFER_EXCEEDED:
		errno = EACCES;
		break;
	case ERROR_BUFFER_OVERFLOW:
		errno = ENAMETOOLONG;
		break;
	case ERROR_NOT_ENOUGH_MEMORY:
		errno = ENOMEM;
		break;
	default:
		errno = ENOENT;
		break;
	}
	return -1;
}

/* We provide our own lstat/fstat functions, since the provided
 * lstat/fstat functions are so slow. These stat functions are
 * tailored for Git's usage (read: fast), and are not meant to be
 * complete. Note that Git stat()s are redirected to mingw_lstat()
 * too, since Windows doesn't really handle symlinks that well.
 */
static int do_stat_internal(int follow, const char *file_name, struct stat *buf)
{
	int namelen;
	char alt_name[PATH_MAX];

	if (!do_lstat(follow, file_name, buf))
		return 0;

	/* if file_name ended in a '/', Windows returned ENOENT;
	 * try again without trailing slashes
	 */
	if (errno != ENOENT)
		return -1;

	namelen = strlen(file_name);
	if (namelen && file_name[namelen-1] != '/')
		return -1;
	while (namelen && file_name[namelen-1] == '/')
		--namelen;
	if (!namelen || namelen >= PATH_MAX)
		return -1;

	memcpy(alt_name, file_name, namelen);
	alt_name[namelen] = 0;
	return do_lstat(follow, alt_name, buf);
}

int mingw_lstat(const char *file_name, struct stat *buf)
{
	return do_stat_internal(0, file_name, buf);
}
int mingw_stat(const char *file_name, struct stat *buf)
{
	return do_stat_internal(1, file_name, buf);
}

int mingw_fstat(int fd, struct stat *buf)
{
	HANDLE fh = (HANDLE)_get_osfhandle(fd);
	BY_HANDLE_FILE_INFORMATION fdata;

	if (fh == INVALID_HANDLE_VALUE) {
		errno = EBADF;
		return -1;
	}
	/* direct non-file handles to MS's fstat() */
	if (GetFileType(fh) != FILE_TYPE_DISK)
		return _fstati64(fd, buf);

	if (GetFileInformationByHandle(fh, &fdata)) {
		buf->st_ino = 0;
		buf->st_gid = 0;
		buf->st_uid = 0;
		buf->st_nlink = 1;
		buf->st_mode = file_attr_to_st_mode(fdata.dwFileAttributes);
		buf->st_size = fdata.nFileSizeLow |
			(((off_t)fdata.nFileSizeHigh)<<32);
		buf->st_dev = buf->st_rdev = 0; /* not used by Git */
		buf->st_atime = filetime_to_time_t(&(fdata.ftLastAccessTime));
		buf->st_mtime = filetime_to_time_t(&(fdata.ftLastWriteTime));
		buf->st_ctime = filetime_to_time_t(&(fdata.ftCreationTime));
		return 0;
	}
	errno = EBADF;
	return -1;
}

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	/* localtime() in MSVCRT.DLL is thread-safe, but not reentrant */
	memcpy(result, localtime(timep), sizeof(struct tm));
	return result;
}

struct group *getgrent (void)
{
	return (struct group *) NULL;
}

void setgrent (void)
{
}

void endgrent (void)
{
}

struct passwd *getpwent (void)
{
	return (struct passwd *) NULL;
}

void setpwent (void)
{
}

void endpwent (void)
{
}

int xutftowcsn(wchar_t *wcs, const char *utfs, size_t wcslen, int utflen)
{
	int upos = 0, wpos = 0;
	const unsigned char *utf = (const unsigned char*) utfs;
	if (!utf || !wcs || wcslen < 1) {
		errno = EINVAL;
		return -1;
	}
	/* reserve space for \0 */
	wcslen--;
	if (utflen < 0)
		utflen = INT_MAX;

	while (upos < utflen) {
		int c = utf[upos++] & 0xff;
		if (utflen == INT_MAX && c == 0)
			break;

		if (wpos >= (int) wcslen) {
			wcs[wpos] = 0;
			errno = ERANGE;
			return -1;
		}

		if (c < 0x80) {
			/* ASCII */
			wcs[wpos++] = c;
		} else if (c >= 0xc2 && c < 0xe0 && upos < utflen &&
				(utf[upos] & 0xc0) == 0x80) {
			/* 2-byte utf-8 */
			c = ((c & 0x1f) << 6);
			c |= (utf[upos++] & 0x3f);
			wcs[wpos++] = c;
		} else if (c >= 0xe0 && c < 0xf0 && upos + 1 < utflen &&
				!(c == 0xe0 && utf[upos] < 0xa0) && /* over-long encoding */
				(utf[upos] & 0xc0) == 0x80 &&
				(utf[upos + 1] & 0xc0) == 0x80) {
			/* 3-byte utf-8 */
			c = ((c & 0x0f) << 12);
			c |= ((utf[upos++] & 0x3f) << 6);
			c |= (utf[upos++] & 0x3f);
			wcs[wpos++] = c;
		} else if (c >= 0xf0 && c < 0xf5 && upos + 2 < utflen &&
				wpos + 1 < (int) wcslen &&
				!(c == 0xf0 && utf[upos] < 0x90) && /* over-long encoding */
				!(c == 0xf4 && utf[upos] >= 0x90) && /* > \u10ffff */
				(utf[upos] & 0xc0) == 0x80 &&
				(utf[upos + 1] & 0xc0) == 0x80 &&
				(utf[upos + 2] & 0xc0) == 0x80) {
			/* 4-byte utf-8: convert to \ud8xx \udcxx surrogate pair */
			c = ((c & 0x07) << 18);
			c |= ((utf[upos++] & 0x3f) << 12);
			c |= ((utf[upos++] & 0x3f) << 6);
			c |= (utf[upos++] & 0x3f);
			c -= 0x10000;
			wcs[wpos++] = 0xd800 | (c >> 10);
			wcs[wpos++] = 0xdc00 | (c & 0x3ff);
		} else if (c >= 0xa0) {
			/* invalid utf-8 byte, printable unicode char: convert 1:1 */
			wcs[wpos++] = c;
		} else {
			/* invalid utf-8 byte, non-printable unicode: convert to hex */
			static const char *hex = "0123456789abcdef";
			wcs[wpos++] = hex[c >> 4];
			if (wpos < (int) wcslen)
				wcs[wpos++] = hex[c & 0x0f];
		}
	}
	wcs[wpos] = 0;
	return wpos;
}

struct passwd *
getpwnam (__attribute__((unused)) const char *name)
{
	return (struct passwd*) NULL;
}
