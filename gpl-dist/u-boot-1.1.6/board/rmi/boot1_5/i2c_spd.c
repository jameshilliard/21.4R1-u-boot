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

#include "rmi/boardconfig.h"
#include "rmi/types.h"
#include "rmi/bridge.h"
#include "rmi/i2c_spd.h"
#include "rmi/board_i2c.h"
#include "rmi/pioctrl.h"
#include "rmi/gpio.h"
#include "rmi/on_chip.h"
#include "rmi/mem_ctrl.h"
#include "rmi/xlr_cpu.h"
#include "rmi/debug.h"
#ifdef CONFIG_FX
#include <configs/fx.h>
#endif

extern void printf (const char *fmt, ...);
extern void write_binfo_eeprom();

#define ARRAY_SHIFT ((sizeof (unsigned int))/(sizeof (I2C_REGSIZE)))

/* Defines */
int spd_i2c_rx(unsigned short slave_addr, unsigned short slave_offset,
           unsigned short bus, unsigned short *retval);

int configure_ics_slave(void);
static unsigned short i2c_board_major;
extern unsigned long phoenix_io_base;
extern void do_i2c_eeprom(int argc, char* argv[]);

extern void somedelay(void);

extern int register_shell_cmd(const char *name,
			      void (*func) (int argc, char *argv[]), 
                  const char *shorthelp);
    
#ifdef CONFIG_FX
extern unsigned char     *cpld_iobase;
#endif

I2C_REGSIZE *get_i2c_base(unsigned short bus)
{
	phoenix_reg_t *mmio = 0;

	if (bus == 0)
		mmio = phoenix_io_mmio(PHOENIX_IO_I2C_0_OFFSET);
	else
		mmio = phoenix_io_mmio(PHOENIX_IO_I2C_1_OFFSET);

	return mmio;
}

void read_eeprom(unsigned short offset, unsigned short *value) {

    spd_i2c_rx(I2C_EEPROM_ADDR, offset, I2C_EEPROM_BUS, value);
}


/* ***************************************
 * I2C initialization block. Includes: -
 *   : Chip-level controller configuration
 *   : Board-level EEPROM and Temp. Sensor 
 *     configurable access routines
 * ***************************************
 */
