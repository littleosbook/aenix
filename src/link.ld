ENTRY(loader)

SECTIONS
{
    . = 0xC0100000;
    kernel_virtual_start = .;
    kernel_physical_start = . - 0xC0000000;

    .text ALIGN (0x1000) : AT(ADDR(.text)-0xC0000000)
    {
        *(.text)
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata)-0xC0000000)
    {
        *(.rodata*)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data)-0xC0000000)
    {
        *(.data)
    }

    .bss ALIGN (0x1000) : AT(ADDR(.bss)-0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }

    kernel_virtual_end = .;
    kernel_physical_end = . - 0xC0000000;
}
