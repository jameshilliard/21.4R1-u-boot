/*
 * Copyright (c) 2006-2013, 2020, Juniper Networks, Inc.
 * All rights reserved.
 *
 */

#include <common.h>
#include <exports.h>
#ifdef CONFIG_JSRXNLE
#include <platform_srxsme.h>
#endif
#ifdef CONFIG_SRX3000
#include <platform_srx3000.h>
#endif
#ifdef CONFIG_MAG8600
#include <platform_mag8600.h>
#endif
#ifdef CONFIG_FX
#include<soho/soho_lcd.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#if (defined(CONFIG_ACX) && !defined(CONFIG_ACX_ICRT))
extern int acx_disk_scan(int dev);
extern int acx_disk_read(int dev, int lba, int totcnt, char *buf);
#elif defined(CONFIG_MX80)
extern int mx80_disk_scan(int dev);
extern int mx80_disk_read(int dev, int lba, int totcnt, char *buf);
#endif

static void dummy(void)
{
}
static int int_dummy (void)
{
    return (0);
}    

unsigned long get_version(void)
{
#if defined(CONFIG_JSRXNLE) || defined(CONFIG_MAG8600) || defined(CONFIG_SRX3000) \
    || defined(CONFIG_ACX500_SVCS)
    return get_uboot_version(); 
#else
	return XF_VERSION;
#endif
}

#ifndef CONFIG_SHOW_ACTIVITY
int lcd_printf_2(const char *fmt, ...)
{
	return -1;
}
#endif

static void
jumptable_assign_disk_functions (void)
{
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_CF_MODELS
    CASE_ALL_SRX650_MODELS
        gd->jt[XF_disk_scan] = (void *) disk_export_scan;
        gd->jt[XF_disk_read] = (void *) disk_export_read;
        gd->jt[XF_disk_write] = (void *) disk_export_write;
        break;
    default:
        gd->jt[XF_disk_scan] = (void *) usb_disk_scan;
        gd->jt[XF_disk_read] = (void *) usb_disk_read;
        gd->jt[XF_disk_write] = (void *)usb_disk_write;
        break;
    }
#elif defined(CONFIG_EX82XX)
	if (!EX82XX_LCPU) {
		gd->jt[XF_disk_scan] = (void *) usb_disk_scan;
		gd->jt[XF_disk_read] = (void *) usb_disk_read;
		gd->jt[XF_disk_write] = (void *)usb_disk_write;
	}
#elif (defined(CONFIG_ACX) && !defined(CONFIG_ACX_ICRT))
    gd->jt[XF_disk_scan] = (void *) acx_disk_scan;
    gd->jt[XF_disk_read] = (void *) acx_disk_read;
    gd->jt[XF_disk_write] = (void *)usb_disk_write;
#elif defined(CONFIG_MX80)
    gd->jt[XF_disk_scan] = (void *) mx80_disk_scan;
    gd->jt[XF_disk_read] = (void *) mx80_disk_read;
    gd->jt[XF_disk_write] = (void *)usb_disk_write;
#elif defined(CONFIG_MAG8600)
    gd->jt[XF_disk_scan] = (void *) disk_export_scan;
    gd->jt[XF_disk_read] = (void *) disk_export_read;
    gd->jt[XF_disk_write] = (void *) disk_export_write;
#else
    gd->jt[XF_disk_scan] = (void *) usb_disk_scan;
    gd->jt[XF_disk_read] = (void *) usb_disk_read;
    gd->jt[XF_disk_write] = (void *)usb_disk_write;
#endif

}


