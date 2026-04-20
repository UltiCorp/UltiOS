all: run

kernel:
	nasm -f elf32 kernel/entry.asm -o kernel/entry.o
	gcc -m32 -ffreestanding -fno-pie -c kernel/kernel.c -o kernel/kernel.o
	ld -m elf_i386 -T link.ld kernel/entry.o kernel/kernel.o -o iso/boot/kernel.bin

run: kernel
	qemu-system-i386 -kernel iso/boot/kernel.bin -m 32 -bios /usr/share/seabios/bios.bin

clean:
	rm -f kernel/*.o iso/boot/kernel.bin
