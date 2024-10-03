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
void printf(const char* fmt, ...);
void install_hdlr(int, interrupt_handler_t*, void*);
void free_hdlr(int);
void *malloc(size_t);
void free(void*);
void udelay(unsigned long);
#if defined CONFIG_OCTEON || defined CONFIG_RMI_XLS
uint64_t get_timer         (uint64_t base);
#else
unsigned long get_timer(unsigned long);
#endif
void vprintf(const char *, va_list);
void do_reset (void);
#ifdef CONFIG_MARVELL
void *realloc(void*, size_t);
void *calloc(size_t, size_t);
void *memalign(size_t, size_t);
u32 mvGetRtcSec(void);
#endif
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);
char *getenv (char *name);
void setenv (char *varname, char *varvalue);
#if (CONFIG_COMMANDS & CFG_CMD_I2C)
#ifdef CONFIG_MARVELL
int i2c_write (uchar, uchar, uint, int , uchar* , int);
int i2c_read (uchar, uchar, uint, int , uchar* , int);
#else
int i2c_write (uchar, uint, int , uchar* , int);
int i2c_read (uchar, uint, int , uchar* , int);
#endif
#endif	/* CFG_CMD_I2C */
int eth_init(bd_t *);
void eth_halt(void);
int eth_send(volatile void *, int);
int eth_receive(volatile void *, int);

int usb_disk_scan(int last);
int usb_disk_read(int dev, int lba, int blks, void *buf);
int usb_disk_write(int dev, int lba, int nlks, void *buf);

int cf_disk_scan(int last);
int cf_disk_read(int dev, int lba, int blks, unsigned char *buf);
int cf_disk_write(int dev, int lba, int nlks, void *buf);

int disk_export_scan(int last);
int disk_export_read(int dev, int lba, int blks, void *buf);
int disk_export_write(int dev, int lba, int nlks, void *buf);

int lcd_printf_2(const char *fmt, ...);

uint32_t get_msec_timer(uint32_t base);

#ifdef CONFIG_FX
#include <pci.h>
#include <common/fx_cpld.h>
pci_dev_t pci_find_devices (struct pci_device_id *ids, int index);
int pci_read_config_byte(pci_dev_t dev, int where, u8 *val);
int pci_write_config_byte(pci_dev_t dev, int where, u8 val);
int pci_read_config_word(pci_dev_t dev, int where, u16 *val);
int pci_write_config_word(pci_dev_t dev, int where, u16 val);
int pci_read_config_dword(pci_dev_t dev, int where, u32 *val);
int pci_write_config_dword(pci_dev_t dev, int where, u32 val);
#endif /* CONFIG_FX */

#if defined(CONFIG_JSRXNLE) || defined(CONFIG_MAG8600)

int i2c_read8(uchar chip, uint addr, uchar *buffer);
int i2c_read16(uchar chip, uint addr, uchar *buffer);
int i2c_write8(uchar chip, uint addr, uchar *buffer);
int i2c_write16(uchar chip, uint addr, uchar *buffer);
int octeon_twsi_write_max(uint8_t dev_addr, uint16_t addr, uint8_t *data,
                          int len);
int octeon_twsi_read_max(uint8_t dev_addr, uint16_t addr, uint8_t *data,
                         int len);
uint32_t get_delay(uint32_t base);

/*
added for PM3 as PM3 has two i2c controllers so we need to specify the 
twsii_bus also and it has i2c switches also that need to be programmed 
before we can talk to the device
*/
int i2c_read8_generic(uint8_t device, uint16_t offset, 
                      uint8_t *val, uint8_t twsii_bus);
int i2c_write8_generic(uint8_t device, uint16_t offset, 
                       uint8_t val, uint8_t twsii_bus);
int i2c_switch_programming(uint8_t i2c_switch_addr, uint8_t value);

uint32_t cvmx_pcie_mem_read32(int pcie_port, uint32_t offset);
void cvmx_pcie_mem_write32(int pcie_port, uint32_t offset, uint32_t val);
uint32_t cvmx_pcie_config_read32(int pcie_port, int bus, int dev, 
                                 int fn, int reg);
void cvmx_pcie_config_write32(int pcie_port, int bus, int dev, 
                              int fn, int reg, uint32_t val);

int32_t srxsme_get_next_bootdev_jt(uint8_t *);
int32_t srxsme_set_next_bootdev(uint8_t *);
void srxsme_show_bootdev_list(void);
void reload_watchdog(int);
uint8_t cpld_read(uint8_t addr);
void cpld_write(uint8_t addr, uint8_t val);
int uart_init(void *uart_info);
int uart_getc(int uart_index) ;
void uart_putc(const char c, int uart_index);
uint32_t octeon_pci_read_io(uint32_t addr, uint8_t size); 
int octeon_pci_write_io(uint32_t addr, uint8_t size, uint32_t val); 
int octeon_read_config(int dev, int reg, int size, uint32_t *val);
void init_octeon_pci (void);
void octeon_gpio_ext_cfg_output(int bit);
void octeon_gpio_set(int bit);
int limit_l2_ways(int ways, int verbose);
uint32_t nand_phys_page_read(uint32_t pgno, uint8_t *buf);

int32_t mag8600_get_next_bootdev(uint8_t *);
int32_t mag8600_set_next_bootdev(uint8_t *);
void mag8600_show_bootdev_list(void);
#ifdef CONFIG_NET_MULTI
void eth_set_current(void);
#endif
void cvmx_pcie_config_regex(void);
#endif

#if defined(CONFIG_ACX500_SVCS)
int i2c_read8_generic(uint8_t device, uint16_t offset, 
                      uint8_t *val, uint8_t twsii_bus);
int i2c_write8_generic(uint8_t device, uint16_t offset, 
                       uint8_t val, uint8_t twsii_bus);
#endif
ulong  get_tbclk(void);
int disable_interrupts (void);
void enable_interrupts (void);

void app_startup(char **);

#if defined(CONFIG_EX2200)
void syspld_watchdog_enable(void);
void syspld_watchdog_disable(void);
void syspld_watchdog_petting(void);
#endif

#if defined(CONFIG_EX82XX) || defined(CONFIG_EX3242) || \
    defined(CONFIG_EX45XX) || defined(CONFIG_EX2200) || defined(CONFIG_EX3300)
int32_t ex_get_next_bootdev(char *);
void ex_reload_watchdog(int);
#endif
#if defined(CONFIG_RPS200)
void rps_reset (void);
void i2c_cpld_reset (void);
void i2c_reset(void);
int i2c_slave_write_wait(uint8_t slave, uint8_t *pBlock, uint32_t blockSize, uint32_t *pSize, uint32_t iwait);
int i2c_slave_read_wait(uint8_t slave, uint8_t *pBlock, uint32_t blockSize, uint32_t iwait);
int rps_nand_read(ulong offset, ulong *len, u_char *buf);
int rps_nand_write(ulong offset, ulong *len, u_char *buf);
int rps_nand_block_isbad(ulong offset);
int rps_nand_erase(ulong offset, ulong size);
void watchdog_init(uint32_t ms);
void watchdog_disable(void);
void watchdog_enable(void);
void watchdog_reset(void);
void upgrade_rps_image(void);
#endif

#endif    /* ifndef __ASSEMBLY__ */

enum {
#define EXPORT_FUNC(x) XF_ ## x ,
#include <_exports.h>
#undef EXPORT_FUNC

	XF_MAX
};

#define XF_VERSION	7

#if defined(CONFIG_I386)
extern gd_t *global_data;
#endif

#endif	/* __EXPORTS_H__ */
