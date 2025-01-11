#include <stdint.h>
#include <stdarg.h>

#include "interrupts.h"
#include "registers.h"
#include "linker_symbols.h"
#include "prelude.h"

/***************************
 *         Globals         *
 ***************************/

#define HEAP_SIZE   4096
#define CANARY_BYTE 0x5a

static uint32_t _heap_base;
static uint32_t _heap_tail;

struct heap_block_header {
    unsigned int allocated : 1;
    uint16_t len : 15;
} __attribute__((packed));
_Static_assert(sizeof(struct heap_block_header) == 2, "heap block header size err");

/****************************
 * Common library functions *
 ****************************/

// Emit byte to console without converting newline.
static int putbyte(int c) {
    uint32_t full;
    c &= 0xff;
    do {
        // amoor.w rd, rs1, rs2
        // rs1 holds the memory address.
        // for whatever reason, it needs to be written as
        // amoor.w rd, rs2, (rs1)
        __asm__ volatile("amoor.w %0, %2, (%1)\n"
            : "=r"(full)
            : "r"(REG_UART0_TXDATA), "r"(c)
        );
    } while (full & 0x8000'0000);
    return c;
}

int putchar(int c) {
    if (c == '\n') putbyte('\r');
    return putbyte(c);
}

// Returns string length
static int putstr(const char* str) {
    int len = 0;
    while (*str) {
        putchar(*str++);
        len++;
    }
    return len;
}

int puts(const char *str) {
    int len = putstr(str);
    putchar('\n');
    return len;
}

unsigned int sleep(unsigned int seconds) {
    int64_t timeout_tick = REG64(CLINT_MTIME);
    // 32768 ticks = 1 second
    while (seconds --> 0) timeout_tick += 32768;
    // Busy loop until the time.
    while ((int64_t)REG64(CLINT_MTIME) - timeout_tick < 0);
    return 0;
}

void* malloc(unsigned int size) {
    void* ptr = (void*) _heap_base;
    struct heap_block_header *header, *new_header;

    // find a large enough empty block
    while (1) {
        if ((uint32_t)ptr >= _heap_tail) return NULL;
        header = (struct heap_block_header*) ptr;
        if (header->allocated == 0 && header->len >= size) break;
        ptr += sizeof(struct heap_block_header) + header->len;
    }

    // Split the block if viable
    if (header->len >= size + 3) {
        new_header = (struct heap_block_header*)(ptr + sizeof(struct heap_block_header) + size);
        new_header->allocated = 0;
        new_header->len = header->len - size - sizeof(struct heap_block_header);
        header->len = size;
    }

    // Mark as allocated
    header->allocated = 1;
    return ptr + sizeof(struct heap_block_header);
}

void free(void* ptr) {
    struct heap_block_header *prev = NULL;
    struct heap_block_header *header = NULL;
    struct heap_block_header *next = NULL;
    void* tmp;

    if ((uint32_t) ptr < _heap_base || (uint32_t)ptr >= _heap_tail) return;

    // Finds the block ptr points to, and the one before it.
    tmp = (void*) _heap_base;
    ptr -= sizeof(struct heap_block_header);
    while (1) {
        if ((uint32_t)tmp >= _heap_tail) return;

        if (tmp == ptr) {
            header = (struct heap_block_header*)tmp;
            break;
        } else {
            prev = (struct heap_block_header*)tmp;
            tmp += sizeof(struct heap_block_header) + prev->len;
        }
    }

    // Find next block if exists
    if (((uint32_t)tmp) + sizeof(struct heap_block_header) + header->len + sizeof(struct heap_block_header) < _heap_tail) {
        next = (struct heap_block_header*)(tmp + sizeof(struct heap_block_header) + header->len);
    }

    // Mark as unallocated and try to merge
    header->allocated = 0;
    if (next != NULL && next->allocated == 0) {
        header->len += sizeof(struct heap_block_header) + next->len;
    }
    if (prev != NULL && prev->allocated == 0) {
        prev->len += sizeof(struct heap_block_header) + header->len;
    }
}

void* memcpy(void* dst, const void* src, unsigned int n) {
    void* orig_dst = dst;
    unsigned int copied = 0;

    while (copied != n) {
        *(char*)dst = *(char*)src;
        dst++;
        src++;
        copied++;
    }

    return orig_dst;
}

void* memset(void* dst, int data, size_t count) {
    char* ptr = dst;
    while (count-- > 0) {
        *(ptr++) = (char) data;
    }
    return dst;
}

char* itoa(int n, char* buf) {
    char tmp[10];

    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    if (n == -2147483648) {
        memcpy(buf, "-2147483648", 12);
        return buf;
    }

    int is_negative = n < 0;
    if (is_negative) n = -n;

    int len = 0;
    while (n > 0) {
        tmp[len++] = '0' + (n % 10);
        n = n / 10;
    }

    int to_idx = 0;
    if (is_negative) {
        buf[0] = '-';
        to_idx = 1;
    }

    while (len --> 0) {
        buf[to_idx++] = tmp[len];
    }
    buf[to_idx] = '\0';

    return buf;
}

size_t strlen(const char *s) {
    const char* sbegin = s;
    while(*s != '\0') s++;
    return s-sbegin;
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = 0;
    for (const char* ptr = format;; ptr++) {
        char ch = *ptr;
        char next = *(ptr+1);
        if (ch == '\0') {
            break;
        } else if (ch == '%') {
            if (next == 's') {
                ret += putstr(va_arg(args, const char*));
                ptr++;
            } else if (next == 'd') {
                char* nbr = (char*) malloc(256);
                itoa(va_arg(args, int), nbr);
                ret += putstr(nbr);
                free(nbr);
                ptr++;
            } else if (next == 'f') {
                char* buf = (char*) malloc(256);
                int len = double2str(va_arg(args, double), buf, 256);
                if (len <= 0) {
                    ret += putstr("[float err]");
                } else {
                    ret += putstr(buf);
                }
                free(buf);
                ptr++;
            } else {
                va_arg(args, int);
                putchar(ch);
                ret++;
            }
        } else {
            putchar(ch);
            ret++;
        }
    }
    va_end(args);
    return ret;
}

int double2str(double f, char* buf, size_t len) {
    if (len < 2) return 0;

    int p = 0;
    if (f < 0) {
        buf[p++] = '-';
        f = -f;
        if (p >= len) return 0;
    }

    int upper_digits = 0;
    double tmp = f;
    while (tmp >= 1) {
        upper_digits++;
        tmp /= 10.0;
    }

    if (upper_digits == 0) {
        buf[p++] = '0';
        if (p >= len) return 0;
    }
    
    for (int i=0; i< upper_digits; ++i) {
        tmp *= 10.0;
        int d = 0;
        while (tmp >= 1) {
            d += 1;
            tmp -= 1;
        }
        buf[p++] = '0' + d;
        if (p >= len) return 0;
    }

    buf[p++] = '.';
    if (p >= len) return 0;
    do {
        tmp *= 10;
        int d = 0;
        while (tmp >= 1 - 1e-10) {
            d += 1;
            tmp -= 1;
        }
        buf[p++] = '0' + d;
        if (p >= len) return 0;
    } while(tmp > 1e-10);
    buf[p] = '\0';
    return p;
}

void halt(const char* msg) {
    // Disable interrupts
    unset_mstatus_mie();

    putstr("halt: ");
    puts(msg);
    while (1) {}
}

void fatal(const char* format, ...) {
    // Disable interrupts
    unset_mstatus_mie();

    va_list args;
	va_start(args, format);
    putstr("halt: ");
    printf(format, args);
    while (1) {}
}

void check_heap_smash() {
    const char* ptr = (const char*)_heap_tail;
    for (int i = 0; i < 16; ++i) {
        if (*(ptr++) != CANARY_BYTE) {
            halt("heap smashed!");
        }
    }
}

/*************************
 *        Prelude        *
 *************************/

static void _init_heap(void) {
    // Heap memory are allocated in blocks:
    // [2 bytes header][N bytes]
    // Low 15 bits of the header is N.
    // If the block is allocated, the MSB of the header is 1.

    _heap_base = (uint32_t) &_lds_bss_end;
    _heap_tail = _heap_base + sizeof(struct heap_block_header) + HEAP_SIZE;

    struct heap_block_header* header = (struct heap_block_header*) _heap_base;
    header->allocated = 0;
    header->len = HEAP_SIZE;

    // Put 16 bytes of canary at the end of the heap.
    memset((void*)_heap_tail, CANARY_BYTE, 16);
}

// Prepare runtime for the main function.
void _prelude(void) {
    _init_heap();
    _init_interrupts();

    // Call main(). TODO print return value.
    int main(void);
    // And call it.
    main();
    puts("main() returned");
    while(1) { }
}
