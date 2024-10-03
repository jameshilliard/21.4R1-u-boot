/*
 * Copyright (c) 2009-2013, Juniper Networks, Inc.
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

#include <command.h>
#include <rmi/pioctrl.h>
#include <common/fx_common.h>
#include <common/fx_cpld.h>
#include <common/eeprom.h>
#include <configs/fx.h>
#include "rmi/pioctrl.h"
#include <common/post.h>
#include "fx_cpld.h"
#include "platform_defs.h"
#include <soho/soho_board.h>
#include <common/i2c.h>
#include <common/fx_gpio.h>
#include <soho/tor_cpld_i2c.h>

extern int i2c_eeprom_read(uint index, int offset, char *get_buffer, int nbytes);
static uint8_t eeprom_read_major_rev(void);

unsigned char     eeprom_mac_addr[6];

uint8_t fx_is_cb (void);
uint8_t g_board_ver;

/*
 * Function Name: init_board_type
 * Arguments: rmi_board_info
 * Return: None
 * Description: This is called at the start of boot2 from
 * rmi_early_board_init. It should be used to set any board specific 
 * values that can later be used. 
 */

void init_board_type (struct rmi_board_info * rmib)
{
    fx_get_board_type();

    fx_piobus_setup(rmib);
    fx_tweak_pio_cs(rmib);
    fx_init_piobus(rmib);
    fx_init_piobus_devices();
}

void set_board_type (void)
{
    char *s;
    char *post_verbose = NULL;
    char temp[20];
    uint8_t buf = 0;

    sprintf(temp,"%04x", g_board_type);
    setenv("hw.board.type", temp);
    sprintf(temp, "%x", g_board_ver);
    setenv("hw.board.rev", temp); 

    if (fx_is_cb()) {
        setenv("hw.pci.enable_static_config", "1");
    } else if (fx_is_soho()) {
        setenv("hw.pci.enable_static_config", "1");
    } else {

           setenv("hw.pci.enable_static_config", "0");
    }

    sprintf(temp,"0x%08x", CFG_FLASH_BASE);
    setenv("flash_base_addr", temp);

    sprintf(temp,"0x%06x", CFG_FLASH_SIZE);
    setenv("flash_size", temp);
    
    if (fx_cpld_get_current_partition()) {
        sprintf(temp,"0x%08x", CFG_FLASH_BASE_PARTITION0);
        setenv("boot.upgrade.uboot", temp);
    
        sprintf(temp,"0x%08x", CFG_UPG_LOADER_PARTITION0_ADDR);
        setenv("boot.upgrade.loader", temp);

        setenv("bootcmd", CONFIG_BOOTCOMMAND_PARTITION1);
    } else {	
        sprintf(temp,"0x%08x", CFG_FLASH_BASE_PARTITION1);
        setenv("boot.upgrade.uboot", temp);

        sprintf(temp,"0x%08x", CFG_UPG_LOADER_PARTITION1_ADDR);
        setenv("boot.upgrade.loader", temp);

        setenv("bootcmd", CONFIG_BOOTCOMMAND_PARTITION0);
    }

    sprintf(temp,"%d", CFG_POST_VERBOSE);

    /* Create post_verbose env variable if not already present */

    post_verbose = getenv("post_verbose");

    if (post_verbose == NULL)
       setenv("post_verbose", temp);

    sprintf(temp,"0x%08x", CFG_ENV_ADDR);
    setenv("boot.env.start", temp);

    sprintf(temp,"0x%05x", CFG_ENV_SIZE);
    setenv("boot.env.size", temp);

    setenv("console", "comconsole");

    sprintf(temp, "%d.%d.%d",
            CFG_VERSION_MAJOR, CFG_VERSION_MINOR, CFG_VERSION_PATCH);
    setenv("boot.version", temp);
    sprintf(temp, "%d", fx_cpld_get_version());
    setenv("cpld.version", temp);

    if (fx_is_wa()) {
        sprintf(temp, "%d", eeprom_read_major_rev());
        setenv("hw.board.rev", temp);
    }
    b2_debug("\nFirmware Version: --- %02X.%02X.%02X ---\n",CFG_VERSION_MAJOR,  \
            CFG_VERSION_MINOR, CFG_VERSION_PATCH);
    b2_debug("\nCPLD image version: %02X \n", fx_cpld_get_version());

    if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR,
                 FX_BOARD_VERSION_OFFSET, &buf, 1) != 0) {

        printf("\nERROR: Unable to read the HW major version: 0x%x \n", buf);
        return 0;
    }

    sprintf(temp, "%d", buf);
    setenv("hw.version", temp);
}

