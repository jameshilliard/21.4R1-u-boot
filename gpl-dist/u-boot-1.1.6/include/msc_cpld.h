
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


#ifndef _msc_cpld_h_
#define _msc_cpld_h_

#define CPLD_BASE	(0xFFFFFFFFBDB00000ULL)

/* MASTER CPLD */

#define CPLD_SCRATCH_REGISTER		0x00 	/* r/w */

#define CPLD_MAJOR_FW_VERSION		0x01    /* ro  */

#define CPLD_MINOR_FW_VERSION   	0x02    /* ro  */

#define CPLD_PATCH_FW_VERSION		0x03 	/* ro  */

#define CPLD_INTERRUPT_STATUS		0x04    /* r/w */

#define CPLD_INTERRUPT_STATUS_FAN	(0x01 << 0) 
#define CPLD_INTERRUPT_STATUS_PWR_SUP	(0x01 << 1)
#define CPLD_INTERRUPT_STATUS_CARD_PRS  (0x01 << 2)
#define CPLD_INTERRUPT_STATUS_PWR_BUT	(0x01 << 3)

typedef enum 
{
  ENABLE,
  DISABLE
} enable_mode_t; 

#define CPLD_INTERRUPT_ENABLE		0x05    /* r/w */

#define CPLD_INTERRUPT_ENABLE_FAN       (0x01 << 0)
#define CPLD_INTERRUPT_ENABLE_PWR_SUP   (0x01 << 1)
#define CPLD_INTERRUPT_ENABLE_CARD_PRS  (0x01 << 2)
#define CPLD_INTERRUPT_ENABLE_PWR_BUT   (0x01 << 3)

#define CPLD_PS_STATUS_1		0x06 	/* ro  */
#define CPLD_PS_STATUS_2		0x07	/* ro  */

typedef enum {
  INT = 0,
  INSERTION,
  INPUT_VOLTAGE,
  OUTPUT_VOLTAGE
} status_type_t;

/* 
 * this is the shift to access the upper nibble of the 
 * power supply control register to affect off condition
 */
#define NIB_SHIFT 4

/* Maximun number of Power Supplies on MSC board */
#define MAX_NUM_SUPPLY 4

#define CPLD_PS_CTL			0x08	/* r/w */


typedef enum {
  PS_ON = 0,
  PS_OFF,
} ps_ctl_state_t;



#define MAX_NUMBER_OF_CARDS 12

#define CPLD_CARD_PRESENCE_1		0x09 	/* ro */
#define CPLD_CARD_0_PRESENT		(0x01 << 0)
#define CPLD_CARD_1_PRESENT		(0x01 << 1)
#define CPLD_CARD_2_PRESENT		(0x01 << 2)
#define CPLD_CARD_3_PRESENT		(0x01 << 3)
#define CPLD_CARD_4_PRESENT		(0x01 << 4)
#define CPLD_CARD_5_PRESENT		(0x01 << 5)
#define CPLD_CARD_6_PRESENT		(0x01 << 6)
#define CPLD_CARD_7_PRESENT		(0x01 << 7)

#define CPLD_CARD_PRESENCE_2		0x0A 	/* ro */
#define CPLD_CARD_8_PRESENT		(0x01 << 0)
#define CPLD_CARD_9_PRESENT		(0x01 << 1)
#define CPLD_CARD_10_PRESENT		(0x01 << 2)
#define CPLD_CARD_11_PRESENT		(0x01 << 3)
/* bit 7-4 of this reg are for now reserved */

#define CPLD_SERIAL_MUX_SELECT  	0x0B    /* r/w */
#define CPLD_SERIAL_MUX_SELECT_MASK	0x1F    

typedef enum {
  LION = 0, 
  CHEETAH,
  MSC, 
  SW
}device_type_t;

#define CPLD_DEVICE_RESET		0x0C	/* r/w */
#define CPLD_DEVICE_RESET_LION		0x01 << 0
#define CPLD_DEVICE_RESET_CHEETAH	0x01 << 1
#define CPLD_DEVICE_RESET_MSC		0x01 << 2
#define CPLD_DEVICE_RESET_SW		0x01 << 3


