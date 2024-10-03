/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * $Id: pcf8584.c,v 1.1.86.1 2009-08-27 06:23:47 dtang Exp $
 *
 * pcf8584.c -- PCF8584 I2C driver
 *
 * Rob Clevenger, March 1999
 * Adapted from embedded microkernel driver originally written by 
 * Michael Beesley, July 1997
 *
 * Copyright (c) 1999-2003, 2006, Juniper Networks, Inc.
 * All rights reserved.
 */


/*
 * Include files
 */
#include <common.h>
#include "pcf8584.h"
#include <asm-ppc/errno.h>

#define TRUE 	1
#define FALSE	0

#define  DEBUG 1
#ifdef DEBUG
#define DEBUGF(x...) printf(x)
#else
#define DEBUGF(x...)
#endif /* DEBUG */

/*
 * Local storage
 */
u_char current_group;
static u_int iic_busy_timeouts;
static u_int iic_busy_loops;
static u_int iic_bytes_read;
static u_int iic_bytes_written;
static u_int iic_target_ack_timeouts;
static u_int iic_target_timeouts;
static u_int iic_xmit_failures;
u_int iic_hw_delay_usec = 1;
iic_hw_init_vect_t iic_hw_init_vect = NULL;
iic_hw_read_vect_t iic_hw_read_vect = NULL;
iic_hw_write_vect_t iic_hw_write_vect = NULL;
iic_hw_group_select_vect_t iic_hw_group_select_vect = NULL;

/*
 * pcf8584_init:
 * Init routine. Must be called before all other primitives.
 */
void
iic_ctlr_init (void) {
    if (iic_hw_init_vect) (*iic_hw_init_vect)();
}

/*
 * pcf8584_wait:
 * Wait on a bitmask in i2c_s1 register. Return 0 on success else error.
 */
static int
pcf8584_wait (u_int8_t mask, int onoff) {
    int i;

    if (!iic_hw_read_vect) return(0);

    for (i=0; i<PCF8584_MAX_LOOP; i++) {
        if (i > iic_busy_loops) iic_busy_loops = i;

        if (onoff && ((*iic_hw_read_vect)(PCF8584_S1) & mask)) {
            return(0);
        }
        if (!onoff && !((*iic_hw_read_vect)(PCF8584_S1) & mask)) {
            return(0);
        }

        if (iic_hw_delay_usec) {
            DELAY(iic_hw_delay_usec);
        }
    }
    return(EBUSY);          /* timed out */
}

/*
 * Attempt to clear a busy condition.
 * Reset the controller and drive (min. 10) SCLKs onto the bus
 * to clear a locked up misbehaving slave device.  Reset the
 * controller again.
 * Note: The delay to allow for 10 SCLKs is frequeny dependant.
 */
static void
pcf8584_clr_busy (void) {

    iic_ctlr_init();   /* re-init this pup */
    DELAY(2);

    if (!iic_hw_write_vect) return;

    /*
     * Start a transaction to drive SCLK onto the bus
     */
    (*iic_hw_write_vect)(PCF8584_S1,
                         (PCF8584_CR_ES0 |
                          PCF8584_CR_STA | PCF8584_CR_ACT));

    DELAY(120);  /* allow time for 10 SCLKs */

    iic_ctlr_init();   /* re-init this pup again */
    DELAY(2);
}

/*
 * iicbus_read:
 * Read a number of bytes from an I2C device. Return 0 on success,
 * else error code.
 */