static void
mac_addr_increment (uint8_t *mac_addr, uint16_t count)
{
    uint8_t     i = 0;

    if ((0xff - mac_addr[5]) < count) {
        i = 4;
        while (i > 0) {
            if (mac_addr[i] != 0xff) {
                mac_addr[i]++;
                break;
            } else {
                mac_addr[i]++;
                i--;
            }
        }
    }
    mac_addr[5] += (count);
}

static uint8_t
eeprom_read_major_rev(void)
{
    uint8_t   buf = 0, i;
    uint8_t   major_rev = 0;

    if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR,
                 FX_MAC_ADDR_MAGIC_CODE_OFFSET, &buf, 1) != 0) {
        printf("\nERROR: Unable to read the MAC MAGIC: 0x%x \n", buf);
        return 0;
    }
    if (buf == FX_MAC_ADDR_MAGIC_CODE) {
        if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR,
                     (FX_MAJOR_REV_OFFSET), &major_rev, 1) != 0) {
            printf("\nERROR: Unable to read low major Rev from EEPROM\n");
            return 0;
        }
    }
    return major_rev;
}

static uint8_t
eeprom_read_mac_addr (void)
{
    uint8_t   buf = 0, i;
    uint8_t   count_low = 0;
    uint8_t   count_high = 0;
    uint16_t  count = 0;
    char      mac_addr[20];
    uint8_t   temp_mac_addr[6];

    if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR,
                 FX_MAC_ADDR_MAGIC_CODE_OFFSET, &buf, 1) != 0) {

        printf("\nERROR: Unable to read the MAC MAGIC: 0x%x \n", buf);
        return 0;
    }

    if (buf == FX_MAC_ADDR_MAGIC_CODE) {

        if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR, 
                     FX_MAC_ADDR_BASE_OFFSET, eeprom_mac_addr, 6) != 0) {
      
            printf("\nERROR: Unable to read the MAC Address from EEPROM\n");
            return 0;
        }
        if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR,
                     (FX_MAC_ADDR_BASE_OFFSET-1), &count_low, 1) != 0) {
            printf("\nERROR: Unable to read low MAC count from EEPROM\n");
            return 0;
        }
        if (i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR,
                     (FX_MAC_ADDR_BASE_OFFSET-2), &count_high, 1) != 0) {
            printf("\nERROR: Unable to read high MAC count from EEPROM\n");
            return 0;
        }
        count = (uint16_t)((count_high << 8) | count_low);
        debug("\ncount_high: 0x%x, count_low: 0x%x, count: 0x%x\n", 
              count_high, count_low, count); 
    } else {
        b2_debug("\nNo MAC Address programmed in the EEPROM!!!!\n");
        return 0;
    }
    /*
     * The last 8 MAC addresses are assigned for the 8 GMAC ports.
     * On TORs, GMAC 4 is the debug port and on CB GMAC 1 is the
     * debug port. The ethaddr env variable is set to the debug
     * port.
     */
    if ((count !=  0x08) && (count != 0x104)) {
        /*
         * This should not happen on any board but right now
         * there are lots of proto boards that do not have
         * correct EEPROM settings. This is just to warn the 
         * user to get the MAC block re-programmed.
         */

        /*
         * TORs have 260 MACs programmed and the CBs have 8 MACs.
         * If we see any other MAC count, that is invalid.
         */    

        printf("\nWrong MAC Address Count.. Please re-program EEPROM!!\n");
        return 0;
    }

    /*
     * If  the last byte of MAC address overflows, the preceding
     * byte has to be incremented.
     */
    mac_addr_increment(&eeprom_mac_addr[0], (count - RMI_GMAC_TOTAL_PORTS));

    for (i = 0; i < 6; i++) {
        temp_mac_addr[i] = eeprom_mac_addr[i];
    }
    if (fx_is_tor()) {
        count = 4;
    } else if (fx_is_cb()) {
        count = 1;
    }
    
    mac_addr_increment(&temp_mac_addr[0], count);

    sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
            temp_mac_addr[0], temp_mac_addr[1], temp_mac_addr[2],
            temp_mac_addr[3], temp_mac_addr[4], temp_mac_addr[5]);
    
    setenv("ethaddr", mac_addr);
    return 1;
}


