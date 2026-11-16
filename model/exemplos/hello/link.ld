/*
 * Linker script mínimo para os exemplos do ISS, no mapa da especificação,
 * seção 2. PROVISÓRIO: o oficial é sw/common/link.ld, da meta M4.1.
 */
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    IMEM (rx) : ORIGIN = 0x00000000, LENGTH = 64K
    DMEM (rw) : ORIGIN = 0x10000000, LENGTH = 128K
}

SECTIONS
{
    .text : { *(.text.init) *(.text .text.*) } > IMEM
    .rodata : { *(.rodata .rodata.* .srodata .srodata.*) } > DMEM
    .data : { *(.data .data.*) } > DMEM
    .sdata : {
        __global_pointer$ = . + 0x800;
        *(.sdata .sdata.*)
    } > DMEM
    .bss : {
        __bss_start = .;
        *(.sbss .sbss.* .bss .bss.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > DMEM
    __stack_top = ORIGIN(DMEM) + LENGTH(DMEM);
}
