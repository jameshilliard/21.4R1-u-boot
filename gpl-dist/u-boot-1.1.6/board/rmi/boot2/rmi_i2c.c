/***************************************************************************************************
      Low level polling driver for i2c host controllers residing on RMI  processer chip.
***************************************************************************************************/


#include "rmi/i2c_spd.h"
#include "rmi/on_chip.h"
#include "rmi/types.h"

#include <exports.h>
#include <common.h>

#define I2C_REGSIZE volatile unsigned int
#define ARRAY_SHIFT ((sizeof (unsigned int))/(sizeof (I2C_REGSIZE)))


#define phoenix_io_mmio(offset) ((phoenix_reg_t *)(phoenix_io_base+(offset)))

void i2c_delay(void){
	int j;
	for(j=0;j<8000000;j++);
}

 

I2C_REGSIZE *get_i2c_base_offset(unsigned short bus)
{
        phoenix_reg_t *mmio = 0;

        if (bus == 0)
                mmio = phoenix_io_mmio(PHOENIX_IO_I2C_0_OFFSET);
        else
                mmio = phoenix_io_mmio(PHOENIX_IO_I2C_1_OFFSET);

        return mmio;
}

/* ***************************************
 * I2C initialization block. Includes: -
 *   : Chip-level controller configuration
 *   : Board-level EEPROM and Temp. Sensor
 *     configurable access routines
 * ***************************************
 */
void _i2c_init( void ) {

    I2C_REGSIZE *iobase_i2c_regs = 0;
    unsigned short bus;

    for (bus=0; bus<2; bus++) {
            iobase_i2c_regs = get_i2c_base_offset(bus);
        iobase_i2c_regs[I2C_CLKDIV * ARRAY_SHIFT]   = I2C_CLOCK_DIVISOR;
        iobase_i2c_regs[I2C_HDSTATIM * ARRAY_SHIFT] = I2C_HDSTA_DELAY;
    }
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
int _i2c_rx(unsigned short slave_addr, unsigned short slave_offset,
		  unsigned short bus, unsigned  char *retval) {

    I2C_REGSIZE *iobase_i2c_regs = 0;
	I2C_REGSIZE i2c_status;

	iobase_i2c_regs = get_i2c_base_offset(bus);

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

        //printf("\n%s : %x %x %x %x",__func__,slave_addr,slave_offset,bus,*retval);
	return 0;
}


int _i2c_tx(unsigned short slave_addr, unsigned short slave_offset,
                  unsigned short bus, unsigned char dataout) {

  I2C_REGSIZE *iobase_i2c_regs = 0;
  I2C_REGSIZE i2c_status;

  iobase_i2c_regs = get_i2c_base_offset(bus);

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
      printf("I2C ACK ERROR\n\r");
      return -1;
    }
  } while(i2c_status & 0x30);
 
  //printf("\n%s : %x %x %x %x",__func__,slave_addr,slave_offset,bus,dataout);
  return 0;
}


/********************************************************************
 *  Do i2c probe of a device on a bus.
 *   
 *     - starts an i2c session to a slave without sending any data
 *     - Probe fails if 
 *         - Ackerr
 *       
 *******************************************************************/

int _i2c_prb(unsigned char bus, unsigned int slave_addr) {

       I2C_REGSIZE *iobase_i2c_regs = 0;
        I2C_REGSIZE i2c_status;

        iobase_i2c_regs = get_i2c_base_offset(bus);

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
        iobase_i2c_regs[I2C_ADDR * ARRAY_SHIFT] = (I2C_REGSIZE) 0;

        do {
                iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] = I2C_TND_XFR;
                i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

                /* Wait for busy bit to go away */
                while (i2c_status & 1) {
                    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
                }

                if (i2c_status & 8) {
                    return -1;
                }
        } while (i2c_status & 0x30);    /* LOST ARB or STARTERR -- repeat */
      
      return 0; 
}