#define CPLD_LED_CTRL_1			0x0D	/* r/w */
#define CPLD_LED_CTRL_LED0_G_ACTIV	(0x01 << 0)
#define CPLD_LED_CTRL_LED0_G_BLINK	(0x01 << 1)
#define CPLD_LED_CTRL_LED0_R_ACTIV	(0x01 << 2)
#define CPLD_LED_CTRL_LED0_R_BLINK	(0x01 << 3)

#define CPLD_LED_CTRL_LED1_G_ACTIV	(0x01 << 4)
#define CPLD_LED_CTRL_LED1_G_BLINK	(0x01 << 5)
#define CPLD_LED_CTRL_LED1_R_ACTIV	(0x01 << 6)
#define CPLD_LED_CTRL_LED1_R_BLINK	(0x01 << 7)

#define CPLD_LED_CTRL_2			0x0E	/* r/w */
#define CPLD_LED_CTRL_LED2_G_ACTIV	(0x01 << 0)
#define CPLD_LED_CTRL_LED2_G_BLINK	(0x01 << 1)
#define CPLD_LED_CTRL_LED2_R_ACTIV	(0x01 << 2)
#define CPLD_LED_CTRL_LED2_R_BLINK	(0x01 << 3)

/*
 * I2C Bus Select
 * 
 */
#define CPLD_I2C_BUS_SL			0x0F	/* r/w */

#define CPLD_I2C_CSR			0x10	/* r/w */

#define CPLD_I2C_CSR_ST                 0x01    /* Operation Start Clear when finished */
#define CPLD_I2C_CDR_STP                0x02    /* Operation Stop Cleared auto by hw */

#define CPLD_I2C_CDR_READ_BYTE          (0x00 << 4)
#define CPLD_I2C_CDR_WRITE_BYTE         (0x01 << 4)
#define CPLD_I2C_CDR_ACK_POLL           (0x10 << 4)

#define CPLD_I2C_DA			0x11	/* r/w */

#define CPLD_I2C_DATA			0x12	/* r/w */

#define CPLD_I2C_STAT			0x13	/* r/w */

#define CPLD_I2C_STAT_NO_ACK            0x01    /* bit 0 No acknowledge RO */
#define CPLD_I2C_STAT_TOUT              0x02    /* bit 1 Timeout RO*/
#define CPLD_I2C_STAT_I2C_FLT           0x04    /* bit 2 I2C Fault RO */

#define CPLD_I2C_ADDR_0			0x14	/* r/w */

#define CPLD_I2C_ADDR_1			0x15	/* r/w */

#define CPLD_I2C_CTL			0x16	/* r/w */
/* Register 0x17-0x3F are for now reserved */


#define CPLD_I2C_CTL_1000		0
#define CPLD_I2C_CTL_500		1<<4
#define CPLD_I2C_CTL_100		2<<4
#define CPLD_I2C_CTL_50			3<<4
#define CPLD_I2C_STOP_CMD		0x02
#define CPLD_I2C_RD_CMD			0x01
#define CPLD_I2C_WR_CMD			0x11
#define IM_I2C_DLY			4000
#define IM_I2C_DLY2			40000

/* Slave CPLD */
#define S_CPLD_SCRATCH_REGISTER		0x00

#define S_CPLD_MAJOR_FW_VERSION		0x01    /* ro  */

#define S_CPLD_MINOR_FW_VERSION   	0x02    /* ro  */

#define S_CPLD_PATCH_FW_VERSION		0x03 	/* ro  */

#define S_CPLD_HOST_SW_TYPE		0x04	/* ro */
#define S_CPLD_HOST_SW_TYPE_MASK	0x1F   

#define S_CPLD_SWTYPE_INVALID		0x00
#define S_CPLD_SWTYPE_SA		0X01
#define S_CPLD_SWTYPE_IC		0x02
#define S_CPLD_SWTYPE_URSYNC_SERVER	0x03
#define S_CPLD_SWTYPE_IF_MAP_SERVER	0x04	
#define S_CPLD_SWTYPE_UMD_SENSOR	0x05	
#define S_CPLD_SWTYPE_UMD_COLLECTOR	0x06
#define S_CPLD_SWTYPE_SA_IC_SLB		0x07
#define S_CPLD_SWTYPE_HVISOR_VMWARE	0x08
#define S_CPLD_SWTYPE_HVISOR_HYPERV	0x09
/* values b1011 - b111 */ 

