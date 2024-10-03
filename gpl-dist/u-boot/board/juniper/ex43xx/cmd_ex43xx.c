/*
 * Copyright (c) 2011-2013, Juniper Networks, Inc.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#include <common.h>
#include <command.h>

#include <asm/processor.h>
#include <asm/immap_85xx.h>
#include <asm/fsl_i2c.h>
#include <asm/io.h>
#include <i2c.h>
#include <usb.h>
#include <miiphy.h>
#include <ns16550.h>
#include <part.h>
#include <pci.h>
#include <post.h>
#include <i2c.h>
#include <linux/ctype.h>
#include <tsec.h>
#include "ex43xx_local.h"
#include "../common/ex_common.h"
#include "cmd_ex43xx.h"
#include "hush.h"
#include "epld.h"
#include "epld_watchdog.h"
#include "led.h"
#include "lcd.h"

DECLARE_GLOBAL_DATA_PTR;

int i2c_manual = 0;
int i2c_mfgcfg = 0;

#if !defined(CONFIG_PRODUCTION)
int i2c_cksum = 0;
#endif /* CONFIG_PRODUCTION */

extern uint read_phy_reg_private(struct tsec_private *priv, uint regnum);
extern void write_phy_reg_private(struct tsec_private *priv, uint regnum, uint value);
extern char console_buffer[];

#if !defined(CONFIG_PRODUCTION)

/* i2c command structures */
typedef struct _hw_cmd_ {
    unsigned char  i2c_cmd;
    unsigned char  flags;
    unsigned short len;
} hw_cmd_t;

typedef struct _eeprom_cmd_ {
    hw_cmd_t         cmd;
    unsigned short   eeprom_addr;
    unsigned short   eeprom_len;
    unsigned char    buf[MAX_BUF_LEN];
} eeprom_cmd_t;

static int
secure_eeprom_write (unsigned i2c_addr, int addr, void *in_buf, int len )
{
    int            status = 0;
    eeprom_cmd_t   eeprom_cmd;
    unsigned short length;

    if (len > 128) {
	printf("secure eeprom size > 128 bytes.\n");
	return (-1);
    }

    if (len >= MAX_BUF_LEN) {
	length = MAX_BUF_LEN;
    } else {
	length = len;
    }
    eeprom_cmd.eeprom_len = length;
    eeprom_cmd.eeprom_addr = addr;

    eeprom_cmd.cmd.i2c_cmd = WRITE_EEPROM_DATA;
    eeprom_cmd.cmd.flags = 0;
    eeprom_cmd.cmd.len = sizeof(hw_cmd_t) + 4 + length;

    memcpy((void *)eeprom_cmd.buf, in_buf, length);

    status = i2c_write (i2c_addr, 0, 0,
	(uint8_t *) &eeprom_cmd, eeprom_cmd.cmd.len);
    if (status) {
	printf("secure eeprom write failed status = %d.\n", status);
	return (status);
    }

    udelay (50000);  /* ~500 bytes */

    return (status);
}
#endif /* CONFIG_PRODUCTION */


/*
 * On EX4300, flash is in a 8 bit mode,
 * also the flash does not support parallel
 * read/writes, hence if we are not relocated
 * and this function will simply return.
 */
int
flash_write_direct (ulong addr, char *src, ulong cnt)
{
    volatile uint8_t *faddr = (uint8_t *)addr;
    volatile uint8_t *faddrs = (uint8_t *)addr;
    volatile uint8_t *faddr1 = (uint8_t *)((addr & 0xFFFFF000) | 0xAAA);
    volatile uint8_t *faddr2 = (uint8_t *)((addr & 0xFFFFF000) | 0x555);
    uint8_t *data = (uint8_t *) src;
    unsigned long long timeval = 0;
    ulong saddr;
    ulong i;

    /* flash addr range is 0xFF800000 - 0xFFFFFFFF */
    if ((addr < 0xff800000) || (addr >= 0xffffffff)) {
	printf("flash address 0x%lx out of range 0xFF800000 - 0xFFFFFFFF\n",
		addr);
	return 1;
    }

    /*
     * if we are yet to relocate to RAM,
     * do nothing, simply return 0 so as
     * to make this attempt a dummy success
     *
     */
    if (!(gd->flags & GD_FLG_RELOC)) {
	return 0;
    }

    saddr = addr & 0xffff0000;
    faddrs = (uint8_t *)saddr;
    faddr1 = (uint8_t *)((saddr & 0xFFFFF000) | 0xAAA);
    faddr2 = (uint8_t *)((saddr & 0xFFFFF000) | 0x555);

    /* un-protect sector */
    *faddrs = FLASH_CMD_CLEAR_STATUS;
    *faddrs = FLASH_CMD_PROTECT;
    *faddrs = FLASH_CMD_PROTECT_CLEAR;
    timeval = get_ticks();
    while ((*faddrs & AMD_STATUS_TOGGLE) !=
	   (*faddrs & AMD_STATUS_TOGGLE)) {
	if (get_ticks() - timeval > usec2ticks(0x2000)) {
	    *faddrs = AMD_CMD_RESET;
	    return 1;
	}
	udelay(1);
    }

    /* erase sector */
    *faddr1 = AMD_CMD_UNLOCK_START;
    *faddr2 = AMD_CMD_UNLOCK_ACK;
    *faddr1 = AMD_CMD_ERASE_START;
    *faddr1 = AMD_CMD_UNLOCK_START;
    *faddr2 = AMD_CMD_UNLOCK_ACK;
    *faddrs = AMD_CMD_ERASE_SECTOR;
    timeval = get_ticks();
    while ((*faddrs & AMD_STATUS_TOGGLE) !=
	   (*faddrs & AMD_STATUS_TOGGLE)) {
	if (get_ticks() - timeval > usec2ticks(0x100000)) {
	    *faddrs = AMD_CMD_RESET;
	    return 1;
	}
	udelay(1);
    }

    for (i = 0; i < cnt ; i++) {
	/* write data byte */
	*faddr1 = AMD_CMD_UNLOCK_START;
	*faddr2 = AMD_CMD_UNLOCK_ACK;
	*faddr1 = AMD_CMD_WRITE;

	*(faddr+i) = data[i];
	timeval = get_ticks();
	while ((*faddrs & AMD_STATUS_TOGGLE) !=
	       (*faddrs & AMD_STATUS_TOGGLE)) {
	    if (get_ticks() - timeval > usec2ticks(0x2000)) {
		*faddrs = AMD_CMD_RESET;
		return 1;
	    }
	    udelay(1);
	}
    }

    /* protect sector */
    *faddrs = FLASH_CMD_CLEAR_STATUS;
    *faddrs = FLASH_CMD_PROTECT;
    *faddrs = FLASH_CMD_PROTECT_SET;
    timeval = get_ticks();
    while ((*faddrs & AMD_STATUS_TOGGLE) !=
	   (*faddrs & AMD_STATUS_TOGGLE)) {
	if (get_ticks() - timeval > usec2ticks(0x2000)) {
	    *faddrs = AMD_CMD_RESET;
	    return 1;
	}
	udelay(1);
    }
   return 0;
}

/* upgrade commands
 *
 * Syntax:
 *    upgrade get
 *    upgrade set <state>
 */
int
do_upgrade (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1;
    int state;
    volatile int *ustate;
    int saved_ustate = 0;

    if (argc < 2)
        goto usage;

    /* enable both banks access before accessing upgrade state */
    enable_both_banks(); 
    ustate = (int *) (CONFIG_SYS_FLASH_BASE +
        CFG_UPGRADE_BOOT_STATE_OFFSET);

    /* save the upgrade state flash location before 
     * disabling bank1 access.
     */
    saved_ustate = *ustate;

    /* disable access to bank1 once done with flash read. */
    enable_single_bank_access();

    cmd1 = argv[1][0];

    switch (cmd1) {
    case 'g':       /* get state */
    case 'G':
	printf("upgrade state = 0x%x\n", saved_ustate);
	break;

    case 's':                 /* set state */
    case 'S':
	state = simple_strtoul(argv[2], NULL, 16);
	if (state == saved_ustate)
	    return (1);

	/* We need to expose entire flash when we are in bank0.
	 * This is because the U-Boot env is stored in the
	 * upper 4Mb region (at location 0xff810000), and this is
	 * not accessible when we are in bank0. Note that when in
	 * bank0, software sees only lower 4Mb flash.
	 *
	 * We also need to disable the full flash access once
	 * we are done with writing the U-Boot env. This is required
	 * to keep the swizzle state machine sane.
	 *
	 *    +---------+ 0xff800000
	 *    |         |
	 *    |  bank1  |
	 *    |         |
	 *    +---------+ 0xffc00000
	 *    |         |
	 *    |  bank0  |
	 *    |         |
	 *    +---------+ 0xffffffff
	 *
	 */
	if (state == UPGRADE_OK_B1) {
	    /* enable access to both bank0 and bank1 */
	    enable_both_banks();
	    if (flash_write_direct(CONFIG_SYS_FLASH_BASE +
		  CFG_UPGRADE_BOOT_STATE_OFFSET,
		  (char *)&state, sizeof(int)) != 0) {
		printf("upgrade set state 0x%x failed.\n", state);
		return 1;  /* failure */
	    }
	    /* disable access to upper 4Mb flash */
	    enable_single_bank_access();
	} else {
	    if (flash_write_direct(CONFIG_SYS_FLASH_BASE +
		  CFG_UPGRADE_BOOT_STATE_OFFSET,
		  (char *)&state, sizeof(int)) != 0) {
		printf("upgrade set state 0x%x failed.\n", state);
		return 1;  /* failure */
	    }
	}
	return (0);  /* success */
	break;

    default:
	printf("???");
	break;
    }

    return (1);
usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

extern flash_info_t flash_info[];       /* info for FLASH chips */
int resetenv_cmd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    printf("Un-Protect ENV Sector\n");

    flash_protect(FLAG_PROTECT_CLEAR,
		  CONFIG_ENV_ADDR,CONFIG_ENV_ADDR + CONFIG_ENV_SECT_SIZE - 1,
		  &flash_info[0]);


    printf("Erase sector %d ... ",CFG_ENV_SECTOR);
    flash_erase(&flash_info[0], CFG_ENV_SECTOR, CFG_ENV_SECTOR);
    printf("done\nProtect ENV Sector\n");

    flash_protect(FLAG_PROTECT_SET,
		  CONFIG_ENV_ADDR,CONFIG_ENV_ADDR + CONFIG_ENV_SECT_SIZE - 1,
		  &flash_info[0]);

    printf("\nWarning: Default Environment Variables will take effect Only "
	   "after RESET \n\n");
    return 1;
}

