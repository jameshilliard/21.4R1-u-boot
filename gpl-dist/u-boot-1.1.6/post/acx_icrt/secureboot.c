/*
 * $Id$
 *
 * secureboot.c -- Diagnostics for the Secure Boot
 * functionality in iCRT
 *
 * Amit Verma, March 2014
 *
 * Copyright (c) 2014, Juniper Networks, Inc.
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
#include <post.h>
#include "acx_icrt_post.h"
#include <acx_syspld.h>
#include <malloc.h>

#if CONFIG_POST & CFG_POST_SECUREBOOT

/*
 * helpful #defines
 */
/* nor flash */
#define SB_NOR_WP_NONE		0
#define SB_NOR_WP_PARAMS	1
#define SB_NOR_WP_LOADER	2
#define SB_NOR_WP_UBOOT		4
#define SB_NOR_WP_ICRT		8
/* nvram */
#define SB_NVRAM_WP_DISABLE	0
#define SB_NVRAM_WP_ENABLE	1
/* TPM PP pin */
#define SB_TPM_PP_CLEAR		0
#define SB_TPM_PP_SET		1
/* BootCPLD update */
/* slow here, ENABLE = 0, DISABLE = 1 */
#define SB_CPLD_UPDATE_ENABLE	0
#define SB_CPLD_UPDATE_DISABLE	1
/* UART control */
#define SB_UART_TX_DISABLE	0
#define SB_UART_TX_ENABLE	1
#define SB_UART_RX_DISABLE	0
#define SB_UART_RX_ENABLE	1
#define SB_ENTRY_TIME		15

/*
 * helpful structure for NOR flash layout in the iCRT world
 */
/*
 * Kotinos uses a uniform sector nor flash
 * Spansion S29GL064N90TFI010 TSOP56 8MB 107200000064A
 *
 * DEVICE NUMBER/DESCRIPTION
 * S29GL064N, 64 Megabit Page-Mode Flash Memory
 *
 * SPEED OPTION
 * 90 = 90 ns
 *
 * PACKAGE TYPE
 * T = Thin Small Outline Package (TSOP) Standard Pinout
 *
 * PACKAGE MATERIAL SET
 * F = Pb-Free
 *
 * MODEL NUMBER
 * 01 = x8/x16, VCC = VIO = 2.7 . 3.6 V, Uniform sector, WP#/ACC = VIL
 *	protects highest addressed sector
 *
 * PACKING TYPE
 * 0 = Tray
 */
/* 64k is the sector size */
#define SB_NOR_SECT_SIZE	(64 * 1024)
struct FlashControl {
	ulong	start;
	uint8_t	wp;
} secureboot_nor_region[] = {
	/* Security parameter block */
	{ 0xFFC00000, 0x01 },
	/* Loader */
	{ 0xFFD00000, 0x02 },
	/* U-boot + boot env. variables */
	{ 0xFFE00000, 0x04 },
	/* iCRT */
	{ 0xFFF00000, 0x08 }
};
static uint8_t *secureboot_nor_buffer;

/* logging help */
#define SB_POST_LOG(x,y) \
	POST_LOG(POST_LOG_IMP, "================ SECUREBOOT " x " TEST " y " ==================\n")

/*
 * This *manual* test checks for the push button on the chassis
 * to be working fine.
 */
int secureboot_pb_post_test(int flags)
{
	SB_POST_LOG("PUSH BUTTON", "START");

	if (syspld_push_button_detect()) {
		POST_LOG(POST_LOG_INFO, "Push button is pressed.\n");
	} else {
		POST_LOG(POST_LOG_INFO, "Push button is not pressed.\n");
	}

	SB_POST_LOG("PUSH BUTTON", "END");
	return 0;
}

/*
 * This *manual* test checks for the recovery jumper on the chassis
 * to be working fine.
 */
int secureboot_recovery_jumper_post_test(int flags)
{
	SB_POST_LOG("RECOVERY JUMPER", "START");

	if (syspld_jumper_detect()) {
		POST_LOG(POST_LOG_INFO, "Recovery Jumper is installed.\n");
	} else {
		POST_LOG(POST_LOG_INFO, "Recovery Jumper is not installed.\n");
	}

	SB_POST_LOG("RECOVERY JUMPER", "END");
	return 0;
}

// #define SECUREBOOT_POST_DEBUG 1

