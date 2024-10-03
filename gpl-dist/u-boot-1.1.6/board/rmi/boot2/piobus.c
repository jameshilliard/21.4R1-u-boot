#include "linux/types.h"
#include "common.h"
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
#include "rmi/pcmcia.h"

#define MAX_CHIP_SELECTS 10

/* Currently these are the devices supported on PIO bus */
struct pio_dev cpld_i2c_dev;
struct pio_dev cpld_dev;
struct pio_dev cfi_flash_dev;
struct pio_dev nand_flash_dev;
struct pio_dev pcmcia_dev;


void rmi_piobus_setup(struct rmi_board_info * rmib)
{
	struct piobus_info *pio;

	pio = &rmib->pio;

	memset(pio, 0, sizeof(struct piobus_info));

	pio->nand_flash = 1;
	pio->cfi_flash = 1;
	pio->pcmcia = 1;

	if(rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_III || 
			rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_V )
		pio->pcmcia = 0;

	/* All XLR boards have cpld_i2c device */
	if((rmib->maj >= RMI_PHOENIX_BOARD_ARIZONA_I) &&
			(rmib->maj <= RMI_PHOENIX_BOARD_ARIZONA_V)) {
		pio->cpld_i2c = 1;
		/* XLR boards don't support nand flash */
		pio->nand_flash = 0; 
	}
	/* All RMI boards have cpld device */
	pio->cpld = 1;

	/* XLS LTE board does not CFI flash */
	if(rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_VIII)
		pio->cfi_flash = 0;

	/* Initialize the piobus devices with the default values */
	memset(&cpld_i2c_dev, 0, sizeof(struct pio_dev));
	memset(&cpld_dev, 0, sizeof(struct pio_dev));
	memset(&cfi_flash_dev, 0, sizeof(struct pio_dev));
	memset(&nand_flash_dev, 0, sizeof(struct pio_dev));
	memset(&pcmcia_dev, 0, sizeof(struct pio_dev));

	if(pio->cpld_i2c) {
		cpld_i2c_dev.enabled = 1;
		cpld_i2c_dev.chip_sel = CS_CPLD_I2C;
		cpld_i2c_dev.offset = (MB(16 + 8 + 1) + KB(64));
		cpld_i2c_dev.size = KB(64);
	}

	if(pio->cpld) {
		cpld_dev.enabled = 1;
		cpld_dev.chip_sel = CS_CPLD;
		cpld_dev.offset = (MB(16 + 8 + 1));
		cpld_dev.size = MB(1);
	}

	if(pio->cfi_flash) {
		cfi_flash_dev.enabled = 1;
		/* This may be changed later depending on the flash used for booting*/
		cfi_flash_dev.chip_sel = CS_BOOT_FLASH;
		cfi_flash_dev.offset = MB(0);
		cfi_flash_dev.size = MB(16);
	}

	if(pio->pcmcia) {
		pcmcia_dev.enabled = 1;
		pcmcia_dev.chip_sel = CS_PCMCIA;
		pcmcia_dev.offset = MB(16);
		pcmcia_dev.size = MB(8);
	}

	if(pio->nand_flash) {
		nand_flash_dev.enabled = 1;
		/* This may be changed later depending on the flash used for booting*/
		nand_flash_dev.chip_sel = CS_BOOT_FLASH;
		nand_flash_dev.offset = MB(16) + MB(8);
		nand_flash_dev.size = MB(1);
	}
}

void rmi_tweak_pio_cs(struct rmi_board_info * rmib)
{
	phoenix_reg_t *mmio = phoenix_io_mmio(PHOENIX_IO_GPIO_OFFSET);
	phoenix_reg_t value;

	rmib->bfinfo.primary_flash_type = FLASH_TYPE_NOR;
	rmib->bfinfo.secondary_flash_type = FLASH_TYPE_NONE;
	if (chip_processor_id_xls()) {
		value = phoenix_read_reg(mmio, 21);
		if ( (value >> 16) & 0x01) {
			/* If booted from nand, chip select for nand flash is 
			   CS_BOOT_FLASH(0)  and chip select for nor flash is 
			   CS_SECONDARY_FLASH(2)
			 */
			nand_flash_dev.chip_sel = CS_BOOT_FLASH;
			cfi_flash_dev.chip_sel = CS_SECONDARY_FLASH;
			rmib->bfinfo.primary_flash_type = FLASH_TYPE_NAND;
			if(rmib->pio.cfi_flash)
				rmib->bfinfo.secondary_flash_type = FLASH_TYPE_NOR;
			else
				rmib->bfinfo.secondary_flash_type = FLASH_TYPE_NONE;
		} else {
			/* If booted from nor, chip select for nor flash is 
			   CS_BOOT_FLASH(0) and chip select for nand flash is 
			   CS_SECONDARY_FLASH(2)
			*/
			nand_flash_dev.chip_sel = CS_SECONDARY_FLASH;
			cfi_flash_dev.chip_sel = CS_BOOT_FLASH;
			rmib->bfinfo.primary_flash_type = FLASH_TYPE_NOR;
#ifdef TRIBECA
                        rmib->bfinfo.secondary_flash_type = FLASH_TYPE_NONE;
#else
			rmib->bfinfo.secondary_flash_type = FLASH_TYPE_NAND;
#endif
		}
	}
}