int
i2c_mux (int ctlr, uint8_t mux, uint8_t chan, int ena)
{
    uint8_t ch;
    int ret = 0;

    i2c_set_bus_num(ctlr);
    if (ena) {
	ch = 1 << chan;
	ret = i2c_write(mux, 0, 0, &ch, 1);
    }
    else {
	ch = 0;
	ret = i2c_write(mux, 0, 0, &ch, 1);
    }

    return ret;
}

void
rtc_init (void)
{
    uint8_t temp[16];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P3_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 3, 1)) {
	printf("i2c failed to enable mux 0x%x channel 5.\n",
		CFG_I2C_C1_9548SW1);
    }
    else {
	if (i2c_read(i2cAddr, 0, 1, temp, 16)) {
	    printf("I2C read from device 0x%x failed.\n", i2cAddr);
	    return;
	}

	if (temp[2] & 0x80) {
	    temp[2] &= (~0x80);
	    i2c_write(i2cAddr, 2, 1, &temp[2], 1);
	}
	temp[0] = 0x20;
	temp[1] = 0;
	temp[9] = temp[10] = temp[11] = temp[12] = 0x80;
	temp[13] = 0;
	temp[14] = 0;
	i2c_write(i2cAddr, 0, 1, temp, 16);
	temp[0] = 0;
	i2c_write(i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

void
rtc_start (void)
{
    uint8_t temp[2];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P3_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 3, 1)) {
	printf("i2c failed to enable mux 0x%x channel 5.\n",
		CFG_I2C_C1_9548SW1);
    }
    else {
	temp[0] = 0;
	i2c_write(i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

void
rtc_stop (void)
{
    uint8_t temp[2];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P3_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 3, 1)) {
	printf("i2c failed to enable mux 0x%x channel 5.\n",
		CFG_I2C_C1_9548SW1);
    }
    else {
	temp[0] = 0x20;
	i2c_write(i2cAddr, 0, 1, temp, 1);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

void
rtc_set_time (int yy, int mm, int dd, int hh, int mi, int ss)
{
    uint8_t temp[9];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P3_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 3, 1)) {
	printf("i2c failed to enable mux 0x%x channel 5.\n",
		CFG_I2C_C1_9548SW1);
    }
    else {
	i2c_read(i2cAddr, 0, 1, temp, 9);
	temp[8] = ((yy / 10) << 4) + (yy % 10);
	temp[7] &= 0x80;
	temp[7] = ((mm / 10) << 4) + (mm % 10);
	temp[5] = ((dd / 10) << 4) + (dd % 10);
	temp[4] = ((hh / 10) << 4) + (hh % 10);
	temp[3] = ((mi / 10) << 4) + (mi % 10);
	temp[2] = ((ss / 10) << 4) + (ss % 10);
	temp[6] = 0; /* weekday? */
	i2c_write(i2cAddr, 0, 1, temp, 9);
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

#if !defined(CONFIG_PRODUCTION)
static void
rtc_reg_dump (void)
{
    uint8_t temp[20];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P3_RTC;
    int i;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 3, 1)) {
	printf("i2c failed to enable mux 0x%x channel 5.\n",
		CFG_I2C_C1_9548SW1);
    }
    else {
	if (i2c_read(i2cAddr, 0, 1, temp, 16)) {
	    printf("I2C read from device 0x%x failed.\n", i2cAddr);
	}
	else {
	    printf("RTC register dump:\n");
	    for (i = 0; i < 16; i++)
		printf("%02x ", temp[i]);
		printf("\n");
	}
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

static void
rtc_get_status (void)
{
    uint8_t temp[20];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P3_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 3, 1)) {
	printf("i2c failed to enable mux 0x%x channel 5.\n",
		CFG_I2C_C1_9548SW1);
    }
    else {
	if (i2c_read(i2cAddr, 0, 1, temp, 16)) {
	    printf("I2C read from device 0x%x failed.\n", i2cAddr);
	}
else {
	printf("RTC status:\n");
	printf("Reg 0x0 (0x%02x) STOP = %d\n",
		temp[0], (temp[0] & 0x20) ? 1 : 0);
	printf("Reg 0x1 (0x%02x) TI/TP = %d, AF = %d, TF = %d, "
	       "AIE = %d, TIE = %d\n",
		temp[1],
		(temp[1] & 0x10) ? 1 : 0,
		(temp[1] & 0x08) ? 1 : 0,
		(temp[1] & 0x04) ? 1 : 0,
		(temp[1] & 0x02) ? 1 : 0,
		(temp[1] & 0x01) ? 1 : 0
	     );
	printf("Reg 0x2 (0x%02x) VL = %d\n",
		temp[2],
		(temp[2] & 0x80) ? 1 : 0
	     );
	printf("Reg 0x7 (0x%02x) C = %d\n",
		temp[7],
		(temp[7] & 0x80) ? 1 : 0
	     );
	printf("Reg 0x9 (0x%02x) AE = %d\n",
		temp[9],
		(temp[9] & 0x80) ? 1 : 0
	      );
	printf("Reg 0xA (0x%02x) AE = %d\n",
		temp[0xA],
		(temp[0xA] & 0x80) ? 1 : 0
	      );
	printf("Reg 0xB (0x%02x) AE = %d\n",
		temp[0xB],
		(temp[0xB] & 0x80) ? 1 : 0
	      );
	printf("Reg 0xC (0x%02x) AE = %d\n",
		temp[0xC],
		(temp[0xC] & 0x80) ? 1 : 0
	      );
	printf("Reg 0xD (0x%02x) FE = %d, FD1 = %d, FD0 = %d\n",
		temp[0xD],
		(temp[0xD] & 0x80) ? 1 : 0,
		(temp[0xD] & 0x02) ? 1 : 0,
		(temp[0xD] & 0x01) ? 1 : 0
	      );
	printf("Reg 0xE (0x%02x) TE = %d, TD1 = %d, TD0 = %d\n",
		temp[0xE],
		(temp[0xE] & 0x80) ? 1 : 0,
		(temp[0xE] & 0x02) ? 1 : 0,
		(temp[0xE] & 0x01) ? 1 : 0
	      );

	}
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}

static void
rtc_get_time (void)
{
    uint8_t temp[9];
    uint8_t i2cAddr = CFG_I2C_C1_9548SW1_P3_RTC;

    if (i2c_mux(0, CFG_I2C_C1_9548SW1, 3, 1)) {
	printf("i2c failed to enable mux 0x%x channel 5.\n",
		CFG_I2C_C1_9548SW1);
    } else {
	if (i2c_read(i2cAddr, 0, 1, temp, 9)) {
	    printf("I2C read from device 0x%x failed.\n", i2cAddr);
	} else {
	    printf("RTC time:  ");
	    printf("%d%d/", (temp[8] & 0xf0) >> 4, temp[8]&0xf);
	    printf("%d%d/", (temp[7] & 0x10) >> 4, temp[7]&0xf);
	    printf("%d%d ", (temp[5] & 0x30) >> 4, temp[5]&0xf);
	    printf("%d%d:", (temp[4] & 0x30) >> 4, temp[4]&0xf);
	    printf("%d%d:", (temp[3] & 0x70) >> 4, temp[3]&0xf);
	    printf("%d%d\n", (temp[2] & 0x70) >> 4, temp[2]&0xf);
	}
    }
    i2c_mux(0, CFG_I2C_C1_9548SW1, 0, 0);  /* disable mux */
}


int
do_rtc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *data;
    char tbyte[3];
    int i, len, temp, time1[3], time2[3];
    char *cmd = "status";  /* default command */

    if (argc == 1) {
	if (!strncmp(argv[0], "rtc", 3)) {
	    rtc_get_time();
	}
    else
	goto usage;
    }
    else if (argc == 2) {
	cmd = argv[1];
	if (!strncmp(cmd, "reset", 3)) {
	    rtc_set_time(0, 0, 0, 0, 0, 0);
	} else if (!strncmp(cmd, "init", 4)) {
	    rtc_init();
	} else if (!strncmp(cmd, "start", 4)) {
	    rtc_start();
	} else if (!strncmp(cmd, "stop", 3)) {
	    rtc_stop();
	} else if (!strncmp(cmd, "status", 4)) {
	    rtc_get_status();
	} else if (!strncmp(cmd, "dump", 4)) {
	    rtc_reg_dump();
	}
	else
		goto usage;
    }
    else if (argc == 4) {
	cmd = argv[1];
	if (!strncmp(cmd, "set", 3)) {
	    data = argv [2];
	    len = strlen(data) / 2;
	    tbyte[2] = 0;
	    for (i = 0; i < len; i++) {
		tbyte[0] = data[2 * i];
		tbyte[1] = data[2 * i + 1];
		temp = atod(tbyte);
		time1[i] = temp;
	    }
	    data = argv [3];
	    len = strlen(data) / 2;
	    for (i = 0; i < len; i++) {
		tbyte[0] = data[2 * i];
		tbyte[1] = data[2 * i + 1];
		temp = atod(tbyte);
		time2[i] = temp;
	    }
	    rtc_set_time(time1[0], time1[1], time1[2], time2[0],
			 time2[1], time2[2]);
	}
	else
	    goto usage;
    }
    else
	goto usage;

    return (1);
usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return (1);
}


int
do_i2c_controller (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int controller = 1;  /* default I2C controller 1 */

    if (argc == 1) {
	printf("current i2c controller = %d\n", i2c_get_bus_num() + 1);
    } else if (argc == 2) {
	/* Set new base address. */
	controller = simple_strtoul(argv[1], NULL, 10);
	i2c_set_bus_num(controller - 1);
    } else {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }

    return (0);
}

int
do_i2c_diag (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int controller, rw, len,i,temp;
    unsigned int  slave_addr, reg, ret, noreg = 1;
    char *data;
    char tbyte[3];
    uint8_t tdata[128];

    /* check if all the arguments are passed */
    if (argc != 7) {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }
    /* cmd arguments */
    rw = simple_strtoul(argv[1], NULL, 10);
    controller = simple_strtoul(argv[2], NULL, 10);
    slave_addr = simple_strtoul(argv[3], NULL, 16);
    reg = simple_strtoul(argv[4], NULL, 16);
    data = argv [5];
    len = simple_strtoul(argv[6], NULL, 10);
    if (reg == 0xFF)
	noreg = 0;
    /* set the ctrl */
    i2c_set_bus_num(controller - 1);

    if (rw == 1)
    {
	ret = i2c_read(slave_addr, reg, noreg, tdata, len);
	if (ret == 0)
	    printf("The data read - \n");
	for (i = 0; i < len; i++)
	    printf("%02x ", tdata[i]);
	printf("\n");
    }
    else
    {
	tbyte[2] = 0;
	for (i = 0; i < len; i++) {
	    tbyte[0] = data[2 * i];
	    tbyte[1] = data[2 * i + 1];
	    temp = atoh(tbyte);
	    tdata[i] = temp;
	}
	ret = i2c_write(slave_addr, reg, noreg, tdata, len);
    }

    controller = 1; /* set back to default */
    i2c_set_bus_num(controller - 1);

    if (ret != 0)
    {
	printf("i2c r/w failed\n");
	return (1);
    }
    return (0);
}

int
do_i2c_diag_switch (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int controller, rw, len, temp, i;
    unsigned int sw_addr, channel, slave_addr, reg, ret, noreg = 1;
    uint16_t cksum = 0;
    unsigned char ch;
    char *data;
    char tbyte[3];
    uint8_t tdata[128];

    /* check if all the arguments are passed */
    if (argc != 9) {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }

    /* cmd arguments */
    rw = simple_strtoul(argv[1], NULL, 10);
    controller = simple_strtoul(argv[2], NULL, 10);
    sw_addr = simple_strtoul(argv[3], NULL, 16);
    channel = simple_strtoul(argv[4], NULL, 10);
    slave_addr = simple_strtoul(argv[5], NULL, 16);
    reg = simple_strtoul(argv[6], NULL, 16);
    data = argv [7];
    len = simple_strtoul(argv[8], NULL, 10);
    if (reg == 0xFF)
	noreg = 0;

    i2c_set_bus_num(controller - 1);

    /* enable the channel */
    ch = 1 << channel;
    i2c_write(sw_addr, 0, 0, &ch, 1);

    if (rw == 1)
    {
	ret = i2c_read(slave_addr, reg, noreg, tdata, len);
	if (ret == 0)
	    printf("The data read - \n");
	for (i = 0; i < len; i++)
	    printf("%02x ", tdata[i]);
	printf("\n");
    } else {
	tbyte[2] = 0;
	for (i = 0; i < len; i++) {
	    tbyte[0] = data[2 * i];
	    tbyte[1] = data[2 * i + 1];
	    temp = atoh(tbyte);
	    tdata[i] = temp;
	}
	if (i2c_cksum) {
	    for (i = 0; i < len; i++) {
		cksum += tdata[i];
	    }
	    tdata[len] = (cksum & 0xff00) >> 8;
	    tdata[len + 1] = cksum & 0xff;
	    len = len + 2;
	}
	ret = i2c_write(slave_addr, reg, noreg, tdata, len);
    }

    /* disable the channel */
    ch = 0x0;
    i2c_write(sw_addr, 0, 0, &ch, 1);

    controller = 1; /* set back to default */
    i2c_set_bus_num(controller - 1);
    if (ret != 0)
    {
	printf("i2c r/w failed\n");
	return (1);
    }
    return (0);
}

int
do_i2c_cksum (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int cksum;

    if (argc <= 1) {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }

    cksum = simple_strtoul(argv[1], NULL, 10);
    i2c_cksum = (cksum == 1);

    return (0);
}

/* FIXME - have stub function for now. */
static void
i2c_set_fan (int fan, int duty)
{
}

/* FIXME - have stub function for now. */
static uint8_t
tempRead (int32_t sensor)
{
    return 0;
}

/* FIXME - have stub function for now. */
static int
i2c_read_temp (uint32_t threshold)
{
    return 0;
}

/* FIXME - have stub function for now. */
static uint16_t
voltRead (uint8_t addr, int32_t sensor)
{
    return 0;
}

/* FIXME - have stub function for now. */
static void
i2c_read_volt (void)
{
}

/* I2C commands
 *
 * Syntax:
 */
int
do_i2c_ex43xx (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1, cmd2, cmd3, cmd4_0, cmd4_1, cmd4_2, cmd5, cmd6_0, cmd6_1;
    int led;
    LedColor color;
    LedState state;
    uint mux = 0, device = 0, channel = 0;
    unsigned char ch, bvalue;
    uint regnum = 0;
    int i = 0, ret, reg = 0, temp, len = 0, offset = 0, secure = 0, nbytes;
    char tbyte[3];
    uint8_t tdata[2048];
    char *data;
    uint16_t cksum = 0;
    int unit, rsta = 0;
    ulong value, mloop = 1, threshold = 255;

    if (argc < 2)
	goto usage;

    cmd1 = argv[1][0];
    cmd2 = argv[2][0];

    switch (cmd1) {
    case 'l':       /* LED */
    case 'L':
	unit = simple_strtoul(argv[2], NULL, 10);
	cmd3 = argv[3][0];
	switch (cmd3) {
	case 'u':    /* LED update */
	case 'U':
	    if (unit)
		update_led_back();
	    else
		update_led();
	    break;

	case 's':
	case 'S':    /* LED set */
	    if (argc <= 6)
		goto usage;

	    cmd4_0 = argv[4][0];
	    cmd4_1 = argv[4][1];
	    cmd4_2 = argv[4][2];
	    cmd5 = argv[5][0];
	    cmd6_0 = argv[6][0];
	    cmd6_1 = argv[6][1];
	    switch (cmd4_0) {
	    case 't':
	    case 'T':
		if (unit)
		    led = 2;
		else
		    led = 4;
		break;

	    case 'm':
	    case 'M':
		if (unit)
		    led = 1;
		else
		    led = 3;
		break;

	    case 'b':
	    case 'B':
		if (unit)
		    led = 0;
		else
		    led = 1;
		break;

	    default:
		goto usage;
		break;
	    }

	    if ((cmd5 == 'a') || (cmd5 == 'A'))
		color = LED_AMBER;
	    else if ((cmd5 == 'g') || (cmd5 == 'G'))
		color = LED_GREEN;
	    else if ((cmd5 == 'r') || (cmd5 == 'R'))
		color = LED_RED;
	    else if ((cmd5 == 'o') || (cmd5 == 'O'))
		color = LED_OFF;
	    else
		goto usage;

	    if ((cmd6_0 == 's') || (cmd6_0 == 'S')) {
		if ((cmd6_1 == 't') || (cmd6_1 == 'T'))
		    state = LED_STEADY;
		else if ((cmd6_1 == 'l') || (cmd6_1 == 'L'))
		    state = LED_SLOW_BLINKING;
		else
		    goto usage;
	    } else if ((cmd6_0 == 'f') || (cmd6_0 == 'F'))
		state = LED_FAST_BLINKING;
	    else
		goto usage;

	    if (unit)
		set_led_back(led, color, state);
	    else
		set_led(led, color, state);
	    break;
	default:
	    printf("???");
	    break;
	}

	break;

    case 'b':   /* byte read/write */
    case 'B':
	device = simple_strtoul(argv[2], NULL, 16);
	if ((argv[1][1] == 'r') || (argv[1][1] == 'R')) {
	    bvalue = 0xff;
	    if ((ret = i2c_read_noaddr(device, &bvalue, 1)) != 0) {
		printf("i2c failed to read from device 0x%x.\n", device);
		return (0);
	    }
	    printf("i2c read addr=0x%x value=0x%x\n", device, bvalue);
	} else {
	    if (argc <= 3)
		goto usage;
	    bvalue = simple_strtoul(argv[3], NULL, 16);
	    if ((ret = i2c_write_noaddr(device, &bvalue, 1)) != 0) {
		printf("i2c failed to write to address 0x%x value 0x%x\n",
		    device, bvalue);
		return (0);
	    }
	}
	break;

    case 'm':
    case 'M':
	if ((argv[1][1] == 'a') || (argv[1][1] == 'A')) {  /* manual */
	    if (argc < 3)
		goto usage;
	    if (!strncmp(argv[2], "on", 2))
		i2c_manual = 1;
	    else
		i2c_manual = 0;
	    break;
	}

	if ((argv[1][1] == 'u') || (argv[1][1] == 'U')) {  /* mux channel */
	    if (argc <= 3)
		goto usage;

	    mux = simple_strtoul(argv[3], NULL, 16);
	    channel = simple_strtoul(argv[4], NULL, 10);

	    if ((argv[2][0] == 'e') || (argv[2][0] == 'E')) {
		ch = 1 << channel;
		if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
		    printf("i2c failed to enable mux 0x%x channel %d\n",
			mux, channel);
		} else
		    printf("i2c enabled mux 0x%x channel %d\n",
			mux, channel);
	    }else {
		ch = 0;
		if ((ret = i2c_write(mux, 0, 0, &ch, 1)) != 0) {
		    printf("i2c failed to disable mux 0x%x channel %d.\n",
			mux, channel);
		} else
		    printf("i2c disabled mux 0x%x channel %d.\n",
			mux, channel);
	    }
	}
	break;

    case 'd':   /* Data */
    case 'D':
	if (argc <= 4)
	    goto usage;

	device = simple_strtoul(argv[2], NULL, 16);
	regnum = simple_strtoul(argv[3], NULL, 16);
	reg = 1;
	if (regnum == 0xff) {
	    reg = 0;
	    regnum = 0;
	}
	if ((argv[1][1] == 'r') || (argv[1][1] == 'R')) {  /* read */
	    len = simple_strtoul(argv[4], NULL, 10);
	    if (len) {
		if (reg == 0) {
		    if ((ret = i2c_read_noaddr(device, tdata, len)) != 0) {
			printf("I2C read from device 0x%x failed.\n",
			    device);
		    }
		} else {
		    if ((ret = i2c_read(device, regnum, reg,
			    tdata, len)) != 0) {
			printf("I2C read from device 0x%x failed.\n",
			    device);
		    }
		}
		if (ret == 0) {
		    printf("The data read - \n");
		    for (i = 0; i < len; i++)
			printf("%02x ", tdata[i]);
		    printf("\n");
		}
	    }
	} else {  /* write */
	    data = argv [4];
	    len = strlen(data) / 2;
	    tbyte[2] = 0;
	    for (i = 0; i < len; i++) {
		tbyte[0] = data[2 * i];
		tbyte[1] = data[2 * i + 1];
		temp = atoh(tbyte);
		tdata[i] = temp;
	    }
	    if ((argv[5][0] == 'c') || (argv[5][0] == 'C')) {
		for (i = 0; i < len; i++) {
		    cksum += tdata[i];
		}
		tdata[len] = (cksum & 0xff00) >> 8;
		tdata[len + 1] = cksum & 0xff;
		len = len + 2;
	    }
	    if (reg == 0) {
		if ((ret = i2c_write_noaddr(device, tdata, len)) != 0)
		    printf("I2C write to device 0x%x failed.\n", device);
	    } else {
		if ((ret = i2c_write(device, regnum, reg, tdata, len))!= 0)
		    printf("I2C write to device 0x%x failed.\n", device);
	    }
	}
	break;

    case 'e':   /* EEPROM */
    case 'E':
	if (argc <= 4)
	    goto usage;

	device = simple_strtoul(argv[2], NULL, 16);
	offset = simple_strtoul(argv[3], NULL, 10);
	if ((argv[1][1] == 'r') || (argv[1][1] == 'R')) {  /* read */
	    len = simple_strtoul(argv[4], NULL, 10);
	    if (len) {
		if ((argv[1][2] == 's') || (argv[1][2] == 'S')) {
		    if ((ret = secure_eeprom_read(device, offset, tdata,
			    len)) != 0) {
			printf("I2C read from secure EEPROM device 0x%x "
			    "failed.\n", device);
			return (-1);
		    }
		} else {
		    if ((ret = eeprom_read(device, offset, tdata,
			    len)) != 0) {
			printf("I2C read from EEPROM device 0x%x "
			    "failed.\n", device);
			return (-1);
		    }
		}
		printf("The data read from offset:%x - \n", offset);
		for (i = 0; i < len; i++)
		    printf("%02x ", tdata[i]);
		printf("\n");
	    }
	} else {  /* write -- offset auto increment */
	    if ((argv[1][2] == 'o') || (argv[1][2] == 'O')) {
		if ((argv[4][0] == 's') || (argv[4][0] == 'S'))
		    secure = 1;

		/* Print the address, followed by value.
		 * Then accept input for the next value.
		 * A non-converted value exits.
		 */
		do {
		    if (secure) {
			if ((ret = secure_eeprom_read(device, offset,
				tdata, 1)) != 0) {
			    printf("I2C read from EEPROM device "
				"0x%x at 0x%x failed.\n",
				device, offset);
			    return (1);
			}
		    } else {
			if ((ret = eeprom_read(device, offset,
				tdata, 1)) != 0) {
			    printf("I2C read from EEPROM device "
				"0x%x at 0x%x failed.\n",
				device, offset);
			    return (1);
			}
		    }
		    printf("%4d: %02x", offset, tdata[0]);

		    nbytes = readline (" ? ");
		    if (nbytes == 0 ||
			(nbytes == 1 && console_buffer[0] == '-')) {
			/* <CR> pressed as only input, don't modify current
			 * location and move to next.
			 * "-" pressed will go back.
			 */
			offset += nbytes ? -1 : 1;
			nbytes = 1;
		    } else {
			char *endp;
			value = simple_strtoul(console_buffer, &endp, 16);
			nbytes = endp - console_buffer;
			if (nbytes) {
			    tdata[0] = value;
			    if (secure) {
				if ((ret = secure_eeprom_write(device,
					offset,
					tdata, 1))
				    != 0) {
				    printf("I2C write to EEPROM device "
					"0x%x at 0x%x failed.\n",
					device, offset);
				    return (1);
				}
			    } else {
				if ((ret = eeprom_write(device, offset,
					tdata, 1)) != 0) {
				    printf("I2C write to EEPROM device "
					"0x%x at 0x%x failed.\n",
					device, offset);
				    return (1);
				}
			    }
			}
			offset += 1;
		    }
		} while (nbytes);
		return (1);
	    }

	    data = argv [4];
	    if ((argv[5][0] == 'h') || (argv[5][0] == 'H')) {
		len = strlen(data) / 2;
		tbyte[2] = 0;
		for (i = 0; i < len; i++) {
		    tbyte[0] = data[2 * i];
		    tbyte[1] = data[2 * i + 1];
		    temp = atoh(tbyte);
		    tdata[i] = temp;
		}
	    } else {
		len = strlen(data);
		for (i = 0; i < len; i++) {
		    tdata[i] = data[i];
		}
	    }
	    if ((argv[1][2] == 's') || (argv[1][2] == 'S')) {
		if ((ret = secure_eeprom_write(device, offset, tdata,
			len)) != 0) {
		    printf("I2C write to EEPROM device 0x%x failed.\n",
			device);
		}
	    } else {
		if ((ret = eeprom_write(device, offset, tdata, len))
		    != 0) {
		    printf("I2C write to EEPROM device 0x%x failed.\n",
			device);
		}
	    }
	}
	break;

    case 't':          /* I2C temperature */
    case 'T':
	value = 1000000;
	if (argc > 2) {
	    if ((argv[2][0] == 's') || (argv[3][0] == 'S')) {/* SFP+ temp */
		//i2c_sfp_temp();
		return 1;
	    }
	    value = simple_strtoul(argv[2], NULL, 10);
	}
	if (argc > 3)
	    mloop = simple_strtoul(argv[3], NULL, 10);
	if (argc > 4)
	    threshold = simple_strtoul(argv[4], NULL, 10);

	if (mloop) {
	    for (i = 0; i < mloop; i++) {
		if (i2c_read_temp(threshold))
		    break;
		if (ctrlc()) {
		    break;
		}
		/* 1000 ms */
		udelay(value);
	    }
	} else {
	    for (;;) {
		mloop++;
		printf("loop %6ld\n", mloop);

		if (i2c_read_temp(threshold))
		    break;
		if (ctrlc()) {
		    break;
		}
		/* 1000 ms */
		udelay(value);
	    }
	}
	break;

    case 'v':          /* voltage */
    case 'V':
	i2c_read_volt();
	break;

    case 's':          /* scan */
    case 'S':
	if ((argc == 3) && ((argv[2][0] == 'r') || (argv[2][0] == 'R')))
	    rsta = 1;

	for (i = 0; i < 0x7F; i++) {
	    if (rsta)
		ret = i2c_read(i, 0, 1, tdata, 1);
	    else
		ret = i2c_read_norsta(i, 0, 1, tdata, 1);
	    if (ret == 0)
		printf("Found I2C device at 0x%02x.\n", i);
	    /* 100 ms */
	    udelay(100000);
	}
	break;

    default:
	printf("???");
	break;
    }

    return (1);

usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

/*
 * Send POE command/request to the POE controller.
 */
static int
poe_send_request (uint8_t *tdata) {

    if ((i2c_write_noaddr(CFG_I2C_C2_9548SW1_P3_POE, tdata, 15)) != 0) {
	printf("i2c failed to write poe data to address 0x%x.\n",
	    CFG_I2C_C2_9548SW1_P3_POE);
	return -1;
    }
    return 0;
}

/*
 * Read the response (telemetry/report) from the POE controller.
 */
static int
poe_read_response (uint8_t *rdata) {

    if ((i2c_read_noaddr(CFG_I2C_C2_9548SW1_P3_POE, rdata, 15)) != 0) {
	printf("i2c failed to read poe status from address 0x%x.\n",
	    CFG_I2C_C2_9548SW1_P3_POE);
	return -1;
    }
    return 0;
}

static int
poe_write (char *data) {

    uint32_t i, len;
    int dtemp;
    uint8_t tdata[15];
    uint16_t cksum = 0;
    char tbyte[3];

    /* 
     * User need to enter 13 bytes of data which includes
     * key. The 2 byte checksum will be calculated and 
     * 15 byte (data plus checksum) will be sent to 
     * POE controller.
     */
    len = strlen(data)/2;
    if (len != 13) {
	printf("Enter 13 bytes. See usage\n");
	return -1;
    }

    /* 
     * The '0x4e' equivalent of ascii char 'N' is needed for the POE
     * controller. The POE controller expects some fields of 15 byte to be set
     * with '0x4e'.
     */
    memset(tdata, 0x4E, 15);

    tbyte[2] = 0;
    for (i = 0; i < len; i++) {
	tbyte[0] = data[2*i];
	tbyte[1] = data[2*i+1];
	dtemp = atoh(tbyte);
	tdata[i] = dtemp;
    }

    /*
     * POE checksum is a 16 bit word, containing the arithmetic
     * sum of the first 13 message bytes (without checksum bytes).
     */
    for (i = 0; i < 13; i++) {
	cksum += tdata[i];
    }
    tdata[13] = (cksum & 0xff00) >> 8;
    tdata[14] = cksum & 0xff;

    /* send 15 byte command/request to POE controller */
    if (poe_send_request(tdata) == -1) {
	printf("Failed to send the POE request\n");
	return -1;
    }
    return 0;
}

/* POE commands */
int
do_poe (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]) {
    char cmd;
    uint8_t rdata[15];
    uint32_t i;
    char *data;

    if (argc <= 1)
	goto usage;

    cmd = argv[1][0];

    switch (cmd) {
    case 'o':       /* On/Off */
    case 'O':
	if ((argv[1][1]=='n') || (argv[1][1]=='N')) {
	    if ((i2c_mux(CFG_I2C_CTRL_2, CFG_I2C_C2_9548SW1, 3, 1)) != 0) {
		printf("Failed to set POE channel on %d, 0x%x: \n",
		    CFG_I2C_CTRL_2, CFG_I2C_C2_9548SW1);
		return -1;
	    }
	} else if (((argv[1][1]=='f') || (argv[1][1]=='F')) &&
	    ((argv[1][2]=='f') || (argv[1][2]=='F'))) {
	    if ((i2c_mux(CFG_I2C_CTRL_2, CFG_I2C_C2_9548SW1, 0, 0)) != 0) {
		printf("Failed to disable POE channel on %d, 0x%x: \n",
		    CFG_I2C_CTRL_2, CFG_I2C_C2_9548SW1);
		return -1;
	    }
	} else {
	    goto usage;
	}

	break;

    case 'w':      /* Write request & telemetry */
    case 'W':
	if (argc <= 2)
	    goto usage;

	data = argv[2];

	if (poe_write(data) == -1) {
	    return -1;
	}

	break;

    case 'r':       /* Read command */
    case 'R':
	if (argc < 2)
	    goto usage;

	data = argv[2];

	if (poe_read_response(rdata) == -1) {
	    return -1;
	}

	printf ("Report message from POE - \n");
	for (i = 0; i < 15; i++)
	    printf("0x%x ", rdata[i]);
	printf("\n");

	break;

    default:
	goto usage;
	break;
    }

    return 0;
usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return -1;
}