/*
 * This test is for checking the write protection feature
 * in nor flash
 *
 * This needs to be run from iCRT as well as u-boot.
 *
 */
int secureboot_nor_wp_post_test(int flags)
{
	int status = -1; /* failure */
	int i = 0; /* regions of the nor flash */
	uint8_t * buff ;
	int count = 0;
	int retval = 0;
	uint8_t saved_gcr;

	/* get the buffer going */
	if (!secureboot_nor_buffer) {
		secureboot_nor_buffer = (uint8_t*) malloc(SB_NOR_SECT_SIZE);
	}
	if (!secureboot_nor_buffer) {
		POST_LOG(POST_LOG_ERR,
			 "Can not allocate buffer for use. Exiting.\n");
	}

	/* save gcr */
	saved_gcr = syspld_get_gcr();

	status = 0; /* success to begin with */
	SB_POST_LOG("NOR WRITE PROTECT", "START");
	for (i = 0 ; i < sizeof(secureboot_nor_region) /
	     sizeof (secureboot_nor_region[0]) ; i++) {
		POST_LOG(POST_LOG_INFO, "************ Testing region %2d *********\n", i);
		if (!syspld_nor_flash_wp_set(secureboot_nor_region[i].wp)) {
			POST_LOG(POST_LOG_ERR,
				 "(%d): s=0x%0x sz=%d wp=0x%0x. Failed.\n",
				 i,
				 secureboot_nor_region[i].start,
				 SB_NOR_SECT_SIZE,
				 secureboot_nor_region[i].wp);
			status--;
			/* Attempt to check the next region */
			continue;
		}
		udelay(100*1000);
		/* read what is in the first sector of this region */
		buff = (uint8_t*) secureboot_nor_region[i].start;
		for (count = 0; count < SB_NOR_SECT_SIZE ; count++) {
			/* invert the bits on the fly */
			secureboot_nor_buffer[count] = 
				buff[count] ^ 0xff;
#ifdef SECUREBOOT_POST_DEBUG
			if (count < 4) {
				printf("{0x%0x => 0x%0x} ",
				       buff[count], secureboot_nor_buffer[count]);
			}
#endif /* SECUREBOOT_POST_DEBUG */
		}
		udelay(10*1000);
		/* write back */
			/* erase first */
		retval = flash_sect_erase(secureboot_nor_region[i].start,
					  (secureboot_nor_region[i].start +
					   SB_NOR_SECT_SIZE - 1));
		if (retval) {
			POST_LOG(POST_LOG_ERR,
				 "Flash sector erase failed(%d)."
				 " (%d, 0x%0x, 0x%0x)\n",
				 retval,
				 i,
				 secureboot_nor_region[i].start,
				 (secureboot_nor_region[i].start +
				  SB_NOR_SECT_SIZE - 1));
			status--;
			continue;
		}
		udelay(1000*1000);
		/* write later */
		retval = flash_write((char*)secureboot_nor_buffer,
				     secureboot_nor_region[i].start,
				     SB_NOR_SECT_SIZE);
#ifdef SECUREBOOT_POST_DEBUG
		if (retval) {
			POST_LOG(POST_LOG_ERR,
				 "Flash sector write failed (%d). But we will continue..."
				 " (%d, 0x%0x, 0x%0x)\n",
				 retval,
				 i,
				 secureboot_nor_region[i].start,
				 (secureboot_nor_region[i].start +
				  SB_NOR_SECT_SIZE - 1));
		}
#endif /* SECUREBOOT_POST_DEBUG */
		udelay(100*1000);
		/* don't check the write status for testing the diag result */
		/* read the first block of this region again */
		buff = (uint8_t*) secureboot_nor_region[i].start;
		for (count = 0; count < SB_NOR_SECT_SIZE; count++) {
#ifdef SECUREBOOT_POST_DEBUG
			if (count < 4) {
				printf("{0x%0x v/s 0x%0x} ",
				       secureboot_nor_buffer[count],
				       buff[count]);
			}
			if (count == 4) {
				printf("\n");
			}
#endif /* SECUREBOOT_POST_DEBUG */

			if (secureboot_nor_buffer[count] == buff[count]) {
				/* we found a match, i.e. write succeeded */
#ifdef SECUREBOOT_POST_DEBUG
				printf("\n");
#endif /* SECUREBOOT_POST_DEBUG */
				break;
			}
		}
		/* check if the bits were written inverted */
		/* if the write was successful, you fail the test */
		if (count == SB_NOR_SECT_SIZE) {
			POST_LOG(POST_LOG_INFO,
				 "Pass : Write protection worked for region."
				 " (%d, 0x%0x, 0x%0x).\n",
				 i,
				 secureboot_nor_region[i].start,
				 (secureboot_nor_region[i].start +
				  SB_NOR_SECT_SIZE - 1));
		} else {
			status --;
			POST_LOG(POST_LOG_ERR,
				 "Fail : Write protection did not work for region."
				 " count=%d, region=%d buf=0x%0x, nor=0x%0x\n",
				 count,
				 i,
				 secureboot_nor_buffer[count],
				 buff[count]);
			/* restore the original data in nor flash */
			for (count = 0 ; count < SB_NOR_SECT_SIZE ; count++) {
				secureboot_nor_buffer[count] ^= 0xff;
#ifdef SECUREBOOT_POST_DEBUG
				if (count < 4) {
					printf("{Restore 0x%0x} ",
					       secureboot_nor_buffer[count]);
				}
				if (count == 4) {
					printf("\n");
				}
#endif /* SECUREBOOT_POST_DEBUG */
			}
			/* erase the flash sector */
			retval = flash_sect_erase(secureboot_nor_region[i].start,
						  (secureboot_nor_region[i].start +
						   SB_NOR_SECT_SIZE - 1));
			if (retval) {
				POST_LOG(POST_LOG_ERR,
					 "Sector erase failed during restore (%d)."
					 " (%d, 0x%0x, 0x%0x)\n",
					 retval,
					 i,
					 secureboot_nor_buffer,
					 secureboot_nor_region[i].start);
				status--;
			}
			retval = flash_write((char*)secureboot_nor_buffer,
					     secureboot_nor_region[i].start,
					     SB_NOR_SECT_SIZE);
			if (retval) {
				POST_LOG(POST_LOG_ERR,
					 "Sector write failed during restore. (%d)"
					 " (%d, 0x%0x, 0x%0x)\n",
					 retval,
					 i,
					 secureboot_nor_buffer,
					 secureboot_nor_region[i].start);
				status--;
			}
		}
		/* For the test to pass, all the four regions should pass */
	}

	if (status) {
		POST_LOG(POST_LOG_ERR,
			 "NOR flash write protection test failed.\n");
	} else {
		POST_LOG(POST_LOG_INFO,
			 "NOR flash write protection test passed.\n");
	}

	SB_POST_LOG("NOR WRITE PROTECT", "END");

	/* restore gcr */
	syspld_set_gcr(saved_gcr);

	return status;
}