uint64_t
mac_strtoull (char *mac_addr)
{
    char *end;
    uint8_t i;
    uint64_t mac = 0;
    uint32_t tmp_mac = 0;

    if (mac_addr) {
        for (i = 0; i < 6; i++) {
            mac <<= 8;
            tmp_mac = mac_addr ? simple_strtoul(mac_addr, &end, 16) : 0;
            mac |= (tmp_mac & 0xff);
            if (mac_addr) {
                mac_addr = (*end) ? end + 1 : end;
            }
        }
    }
    return mac;
}


/* This function is called from the eth_init when the gmacs are first
 * initialized. The MAC address for each GMAC is assigned either from
 * the EEPROM or the enviroment or if everything fails, from the IP
 * address. */
uint64_t
get_macaddr (void)
{
    char  env_mac_addr[20];
    unsigned long ipaddr;

    if (eeprom_read_mac_addr()) {
        sprintf(env_mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                eeprom_mac_addr[0], eeprom_mac_addr[1], eeprom_mac_addr[2],
                eeprom_mac_addr[3], eeprom_mac_addr[4], eeprom_mac_addr[5]);
        return (mac_strtoull(env_mac_addr));
    }

    /*
     * Cook up the MAC Address from the IP Address: Mac =
     * 00:00:<ipaddr> + gmac_id
     */
    ipaddr = getenv_IPaddr("ipaddr");
    if (!ipaddr) {
        printf("\n Please set the ipaddr in the env... \n");
        return 0;
    }
    sprintf(env_mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
            0, 0, ((ipaddr >> 24) & 0xff), ((ipaddr >> 16) & 0xff),
            ((ipaddr >> 8) & 0xff), (ipaddr & 0xff));
    debug("\nMAC from IP: %s\n", env_mac_addr);
    setenv("ethaddr", env_mac_addr);
    return (mac_strtoull(env_mac_addr));
}
 
#define CHECK_SET_NEXT_LOADDEV(slice, value)                  \
do {                                                          \
    int num_tries = get_num_kernel_boot_tries(slice);         \
    if (num_tries > NUM_TRIES_THRESHOLD) {                    \
        setenv("loaddev", value);                             \
        debug("\nSetting loaddev to: %s\n", value);           \
    } else {                                                  \
        set_num_kernel_boot_tries(slice, (num_tries + 1));    \
    }                                                         \
} while(0)

static int
fx_check_recovery_needed (void)
{
    uint8_t num_tries1;
    uint8_t num_tries2;

    num_tries1 = get_num_kernel_boot_tries(1);
    num_tries2 = get_num_kernel_boot_tries(2);

    if ((num_tries1 > NUM_TRIES_THRESHOLD) &&
        (num_tries2 > NUM_TRIES_THRESHOLD)) {
        return 1;
    }
    return 0;
}

static void
fx_set_next_bootdev (void)
{
    char *dev;
    char *old_dev;
    char *copy_dev;

    dev = copy_dev = getenv("loaddev");
    old_dev = getenv("old_loaddev");
    
    /*
     * The loaddev should be set appropriately so that
     * the right kernel will be loaded. The external USB
     * is given priority in case one is found. If there is
     * no kernel in the external USB, we fall to the loader
     * prompt and give up booting.
     * Forget the external USB and boot from
     * the internal one if loaddev_override is set.
     */
    if ((usb_max_devs <= 1) || (getenv("loaddev_override"))) {
        if (!dev) {
            debug("\nloaddev not set... setting it to disk0\n");
            setenv("loaddev", "disk0:");
            return;
        }
        if (strncmp(dev, "disk0", 5)) {
            /*
             * If external USB is not present and loaddev is not
             * disk0, the external USB might be removed. See if there
             * was any old_loaddev saved to get to the right partition.
             */
            if (old_dev && (strncmp(old_dev, "disk0", 5) == 0)) {
                debug("\nSetting loaddev to old_loaddev: %s\n", old_dev);
                setenv("loaddev", old_dev);
                setenv("old_loaddev", "");
            } else {
                debug("\nSetting loaddev to disk0\n");
                setenv("loaddev", "disk0:");
            }
        }
        /*
         * Check if this is a Watchdog reset. If it is, check if the
         * current slice is tried and is exceeding threshold. If it
         * exceeds, set the next slice as new loaddev. If the slice
         * info is not there, default to slice 0.
         */
        if (fx_cpld_get_reset_type() == WATCHDOG_TIMER_RESET) {
            dev = getenv("loaddev");
            if (fx_check_recovery_needed()) {
                /*
                 * Both partitions does not have a bootable image. Attempt to
                 * kickstart software installation if one exists (the image
                 * will be saved as 'incoming-package.tgz' in the recovery
                 * partition.
                 */
                setenv("run_once", "install disk0s3a:///incoming-package.tgz");
                return;
            }
            if (strncmp(dev, "disk0s1", 7) == 0) {
                CHECK_SET_NEXT_LOADDEV(1, "disk0s2:");
            } else if (strncmp(dev, "disk0s2", 7) == 0) {
                CHECK_SET_NEXT_LOADDEV(2, "disk0s1:");
            } else {
                setenv("loaddev", "disk0s1:");
                set_num_kernel_boot_tries(1, 0);
                set_num_kernel_boot_tries(2, 0);   
            }
        } else {
            set_num_kernel_boot_tries(1, 0);
            set_num_kernel_boot_tries(2, 0);
        }
    } else {
        if (!dev) {
            debug("\nloaddev not set... setting it to disk1\n");
            setenv("loaddev", "disk1:");
        } else if (strncmp(dev, "disk1", 5)) {
            debug("\nSaving old_loaddev: %s\n", dev);
            setenv("old_loaddev", dev);
            debug("\nSetting loaddev to disk1\n");
            setenv("loaddev", "disk1:");
        }
    }

    /* check loaddev had been changed */
    dev = getenv("loaddev");
    if ((copy_dev) && (strcmp(dev, copy_dev))) {
        saveenv();
    }
}

