#ifndef __PRELUDE_H__
#define __PRELUDE_H__

int putchar(int c);
int puts(const char *str);
unsigned int sleep(unsigned int seconds);

// returns nullptr if size is 0
void* malloc(unsigned int size);
void free(void* ptr);

#endif  // __PRELUDE_H__