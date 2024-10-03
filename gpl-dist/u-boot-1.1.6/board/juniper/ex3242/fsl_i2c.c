/*
 * Copyright (c) 2006-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * Copyright 2006 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#if defined(CONFIG_EX3242) || defined(CONFIG_EX45XX)
#ifdef CONFIG_HARD_I2C

#include <command.h>
#include <i2c.h>		/* Functional interface */ 

#include <asm/io.h>
#include "../ex3242/fsl_i2c.h"	/* HW definitions */

#define I2C_TIMEOUT	(CFG_HZ / 4)

static unsigned int i2c_bus_num __attribute__ ((section ("data"))) = 0;

static volatile struct fsl_i2c *i2c_dev[2] = {
	(struct fsl_i2c *) (CFG_IMMR + CFG_I2C_OFFSET),
#ifdef CFG_I2C2_OFFSET
	(struct fsl_i2c *) (CFG_IMMR + CFG_I2C2_OFFSET)
#endif
};

static int i2c_busy = 0;
static int i2c2_busy = 0;
#define I2C_ARRAY_SIZE  32
int i2cController[I2C_ARRAY_SIZE];
int i2cReadWrite[I2C_ARRAY_SIZE];
uint8_t lastDev[I2C_ARRAY_SIZE];
uint16_t lastAddr[I2C_ARRAY_SIZE];
uint8_t lastData[I2C_ARRAY_SIZE]; 
int lastLength[I2C_ARRAY_SIZE];
int currentIndex = 0;

int i2c_bus_busy(void)
{
	return i2c_busy;
}

int i2c_controller2_busy(void)
{
	return i2c2_busy;
}

int current_i2c_controller(void)
{
	return i2c_bus_num;
}

void i2c_controller(int controller)
{
	i2c_bus_num = (controller == CFG_I2C_CTRL_2) ? 
                 CFG_I2C_CTRL_2 : CFG_I2C_CTRL_1;
       i2c2_busy = (i2c_bus_num == CFG_I2C_CTRL_2) ? 1 : 0;
        
}

void
i2c_init(int speed, int slaveadd)
{
	volatile struct fsl_i2c *dev;

	dev = (struct fsl_i2c *) (CFG_IMMR + CFG_I2C_OFFSET);

	writeb(0, &dev->cr);			/* stop I2C controller */
       udelay(5);
#if defined(CONFIG_EX45XX)
	writeb(0x0d, &dev->fdr);		/* set bus speed */
#else
	writeb(0x2c, &dev->fdr);		/* set bus speed */
#endif
	writeb(0x10, &dev->dfsrr);		/* set default filter */
	writeb(slaveadd, &dev->adr);	/* write slave address */
	writeb(0x0, &dev->sr);			/* clear status register */
	writeb(I2C_CR_MEN, &dev->cr);		/* start I2C controller */

#ifdef	CFG_I2C2_OFFSET
	dev = (struct fsl_i2c *) (CFG_IMMR + CFG_I2C2_OFFSET);

	writeb(0, &dev->cr);			/* stop I2C controller */
       udelay(5);
#if defined(CONFIG_EX45XX)
	writeb(0x0d, &dev->fdr);		/* set bus speed */
#else
	writeb(0x2c, &dev->fdr);		/* set bus speed */
#endif
	writeb(0x10, &dev->dfsrr);		/* set default filter */
	writeb(slaveadd, &dev->adr);		/* write slave address */
	writeb(0x0, &dev->sr);			/* clear status register */
	writeb(I2C_CR_MEN, &dev->cr);		/* start I2C controller */
#endif	/* CFG_I2C2_OFFSET */
}

