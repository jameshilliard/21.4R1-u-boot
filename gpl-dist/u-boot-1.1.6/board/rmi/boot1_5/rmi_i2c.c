/***************************************************************************************************
  Low level polling driver for i2c host controllers residing on RMI  processer chip.
 ***************************************************************************************************/

#include <rmi/i2c_spd.h>
#include <rmi/on_chip.h>
#include <rmi/types.h>

#include <exports.h>
#include <common.h>

#define I2C_REGSIZE volatile unsigned int
#define ARRAY_SHIFT ((sizeof (unsigned int))/(sizeof (I2C_REGSIZE)))


#define phoenix_io_mmio(offset) ((phoenix_reg_t *)(phoenix_io_base+(offset)))

/* Register set on RMI i2c host controller */
unsigned char i2c_reg_name[ ][10] = {
    "config",
    "clk_div",
    "dev_addr",
    "addr",
    "dataout",
    "datain",
    "status",
    "startxfr",
    "bytecnt",
    "hdstatim"
};

extern void udelay (unsigned long usec);

void i2c_delay(unsigned long d){
    udelay(d);
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

/***********************************************************************
Desc: Print i2c registers on individual i2c bus master or controller 
Input: bus master or controller number.
Output: 0 on success
-1 on Failure
 ************************************************************************/
int _i2c_debug_regs(unsigned short bus)
{ 
    volatile int i;

    I2C_REGSIZE *iobase_i2c_regs = 0;

    if(bus < 0 || bus > 1){printf("Illegal bus number %d\n",bus); return -1;}

    iobase_i2c_regs = get_i2c_base_offset(bus);


    printf("iic Reg set on bus %d::\n",bus);

    for(i=0;i<10;i++)
        printf("%8s::0x%x\n", i2c_reg_name[i], iobase_i2c_regs[(i * ARRAY_SHIFT)]);

    return 0;
}

int _i2c_set(unsigned short bus, unsigned char reg, int val)
{ 
    volatile int i;

    I2C_REGSIZE *iobase_i2c_regs = 0;

    if(bus < 0 || bus > 1){printf("Illegal bus number %d\n",bus); return -1;}

    iobase_i2c_regs = get_i2c_base_offset(bus);

    iobase_i2c_regs[reg * ARRAY_SHIFT]   = val;

    return 0;
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

    do{
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

    do{
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

/* *******************************************************
 * The main on-chip I2C Controller Write Routine.
 * Inputs:
 *   slave_addr   : I2C address of the target chip
 *   slave_offset : Offset within the target chip
 *   bus          : I2C bus on which the target is present
 *   dataout      : value to write
 *
 *  Output: 0 : success
 *         -1 : Failure
 * *******************************************************
 */

int _i2c_tx(unsigned short slave_addr, unsigned short slave_offset,
            unsigned short bus, unsigned char dataout) {

    I2C_REGSIZE *iobase_i2c_regs = 0;
    I2C_REGSIZE i2c_status;
    volatile int  i =0;

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


/**************************************************************************
 *  Desc: 8 - bit seq read 
 *  Input:      slave_addr: slave addr
 *              bus: i2c bus controller 
 *              addr: offset on the device
 *              buffer: buffer into which contents will be read
 *              len : number of bytes to be read
 ***************************************************************************/ 
int _i2c_rx_seq(unsigned short slave_addr, unsigned short bus,
                unsigned short addr, unsigned char *buffer, unsigned  int len) {

    I2C_REGSIZE *iobase_i2c_regs = 0;
    I2C_REGSIZE i2c_status;
    volatile int i =0,timeout;
    iobase_i2c_regs = get_i2c_base_offset(bus);

    /* printf("%s %x %x %x %x\n",__FUNCTION__, slave_addr,addr, bus,len);	 
       printf("%s %x %x\n",__FUNCTION__, addr, buffer[0], buffer);
     */

    /* Wait if busy */
    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];	
    while (i2c_status & 1) {
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];		
    }

    iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT]  = 0xf8;
    iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = (I2C_REGSIZE) 0;
    iobase_i2c_regs[I2C_DEVADDR * ARRAY_SHIFT] = (I2C_REGSIZE) slave_addr;
    iobase_i2c_regs[I2C_ADDR * ARRAY_SHIFT]    = (I2C_REGSIZE) addr;

    do {
        iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] =  I2C_TND_XFR; 

        /* Wait for busy bit to go away */
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
        while (i2c_status & 1) {
            i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
        }	

        if (i2c_status & 8) {
            printf("Ack error %x\n",slave_addr); return -1;	/* ACK_ERROR */
        }
    } while (i2c_status & 0x30);	/* LOST ARB or STARTERR -- repeat */


    i2c_delay(80);
    /* Wait for busy bit to go away */
    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
    while (i2c_status & 1) {
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
    }	

    /* Interrupts off
     * 8 bit internal address, not transmitted
     * 7 bit device address, transmitted
     */
    iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT]  = (I2C_REGSIZE)0xfa;
    iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = (I2C_REGSIZE)(len-1);

    do {
        iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] =  I2C_READ_XFR; 
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

        if(i2c_status & 1 ) continue;

        if (i2c_status & 8) {
            printf("Ack error \n");
            return -1;	/* ACK_ERROR */
        }
    } while (i2c_status & 0x30);	/* LOST ARB or STARTERR -- repeat */

    i = 0;
    while(i < len)
    {
        timeout = 0x10000;

        do{
            i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

            if(i2c_status & 4){
                i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
                buffer[i] = (I2C_REGSIZE) iobase_i2c_regs[I2C_DATAIN * ARRAY_SHIFT];
                break;
            }
        }while(timeout--);

        if(!timeout){
            printf("Timeout in sequential read of slave addr %x\n ", slave_addr);
            return -1;
        }
        i++;
    }

    return 0;
}

