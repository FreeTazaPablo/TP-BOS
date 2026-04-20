ENTRY(_start)

SECTIONS {
    . = 1M;     /* GRUB carga el kernel en 1 MB */

    /* Multiboot header PRIMERO — GRUB escanea los primeros 32 KB */
    .multiboot ALIGN(8) : {
        *(.multiboot)
    }

    .text ALIGN(4K) : {
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }

    /* Descartar secciones que no necesitamos en el binario final */
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note*)
    }
}
