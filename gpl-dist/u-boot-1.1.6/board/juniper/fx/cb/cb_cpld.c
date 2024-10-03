#include <common.h>
#include <common/fx_cpld.h>

#define CB_CPLD_EXT_RST                 0x51
#define CB_CPLD_VERSION                 0x7C

#define SATA_BRIDGE_RESET               (0x1 << 3)
#define FLASH_RESET                     (0x1 << 2)
#define PHY_RESET                       (0x1 << 0)
#define DATA_BUS_8b_MASK                ~(0x3 << 7)
#define FLASH_CS_DEV_PARAM_OFF          0x20

u_int8_t *cb_cpld_base;

void 
cb_cpld_init (struct pio_dev *mcpld_dev)
{
    phoenix_reg_t *flash_mmio = (phoenix_reg_t *)(DEFAULT_RMI_PROCESSOR_IO_BASE +
                                                PHOENIX_IO_FLASH_OFFSET);
    uint32_t data;

    cb_cpld_base = (u_int8_t *)(mcpld_dev->iobase);
    data = phoenix_read_reg(flash_mmio, 
                            FLASH_CS_DEV_PARAM_OFF + mcpld_dev->chip_sel);
    phoenix_write_reg(flash_mmio, FLASH_CS_DEV_PARAM_OFF + mcpld_dev->chip_sel, 
                      (data & (DATA_BUS_8b_MASK)));

    debug("%s: cb_cpld_base=0x%x\n", __func__, cb_cpld_base);
}

void
cb_cpld_init_dev (u_int8_t *cpld_iobase)
{
    if (fx_cpld_write(cb_cpld_base, CB_CPLD_EXT_RST, 
                      (SATA_BRIDGE_RESET | PHY_RESET)) == -1) {
        printf("\n Error: Cannot initialize PHY Devices!! \n");
    }
    udelay(1000);
}

u_int8_t
cb_cpld_get_version (void)
{
    return fx_cpld_read(cb_cpld_base, CB_CPLD_VERSION);                 
}