/*
 * This *manual* test is for testing the swizzle functionality of the
 * immutable CRT to the CRT
 */
int secureboot_sreset_post_test(int flags)
{
	unsigned long long base;
	int secs, ints;
	ulong clk;

	SB_POST_LOG("CPU ONLY SOFT RESET", "START");

	if (nvram_get_uboot_sreset_test() == SRESET_TEST_IN_PROGRESS) {
		nvram_set_uboot_sreset_test(0);

		if (syspld_cpu_only_reset()) {
			POST_LOG(POST_LOG_INFO,
				 "Soft reset test passed.\n");
			POST_LOG(POST_LOG_INFO,
				 "... Kindly powercycle the board now.\n");
			return 0;
		} else {
			POST_LOG(POST_LOG_ERR,
				 "Soft reset test failed.\n");
			POST_LOG(POST_LOG_INFO,
				 "... Kindly powercycle the board now.\n");
			return -1;
		}
	} else {
		nvram_set_uboot_sreset_test(SRESET_TEST_IN_PROGRESS);
		POST_LOG(POST_LOG_INFO,
			 "To test the functionality of CPU only soft reset\n");
		POST_LOG(POST_LOG_INFO,
			 "Test tell will cause a CPU only softreset by asserting"
			 " the syspld bit. This should cause the swizzle to the"
			 " CRT code and you should see the CRT code starting"
			 " up again after reset.\n");
		POST_LOG(POST_LOG_INFO,
			 "Once the CRT code has started up, run this test again to"
			 " check if the test succeeded or not.\n");

		/* wait for 5 seconds */
		clk = get_tbclk();
		base = get_ticks();
		ints = disable_interrupts();

		for (secs = 1; secs < 10 ; secs++) {
			while (((unsigned long long)get_ticks() - base) / secs < clk)
				;
			if (secs == 4) {
				POST_LOG(POST_LOG_INFO,
					 "Going to assert cpu only reset now.\n");
			}
			if (secs == 5) {
				/* assert srest */
				syspld_assert_sreset();
			}
			POST_LOG(POST_LOG_INFO, "%d..\n", secs);
		}
		/* if we are still alive, it is not good */
		
		/* enable interrupts */
		if (ints) {
			enable_interrupts();
		}

		/* tell we failed */
		POST_LOG(POST_LOG_INFO,
			 "The CPU only reset failed. Test failed.\n");
		nvram_set_uboot_sreset_test(0);
	}
		
	SB_POST_LOG("CPU ONLY SOFT RESET", "END");

	return -1;
}
/*
 * This test is for checking the write protection of the NVRAM
 */
