#ifndef __LINKER_SYMBOLS_H
#define __LINKER_SYMBOLS_H

extern unsigned char _lds_stack_size;
extern unsigned char _lds_stack_bottom;
extern unsigned char _lds_stack_top;

extern unsigned char _lds_bss_start;
extern unsigned char _lds_bss_end;

extern unsigned char _lds_data_vma_start;
extern unsigned char _lds_data_vma_end;
extern unsigned char _lds_data_lma_start;

extern unsigned char _lds_text_vma_start;
extern unsigned char _lds_text_vma_end;
extern unsigned char _lds_text_lma_start;

#endif  //__LINKER_SYMBOLS_H