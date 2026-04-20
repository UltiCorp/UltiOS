ENTRY(_start)
SECTIONS {
    . = 0x8000;
    .text : {
        */entry.o(.multiboot)
        */entry.o(.text)
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}