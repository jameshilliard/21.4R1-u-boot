#include "linux/types.h"
#include "common.h"
#include "configs/rmi_boards.h"
#include "rmi/shared_structs.h"
#include "rmi/system.h"
#include "rmi/board.h"
#include "rmi/physaddrspace.h"
#include "rmi/bridge.h"
#include "rmi/pioctrl.h"
#include "rmi/cpld.h"
#include "rmi/flashdev.h"
#include "rmi/cfiflash.h"
#include "rmi/gpio.h"

extern struct pio_dev pcmcia_dev;

void udelay (unsigned long usec);
#define mdelay(x) udelay((x) * 1000)

enum pcmcia_reg_offsets
{
	PCMCIA_ATTRIB_MEM_BASE 		= 0x200,
	PCMCIA_CFG_OPTION_REG 		= (PCMCIA_ATTRIB_MEM_BASE + 00)
};


enum pcmcia_int_status
{
	ILL_PC_INT   = 0x00040000,
	WERR_INT     = 0x00020000,
	RYBY_INT     = 0x00010000,

	WAIT_TO_INT  = 0x00004000,
	MUTL_CS_INT  = 0x00002000,
	ILL_ADDR_INT = 0x00001000,
	RDY_INT      = 0x00000800,

	WP_INT       = 0x00000400, 
	CD_INT       = 0x00000200,
	RYBY_STS     = 0x00000100,
	RDY_STS      = 0x00000080,
	WP_STS       = 0x00000040,
	CD2_STS      = 0x00000020,
	CD1_STS      = 0x00000010,
	BVD2_STS     = 0x00000008,
	BVD1_STS     = 0x00000004
};

/* Initializing the PHOENIX PCMCIA interface in IO Mode */

static void set_pcmcia_status(unsigned int status)
{
	phoenix_reg_t *mmio  = phoenix_io_mmio(PHOENIX_IO_FLASH_OFFSET);
	int cs = 0;
#ifdef RMI_DEBUG
	printf("%s %x\n",__FUNCTION__,(mmio+reg_offset(cs,FLASH_INT_STATUS_REG)));
#endif
	cpu_write_reg(mmio, reg_offset(cs,FLASH_INT_STATUS_REG),status);
}
static unsigned int get_pcmcia_status(void)
{
	phoenix_reg_t *mmio  = phoenix_io_mmio(PHOENIX_IO_FLASH_OFFSET);
	int cs = 0;
#ifdef RMI_DEBUG
	printf("%s %x\n",__FUNCTION__,(mmio+reg_offset(cs,FLASH_INT_STATUS_REG)));
#endif
	return cpu_read_reg(mmio, reg_offset(cs,FLASH_INT_STATUS_REG));
}

static void set_pcmcia_config(unsigned long config)
{
	phoenix_reg_t *mmio  = phoenix_io_mmio(PHOENIX_IO_FLASH_OFFSET);
	int cs = 0;
#ifdef RMI_DEBUG
	printf("%s %x\n",__FUNCTION__,(mmio+reg_offset(cs,FLASH_INT_MASK_REG)));
#endif
	cpu_write_reg(mmio, reg_offset(cs,FLASH_INT_MASK_REG),config);
}

static void set_pcmcia_timing(unsigned long timing)
{
	phoenix_reg_t *mmio  = phoenix_io_mmio(PHOENIX_IO_FLASH_OFFSET);
	int cs = pcmcia_dev.chip_sel;
#ifdef RMI_DEBUG
	printf("%s %x\n",__FUNCTION__,(mmio+reg_offset(cs,FLASH_CSTIME_PARAMA_REG)));
#endif
	cpu_write_reg(mmio, reg_offset(cs,FLASH_CSTIME_PARAMA_REG),timing);
}


static int pcmcia_reset(unsigned long iobase)
{
	unsigned int status = 0;
	unsigned int ready = (WP_STS | CD2_STS | CD1_STS);
	int retVal = -1;

	printf("[Initializing PCMCIA inferface in IO mode] \n");
	/* Issue a software reset */
	set_pcmcia_config(0xff); 
	mdelay(1000);

	set_pcmcia_status(0xffffffff);	/* Clear all the pending Interrupts */
	mdelay(100);
	status = get_pcmcia_status();
	if(status & ready)
	{
		printf("No PCMCIA card detected.\n");
		retVal = -1;
	}
	else
	{
		printf("PCMCIA card detected.\n");
		retVal = 1;
	}
	mdelay(100);
	
	/*Enable the PCMCIA, Reg access and unmask PCMCIA intr and flash intr */
	set_pcmcia_config(0x33); 
	mdelay(100);

	/* Enable Memory mode so that we can write to Attrib mem */
	cpld_set_pcmcia_mode(0);
	mdelay(100);
        /* IO MODE PRIMARY and Level IRQ */
	phoenix_outb((iobase+PCMCIA_CFG_OPTION_REG),0x42);
	mdelay(100);
	/* Enable IO MODE in the CPLD reg */
	cpld_set_pcmcia_mode(1);
	mdelay(100);

	/* Tune the CS6 timing to support the IO MODE */
	set_pcmcia_timing(0x4F420E22);
	mdelay(1000);
	
	return retVal;
}



int rmi_pcmcia_init(void)
{
	return pcmcia_reset(pcmcia_dev.iobase);
}
