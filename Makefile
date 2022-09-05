CC=clang
CFLAGS=--target=riscv32 -mcpu=sifive-e31 -nostdlib -mno-relax -std=c2x
LD=ld.lld
LDFLAGS=

program.elf program.map: start.o main.o fe310.lds linker_symbols.h Makefile
	$(LD) $(LDFLAGS) -T fe310.lds start.o main.o -o program.elf -Map=program.map

start.o : start.c linker_symbols.h registers.h Makefile
	$(CC) $(CFLAGS) -c start.c -o start.o

main.o : main.c registers.h Makefile
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	$(RM) *.o *.elf *.map

program: program.elf
	openocd -f board/sifive-hifive1-revb.cfg -c "program program.elf verify reset exit"