static void
epld_dump (void)
{
    int i;
    uint16_t val;

    for (i = EPLD_RESET_BLOCK_UNLOCK; i <= EPLD_LAST_REG; i++) {
	if (epld_reg_read(i, &val))
	    printf("EPLD addr 0x%02x = 0x%04x.\n", i*2, val);
	else
	    printf("EPLD addr 0x%02x read failed.\n", i*2);
    }
}

/* EPLD commands
 *
 * Syntax:
 *    epld register read [addr]
 *    epld register write addr value
 */
int
do_epld (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int addr = 0;
    uint16_t value = 0, mask, temp, lcmd;
    char cmd1, cmd2, cmd3;
    int port;
    PortLink link;
    PortDuplex duplex;
    PortSpeed speed;

    if (argc <= 2)
	goto usage;

    cmd1 = argv[1][0];
    cmd2 = argv[2][0];

    switch (cmd1) {
    case 'r':       /* Register */
    case 'R':
	switch (cmd2) {
	case 'd':   /* dump */
	case 'D':
	    epld_dump();
	    break;

	case 'r':   /* read */
	case 'R':
	    if (argc <= 3)
		goto usage;

	    addr = simple_strtoul(argv[3], NULL, 16);
	    if (epld_reg_read(addr / 2, &value))
		printf("EPLD addr 0x%02x = 0x%04x.\n", addr, value);
	    else
		printf("EPLD addr 0x%02x read failed.\n", addr);
	    break;

	case 'w':   /* write */
	case 'W':
	    if (argc <= 4)
		goto usage;

	    addr = simple_strtoul(argv[3], NULL, 16);
	    value = simple_strtoul(argv[4], NULL, 16);
	    if (argc >= 6) {
		mask = simple_strtoul(argv[5], NULL, 16);
		epld_reg_read(addr / 2, &temp);
		value = (temp & mask) | value;
	    }
	    if (!epld_reg_write(addr / 2, value))
		printf("EPLD write addr 0x%02x value 0x%04x failed.\n",
		    addr, value);
	    else {
		if (epld_reg_read(addr / 2, &value))
		    printf("EPLD read back addr 0x%02x = 0x%04x.\n",
			addr, value);
		else
		    printf("EPLD addr 0x%02x read back failed.\n",
			addr);
	    }
	    break;
	case 'p':    /* Plain_write with NO readback */	
	case 'P':
	    if (argc != 5)
		goto usage;

	    addr = simple_strtoul(argv[3], NULL, 16);
	    value = simple_strtoul(argv[4], NULL, 16);
	    if (!epld_reg_write(addr / 2, value))
		printf("EPLD write addr 0x%02x val 0x%04x failed.\n", addr, value);
	    else 
		printf("EPLD value written to 0x%02x is = 0x%04x\n", addr, value);
	    break;

	default:
	    printf("???");
	    break;
	}
	break;

    case 'w':   /* Watchdog */
    case 'W':
	switch (cmd2) {
	case 'e':
	case 'E':
	    epld_watchdog_enable();
	    break;

	case 'd':
	case 'D':
	    if ((argv[2][1] == 'u') || (argv[2][1] == 'U'))
		epld_watchdog_dump();
	    else
		epld_watchdog_disable();
	    break;

	case 'r':
	case 'R':
	    epld_watchdog_reset();
	    break;

	default:
	    printf("???");
	    break;
	}
	break;

    case 'c':       /* CPU */
    case 'C':
	if (argc <= 3)
	    goto usage;

	if ((cmd2 == 'r') || (cmd2 == 'R')) {
	    cmd3 = argv[3][0];
	    if ((cmd3 == 's') || (cmd3 == 'S'))
		printf("EPLD does not support cpu soft reset\n");
	    else if ((cmd3 == 'h') || (cmd3 == 'H'))
		epld_cpu_hard_reset();
	    else
		printf("???");
	} else
	    printf("???");

	break;

    case 's':       /* System */
    case 'S':
	if ((cmd2 == 'r') || (cmd2 == 'R')) {
	    if (argc == 3)
		epld_system_reset();
	    else if ((argc == 4) && ((argv[3][0] == 'h') ||
		  (argv[3][0] == 'H')))
		epld_system_reset_hold();
	    else
		printf("???");
	} else
	    printf("???");

	break;

    case 'l':        /* LED/LCD */
    case 'L':
	if (((cmd2 == 'p') || (cmd2 == 'P')) && ((argv[2][1] =='o') ||
	      (argv[2][1] == 'O'))) {       /* port LED */
	    if (argc <= 6)
		goto usage;

	    port = simple_strtol(argv[3], NULL, 10);
	    if ((argv[4][0] == 'u') || (argv[4][0] == 'U'))
		link = LINK_UP;
	    else if ((argv[4][0] == 'd') || (argv[4][0] == 'D'))
		link = LINK_DOWN;
	    else
		goto usage;

	    if ((argv[5][0] == 'h') || (argv[5][0] == 'H'))
		duplex = HALF_DUPLEX;
	    else if ((argv[5][0] == 'f') || (argv[5][0] == 'F'))
		duplex = FULL_DUPLEX;
	    else
		goto usage;

	    if (argv[6][0] != '1')
		goto usage;

	    if ((argv[6][1] == 'g') || (argv[6][1] == 'G'))
		speed = SPEED_1G;
	    else if (argv[6][1] == '0') {
		if ((argv[6][2] == 'g') || (argv[6][2] == 'G'))
		    speed = SPEED_10G;
		else if ((argv[6][2] == 'm') || (argv[6][2] == 'M'))
		    speed = SPEED_10M;
		else if ((argv[6][2] == '0') && ((argv[6][3] == 'm') ||
		      (argv[6][3] == 'M')))
		    speed = SPEED_100M;
		else
		    goto usage;
	    } else
		goto usage;

	    set_port_led(port, link, duplex, speed);
	} else {      /* LCD */
	    switch (cmd2) {
	    case 'o':
	    case 'O':
		if ((argv[2][1] == 'n') || (argv[2][1] == 'N'))
		    lcd_init(0);
		else if ((argv[2][1] == 'f') || (argv[2][1] == 'F'))
		    lcd_off(0);
		else
		    goto usage;
		break;

	    case 'b':
	    case 'B':
		if ((argv[3][0] == 'o') || (argv[3][0] == 'O')) {
		    if ((argv[3][1] == 'n') || (argv[3][1] == 'N'))
			lcd_backlight_control(0, LCD_BACKLIGHT_ON);
		    else if ((argv[3][1] == 'f') || (argv[3][1] == 'F'))
			lcd_backlight_control(0, LCD_BACKLIGHT_OFF);
		    else
			goto usage;
		} else
		    goto usage;
		break;

	    case 'h':
	    case 'H':
		lcd_heartbeat(0);
		break;

	    case 'i':
	    case 'I':
		lcd_init(0);
		break;

	    case 'p':
	    case 'P':
		lcd_printf(0, "%s\n",argv[3]);
		break;

	    case 'C':
	    case 'c':
		lcmd = simple_strtoul(argv[3], NULL, 16);
		lcd_cmd(0, lcmd);
		break;

	    case 'D':
	    case 'd':
		lcd_dump(0);
		break;

	    default:
		printf("???");
		break;
	    }
	}
	break;

    default:
	printf("???");
	break;
    }

    return (1);
usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

int
do_runloop (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
    int i, j = 0;

    if (argc < 2) {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }
    for (;;) {
	if (ctrlc()) {
	    putc('\n');
	    return 0;
	}
	printf("\n\nLoop:  %d\n", j);
	j++;
	for (i = 1; i <argc; ++i) {
	    char *arg;

	    if ((arg = getenv(argv[i])) == NULL) {
		printf("## Error: \"%s\" not defined\n", argv[i]);
		return (1);
	    }
#ifndef CFG_HUSH_PARSER
	    if (run_command(arg, flag) == -1)
		return (1);
#else
	    if (parse_string_outer(arg,
		FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0)
		return (1);
#endif
	}
    }
    return (0);
}

int
do_usleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    ulong delay;

    if (argc != 2) {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }

    delay = simple_strtoul(argv[1], NULL, 10);
    udelay(delay);

    return (0);
}

typedef struct
    { char *name; ushort id; char *clei; ushort macblk; }
    assm_id_type;

assm_id_type assm_id_list_system [] = {
    { "EX4300-24T"     , I2C_ID_JUNIPER_EX4300_24T, "N/A       " ,0x40},
    { "EX4300-48T"     , I2C_ID_JUNIPER_EX4300_48T, "N/A       " ,0x40},
    { "EX4300-24P"     , I2C_ID_JUNIPER_EX4300_24_P,  "N/A     ", 0x40},
    { "EX4300-48P"     , I2C_ID_JUNIPER_EX4300_48_P,  "N/A     ", 0x40},
    { "EX4300-48T-BF"  , I2C_ID_JUNIPER_EX4300_48_T_BF, "N/A   ", 0x40},
    { "EX4300-48T-DC"  , I2C_ID_JUNIPER_EX4300_48_T_DC, "N/A   ", 0x40},
    { "EX4300-32F"     , I2C_ID_JUNIPER_EX4300_32F, "N/A       ", 0x40},
    {  NULL             , 0                          }
};

int
do_mfgcfg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    assm_id_type *assm_id_list = assm_id_list_system;
    int access = 0, present, rev = 0;
    uint16_t id;
    uint8_t device, mux[2], chan[2], nmux = 0, cmux = 0;
    unsigned int start_address = 0;
    int controller = CFG_I2C_CTRL_1;
    int ret, assm_id = 0;
    int i, j, nbytes;
    char tbyte[3], tdata[0x80];

    if (argc <= 2) {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }

    if (strcmp(argv[1], "system") == 0) {
	access = 0;
    } else {
	printf("Usage:\n%s\n", cmdtp->usage);
	return (1);
    }

    switch(access)
    {
    case 0: /* ex4300 */
	controller   = CFG_I2C_CTRL_1;
	mux[0] = CFG_I2C_C1_9548SW1;
	chan[0] = 4;
	nmux = 1;
	device = CFG_I2C_C1_9548SW1_P4_R5H30211;
	assm_id_list = assm_id_list_system;
	break;
    default:
	break;
    }

    cmux = 0;
    i2c_mfgcfg = 1;
    for (i = 0; i < nmux; i++) {
	if ((ret = i2c_mux(controller, mux[i], chan[i], 1)) != 0) {
	    printf("i2c failed to enable mux 0x%x channel 0x%x.\n",
		    mux[i], chan[i]);
	    cmux = i;
	    break;
	}
    }

    if (cmux) {  /* setup failed, reversal to clear setup */
	for (i = cmux; i > 0; i--)
	    i2c_mux(controller, mux[i], chan[i], 0);
	i2c_mfgcfg = 0;
	return (1);
    }

    if (secure_eeprom_read(device, start_address, tdata, 0x80)) {
	printf("ERROR reading eeprom at device 0x%x\n", device);
	for (i = nmux; i > 0; i--)
	    i2c_mux(controller, mux[i], chan[i], 0);
	i2c_mfgcfg = 0;
	return (1);
    }

    for (i = 0; i < 0x80; i+=8)
    {
	printf("%02X : ",i + start_address);
	for (j = 0; j < 8; j++)
	    printf("%02X ",tdata[i + j]);
	printf("\n");
    }

    printf("Start address (Hex)= 0x%04X\n", start_address);
    assm_id   = (tdata[4] << 8) + tdata[5];
    printf("Assembly Id (Hex)= 0x%04X\n", assm_id);
    for (i = 0; assm_id_list[i].name; i++)
	if (assm_id_list[i].id == assm_id)
	    break;

    if (assm_id_list[i].name)
	printf("Assembly name = %s\n", assm_id_list[i].name);
    else
	printf("Invalid / Undefined Assembly Id\n");

    if (argc >= 3)
    {
	if (!strcmp(argv[2], "update"))
	{
	    /* constants for all eeproms*/
	    memcpy(tdata      , "\x7F\xB0\x02\xFF", 4);
	    memcpy(tdata + 0x08 , "REV "            , 4);
	    memcpy(tdata + 0x31 , "\xFF\xFF\xFF"    , 3);
	    tdata[0x2c] = 0;
	    tdata[0x44] = 1;
	    memset(tdata + 0X74, 0x00, 12);

	    printf("List of assemblies:\n");
	    for (i = 0; assm_id_list[i].name; i++)
		printf("%2d = %s\n", i + 1, assm_id_list[i].name);

	    nbytes = readline("\nSelect assembly:");
	    if (nbytes) {
		i = simple_strtoul(console_buffer, NULL, 10) - 1;
		assm_id = assm_id_list[i].id;
		printf("new Assm Id (Hex)= 0x%04X\n", assm_id);
		tdata[4] = assm_id >>   8;
		tdata[5] = assm_id & 0xFF;
	    } else {
		for (i = 0; assm_id_list[i].name; i++)
			if (assm_id_list[i].id == assm_id)
		break;
	    }
	    printf("\nHW Version : '%.3d'\n "   ,tdata[0x6]);
	    nbytes = readline("Enter HW version: ");
	    if (nbytes)
		tdata[0x06] = simple_strtoul(console_buffer, NULL, 10);

	    printf("\n"
		   "Assembly Part Number  : '%.12s'\n "   ,tdata + 0x14);
	    printf("Assembly Rev          : '%.8s'\n"     ,tdata + 0x0C);
	    nbytes = readline("Enter Assembly Part Number: ");
	    if (nbytes >= 10) {
		strncpy(tdata + 0x14, console_buffer,10);
		tdata[0x14 + 10] = 0;
		if (nbytes >= 16 &&
		    (!strncmp(console_buffer + 10, " REV ", 5) ||
		     !strncmp(console_buffer + 10, " Rev ", 5) ))
		{
		    strncpy(tdata + 0x0C, console_buffer + 15,8);
		    tdata[0x07] = simple_strtoul(console_buffer + 15, NULL, 10);
		} else {   /* enter Rev separate */
		    nbytes = readline("Enter Rev: ");
		    strncpy(tdata + 0x0C, console_buffer,8);
		    tdata[0x07] = simple_strtoul(console_buffer, NULL, 10);
		}
	    }

	    /* only ask for Model number/rev for FRU */
	    if (assm_id_list[i].name &&
		strncmp(assm_id_list[i].clei, "N/A", 10)) {
		printf("\n"
		       "Assembly Model Name   : '%.23s'\n " ,tdata + 0x4f);
		printf("Assembly Model Rev    : '%.3s'\n"   ,tdata + 0x66);
		nbytes = readline("Enter Assembly Model Name: ");
		if (nbytes >= 13) {
		    strncpy(tdata + 0x4f, console_buffer,13);
		    tdata[0x4f + 13] = 0;

		    if (nbytes >= 19 &&
			(!strncmp(console_buffer + 13, " REV ", 5) ||
			 !strncmp(console_buffer + 13, " Rev ", 5) ))
			strncpy(tdata + 0x66, console_buffer + 18,3);
		    else {    /* enter Rev separate */
			nbytes = readline("Enter Rev: ");
			strncpy(tdata + 0x66, console_buffer,3);
		    }
		}
	    } else
		tdata[0x44] = 0;     /* not a FRU */

	    printf("\nAssembly Serial Number: '%.12s'\n"   ,tdata + 0x20);
	    nbytes = readline("Enter Assembly Serial Number: ");
	    if (nbytes)
		strncpy(tdata + 0x20, console_buffer,12);

	    if (!strcmp(argv[1], "system")) {
		/* constants for system eeprom */
		memcpy(tdata + 0x34, "\xAD\x01\x00",3);

retry_mac:
		printf("\nBase MAC: ");
		for (j = 0; j < 6; j++)
		    printf("%02X",tdata[0x38+j]);

		nbytes = readline("\nEnter Base MAC: ");
		for (i = j = 0; console_buffer[i]; i++)
		    if (isxdigit(console_buffer[i]))
			console_buffer[j++] = console_buffer[i];
		console_buffer[j] = 0;
		nbytes = j;

		if (nbytes) {
		    if (nbytes==12) {
			tbyte[2] = 0;
			for (i = 0; i < 6; i++) {
			    tbyte[0]      = console_buffer[2 * i];
			    tbyte[1]      = console_buffer[2 * i + 1];
			    tdata[0x38 + i] = atoh(tbyte);
			}
		    } else {
			printf("Mac must be 12 characters! "
			       "No update performed\n");
			goto retry_mac;
		    }
		}

		assm_id   = (tdata[4] << 8) + tdata[5];
		for (i = 0; assm_id_list[i].name; i++)
		    if (assm_id_list[i].id == assm_id)
			break;

		if (assm_id_list[i].name) {
		    tdata[0x37] = assm_id_list[i].macblk;
		}
	    }

	    printf("\nDeviation Info        : '%.10s'\n"   ,tdata + 0x69);
	    nbytes = readline("Enter Deviation Info: ");
	    if (nbytes)
		strncpy(tdata + 0x69, console_buffer,10);

	    /* prog clei */
	    for (i = 0; assm_id_list[i].name; i++)
		if (assm_id_list[i].id == assm_id)
		    break;
	    if (assm_id_list[i].name)
		strncpy(tdata + 0x45, assm_id_list[i].clei,10);

	    /* calculate checksum */
	    for (j = 0, i = 0x44; i <= 0x72; i++)
		j += tdata[i];
	    tdata[0x73] = j;

	    if (secure_eeprom_write(device, start_address, tdata, 0x80)) {
		printf("ERROR writing eeprom at device 0x%x\n", device);
	    }
	} else if (!strcmp(argv[2], "clear")) {
	    memset(tdata, 0xFF, 0x80);
	    if (secure_eeprom_write(device, start_address, tdata, 0x80)) {
		printf("ERROR clearing eeprom at device 0x%x\n", device);
	    }
	}

	/* always reread */
	if (secure_eeprom_read(device, start_address, tdata, 0x80)) {
	    printf("ERROR reading eeprom at device 0x%x\n", device);
	} else {
	    for (i = 0; i < 0x80; i+=8) {
		printf("%02X : ",i + start_address);
		for (j = 0; j < 8; j++)
		    printf("%02X ",tdata[i+j]);
		printf("\n");
	    }
	}
    }

    for (i = nmux; i > 0; i--)
	i2c_mux(controller, mux[i], chan[i], 0);
    i2c_mfgcfg = 0;

    assm_id   = (tdata[4] << 8) + tdata[5];
    for (i = 0; assm_id_list[i].name; i++)
	if (assm_id_list[i].id == assm_id)
	    break;

    printf("Assembly Id (Hex)     : '0x%04X'\n"  , assm_id);
    if (assm_id_list[i].name)
    {
	printf("HW Version            : '%.3d'\n"    , tdata[0x06]);
	printf("Assembly Model Name   : '%.23s'\n"   , tdata + 0x4F);
	printf("Assembly Model rev    : '%.3s'\n"    , tdata + 0x66);
	printf("Assembly Clei         : '%.10s'\n"   , tdata + 0x45);
	printf("Assembly Part Number  : '%.12s'\n"   , tdata + 0x14);
	printf("Assembly Rev          : '%.8s'\n"    , tdata + 0x0C);
	printf("Assembly Serial Number: '%.12s'\n"   , tdata + 0x20);
	if (!strcmp(argv[1], "system")) {
	    printf("Base MAC              : ");
	    for (j = 0; j < 6; j++)
		printf("%02X",tdata[0x38 + j]);
	}
	printf("\n"
	       "Deviation Info        : '%.10s'\n"   ,tdata+0x69);
    }
    return (1);
}