extern char post_status[ ];

void
log_eeprom_sw_init (void)
{
    uint32_t bus = LOG_EEPROM_I2C_BUS;
#ifdef EEPROM_LOG_FS
    eeprom[0].i2c_bus  = LOG_EEPROM_I2C_BUS; 
    eeprom[0].i2c_addr = LOG_EEPROM_I2C_ADDR;

    api.read  = i2c_eeprom_read;
    api.write = i2c_eeprom_write;
    api.erase = i2c_eeprom_erase;

    eeprom[0].eeprom_api = (void *) &api;
    eeprom_log_init();
#endif

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    memset(post_status, 0, MAX_POST_TESTS + PLT_POST_TESTS);

    if (i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                POST_STATUS_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                &post_status, (MAX_POST_TESTS + PLT_POST_TESTS)) != 0) {
        printf("\ni2c write to init POST status in log FAILED !!!!\n");
        return;
    }

    b2_debug("Log EEPROM inited\n");
}


int 
post_test_status_set (int test, int value) 
{
    uint32_t bus = LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (test < 0 || test >= (MAX_POST_TESTS + PLT_POST_TESTS)) {
        printf("\ni2c write to init POST out of range (0<%d<%u)!\n",
                           test, MAX_POST_TESTS + PLT_POST_TESTS);
         return -1;
    }
    
    post_status[test] = value & 0xff;

    if (i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                POST_STATUS_OFFSET + test, LOG_EEPROM_DEV_WORD_LEN,
                &post_status[test], 1) != 0) {
        printf("\ni2c write to init POST status in log FAILED !!!!\n");
        return -1;
    }
    
    return 0;
}

int 
post_test_nums_set (unsigned char type, char value) 
{
    unsigned char num_tests = value;
    uint32_t bus = LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if (type > 1) {
        printf("\ni2c write to init POST nums out of range (%d)!\n", type);
        return -1;
    }
    
    if (i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                   POST_NUM_TESTS_OFFSET + type, LOG_EEPROM_DEV_WORD_LEN,
                   &num_tests, 1) != 0) {
        printf("\ni2c write to init POST num tests for %d in log FAILED !!!!\n",
               type);
        return -1;
    }
    
    return 0;
}

/* 
 * This is a place holder for any board specific initialization
 * to be done. 
 */
