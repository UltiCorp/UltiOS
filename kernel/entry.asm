[BITS 32]

MBOOT_MAGIC   equ 0x1BADB002
MBOOT_FLAGS   equ 0x00
MBOOT_CHECKSUM equ -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot
dd MBOOT_MAGIC
dd MBOOT_FLAGS
dd MBOOT_CHECKSUM

section .text
global _start
extern kernel_main

_start:
    call kernel_main
    hlt
