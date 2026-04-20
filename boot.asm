[BITS 16]
[ORG 0x7C00]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, logo
    call print16

    mov cx, 36
.barra:
    push cx
    mov ah, 0x0E
    mov al, 0xDB
    int 0x10
    mov cx, 30000
.delay:
    loop .delay
    pop cx
    loop .barra

    ; carrega kernel do disco
    mov ah, 0x02
    mov al, 15
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov bx, 0x1000
    mov es, bx
    mov bx, 0
    int 0x13

    ; entra em modo protegido
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected

[BITS 32]
protected:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x1000

print16:
    lodsb
    or al, al
    jz .fim
    mov ah, 0x0E
    int 0x10
    jmp print16
.fim:
    ret

logo:
    db 13, 10, 13, 10, 13, 10, 13, 10, 13, 10, 13, 10, 13, 10, 13, 10
    db "                 _   _   _   _     _    ___    ___ ", 13, 10
    db "                | | | | | | | |_  (_)  / _ \  / __|", 13, 10
    db "                | |_| | | | |  _| | | | (_) | \__ \", 13, 10
    db "                 \___/  |_|  \__| |_|  \___/  |___/", 13, 10
    db 13, 10
    db "                ====================================", 13, 10
    db 13, 10
    db "                ", 0

gdt:
    dq 0
    db 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
    db 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_desc:
    dw 23
    dd gdt

times 510-($-$$) db 0
dw 0xAA55
