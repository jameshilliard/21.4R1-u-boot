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
#include "micron_nand.h"


extern struct rmi_board_info rmi_board;
extern struct pio_dev nand_flash_dev;

struct flash_device nand_flash;
int nand_flash_detect(struct flash_device *this_flash);

void rmi_nand_flash_init(void)
{
	struct rmi_board_info * rmib = &rmi_board;

	if(rmib->bfinfo.primary_flash_type == FLASH_TYPE_NAND)
		printf("NAND flash is primary/boot flash\n");
	else
		printf("NAND flash is secondary flash\n");

	nand_flash.iobase = nand_flash_dev.iobase;
	nand_flash.chip_select = nand_flash_dev.chip_sel;

#ifdef RMI_DEBUG
	printf("Detecting NAND flash with iobase %x CS %d\n", 
			nand_flash.iobase, nand_flash.chip_select);
#endif

	if((nand_flash_detect(&nand_flash)) != 0) {
		printf("NAND detection failed\n");
	}
}

