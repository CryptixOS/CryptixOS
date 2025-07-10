/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kernelStart)

PHDRS
{
    limine_requests PT_LOAD;
    text    PT_LOAD;
    kernel_init PT_LOAD;
    module_init PT_LOAD;
    rodata  PT_LOAD;
    data    PT_LOAD;
    dynamic PT_DYNAMIC FLAGS((1 << 1) | (1 << 2)) ;
}

SECTIONS
{
    . = 0xffffffff80000000;

    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    text_start_addr = .;
    .text : { *(.text .text.*) } :text
    text_end_addr = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    kernel_init_start_addr = .;
    .kernel_init : { *(.kernel_init .kernel_init.*) } :kernel_init
    kernel_init_end_addr = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    module_init_start_addr = .;
    .module_init : { KEEP(*(.module_init .module_init.*)) } :module_init
    module_init_end_addr = .;

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    rodata_start_addr = .;
    .rodata : { *(.rodata .rodata.*) } :rodata
    rodata_end_addr = .;

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    data_start_addr = .;
    .data : { *(.data .data.*) } :data

    .dynamic : {
        *(.dynamic)
    } :data :dynamic

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    /* If you need, for example, .init_array and .fini_array, those should be placed */
    /* above this. */
    .bss : { *(COMMON) *(.bss .bss.*) } :data
    data_end_addr = .;

    /* Discard .note.* and .eh_frame since they may cause issues on some hosts. */
    /DISCARD/ : { *(.eh_frame*) *(.note .note.*) }
}
