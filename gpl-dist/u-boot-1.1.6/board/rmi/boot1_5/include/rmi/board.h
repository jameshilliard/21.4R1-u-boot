/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/
#ifndef __BOARD_H__
#define __BOARD_H__
#include "rmi/shared_structs.h"
#include "rmi/rmi_processor.h"
#include "rmi/device.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"

struct board {
	const char *name;
	struct processor *this_processor;
	int major;
	int minor;
	struct boot1_info *boot1_info;
	int (*init) (struct board *this_board);
	void (*shutdown) (struct board *this_board);
	void (*reboot) (struct board *this_board);
	struct device  *device_list;
	struct env_dev *env_dev;
	void *private_data;
};

#define RMI_PHOENIX_BOARD_ARIZONA_I        1
#define RMI_PHOENIX_BOARD_ARIZONA_II       2
#define RMI_PHOENIX_BOARD_ARIZONA_III      3
#define RMI_PHOENIX_BOARD_ARIZONA_IV       4
#define RMI_PHOENIX_BOARD_ARIZONA_V        5
#define RMI_PHOENIX_BOARD_ARIZONA_VI       6
#define RMI_PHOENIX_BOARD_ARIZONA_VII      7
#define RMI_PHOENIX_BOARD_ARIZONA_VIII     8
#define RMI_PHOENIX_BOARD_ARIZONA_XI       11
#define RMI_PHOENIX_BOARD_ARIZONA_XII      12


static __inline__ int xlr_board_atx_i(void)
{
	return ((boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_I)||
	        (boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_IV));
}

static __inline__ int xlr_board_atx_ii(void)
{
	return boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_II;
}

static __inline__ int xlr_board_atx_ii_b(void)
{
	return  (boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_II) &&
		(boot1_info->board_minor_version == 1);
		
}

static __inline__ int xlr_board_atx_iii(void)
{
	return (boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_III);
}

static __inline__ int xlr_board_atx_iii_256(void)
{
	return (boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_III)
		&& (boot1_info->board_minor_version == 0);
}

static __inline__ int xlr_board_atx_iii_512(void)
{
	return (boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_III)
		&& (boot1_info->board_minor_version == 1);
}

static __inline__ int xlr_board_atx_iv(void)
{
	return boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_IV;
}

static __inline__ int xlr_board_atx_v(void)
{
	return boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_V;
}

static __inline__ int xls_board_atx_vi(void)
{
	return boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_VI;
}

static __inline__ int xls_board_atx_vii(void)
{
	return boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_VII;
}

static __inline__ int xls_board_atx_viii(void)
{
	return boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_VIII;
}

static __inline__ int xls_board_atx_xi_xii(void)
{
	if ((boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_XI) ||
	(boot1_info->board_major_version == RMI_PHOENIX_BOARD_ARIZONA_XII))
		return 1;
	return 0;
}


static __inline__ void attach_device(struct board *this_board, struct device *this_dev)
{
	struct device **p = &this_board->device_list;
	while(*p)
	{
		p = &(*p)->next;
	}
	*p = this_dev;
}
#define attach_device_to_board(this_board,this_processor,this_device,component) \
	if(component_available(this_processor,component)) \
               attach_device(this_board,&this_device)
extern struct board * detect_board(struct processor *this_processor);
void enumerate_on_board_devices(struct board *this_board);
extern struct board *get_board_info(void);
#define get_processor_on_this_board(this_board) ((this_board->this_processor))


enum RMI_BOARDS_MINOR
{
	MINOR_A  = 0,
	MINOR_B  = 1,
	MINOR_C  = 2,
	MINOR_XLS= 3,
	MINOR_XLS_B = 4,
	MINOR_LAST  = 0xf,

};

struct rmi_processor_info {
   char name[25];
   uint32_t processor_id;
   uint32_t revision;
   unsigned long io_base;
};

/* This structure, when initialized, indicates the devices present on the 
   piobus
   */
struct piobus_info {
	int cpld_i2c;
	int	cpld;
	int	tor_scpld;
	int pa_acc_fpga;
	int	soho_reset_cpld;
	int nand_flash;
	int	cfi_flash;
	int	pcmcia;
};

#define	FLASH_TYPE_NOR	1
#define	FLASH_TYPE_NAND	2
#define	FLASH_TYPE_NONE	4

struct board_flash_info {
	int	primary_flash_type;
	int	secondary_flash_type;
};

#define RMI_MAX_FLASH_PARTITIONS	4
struct flash_partition {
	char 	name[32];
	uint32_t	base;
	uint32_t	size;
};

struct flash_map {
	int	count; /* number of partitions */
	struct flash_partition fl_part[RMI_MAX_FLASH_PARTITIONS];
};

struct pio_dev {
	int		enabled;
	int		chip_sel;
	uint32_t	offset; /* offset from the PIOBASE */
	uint32_t	size; /* size of the flash to be mapped */
	unsigned long	iobase;
};

struct rmi_board_info {
   char name[25];
   unsigned int maj;
   unsigned int min;
   unsigned int isXls;
   unsigned int gmac_num;
   unsigned int gmac_list;
   struct piobus_info pio;
   struct board_flash_info  bfinfo;
};

	
int rmi_add_phys_region(uint64_t start, uint64_t size, uint32_t type);

#endif
