#ifndef _MINGW_MMAP_H_
#define _MINGW_MMAP_H_

#ifndef PROT_READ
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 1
#define MAP_SHARED MAP_PRIVATE
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include <stdlib.h>

extern void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
extern int munmap(void *start, size_t length);

#endif