#define S_CPLD_HOST_MAJOR_SW_VERSION		0x05    /* ro  */

#define S_CPLD_HOST_MINOR_SW_VERSION   		0x06    /* ro  */

#define S_CPLD_HOST_PATCH_SW_VERSION		0x07 	/* ro  */

#define S_CPLD_LICENSE_FIELD_1			0x08	/* ro */
#define S_CPLD_LICENSE_FIELD_2			0x09	/* ro */
#define S_CPLD_LICENSE_FIELD_3			0x0A	/* ro */
#define S_CPLD_LICENSE_FIELD_4			0x0B	/* ro */

/* 
 * It is a value represented on 2 bytes 
 *   in multiple of 256MB 
 */
#define S_CPLD_HOST_MEM_CFG_HI			0x0C	/* ro */
#define S_CPLD_HOST_MEM_CFG_LO			0x0D	/* ro */

#define S_CPLD_CARD_PWR_STATE			0x0E	/* ro */
#define S_CPLD_PREV_CARD_PWR_STATE		0x0F	/* ro */
#define S_CPLD_CARD_PWR_STATE_MASK		0x07	/* ro */

#define S_CPLD_CARD_PWR_STATE_INVALID		0x00
#define S_CPLD_CARD_PWR_STATE_CRD_OFFLINE	0x01
#define S_CPLD_CARD_PWR_STATE_CRD_PWR_UP	0x02
#define S_CPLD_CARD_PWR_STATE_CRD_SYS_CHECK	0x03
#define S_CPLD_CARD_PWR_STATE_CRD_ONLINE	0x04
#define S_CPLD_CARD_PWR_STATE_CRD_PWR_DOWN	0x05

/* value b110-b111 reserved */



#define S_CPLD_CARD_MAJOR_RUNTIME_STATE		0x10	/* ro */
#define S_CPLD_CARD_PREV_MAJOR_RUNTIME_STATE	0x11	/* ro */


#define S_CPLD_CARD_MINOR_RUNTIME_STATE		0x12	/* ro */
#define S_CPLD_CARD_PREV_MINOR_RUNTIME_STATE	0x13	/* ro */

#define S_CPLD_SLOT_ID				0x14	/* ro */

#define S_CPLD_LED_CTRL_1			0x15    /* r/w */

#define S_CPLD_LED_CTRL_2			0x16	/* r/w */

#define S_CPLD_SCARPINE_SIGNALS			0x17 	/* r/w */
#define S_CPLD_INTERRUPT_STATUS			0x18 	/* ro */

/* 0x19 - 0x1f */

#define S_CPLD_MI_CSR				0x20
#define S_CPLD_MI_MCTL				0x21
#define S_CPLD_MI_DATA_0			0x22
#define S_CPLD_MI_DATA_1			0x23
#define S_CPLD_MI_DEVAD				0x24
#define S_CPLD_MI_PRTAD				0x25

/* 0x26 - 0x3F reserved */


/* Functions prototypes */

uint8_t cpld_rd(uint8_t offset);
void cpld_wr(uint8_t offset,uint8_t val);
int cpld_get_version (uint8_t *version);
uint8_t cpld_is_fan_interrupt(void);
void cpld_clr_fan_interrupt(void);
uint8_t cpld_is_pwrs_interrupt(void);
void cpld_clr_pwrs_interrupt(void);
void cpld_clr_crdprs_interrupt(void);
uint8_t cpld_is_pwrbut_interrupt(void);
void cpld_clr_pwrbut_interrupt(void);
uint8_t cpld_is_interrupt(int interrupt);
void cpld_clr_interrupt(int interrupt);
uint8_t cpld_is_enable_interrupt( int interrupt);
void cpld_enable_interrupt(uint8_t interrupt, enable_mode_t mode);
int cpld_ps_status(uint8_t supply_number, status_type_t status_type);
void cpld_ps_control_set( uint8_t supply_number, ps_ctl_state_t state);
uint8_t cpld_is_card_present(uint8_t card_number);
void cpld_serial_select ( uint8_t card_number);
uint8_t cpld_which_serial_selected(void);
void cpld_device_reset(device_type_t device);


#endif /* msc_cpld_h */
