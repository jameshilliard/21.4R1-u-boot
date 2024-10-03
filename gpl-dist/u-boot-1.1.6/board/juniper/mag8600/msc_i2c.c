/*
 * Copyright (c) 2011, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */


#include <common.h>

#ifdef CONFIG_MAG8600
#include "msc_cpld.h"
#include "msc_i2c.h"

#define MSC_I2C_DEBUG
#define MSC_I2C_TRACE
#undef MSC_I2C_TRACE
#undef MSC_I2C_DEBUG


#ifdef MSC_I2C_DEBUG
#define dprintf printf
#ifdef MSC_I2C_TRACE
#define dtrace dprintf
#else 
#define dtrace(...)
#endif 
#else
#define dprintf(...)
#define dtrace(...)
#endif


/**
 * @INTERNAL
 *
 * This function gets the clock setting for MSC I2C 
 * from the environment ( variables i2c_clk_by_XXX) 
 * or default to 1000 if no env variable is present
 * 
 * @param none
 * @return a clock value as defined in msc_cpld.h ( CPLD_I2C_CTL_<clock_value>)
 */

uint8_t 
msc_get_clk (void)
{
    uint8_t clk;

    /* set clock accordingly to env */
    if (getenv("i2c_clk_by_1000")) {
        clk = CPLD_I2C_CTL_1000;
        dprintf("Set i2c by 1000\n");
    } else if (getenv("i2c_clk_by_500")) {
        clk = CPLD_I2C_CTL_500;
        dprintf("Set i2c by 500\n");
    } else if ( getenv("i2c_clk_by_100")) {
        clk = CPLD_I2C_CTL_100;
        dprintf("Set i2c by 100\n");
    } else if ( getenv("i2c_clk_by_50")) {
        clk = CPLD_I2C_CTL_50;
        dprintf("Set i2c by 50\n");
    } else {
        clk = CPLD_I2C_CTL_1000;
        dprintf("Set i2c to default by 1000\n");
    }
    return clk;
}

/**
 * @INTERNAL
 *
 * This function reads a 8 bit quantiy fro, I2C
 * device specified by bus # device address register address ( within device)
 * 
 * @param bus an I2C bus as described in the file msc_i2c.h
 * @param dev a device address on the spefied bus
 * @param addr a register address within the specified device 
 *                        adressed with ( bus, dev) pair
 * @param *data a pointer to an 8 bit quantity to store the read data
 * @param clk a clock indicator used by the cpld to provide 
 *              timing to the operation
 * @return success or failure of the operation
 */

uint8_t 
msc_i2c_read_8 (uint8_t bus,uint8_t dev,uint8_t addr,
                uint8_t *data,uint8_t clk)
{

    uint8_t ctl = clk;
    uint8_t status = 0;
    uint8_t tmp;

    dtrace(">> %s bus: %x dev: %x addr: %x clk: %x\n", __FUNCTION__, bus, dev, addr, clk);
    /* Tell CPL to set and perform I2C op
     * some delay is required to let CPL do the work  
     */

    /* Set SDA, SCL high */
    cpld_wr(CPLD_I2C_CTL, (ctl | 0x0C));
    udelay (IM_I2C_DLY);

    dprintf("ctl = 0x%x\n", cpld_rd(CPLD_I2C_CTL));

    /* Set the Bus Selet bits 4:0 */
    cpld_wr(CPLD_I2C_BUS_SL, bus & 0x1f);
    udelay (IM_I2C_DLY);
    dprintf("bus = 0x%x\n", cpld_rd(CPLD_I2C_BUS_SL));

    /* Set the Device Addres ( bit 0 always 0) */
    cpld_wr(CPLD_I2C_DA,dev<<1);
    udelay (IM_I2C_DLY);
    dprintf("da = 0x%x\n", cpld_rd(CPLD_I2C_DA));


    /* Set the lower 8 bits of the I2C address */
    cpld_wr(CPLD_I2C_ADDR_0, addr);
    udelay (IM_I2C_DLY);
    dprintf("addr_0 = 0x%x\n", cpld_rd(CPLD_I2C_ADDR_0));

    /* Issue the Read Command */
    cpld_wr(CPLD_I2C_CSR, CPLD_I2C_RD_CMD);
    udelay (IM_I2C_DLY2);
    udelay (IM_I2C_DLY2);


    /* need a timeout on this one */
    while (cpld_rd(CPLD_I2C_CSR) &  CPLD_I2C_CSR_ST);

    dprintf("cmd = 0x%x\n", cpld_rd(CPLD_I2C_CSR));


    /* Retrieve  Byte */
    tmp = cpld_rd(CPLD_I2C_DATA);
    dprintf("data  = 0x%x\n", tmp);
    status = cpld_rd(CPLD_I2C_STAT);
    dprintf("status = 0x%x\n", status);
    if (status & CPLD_I2C_STAT_NO_ACK) {
        dprintf("No Acknowledge from device\n");
    }
    if (status & CPLD_I2C_STAT_TOUT) {
        dprintf("Timeout - SLC line low for 16*bit period\n");
    }
    if (status & CPLD_I2C_STAT_I2C_FLT) {
        dprintf("I2C Fault SLC or SDA stuck low \n"); 
    }

    /* tell caller the all story */
    *data = tmp;
    return status;
}