/* FIXME - have stub function for now. */
int
do_si (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    return 0;
}

#endif

/***************************************************/

U_BOOT_CMD(
    upgrade, 3, 1, do_upgrade,
    "upgrade - upgrade state\n",
    "\n"
    "upgrade get\n"
    "    - get upgrade state\n"
    "upgrade set <state>\n"
    "    - set upgrade state\n"
);

U_BOOT_CMD(
    resetenv,      1,     1,      resetenv_cmd,
    "resetenv	- Return all environment variable to default.\n",
    " \n"
    "\t Erase the environemnt variable sector.\n"
);

#if !defined(CONFIG_PRODUCTION)
U_BOOT_CMD(
    i2c_ctrl,   2,  1,  do_i2c_controller,
    "i2c_ctrl     - select I2C controller\n",
    "ctrl\n    - I2C controller number\n"
    "      (valid controller values 1..2, default 1)\n"
);

U_BOOT_CMD(
    i2c_expander, 7, 1, do_i2c_diag,
    "i2c_expander   - diag on slave device directly on i2c + IOExpander\n",
    "   r/w read (1) or write (0) operation\n"
    "   ctrl specifies the i2c ctrl 1 or 2\n"
    "   slave_addr  address (in hex) of the device on i2c\n"
    "   reg - register (in hex) in the device to which data to be r/w\n"
    "   data     the data to be r/w\n"
    "   length_data length (decimal) of the data to r/w; for IOexp, this is"
    "1\n"
);