int
iicbus_read (u_int8_t group, u_int8_t device,
             size_t bytes, u_int8_t *buf) {
    size_t      index;
    u_char      s0_data;
    int         slave_address_read;

#ifdef DEBUG
    char routine_id[] = "PCF8584(RD):";
#endif

    /*
     * Make sure there are some bytes
     */
    if (bytes == 0) return(0);

    /*
     * make sure that we have methods to talk with the hardware
     */
    if (!iic_hw_read_vect || !iic_hw_write_vect) return(ENOENT);

    /*
     * If the select_reg pointer is not NULL, select the group
     */
    if (iic_hw_group_select_vect) {
        (*iic_hw_group_select_vect)(group);
    }

    /*
     * Make sure the device is not busy
     */
    for (index = 0; ; index++) {

        if (pcf8584_wait(PCF8584_SR_BUSY, TRUE) == 0) break;

        if (index > 2) {
            iic_busy_timeouts++;
            DEBUGF("%s busy timeout at start\n", routine_id);
            DEBUGF("%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
                   routine_id, (*iic_hw_read_vect)(PCF8584_S1), group, device);
            return(EBUSY);
        }

        DEBUGF("%s busy at start, attempting to clear\n", routine_id);
        DEBUGF("%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
               routine_id, (*iic_hw_read_vect)(PCF8584_S1), group, device);

        pcf8584_clr_busy();
    }

    /*
     * Send the slave device address and start and wait for PIN
     */
    (*iic_hw_write_vect)(PCF8584_S0, (device << 1) | PCF8584_I2C_RD);
    (*iic_hw_write_vect)(PCF8584_S1, (PCF8584_CR_PIN | PCF8584_CR_ES0 |
                                      PCF8584_CR_STA | PCF8584_CR_ACT));
    slave_address_read = FALSE;

    /*
     * Read the required bytes
     */
    index = 0;
    while (TRUE) {

        /*
         * Wait for the controller
         */
        if (pcf8584_wait(PCF8584_SR_PIN, FALSE) != 0) {
            DEBUGF("%s target timeout\n", routine_id);
            DEBUGF("%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
                   routine_id, (*iic_hw_read_vect)(PCF8584_S1), 
                   group, device);
            iic_target_timeouts++;
            return(EIO);
        }

        /*
         * Check for LRB = 0 (slave acknowledge)
         */
        if (pcf8584_wait(PCF8584_SR_LRB, FALSE) != 0) {
            DEBUGF("%s target ack timeout\n", routine_id);
            DEBUGF("%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
                   routine_id, (*iic_hw_read_vect)(PCF8584_S1), 
                   group, device);

            /* generate stop condition */
            (*iic_hw_write_vect)(PCF8584_S1,
                                 (PCF8584_CR_PIN | PCF8584_CR_ES0 |
                                  PCF8584_CR_STO | PCF8584_CR_ACT));
            /* read and discard */
            s0_data = (*iic_hw_read_vect)(PCF8584_S0);
            iic_target_ack_timeouts++;
            return(EIO);
        }

        /*
         * Are we done ?
         */
        if (bytes == 1 || 
            (slave_address_read && (index + 2) >= bytes))
            break;

        /*
         * Read a byte. If this is the slave_address, dump it and go
         * on to the next byte. Otherwise keep it.
         */
        s0_data = (*iic_hw_read_vect)(PCF8584_S0);
        if (!slave_address_read) {
            slave_address_read = TRUE;
        } else {
            iic_bytes_read++;
            buf[index++] = s0_data;
        }

    }

    /*
     * Negative acknowledge and read byte before last. Dump it if it
     * is the slave address.
     */
    (*iic_hw_write_vect)(PCF8584_S1, PCF8584_CR_ES0);
    s0_data = (*iic_hw_read_vect)(PCF8584_S0);
    if (slave_address_read) {
        iic_bytes_read++;
        buf[index++] = s0_data;
    }

    /*
     * Wait for the controller
     */
    if (pcf8584_wait(PCF8584_SR_PIN, FALSE) != 0) {
        DEBUGF("%s ack failure on 2nd last byte\n", routine_id);
        DEBUGF("%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
               routine_id, (*iic_hw_read_vect)(PCF8584_S1), group, device);
        iic_target_ack_timeouts++;
        return(EIO);
    }


    /*
     * Send stop and read the last byte
     */
    (*iic_hw_write_vect)(PCF8584_S1, (PCF8584_CR_PIN | PCF8584_CR_ES0 |
                                      PCF8584_CR_STO | PCF8584_CR_ACT));
    buf[index] = (*iic_hw_read_vect)(PCF8584_S0);
    iic_bytes_read++;

    /*
     * Wait for the controller
     */
    if (pcf8584_wait(PCF8584_SR_BUSY, TRUE) != 0) {
        DEBUGF("%s busy timeout at end\n", routine_id);
        DEBUGF("%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
               routine_id, (*iic_hw_read_vect)(PCF8584_S1), group, device);
        iic_busy_timeouts++;
    }
    return(0);
}

