/*
 * Copyright (c) 2008-2011, Juniper Networks, Inc.
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
 */

/*
 * MDK functions
 *
 */

#include <common.h>
#include <command.h>
#include <configs/jsrxnle.h>
#include <cvmx.h>
#include <cvmx-csr-enums.h>
#include <cvmx-csr-typedefs.h>
#include <cvmx-csr-addresses.h>
#include <cvmx-helper.h>
#include <cvmx-helper-sgmii.h>

static uint32_t bcm_mdk_dram_start_addr = CFG_MDK_DRAM_START_ADDR;
static uint32_t bcm_mdk_sz = CFG_USHELL_SIZE;
extern int loader_init;

static int
run_mdk (void)
{
    int rc;
    rc = ((int (*)(int, char *[]))bcm_mdk_dram_start_addr)(0, NULL);

    return rc;
}

static int 
check_magic_key (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t bcm_mdk_boot_start_addr;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX240_MODELS
        bcm_mdk_boot_start_addr = CFG_4MB_USHELL_START;
        break;
    CASE_ALL_SRX650_MODELS
    CASE_ALL_SRX550_MODELS
        bcm_mdk_boot_start_addr = CFG_8MB_USHELL_START;
        break;
    default:
        return -1;
    }

    if ((*(uint32_t *)(bcm_mdk_boot_start_addr) & 0xffff0000)
            != CFG_MDK_MAGIC_KEY) {
        return -1;
    }
    return 0;
}


/* 
 * copies ushell from bootflash to DRAM
 */
void
copy_mdk(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    int loop;
    uint32_t bcm_mdk_boot_start_addr;
    /* 
     * Check magic key of MDK application,If it doesn't
     * exist then don't initialize the switch
     */
    if (check_magic_key() < 0) {
        printf("Switch driver image not programmed properly in bootflash\n");
        printf("Expected 0x%x, actual 0x%x\n", 
                CFG_MDK_MAGIC_KEY, *(uint32_t *)(bcm_mdk_boot_start_addr));
        return;
    }
    
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX240_MODELS
        bcm_mdk_boot_start_addr = CFG_4MB_USHELL_START;
        break;
    CASE_ALL_SRX650_MODELS
    case I2C_ID_SRX550_CHASSIS:
        bcm_mdk_boot_start_addr = CFG_8MB_USHELL_START;
        break; 
    case I2C_ID_SRX550M_CHASSIS:
        bcm_mdk_boot_start_addr = CFG_8MB_USHELL_START;
        bcm_mdk_sz = CFG_550M_USHELL_SIZE;
        break;
    default:
        return;
    }

    for (loop = 0; loop < bcm_mdk_sz/4; ++loop) {
        memcpy((void *)(bcm_mdk_dram_start_addr + (loop * 4)),
           (void *)(bcm_mdk_boot_start_addr + (loop * 4)),
           sizeof (uint32_t));
    }

}

int
mdk_init (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    int rc;
    int rcode = 0;
    int loop;
    int iface;
    uint32_t bcm_mdk_boot_start_addr;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX240_MODELS
        bcm_mdk_boot_start_addr = CFG_4MB_USHELL_START;
        iface = CVMX_SRX240_IFACE;
        break;
    CASE_ALL_SRX650_MODELS
        bcm_mdk_boot_start_addr = CFG_8MB_USHELL_START;
        iface = CVMX_SRX650_IFACE;
        break;
    case I2C_ID_SRX550_CHASSIS:
        bcm_mdk_boot_start_addr = CFG_8MB_USHELL_START;
        iface = CVMX_SRX550_IFACE;
        break;
    case I2C_ID_SRX550M_CHASSIS:
        bcm_mdk_boot_start_addr = CFG_8MB_USHELL_START;
        bcm_mdk_sz = CFG_550M_USHELL_SIZE;
        iface      = CVMX_SRX550_IFACE;
        break;
    default:
        return 0;
    }

    if (cvmx_get_iface_speed(iface) == CVMX_INTERFACE_SPEED_XGMII_3125M) {
        setenv("uplinkSpeed", CVMX_INTERFACE_SPEED_XGMII_3125M_STR);
    } else if (cvmx_get_iface_speed(iface) == CVMX_INTERFACE_SPEED_XAUI) {
        setenv("uplinkSpeed", CVMX_INTERFACE_SPEED_XAUI_STR);
    } else {
        setenv("uplinkSpeed", CVMX_INTERFACE_SPEED_XGMII_1000M_STR);
    }

    /* Check magic key in MDK application. If it doesn't
     * exist, then don't initialize the switch
     */
    if (check_magic_key() < 0) {
        printf("Switch driver image not programmed properly in bootflash\n");
        printf("Expected 0x%x, actual 0x%x\n", 
                CFG_MDK_MAGIC_KEY, *(uint32_t *)(bcm_mdk_boot_start_addr));
        return 0;
    }


    for (loop = 0; loop < bcm_mdk_sz/4; ++loop) {
        memcpy((void *)(bcm_mdk_dram_start_addr + (loop * 4)),
           (void *)(bcm_mdk_boot_start_addr + (loop * 4)),
           sizeof (uint32_t));
    }

    /*
     * pass address parameter as argv[0] (aka command name),
     * and all remaining args
     */
    rc = run_mdk();
    if (rc != 0) rcode = 1;

    /*
     * pass address parameter as argv[0] (aka command name),
     * and all remaining args
     */
    rc = run_mdk();
    if (rc != 0) rcode = 1;

    if (rcode) {
        printf("Error: Broadcom Mini-Driver Kit Initialization failed\n");
    }

    return rcode;
}