U_BOOT_CMD(
    i2c_switch, 9, 1,   do_i2c_diag_switch,
    "i2c_switch   - i2c on a specific slave device with switch\n",
    "   r/w  read (1) or write (0) operation\n"
    "   ctrl specifies the i2c ctrl 1 or 2\n"
    "   switch  the switch address (hex) the dev sits on\n"
    "   channel the channel number on the switch\n"
    "   slave_addr address (hex) of the device on i2c\n"
    "   reg - register (hex) in the device to which data to be r/w, "
    "0xff - reg not applicable\n"
    "   data    the data to be r/w\n"
    "   length_data length (decimal) of the data to r/w\n"
);

U_BOOT_CMD(
    i2c_cksum, 2, 1,    do_i2c_cksum,
    "i2c_cksum   - i2c Tx checksum on/off\n",
    "   on/off   Tx checksum on (1) or off (0)\n"
);

U_BOOT_CMD(
    runloop,	CONFIG_SYS_MAXARGS,	1,	do_runloop,
    "runloop - run commands in an environment variable\n",
    "var [...]\n"
    "    - run the commands in the environment variable(s) 'var' "
    "infinite loop\n"
);

U_BOOT_CMD(
    usleep ,    2,    2,     do_usleep,
    "usleep  - delay execution for usec\n",
    "\n"
    "    - delay execution for N micro seconds (N is _decimal_ !!!)\n"
);

