#ifndef __EXPORTS_H__
#define __EXPORTS_H__

#ifndef __ASSEMBLY__

#include <common.h>

/* These are declarations of exported functions available in C code */
unsigned long get_version(void);
int  getc(void);
int  tstc(void);
void putc(const char);
void puts(const char*);
int printf(const char* fmt, ...);
void install_hdlr(int, interrupt_handler_t*, void*);
void free_hdlr(int);
void *malloc(size_t);
void free(void*);
void __udelay(unsigned long);
unsigned long get_timer(unsigned long);
int vprintf(const char *, va_list);
void do_reset(void);
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);
int strict_strtoul(const char *cp, unsigned int base, unsigned long *res);
char *getenv(char *name);
int setenv(char *varname, char *varvalue);
long simple_strtol(const char *cp,char **endp,unsigned int base);
int strcmp(const char * cs,const char * ct);
int ustrtoul(const char *cp, char **endp, unsigned int base);
#ifdef CONFIG_HAS_UID
void forceenv(char *varname, char *varvalue);
#endif
#if defined(CONFIG_CMD_I2C)
int i2c_write(uchar, uint, int , uchar* , int);
int i2c_read(uchar, uint, int , uchar* , int);
#endif
#include <spi.h>

/* JUNOS Start */
#ifdef CONFIG_CMD_USB
int usb_disk_scan(int last);
int usb_disk_read(int dev, int lba, int blks, void *buf);
int usb_disk_write(int dev, int lba, int nlks, void *buf);
#endif
int jt_eth_init(bd_t *);
void eth_halt(void);
int eth_send(volatile void *, int);
int jt_eth_receive(volatile void *, int);
#if defined(CONFIG_PRODUCT_EXSERIES)
int32_t ex_get_next_bootdev(char *bootdev);
void ex_reload_watchdog(int32_t val);
void timer_disable_periodic_hw_access(void);
#endif
/* JUNOS End */

void app_startup(char * const *);

#endif    /* ifndef __ASSEMBLY__ */

enum {
#define EXPORT_FUNC(x) XF_ ## x ,
#include <_exports.h>
#undef EXPORT_FUNC

	XF_MAX
};

#define XF_VERSION	7

#if defined(CONFIG_X86)
extern gd_t *global_data;
#endif

#endif	/* __EXPORTS_H__ */