int secureboot_nvram_wp_post_test(int flags)
{
	int status = 0; /* failure */
	uint32_t val;

	SB_POST_LOG("NVRAM WRITE PROTECT", "START");
	/* piggyback on sreset test nvram field */
	/* disable nvram wp */
	if (!syspld_nvram_wp(SB_NVRAM_WP_DISABLE)) {
		POST_LOG(POST_LOG_ERR,
			 "Failed to disable nvram write protection.\n");
		status--;
	}
	val = nvram_get_uboot_sreset_test();
	val ^= (unsigned int)0xffffffff;
	nvram_set_uboot_sreset_test(val);
	if (val == nvram_get_uboot_sreset_test()) {
		/* good, till now the nvram is writable */
		POST_LOG(POST_LOG_INFO,
			 "NVRAM looks good to begin with.\n");
	} else {
		/* bad, nvram could not be modified */
		POST_LOG(POST_LOG_ERR,
			 "NVRAM write failed.\n");
		status--;
	}

	/* get old value */
	val = nvram_get_uboot_sreset_test();
	POST_LOG(POST_LOG_INFO,
		 "%s : nvram has this value (0x%0x)\n",
		 __FUNCTION__,
		 val);
	/* enable nvram wp */
	if (!syspld_nvram_wp(SB_NVRAM_WP_ENABLE)) {
		POST_LOG(POST_LOG_ERR,
			 "Failed to enable nvram write protection.\n");
		status--;
	}
	/* write new value */
	val ^= (unsigned int)0xffffffff;
	nvram_set_uboot_sreset_test(val);
	/* check the value in nvram */
	if (val != nvram_get_uboot_sreset_test()) {
		/* good, nvram wp success */
		POST_LOG(POST_LOG_INFO, "NVRAM wp feature works.\n");
	} else {
		/* bad, nvram was modified after being wp */
		POST_LOG(POST_LOG_ERR, "NVRAM wp feature failed.\n");
		status--;
	}

	/*
	 * Once nvram wp is enabled, it can not be changed back to disabled
	 * state until next power cycle when all the bits are un-locked for
	 * modification
	 */
	val = nvram_get_uboot_sreset_test();
	POST_LOG(POST_LOG_INFO,
		 "%s : nvram has this value (0x%0x)\n",
		 __FUNCTION__,
		 val);
	SB_POST_LOG("NVRAM WRITE PROTECT", "END");

	return status;
}

/*
 * This *manual* test is to check the UART transmit/receive disable
 */