U_BOOT_CMD(
    i2c_ex43xx,    8,  1,  do_i2c_ex43xx,
    "i2c_ex43xx     - i2c test commands\n",
    "\n"
    "i2c_ex43xx bw <address> <value>\n"
    "    - write byte to i2c address\n"
    "i2c_ex43xx br <address>\n"
    "    - read byte from i2c address\n"
    "i2c_ex43xx dw <address> <reg> <data> [checksum]\n"
    "    - write data to i2c address\n"
    "i2c_ex43xx dr <address> <reg> <len>\n"
    "    - read data from i2c address\n"
    "i2c_ex43xx ew <address> <offset> <data> [hex|char]\n"
    "    - write data to EEPROM at i2c address\n"
    "i2c_ex43xx er <address> <offset> <len>\n"
    "    - read data from EEPROM at i2c address\n"
    "i2c_ex43xx ews <address> <offset> <data> [hex|char]\n"
    "    - write data to secure EEPROM at i2c address\n"
    "i2c_ex43xx ers <address> <offset> <len>\n"
    "    - read data from secure EEPROM at i2c address\n"
    "i2c_ex43xx ewo <address> <offset> [secure]\n"
    "    - write byte to offset (auto-incrementing) of the device EEPROM\n"
    "i2c_ex43xx mux [enable|disable] <mux addr> <channel>\n"
    "    - enable/disable mux channel\n"
    "i2c_ex43xx led <unit> set <led> <color> <state>\n"
    "    - i2c led [0|1] set [top|middle|bottom] [off|red|green|amber] "
    "[steady|slow|fast]\n"
    "i2c_ex43xx led <unit> update\n"
    "    - i2c led [0|1] update led\n"
    "i2c_ex43xx fan [[<delay> |<fan> duty <percent>]|[<fan> rpm]]\n"
    "    - i2c fan test interval delay #ms (1000ms) or individual fan duty\n"
    "i2c_ex43xx temperature [SFP+ |<delay>] [<loop>] [<threshold>]\n"
    "    - i2c temperature reading delay #us (100000) for Winbond read "
    "temperature\n"
    "i2c_ex43xx voltage\n"
    "    - i2c voltage reading for Winbond\n"
    "i2c_ex43xx fdr <fdr>\n"
    "    - i2c set fdr\n"
    "i2c_ex43xx scan [rstart]\n"
    "    - i2c scan 0-0x7F [repeat start]\n"
    "i2c_ex43xx manual [off|on]\n"
    "    - i2c manual operation off (default) or on\n"
);

