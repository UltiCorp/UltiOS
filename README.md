<div align="center">

![Boot Screen](assets/boot.PNG)

# UltiOS

![Assembly](https://img.shields.io/badge/Assembly-NASM-red?style=for-the-badge&logo=assemblyscript)
![C](https://img.shields.io/badge/C-GCC-blue?style=for-the-badge&logo=c)
![QEMU](https://img.shields.io/badge/Emulator-QEMU-orange?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Em%20desenvolvimento-yellow?style=for-the-badge)

A simple operating system built from scratch using Assembly and C.

[English](#english) • [Português](#português)

</div>

---

## English

### About

UltiOS is an experimental operating system built entirely from scratch, without any base OS or framework. The goal is to understand how a computer works at the lowest level — from the first instruction executed at boot to a functional terminal shell.

This is a learning/portfolio project, not a production OS.

### Features

- Custom bootloader written in x86 Assembly (NASM)
- ASCII art logo and animated loading bar at boot
- 32-bit protected mode kernel written in C
- Direct VGA memory writing (0xB8000)
- UltiCMD shell with keyboard input, backspace and enter support

### Screenshots

| Boot | Shell |
|------|-------|
| ![Boot](assets/boot.PNG) | ![Shell](assets/os.PNG) |

### Architecture

```
BIOS
 └── Bootloader (Assembly - 512 bytes)
      └── Logo + Loading bar
          └── Kernel (C - 32-bit protected mode)
               └── UltiCMD Shell
```

### Requirements

- NASM
- GCC (with m32 support)
- QEMU
- GNU Binutils
- SeaBIOS

On Ubuntu/WSL2:
```bash
sudo apt install nasm gcc binutils qemu-system-x86 seabios make
```

### Building and Running

```bash
git clone https://github.com/UltimateStrength/UltiOS.git
cd UltiOS
make
```

### Project Structure

```
UltiOS/
├── boot.asm          # Bootloader (x86 Assembly)
├── link.ld           # Linker script
├── Makefile          # Build automation
├── kernel/
│   ├── entry.asm     # Kernel entry point (Assembly)
│   └── kernel.c      # Kernel + UltiCMD shell (C)
├── iso/
│   └── boot/
│       └── grub/
│           └── grub.cfg
└── assets/
    ├── boot.PNG
    └── os.PNG
```

### How it works

**Bootloader** — The BIOS loads the first 512 bytes of the disk into memory at address `0x7C00` and jumps to it. The bootloader displays the UltiOS logo and an animated loading bar using BIOS interrupts, then hands control to the kernel.

**Kernel** — Runs in 32-bit protected mode. Instead of using BIOS calls (unavailable in protected mode), it writes characters directly to VGA memory at `0xB8000`. Each character takes 2 bytes: one for the ASCII code and one for the color.

**UltiCMD** — Reads keyboard scancodes directly from I/O port `0x60`, converts them to ASCII, and renders them on screen. Supports typing, backspace, and enter.

---

## Português

### Sobre

UltiOS é um sistema operacional experimental construído do zero, sem nenhum OS base ou framework. O objetivo é entender como um computador funciona no nível mais baixo — desde a primeira instrução executada no boot até um terminal shell funcional.

Este é um projeto de aprendizado e portfólio, não um OS de produção.

### Funcionalidades

- Bootloader customizado em Assembly x86 (NASM)
- Logo em ASCII art e barra de loading animada no boot
- Kernel em modo protegido 32-bit escrito em C
- Escrita direta na memória VGA (0xB8000)
- Shell UltiCMD com input de teclado, backspace e enter

### Arquitetura

```
BIOS
 └── Bootloader (Assembly - 512 bytes)
      └── Logo + Barra de loading
          └── Kernel (C - modo protegido 32-bit)
               └── Shell UltiCMD
```

### Requisitos

- NASM
- GCC (com suporte a m32)
- QEMU
- GNU Binutils
- SeaBIOS

No Ubuntu/WSL2:
```bash
sudo apt install nasm gcc binutils qemu-system-x86 seabios make
```

### Como rodar

```bash
git clone https://github.com/UltimateStrength/UltiOS.git
cd UltiOS
make
```

### Como funciona

**Bootloader** — A BIOS carrega os primeiros 512 bytes do disco na memória no endereço `0x7C00` e pula pra ele. O bootloader exibe a logo do UltiOS e uma barra de loading animada usando interrupções da BIOS, depois passa o controle pro kernel.

**Kernel** — Roda em modo protegido 32-bit. Em vez de usar chamadas da BIOS (indisponíveis no modo protegido), escreve caracteres diretamente na memória VGA no endereço `0xB8000`. Cada caractere ocupa 2 bytes: um pro código ASCII e um pra cor.

**UltiCMD** — Lê scancodes do teclado diretamente da porta I/O `0x60`, converte pra ASCII e renderiza na tela. Suporta digitação, backspace e enter.

---

<div align="center">
Made by <a href="https://github.com/UltimateStrength">UltimateStrength</a>
</div>