void i2c_fdr(uint8_t clock, int slaveadd)
{
	/* stop I2C controller */
	writeb(0x0, &i2c_dev[i2c_bus_num]->cr);

       udelay(5);
       
       /* set clock */
       writeb(clock, &i2c_dev[i2c_bus_num]->fdr);

	/* set default filter */
	writeb(0x10, &i2c_dev[i2c_bus_num]->dfsrr);

	/* write slave address */
	writeb(slaveadd, &i2c_dev[i2c_bus_num]->adr);

	/* clear status register */
	writeb(0x0, &i2c_dev[i2c_bus_num]->sr);

	/* start I2C controller */
	writeb(I2C_CR_MEN, &i2c_dev[i2c_bus_num]->cr);

       udelay(5);
}

static __inline__ int
i2c_wait4bus(void)
{
	ulong timeval = get_timer(0);
	int loops =0;
       int i = 0;

	while (readb(&i2c_dev[i2c_bus_num]->sr) & I2C_SR_MBB) {
		if (++loops > 100000)
		{
    		//for (loops=50; loops--;)
  		    printf ("!!! I2C Hardware defect - bus not getting idle!!!\n");
                  for (i = 0; i<I2C_ARRAY_SIZE; i++)
  		        printf ("%s i2c controller %d %s dev=0x%x, addr=0x%x, data=0x%x, len=0x%x\n",
                     (i == currentIndex) ? "*" : " ", i2cController[i], 
  		        i2cReadWrite[i] ? "write" : "read", lastDev[i], lastAddr[i], lastData[i], lastLength[i]);
//  		    printf ("!!! I2C Hardware defect - bus not getting idle !!!\n");
//    		for (loops=0;;)  // forever
//  		        printf ("%d\r", loops++);
    		return -1;
		}
		if (get_timer(timeval) > I2C_TIMEOUT) {
			return -1;
		}
	}
	return 0;
}

static __inline__ int
i2c_wait(int write)
{
	u32 csr;
	ulong timeval = get_timer(0);

	do {
		csr = readb(&i2c_dev[i2c_bus_num]->sr);
		if (!(csr & I2C_SR_MIF))
			continue;

		writeb(0x0, &i2c_dev[i2c_bus_num]->sr);

		if (csr & I2C_SR_MAL) {
			debug("i2c_wait: MAL\n");
			return -1;
		}

		if (!(csr & I2C_SR_MCF))	{
			debug("i2c_wait: unfinished\n");
			return -1;
		}

		if (write == I2C_WRITE && (csr & I2C_SR_RXAK)) {
			debug("i2c_wait: No RXACK\n");
			return -1;
		}

		return 0;
	} while (get_timer (timeval) < I2C_TIMEOUT);

	debug("i2c_wait: timed out\n");
	return -1;
}

static __inline__ int
i2c_write_addr (u8 dev, u8 dir, int rsta)
{
	writeb(I2C_CR_MEN | I2C_CR_MSTA | I2C_CR_MTX
	       | (rsta ? I2C_CR_RSTA : 0),
	       &i2c_dev[i2c_bus_num]->cr);

	writeb((dev << 1) | dir, &i2c_dev[i2c_bus_num]->dr);

	if (i2c_wait(I2C_WRITE) < 0)
		return 0;

	return 1;
}

static __inline__ int
__i2c_write(u8 *data, int length)
{
	int i;

	writeb(I2C_CR_MEN | I2C_CR_MSTA | I2C_CR_MTX,
	       &i2c_dev[i2c_bus_num]->cr);

	for (i = 0; i < length; i++) {
		writeb(data[i], &i2c_dev[i2c_bus_num]->dr);

		if (i2c_wait(I2C_WRITE) < 0)
			break;
	}

	return i;
}

static __inline__ int
__i2c_read(u8 *data, int length)
{
	int i;

	writeb(I2C_CR_MEN | I2C_CR_MSTA | ((length == 1) ? I2C_CR_TXAK : 0),
	       &i2c_dev[i2c_bus_num]->cr);

	/* dummy read */
	readb(&i2c_dev[i2c_bus_num]->dr);

	for (i = 0; i < length; i++) {
		if (i2c_wait(I2C_READ) < 0)
			break;

		/* Generate ack on last next to last byte */
		if (i == length - 2)
			writeb(I2C_CR_MEN | I2C_CR_MSTA | I2C_CR_TXAK,
			       &i2c_dev[i2c_bus_num]->cr);

		/* Generate stop on last byte */
		if (i == length - 1)
			writeb(I2C_CR_MEN | I2C_CR_TXAK, &i2c_dev[i2c_bus_num]->cr);

		data[i] = readb(&i2c_dev[i2c_bus_num]->dr);
	}

	return i;
}

