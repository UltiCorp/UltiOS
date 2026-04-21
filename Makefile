all: run

kernel:
	nasm -f elf32 kernel/entry.asm -o kernel/entry.o
	nasm -f elf32 kernel/idt.asm -o kernel/idt.o
	gcc -m32 -ffreestanding -fno-pie -c kernel/kernel.c -o kernel/kernel.o
	ld -m elf_i386 -T link.ld kernel/entry.o kernel/idt.o kernel/kernel.o -o kernel/kernel_linked.bin
	objcopy -O binary kernel/kernel_linked.bin kernel/kernel_raw.bin

run: kernel
	dd if=/dev/zero of=/home/ulti/ultios/os.img bs=512 count=2880
	dd if=/home/ulti/ultios/boot.bin of=/home/ulti/ultios/os.img bs=512 conv=notrunc seek=0
	dd if=kernel/kernel_raw.bin of=/home/ulti/ultios/os.img bs=512 conv=notrunc seek=1
	qemu-system-i386 -drive format=raw,file=/home/ulti/ultios/os.img -m 32 -bios /usr/share/seabios/bios.bin

clean:
	rm -f kernel/*.o kernel/kernel_linked.bin kernel/kernel_raw.bin