/**
 * @INTERNAL
 *
 * This function reads a 16 bit quantiy from I2C
 * device specified by bus # device address register address ( within device)
 * 
 * @param bus an I2C bus as described in the file msc_i2c.h
 * @param dev a device address on the spefied bus
 * @param addr a register address within the specified device 
 *                        adressed with ( bus, dev) pair
 * @param *data a pointer to an 16 bit quantity to store the read data
 * @param clk a clock indicator used by the cpld to provide 
 *              timing to the operation
 * @return success or failure of the operation
 */
uint8_t 
msc_i2c_read_16 (uint8_t bus,uint8_t dev,uint8_t addr, 
                 uint16_t *data,uint8_t clk)
{

    uint8_t data_high;
    uint8_t data_low;
    uint8_t status; 

    dtrace(">> %s bus: %x dev: %x clk: %x\n", __FUNCTION__, bus, dev, addr, clk);

    status  =  msc_i2c_read_8(bus, dev, addr, &data_high, clk);
    status |=  msc_i2c_read_8(bus, dev, addr, &data_low,  clk);
    *data = ( data_high << 8 | data_low);
    return status;
}


/**
 * @INTERNAL
 *
 * This function write a 8 bit quantiy to I2C
 * device specified by bus # device address register address ( within device)
 * 
 * @param bus an I2C bus as described in the file msc_i2c.h
 * @param dev a device address on the spefied bus
 * @param addr a register address within the specified device 
 *                        adressed with ( bus, dev) pair
 * @param data  an 8 bit quantity to be written to the device
 * @param clk a clock indicator used by the cpld to provide 
 *              timing to the operation
 * @return success or failure of the operation
 */
uint8_t 
msc_i2c_write_8 (uint8_t bus,uint8_t dev,uint8_t addr, 
                 uint8_t data,uint8_t clk)
{ 
    uint8_t ctl = clk;
    uint8_t status = 0;

    dtrace(">> %s bus: %x dev: %x clk: %x\n", __FUNCTION__, bus, dev, addr, clk);	
    /* Set SDA, SCL high */
    cpld_wr(CPLD_I2C_CTL,ctl|0x0C);
    udelay (IM_I2C_DLY);
    dprintf("ctl = 0x%x\n", cpld_rd(CPLD_I2C_CTL));

    /* Set the Bus Selet bits 4:0 */
    cpld_wr(CPLD_I2C_BUS_SL,bus & 0x1f);
    udelay (IM_I2C_DLY);
    dprintf("bus = 0x%x\n", cpld_rd(CPLD_I2C_BUS_SL));

    /* Set the Device address */
    cpld_wr(CPLD_I2C_DA,dev<<1);
    udelay (IM_I2C_DLY);
    dprintf("da = 0x%x\n", cpld_rd(CPLD_I2C_DA));

    /* Set the address */
    cpld_wr(CPLD_I2C_ADDR_0, addr);
    udelay (IM_I2C_DLY);
    dprintf("addr_0 = 0x%x\n", cpld_rd(CPLD_I2C_ADDR_0));

    /* Write the Data */
    cpld_wr(CPLD_I2C_DATA, data);
    udelay (IM_I2C_DLY2);
    dprintf("addr_0 = 0x%x\n", cpld_rd(CPLD_I2C_DATA));

    /* Issue the write command */
    cpld_wr(CPLD_I2C_CSR,CPLD_I2C_WR_CMD);
    while ( cpld_rd(CPLD_I2C_CSR) &  CPLD_I2C_CSR_ST);
    dprintf("cmd = 0x%x\n", cpld_rd(CPLD_I2C_CSR));

    status = cpld_rd(CPLD_I2C_STAT);
    dprintf("status = 0x%x\n", status);
    if (status & CPLD_I2C_STAT_NO_ACK) {
        dprintf("No Acknowledge from device\n");
    }
    if (status & CPLD_I2C_STAT_TOUT) {
        dprintf("Timeout - SLC line low for 16*bit period\n");
    }
    if (status & CPLD_I2C_STAT_I2C_FLT) {
        dprintf("I2C Fault SLC or SDA stuck low \n"); 
    }
    return status;
}





/**
 * @INTERNAL
 *
 * This function write a 16 bit quantiy to I2C
 * device specified by bus # device address register address ( within device)
 * 
 * @param bus an I2C bus as described in the file msc_i2c.h
 * @param dev a device address on the spefied bus
 * @param addr a register address within the specified device 
 *                        adressed with ( bus, dev) pair
 * @param data  an 16 bit quantity to be written to the device
 * @param clk a clock indicator used by the cpld to provide 
 *              timing to the operation
 * @return success or failure of the operation
 */
uint8_t 
msc_i2c_write_16 (uint8_t bus,uint8_t dev,uint8_t addr, 
                  uint16_t data, uint8_t clk) 
{
    uint8_t status; 

    dtrace(">> %s bus: %x dev: %x clk: %x\n", __FUNCTION__, bus, dev, addr, clk);	
    status  = msc_i2c_write_8(bus, dev, addr, (data >> 8), clk);
    status |=msc_i2c_write_8(bus, dev, addr, (data & 0xff), clk);

    return status;
}

#endif /* CONFIG_OCTEON_MSC */