int
i2c_read(u8 dev, uint addr, int alen, u8 *data, int length)
{
	int i = -1;
	u8 *a = (u8*)&addr;
       int ret = 0;

       i2c_busy = 1;
       currentIndex++;
       currentIndex &= (I2C_ARRAY_SIZE - 1);
       i2cController[currentIndex] = i2c_bus_num;
       i2cReadWrite[currentIndex] = 0;
       lastDev[currentIndex] = dev;
       lastAddr[currentIndex] = addr;
       lastData[currentIndex] = *data, 
       lastLength[currentIndex] = length;
       
	if (i2c_wait4bus() >= 0
	    && i2c_write_addr(dev, I2C_WRITE, 0) != 0
	    && __i2c_write(&a[4 - alen], alen) == alen)
	    i = 0;
    
       if (length
	    && i2c_write_addr(dev, I2C_READ, 1) != 0) {
		i = __i2c_read(data, length);
	}

	writeb(I2C_CR_MEN, &i2c_dev[i2c_bus_num]->cr);

	if (i == length)
	    ret = 0;
       else
           ret = -1;

       i2c_busy = 0;
	return ret;
#if 0
       if (i2c_wait4bus() < 0) 
           ret = -1; 

       if (ret == 0) {
           if (i2c_write_addr(dev, I2C_WRITE, 0) == 0) 
               ret = -1; 
       }

       if (ret == 0) {
           if (__i2c_write(&a[4 - alen], alen) != alen) {
               ret = -1;
           }
       }

       if (ret == 0) {
           if (i2c_wait4bus() < 0) 
               ret = -1; 
       }

       if (ret == 0) {
           if (length) { 
               if (i2c_write_addr(dev, I2C_READ, 1) == 0)
               ret = -1; 
           }
           if (ret == 0) {
               if (__i2c_read(data, length) != length) 
                   ret = -1; 
           }
       }

       writeb(I2C_CR_MEN, &I2C->cr); 

       i2c_busy = 0;
       return ret; 
#endif

}

int
i2c_read_norsta(u8 dev, uint addr, int alen, u8 *data, int length)
{
	u8 *a = (u8*)&addr;
       int ret = 0;

       i2c_busy = 1;
       currentIndex++;
       currentIndex &= (I2C_ARRAY_SIZE - 1);
       i2cController[currentIndex] = i2c_bus_num;
       i2cReadWrite[currentIndex] = 0;
       lastDev[currentIndex] = dev;
       lastAddr[currentIndex] = addr;
       lastData[currentIndex] = *data, 
       lastLength[currentIndex] = length;

       if (i2c_wait4bus() < 0) 
           ret = -1; 

       if (ret == 0) {
           if (i2c_write_addr(dev, I2C_WRITE, 0) == 0) 
               ret = -1; 
       }

       if (ret == 0) {
           if (__i2c_write(&a[4 - alen], alen) != alen) {
               ret = -1;
           }
           else {
               /* force a stop */
	       writeb(I2C_CR_MEN, &i2c_dev[i2c_bus_num]->cr);
           }
           udelay(3000);
       }

       if (ret == 0) {
           if (i2c_wait4bus() < 0) 
               ret = -1; 
       }

       if (ret == 0) {
           if (length) { 
               if (i2c_write_addr(dev, I2C_READ, 0) == 0)
               ret = -1; 
           }
           if (ret == 0) {
               if (__i2c_read(data, length) != length) 
                   ret = -1; 
           }
       }

       writeb(I2C_CR_MEN, &i2c_dev[i2c_bus_num]->cr); 

       i2c_busy = 0;
       return ret; 

}

