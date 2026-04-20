; TP-BOS boot stub — Multiboot2, modo protegido 32-bit puro
; Compilar con: nasm -f elf32

bits 32

; ── Multiboot header ─────────────────────────────────────────────────────────
section .multiboot
align 4
    dd 0x1BADB002             ; Magic para Multiboot 1
    dd 0x03                   ; Flags (pide info de memoria y alinea)
    dd -(0x1BADB002 + 0x03)   ; Checksum

; ── GDT propia ────────────────────────────────────────────────────────────────
; GRUB puede dejar una GDT con el segmento CS sin el bit Execute.
; Definimos la nuestra: null, code32 (exec+read), data32 (read+write).
section .data
align 8
gdt_start:
    ; Descriptor 0: null
    dq 0
    ; Descriptor 1: codigo 32-bit, base=0, limite=4GB, DPL=0, Execute+Read
    dw 0xFFFF   ; Limit 15:0
    dw 0x0000   ; Base  15:0
    db 0x00     ; Base  23:16
    db 0x9A     ; Access: Present, DPL=0, S=1, Type=0xA (code exec/read)
    db 0xCF     ; Flags: Gran=1, D/B=1(32bit), Limit 19:16=0xF
    db 0x00     ; Base  31:24
    ; Descriptor 2: datos 32-bit, base=0, limite=4GB, DPL=0, Read+Write
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92     ; Access: Present, DPL=0, S=1, Type=0x2 (data read/write)
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ 0x08
DATA_SEG equ 0x10

; ── Stack ─────────────────────────────────────────────────────────────────────
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; ── Entry point ───────────────────────────────────────────────────────────────
section .text
global _start
extern kernel_main

_start:
    cli

    ; Cargar GDT limpia con segmentos correctos
    lgdt [gdt_descriptor]

    ; Far jump para recargar CS con nuestro descriptor de codigo
    jmp CODE_SEG:.reload_cs
.reload_cs:
    ; Recargar todos los segmentos con nuestro descriptor de datos
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, stack_top

    push 0
    popf

    push ebx
    push eax
    call kernel_main
    add esp, 8

.hang:
    cli
    jmp .hang
