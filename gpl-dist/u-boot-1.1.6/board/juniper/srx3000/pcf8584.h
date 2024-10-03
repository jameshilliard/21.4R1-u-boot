/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * $Id: pcf8584.h,v 1.1.110.2 2009-08-13 04:04:01 kdickman Exp $
 *
 * pcf8584.h -- PCF8584 I2C driver
 *
 * Rob Clevenger, March 1999
 * Adapted from embedded microkernel driver originally written by 
 * Michael Beesley, July 1997
 *
 * Copyright (c) 1999-2003, 2006, Juniper Networks, Inc.
 * All rights reserved.
 */

#ifndef __PCF8584_H__
#define __PCF8584_H__

#include <linux/types.h>

//#define DELAY(n)        { register int N = (n); while (--N > 0); }
#define DELAY(n)        udelay((n))

/*
 * Denote the two addressable pcf8584 registers
 */
#define PCF8584_S0              0
#define PCF8584_S1              1

/*
 * Read/Write bit in S0 address field
 */
#define PCF8584_I2C_RD          0x01
#define PCF8584_I2C_WR          0x00

/*
 * PCF8584 S1 control bits
 */
#define PCF8584_CR_PIN          (1<<7)  /* no interrupts */
#define PCF8584_CR_ES0          (1<<6)  /* transfer mode */
#define PCF8584_CR_ES1          (1<<5)  /* clock register access */
#define PCF8584_CR_ES2          (1<<4)  /* vector register access */
#define PCF8584_CR_ENI          (1<<3)  /* enable interrupts */
#define PCF8584_CR_STA          (1<<2)  /* start bit */
#define PCF8584_CR_STO          (1<<1)  /* stop bit */
#define PCF8584_CR_ACT          (1<<0)  /* auto send act bit after each byte */

/*
 * PCF8584 S1 status bits
 */
#define PCF8584_SR_PIN          (1<<7)  /* 0=interrupt, 1=no interrupt */
#define PCF8584_SR_BER          (1<<4)  /* bus error occured */
#define PCF8584_SR_LRB          (1<<3)  /* last bit received */
#define PCF8584_SR_BUSY         (1<<0)  /* 0=busy, 1=not busy */

/*
 * PCF8584 misc. defines
 */
#define PCF8584_MAX_LOOP        20      /* will run wait loops 10 times */
#define PCF8584_DELAY           50      /* each time, we'll wait 50usec */
#define PCF8584_CLOCK_12MHZ     0x1C    /* 12Mhz/90Khz SCL clock */
#define PCF8584_CLOCK_4_43MHZ   0x10    /* 4.43Mhz/90Khz SCL clock */
#define PCF8584_CLOCK_3MHZ      0x00    /* 3Mhz/90Khz SCL clock */
#define PCF8584_OWN_ADDR        0x55    /* own address */

#define PCF8584_SELECT_DELAY    10000     /* delay after changing groups, usec */
#define PCF8584_REG_DELAY       DELAY(1000)


/*
 * Protos
 */

/* define vector to IIC HW Controller init routine */
typedef int (*iic_hw_init_vect_t)(void);
typedef uint8_t (*iic_hw_read_vect_t)(int);
typedef int (*iic_hw_write_vect_t)(int, uint8_t);
typedef void (*iic_hw_group_select_vect_t)(u_char);

extern iic_hw_init_vect_t iic_hw_init_vect;
extern iic_hw_read_vect_t iic_hw_read_vect;
extern iic_hw_write_vect_t iic_hw_write_vect;
extern iic_hw_group_select_vect_t iic_hw_group_select_vect;
extern u_char current_group;
extern u_int iic_hw_delay_usec;
extern void iic_ctlr_init (void);
extern int iicbus_read (u_int8_t group, u_int8_t device,size_t bytes, u_int8_t *buf);
extern int iicbus_write (u_int8_t group, u_int8_t device,size_t bytes, u_int8_t *buf);

#endif /* __PCF8584_H__ */

/* end of file */