int secureboot_uart_post_test(int flags)
{
	int i;
	int delay;
	int count;
	char c;

	SB_POST_LOG("UART TRANSMIT/RECEIVE DISABLE", "START");

#if 0
	POST_LOG(POST_LOG_INFO,
		 "We are going to output A-Z. If you see all of the characters\n"
		 " the test is a failure.\n");
	POST_LOG(POST_LOG_INFO,
		 "------------------------------------------------------------\n");
	/* disable tx */
	syspld_uart_tx_control(SB_UART_TX_DISABLE);
	/* push letters of alphabet A-Z */
	for (i = 0 ; i < 26 ; i++) {
		serial_putc_raw('A' + i);
	}
	/* enable tx */
	syspld_uart_tx_control(SB_UART_TX_ENABLE);
	/* flush console */
	serial_putc('\n');
	POST_LOG(POST_LOG_INFO,
		 "------------------------------------------------------------\n");
	/* Only the last 16 characters output should be shown */
#endif

	POST_LOG(POST_LOG_INFO,
		 "In the next 15 seconds, enter some characters on the line\n"
		 "We'll count it. Since receive is disabled we should not\n"
		 " receive any characters.\n");
	/* disable rx */
	syspld_uart_rx_control(SB_UART_RX_DISABLE);
	count = 0;
	/* ask for user entry of a string in next 15 seconds */
	for (delay = 0 ; delay < SB_ENTRY_TIME ; delay++) {
		for (i = 0 ; i < 100 ; i++) {
			if (serial_tstc()) {
				c = serial_getc();
				if (c) {
					count++;
				POST_LOG(POST_LOG_INFO,
					 "%s: received (0x%0x)\n",
					 __FUNCTION__,
					 c);
				}
			}
			udelay(10 * 1000);
		}
	}
	/* see how many characters got entered */
	/* output how many character got entered */
	POST_LOG(POST_LOG_INFO,
		 "Received %d characters on the line.\n",
		 count);
	if (count) {
		POST_LOG(POST_LOG_ERR,
			 "Receive test failed.\n");
	} else {
		POST_LOG(POST_LOG_INFO,
			 "Receive test passed.\n");
	}
	
	/* enable tx & rx */
	syspld_uart_rx_control(SB_UART_RX_ENABLE);
	syspld_uart_tx_control(SB_UART_TX_ENABLE);

	SB_POST_LOG("UART TRANSMIT/RECEIVE DISABLE", "END");

	return (count? -1 : 0);
}

/*
 * This test is to check that the glass safe is really locked
 */
int secureboot_gcr_post_test(int flags)
{
	int		status = -1; /* failure */

	SB_POST_LOG("GLASS SAFE CONTROL", "START");

	if (!syspld_is_glass_safe_open()) {
		POST_LOG(POST_LOG_IMP,
			 "Glass safe is not open. Test Failed\n");
		return status;
	}
	POST_LOG(POST_LOG_INFO, "Glass safe is open. Proceed...\n");

	/* set gcr to reset values */
	syspld_set_gcr(0);

	/* check if the gcr is really clear */
	if (syspld_get_gcr()) {
		POST_LOG(POST_LOG_ERR,
			 "Could not clear glass safe register."
			 " Test failed.\n");
		return status;
	}
	POST_LOG(POST_LOG_INFO, "Cleared glass safe register. Proceed...\n");

	/* close the glass safe */
	if (!syspld_close_glass_safe()) {
		POST_LOG(POST_LOG_ERR,
			 "Could not close glass safe. Test failed.\n");
		return status;
	}
	POST_LOG(POST_LOG_INFO, "Glass safe closed. Proceed...\n");

	/* if the below operations are okay we pass */
	status = 0;

	/* modify nor flash write protect */
	if (syspld_nor_flash_wp_set(SB_NOR_WP_PARAMS)) {
		POST_LOG(POST_LOG_ERR,
			 "Able to change nor flash write protection bits."
			 " Failed.\n");
		status--;
	}
	/* modify nvram write protect */
	if (syspld_nvram_wp(SB_NVRAM_WP_ENABLE)) {
		POST_LOG(POST_LOG_ERR,
			 "Able to change nvram write protection bits."
			 " Failed.\n");
		status--;
	}
	/* modify tpm pp pin control */
	if (syspld_tpm_pp_control(SB_TPM_PP_SET)) {
		POST_LOG(POST_LOG_ERR,
			 "Able to change PP pin control bit. Failed.\n");
		status--;
	}
	/* modify bootcpld update */
	if (syspld_cpld_update(SB_CPLD_UPDATE_DISABLE)) {
		POST_LOG(POST_LOG_ERR,
			 "Able to change cpld update bit. Failed.\n");
		status--;
	}

	if (status) {
		POST_LOG(POST_LOG_ERR,
			 "Glass Safe Control Test Failed. (%d)\n",
			 status);
	} else {
		POST_LOG(POST_LOG_INFO, "Glass Safe Control Test Passed.\n");
	}
	SB_POST_LOG("GLASS SAFE CONTROL", "END");
	return status;
}

/************************* static functions ***************/
#endif /* CONFIG_POST & CFG_POST_SECUREBOOT */