int
i2c_read_noaddr(u8 dev, u8 *data, int length)
{
       int ret = 0;

       i2c_busy = 1;
       currentIndex++;
       currentIndex &= (I2C_ARRAY_SIZE - 1);
       i2cController[currentIndex] = i2c_bus_num;
       i2cReadWrite[currentIndex] = 0;
       lastDev[currentIndex] = dev;
       lastAddr[currentIndex] = 0xff;
       lastData[currentIndex] = *data, 
       lastLength[currentIndex] = length;

       if (i2c_wait4bus() < 0) 
           ret = -1; 

       if (ret == 0) {
           if (length) { 
               if (i2c_write_addr(dev, I2C_READ, 0) == 0)
               ret = -1; 
           }
           if (ret == 0) {
               if (__i2c_read(data, length) != length) 
                   ret = -1; 
           }
       }

       writeb(I2C_CR_MEN, &i2c_dev[i2c_bus_num]->cr); 

       i2c_busy = 0;
       return ret; 

}

int
i2c_write(u8 dev, uint addr, int alen, u8 *data, int length)
{
	int i = -1;
	u8 *a = (u8*)&addr;
       int ret = 0;

       i2c_busy = 1;
       currentIndex++;
       currentIndex &= (I2C_ARRAY_SIZE - 1);
       i2cController[currentIndex] = i2c_bus_num;
       i2cReadWrite[currentIndex] = 1;
       lastDev[currentIndex] = dev;
       lastAddr[currentIndex] = addr;
       lastData[currentIndex] = *data, 
       lastLength[currentIndex] = length;
#if 0
	if (i2c_wait4bus() >= 0
	    && i2c_write_addr(dev, I2C_WRITE, 0) != 0
	    && __i2c_write(&a[4 - alen], alen) == alen) {
		i = __i2c_write(data, length);
	}

	writeb(I2C_CR_MEN, &I2C->cr);

	if (i == length)
	    return 0;

	return -1;
#endif
	if (i2c_wait4bus () < 0)
		ret = -1;

       if (ret == 0) {
	    if (i2c_write_addr (dev, I2C_WRITE, 0) == 0)
		ret = -1;
       }

       if (ret == 0) {
           if (alen) {
	        if (__i2c_write (&a[4 - alen], alen) != alen)
		    ret = -1;
           }
       }

       if (ret == 0) {
	    i = __i2c_write (data, length);
       }

	writeb(I2C_CR_MEN, &i2c_dev[i2c_bus_num]->cr);

       if (ret == 0) {
	    if (i == length)
	        ret = 0;
           else
               ret = -1;
       }

       i2c_busy = 0;
	return ret;
}

int
i2c_write_noaddr(u8 dev, u8 *data, int length)
{
	int i = -1;
       int ret = 0;

       i2c_busy = 1;
       currentIndex++;
       currentIndex &= (I2C_ARRAY_SIZE - 1);
       i2cController[currentIndex] = i2c_bus_num;
       i2cReadWrite[currentIndex] = 1;
       lastDev[currentIndex] = dev;
       lastAddr[currentIndex] = 0xff;
       lastData[currentIndex] = *data, 
       lastLength[currentIndex] = length;
       
	if (i2c_wait4bus () < 0)
		ret = -1;

       if (ret == 0) {
	    if (i2c_write_addr (dev, I2C_WRITE, 0) == 0)
		ret = -1;
       }


       if (ret == 0) {
	    i = __i2c_write (data, length);
       }

	writeb(I2C_CR_MEN, &i2c_dev[i2c_bus_num]->cr);

       if (ret == 0) {
	    if (i == length)
	        ret = 0;
           else
               ret = -1;
       }

       i2c_busy = 0;
	return ret;
}

int
i2c_probe(uchar chip)
{
	int tmp;

	/*
	 * Try to read the first location of the chip.  The underlying
	 * driver doesn't appear to support sending just the chip address
	 * and looking for an <ACK> back.
	 */
	udelay(10000);

	return i2c_read(chip, 0, 1, (uchar *)&tmp, 1);
}

