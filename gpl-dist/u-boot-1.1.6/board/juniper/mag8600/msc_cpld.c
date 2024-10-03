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


uint8_t 
cpld_rd (uint8_t offset)
{
    uint64_t val; 
    val = (CPLD_BASE + offset);
    return *((uint8_t*)val);
}

/**
 * @INTERNAL
 * This function writes  a 8 bit quatity at the specified 
 * offset of the Master CPLD
 * @param offset offset within the Master CPL memory space
 * @return none
 */

void 
cpld_wr (uint8_t offset, uint8_t val)
{
    *(uint8_t*)(CPLD_BASE + offset) = val;
    udelay (40);
}

/**
 * @INTERNAL
 * This function is used to retrieve the MASTER
 * CPLD version information
 * Retrieve version information from CPLD 
 * @param version string 
 * @return number of characters in the the version string
 */

int 
cpld_get_version (uint8_t *version)
{

    uint8_t major;
    uint8_t minor;
    uint8_t patch;

    major = cpld_rd(CPLD_MAJOR_FW_VERSION);
    minor = cpld_rd(CPLD_MINOR_FW_VERSION);
    patch = cpld_rd(CPLD_PATCH_FW_VERSION);
    return ( sprintf((char *)version,"%d.%d.%d\n", major,minor,patch)); 
}


/**
 * @INTERNAL
 *
 * This function check if a FAN interrupt has occured 
 * 
 * @param none
 * @return an indicator that the FAN interrupt occured
 */

uint8_t 
cpld_is_fan_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    return (interrupt_status & CPLD_INTERRUPT_STATUS_FAN);
}


/**
 * @INTERNAL
 *
 * This function clears a FAN interrupt has occured 
 * 
 * @param none
 * @return none
 */

void 
cpld_clr_fan_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    if (interrupt_status & CPLD_INTERRUPT_STATUS_FAN) {
        interrupt_status &=~CPLD_INTERRUPT_STATUS_FAN;
        cpld_wr(CPLD_INTERRUPT_STATUS, interrupt_status);
    }
}

/**
 * @INTERNAL
 *
 * This function check if a Power Supply  interrupt has occured 
 * 
 * @param none
 * @return an indicator that the Power Supply interrupt occured
 */
uint8_t 
cpld_is_pwrs_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    return (interrupt_status &  CPLD_INTERRUPT_STATUS_PWR_SUP);
}



/**
 * @INTERNAL
 *
 * This function clears a Power Supply  interrupt has occured 
 * 
 * @param none
 * @return none
 */

void 
cpld_clr_pwrs_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    if (interrupt_status &  CPLD_INTERRUPT_STATUS_PWR_SUP) {
        interrupt_status &=~CPLD_INTERRUPT_STATUS_PWR_SUP;
        cpld_wr(CPLD_INTERRUPT_STATUS, interrupt_status);
    }
}


/**
 * @INTERNAL
 *
 * This function check if a Card Presence  interrupt has occured 
 * 
 * @param none
 * @return an indicator that the Card Presence interrupt occured
 */
uint8_t 
cpld_is_crdprs_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    return (interrupt_status &  CPLD_INTERRUPT_STATUS_CARD_PRS);
}


/**
 * @INTERNAL
 *
 * This function clears a Card Presence  interrupt has occured 
 * 
 * @param none
 * @return nonean indicator that the Card Presence interrupt occured
 */

void 
cpld_clr_crdprs_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    if (interrupt_status &  CPLD_INTERRUPT_STATUS_CARD_PRS) {
        interrupt_status &=~CPLD_INTERRUPT_STATUS_CARD_PRS;
        cpld_wr(CPLD_INTERRUPT_STATUS, interrupt_status); 
    }
}


/**
 * @INTERNAL
 *
 * This function check if a Power Button interrupt has occured 
 * 
 * @param none
 * @return an indicator that the Power Button interrupt occured
 */
uint8_t 
cpld_is_pwrbut_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    return (interrupt_status &   CPLD_INTERRUPT_STATUS_PWR_BUT);
}


/**
 * @INTERNAL
 *
 * This function clear a Power Button interrupt has occured 
 * 
 * @param none
 * @return an indicator that the Power Button interrupt occured
 */
void 
cpld_clr_pwrbut_interrupt (void)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    if (interrupt_status &   CPLD_INTERRUPT_STATUS_PWR_BUT) {
        interrupt_status &=~CPLD_INTERRUPT_STATUS_PWR_BUT;
        cpld_wr(CPLD_INTERRUPT_STATUS, interrupt_status);
    }
}