void spd_i2c_init(void) {
    
    I2C_REGSIZE *iobase_i2c_regs = 0;
    unsigned short bus;

    for (bus=0; bus<2; bus++) {
	    iobase_i2c_regs = get_i2c_base(bus);
        iobase_i2c_regs[I2C_CLKDIV * ARRAY_SHIFT]   = I2C_CLOCK_DIVISOR;
        iobase_i2c_regs[I2C_HDSTATIM * ARRAY_SHIFT] = I2C_HDSTA_DELAY;
    }

     /*Remove later */
     //write_binfo_eeprom();

#ifdef CFG_BOARD_RMIREF
    {
        unsigned short major=0, minor=0;
        unsigned short value;

        read_eeprom(MAJOR_OFFSET, &major);
        read_eeprom(MINOR_OFFSET, &minor);

        if ((major < '1') || (major > 'C')) {
            printf_err("[ERROR]: Unsupported Board Major Number [%c]\n", major);
            cpu_halt();
	}
	else {
		if(major <= '9')
			printf("Board: RMI Reference, Major [%c], Minor [%c]\n",
					major, minor);
		else
			printf("Board: RMI Reference, Major [%d], Minor [%c]\n",
					major-0x37, minor);

        }

        if (((major=='1') && (minor=='1')) || chip_processor_id_xls()) {
            if (configure_ics_slave() == -1) {
                printf_err("I2C Error: Unable to configure ICS Device\n");
            }
            else {
                printf("Configured ICS I2C Device\n");
            }
        } 
        register_shell_cmd("eeprom", do_i2c_eeprom, "Program EEPROM"); //FIXME
        
        /* Temp. Sensor Block */
        if (major == '8') {
            /* LiTE Board */
            spd_i2c_rx(I2C_ADDR_TEMPSENSOR+0x01, EXT_TEMP_OFFSET+0x06, 
                    I2C_BUS_TEMPSENSOR, &value);
        }
        else {
            spd_i2c_rx(I2C_ADDR_TEMPSENSOR, EXT_TEMP_OFFSET, 
                    I2C_BUS_TEMPSENSOR, &value);
        }
        printf("Processor Junction Temperature : [%d] Celsius\n", value);
    }
#endif

#ifdef CFG_TEMPLATE_I2C_EEPROM
    /* Other boards may have their own EEPROMs with 
     * different EEPROM offset/value pairs. We can't
     * decide what to do with such EEPROMs here, so
     * simply display contents.
     */
    {
        int offset=0;
        unsigned short value;
        
#ifdef CONFIG_FX
        volatile uint32_t *gpio_reg_io = GPIO_IO_REG; 
        volatile uint32_t *gpio_reg_cfg = GPIO_CFG_REG;
        volatile uint32_t *xlr_i2c0_reg = 0xbef16028; 
        volatile uint32_t *xlr_i2c1_reg = 0xbef17028;
                
        /* disable mux */
        *gpio_reg_cfg |= 0x3FF;
        *gpio_reg_io |= 0x40;
        udelay(200);

        /* data and clock low */
        *xlr_i2c0_reg = 0x3;
        *xlr_i2c1_reg = 0x3;
                
        udelay(100);
        /* data and clock high */
        *xlr_i2c0_reg = 0;
        *xlr_i2c1_reg = 0;
                
        udelay(100);
#endif

        EEPROM_PRINTF("[EEPROM Contents]\n");
        for (offset=0; offset<CFG_TEMPLATE_SIZE_EEPROM; offset++) {
            spd_i2c_rx(CFG_TEMPLATE_I2CADDR_EEPROM, offset, 
                    CFG_TEMPLATE_I2CBUS_EEPROM, &value);
            if (offset && !(offset % 8))
                EEPROM_PRINTF("\n");
            EEPROM_PRINTF(" %02x ", value);  
        }
        EEPROM_PRINTF("\n");
    }

#ifdef CONFIG_FX
    volatile uint32_t *gpio_reg_io = GPIO_IO_REG; 
    volatile uint32_t *gpio_reg_cfg = GPIO_CFG_REG;

    /* enable mux, park to spd eeprom */
    *gpio_reg_cfg = (0x3FF | (1 << 24) | ( 1 << 22) | (1 << 19));
    *gpio_reg_io = (0x2 | (1 << 24) | ( 1 << 22) | (1 << 19));
    udelay(200);
#endif
#endif
    
#ifdef CFG_TEMPLATE_I2C_TEMPSENSOR
    {
        unsigned short value;
        spd_i2c_rx(CFG_TEMPLATE_I2CADDR_TEMPSENSOR, CFG_TEMPLATE_EXTTEMP_OFFSET,
               CFG_TEMPLATE_I2CBUS_TEMPSENSOR, &value);
        printf("Processor Junction Temperature : [%d] Celsius\n", value);
    }
#endif
}

/* *******************************************************
 * The main on-chip I2C Controller READ Routine.
 * Inputs:
 *   slave_addr   : I2C address of the target chip
 *   slave_offset : Offset within the target chip
 *   bus          : I2C bus on which the target is present
 *   retval       : READ return value 
 * *******************************************************
 */
