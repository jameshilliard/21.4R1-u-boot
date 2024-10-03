/*
 * Copyright (c) 2009-2011, Juniper Networks, Inc.
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
 
/* This file has all the functions needed for the swizzle logic. All the
 * status checks and the EEPROM reads are performed from here.
 */

#include<common.h>
#include "fx_cpld.h"
#include "fx_common.h"
#include "platform_defs.h"
#include <soho/tor_cpld_i2c.h>

uint8_t eeprom_env_valid;

/* 
 * g_boot_status is used to pass the boot.status information to the loader
 */
uint8_t g_boot_status[10];


/*Forward Declarations*/
void check_eeprom_env_valid (void);
uint32_t get_boot_status (void);
uint8_t get_upgrade_status (void);
uint8_t get_upgraded_partition (void);
uint8_t get_last_booted_partition (void);
void validate_upgrade (void);
uint8_t get_num_boot_tries (uint8_t partition);
uint8_t get_num_upgrade_tries (uint8_t partition);
uint8_t get_next_partition (uint8_t partition);
void set_upgrade_fail (void);
void set_last_boot_partition (uint8_t partition);
void set_boot_status (uint32_t status);
void set_num_boot_tries (uint8_t partition);
void handle_wd_timer_reset (void);
/*End Forward Declarations */


/* Function : check_swizzle
 * Arguments: None
 * Return   : None
 * Description: This is called after the i2c init is done in the
 *              dram SPD. This function initializes the needed
 *              controllers to access the cpld. Based on the
 *              reset type read from the cpld and also the values read
 *              from the EEPROM, a decision to continue booting or
 *              to reset is made. If it is in the middle of an upgrade,
 *              check to see if enough tries were made to determine
 *              that the upgraded partition is bad and declare the
 *              upgrade as bad and continue with the upgrading partition.
 * Notes  : We are relying on the CPLD to boot the correct partition in
 *          other cases of resets. Hence, most of the checks are not done.
 *          Only in the case of COLD BOOT, CPLD loses all the information
 *          and hence relying on the EEPROM.
 */

void check_swizzle (void)
{
    uint8_t reset_type = 0;
    uint8_t curr_partition, last_partition, upgrade_status = 0;

    fx_get_board_type();
    debug("\nCheck_Swizzle!!\n");
    fx_cpld_early_init();
    check_eeprom_env_valid();
    /*
     * Reset should have disabled the watchdog.
     * But in case the CPLD reset did not go through
     * and we reset through the normal MIPS command,
     * we need to explicitly disable the watchdog.
     */
    fx_cpld_enable_watchdog(0, 0);
    reset_type = fx_cpld_get_reset_type();
    last_partition = get_last_booted_partition();
    curr_partition = fx_cpld_get_current_partition();
    if (fx_cpld_get_full_flash_mode() != 0x0) {
        /*
         * This should always be 0 but in case it is
         * not, due to a CPLD issue, notify.
         * This has happened a few times.
         * A reset should fix this but if a CPLD
         * really has a permanent issue, we will
         * not be able to bring up the board further.
         */
        printf("\nFATAL: FFMODE not reset!!!!\n\n");
    }
    printf("\nPartition [%x,%x]\n", curr_partition, last_partition);
    
    switch (reset_type) {
    case COLD_BOOT_RESET:
        if (eeprom_env_valid) {
            if (last_partition != curr_partition) {
                fx_cpld_set_next_boot_partition(last_partition);
                debug("\nLast partition != curr partition...");
                fx_cpld_issue_sw_reset();
                printf("\nFATAL: CPLD Not issuing S/W reset!!\n");
                fx_cpld_dump();
            }
        }
        break;

    case PUSH_BUTTON_RESET:
    case SOFTWARE_RESET:
        break;

    case WATCHDOG_TIMER_RESET:
        /* This is the case where no upgrade is done yet there is a
         * watchdog reset. Here we are not sure if the jloader is 
         * corrupted for some reason or if something happened in the kernel
         * that caused the watchdog to kick in. Should try a few times before
         * giving up the last booted partition. The kernel has to clear the
         * num tries for the current partition once the kernel successfully
         * comes up.
         */ 
        upgrade_status = get_upgrade_status();
        if (!upgrade_status) {
            handle_wd_timer_reset();  
        }
        break;

    case CPLD_RESET:
        break;

    default:
        printf("\nFATAL ERROR: Unknown reset type " 
                      "detected: 0x%x !!!!!\n", reset_type);
        break;
    }
    if (eeprom_env_valid) {
        upgrade_status = get_upgrade_status();
        if (upgrade_status)  {
            debug("\nUpgrade was in progress.. validating...\n"); 
            validate_upgrade();
        }
    }
}

