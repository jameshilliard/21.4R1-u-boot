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

struct flash_device cfi_flash;
extern struct rmi_board_info rmi_board;
extern struct pio_dev cfi_flash_dev;

void rmi_cfi_flash_init(void)
{
	struct rmi_board_info * rmib = &rmi_board;

	if(rmib->bfinfo.primary_flash_type == FLASH_TYPE_NOR)
		printf("NOR flash is primary/boot flash\n");
	else if(rmib->bfinfo.secondary_flash_type == FLASH_TYPE_NOR)
		printf("NOR flash is secondary flash\n");


	cfi_flash.iobase = cfi_flash_dev.iobase;
#ifdef RMI_DEBUG
	printf("Detecting CFI flash with iobase %x\n", cfi_flash.iobase);
#endif

	cfi_flash_detect(&cfi_flash);
}
