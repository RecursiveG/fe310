#ifndef __PRELUDE_H__
#define __PRELUDE_H__

#include <stddef.h>

int putchar(int c);
int puts(const char *str);
unsigned int sleep(unsigned int seconds);

// returns nullptr if size is 0
void* malloc(unsigned int size);
void free(void* ptr);
void* memcpy(void* dst, const void* src, unsigned int n);
char* itoa(int n, char* buf);
size_t strlen(const char *s);
int printf(const char* format, ...);
void* memset(void* dst, int data, size_t count);

void halt(const char* msg) __attribute__((noreturn));
// Same as halt, but accepts printf-like arguments.
void fatal(const char* format, ...) __attribute__((noreturn));
// Hopefully we haven't smashed the .data segment and still
// have space for a few more stack frames.
void check_heap_smash(void);

#define TRACE() do { printf("TRACE: "__FILE__ ": %d\n", __LINE__); } while (0)

#endif  // __PRELUDE_H__
