/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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
#include <ccb_iic.h>
#include <cpld.h>
#include <ex62xx_common.h>
#include <jcb.h>

DECLARE_GLOBAL_DATA_PTR;
extern int flash_write_direct(ulong addr, char* src, ulong cnt);

/* upgrade commands
 *
 * Syntax:
 *    upgrade get
 *    upgrade set <state>
 */
int do_upgrade(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	char cmd1;
	int state;
	volatile int *ustate = (int *)(CONFIG_SYS_FLASH_BASE \
	    + CFG_UPGRADE_BOOT_STATE_OFFSET);

	if (argc < 2) {
		goto usage;
	}

	cmd1 = argv[1][0];

	switch (cmd1) {
	case 'g':	/* get state */
	case 'G':
		printf("upgrade state = 0x%x\n", *ustate);
		break;
	case 's':	/* set state */
	case 'S':
		state = simple_strtoul(argv[2], NULL, 16);
		if (state == *ustate) {
			return 1;
		}

		if (flash_write_direct((ulong)ustate, (char *)&state, sizeof(state)) != 0) {
			printf("upgrade set state 0x%x failed.\n", state);
		}
		return 0;
		break;

	default:
		printf("???");
		break;
	}

	return 1;
usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

U_BOOT_CMD(
	upgrade, 3, 1, do_upgrade,
	"upgrade - upgrade state",
	"upgrade get\n"
	"    - get upgrade state\n"
	"upgrade set <state>\n"
	"    - set upgrade state\n"
	);


int do_i2cs(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char cmd1, cmd2;
    uint8_t group;

    if (argc < 2) {
	goto usage;
    }
    cmd1 = argv[1][0];
    cmd2 = argv[2][0];

    switch (cmd1) {
    case 'g':
    case 'G':
	if (argc < 3) {
	    jcb_fpga_regs_t *ccb_p = (jcb_fpga_regs_t *)ccb_get_regp();
	    group = (ccb_p->jcb_i2cs_mux_ctrl >> 8);
	    group &= JCB_I2C_MUX_GRP_MASK;
	    printf("\ni2c mux group: 0x%x\n", group);
	} else {
	    i2c_set_bus_num(CONFIG_I2C_BUS_MASTER);
	    group  = simple_strtoul(argv[2], NULL, 10) & 0xFF;
	    ccb_iic_ctlr_group_select(group);
	    printf("\ni2c mux group 0x%x selected\n", group);
	}
	break;
    default:
	printf("Unknown command\n");
	goto usage;
	break;
    }
    return 0;
usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    return 1;
}

U_BOOT_CMD(
	i2cs,   3,  1,  do_i2cs,
	"I2C commands using external controller",
	"group <value>\n"
	"    - select the i2c slave mux\n"
	);

int mfgtest_memory(void)
{
    int mtest_argc = 0;
    char *mtest_argv[5]; 
    char tmp_buf1[16], tmp_buf2[16];
    char tmp_buf3[16], tmp_buf4[16];
    int pattern, iter;
    int ret;

    /* run "mtest" command */
    mtest_argv[0] = "mtest";
    /* setup memory range */
    sprintf(tmp_buf1, "0x%x", CONFIG_MEMTEST_RANGE1_START);
    mtest_argv[1] = tmp_buf1;
    sprintf(tmp_buf2, "0x%x", CONFIG_MEMTEST_RANGE1_END);
    mtest_argv[2] = tmp_buf2;
    /* setup pattern and loop count */
    pattern = 0;
    sprintf(tmp_buf3, "%d", pattern);
    mtest_argv[3] = tmp_buf3;
    iter = 2;
    sprintf(tmp_buf4, "%d", iter);
    mtest_argv[4] = tmp_buf4;
    mtest_argc = 5;

    ret = do_mem_mtest(NULL, 0, mtest_argc, mtest_argv);
    printf("\n");

    if (ret || EX62XX_LC) {
	return (ret != 0);
    }

    sprintf(tmp_buf1, "0x%x", CONFIG_MEMTEST_RANGE2_START);
    mtest_argv[1] = tmp_buf1;
    sprintf(tmp_buf2, "0x%x", CONFIG_MEMTEST_RANGE2_END);
    mtest_argv[2] = tmp_buf2;
    pattern = 0;
    sprintf(tmp_buf3, "%d", pattern);
    mtest_argv[3] = tmp_buf3; /* pattern */
    iter = 2;
    sprintf(tmp_buf4, "%d", iter);
    mtest_argv[4] = tmp_buf4;
    mtest_argc = 5;

    ret = do_mem_mtest(NULL, 0, mtest_argc, mtest_argv);
    printf("\n");

    return (ret != 0);
}

int mfgtest_usb(void)
{
    int32_t block, blkcount;
    int32_t disk;
    volatile uint32_t *buf1, *buf2;
    block_dev_desc_t *stor_dev;
    unsigned long n;

    if (EX62XX_LC) {
	/* not supported */
	return -1;
    }

    buf1 = (uint32_t *)CONFIG_USB_TEST_BUF1;
    block = 0;
    blkcount = CONFIG_USB_TEST_NUMBLKS;
    /* test the internal disk */
    disk = 0;

    stor_dev = usb_stor_get_dev(disk);
    printf("Testing access to USB device:\n");
    dev_print(stor_dev);
    printf("\n");

    memset(buf1, 0xff, 512 * blkcount);
    /* Read USB disk */
    n = stor_dev->block_read(disk, block, blkcount, (ulong *)buf1);
    if (n == blkcount) {
	printf("reading %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		n, "OK", disk, block, blkcount);
    } else {
	printf("reading %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		n, "ERROR", disk, block, blkcount);
	/* Error reading USB */
	return 1;
    }

    buf2 = (uint32_t *)CONFIG_USB_TEST_BUF2;
    memset(buf2, 0xff, 512 * blkcount);
    /* Read the USB disk again */
    n = stor_dev->block_read(disk, block, blkcount, (ulong *)buf2);
    if (n == blkcount) {
	printf("\nreading %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		n, "OK", disk, block, blkcount);
    } else {
	printf("\nreading %ld blocks: %s - dev[%d] blk[%d] cnt[%d]\n",
		n, "ERROR", disk, block, blkcount);
	/* Error reading USB */
	return 1;
    }

    /* compare the buffers */
    if (memcmp(buf1, buf2, (512 * blkcount))) {
	return 1;
    }
    return 0;
}

int mfgtest_wdog(void)
{
    uint16_t val_t;

    /* setup btcpld timer */
    btcpld_read(BOOT_CONTROL, &val_t);
    val_t &= ~BOOT_WDT_EN;
    btcpld_write(BOOT_CONTROL, val_t);
    /* configure timeout value */
    val_t = CONFIG_MFGTEST_WD_TO;
    btcpld_write(BOOT_WD_TIMER_HI, (val_t >> 8) & 0xff);
    btcpld_write(BOOT_WD_TIMER_LOW, val_t & 0xff);
    /* enable watchdog timer */
    btcpld_read(BOOT_CONTROL, &val_t);
    val_t |= BOOT_WDT_EN;
    btcpld_write(BOOT_CONTROL, val_t);

    printf("Testing the functionality of CPU watchdog timer in the CPLD.\n");
    printf(" This test will RESET the CPU within 5 seconds and\n");
    printf(" control will be back to the U-Boot prompt\n\n");

    /* wait... */
    udelay(8 * 1000 * 1000);	/* 8 secs */

    /* should not reach here.. test failed! */
    return 1;
}

struct mfgtest_list_t mfgtest_list[] = 
{
    {
	"memory",
	"Memory test",
	&mfgtest_memory
    },
    {
	"usb",
	"USB test",
	&mfgtest_usb
    },
    {
	"watchdog",
	"watchdog functionality test",
	&mfgtest_wdog
    },
};
uint32_t mfgtest_listsize = sizeof(mfgtest_list) / sizeof(struct mfgtest_list_t);

int do_mfgtest(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int loop, run_all = 0;
    int ret = 0;

    if (argc == 1) {
	/* list all the available tests */
	printf("List of tests:\n");
	for (loop = 0; loop < mfgtest_listsize; loop++) {
	    printf("  %s\n", mfgtest_list[loop].name);
	}
    } else {
	if (strcmp(argv[1], "all") == 0) {
	    /* run all the tests */
	    run_all = 1;
	}
	for (loop = 0; loop < mfgtest_listsize; loop++) {
	    if (run_all || (strcmp(argv[1], mfgtest_list[loop].name) == 0)) {
		printf("\n=================================================\n");
		printf("Performing %s:\n\n", mfgtest_list[loop].test_desc);
		ret = mfgtest_list[loop].test_func();
		if (ret == 0) {
		    printf("\ntest PASSED\n");
		} else if (ret == -1) {
		    printf("\ntest invalid on this platform\n");
		} else {
		    printf("\ntest FAILED\n");
		}
		if (!run_all)
		    break;
	    }
	}
	if ((loop == mfgtest_listsize) && !run_all) {
	    printf("\nTest not supported.\nUse 'mfgtest' to print the list of available tests.\n");
	}
    }
    return 0;
}

U_BOOT_CMD(
	mfgtest, 2, 0, do_mfgtest,
	"perform board diagnostics",
	     "    - print list of available tests\n"
	"mfgtest [test]\n"
	"         - run specified tests\n"
	"mfgtest all - run all available tests\n"
);
