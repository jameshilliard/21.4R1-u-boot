/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.    
 * All rights reserved.
 *
 * recb_iic.c -- PCF8584 I2C driver routines for recb FPGA
 *
 * Rajshekar, Nov 2008
 *
 * Copyright (c) 2005, Juniper Networks, Inc.
 * All rights reserved.
 */

#include "common.h"
#include "recb.h"
#include "pcf8584.h"
#include "recb_iic.h"

/*
 * We need a 4msec delay on iic transactions to allow for clock stretching.
 * The iic driver (sys/dev/iic/pcf8584.c) wait API (pcf8584_wait) performs
 * a busy loop in order to wait for the transaction to finish.  This loop 
 * runs for PCF8584_MAX_LOOP cycles.  So the necessary delay for each loop
 * is 4ms / PCF8584_MAX_LOOP or 4000 usec
 */
#define RECB_IIC_HW_DELAY 4000 / PCF8584_MAX_LOOP

static u_char first_time = 1;

/*
 * These routines implement the recb specific interfaces for IIC.
 */

static recb_fpga_regs_t *global_recb_p = 0;

#define WAITFOR_IDLE        10000

static int
recb_slowbus_pcf8584_reg_write(int reg, uint8_t value) {
    int i;
    volatile uint32_t junk;
    recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)global_recb_p;
    recb_slow_bus_t  *slow_bus_p = &recb_p->slow_bus;

    i = 0;
    while ((slow_bus_p->recb_slow_bus_ctrl & RECB_SLOW_BUS_CTRL_ACTIVE) && 
           (i++ < WAITFOR_IDLE)) {
        DELAY(1);
    }
    if (i >= WAITFOR_IDLE) {
#ifdef  RECB_DEBUG
        printf("recb_iic_ctlr_reg_write timed out waiting for idle\n");
#endif  /* RECB_DEBUG */
        return -1;
    }

    slow_bus_p->recb_slow_bus_data = value;
    udelay(10);

    slow_bus_p->recb_slow_bus_addr = ((reg) ? 1 : 0);
    udelay(10);

    slow_bus_p->recb_slow_bus_ctrl = ((RECB_SLOW_BUS_CTRL_SEL_I2C | 
                                       RECB_SLOW_BUS_CTRL_WRITE |
                                       RECB_SLOW_BUS_CTRL_START));
    udelay(10);


    junk = (recb_p->recb_version);     /* dummy read to push the writes out */

    /* don't bother waiting for completion */

    return 0;
}

static uint8_t
recb_slowbus_pcf8584_reg_read(int reg) {
    int i;
    recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)global_recb_p;
    recb_slow_bus_t*  slow_bus_p = &recb_p->slow_bus;

    i = 0;
    while ((slow_bus_p->recb_slow_bus_ctrl & RECB_SLOW_BUS_CTRL_ACTIVE) && 
           (i++ < WAITFOR_IDLE)) {
        DELAY(1);
    }
    if (i >= WAITFOR_IDLE) {
#ifdef  RECB_DEBUG
        printf("recb_iic_ctlr_reg_read timed out waiting for idle\n");
#endif  /* RECB_DEBUG */
        return -1;
    }
    slow_bus_p->recb_slow_bus_addr = ((reg) ? 1 : 0);
    udelay(10);

    slow_bus_p->recb_slow_bus_ctrl = ((RECB_SLOW_BUS_CTRL_SEL_I2C | 
                                           RECB_SLOW_BUS_CTRL_READ |
                                           RECB_SLOW_BUS_CTRL_START));
    udelay(10);

    i = 0;
    while ((slow_bus_p->recb_slow_bus_ctrl & RECB_SLOW_BUS_CTRL_ACTIVE) && 
           (i++ < WAITFOR_IDLE)) {
        DELAY(1);
    }
    if (i >= WAITFOR_IDLE) {
#ifdef  RECB_DEBUG
        printf("recb_iic_ctlr_reg_read timed out waiting for data\n");
#endif  /* RECB_DEBUG */
        return -1;
    }
    udelay(10);

    return(slow_bus_p->recb_slow_bus_data);
}

void
recb_iic_ctlr_group_select ( u_char group ) {
    volatile uint32_t junk;
    recb_fpga_regs_t *recb_p = (recb_fpga_regs_t *)global_recb_p;
    recb_slow_bus_t*  slow_bus_p = &recb_p->slow_bus;

    
    slow_bus_p->recb_slow_bus_i2c_sel =  group;
    udelay(10);
   
    junk = recb_p->recb_version;         /* dummy read to push the write */
    udelay(10);

    // DELAY(PCF8584_SELECT_DELAY);
}

static int
recb_iic_ctlr_init ( void ) {
    /*
     * Put the device in init mode, set the clock to 4.43Mhz and enable
     * communication.
     */
    recb_slowbus_pcf8584_reg_write(PCF8584_S1, PCF8584_CR_PIN);
    recb_slowbus_pcf8584_reg_write(PCF8584_S0, PCF8584_OWN_ADDR);
    recb_slowbus_pcf8584_reg_write(PCF8584_S1, PCF8584_CR_PIN|PCF8584_CR_ES1);
    recb_slowbus_pcf8584_reg_write(PCF8584_S0, PCF8584_CLOCK_4_43MHZ);
    recb_slowbus_pcf8584_reg_write(PCF8584_S1, 
                                  PCF8584_CR_PIN|PCF8584_CR_ES0|PCF8584_CR_ACT);

    if (first_time) {   /* don't mess with group select after the first time */
        recb_iic_ctlr_group_select(0);
        current_group = 0;
        first_time = 0;
    }

    return(0);
}

int
recb_iic_init(void) {

    /* Get the base address for fpga registers*/
    global_recb_p = (uint32_t *)CFG_PARKES_BASE;

    /*
     * Init the RECB IIC controller HW vector
     */
    iic_hw_init_vect = recb_iic_ctlr_init;
    iic_hw_read_vect = recb_slowbus_pcf8584_reg_read;
    iic_hw_write_vect = recb_slowbus_pcf8584_reg_write;
    iic_hw_group_select_vect = recb_iic_ctlr_group_select;
    iic_hw_delay_usec = RECB_IIC_HW_DELAY;

    iic_ctlr_init();

    return(0);
}

int
recb_iic_read (u_int8_t group, u_int8_t device,
              size_t bytes, u_int8_t *buf) {
    return(iicbus_read (group, device,bytes, buf));
}

int
recb_iic_write (u_int8_t group, u_int8_t device,
               size_t bytes, u_int8_t *buf) {
    return(iicbus_write (group, device,bytes, buf));
}

int
recb_iic_probe (u_int8_t group, u_int8_t device) {
    u_int8_t buf;
    return(iicbus_read (group, device,1, &buf));
}

#define I2CS_PARITY_MASK  0x80
#define	NBBY		         8	   
u_int8_t
recb_i2cs_set_odd_parity(u_int8_t data) {
    int        i;
    u_int8_t   parity;

    for (i = 0, parity = I2CS_PARITY_MASK; i < NBBY; i++) {
        if (data & (1 << i))
            parity ^=  I2CS_PARITY_MASK;
    }
    data ^= parity;
    return data;
}



/* end of file */

