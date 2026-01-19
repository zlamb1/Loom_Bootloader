ENTRY(_start)

SECTIONS {
    . = 0x7C00;
    stage1s = .;

    .stage1 : {
        *(.stage1)
    }

    . = ALIGN(512);
    stage1e = .;

    stage2s = .;

    .stage2 : {
        *(.stage2)
        *(.stage2.*)
    }

    . = ALIGN(512);

    stage2e = .;

    stage3s = .;

    .stage3.text : {
        stext = .;
        *(.text)
        *(.text.*)
        etext = .;
    }

    .stage3.data : {
        sdata = .;
        *(.data)
        *(.data.*)
        edata = .;
    }

    .stage3.rodata : {
        srodata = .;
        *(.rodata)
        *(.rodata.*)
        erodata = .;
        . = ALIGN(512);
    }

    stage3e = .;

    .stage3.bss (NOLOAD) : {
        sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        ebss = .;
    }

    s2_sectors = (stage2e - stage2s) / 0x200;
    s3_sectors = (stage3e - stage3s) / 0x200;
}