/*
 * pcf8584_write:
 * Write to a device over the I2C bus. Return 0 on success else
 * error code.
 */
int
iicbus_write (u_int8_t group, u_int8_t device,
              size_t bytes, u_int8_t *buf) {
    u_char index;

#ifdef DEBUG
    char routine_id[] = "PCF8584(WR):";
#endif

    /*
     * make sure that we have methods to talk with the hardware
     */
    if (!iic_hw_read_vect || !iic_hw_write_vect) return(ENOENT);

    /*
     * If the select_reg pointer is not NULL, select the group
     */
    if (iic_hw_group_select_vect) {
        (*iic_hw_group_select_vect)(group);
        current_group = group;
        DELAY(PCF8584_SELECT_DELAY);
    }

    /*
     * Make sure the device is not busy
     */
    for (index = 0; ; index++) {

        if (pcf8584_wait(PCF8584_SR_BUSY, TRUE) == 0) break;

        if (index > 2) {
            iic_busy_timeouts++;
            DEBUGF( "%s busy timeout at start\n", routine_id);
            DEBUGF( "%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
                    routine_id, (*iic_hw_read_vect)(PCF8584_S1), group, device);
            return(EBUSY);
        }

        DEBUGF("%s busy at start, attempting to clear\n", routine_id);
        DEBUGF("%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
               routine_id, (*iic_hw_read_vect)(PCF8584_S1), group, device);

        pcf8584_clr_busy();
    }

    /*
     * Send the slave device address and start and write the data
     */
    (*iic_hw_write_vect)(PCF8584_S0, (device << 1) | PCF8584_I2C_WR);
    (*iic_hw_write_vect)(PCF8584_S1,
                         (PCF8584_CR_PIN | PCF8584_CR_ES0 |
                          PCF8584_CR_STA | PCF8584_CR_ACT));

    for (index=0; ; index++) {


        if (pcf8584_wait(PCF8584_SR_PIN, FALSE) != 0) {
            DEBUGF( "%s transmit failure on byte %d\n",
                    routine_id, index);
            DEBUGF( "%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
                    routine_id, (*iic_hw_read_vect)(PCF8584_S1), 
                    group, device);

            /* stop the transmission */
            (*iic_hw_write_vect)(PCF8584_S1,
                                 (PCF8584_CR_PIN | PCF8584_CR_ES0 |
                                  PCF8584_CR_STO | PCF8584_CR_ACT));
            iic_xmit_failures++;
            return(EIO);
        }



        if (pcf8584_wait(PCF8584_SR_LRB, FALSE) != 0) {
            DEBUGF( "%s target ack failure on byte %d\n",
                    routine_id, index);
            DEBUGF( "%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
                    routine_id, (*iic_hw_read_vect)(PCF8584_S1), 
                    group, device);

            /* stop the transmission */
            (*iic_hw_write_vect)(PCF8584_S1,
                                 (PCF8584_CR_PIN | PCF8584_CR_ES0 |
                                  PCF8584_CR_STO | PCF8584_CR_ACT));
            iic_target_ack_timeouts++;
            return(EIO);
        }

        if (index >= bytes) break;

        (*iic_hw_write_vect)(PCF8584_S0, buf[index]);

    }

    /*
     * Send stop
     */
    (*iic_hw_write_vect)(PCF8584_S1,
                         (PCF8584_CR_PIN | PCF8584_CR_ES0 |
                          PCF8584_CR_STO | PCF8584_CR_ACT));

    /*
     * Wait for controller
     */
    if (pcf8584_wait(PCF8584_SR_BUSY, TRUE) != 0) {
        DEBUGF( "%s busy timeout at end\n", routine_id);
        DEBUGF( "%s (i2c_s1=0x%02x, group=%#x, device=%#x)\n",
                routine_id, (*iic_hw_read_vect)(PCF8584_S1), group, device);
        iic_busy_timeouts++;
    }

    iic_bytes_written++;
    return(0);
}

/* end of file */
