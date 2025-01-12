CC=clang
# Use LTO to reduce size
CFLAGS=--target=riscv32 -mcpu=sifive-e31 -nostdlib -mno-relax -std=c2x -flto
LD=ld.lld
LDFLAGS=

program.elf program.map: start.o prelude.o main.o interrupts.o libclang_rt.builtins-riscv32.a fe310.lds linker_symbols.h Makefile
	$(LD) $(LDFLAGS) -T fe310.lds start.o prelude.o main.o interrupts.o -L. -lclang_rt.builtins-riscv32 -o program.elf -Map=program.map

start.o : start.c linker_symbols.h registers.h Makefile
	$(CC) $(CFLAGS) -c start.c -o start.o

prelude.o : prelude.c prelude.h linker_symbols.h registers.h Makefile
	$(CC) $(CFLAGS) -c prelude.c -o prelude.o

interrupts.o : interrupts.c interrupts.h registers.h Makefile
	$(CC) $(CFLAGS) -c interrupts.c -o interrupts.o

main.o : main.c registers.h Makefile
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	$(RM) *.o *.elf *.map

program: program.elf
	openocd -f board/sifive-hifive1-revb.cfg -c "program program.elf verify reset exit"
