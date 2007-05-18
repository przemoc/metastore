#include <stdint.h>

extern int verbosity;
#define MSG_NORMAL    0
#define MSG_DEBUG     1
#define MSG_QUIET    -1
#define MSG_CRITICAL -2
int msg(int level, const char *fmt, ...);

void *xmalloc(size_t size);
char *xstrdup(const char *s);
void binary_print(const char *s, ssize_t len);
void xfwrite(const void *ptr, size_t size, FILE *stream);

void write_int(uint64_t value, size_t len, FILE *to);
void write_binary_string(const char *string, size_t len, FILE *to);
void write_string(const char *string, FILE *to);

uint64_t read_int(char **from, size_t len, const char *max);
char *read_binary_string(char **from, size_t len, const char *max);
char *read_string(char **from, const char *max);

