ENTRY(_start)
OUTPUT_FORMAT("elf32-bigmips", "elf32-bigmips", "elf32-bigmips")

SECTIONS
{
 . = ALIGN(4);
 .text :
 {
   *(.text)
 }

 . = ALIGN(4);
 .rodata : {
  *(.rdata)
  *(.rodata)
 }

 . = ALIGN(4);
 .data : { *(.data) }

 . = ALIGN(4);
 .sdata : { *(.sdata) }

 _gp = . ;

 __got_start = .;
 .got : { *(.got) }
 __got_end = .;

 .sdata : { *(.sdata) }

 . = ALIGN(32);
 _dtb_start = .;
 .uboot:dtb : { *(.uboot:dtb) }
 _dtb_end = .;

 . = ALIGN(32);
 . = .;
 __u_boot_cmd_start = .;
 .u_boot_cmd : { *(.u_boot_cmd) }
 __u_boot_cmd_end = .;

 uboot_end_data = .;
 num_got_entries = (__got_end - __got_start) >> 2;

 . = ALIGN(16);
 __bss_start = . ;
 .sbss : { *(.sbss) }
 .bss : { *(.bss) }
 __bss_end = . ;
 uboot_end = .;
}