int
i2c_probe_java(uint i2c_ctrl, uchar sw_addr, uchar ch_index, uchar chip)
{
	int tmp, ret=1;
	unsigned char ch;
	/*
	 * Try to read the first location of the chip.  The underlying
	 * driver doesn't appear to support sending just the chip address
	 * and looking for an <ACK> back.
	 */
	udelay(10000);

	i2c_controller(i2c_ctrl-1);
	ch= 1 << ch_index;
	i2c_write(sw_addr, 0, 0, &ch, 1); /* enable the coresponding channel on the swtich*/

	ret= i2c_read(chip, 0, 0, (uchar *)&tmp, 1);
	
	ch=0x0;
	i2c_write(sw_addr, 0, 0, &ch, 1);	/*disable the coresponding channel on the switch*/
	return (ret);
}

uchar
i2c_reg_read(uchar i2c_addr, uchar reg)
{
	uchar buf[1];

	i2c_read(i2c_addr, reg, 1, buf, 1);

	return buf[0];
}

int
i2c_reg_write(uchar i2c_addr, uchar reg, uchar val)
{
	return i2c_write(i2c_addr, (uchar)(reg & 0x7f), 1, &val, 1);
}

/*---------------------------- controller -------------------------*/


void
i2c_controller_init(int controller, int speed, int slaveadd)
{
       if (controller > CFG_I2C_CTRL_2)
           return;
       
       i2c_controller (controller);
	/* stop I2C controller */
	writeb(0x0, &i2c_dev[controller]->cr);

       udelay(5);
       
	/* set clock */
	writeb(0x2c, &i2c_dev[controller]->fdr);

	/* set default filter */
	writeb(0x10, &i2c_dev[controller]->dfsrr);

	/* write slave address */
	writeb(slaveadd, &i2c_dev[controller]->adr);

	/* clear status register */
	writeb(0x0, &i2c_dev[controller]->sr);

	/* start I2C controller */
	writeb(I2C_CR_MEN, &i2c_dev[controller]->cr);
}


static __inline__ int
i2c_controller_wait4bus(int controller)
{
	ulong timeval = get_timer(0);
	int loops =0;

	while (readb(&i2c_dev[controller]->sr) & I2C_SR_MBB) {
		if (++loops > 100000)
		{
  		    printf ("!!! I2C Hardware defect - bus not getting idle!!!\n");
    		    return -1;
		}
		if (get_timer(timeval) > I2C_TIMEOUT) {
			return -1;
		}
	}
	return 0;
}


static __inline__ int
i2c_controller_wait(int controller, int write)
{
	u32 csr;
	ulong timeval = get_timer(0);

	do {
		csr = readb(&i2c_dev[controller]->sr);
		if (!(csr & I2C_SR_MIF))
			continue;

		writeb(0x0, &i2c_dev[controller]->sr);

		if (csr & I2C_SR_MAL) {
			debug("i2c_controller_wait: MAL\n");
			return -1;
		}

		if (!(csr & I2C_SR_MCF))	{
			debug("i2c_controller_wait: unfinished\n");
			return -1;
		}

		if (write == I2C_WRITE && (csr & I2C_SR_RXAK)) {
			debug("i2c_controller_wait: No RXACK\n");
			return -1;
		}

		return 0;
	} while (get_timer (timeval) < I2C_TIMEOUT);

	debug("i2c_channel_wait: timed out\n");
	return -1;
}


static __inline__ int
i2c_controller_write_addr (int controller, u8 dev, u8 dir, int rsta)
{
	writeb(I2C_CR_MEN | I2C_CR_MSTA | I2C_CR_MTX
	       | (rsta ? I2C_CR_RSTA : 0),
	       &i2c_dev[controller]->cr);

	writeb((dev << 1) | dir, &i2c_dev[controller]->dr);

	if (i2c_controller_wait(controller, I2C_WRITE) < 0)
		return 0;

	return 1;
}