/* Function: check_eeprom_env_valid
 * Arguments: None
 * Return: None
 * Description: At reset, this function checks if the EEPROM
 *              was used earlier and if it can be trusted. Once valid
 *              information is written into the EEPROM, a signature is
 *              written to offset 0x00 which is used to verify the validity.
 *              If the EEPROM is valid, a global value eeprom_env_valid is set
 *              so that rest of the code can use EEPROM. The valid key is
 *              written when the first available data is written to the EEPROM.
 * Notes: Should see if we have to fill the entire range of values
 *        used in EEPROM to 0xff if detected as not valid or fill before writing
 *        the first usable data.
 */
 
extern uint32_t tor_cpld_i2c_debug_flag;
void check_eeprom_env_valid (void)
{
    uint8_t buf, i;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    tor_cpld_i2c_debug_flag = 0;
    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
               EEPROM_VALID_KEY_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
               &buf, 1);

    if (buf == EEPROM_VALID_KEY) {
        debug("\nEEPROM is valid!!");
        eeprom_env_valid = 1;
    } else {
        buf = 0xff;
        debug("\nEEPROM is Invalid.. Filling 0xff!!\n");

        for (i = 0; i <= MAX_NUM_BOOTING_OFFSETS; i++) {

            i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                       i, LOG_EEPROM_DEV_WORD_LEN, &buf, 1);
        }    
    }
    tor_cpld_i2c_debug_flag = 0;
}

void set_valid_key (void)
{
    uint8_t buf = EEPROM_VALID_KEY;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (!eeprom_env_valid) {
        debug("Setting EEPROM Valid Key!!\n");

        i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                   EEPROM_VALID_KEY_OFFSET, LOG_EEPROM_DEV_WORD_LEN, 
                   &buf, 1);

        eeprom_env_valid = 1;
    }        
}

/* The following functions are get and set functions for various data
 * maintained in the EEPROM. The caller has to ensure that the EEPROM
 * is valid before calling. The read values are not checked for validity
 */

uint32_t get_boot_status (void)
{
    uint32_t buf = 0xff;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
               SYSTEM_BOOT_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
               &buf, 4);

    debug("\nBOOT STATUS from EEPROM: 0x%x\n", buf);
    return (buf);
}

uint32_t get_last_boot_status (void)
{
    uint32_t buf = 0xff;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
               LAST_BOOT_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN, &buf, 4);

    debug("\nLAST BOOT STATUS from EEPROM: 0x%x\n", buf);
    return buf;
}

uint8_t get_upgrade_status (void)
{
    uint8_t buf = get_upgraded_partition();
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if ((buf == 1) || (buf == 0)) {

        i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                   UPGRADE_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN, &buf, 1);
        
if (buf == 0xff) {
            debug("\nUpgrade status: 0x%x !!", buf);
            return 0; 
        }

        if (buf > 0x4) {
            debug("\nUpgrade was a fail...continuing with " 
                          "current partition");
            return (0);
        } else {
            debug("\nUpgrade status: 0x%x !!", buf);
            return (buf);
        }

    } else {
        debug("No Upgrade in Progress!!\n");
        return (0);
    }
}

uint8_t get_upgraded_partition (void)
{
    uint8_t buf = 0xff;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR, 
               UPGRADED_PARTITION_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
               &buf, 1);

    debug("\nLast Upgraded partition: 0x%x\n", buf);
    return (buf);
}

uint8_t get_last_booted_partition (void)
{
    uint8_t buf = 0xff;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
               LAST_BOOTED_PARTITION_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
               &buf, 1);

    debug("\nLAST BOOTED PARTITION: 0x%x\n", buf);
    return (buf);
}


/* Function: validate_upgrade
 * Arguments: None
 * Return   : None
 * Description: The control reaches here only if in
 * the middle of an upgrade. If it is a watch dog timer reset,
 * it is concluded that the upgraded u-boot image is good but
 * either the loader or the kernel has a problem. In that case,
 * after a threshold number of tries, the upgrade is declared bad
 * and boots from the upgrading partition. If the reset is not a
 * watchdog timer reset, the u-boot itself may be bad and it is tried
 * for a thresold number of times before giving up.
 */

