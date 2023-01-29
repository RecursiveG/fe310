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

#endif  // __PRELUDE_H__
