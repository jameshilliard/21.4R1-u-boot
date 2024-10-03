/*
 * Copyright (c) 2008-2012, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "rps.h"
#include <exports.h>
#include "rps_command.h"
#include "rps_etq.h"
#include "rps_cpld.h"
#include "rps_led.h"
#include "rps_psMon.h"
#include "rps_priority.h"

//#define DEBUG

static int32_t gPSTid = 0;
static uint8_t ps_working = 0;
static uint16_t psid[CPLD_MAX_PS] = { 0, 0, 0 };
static uint16_t psStatus = 0;
static uint16_t ps_disable[CPLD_MAX_PS] = { 0, 0, 0 };
char model_no[CPLD_MAX_PS][PS_MODEL_SIZE];

void 
ps_display (void);

uint8_t 
getPSNumber (void)
{
    int i;
    uint8_t num = 0;
    
    for (i = 0; i < CPLD_MAX_PS; i++) {
        if ((ps_working & (0x3 << i*2)) == (0x3 << i*2))
            num++;
    }

    return (num);
}

uint16_t 
getPSStatus (void)
{
    return (psStatus);
}

void 
setPSEnable (int ps, int ena)
{
    uint8_t temp, mask;
    uint8_t ch = 1 << 6;
    uint8_t data[2];

     if (ena) {
        /* check manual disable */
        if (ps_disable[ps])
            return;
    }

    if (ps == PS_1)
        mask = PS_1_MASK;
    else if (ps == PS_3)
        mask = PS_3_MASK;
    else {
        if (ena) {
            /* set AD5337 for U73 */
            i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
            data[0] = 0x2C;
            data[1] = 0xD0;
            i2c_write(0, DAC_0_ADDR, 1, 1, data, 2);
            i2c_write(0, DAC_0_ADDR, 2, 1, data, 2);
            /* set AD5337 for U74 for PoE */
            data[0] = 0x22;  /* PoE */
            data[1] = 0xF0;  /* PoE */
            i2c_write(0, DAC_1_ADDR, 1, 1, data, 2);
            data[0] = 0x21;  /* PoE */
            data[1] = 0x20;  /* PoE */
            i2c_write(0, DAC_1_ADDR, 2, 1, data, 2);
            ch = 0;
            i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
        }
        return;
    }

    if (cpld_read_direct(MAX_ACTIVE, &temp, 1))
        return;

    if (ena) {
        /* enable same type with PS_2 only */
        if (psid[ps] != psid[PS_2])
            return;
        temp |= mask;
    }
    else
        temp &= ~mask;

    cpld_reg_write(MAX_ACTIVE, temp);
}

void 
initPSMon (void)
{
    int i;
    
    gPSTid = 0;
    ps_working = 0;
    psStatus = 0;

    for (i = 0; i < CPLD_MAX_PS; i++) {
        psid[i] = 0;
        ps_disable[i] = 0;
    }
 }

/*
 * Check if inserted power supply is supported
 */ 
int8_t 
validate_psu (uint16_t id, char *model_no)
{
    if (id == I2C_ID_EX32X42X_PWR_930) {
        if ((strcmp(model_no, "EX-PWR-930-AC") == 0) ||
            (strcmp(model_no, "EX-PWR2-930-AC") == 0)) {
            return -1;
        }   
        else {
            return 0;
        }     
    } else {
        return -1;
    }    
}    