int spd_i2c_rx(unsigned short slave_addr, unsigned short slave_offset,
		  unsigned short bus, unsigned short *retval) {

    I2C_REGSIZE *iobase_i2c_regs = 0;
	I2C_REGSIZE i2c_status;

	iobase_i2c_regs = get_i2c_base(bus);

	/* Wait if busy */
	i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
	while (i2c_status & 1) {
		i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
	}

	/* Interrupts off
	 * 8 bit internal address
	 * 7 bit i2c dev. addr and direction bit
	 * both addresses enabled
	 */
	iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT] = 0xf8;
	iobase_i2c_regs[I2C_DEVADDR * ARRAY_SHIFT] = (I2C_REGSIZE) slave_addr;
	iobase_i2c_regs[I2C_ADDR * ARRAY_SHIFT] = (I2C_REGSIZE) slave_offset;

	do {
		iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] = I2C_TND_XFR;
		i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

		/* Wait for busy bit to go away */
		while (i2c_status & 1) {
			i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
		}

		if (i2c_status & 8) {
			*retval = -1;
			return -1;
		}
	} while (i2c_status & 0x30);	/* LOST ARB or STARTERR -- repeat */

	/* Interrupts off
	 * 8 bit internal address, not transmitted
	 * 7 bit device address, transmitted
	 */
	iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT] = 0xfa;
	iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = 0;

	do {
		iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] = I2C_READ_XFR;
		i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
		/* Wait for busy bit to go away */
		while (i2c_status & 1) {
			i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
		}
		if (i2c_status & 8) {
			*retval = -1;
			return -1;	/* ACK_ERROR */
		}
	} while (i2c_status & 0x30);	/* LOST ARB or STARTERR -- repeat */

	while (!(i2c_status & 4)) {
		i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
	}
	*retval = (I2C_REGSIZE) iobase_i2c_regs[I2C_DATAIN * ARRAY_SHIFT];
	return 0;
}


int spd_i2c_tx(unsigned short slave_addr, unsigned short slave_offset,
                  unsigned short bus, unsigned short dataout) {

  I2C_REGSIZE *iobase_i2c_regs = 0;
  volatile I2C_REGSIZE i2c_status;

  iobase_i2c_regs = get_i2c_base(bus);

  /* Wait if busy */
  i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
  while (i2c_status & 1) {
    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
  }

  /* Interrupts off
     8 bit internal address
     7 bit i2c device address followed by direction bit
     both addresses enabled
  */
  
  iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT]  = 0xf8;
  iobase_i2c_regs[I2C_DEVADDR * ARRAY_SHIFT] = (I2C_REGSIZE)slave_addr;
  iobase_i2c_regs[I2C_ADDR * ARRAY_SHIFT]    = (I2C_REGSIZE)slave_offset;
  iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = (I2C_REGSIZE)0;
  iobase_i2c_regs[I2C_DATAOUT * ARRAY_SHIFT] = (I2C_REGSIZE)dataout;

  do {
    iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] = I2C_WRITE_XFR;
    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

    if (i2c_status & 8) {
      printf_err("I2C ACK ERROR\n\r");
      return -1;
    }
  } while(i2c_status & 0x30);

  return 0;
}

int dram_read_spd_eeprom(unsigned short which_eeprom, unsigned short which_param,
		    unsigned short *retval)
{
#ifdef CFG_BOARD_RMIREF
	return (spd_i2c_rx(which_eeprom, which_param, SPD_BUS, retval));
#else
	return (spd_i2c_rx(which_eeprom, which_param, CFG_TEMPLATE_I2CBUS_SPD, retval));
#endif
}

static void read_i2c_board_major(void)
{
    spd_i2c_rx(I2C_EEPROM_ADDR, MAJOR_OFFSET, I2C_EEPROM_BUS, &i2c_board_major);
}

static int is_board_xls_2xx_lte(void)
{
    if ((i2c_board_major == '7') || (i2c_board_major == '8')) 
        return 1;
    return 0;
}

static int is_board_xi_xii(void)
{
    if ((i2c_board_major == 0x42) || (i2c_board_major == 0x43)) {
        return 1;
    }
    return 0;
}

static void i2c_write_ics(unsigned char offset, unsigned char value)
{
    *(unsigned long *)(0xbef17000 + offset) = value;
}