static inline void setup_chip_sel(phoenix_reg_t *mmio, uint32_t csl, 
		uint32_t bar, uint32_t mask)
{
	cpu_write_reg(mmio, reg_offset(csl, FLASH_CSBASE_ADDR_REG),bar);
	cpu_write_reg(mmio, reg_offset(csl, FLASH_CSADDR_MASK_REG),mask);

};


void rmi_init_piobus(struct rmi_board_info * rmib)
{
	struct piobus_info *pio;
	phoenix_reg_t *bridge_mmio = phoenix_io_mmio(PHOENIX_IO_BRIDGE_OFFSET);
	phoenix_reg_t *iobus_mmio  = phoenix_io_mmio(PHOENIX_IO_FLASH_OFFSET);

	uint32_t bar = 0;
	uint32_t mask = 0, size;
	uint32_t en = 0x1;
	uint32_t value = 0;
	uint32_t iobus_addr_space_start = 0;
	uint32_t iobus_addr_space_size  = 0;
	unsigned long cs_base = 0;
	unsigned int chip_sel = 0;
	int i = 0;


	pio = &rmib->pio;

	iobus_addr_space_start = PIO_BASE_PHYS;
	iobus_addr_space_size = PIO_BASE_SIZE;

	/* program the bridge */
	bar = ((iobus_addr_space_start >> 24) << 16);
	mask = ( (iobus_addr_space_size >> 24) - 1 ) << 5;
	cpu_write_reg(bridge_mmio, FLASH_BAR, (bar | mask | en));

	rmi_add_phys_region((uint64_t)iobus_addr_space_start, 
				(uint64_t)iobus_addr_space_size, 
				FLASH_CONTROLLER_SPACE);

	cs_base = iobus_addr_space_start & (iobus_addr_space_size - 1);

	/* First map all chip selects to upper 64kb of the iobus_addr_space region */
	value = cs_base + iobus_addr_space_size - KB(64);
	for (i = 0; i < MAX_CHIP_SELECTS; i++) {
		setup_chip_sel(iobus_mmio,i,(value>>16),0);
	}

	/* map chip selects for available devices */
	if(pcmcia_dev.enabled) {
		bar = pcmcia_dev.offset;
		size = pcmcia_dev.size;

		mask = ((size / KB(64)) - 1) &0xffff;
		chip_sel = pcmcia_dev.chip_sel;

		setup_chip_sel(iobus_mmio,chip_sel,(bar>>16),mask);
		pcmcia_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
		pcmcia_dev.iobase = (unsigned long)
					phys_to_kseg1(pcmcia_dev.iobase);
	}
	if(nand_flash_dev.enabled) {
		bar = nand_flash_dev.offset;
		size = nand_flash_dev.size;

		mask = ((size / KB(64)) - 1) &0xffff;
		chip_sel = nand_flash_dev.chip_sel;

		setup_chip_sel(iobus_mmio,chip_sel,(bar>>16),mask);
		nand_flash_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
		nand_flash_dev.iobase = (unsigned long)
					phys_to_kseg1(nand_flash_dev.iobase);
	}

	if(cfi_flash_dev.enabled) {
		bar = cfi_flash_dev.offset;
		size = cfi_flash_dev.size;

		mask = ((size / KB(64)) - 1) &0xffff;
		chip_sel = cfi_flash_dev.chip_sel;

		setup_chip_sel(iobus_mmio,chip_sel,(bar>>16),mask);
		cfi_flash_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
		cfi_flash_dev.iobase = (unsigned long)
					phys_to_kseg1(cfi_flash_dev.iobase);
	}

	if(cpld_dev.enabled) {
		bar = cpld_dev.offset;
		size = cpld_dev.size;

		mask = ((size / KB(64)) - 1) &0xffff;
		chip_sel = cpld_dev.chip_sel;

		setup_chip_sel(iobus_mmio,chip_sel,(bar>>16),mask);
		cpld_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
		cpld_dev.iobase = (unsigned long)
					phys_to_kseg1(cpld_dev.iobase);
	}
	if(cpld_i2c_dev.enabled) {
		bar = cpld_i2c_dev.offset;
		size = cpld_i2c_dev.size;

		mask = ((size / KB(64)) - 1) &0xffff;
		chip_sel = cpld_i2c_dev.chip_sel;

		setup_chip_sel(iobus_mmio,chip_sel,(bar>>16),mask);
		cpld_i2c_dev.iobase = (unsigned long)(PIO_BASE_PHYS + bar);
		cpld_i2c_dev.iobase = (unsigned long)
					phys_to_kseg1(cpld_i2c_dev.iobase);
	}

}



void rmi_init_piobus_devices(struct rmi_board_info * rmib)
{
	struct piobus_info *pio;

	pio = &rmib->pio;

	/* DO NOT CHANGE THE ORDER OF THESE INITs*/
	if(pio->cpld) {
#ifdef TRIBECA
        extern void fx_bcpld_init(void);
        extern void fx_bcpld_init_dev(void);
        fx_bcpld_init();
        fx_bcpld_init_dev();
#else
		rmi_cpld_init();
#endif
    }
        /*
         * The RMI Eval boards use the RMI Flash Driver but for other boards,
         * init is done later with the UBOOT CFI Flash Driver
         */      
#ifdef RMI_EVAL_BOARD
	if(pio->nand_flash)
		rmi_nand_flash_init();
	if(pio->cfi_flash)
		rmi_cfi_flash_init();
#endif
	if(pio->pcmcia)
		rmi_pcmcia_init();
}
