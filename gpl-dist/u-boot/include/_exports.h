/*
 * You do not need to use #ifdef around functions that may not exist
 * in the final configuration (such as i2c).
 */
EXPORT_FUNC(get_version)
EXPORT_FUNC(getc)
EXPORT_FUNC(tstc)
EXPORT_FUNC(putc)
EXPORT_FUNC(puts)
EXPORT_FUNC(printf)
EXPORT_FUNC(install_hdlr)
EXPORT_FUNC(free_hdlr)
EXPORT_FUNC(malloc)
EXPORT_FUNC(free)
EXPORT_FUNC(udelay)
EXPORT_FUNC(get_timer)
EXPORT_FUNC(vprintf)
EXPORT_FUNC(do_reset)
EXPORT_FUNC(getenv)
EXPORT_FUNC(setenv)
EXPORT_FUNC(simple_strtoul)
EXPORT_FUNC(i2c_write)
EXPORT_FUNC(i2c_read)
EXPORT_FUNC(jt_eth_init)
EXPORT_FUNC(eth_halt)
EXPORT_FUNC(eth_send)
EXPORT_FUNC(jt_eth_receive)
EXPORT_FUNC(disk_scan)
EXPORT_FUNC(disk_read)
EXPORT_FUNC(disk_write)
EXPORT_FUNC(lcd_printf)
EXPORT_FUNC(saveenv)
/*
 * fill jumptable with dummy functions and
 * be compatible with the jt used by the loader
 * (bsd/sys/boot/uboot-1.1.6) that is in use
 * currently.
 */
EXPORT_FUNC(dummy1)
EXPORT_FUNC(dummy2)
EXPORT_FUNC(dummy3)
EXPORT_FUNC(dummy4)
EXPORT_FUNC(dummy5)
EXPORT_FUNC(dummy6)
EXPORT_FUNC(dummy7)
EXPORT_FUNC(dummy8)
EXPORT_FUNC(dummy9)
EXPORT_FUNC(dummy10)
EXPORT_FUNC(dummy11)
EXPORT_FUNC(dummy12)
EXPORT_FUNC(dummy13)
EXPORT_FUNC(dummy14)
EXPORT_FUNC(dummy15)
EXPORT_FUNC(get_next_bootdev)
EXPORT_FUNC(set_next_bootdev)
EXPORT_FUNC(show_bootdev_list)
EXPORT_FUNC(reload_watchdog)
EXPORT_FUNC(disable_hw_access)