void validate_upgrade (void)
{
    uint8_t curr_partition = fx_cpld_get_current_partition();
    uint8_t next_partition = get_next_partition(curr_partition);

    debug("Validating Upgrade!!!\n");
    if (fx_cpld_get_reset_type() == WATCHDOG_TIMER_RESET) {
        if (get_boot_status() < BS_LOADER_BOOTING_KERNEL) {
            if (get_num_boot_tries(next_partition) >=
                                   NUM_TRIES_THRESHOLD) {
                debug("\nSetting UPG FAIL for other partition!!\n");
                set_upgrade_fail();
            } else {
                debug("\nThreshold not reached... reset to next partition!\n");
                set_num_boot_tries(next_partition);
                fx_cpld_set_next_boot_partition(next_partition);
                fx_cpld_issue_sw_reset();
            }
        } else {
            if (get_num_boot_tries(curr_partition) >=
                                   NUM_TRIES_THRESHOLD) {
                debug("\nCURR PARTITION UPG FAIL...booting next partition");
                set_upgrade_fail();
                fx_cpld_set_next_boot_partition(next_partition);
                fx_cpld_issue_sw_reset();
            } else {
                debug("\nTrying Current partition again!\n");
                set_num_boot_tries(curr_partition);
            }
        }
    } else {
        if (curr_partition != get_upgraded_partition()) {
            if (get_num_boot_tries(next_partition) >=
                                   NUM_TRIES_THRESHOLD) {
                debug("\nSetting UPG FAIL for next partition!\n");
                set_upgrade_fail();
            } else {
                debug("\nNum tries < threshold! Booting to next partition\n");
                set_num_boot_tries(next_partition);
                fx_cpld_set_next_boot_partition(next_partition);
                fx_cpld_issue_sw_reset();
            }
        }
    }
}

/* FUNCTION: handle_wd_timer_reset
 * Arguments: None
 * Return: None
 * Description: This function handles the watchdog timer reset when 
 *              there is no upgrade in progress. The following are the
 *              reasons for why a watchdog reset can occur:
 *              
 *              1. Jloader is corrupted for some reason.
 *              2. Kernel is unable to boot.
 *              3. VCCPD cannot start and there is no access to outside
 *                 network.
 *              4. kernel hanged and the watchdog is not turned off.
 *              
 *              Since, we clear off the number of tries when the kernel and
 *              the VCCPD are completely up, we try to boot from the last 
 *              successfully booted partition for a few times before we 
 *              give up and go to the next partition. 
 *              If for some reason the kernel is hanging after being up for
 *              sometime, we may not capture it here. The upper layer needs
 *              to keep track of that. 
 *
 * Notes: If the jloader is fine and the kernel 
 *        is unable to come up completely, decision needs to be taken 
 *        whether to use the next flash partition or use the older version
 *        of kernel.
 */

void handle_wd_timer_reset (void)
{
    uint8_t curr_partition = fx_cpld_get_current_partition();
    uint8_t next_partition = get_next_partition(curr_partition);

    debug("Handling WD Reset!!\n");
    if (get_boot_status() < BS_LOADER_WAITING_FOR_WATCHDOG) {
        if (get_num_boot_tries(next_partition) >=
                               NUM_TRIES_THRESHOLD) {
            debug("\nToo many Boot failures for other partition!");
        } else {
            set_num_boot_tries(next_partition);
            fx_cpld_set_next_boot_partition(next_partition);
            debug("Issuing S/W reset as NUM Tries less than threshold!!\n");
            fx_cpld_issue_sw_reset();
        }
    } else {
        if (get_num_boot_tries(curr_partition) >=
                               NUM_TRIES_THRESHOLD) {
            /*
             * FIXME: Here the decision needs to be taken to boot from next
             * Flash partition or roll back the kernel to older version!!!
             */ 
            debug("\nToo many kernel boot failures for current partition!");
        } else {
            set_num_boot_tries(curr_partition);
        }
    }           
}


uint8_t get_num_boot_tries (uint8_t partition)
{
    uint8_t buf = 0x0;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (partition == 0) {
        i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                   BOOT_NUM_TRIES_PART0_OFFSET, LOG_EEPROM_DEV_WORD_LEN, 
                   &buf, 1);
    } else {
        i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                   BOOT_NUM_TRIES_PART1_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                   &buf, 1);
    }

    if (buf == 0xff) {
        buf = 0;
    }

    debug("NUM BOOT TRIES for Partition: %d is 0x%x\n", 
                  partition, buf);
    return (buf);
}


uint8_t get_num_upgrade_tries (uint8_t partition)
{
    uint8_t buf = 0x0;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (partition == 0) {
        i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                   UPG_NUM_TRIES_PART0_OFFSET, LOG_EEPROM_DEV_WORD_LEN, &buf, 1);
    } else {
        i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                   UPG_NUM_TRIES_PART1_OFFSET, LOG_EEPROM_DEV_WORD_LEN, &buf, 1);
    }

    if (buf == 0xff) {
        buf = 0;
    }

    return (buf);
}

