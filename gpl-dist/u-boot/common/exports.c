#include <common.h>
#include <exports.h>

DECLARE_GLOBAL_DATA_PTR;

__attribute__((unused)) static void dummy(void)
{
}

unsigned long get_version(void)
{
	return XF_VERSION;
}

/* Reuse _exports.h with a little trickery to avoid bitrot */
#define EXPORT_FUNC(sym) gd->jt[XF_##sym] = (void *)sym;

#if !defined(CONFIG_X86) && !defined(CONFIG_PPC)
# define install_hdlr      dummy
# define free_hdlr         dummy
#else /* kludge for non-standard function naming */
# define install_hdlr      irq_install_handler
# define free_hdlr         irq_free_handler
#endif
#ifndef CONFIG_CMD_I2C
# define i2c_write         dummy
# define i2c_read          dummy
#endif
#ifndef CONFIG_CMD_SPI
# define spi_init          dummy
# define spi_setup_slave   dummy
# define spi_free_slave    dummy
# define spi_claim_bus     dummy
# define spi_release_bus   dummy
# define spi_xfer          dummy
#endif
#ifndef CONFIG_HAS_UID
# define forceenv          dummy
#endif
#ifdef CONFIG_CMD_USB
#define disk_scan	usb_disk_scan
#define disk_read	usb_disk_read
#define disk_write	usb_disk_write
#else
#define disk_scan	dummy
#define disk_read	dummy
#define disk_write	dummy
#endif
#define lcd_printf	dummy
#if defined(CONFIG_PRODUCT_EXSERIES)
#define	get_next_bootdev    ex_get_next_bootdev
#define	reload_watchdog	    ex_reload_watchdog	
#define	show_bootdev_list   dummy
#define	set_next_bootdev    dummy
#define disable_hw_access   timer_disable_periodic_hw_access
#else
#define disable_hw_access   dummy
#endif
#if defined(CONFIG_ACX)
#define	get_next_bootdev    dummy
#define	reload_watchdog	    dummy 
#define	show_bootdev_list   dummy
#define	set_next_bootdev    dummy
#endif

#define dummy1 dummy
#define dummy2 dummy
#define dummy3 dummy
#define dummy4 dummy
#define dummy5 dummy
#define dummy6 dummy
#define dummy7 dummy
#define dummy8 dummy
#define dummy9 dummy
#define dummy10	dummy
#define dummy11	dummy
#define dummy12	dummy
#define dummy13	dummy
#define dummy14	dummy
#define dummy15	dummy

void jumptable_init(void)
{
	gd->jt = malloc(XF_MAX * sizeof(void *));
#include <_exports.h>
}
