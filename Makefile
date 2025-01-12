CC=clang
# Use LTO to reduce size
CFLAGS=--target=riscv32 -mcpu=sifive-e31 -nostdlib -mno-relax -std=c2x -flto
LD=ld.lld
LDFLAGS=
COMMON_DEPS=Makefile linker_symbols.h registers.h prelude.h interrupts.h gpio.h

program.elf program.map: $(COMMON_DEPS) start.o prelude.o main.o interrupts.o gpio.o libclang_rt.builtins-riscv32.a fe310.lds
	$(LD) $(LDFLAGS) -T fe310.lds start.o prelude.o main.o interrupts.o gpio.o -L. -lclang_rt.builtins-riscv32 -o program.elf -Map=program.map

start.o : $(COMMON_DEPS) start.c
	$(CC) $(CFLAGS) -c start.c -o start.o

prelude.o : $(COMMON_DEPS) prelude.c
	$(CC) $(CFLAGS) -c prelude.c -o prelude.o

interrupts.o : $(COMMON_DEPS) interrupts.c
	$(CC) $(CFLAGS) -c interrupts.c -o interrupts.o

gpio.o : $(COMMON_DEPS) gpio.c
	$(CC) $(CFLAGS) -c gpio.c -o gpio.o

main.o : $(COMMON_DEPS) main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	$(RM) *.o *.elf *.map

program: program.elf
	openocd -f board/sifive-hifive1-revb.cfg -c "program program.elf verify reset exit"