/**
 * @INTERNAL
 *
 * This function checks if an interrupt for the specifed source 
 * has occured
 * 
 * @param interrupt interrupt source to probe
 * @return the indicator of interrupt occured for the specified 
 *         source
 */

uint8_t 
cpld_is_interrupt (int interrupt)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    return (interrupt_status & (interrupt &0xf));
}


/**
 * @INTERNAL
 *
 * This function 
 * 
 * @param
 * @return 
 */
void 
cpld_clr_interrupt (int interrupt)
{
    uint8_t interrupt_status;

    interrupt_status = cpld_rd(CPLD_INTERRUPT_STATUS);
    if (interrupt_status & (interrupt &0xf)) {
        interrupt_status &=~interrupt;
        cpld_wr(CPLD_INTERRUPT_STATUS, interrupt_status);
    }
}
/**
 * @INTERNAL
 *
 * This function return the status of the specified interrupt
 * as set or reset by the cpld_enable_interrupt function
 * 
 * @param interrupt interrupt enabled/disabled status to probe
 * @return a value containing all the possible status for all 
 *         interrupt source
 */
uint8_t 
cpld_is_enable_interrupt ( int interrupt)
{
    uint8_t interrupt_enabled;
    interrupt_enabled = cpld_rd(CPLD_INTERRUPT_ENABLE);
    return (interrupt_enabled & (interrupt &0xf));
}


/**
 * @INTERNAL
 *
 * This function enable or disable the production of interrupt 
 * for the various interrupt producing MSC components
 * 
 * @param interrupt interrupt number taken among 
 *                  FAN,PWR_SUP,STATUS_CARD_PRS,PWR_BUT
 * 
 * @param mode ENABLE or DISABLE as per desired outcome
 * @return none
 */

void 
cpld_enable_interrupt (uint8_t interrupt, enable_mode_t mode)
{
    uint8_t interrupt_enabled;
    interrupt_enabled = cpld_rd(CPLD_INTERRUPT_ENABLE);
    switch (mode) {
    case ENABLE:
        interrupt_enabled |= interrupt;
        break;
    case DISABLE:
        interrupt_enabled &=~ interrupt;
        break;
    default:
        break;
    } 
    cpld_wr(CPLD_INTERRUPT_ENABLE, interrupt_enabled);
}


/**
 * @INTERNAL
 *
 * This function returns the specified type of status
 * of the specified power supply 
 * 
 * 
 * @param supply_number number of the power supply to probe status for
 * @param status_type 
 * @return a status indicator for the specified power suppply
 */
int 
cpld_ps_status (uint8_t supply_number, status_type_t status_type)
{
    uint8_t ps_status[2];


    switch (status_type) {
    case INT:
        ps_status[0] = cpld_rd(CPLD_PS_STATUS_1);
        return ( ps_status[0] & (0x01 << (supply_number + NIB_SHIFT)));
    case INSERTION:
        ps_status[0] = cpld_rd(CPLD_PS_STATUS_1);
        return ( ps_status[0] & (0x01 << supply_number));
    case INPUT_VOLTAGE:
        ps_status[1] = cpld_rd(CPLD_PS_STATUS_2);
        return ( ps_status[1] & (0x01 << (supply_number + NIB_SHIFT)));
    case OUTPUT_VOLTAGE:
        ps_status[1] = cpld_rd(CPLD_PS_STATUS_2);
        return ( ps_status[1] & (0x01 << supply_number));
    default:
        return 0xff; /* invalid */
    }
    return 0xff;
}


/**
 * @INTERNAL
 *
 * This function 
 * 
 * bits 7:4	PS_OFF[3:0]
 * Writing a 1 into any of these bit positions clears 
 * the corresponding PS_ON bit.
 * These bits are cleared automatically by the hardware.
 * bits 3:0	PS_ON[3:0]
 * These bits drive the power supply PS_ON_[3:0] signals.
 * They are all set to 1 when the power switch is pressed and the power is turned ON.
 * Writing a 1 into any of these bit positions sets the bit to 1.
 * Writing a 0 into any of these bit positions has no effect.
 * To clear any of these bits (to turn OFF any of the power supplies), a 1 must be written into the 
 * corresponding PS_OFF bit position.
 * Writing a 1 into both a PS_ON bit and the corresponding PS_OFF bit causes an indeterminate result.
 * 
 * @param supply number the power supply number in the range [0-MAX_NUM_SUPPLY]
 * @param state desired state of the specified power supply PS_ON or PS_OFF
 * @return none
 */