static __inline__ int
__i2c_controller_write(int controller, u8 *data, int length)
{
	int i;

	writeb(I2C_CR_MEN | I2C_CR_MSTA | I2C_CR_MTX,
	       &i2c_dev[controller]->cr);

	for (i = 0; i < length; i++) {
		writeb(data[i], &i2c_dev[controller]->dr);

		if (i2c_controller_wait(controller, I2C_WRITE) < 0)
			break;
	}

	return i;
}


static __inline__ int
__i2c_controller_read(int controller, u8 *data, int length)
{
	int i;

	writeb(I2C_CR_MEN | I2C_CR_MSTA | ((length == 1) ? I2C_CR_TXAK : 0),
	       &i2c_dev[controller]->cr);

	/* dummy read */
	readb(&i2c_dev[controller]->dr);

	for (i = 0; i < length; i++) {
		if (i2c_controller_wait(controller, I2C_READ) < 0)
			break;

		/* Generate ack on last next to last byte */
		if (i == length - 2)
			writeb(I2C_CR_MEN | I2C_CR_MSTA | I2C_CR_TXAK,
			       &i2c_dev[controller]->cr);

		/* Generate stop on last byte */
		if (i == length - 1)
			writeb(I2C_CR_MEN | I2C_CR_TXAK, &i2c_dev[controller]->cr);

		data[i] = readb(&i2c_dev[controller]->dr);
	}

	return i;
}


int
i2c_controller_read(int controller, u8 dev, uint addr, int alen, u8 *data, int length)
{
	int i = -1;
	u8 *a = (u8*)&addr;
       int ret = 0;

       if (controller > CFG_I2C_CTRL_2)
           return -1;
       
       i2c_controller (controller);
       
	if (i2c_controller_wait4bus(controller) >= 0
	    && i2c_controller_write_addr(controller, dev, I2C_WRITE, 0) != 0
	    && __i2c_controller_write(controller, &a[4 - alen], alen) == alen)
	    i = 0;
    
       if (length
	    && i2c_controller_write_addr(controller, dev, I2C_READ, 1) != 0) {
		i = __i2c_controller_read(controller, data, length);
	}

	writeb(I2C_CR_MEN, &i2c_dev[controller]->cr);

	if (i == length)
	    ret = 0;
       else
           ret = -1;

       i2c_busy = 0;
	return ret;

}


int
i2c_controller_write(int controller, u8 dev, uint addr, int alen, u8 *data, int length)
{
	int i = -1;
	u8 *a = (u8*)&addr;
       int ret = 0;

	if (controller > CFG_I2C_CTRL_2)
		ret = -1;
    
       i2c_controller (controller);
       
	if (i2c_controller_wait4bus (controller) < 0)
		ret = -1;

       if (ret == 0) {
	    if (i2c_controller_write_addr (controller, dev, I2C_WRITE, 0) == 0)
		ret = -1;
       }

       if (ret == 0) {
           if (alen) {
	        if (__i2c_controller_write (controller, &a[4 - alen], alen) != alen)
		    ret = -1;
           }
       }

       if (ret == 0) {
	    i = __i2c_controller_write (controller, data, length);
       }

	writeb(I2C_CR_MEN, &i2c_dev[controller]->cr);

       if (ret == 0) {
	    if (i == length)
	        ret = 0;
           else
               ret = -1;
       }

       i2c_busy = 0;
	return ret;
}

uchar
i2c_controller_reg_read(int controller, uchar i2c_addr, uchar reg)
{
	uchar buf[1];

	i2c_controller_read(controller, i2c_addr, reg, 1, buf, 1);

	return buf[0];
}

void
i2c_controller_reg_write(int controller, uchar i2c_addr, uchar reg, uchar val)
{
	i2c_controller_write(controller, i2c_addr, (uchar)(reg & 0x7f), 1, &val, 1);
}

#endif /* CONFIG_HARD_I2C */
#endif /* CONFIG_EX3242 */
