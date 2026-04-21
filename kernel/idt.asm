[BITS 32]

global idt_load
global timer_handler

extern pit_tick

section .text

timer_handler:
    pusha
    call pit_tick
    popa
    iret

idt_load:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    lidt [eax]
    pop ebp
    ret