/**************************************************************************
 *  Desc:       8 - bit seq write 
 *  Input:      slave_addr: slave addr
 *              bus: i2c bus controller 
 *              addr: offset on the device
 *              buffer: buffer into which contents will be read
 *              len : number of bytes to be read
 *  Output:     0 : successful write
 *              1 : Failure
 *  Note: There is a limit on number of bytes you can write sequetially in one 
 *        shot. It varies from device to device. Consult the device data sheet
 *        before using this sequential write.
 ***************************************************************************/ 
int _i2c_tx_seq(unsigned short slave_addr, unsigned short bus,
                unsigned short addr, unsigned char *buffer, unsigned  int len) {

    I2C_REGSIZE *iobase_i2c_regs = 0;
    I2C_REGSIZE i2c_status;
    volatile int i =0,timeout=0x1000;
    iobase_i2c_regs = get_i2c_base_offset(bus);


    /* Wait if busy */
    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
    while (i2c_status & 1) {
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
    }

    iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT]  = 0xf8;
    iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = (I2C_REGSIZE)(len-1);
    iobase_i2c_regs[I2C_DEVADDR * ARRAY_SHIFT] = (I2C_REGSIZE) slave_addr;	
    iobase_i2c_regs[I2C_ADDR * ARRAY_SHIFT]    = (I2C_REGSIZE) addr;
    iobase_i2c_regs[I2C_DATAOUT * ARRAY_SHIFT] = (I2C_REGSIZE) buffer[0];


    do {
        iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] =  I2C_WRITE_XFR;

        /* Wait for busy bit to go away */
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

        if(i2c_status & 0x1) continue;

        if (i2c_status & 8) {
            printf("Ack error on slave %x", slave_addr);
            return -1;	/* ACK_ERROR */
        }
    } while (i2c_status & 0x30);	/* LOST ARB or STARTERR -- repeat */

    i2c_delay(100);

    i = 1;

    while(i < len){
        timeout = 0x1000000;
        do{
            i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];		
            if(i2c_status & 0x02){
                iobase_i2c_regs[I2C_DATAOUT * ARRAY_SHIFT] = (I2C_REGSIZE)buffer[i];
                i2c_delay(80);
                i = i + 1;
                break;
            }
            timeout= timeout-1;
        }while(timeout);

        if(timeout == 0)
        {
            printf("Timed out in %s\n",__FUNCTION__); return -1;
        }
    }
    return 0;
}

/*********************************************************** 
  Support for Slaves with greater than 8-bit addr space
 *************************************************************/


unsigned char get_seq_byte(I2C_REGSIZE *iobase_i2c_regs, unsigned int slave_addr)
{
    volatile unsigned char ch;
    I2C_REGSIZE i2c_status;

    iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT] = 0xfa;
    iobase_i2c_regs[I2C_DEVADDR * ARRAY_SHIFT] = (I2C_REGSIZE) slave_addr;
    iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = (I2C_REGSIZE) 0;

    do {
        iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] =  I2C_READ_XFR;
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
        /* Wait for busy bit to go away */
        while (i2c_status & 1) {
            i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
        }
        if (i2c_status & 8) {
            return -1;	/* ACK_ERROR */
        }
    } while (i2c_status & 0x30);	/* LOST ARB or STARTERR -- repeat */

    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
    while (!(i2c_status & 4)) {
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
    }

    ch = (I2C_REGSIZE) iobase_i2c_regs[I2C_DATAIN * ARRAY_SHIFT];
    return ch;
}