uint8_t get_next_partition (uint8_t partition)
{
    if (partition == 0) {
        return (1);
    }
    return (0);
}

/* Function: set_upgrade_fail
 * Arguments: None
 * Return: None
 * Description: This function sets the upgraded partition as failed when
 *              it is tried for a specific number of times unsuccessfully. 
 * Notes: Only set the failure when the upgrade has started. If in any case
 *        control reaches here and an upgrade is either not started or is 
 *        successfully completed, do not set it as a failure. The upper 
 *        layers starting the upgrade (kernel) should be able to handle 
 *        that condition.
 */

void set_upgrade_fail (void)
{
    uint8_t buf = get_upgrade_status();
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (buf == UPGRADE_START) {
        debug("\nSetting Upgrade ONE Fail!!\n");
        buf = UPGRADE_ONE_FAIL;
        i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                   UPGRADE_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN, &buf, 1);
    } else if (buf == UPGRADE_TWO_START) {
        debug("\nSetting Upgrade TWO Fail!!\n");
        buf = UPGRADE_TWO_FAIL;
        i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                   UPGRADE_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN, &buf, 1);
    }
}

void set_last_boot_partition (uint8_t partition)
{
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }
    debug("\nSetting Last BOOT Partition to: 0x%x\n", partition);
    i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
               LAST_BOOTED_PARTITION_OFFSET, LOG_EEPROM_DEV_WORD_LEN, 
               &partition, 1);
}

void set_boot_status (uint32_t status)
{
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }
    debug("\nSetting Boot Status: 0x%x\n", status);
    i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
               SYSTEM_BOOT_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN, 
               &status, 4);
}

/* 
 * set_last_boot_status function saves the last system boot status
 * in case we need to know at what point during the bootup, the system
 * was reset. Right now the value is not used but it is saved in case
 * we need to know the actual failure point
 */ 
void set_last_boot_status (void)
{
    uint32_t buf = 0xff;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    buf = get_boot_status();
    i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
               LAST_BOOT_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN, &buf, 4);
}    

void set_num_boot_tries (uint8_t partition)
{
    uint8_t buf;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus= TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (partition) {

        i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                   BOOT_NUM_TRIES_PART1_OFFSET, LOG_EEPROM_DEV_WORD_LEN, 
                   &buf, 1);

        if (buf == 0xff) {
            buf = 1;
        } else {
            buf++;
        }
        debug("Setting Num Boot Tries for partition 1 to 0x%x\n", buf);
        i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                   BOOT_NUM_TRIES_PART1_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                   &buf, 1);
    } else {

        i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                   BOOT_NUM_TRIES_PART0_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                   &buf, 1);

        if (buf == 0xff) {
            buf = 1;
        } else {
            buf++;
        }
        debug("Setting Num Boot Tries for partition 0 to 0x%x\n", buf);
        i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                   BOOT_NUM_TRIES_PART0_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                   &buf, 1);
    }
}

void set_num_kernel_boot_tries (uint8_t slice, uint8_t value)
{
    uint32_t offset;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus= TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }


    if (slice == 1) {
        offset = KERNEL_SLICE1_NUM_TRIES_OFFSET;
    } else {
        offset = KERNEL_SLICE2_NUM_TRIES_OFFSET;
    }
    debug("\nSetting NUM kernel boot tries of slice: 0x%x to 0x%x\n",
          slice, value);

    i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
               offset, LOG_EEPROM_DEV_WORD_LEN,
               &value, 1);
}

uint8_t get_num_kernel_boot_tries (uint8_t slice)
{
    uint32_t offset;
    uint8_t  buf;
    uint32_t bus= LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (slice == 1) {
        offset = KERNEL_SLICE1_NUM_TRIES_OFFSET;
    } else {
        offset = KERNEL_SLICE2_NUM_TRIES_OFFSET;
    }

    i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
               offset, LOG_EEPROM_DEV_WORD_LEN,
               &buf, 1);

    if (buf == 0xff) {
        return 0;
    }
    return (buf);
}

/* The EEPROM is accessed only for the boot_status here!!
 */ 
void eeprom_do_setenv (char *value)
{
    uint32_t status;

    status = (uint32_t) simple_strtoul(value, NULL, 10);
    /*
     * Here we care only about the status flags. We are masking off the
     * request flags and we always assume that no request can be set
     * for the FX platforms
     */
    status &= 0xffff;
    debug("\neeprom_do_setenv: The status to set is: 0x%x\n", status);
    set_boot_status(status);
}

char *eeprom_do_getenv (void)
{
    uint32_t status;

    status = get_boot_status();
    status &= 0x0000ffff;
    sprintf(g_boot_status, "0x%X", status);
    return((char *) g_boot_status);
}
