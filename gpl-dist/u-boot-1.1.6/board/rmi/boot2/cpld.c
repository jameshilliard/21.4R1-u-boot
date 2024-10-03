#include "linux/types.h"
#include "configs/rmi_boards.h"
#include "rmi/shared_structs.h"
#include "rmi/board.h"
#include "rmi/physaddrspace.h"
#include "rmi/bridge.h"
#include "rmi/pioctrl.h"
#include "rmi/cpld.h"
#include "rmi/flashdev.h"
#include "rmi/cfiflash.h"
#include "rmi/gpio.h"

extern struct pio_dev cpld_i2c_dev;
extern struct pio_dev cpld_dev;
extern struct pio_dev cfi_flash_dev;
extern struct pio_dev nand_flash_dev;

void rmi_cpld_init(void)
{
	unsigned long iobase;
	unsigned long size;
	unsigned int val = 0;
	phoenix_reg_t *mmio = 0;

	iobase = cpld_dev.iobase;
	size = cpld_dev.size;

	mmio = (phoenix_reg_t *)iobase;
	if(chip_processor_id_xls())
		val = cpu_read_reg(mmio,CPLD_MISC_CTRL_REG_XLS);
	else
		val = cpu_read_reg(mmio,CPLD_MISC_CTRL_REG);
	val |= 0xffff0000;
	if(chip_processor_id_xls())
		cpu_write_reg(mmio,CPLD_MISC_CTRL_REG_XLS,val);
	else
		cpu_write_reg(mmio,CPLD_MISC_CTRL_REG,val);

}

void cpld_enable_flash_write(unsigned char en)
{
	unsigned long iobase = cpld_dev.iobase;
	unsigned char old_val;
	unsigned char en_bit;
	unsigned long addr;

	/* On the XLS, CPLD enables the
	 * NOR Flash Write by default..
	 */
	if (!chip_processor_id_xls()) {
		en_bit = 6;
		addr = 0x10;
		old_val = phoenix_inb((iobase + addr));
		if(en) {
			phoenix_outb((iobase+addr),old_val | (1 << en_bit));
		}
		else {
			phoenix_outb((iobase+0x10),old_val & ~(1 << en_bit));
		}
	}
}

void cpld_enable_nand_write(unsigned char en)
{
	unsigned long iobase = cpld_dev.iobase;
	unsigned short old_val, new_val;
	unsigned char en_bit;
	unsigned long addr;

	en_bit = 0;
	addr = 0xe;

	old_val = phoenix_inw((iobase + addr));
	if(en) {
		new_val = old_val | (1 << en_bit);
		phoenix_outw((iobase+addr),__swab16(new_val));
	}
	else {
		new_val = old_val & ~(1 << en_bit);
		phoenix_outw((iobase+addr), __swab16(new_val));
	}
}

void cpld_set_pcmcia_mode(unsigned char io)
{
	unsigned long iobase = cpld_dev.iobase;
	unsigned long  cpld_offset = 0;
	unsigned short old_val     = 0;

	if(chip_processor_id_xls()) {
		cpld_offset = iobase + 0x0e;
		old_val = phoenix_inw(cpld_offset);
		if (io) {
			phoenix_outw(cpld_offset, __swab16(old_val) | 0x1000);
		}
		else {
			phoenix_outw(cpld_offset, __swab16(old_val) & 0xefff);
		}
	}
	else {
		/* XLR */
		cpld_offset = iobase + 0x10;
		old_val = phoenix_inw(cpld_offset);
		if (io) {
			phoenix_outw(cpld_offset, __swab16(old_val) | 0x8000);
		}
		else {
			phoenix_outw(cpld_offset, __swab16(old_val) & ~0x8000);
		}
	}
}

/*
 * On the LiTE board, Jumper Settings control PCIE / USB lane
 * routing logic through CPLD. Read and display these settings
 * here. Take into account that LiTE board can host both XLS4xx
 * and XLS2XX parts. TODO: Add warnings for illegal settings.
 */
void cpld_check_lte_routing(void) {

	 unsigned long iobase, cpld_offset;

	 iobase = (unsigned long)(MB(16 + 8) + KB(128) + KB(128));
	 cpld_offset = phys_to_kseg1 (PIO_BASE_PHYS + iobase);

    unsigned short curr_val    = __swab16(phoenix_inw(cpld_offset)) >> 8;
    unsigned char  xls_2xx     = 0;


    if (chip_processor_id_xls2xx())
        xls_2xx = 1;

    /* printf("JP Configuration=0x%x\n", (curr_val & 0x07)); */

    switch (curr_val & 0x07) {

        case 0x0:
        case 0x4: 
            if (xls_2xx) {
                printf("Enabling Slots[0,1,2,3]\n");
            }
            else {
                printf("Enabling Slots[0,2]\n");
            }
            break;
        case 0x1:
            if (xls_2xx) {
                printf("Enabling Slots[1,2,3] + ExpressCard (no USB)\n");
            }
            else {
                printf("Enabling Slots[2] + ExpressCard (no USB)\n");
            }
            break;
        case 0x2:
            if (xls_2xx) {
                printf("Enabling Slots[0,1,3] + mini-PCIE (no USB)\n");
            }
            else {
                printf("Enabling Slots[0] + mini-PCIE (no USB)\n");
            }
            break;
        case 0x3:
            if (xls_2xx) {
                printf("Enabling Slots[1,3] + mini-PCIE (no USB) + ExpressCard (no USB)\n");
            }
            else {
                printf("Enabling mini-PCIE (no USB) + ExpressCard (no USB)\n");
            }
            break;
        case 0x5:
            if (xls_2xx) {
                printf("Enabling Slots[1,2,3] + ExpressCard (with USB)\n");
            }
            else {
                printf("Enabling Slots[2] + ExpressCard (with USB)\n");
            }
            break;
        case 0x6:
            if (xls_2xx) {
                printf("Enabling Slots[0,1,3] + mini-PCIE (with USB)\n");
            }
            else {
                printf("Enabling Slots[0] + mini-PCIE (with USB)\n");
            }
            break;
        case 0x7:
            if (xls_2xx) {
                printf("Enabling Slots[1,3] + mini-PCIE (with USB) + ExpressCard (with USB)\n");
            }
            else {
                printf("Enabling mini-PCIE (with USB) + ExpressCard (with USB)\n");
            }
            break;
        default:
            printf("Illegal Value for CPLD GP Register\n");
            break;
    }
}