void jumptable_init (void)
{
	int i;

	gd->jt = (void **) malloc (XF_MAX * sizeof (void *));
	for (i = 0; i < XF_MAX; i++)
		gd->jt[i] = (void *) dummy;

	gd->jt[XF_get_version] = (void *) get_version;
	gd->jt[XF_malloc] = (void *) malloc;
	gd->jt[XF_free] = (void *) free;
	gd->jt[XF_getenv] = (void *) getenv;
	gd->jt[XF_setenv] = (void *) setenv;
	/*
	 * The following expect an integer to be returned
	 * When the loader calls these functions, they are called
	 * expecting an int in return. The previous dummy call did not
	 * return any which made the loader to fault. Adding a new dummy
	 * function to return some integer so that the loader is happy.
	 */
	gd->jt[XF_get_next_bootdev] = (void *) int_dummy;
	gd->jt[XF_set_next_bootdev] = (void *) int_dummy;

/* The get_timer function should not be used by any platform here as the
 * upper layers (J-Loader - lib/time.c) expects a Millisecond timer. Not
 * sure if any other platforms are using this currently hence
 * using a define
 */


#if defined (CONFIG_JSRXNLE) || defined(CONFIG_MAG8600) || defined (CONFIG_FX) \
    || defined(CONFIG_ACX500_SVCS)
        gd->jt[XF_get_timer] = (void *) get_msec_timer;
#elif defined(CONFIG_EX82XX) || \
    defined(CONFIG_EX3242) || \
    defined(CONFIG_SRX3000) || \
    defined(CONFIG_MX80) || \
    defined(CONFIG_ACX) || \
    defined(CONFIG_EX45XX) || \
    defined(CONFIG_EX2200) || \
    defined(CONFIG_RPS200) || \
    defined(CONFIG_EX3300)
        gd->jt[XF_get_timer] = (void *) get_timer;
#else
        gd->jt[XF_get_timer] = (void *) get_delay;
#endif

	gd->jt[XF_simple_strtoul] = (void *) simple_strtoul;
	gd->jt[XF_udelay] = (void *) udelay;
#ifdef CONFIG_JSRXNLE
	gd->jt[XF_do_reset] = (void *) do_reset_jt;
#else
	gd->jt[XF_do_reset] = (void *) do_reset;
#endif
#if defined(CONFIG_I386) || defined(CONFIG_PPC)
	gd->jt[XF_install_hdlr] = (void *) irq_install_handler;
	gd->jt[XF_free_hdlr] = (void *) irq_free_handler;
#endif	/* I386 || PPC */
#if (CONFIG_COMMANDS & CFG_CMD_I2C)
	gd->jt[XF_i2c_write] = (void *) i2c_write;
	gd->jt[XF_i2c_read] = (void *) i2c_read;
#endif	/* CFG_CMD_I2C */
#if (CONFIG_COMMANDS & CFG_CMD_NET)
	gd->jt[XF_eth_init] = (void *) eth_init;
	gd->jt[XF_eth_halt] = (void *) eth_halt;
	gd->jt[XF_eth_send] = (void *) eth_send;
	gd->jt[XF_eth_receive] = (void *) eth_receive;
#endif

	jumptable_assign_disk_functions();

#ifndef CONFIG_JSRXNLE
	gd->jt[XF_lcd_printf] = (void *) lcd_printf_2;
#endif	/* CONFIG_JSRXNLE */
	gd->jt[XF_saveenv] = (void *) saveenv;

/*
 *  * DCBG exports.
 *   */
#ifdef CONFIG_FX
	gd->jt[XF_lcd_printf] = (void *) fx_lcd_write;
	gd->jt[XF_pci_find_devices] = (void *) pci_find_devices;
        gd->jt[XF_pci_read_config_byte] = (void *) pci_read_config_byte;
        gd->jt[XF_pci_write_config_byte] = (void *) pci_write_config_byte;
        gd->jt[XF_pci_read_config_word] = (void *) pci_read_config_word;
        gd->jt[XF_pci_write_config_word] = (void *) pci_write_config_word;
        gd->jt[XF_pci_read_config_dword] = (void *) pci_read_config_dword;
        gd->jt[XF_pci_write_config_dword] = (void *) pci_write_config_dword;
        gd->jt[XF_vprintf] = (void *) vprintf;
#endif /* CONFIG_DCBG */

#ifdef CONFIG_JSRXNLE
	gd->jt[XF_i2c_read8]   = (void *) i2c_read8;
	gd->jt[XF_i2c_read16]  = (void *) i2c_read16;
	gd->jt[XF_i2c_write8]  = (void *) i2c_write8;
	gd->jt[XF_i2c_write16] = (void *) i2c_write16;
	gd->jt[XF_octeon_twsi_write_max] = (void *) octeon_twsi_write_max;
	gd->jt[XF_octeon_twsi_read_max] = (void *) octeon_twsi_read_max;
	gd->jt[XF_i2c_read8_generic]   = (void *) i2c_read8_generic;
	gd->jt[XF_i2c_write8_generic]  = (void *) i2c_write8_generic;
	gd->jt[XF_i2c_switch_programming]  = (void *) i2c_switch_programming;
	gd->jt[XF_vprintf] = (void *) vprintf;
	gd->jt[XF_cvmx_pcie_mem_read32] = (void *) cvmx_pcie_mem_read32;
	gd->jt[XF_cvmx_pcie_mem_write32] = (void *) cvmx_pcie_mem_write32;
	gd->jt[XF_cvmx_pcie_config_read32] = (void *) cvmx_pcie_config_read32;
	gd->jt[XF_cvmx_pcie_config_write32] = (void *) cvmx_pcie_config_write32;
	gd->jt[XF_uart_init] = (void *)uart_init;
	gd->jt[XF_uart_getc] = (void *)uart_getc;
	gd->jt[XF_uart_putc] = (void *)uart_putc;
	gd->jt[XF_octeon_pci_read_io] = (void *)octeon_pci_read_io;
	gd->jt[XF_octeon_pci_write_io] = (void *)octeon_pci_write_io;
	gd->jt[XF_octeon_read_config] = (void *)octeon_read_config;
	gd->jt[XF_init_octeon_pci] = (void *)init_octeon_pci;
	gd->jt[XF_octeon_gpio_ext_cfg_output] = (void *)octeon_gpio_ext_cfg_output;
	gd->jt[XF_octeon_gpio_set] = (void *)octeon_gpio_set;
#ifdef CONFIG_NET_MULTI
	gd->jt[XF_eth_set_current] = (void *) eth_set_current;
#endif
	gd->jt[XF_cvmx_pcie_config_regex] = (void *) cvmx_pcie_config_regex;
	gd->jt[XF_get_next_bootdev] = (void *) srxsme_get_next_bootdev_jt;
	gd->jt[XF_set_next_bootdev] = (void *) srxsme_set_next_bootdev;
	gd->jt[XF_show_bootdev_list] = (void *) srxsme_show_bootdev_list;
	gd->jt[XF_cpld_read] = (void *)cpld_read;
	gd->jt[XF_cpld_write] = (void *)cpld_write;
	gd->jt[XF_limit_l2_ways] = (void *)limit_l2_ways;
	gd->jt[XF_nand_phys_page_read] = (void *)nand_phys_page_read;
#endif
#ifdef CONFIG_MAG8600
	gd->jt[XF_i2c_read8]   = (void *) i2c_read8;
	gd->jt[XF_i2c_read16]  = (void *) i2c_read16;
	gd->jt[XF_i2c_write8]  = (void *) i2c_write8;
	gd->jt[XF_i2c_write16] = (void *) i2c_write16;
	gd->jt[XF_octeon_twsi_write_max] = (void *) octeon_twsi_write_max;
	gd->jt[XF_octeon_twsi_read_max] = (void *) octeon_twsi_read_max;
	gd->jt[XF_i2c_read8_generic]   = (void *) i2c_read8_generic;
	gd->jt[XF_i2c_write8_generic]  = (void *) i2c_write8_generic;
	gd->jt[XF_vprintf] = (void *) vprintf;
	gd->jt[XF_cvmx_pcie_mem_read32] = (void *) cvmx_pcie_mem_read32;
	gd->jt[XF_cvmx_pcie_mem_write32] = (void *) cvmx_pcie_mem_write32;
	gd->jt[XF_cvmx_pcie_config_read32] = (void *) cvmx_pcie_config_read32;
	gd->jt[XF_cvmx_pcie_config_write32] = (void *) cvmx_pcie_config_write32;
	gd->jt[XF_eth_set_current] = (void *) eth_set_current;
	gd->jt[XF_get_next_bootdev] = (void *) mag8600_get_next_bootdev;
	gd->jt[XF_set_next_bootdev] = (void *) mag8600_set_next_bootdev;
	gd->jt[XF_show_bootdev_list] = (void *) mag8600_show_bootdev_list;
#endif
#if defined(CONFIG_ACX500_SVCS)
	gd->jt[XF_i2c_read8_generic]   = (void *) i2c_read8_generic;
	gd->jt[XF_i2c_write8_generic]  = (void *) i2c_write8_generic;
#endif
#if defined (CONFIG_JSRXNLE) || defined (CONFIG_FX)
	gd->jt[XF_reload_watchdog] = (void *) reload_watchdog; 
#endif
#if defined(CONFIG_RPS200)
	gd->jt[XF_rps_reset] = (void *) rps_reset;
	gd->jt[XF_i2c_cpld_reset] = (void *) i2c_cpld_reset;
	gd->jt[XF_i2c_reset] = (void *) i2c_reset;
	gd->jt[XF_i2c_slave_write_wait] = (void *) i2c_slave_write_wait;
	gd->jt[XF_i2c_slave_read_wait] = (void *)i2c_slave_read_wait;
	gd->jt[XF_watchdog_init] = (void *)watchdog_init;
	gd->jt[XF_watchdog_disable] = (void *)watchdog_disable;
	gd->jt[XF_watchdog_enable] = (void *)watchdog_enable;
	gd->jt[XF_watchdog_reset] = (void *)watchdog_reset;
	gd->jt[XF_upgrade_rps_image] = (void *)upgrade_rps_image;
#endif
	gd->jt[XF_get_tbclk] = (void *) get_tbclk;
#ifdef CONFIG_MARVELL
	gd->jt[XF_calloc] = (void *) calloc;
	gd->jt[XF_realloc] = (void *) realloc;
	gd->jt[XF_memalign] = (void *) memalign;
#ifndef MV_TINY_IMAGE
	gd->jt[XF_mvGetRtcSec] = (void *) mvGetRtcSec;
#endif
#endif
#if defined(CONFIG_EX82XX) || defined(CONFIG_EX3242) || \
    defined(CONFIG_EX45XX) || defined(CONFIG_EX2200) || defined(CONFIG_EX3300)
	gd->jt[XF_get_next_bootdev] = (void *) ex_get_next_bootdev;
	gd->jt[XF_reload_watchdog] = (void *) ex_reload_watchdog; 
	gd->jt[XF_disable_hw_access] = (void *) dummy;
#endif
}
