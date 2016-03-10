/**
  * Source: https://github.com/git/git/blob/master/compat/mingw.h
  * License: https://github.com/git/git/blob/master/COPYING (GPL v2)
  */

#ifndef _MINGW_H_
#define _MINGW_H_

#undef __STRICT_ANSI__

#include <io.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

typedef int uid_t;
typedef int gid_t;

struct group {
  char *gr_name;
  gid_t gr_gid;
  char *gr_passwd;
  char **gr_mem;
};

#define S_IFLNK    0120000 /* Symbolic link */
#define S_ISLNK(x) (((x) & S_IFMT) == S_IFLNK)

#define mkdir(a, b) _mkdir(a)

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/*
 * Use mingw specific stat()/lstat()/fstat() implementations on Windows.
 */
#ifndef __MINGW64_VERSION_MAJOR
#define off_t off64_t
#define lseek _lseeki64
#endif

/* use struct stat with 64 bit st_size */
#ifdef stat
#undef stat
#endif
#define stat _stati64
int mingw_lstat(const char *file_name, struct stat *buf);
int mingw_stat(const char *file_name, struct stat *buf);
int mingw_fstat(int fd, struct stat *buf);
#ifdef fstat
#undef fstat
#endif
#define fstat mingw_fstat
#ifdef lstat
#undef lstat
#endif
#define lstat mingw_lstat

#ifndef _stati64
# define _stati64(x,y) mingw_stat(x,y)
#elif defined (_USE_32BIT_TIME_T)
# define _stat32i64(x,y) mingw_stat(x,y)
#else
# define _stat64(x,y) mingw_stat(x,y)
#endif

#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)

#define bcmp(s1 ,s2, n) memcmp((s1), (s2), (size_t)(n))

extern struct tm *localtime_r(const time_t *timep, struct tm *result);

#include "sys/types.h"

struct passwd {
    char    *pw_name;       /* user name */
    char    *pw_passwd;     /* encrypted password */
    int pw_uid;         /* user uid */
    int pw_gid;         /* user gid */
    char    *pw_comment;        /* comment */
    char    *pw_gecos;      /* Honeywell login info */
    char    *pw_dir;        /* home directory */
    char    *pw_shell;      /* default shell */
};

extern void		 endgrent (void);
extern struct group	*getgrent (void);
extern void		 setgrent (void);

extern void		 endpwent (void);
extern struct passwd  *getpwent (void);
extern void		 setpwent (void);

#define S_IFLNK    0120000 /* Symbolic link */

#ifndef S_IRWXG
#define S_IRGRP (S_IREAD  >> 3)
#define S_IWGRP (S_IWRITE >> 3)
#define S_IXGRP (S_IEXEC  >> 3)
#endif
#ifndef S_IRWXO
#define S_IROTH (S_IREAD  >> 6)
#define S_IWOTH (S_IWRITE >> 6)
#define S_IXOTH (S_IEXEC  >> 6)
#endif

#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#define S_IFSOCK 0

#endif