void 
psUpdate (uint8_t ps, uint8_t fan)
{
    uint8_t ch, ch_eeprom = 1<<PS_EEPROM_CHANNEL;
    uint16_t id;
    int i;
    static uint8_t fan_prev = 0;
    static uint8_t no_arm = 0;
    uint8_t psu_state[CPLD_MAX_PS] = { 0, 0, 0 };
    uint8_t pwr_good;

    psStatus = 0;
    for (i = 0; i < CPLD_MAX_PS; i++) {
        psStatus |= (ps & (0x3 << i*2)) << i; /* PS present + power good */
        /*
         * If fan fail, then set appropriate bits in psStatus 
         */ 
        if (fan & (0x1 << i))
            psStatus |= (1 << (i*3+2)); /* fan fail */
    }

    if ((ps_working != ps) || (fan_prev != fan)) {
        /* fan failure LED */
        if (((ps & PS1_PRESENT) && (fan & PS1_FAN_FAILURE)) ||
            ((ps & PS2_PRESENT) && (fan & PS2_FAN_FAILURE)) ||
            ((ps & PS3_PRESENT) && (fan & PS3_FAN_FAILURE)))
            set_fanfail_led(LED_AMBER);
        else
            set_fanfail_led(LED_GREEN);
    
        for (i = 0; i < CPLD_MAX_PS; i++) {
            if ((ps & (1<<(i*2))) > (ps_working & (1<<(i*2)))) {
                
                /* ps insert */
                i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch_eeprom, 1);
                ch = 1<<i;
                i2c_write(0, PS_EEPROM_MUX, 0, 0, &ch, 1);
                if (i2c_read(0, PS_I2C_ADDR, PS_ID_OFFSET, 1, 
                    (uint8_t *)&id, PS_ID_SIZE) == 0) {
                    psid[i] = swap_ushort(id);
                }
                i2c_read(0, PS_I2C_ADDR, PS_MODEL_OFFSET, 1, 
                    (uint8_t *)model_no[i], PS_MODEL_SIZE); 
                ch = 0;
                i2c_write(0, PS_EEPROM_MUX, 0, 0, &ch, 1);
                i2c_write(0, RPS_MAIN_MUX, 0, 0, &ch, 1);
                if (psid[i])
                    setPSEnable (i, 1);
            }
            else if ((ps & (1<<(i*2))) < (ps_working & (1<<(i*2)))) {
                /* not for PS_2 */
                if (i == PS_2)
                    continue;
                
                /* ps remove */
                if (ps & (1 << (i * 2 + 1)))
                setPSEnable (i, 0);
                psid[i] = 0;
            }
#if defined(DEBUG)
    printf("PS%d  id = %04x\n", i, psid[i]);
#endif
        }
            
        ps_working = ps;
        fan_prev = fan;
    }
   
    /*
     * For un-supported PSU models, and if not PSU1, 
     * disable the PSU
     */ 
    for (i = 0; i < CPLD_MAX_PS; i++) {
        pwr_good = (psStatus & (1 << (i * 3) + 1)) ? 1 : 0;
        if (psid[i]) {
            /*
             * PSU present
             */ 
            if (validate_psu(psid[i], model_no[i])) {
                /* 
                 * Invalid PSU
                 */ 
                psStatus &= ~(1 << ((i * 3) + 1));
                psu_state[i] = 0;
                if ( i != 1) {
                    setPSEnable (i, 0);
                }   
            } else if (!(pwr_good)) {
                /* 
                 * PSU present & valid, but power is removed
                 */ 
                psu_state[i] = 0;
            } else {   
                /*
                 * Valid PSU, with power present
                 */ 
                psu_state[i] = 1;
            }       
        } else {     
            psu_state[i] = 0;
        }
    }
    for(i = 0; i < CPLD_MAX_PS; i++) {
        if (psu_state[i])
            break;
    }
    if (i == CPLD_MAX_PS) {
        /*
         * No valid PSUs
         */
        set_rps_led(LED_AMBER);
        /* 
         * Set all ports to OFF state
         */
        for (i = 0; i < CPLD_MAX_PORT; i++) {
            set_port_led(i, LED_OFF);
            setPriority(i, 0);
        } 
        /*
         * Flag to indicate that RPS will not arm because there are 
         * no valid psus available 
         */ 
        no_arm = 1;
    }  else {
        if (no_arm) {
            /* 
             * Valid PSU present, so arming is enabled
             */ 
            set_rps_led(LED_GREEN);
            no_arm = 0;
        }     
    }    
}

void 
psMonitor (ptData ptdata)
{
    uint8_t data[2];
    
    if (cpld_read_direct(PS_WORKING, data, 2) == 0) {
        /* fan failure LED */
        psUpdate(data[0], data[1]);
    }
}

void 
startPSMon (int32_t start_time, int32_t cycle_time)
{
    tData tdata;
    int32_t tid;
    int32_t start = start_time / MS_PER_TICK;
    int32_t ticks = cycle_time / MS_PER_TICK;

    tdata.param[0] = tdata.param[1] = tdata.param[2] = 0;
    if  (addTimer(tdata, &tid, PERIDICALLY, start, ticks, psMonitor) == 0)
    {
        gPSTid = tid;
    }
}

