ENTRY(_start)

SECTIONS
{
    . = 0x7C00;

    _sboot = .;
    
    .stage1 : {
        _sstage1 = .;
        *(.stage1)
        _estage1 = .;
    }

    .stage1_5 : {
        _sstage1_5 = .;
        *(.stage1_5)
        _estage1_5 = .;
    }

    .text : {
        *(.text*)
    }

    .data : {
        *(.data*)
    }

    .rodata : {
        *(.rodata*)
        . = ALIGN(0x200);
    }

    _eboot = .;

    .bss : {
        _sbss = .;
        *(.bss*)
        _ebss = .;
    }

    _sectors = (_eboot - _sboot) / 0x200;
}