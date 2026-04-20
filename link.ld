ENTRY(_start)
SECTIONS {
    . = 0x100000;
    .text : {
        */entry.o(.multiboot)
        */entry.o(.text)
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}