void 
stopPSMon (void)
{
    delTimer(gPSTid);
    gPSTid = 0;
}

void 
psPower (uint16_t *walts_12V, uint16_t *walts_48V)
{
    int i;
    uint16_t temp_12v=0, temp_48v=0;
    
    for (i = 0; i < CPLD_MAX_PS; i++) {
        if (psid[i] == I2C_ID_EX32X42X_PWR_930) {
            temp_12v += 190;
            temp_48v += (930-190);
        }
    }
    *walts_12V = temp_12v;
    *walts_48V = temp_48v;
}

uint16_t 
getPSID (int32_t slot)
{
    if ((slot < CPLD_MAX_PS) && (slot >= 0))
        return (psid[slot]);
    return (0);
}

void 
ps_display (void)
{
    uint8_t data[3];
    int i;
    
    if (cpld_read(PS_WORKING, data, 3)) {
        return;
    }

    /* PS_2 always enabled */
    data[2] &= (PS_1_MASK | PS_3_MASK);
    data[2] |= ((data[2] & PS_1_MASK) >> 1);
    data[2] |= 0x40;
    
    printf("\n");
#if defined(DEBUG)
    printf("PS working  = 0x%x, PS failure = 0x%x\n", data[0], data[1]);
#endif
    for (i = 0; i < CPLD_MAX_PS; i++) {
        printf("PS%d[%04x] -- %-13s%-10s%-16s%-16s\n", i, psid[i],
            ((data[0] & (1<<(i*2))) ? "present" : "not present"),
            ((data[2] & (1<<(5+i))) ? "enabled" : "disabled"),
            ((data[0] & (1<<(i*2))) ? ((data[0] & (1<<(i*2+1))) ? 
            "power good" : "power not good") : "--"),
            ((data[0] & (1<<(i*2))) ? 
            ((data[1] & (1<<i)) ? "fan failure" : "fan working") : "--"));
    }
}

/* 
 *
 * envmon commands
 * 
 */
int
do_psmon (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong start=0, cycle=0;
    char *cmd = "status";  /* default command */
    char *cmd_1;
    
    if (argc <= 1)
        goto usage;
    
    if (argc >= 2) cmd = argv[1];
    if (argc == 4) {
        start = simple_strtoul(argv[2], NULL, 10) / MS_PER_TICK;
        cycle = simple_strtoul(argv[3], NULL, 10) / MS_PER_TICK;
    }

    if (!strncmp(cmd,"start", 4)) { startPSMon(start, cycle); }
    else if (!strncmp(cmd,"stop", 3)) { stopPSMon(); }
    else if (!strncmp(cmd,"status", 4)) { ps_display(); }
    else if (!strncmp(cmd,"enable", 3)) {
        if (argc >= 3) 
            cmd_1 = argv[2];
        else
            goto usage;
        if ((!strncmp(cmd_1,"PS0", 3)) || (!strncmp(cmd_1,"ps0", 3))) {
            ps_disable[PS_1] = 0;
            setPSEnable(PS_1, 1);
        }
        else if ((!strncmp(cmd_1,"PS2", 3)) || (!strncmp(cmd_1,"ps2", 3))) {
            ps_disable[PS_3] = 0;
            setPSEnable(PS_3, 1);
        }
    }
    else if (!strncmp(cmd,"disable", 3)) {
        if (argc >= 3) 
            cmd_1 = argv[2];
        else
            goto usage;
        if ((!strncmp(cmd_1,"PS0", 3)) || (!strncmp(cmd_1,"ps0", 3))) {
            setPSEnable(PS_1, 0);
            ps_disable[PS_1] = 1;
        }
        else if ((!strncmp(cmd_1,"PS2", 3)) || (!strncmp(cmd_1,"ps2", 3))) {
            setPSEnable(PS_3, 0);
            ps_disable[PS_3] = 1;
        }
    }
    else goto usage;
    return (1);

 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    psmon,    4,  1,  do_psmon,
    "psmon   - power supply monitor commands\n",
    "\n"
    "psmon start <start time> <cycle time> \n"
    "    - start ps monitor cycle time in ms\n"
    "psmon stop \n"
    "    - stop ps monitor\n"
    "psmon status\n"
    "    - power supply status\n"
    "psmon [enable | disable] [PS0 | PS2]\n"
    "    - manual overwrite PS0/PS2 enable/disable\n"
);