U_BOOT_CMD(
    epld,   7,  1,  do_epld,
    "epld    - EPLD test commands\n",
    "\n"
    "epld register read <address>\n"
    "    - read EPLD register\n"
    "epld register write <address> <value> [<mask>]\n"
    "    - write EPLD register with option mask\n"
    "epld register plain_write <address> <value>\n"
    "    - a plain write to epld reg with no readbacks\n"
    "epld register dump\n"
    "    - dump EPLD registers\n"
    "epld watchdog [enable | disable | reset | dump]\n"
    "    - EPLD watchdog operation\n"
    "epld CPU reset [hard]\n"
    "    - EPLD CPU reset\n"
    "epld system reset [hold]\n"
    "    - EPLD system reset\n"
    "epld i2c [reset |clear] <1..5>\n"
    "    i2c mux reset/clear I2C_RST_L<n>\n"
    "    1-C0 70(U17) / C1 71(U19) / C1 73(U20)\n"
    "    2-C1 71P0 74(U51) / C1 71P1 74(U53) / C1 71P2 74(U52)\n"
    "    3-C1 21(U16) / C1 M71P0 24(U55) / C1 M71P2 24(U208)/\n"
    "      C1 M71P2 20(U207) / C1 M71P2 22(U210)\n"
    "    4-C1 M71P3 74(U67) / C1 M72P0 74(U68)\n"
    "    5-C1 M71P1 20(U65) / C1 M71P1 22(U66) / C1 M71P1 24(U64)\n"
    "epld LCD backlight [on | off]\n"
    "    - EPLD LCD backlight\n"
    "epld LCD init\n"
    "    - EPLD init\n"
    "epld LCD [on | off]\n"
    "    - EPLD LCD on/off\n"
    "epld LCD heartbeat <unit>\n"
    "    - EPLD LCD heartbeat\n"
    "epld LCD print <string>\n"
    "    - EPLD LCD print\n"
    "epld LCD dump\n"
    "    - EPLD LCD DDRAM\n"
    "epld LCD command <cmd>\n"
    "    - EPLD LCD control command\n"
);