void 
do_link_update (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    int rc;
    char *s = NULL;

    /* If mdk_init not happened then don't do link update */
    if (!ethact_init) {
        return;
    }

    /* Do link update for SRX240 boards. This is needed to update
     * internal phy and MAC with autoneg results. The same is not
     * required for SRX650 as there we only use external PHY as we
     * have sysio BCM in between. Internal PHY and MAC of base board
     * remains at 1G there
     */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX240_MODELS
        if (check_magic_key() < 0)
            return;

        if (loader_init == SRX240_LOADER_INIT_COMPLETE) {
            return;
        }
        /* Process link update on BCM ushell */
        setenv("linkUpdate", "1");

        /* 
         * when tftp is going on and if tftp server is killed, u-boot tries
         * to re-init the mdk and if tftp has transfered more than 1MB 
         * it would have overwritten ushell residing at 0x80200000, so 
         * verify ushell in DRAM before calling run_mdk()
         */
        if ((*(uint32_t *)(bcm_mdk_dram_start_addr)& 0xffff0000) 
                != CFG_MDK_MAGIC_KEY) {
            copy_mdk();
        }

        (void)run_mdk();
        setenv("linkUpdate", "0");
        break;
    CASE_ALL_SRX550_MODELS
        if (check_magic_key() < 0)
            return;

        if (loader_init == SRX240_LOADER_INIT_COMPLETE) {
            return;
        }
        /* Process link update on BCM ushell */
        setenv("linkUpdate", "1");

        /* 
         * when tftp is going on and if tftp server is killed, u-boot tries
         * to re-init the mdk and if tftp has transfered more than 1MB 
         * it would have overwritten ushell residing at 0x80200000, so 
         * verify ushell in DRAM before calling run_mdk()
         */
        if ((*(uint32_t *)(bcm_mdk_dram_start_addr)& 0xffff0000) 
                != CFG_MDK_MAGIC_KEY) {
            copy_mdk();
            /* 
             * A second call to run mdk() ensures that the link update of the 
             * front/uplink port happens.
             * This is required when ushell is loaded again before tftp install
             */
            (void)run_mdk();
        }

        (void)run_mdk();
        setenv("linkUpdate", "0");
        break;
    CASE_ALL_SRX650_MODELS

        if (check_magic_key() < 0)
            return;

        /* Process link update on BCM ushell */
        setenv("linkUpdate", "1");
        
        /*
         * when tftp is going on and if tftp server is killed, u-boot tries
         * to re-init the mdk and if tftp has transfered more than 1MB 
         * it would have overwritten ushell residing at 0x80200000, so 
         * verify ushell in DRAM before calling run_mdk()
         */
        if ((*(uint32_t *)(bcm_mdk_dram_start_addr) & 0xffff0000) 
                != CFG_MDK_MAGIC_KEY) {
            copy_mdk();
            /* 
             * A second call to run mdk() ensures that the link update of the 
             * front/uplink port happens.
             * This is required when ushell is loaded again before tftp install
             */
            (void)run_mdk();
        }
        (void)run_mdk();
        setenv("linkUpdate", "0");
        break;
    default:
        break;
    }
}

int 
do_mdk_init (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int     rcode = 0;

    rcode = mdk_init();

    return rcode;
}

U_BOOT_CMD(
        mdkinit, 2, 1,	do_mdk_init,
        "mdkinit      - start MDK\n",
        "mdkinit      - start MDK\n"
        );