void fx_board_specific_init (void)
{
    uint8_t buf = 0xff;
    char temp[5];    
    char *override = NULL;

    uint32_t bus = LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    log_eeprom_sw_init();
    set_last_boot_status();

    set_last_boot_status();

    switch(g_board_type) {
    case QFX3500_BOARD_ID:
    CASE_SE_WESTERNAVENUE_SERIES:
    case QFX3500_48S4Q_AFO_BOARD_ID:
        if (i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                       TOR_MODE_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                       &buf, 1) != 0) {
            printf("\ni2c read Failed! Defaulting to Standalonemode!\n");
        }
        sprintf(temp, "%d", buf);
        setenv("hw.board.tor_mode", temp);
        break;
    CASE_UFABRIC_QFX3600_SERIES:
       /* 
        * Check for Override variaable is set for any Series, 
        * then set it as Standalone 
        */
        override = getenv("ufabric_override_sa");
        if (override != NULL) {
            sprintf(temp, "%s", override);
            setenv("hw.board.tor_mode", temp);
            sprintf(temp, "%04x", QFX3600_SE_BOARD_ID_AFI);
            setenv("hw.board.type", temp);
        } else { 
            if (i2c_seq_rd(bus, LOG_EEPROM_I2C_ADDR,
                           TOR_MODE_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                           &buf, 1) != 0) {
               printf("\ni2c read Failed! Defaulting to Standalonemode!\n");
            }
            if (buf >= TOR_MODE_UFAB) {
                sprintf(temp, "%d", TOR_MODE_UFAB);
                setenv("hw.board.tor_mode", temp);
            } else {
                sprintf(temp, "%d", buf);
                setenv("hw.board.tor_mode", temp);
            }
        }
        break;    
    default:
        break;
    }
    fx_set_next_bootdev();
    
    if (fx_is_pa()) {
        uint32_t gpio_data  = GPIO_WRITE_DATA_VAL;
        uint32_t gpio_cfg = GPIO_CFG_IO_VAL;
        uint8_t  therm_limit = 0x60;
        /* access CPU GPIO register */
        volatile uint32_t *gpio_data_register = (volatile uint32_t *)XLR_GPIO_DATA_REG;
        uint32_t temp = *gpio_data_register;

        gpio_cfg_io_set(gpio_cfg | 0x3F | (1 << 24));
        gpio_write_data_set(((gpio_data & CHAN_MASK) | 
                            4                     | 
                            (1 << 24)));
        udelay(1000);  /* 1ms delay */

        /* die temperature remote/local therm limit */
        i2c_write(I2C_BUS0, 0x4C, 0x19, &therm_limit, 1);
        i2c_write(I2C_BUS0, 0x4C, 0x20, &therm_limit, 1);

        gpio_write_data_set(temp);
        udelay(1000);  /* 1ms delay */
    }
}

static void
cmd_set_mode (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char    *buf;
    uint32_t bus = LOG_EEPROM_I2C_BUS;

    if (fx_use_i2c_cpld(LOG_EEPROM_I2C_ADDR)) {
        bus = TOR_CPLD_I2C_CREATE_MUX(1, 0, 0);
    }

    if ((g_board_type != QFX3500_BOARD_ID) &&
         (g_board_type != QFX3500_48S4Q_AFO_BOARD_ID) && 
        (!PRODUCT_IS_OF_UFABRIC_WA_SERIES(g_board_type)) && 
        (!PRODUCT_IS_OF_SE_WA_SERIES(g_board_type))) {
        printf("\nCommand is not supported on this board!!\n");
        return;
    }
    if (argc != 2) {
        printf("\nInvalid number of arguments!!\n");
        printf ("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    buf = argv[1];
    if (i2c_seq_wr(bus, LOG_EEPROM_I2C_ADDR,
                   TOR_MODE_OFFSET, LOG_EEPROM_DEV_WORD_LEN,
                   buf, 1) != 0) {
        printf("\ni2c write Failed! Cannot set TOR Mode!\n");
        return;
    }
    setenv("hw.board.tor_mode", buf);
}

int
do_mmap (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    rmi_print_board_map();
    return 0;
}

U_BOOT_CMD(
        dump_cpld, 1, 1, fx_cpld_dump,
        "dump_cpld - Dump the basic Summary of all CPLD registers\n",
        "\n- print all the FX CPLD registers and their summary\n"
);

U_BOOT_CMD(
        cpld_reset, 1, 1, fx_cpld_reset_to_next_partition,
        "cpld_reset - Reset to the other partition\n",
        "\n- Reboot from the other Boot Flash partition\n"
);

U_BOOT_CMD(
        set_mode, 2, 1, cmd_set_mode,
        "set_mode {mode} - Set the TOR mode\n",
        "\nSets the TOR mode.\n"
);

U_BOOT_CMD(
        mmap, 1, 1, do_mmap,
        "mmap    - memory map\n",
        "\nShow the U-boot memory map.\n"
);