void 
cpld_ps_control_set (uint8_t supply_number, ps_ctl_state_t state)
{

    switch (state) {
    case PS_ON:
        cpld_wr(CPLD_PS_CTL, (0x01 << supply_number));
        break;
    case PS_OFF:
        cpld_wr(CPLD_PS_CTL, (0x01 << (supply_number + NIB_SHIFT)));
        break;
    default:
        break;
    }
}


/**
 * @INTERNAL
 *
 * This function rturn a presence indication for the specified 
 * card slot. The max number of cards in the system is specifed
 * for MSC board to be 12 (see msc_cpld.h)
 * CARD_PRESENCE_1 - Card Presence Status 1, read only
 * A bit is set to 1 when the corresponding card is inserted in the bus slot
 * 
 * bit 7	Card 7 presence
 * bit 6	Card 6 presence
 * bit 5	Card 5 presence
 * bit 4	Card 4 presence
 * bit 3	Card 3 presence
 * bit 2	Card 2 presence
 * bit 1	Card 1 presence
 * bit 0	Card 0 presence

 * CARD_PRESENCE_2 - Card Presence Status 2, read only

 * bit 7:4	reserved
 * bit 3	Card 11 presence
 * bit 2	Card 10 presence
 * bit 1	Card 9 presence
 * bit 0	Card 8 presence
 * 
 * @param card_number card number ( slot number ) of the card 
 *                    to probe
 * @return a boolean indicator of precense or absence 
 */

uint8_t 
cpld_is_card_present (uint8_t card_number)
{
    if (card_number >= 0 && card_number < MAX_NUMBER_OF_CARDS) {
        if (card_number < 8) {
            return (cpld_rd(CPLD_CARD_PRESENCE_1) & (0x01 << card_number));
        } else {
            return (cpld_rd(CPLD_CARD_PRESENCE_2) & (0x01 << (card_number - 8)));
        }
    }
    return 0;
}


/**
 * @INTERNAL
 *
 * This function allow the selection of one serial port taken in the 
 * range 0-MAX_NUMBER_OF_CARDS
 * 
 * @param card number card number taken in the range 0-MAX_NUMBER_OF_CARDS
 * @return none
 */

void 
cpld_serial_select (uint8_t card_number)
{
    if (card_number >= 0 || card_number < MAX_NUMBER_OF_CARDS) {
        cpld_wr(CPLD_SERIAL_MUX_SELECT, (card_number & CPLD_SERIAL_MUX_SELECT_MASK));
    }
}


/**
 * @INTERNAL
 *
 * This function returns the currently selected serial port 
 * 
 * @param none
 * @return which or the cards in the range 0-MAX_NUMBER_OF_CARDS has 
 *         the serail port
 */
uint8_t 
cpld_which_serial_selected (void) 
{
    return (cpld_rd( CPLD_SERIAL_MUX_SELECT)& CPLD_SERIAL_MUX_SELECT_MASK);
}

/**
 * @INTERNAL
 *
 * This function 
 * 
 * 
 * DEVICE_RESET -
 * bits 7:4	reserved
 * bit 3	Software Reset
 * When a 1 is written into this bit position, the CPLD is reset
 * Cleared automatically by the hardware.
 * 
 * bit 2	MSC Octeon Processor Reset (‘self-kill’)
 * When a 1 is written into this bit position, the Octeon processor Reset  signal is asserted for 
 * 	about 30 ms. Cleared automatically by the hardware.
 * 
 * bit 1	Cheetah Switch Reset
 * When a 1 is written into this bit position, the Cheetah Switch Reset  signal is asserted for 
 * 	about 8 ms. Cleared automatically by the hardware.
 * 
 * bit 0	Lion Switch Reset
 * When a 1 is written into this bit position, the Lion Switch Reset  signal is asserted for 
 * 	about 8 ms. Cleared automatically by the hardware. 
 * @param device device to be reset 
 * @return none
 */

void 
cpld_device_reset (device_type_t device)
{
    switch (device) {
    case LION:
        cpld_wr(CPLD_DEVICE_RESET,  CPLD_DEVICE_RESET_LION);
        break;
    case CHEETAH:
        cpld_wr(CPLD_DEVICE_RESET,  CPLD_DEVICE_RESET_CHEETAH);
        break;
    case MSC:
        cpld_wr(CPLD_DEVICE_RESET,  CPLD_DEVICE_RESET_MSC);
        break;
    case SW:
        cpld_wr(CPLD_DEVICE_RESET,  CPLD_DEVICE_RESET_SW);
        break;
    default:
        break;
    }
}


/* LED will not be implemented for now */
/* The I2C CPLD interface is exercised in the module miix */
/* The Slave CPLD functionality will not be implemented for now */


#endif /* CONFIG_OCTEON_MSC */