static void ics_write(unsigned char offset, unsigned char value)
{
    i2c_write_ics(0x00, 0xf8);  
    i2c_write_ics(0x20, 0x01);  
    i2c_write_ics(0x08, 0x69);  
    i2c_write_ics(0x0c, offset);    /* Slave Offset */
    i2c_write_ics(0x10, value);     /* Value */
    i2c_write_ics(0x1c, 0x00);  
}


int configure_ics_slave(void) {

    volatile I2C_REGSIZE i2c_status;
    unsigned long *iobase_i2c_regs = (unsigned long *)get_i2c_base(0x01);
    phoenix_reg_t *gpio_mmio = phoenix_io_mmio(0x18000);
    unsigned int value = gpio_mmio[21];
    int srio_mode_check=0;
    int timeOut=0x10;               /* Random timeout */

    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT] & 0x0001;
    while (i2c_status && timeOut--) {
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT] & 0x0001;
    }
    if (timeOut == 0x00)
        return -1;  /* Timed Out */

    read_i2c_board_major();

    if (is_board_xi_xii()) {
        if ((value >> 26) & 0x3) {
            printf("Enabling SRIO Mode...\n");
            srio_mode_check = 1;
        }
        else {
            printf("Enabling PCIE Mode...\n");
        }
    }

    /* On the XLS, program the ICS Slave Chip to get
     * better granularity on the PCIE Clock Frequency.
     */
    if (chip_processor_id_xls()) {

        /* Disable Spread Spectrum, Default to 100MHz */
        if(is_board_xls_2xx_lte() || is_board_xi_xii()) {
            ics_write(0x00, 0x85);
        }
        else {
            ics_write(0x00, 0x04);
        }
        somedelay();

        /* Enable MN Divider Programming */
        ics_write(0x0a, 0x80);
        somedelay();
        somedelay();
        
        /* PLL 2 */
        if (srio_mode_check) {
            /* Source Core Clock from 
             * PLL2 LTE:XLS B0 etc... 
             */
            ics_write(0x01, 0x78);
            somedelay();
            somedelay();
            /* PLL 2 M-N Values : XLS B0-SRIO */
            /* MN Value */
            ics_write(0x0f, 0xce);
            somedelay();
            somedelay();
            /* N Value */
            ics_write(0x10, 0x76);
            somedelay();
            somedelay();
            /* Disable Spread Spectrum, 
             * Default to 100MHz 
             */
            ics_write(0x00, 0x85);
            somedelay();
            somedelay();
        }

        /* PLL 1 */
        /* MN Value */
        if(is_board_xls_2xx_lte() || is_board_xi_xii()) {
            if (srio_mode_check) {
                /* XLS B0-SRIO */
                ics_write(0x0b, 0xcb);
            }
            else {
                ics_write(0x0b, 0xce);
            }
        } else {
            ics_write(0x0b, 0xde);
        }
        somedelay();
        somedelay();

        /* N Value */
        if (srio_mode_check) {
            ics_write(0x0c, 0x84);
        }
        else {
            ics_write(0x0c, 0x76);
        }
        somedelay();
        somedelay();

        if(is_board_xls_2xx_lte() || is_board_xi_xii()) {
            /*Disable unused clocks*/
            ics_write(0x02, 0x00);
            somedelay();
            ics_write(0x03, 0xb8);
            somedelay();
            ics_write(0x04, 0xf5);
            somedelay();
        } else {
            /*Disable unused clocks*/
            ics_write(0x01, 0x38);
            somedelay();
            ics_write(0x02, 0x02);
            somedelay();
            ics_write(0x03, 0x80);
            somedelay();
        }
    }
    else {
        i2c_write_ics(0x00, 0xf8);  
        i2c_write_ics(0x20, 0x01);  
        i2c_write_ics(0x08, 0x6e);  
        i2c_write_ics(0x0c, 0x00);  
        i2c_write_ics(0x10, 0x06);  
        i2c_write_ics(0x1c, 0x00);  
    }
    return 0;
}