U_BOOT_CMD(
    rtc,	4,	1,	do_rtc,
    "rtc     - get/set/reset date & time\n",
    "\n"
    "rtc\n"
    "    - rtc get time\n"
    "rtc reset\n"
    "    - rtc reset time\n"
    "rtc [init | start |stop]\n"
    "    - rtc start/stop\n"
    "rtc set YYMMDD hhmmss\n"
    "    - rtc set time YY/MM/DD hh:mm:ss\n"
    "rtc status\n"
    "    - rtc status\n"
    "rtc dump\n"
    "    - rtc register dump\n"
);

U_BOOT_CMD(
    mfgcfg,    3,    0,     do_mfgcfg,
    "mfgcfg  - manufacturing config of EEPROMS\n",
    "\nmfgcfg [system|m1-40|m1-80|m2] [read|update|clear]\n"
);

U_BOOT_CMD(
    poe,    3,  1,  do_poe,
    "poe - poe test commands\n",
    "\n"
    "poe [on|off]\n"
    "    - poe on/off\n"
    "poe write <13 bytes of data including key and excluding checksum>\n"
    "    - poe write\n"
    "poe read <13 bytes of data including key and excluding checksum>\n"
    "    - poe read\n"
);

U_BOOT_CMD(
    si,    4,    1,     do_si,
    "si      - signal integrate\n",
    "\n"
    "si gpin \n"
    "    - gpin status bit0-7\n"
    "si gpout [on | off] <device>\n"
    "    - gpout [on|off]  0 -M1-40, 1 - M1-80k, 2 - M2, 3 - ISP1568, "
    "4 - ST72681, 5 - W83782G, 6 - ISP1520\n"
    "si fm [-5|-3|0|1|2|3|4|5]\n"
    "    - Frequency margin MK1493 -5/-3/0/1/2/3/4/5 %\n"
    "si vm [3.3|2.5|1.1|1.0] [-5|+5|0]\n"
    "    - Voltage margin 3.3/2.5/1.1/1.0 V -5/+5/0 %\n"
    "si vm_pwr1220 [SFP+|PHY1|PHY2|L1|L2] [-5|+5|0]\n"
    "    - Voltage margin SFP+(3.3V)/PHY(1.2V)/Lion(1.8V) -5/+5/0 %\n"
);
#endif