int _i2c_rx_alen_seq(unsigned short slave_addr, unsigned short bus,
                     unsigned short alen, unsigned char *buffer, unsigned  int len) {

    I2C_REGSIZE *iobase_i2c_regs = 0;
    I2C_REGSIZE i2c_status;
    volatile int i =0,timeout;
    iobase_i2c_regs = get_i2c_base_offset(bus);

    /*printf("%s %x %x %x %x\n",__FUNCTION__, slave_addr,bus,alen, len);	 
      printf("%s %x %x %x \n\n",__FUNCTION__, buffer[0],buffer[1], buffer);
     */

    /* Wait if busy */
    i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

    while (i2c_status & 1) {
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
    }

    /* Interrupts off
     * 8 bit internal address, not transmitted
     * 7 bit device address, transmitted
     */
    iobase_i2c_regs[I2C_CONFIG * ARRAY_SHIFT]  = (I2C_REGSIZE) 0xf8;
    iobase_i2c_regs[I2C_DEVADDR * ARRAY_SHIFT] = (I2C_REGSIZE) slave_addr;
    iobase_i2c_regs[I2C_ADDR * ARRAY_SHIFT]    = (I2C_REGSIZE) buffer[0];
    iobase_i2c_regs[I2C_DATAOUT * ARRAY_SHIFT] = (I2C_REGSIZE) buffer[1];
    iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = (I2C_REGSIZE) 0;

    do {
        iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] =  I2C_WRITE_XFR;
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
        /* Wait for busy bit to go away */
        while (i2c_status & 1) {
            i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
        }
        if (i2c_status & 8) {
            return -1; /* ACK_ERROR */
        }
    } while (i2c_status & 0x30);	/* LOST ARB or STARTERR -- repeat */

    /*i2c_delay(100);*/

    i = 0;

    while(i< len) {
        buffer[i] = get_seq_byte(iobase_i2c_regs, slave_addr);
        /*printf("Buffer[%d] = %x\n",i, buffer[i]);*/
        i++;
    }

    return 0;
}


/* Sequential write for devices > 8- bit addr space */
int _i2c_tx_alen_seq(unsigned short slave_addr, unsigned short bus,
                     unsigned short alen, unsigned char *offset_buff, 
                     unsigned char *buffer, unsigned int len) {
    volatile int i=0,timeout;
    I2C_REGSIZE *iobase_i2c_regs = 0;
    I2C_REGSIZE i2c_status;
    unsigned char ch;

    iobase_i2c_regs = get_i2c_base_offset(bus);

    /*printf("%s %x %x %x %x %x\n\n",__FUNCTION__, bus, 
     *        offset_buff[0],offset_buff[1], len, (len + alen -1));
     */

    i=0;
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
    iobase_i2c_regs[I2C_BYTECNT * ARRAY_SHIFT] = (I2C_REGSIZE) (len + (alen -1) - 1);
    iobase_i2c_regs[I2C_DEVADDR * ARRAY_SHIFT] = (I2C_REGSIZE) slave_addr;
    iobase_i2c_regs[I2C_ADDR * ARRAY_SHIFT]    = (I2C_REGSIZE) offset_buff[0];
    iobase_i2c_regs[I2C_DATAOUT * ARRAY_SHIFT] = (I2C_REGSIZE) offset_buff[1];

    do {
        iobase_i2c_regs[I2C_STARTXFR * ARRAY_SHIFT] = I2C_WRITE_XFR;
        i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];

        if(i2c_status & 0x1) continue;

        if (i2c_status & 8) {
            printf("I2C ACK ERROR\n");
            return -1;
        }
    } while(i2c_status & 0x30);

    i = 0;
    /* Continue to write data meant for the device*/
    while(i< len) {
        timeout = 0x1000000;

        do{
            i2c_status = iobase_i2c_regs[I2C_STATUS * ARRAY_SHIFT];
           /* We have to write to dataout register within a certain time 
            * after bit 1 is set, do not put any statements inside the 
            * below if statement. 
            */
            if(i2c_status & 0x02){
                ch = buffer[i];
                iobase_i2c_regs[I2C_DATAOUT * ARRAY_SHIFT] = ch; 
                i = i + 1;
                break;
            }
            timeout--; 
        } while(timeout);

        if(timeout == 0 ) {printf("Timed out in %s\n",__FUNCTION__); return -1;}
    }

    return 0;
}
