/*
 * (C) Copyright 2004,2005
 * Cavium Networks
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

#include <watchdog_cpu.h>
#include <common.h>
#include <command.h>
#include <octeon_boot.h>
#include <lib_octeon_shared.h>
#include <lib_octeon.h>
#include <jsrxnle_ddr_params.h>
#include <octeon_hal.h>
#include <platform_srxsme.h>

#ifdef CONFIG_JSRXNLE

uint32_t srx210_mpim_led_registers[] = {
    ((MINPIM_SRX210_BASE << 16) + MPIM_LED_OFFSET), /* only one is there */
    0
};

uint32_t srx220_mpim_led_registers[] = {
    ((SRX220_MPIM1_BASE << 16) + MPIM_LED_OFFSET), /* mpim 0 led address */
    ((SRX220_MPIM2_BASE << 16) + MPIM_LED_OFFSET), /* mpim 1 led address */
    0
};

uint32_t srx240_mpim_led_registers[] = {
    ((SRX240_MPIM1_BASE << 16) + MPIM_LED_OFFSET), /* mpim 0 led address */
    ((SRX240_MPIM2_BASE << 16) + MPIM_LED_OFFSET), /* mpim 1 led address */
    ((SRX240_MPIM3_BASE << 16) + MPIM_LED_OFFSET), /*   "  2   "   "     */
    ((SRX240_MPIM4_BASE << 16) + MPIM_LED_OFFSET), /*   "  3   "   "     */
    0
};
    
/* SRX220 mpim select masks */
uint8_t srx220_mpim_select_mask[] = {
    SRX220_MPIM0_MUX_MASK,
    SRX220_MPIM1_MUX_MASK
};

/* SRX240 mpim select masks */
uint8_t srx240_mpim_select_mask[] = {
    SRX240_MPIM0_MUX_MASK,
    SRX240_MPIM1_MUX_MASK,
    SRX240_MPIM2_MUX_MASK,
    SRX240_MPIM3_MUX_MASK
};


void srx650_send_relinquish(void);
void jsrxnle_uboot_done(void);
void enable_ciu_watchdog(int mode);
void disable_ciu_watchdog(void);
void pat_ciu_watchdog(void);
uint32_t num_usb_disks(void);

/* set the ethaddr */
void board_get_enetaddr (uchar * enet)
{
    unsigned char temp[20];
    DECLARE_GLOBAL_DATA_PTR;

    memcpy(enet, &gd->mac_desc.mac_addr_base[0], 6);
    sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",
            enet[0], enet[1], enet[2], enet[3], enet[4], enet[5]);
    setenv("ethaddr", temp);
    return;
}

static int jsrxnle_entire_bootflash_access (uint8_t version) 
{
    DECLARE_GLOBAL_DATA_PTR;

    switch (gd->board_desc.board_type)
    {
        /* SRX220 doesn't care about CPLD version. */
        CASE_ALL_SRX220_MODELS
            return TRUE;
            break;
        default:
            break;
    }

    if ((version >= SRX210_CPLD_REV_VER_80) &&
        (version <= SRX210_CPLD_REV_VER_8F)) {
        return FALSE;
    }
    return TRUE;
}

static void jsrxnle_enableall_sectors (void) 
{
    uint8_t cpld_rev_ver = 0;
    uint8_t cpld_reg_val = 0;
    cpld_rev_ver =  cpld_read(SRX210_CPLD_VER_REG);
    
    if (jsrxnle_entire_bootflash_access(cpld_rev_ver)) {
        /* 
         * Enable accesses to both active and backup
         * sector of bootflash using boot_sector_ctl
         * bit of cpld
         */
        cpld_reg_val = cpld_read(SRX210_CPLD_MISC_REG);
        cpld_write(SRX210_CPLD_MISC_REG, (cpld_reg_val |
                   SRX210_CPLD_BOOT_SEC_CTL_MASK));
        return;
    } else {
        printf("Entire bootflash access not allowed\n");
    }
    return;
}

static void srx650_enableall_sectors (void) 
{
    printf("has to be filled as PM3 schematics\n");
}

#endif

#if defined(CONFIG_PCI)
extern void init_octeon_pci (void);

void pci_init_board (void)
{
#ifdef CONFIG_JSRXNLE 
    DECLARE_GLOBAL_DATA_PTR;

    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX110_MODELS
        CASE_ALL_SRX210_MODELS
        CASE_ALL_SRX220_MODELS
        if ((gd->flags & GD_FLG_BOARD_EEPROM_READ_FAILED) == 0)
            init_octeon_pci();
            break;
        default:
            break;
    }
#else
	init_octeon_pci();
#endif
}
#endif


uint8_t jsrxnle_get_board_dram (uint16_t i2c_id)
{
    switch (i2c_id) {
    case I2C_ID_JSRXNLE_SRX100_LOWMEM_CHASSIS:
    case I2C_ID_JSRXNLE_SRX210_LOWMEM_CHASSIS:
    case I2C_ID_SRXSME_SRX210BE_CHASSIS:
        return LOW_MEM_4x1Gb;
    case I2C_ID_JSRXNLE_SRX100_HIGHMEM_CHASSIS:
    case I2C_ID_JSRXNLE_SRX210_HIGHMEM_CHASSIS:
    case I2C_ID_JSRXNLE_SRX210_POE_CHASSIS:
    case I2C_ID_JSRXNLE_SRX210_VOICE_CHASSIS:
    case I2C_ID_SRXSME_SRX210HE_CHASSIS:
    case I2C_ID_SRXSME_SRX210HE_POE_CHASSIS:
    case I2C_ID_SRXSME_SRX210HE_VOICE_CHASSIS:
        return HIGH_MEM_8x1Gb;
    case I2C_ID_SRXSME_SRX100H2_CHASSIS:
    case I2C_ID_SRXSME_SRX210HE2_CHASSIS:
    case I2C_ID_SRXSME_SRX210HE2_POE_CHASSIS:
	return QIMONDA_8x2Gb;	
    default:
        /*
         * Following platforms uses SPD.
         * All SRX110
         * All SRX220
         * All SRX240
         * ALL SRX550
         * All SRX650
         */
        return DRAM_TYPE_SPD;
    }
}

int checkboard (void)
{
    /* Force us into GMII mode, enable interface */
    /* NOTE:  Applications expect this to be set appropriately
    ** by the bootloader, and will configure the interface accordingly */
    cvmx_write_csr (CVMX_GMXX_INF_MODE (0), 0x3);

    /* Enable SMI to talk with the GMII switch */
    cvmx_write_csr (CVMX_SMI_EN, 0x1);
    cvmx_read_csr(CVMX_SMI_EN);


    return 0;
}

unsigned int
compute_sku_id (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    unsigned int sku_id = 0x00;

    switch (gd->board_desc.board_type) {
    /* VDSL-A, 3G */
    case I2C_ID_SRXSME_SRX110B_VA_CHASSIS:
    case I2C_ID_SRXSME_SRX110H_VA_CHASSIS:
        sku_id = SRX110_SKU_ID_VDSL_PRESENT |
                 SRX110_SKU_ID_EXP_CARD_PRESENT;
        break;
    /* VDSL-B, 3G */
    case I2C_ID_SRXSME_SRX110B_VB_CHASSIS:
    case I2C_ID_SRXSME_SRX110H_VB_CHASSIS:
        sku_id = SRX110_SKU_ID_VDSL_PRESENT |
                 SRX110_SKU_ID_VDSL_B |
                 SRX110_SKU_ID_EXP_CARD_PRESENT;
        break;
    /* 802.11N LM */
    case I2C_ID_SRXSME_SRX110B_WL_CHASSIS:
        sku_id = SRX110_SKU_ID_WLAN_PRESENT;
        break;
    /* 802.11N HM */
    case I2C_ID_SRXSME_SRX110H_WL_CHASSIS:
        sku_id = SRX110_SKU_ID_WLAN_PRESENT |
                 SRX110_SKU_ID_DUAL_RADIO;
        break;
    /* VDSL-A, 802.11N dual-radio */
    case I2C_ID_SRXSME_SRX110H_VA_WL_CHASSIS:
        sku_id = SRX110_SKU_ID_VDSL_PRESENT |
                 SRX110_SKU_ID_WLAN_PRESENT |
                 SRX110_SKU_ID_DUAL_RADIO;
        break;
    /* VDSL-B, 802.11N dual-radio */
    case I2C_ID_SRXSME_SRX110H_VB_WL_CHASSIS:
        sku_id = SRX110_SKU_ID_VDSL_PRESENT |
                 SRX110_SKU_ID_VDSL_B |
                 SRX110_SKU_ID_WLAN_PRESENT |
                 SRX110_SKU_ID_DUAL_RADIO;
        break;
    default:
        break;
    }

    return sku_id;
}

int early_board_init (void)
{
    int cpu_ref = 50;
    unsigned int board_sku_id = 0, expected_sku_id = 0; 
#if(defined ( BOARD_EEPROM_TWSI_ADDR ) || defined ( SRX650_BOARD_EEPROM_TWSI_ADDR ))
    uint8_t mac_magic;
    uint8_t mac_version;
    uint8_t mac_offset;
    int i;
#endif
    uint8_t val;
    uint8_t cpld_reg_val;

    DECLARE_GLOBAL_DATA_PTR;
    
    if (OCTEON_IS_MODEL(OCTEON_CN63XX))
    {
        /* configure clk_out pin */
        cvmx_mio_fus_pll_t fus_pll;

        fus_pll.u64 = cvmx_read_csr(CVMX_MIO_FUS_PLL);
        fus_pll.cn63xx.c_cout_rst = 1;
        cvmx_write_csr(CVMX_MIO_FUS_PLL, fus_pll.u64);

        /* Sel::  0:rclk, 1:pllout 2:psout 3:gnd */
        fus_pll.cn63xx.c_cout_sel = 0;
        cvmx_write_csr(CVMX_MIO_FUS_PLL, fus_pll.u64);
        fus_pll.cn63xx.c_cout_rst = 0;
        cvmx_write_csr(CVMX_MIO_FUS_PLL, fus_pll.u64);
    }


    memset((void *)&(gd->mac_desc), 0x0, sizeof(octeon_eeprom_mac_addr_t));
    memset((void *)&(gd->clock_desc), 0x0, sizeof(octeon_eeprom_clock_desc_t));
    
    /* NOTE: this is early in the init process, so the serial port is not yet configured */

#ifdef CONFIG_JSRXNLE 
    for (i = 0; i < SERIAL_STR_LEN; i++) {
        gd->board_desc.serial_str[i] = 
            jsrxnle_read_eeprom(JSRXNLE_EEPROM_SERIAL_NO_OFFSET + i);
    }
    gd->board_desc.serial_str[SERIAL_STR_LEN] = '\0';

    /* 
     * This is a work around for different types of memory chips we have on
     * the proto boards. We figure out if the board is a proto by looking at
     * the revision string. Proto boards will have these as 'REV X0, REV X1'
     * etc referring to proto 0, proto 1 etc. Shipping boards will never
     * have an 'X'. If it is a proto, we look at an offset beyond the
     * standard 128 bytes of the eeprom to get DRAM details. In shipping
     * boards, we figure out the DRAM configuration based on the i2c id.
     */
    if (jsrxnle_read_eeprom(JSRXNLE_EEPROM_REV_STRING_OFFSET + 4) == 'X' && 
        jsrxnle_read_eeprom(JSRXNLE_EEPROM_REV_STRING_OFFSET + 5) == '0') {
        gd->board_desc.board_dram = 
            jsrxnle_read_eeprom(JSRXNLE_EEPROM_BOARD_DRAM_OFFSET);
    } else {
        gd->board_desc.board_dram = 
            jsrxnle_get_board_dram(gd->board_desc.board_type);
    }
#else

    /* Populate global data from eeprom */
    uint8_t ee_buf[OCTEON_EEPROM_MAX_TUPLE_LENGTH];
    int addr;
    addr = octeon_tlv_get_tuple_addr(CFG_DEF_EEPROM_ADDR, EEPROM_CLOCK_DESC_TYPE, 0, ee_buf, OCTEON_EEPROM_MAX_TUPLE_LENGTH);
    if (addr >= 0)
    {
        memcpy((void *)&(gd->clock_desc), ee_buf, sizeof(octeon_eeprom_clock_desc_t));
    }


    /* Determine board type/rev */
    strncpy((char *)(gd->board_desc.serial_str), "unknown", SERIAL_LEN);
    addr = octeon_tlv_get_tuple_addr(CFG_DEF_EEPROM_ADDR, EEPROM_BOARD_DESC_TYPE, 0, ee_buf, OCTEON_EEPROM_MAX_TUPLE_LENGTH);
    if (addr >= 0)
    {
        memcpy((void *)&(gd->board_desc), ee_buf, sizeof(octeon_eeprom_board_desc_t));
    }
    else
    {
        gd->flags |= GD_FLG_BOARD_DESC_MISSING;
        gd->board_desc.rev_minor = 0;
        gd->board_desc.board_type = CVMX_BOARD_TYPE_CN3020_EVB_HS5;
        gd->board_desc.rev_major = 1;
    }
#endif

    /* Even though the CPU ref freq is stored in the clock descriptor, we don't read it here.  The MCU
    ** reads it and configures the clock, and we read how the clock is actually configured.
    ** The bootloader does not need to read the clock descriptor tuple for normal operation on
    ** ebh3100 boards
    */
#ifdef CONFIG_JSRXNLE 
    cpu_ref = CPU_REF;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX100_MODELS
        gd->ddr_clock_mhz = SRX100_DDR_CLOCK_MHZ;
        gd->ddr_ref_hertz = CN5020_FORCED_DDR_AND_CPU_REF_HZ;
        break;

    CASE_ALL_SRX110_MODELS
        gd->ddr_clock_mhz = SRX110_DDR_CLOCK_MHZ;
        gd->ddr_ref_hertz = CN5020_FORCED_DDR_AND_CPU_REF_HZ;
        break;

    CASE_ALL_SRX210_MODELS
        gd->ddr_clock_mhz = IS_PLATFORM_SRX210_533(gd->board_desc.board_type) ? 
        SRX210_533_DDR_CLOCK_MHZ : SRX210_DDR_CLOCK_MHZ;
        gd->ddr_ref_hertz = CN5020_FORCED_DDR_AND_CPU_REF_HZ;
        break;
       
    CASE_ALL_SRX220_MODELS
        gd->ddr_clock_mhz = SRX220_DDR_CLOCK_MHZ;
        gd->ddr_ref_hertz = CN5020_FORCED_DDR_AND_CPU_REF_HZ;
        break;
  
    CASE_ALL_SRX240_MODELS
        gd->ddr_clock_mhz = SRX240_DDR_CLOCK_MHZ;
        gd->ddr_ref_hertz = cpu_ref * 1000000;
        break;
        
    CASE_ALL_SRX550_MODELS
        gd->ddr_clock_mhz = SRX550_DDR_CLOCK_MHZ;
        gd->ddr_ref_hertz = cpu_ref * 1000000;
        break;

    CASE_ALL_SRX650_MODELS
        gd->ddr_clock_mhz = SRX650_DDR_CLOCK_MHZ;
        gd->ddr_ref_hertz = cpu_ref * 1000000;
        break;

    default:
        gd->ddr_clock_mhz = DDR_CLOCK_MHZ;
        break;
    }
#else
    cpu_ref = octeon_mcu_read_cpu_ref();
    gd->ddr_clock_mhz = octeon_mcu_read_ddr_clock();
#endif

    /* Some sanity checks */
    if (cpu_ref <= 0)
    {
        /* Default to 50 */
        cpu_ref = 50;
    }
    if (gd->ddr_clock_mhz <= 0)
    {
        /* Default to 200 */
        gd->ddr_clock_mhz = 200;
    }

    {
#if(defined ( BOARD_EEPROM_TWSI_ADDR ) || defined ( SRX650_BOARD_EEPROM_TWSI_ADDR ))
        mac_magic = jsrxnle_read_eeprom(JSRXNLE_EEPROM_MAC_MAGIC_OFFSET);
        mac_version = jsrxnle_read_eeprom(JSRXNLE_EEPROM_MAC_VERSION_OFFSET);

        if ((mac_magic == JUNIPER_MAC_MAGIC)
            && (mac_version == JUNIPER_MAC_VERSION))
        {
                switch (gd->board_desc.board_type) {
                CASE_ALL_SRX650_MODELS
                CASE_ALL_SRX550_MODELS
                    gd->mac_desc.count = SRX650_MAC_ADDR_COUNT;
                    break;
                default: 
                    gd->mac_desc.count = 1;
                    break;
                }
                for (mac_offset = 0; mac_offset < 6; mac_offset++) {
                        gd->mac_desc.mac_addr_base[mac_offset] 
                        = jsrxnle_read_eeprom(JSRXNLE_EEPROM_MAC_ADDR_OFFSET + mac_offset);
                }
        }
        else
        {
                /* Make up some MAC addresses */
                gd->mac_desc.count = 255;
                gd->mac_desc.mac_addr_base[0] = 0x00;
                gd->mac_desc.mac_addr_base[1] = 0xDE;
                gd->mac_desc.mac_addr_base[2] = 0xAD;
                gd->mac_desc.mac_addr_base[3] = (gd->board_desc.rev_major<<4) | gd->board_desc.rev_minor;
                gd->mac_desc.mac_addr_base[4] = gd->board_desc.serial_str[0];
                gd->mac_desc.mac_addr_base[5] = 0x00;
        }

#else
        /* Make up some MAC addresses */
        gd->mac_desc.count = 255;
        gd->mac_desc.mac_addr_base[0] = 0x00;
        gd->mac_desc.mac_addr_base[1] = 0xDE;
        gd->mac_desc.mac_addr_base[2] = 0xAD;
        gd->mac_desc.mac_addr_base[3] = (gd->board_desc.rev_major<<4) | gd->board_desc.rev_minor;
        gd->mac_desc.mac_addr_base[4] = gd->board_desc.serial_str[0];
        gd->mac_desc.mac_addr_base[5] = 0x00;
#endif
    }

    /* 
     * Set LED init color
     */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
        /* Configure I2C Expander #1 */
        (void)write_i2c(SRX650_I2C1_DEV_ADDR, SRX650_PIO_CONFIG_REG,
                        SRX650_I2C1_CONFIG_MASK, SRX650_I2C_GROUP_SYSTEM_IO);
        /* Clear sysio reset. It may have set earlier 
	 * due to pressing of system shutdown button
         */
        (void)read_i2c(SRX650_I2C1_DEV_ADDR, SRX650_PIO_OUT_REG,
                       &val, SRX650_I2C_GROUP_SYSTEM_IO);
        SRX650_CLR_SYSTEM_RESET(val);
        (void)write_i2c(SRX650_I2C1_DEV_ADDR, SRX650_PIO_OUT_REG,
                        val, SRX650_I2C_GROUP_SYSTEM_IO);

        /* Set power led */
        srx650_set_sysio_power_led();
        /* Set RE led as amber */
        (void)write_i2c(SRX650_CPLD_I2C_ADDR, SRX650_CPLD_LED_REG,
                        SRX650_RE_LED_INIT_MASK, SRX650_I2C_GROUP_TWSI0);
        /* Set 'Reset PIM2' register */
        (void)write_i2c(SRX650_CPLD_I2C_ADDR, SRX650_CPLD_RESET_PIM2_REG,
                        SRX650_RESET_PIM2_MASK, SRX650_I2C_GROUP_TWSI0);
        /* If other RE is present then send relinquish signal and
         * wait for other RE to complete midplane EEPROM reads
         */
        srx650_send_relinquish();

        break;

    CASE_ALL_SRXLE_MODELS
        /* HA and alarm LED are off by default */
        cpld_reg_val = 0x00;

        /* status is amber */
        cpld_reg_val |= SRXNLE_STAT_LED_AMBER_MASK;

        /* Power is green by default */
        cpld_reg_val |= SRXNLE_PWR_LED_GREEN_MASK;

        /* Lit up LEDs with default color */
        cpld_write(SRX210_CPLD_LED_CTL_REG, cpld_reg_val);
        break;
    
    default:
        break;
    }

    /*
     * SRX110 has a superset SKU which is used internally for testing.
     * It can be programmed to act as any particular SKU, depending on the
     * I2C-ID. The following code takes care of this.
     */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX110_MODELS
        board_sku_id = cpld_read(SRX110_CPLD_SKU_ID);
        if (board_sku_id & SRX110_SKU_ID_SUPERSET) {
            /* Superset SKU: we need to program the SKU-ID register */
            expected_sku_id = compute_sku_id();
            cpld_write(SRX110_CPLD_SKU_ID, expected_sku_id);
        }
        break;
    default:
        break;
    }

    return 0;
}

int i2c_switch_programming (uint8_t i2c_switch_addr, uint8_t value)
{
    uchar temp;
    uchar addr ;
    
    for(addr=I2C_SWITCH1; addr < (I2C_SWITCH4+1); addr++ ){
        temp = (addr == i2c_switch_addr) ? value : DISABLE;
        if ( i2c_write8_generic(addr, I2C_SWITCH_CTRL_REG, temp, 1) ){
            return -1;
        }
    }
    return 0;
}

uint8_t cpld_read (uint8_t addr)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t *ptr;
    uint8_t buf[1];

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX100_MODELS
    CASE_ALL_SRX110_MODELS
    CASE_ALL_SRX210_MODELS
    CASE_ALL_SRX240_MODELS
        ptr = (uint8_t *)(OCTEON_CPLD_BASE_ADDR | addr);
        break;
    CASE_ALL_SRX220_MODELS
    CASE_ALL_SRX550_MODELS
        ptr = (uint8_t *)(OCTEON_SRX220_CPLD_BASE_ADDR | addr);
        break;
    CASE_ALL_SRX650_MODELS /* PM3 CPLD is on I2C */
        i2c_read8_generic(SRX650_CPLD_I2C_ADDR, addr, buf, 0);
        ptr = &buf[0];
        break;
    default:
        break;
    }
    
    return *ptr;
}

void cpld_write (uint8_t addr, uint8_t val)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t *ptr;
    
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX100_MODELS
    CASE_ALL_SRX110_MODELS
    CASE_ALL_SRX210_MODELS
    CASE_ALL_SRX240_MODELS
        ptr = (uint8_t *)(OCTEON_CPLD_BASE_ADDR | addr);
        *ptr = val; 
        break;
    CASE_ALL_SRX220_MODELS
    CASE_ALL_SRX550_MODELS
        ptr = (uint8_t *)(OCTEON_SRX220_CPLD_BASE_ADDR | addr);
        *ptr = val; 
        break;
    CASE_ALL_SRX650_MODELS /* PM3 CPLD is on I2C */
        i2c_write8_generic(SRX650_CPLD_I2C_ADDR, addr, val, 0);
        break;
    default:
        break;
    }
    
}

/*
 * get_proto_id: returns the proto_id by reading
 * proto_id register of the platform.
 */
uint32_t
get_proto_id (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t proto_id;
    
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX100_MODELS
    CASE_ALL_SRX210_MODELS
        proto_id = cpld_read(SRX210_PROTO_ID_REG);
        break;
    default:
        /* 0xff is invalid proto id */
        proto_id = 0xff;
        break;
    }
    return proto_id;
}

/*
 * is_proto_id_supported verifies is platform cpld
 * has support for proto_id
 */ 
int32_t 
is_proto_id_supported (void)
{
    uint8_t proto_id; 
    DECLARE_GLOBAL_DATA_PTR;
    proto_id = get_proto_id();
                   
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX210_MODELS
    CASE_ALL_SRX100_MODELS
        /*
         * on srx210 proto 1,2,3 will return 0xaa and
         * proto 4 onwards retrun teh proto_id which will be
         * actual proto verion
         */ 
        return (proto_id == 0xaa ? 0 : 1);
    default:
        return 0;
    }  
} 

void reset_jsrxnle_watchdog (void)
{
    cpld_write(4, 0);
}

void
srxle_board_reset (uint8_t device_reset_mask)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t temp;

    /*
     * Reset all devices except for CPU
     */
    temp = cpld_read(SRX210_CPLD_RESET_REG);
    temp &= ~(device_reset_mask);
    cpld_write(SRX210_CPLD_RESET_REG, temp);

    /* 
     * wait for 20 milliseconds before pulling
     * devices out of reset.
     */
    udelay(20000);

    /* pull devices out of reset */
    temp |= device_reset_mask;
    cpld_write(SRX210_CPLD_RESET_REG, temp);
    /* wait 100 milliseconds */
    udelay(100000);

    /*
     * Reset CPU
     */
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX220_MODELS
        CASE_ALL_SRX550_MODELS
            /*
             * Register for CPU reset is different for SRX220.
             */
            temp = SRX220_ENABLE_CPU_RESET;
            /* enable CPU reset */
            cpld_write(SRX220_CPLD_SYS_RST_EN_REG, temp);
            temp = SRX220_RESET_CPU;
            cpld_write(SRX220_CPLD_SYS_RST_REG, temp);
            break;
        default:
            temp = cpld_read(SRX210_CPLD_RESET_REG);
            temp &= ~(SRX240_RESET_CPU);
            cpld_write(SRX210_CPLD_RESET_REG, temp);
            break;
    }

    /* give a 100 ms delay for sequence to complete */
    udelay(100000);
}

void
srxsme_board_reset (void)
{
    DECLARE_GLOBAL_DATA_PTR;

    /*
     * Get the mask for which devices should be reset
     * based on the board id.
     */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX100_MODELS
        srxle_board_reset(SRX100_RESET_DEVICES);
        break;

    CASE_ALL_SRX110_MODELS
        srxle_board_reset(SRX110_RESET_DEVICES);
        break;

    CASE_ALL_SRX210_MODELS
        srxle_board_reset(SRX210_RESET_DEVICES);
        break;
    
    CASE_ALL_SRX220_MODELS
    CASE_ALL_SRX550_MODELS
        srxle_board_reset(SRX220_RESET_DEVICES);
        break;

    CASE_ALL_SRX240_MODELS
        srxle_board_reset(SRX240_RESET_DEVICES);
        break;

    default:
        break;
    }

    /* do a CIU soft reset, if above fails */
    octeon_cpu_reset();
}

uint8_t
is_mpim_present (uint8_t mpim_number)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t val;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX220_MODELS
    CASE_ALL_SRX550_MODELS
        if (mpim_number >= SRX220_TOTAL_MPIMS)
            return 0;

        val = cpld_read(SRX220_MPIM_DETECT_REG);
        return(!(val & (1 << mpim_number)));

        break;
    CASE_ALL_SRX240_MODELS
        if (mpim_number >= SRX240_TOTAL_MPIMS)
            return 0;

        /* select the mpim slot */
        cpld_write(SRX240_MPIM_SEL_MUX_REG,
                   srx240_mpim_select_mask[mpim_number]);
        udelay(10);
        break;
    }

    /* read the presence status */
    val = cpld_read(SRX210_CPLD_MISC_REG);
    
    return (!(val & JSRXNLE_MPIM_PRESENCE_MASK)); 
}

void 
turn_off_jsrxnle_mpim_leds (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t val, counter = 0;
    volatile uint8_t *mpim_reg;
    uint32_t *led_registers;

    /* get the register addresses */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX210_MODELS
        led_registers = srx210_mpim_led_registers;
        break;
    
    CASE_ALL_SRX220_MODELS
    CASE_ALL_SRX550_MODELS
        led_registers = srx220_mpim_led_registers;
        break;

    CASE_ALL_SRX240_MODELS
        led_registers = srx240_mpim_led_registers;
        break;
    default:
        return;
    }

    while(led_registers && *led_registers) {
        if (is_mpim_present(counter)) {
            mpim_reg = (uint8_t *)(*led_registers);
            val = *mpim_reg;
            val |= 0x02; /* turn off led */
            *mpim_reg = val;
        }

        led_registers++;
        counter++;
    }
}

void enable_jsrxnle_cpld_watchdog (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t val;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_MODELS
        /* Set 0th bit in CPLD register 3 to enable watchdog */
        val = cpld_read(3);
        cpld_write(3, (val | 0x01));
        break;
    CASE_ALL_SRX650_MODELS
	/* Not supported in SRX650 */
    default:
        break;
    }

    return;
}

void pat_jsrxnle_cpld_watchdog (void)
{
    DECLARE_GLOBAL_DATA_PTR;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_MODELS
        /* Clear CPLD register 4 to clear watchdog counter */
        cpld_write(4, 0x00);
        break;
    CASE_ALL_SRX650_MODELS
	/* Not supported in SRX650 */
    default:
        break;
    }

    return;
}

void disable_jsrxnle_watchdog (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t val;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRXLE_MODELS
        /* Clear 0th bit in CPLD register 3 to disable watchdog */
        val = cpld_read(3);
        cpld_write(3, val & 0xfe);
        break;
    CASE_ALL_SRX650_MODELS
        /* Doing it here for now. Needs to be moved to new function */
        jsrxnle_uboot_done();
    default:
        break;
    }

    return;
}

void reload_cpld_watchdog (int val)
{
    if (val == PAT_WATCHDOG) {
	pat_jsrxnle_cpld_watchdog();
    } else if (val == DISABLE_WATCHDOG) {
	/*
	 * disable watchdog
	 */
	disable_jsrxnle_watchdog();
    } else if (val == ENABLE_WATCHDOG) {
	/*
	 * enable watchdog
	 */
	enable_jsrxnle_cpld_watchdog();
    }
}

#if(defined ( BOARD_EEPROM_TWSI_ADDR ) || defined ( MIDPLANE_BOARD_EEPROM_TWSI_ADDR ))

uchar jsrxnle_read_eeprom (uchar offset)
{
    return read_eeprom(offset);
}
#endif

#ifdef CONFIG_JSRXNLE
void enableall_sectors (void) 
{
    uint16_t board_type;
    DECLARE_GLOBAL_DATA_PTR;
    
    /* check for board type */
    board_type = gd->board_desc.board_type;
    switch (board_type) {
    CASE_ALL_SRXLE_MODELS
        jsrxnle_enableall_sectors();
        break;
    CASE_ALL_SRX650_MODELS
        srx650_enableall_sectors();
        break;
    default:
        printf("Entire bootflash access not allowed\n");
    }
    return;
}

void srx650_send_booted_signal (void)
{
    /* Set BOOTED pin */
    octeon_gpio_cfg_output(SRX650_GPIO_BOOTED_PIN);
    octeon_gpio_set(SRX650_GPIO_BOOTED_PIN);

    /* Drive relinquish bit low. It gives messages to PLD
     * that this RE wants to contest for mastership
     */
    octeon_gpio_cfg_output(SRX650_GPIO_RELINQUISH_PIN);
    octeon_gpio_clr(SRX650_GPIO_RELINQUISH_PIN);
}

void srx650_send_signal_to_master (void)
{
    uint8_t cpld_reg_val = 0;

    /* First send signal to master and then wait for  
     * this RE to become master
     */
    cpld_reg_val = cpld_read(SRX650_CPLD_POWER_REG);
    udelay(100000); /* wait 100 millisecond */

    /* First drive the bit low */
    SRX650_CLR_A_B_BIT(cpld_reg_val);
    cpld_write(SRX650_CPLD_POWER_REG, cpld_reg_val);

    udelay(100000); /* wait 100 millisecond */

    /* Now wait here till this RE becomes master */
    do {
        cpld_reg_val = cpld_read(SRX650_CPLD_MASTER_CTL_REG);
        udelay(10000); /* wait 10 millisecond */
    } while(!SRX650_IS_MASTER(cpld_reg_val));

    /* Now drive the bit high again */
    SRX650_SET_A_B_BIT(cpld_reg_val);
    cpld_write(SRX650_CPLD_POWER_REG, cpld_reg_val);

    return;
}

void srx650_wait_for_master (void)
{
    uint8_t cpld_reg_val = 0;
    int count = SRX650_MASTER_WAIT_CNT;

    /* If other RE is not present, then current is master. 
     * No need to wait. Otherwise wait it to becomes master
     */
    cpld_reg_val = cpld_read(SRX650_CPLD_MASTER_CTL_REG);
    if (!SRX650_IS_OTHER_RE_PRESENT(cpld_reg_val) || 
        SRX650_IS_MASTER(cpld_reg_val)) {
        return;
    }

    udelay(10000); /* wait 10 millisecond */

    do {
        cpld_reg_val = cpld_read(SRX650_CPLD_MASTER_CTL_REG);
        udelay(10000); /* wait 10 millisecond */
    } while(!SRX650_IS_MASTER(cpld_reg_val) && --count);

    /* If still not master then send signal to master */
    /* Commented out for now. Will enable with other dual RE code */
    /*if (!SRX650_IS_MASTER(cpld_reg_val)) {
        srx650_send_signal_to_master();
    }*/
    return;
}

void srx650_send_relinquish (void)
{
    uint8_t cpld_reg_val = 0;

    /* If other RE is not present, then return */
    cpld_reg_val = cpld_read(SRX650_CPLD_MASTER_CTL_REG);
    if (!SRX650_IS_OTHER_RE_PRESENT(cpld_reg_val)) {
        return;
    }
    /* Send reliquish signal and then wait for some 100ms
     * By that time other RE would become master. But again
     * drive the pin back low to contest again for mastership 
     */
    octeon_gpio_cfg_output(SRX650_GPIO_RELINQUISH_PIN);
    octeon_gpio_set(SRX650_GPIO_RELINQUISH_PIN);
    udelay(100000); /* wait 100 millisecond */
    octeon_gpio_clr(SRX650_GPIO_RELINQUISH_PIN);
    return;
}

void jsrxnle_uboot_done (void)
{
    uint8_t cpld_reg_val = 0;
    DECLARE_GLOBAL_DATA_PTR;

    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX650_MODELS
        cpld_reg_val = cpld_read(SRX650_CPLD_HRTBT_REG);
        SRX650_SET_UBOOT_DONE(cpld_reg_val);
        cpld_write(SRX650_CPLD_HRTBT_REG, cpld_reg_val);
        break;
    default:
        break;
    }
    return;
}

int srx650_is_master (void)
{
    uint8_t cpld_reg_val = 0;

    cpld_reg_val = cpld_read(SRX650_CPLD_MASTER_CTL_REG);
    return (SRX650_IS_MASTER(cpld_reg_val));
}

int srx650_get_re_slot (void)
{
    uint8_t cpld_reg_val = 0;
    int     re_slot;

    cpld_reg_val = cpld_read(SRX650_CPLD_MASTER_CTL_REG);
    if (cpld_reg_val & SRX650_RE_SLOT_BIT) {
        re_slot = SRX650_RE_SLOT_UPPER;
    } else {
        re_slot = SRX650_RE_SLOT_LOWER;
    }
    return re_slot;
}

void srx650_set_sysio_power_led (void)
{
    (void)write_i2c(SRX650_SRE_POWER_PLD, SRX650_SYSIO_LED_REG1,
                    SRX650_SYSIO_LED1_INIT_MASK,
                    SRX650_I2C_GROUP_SYSTEM_IO);
}

/*
 * function to disable ciu watchdog
 */ 
void
disable_ciu_watchdog (void)
{
    uint64_t val;

    val = cvmx_read_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET));
    val &= ~WATCH_DOG_DISABLE_MASK;
    cvmx_write_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET), val);
}

/* 
 * function to enable ciu watchdog
 */ 
void
enable_ciu_watchdog (int mode)
{
    uint64_t val;

    val = (WATCH_DOG_ENABLE_MASK | mode);
    cvmx_write_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET), val);
}

/* 
 * function to tickle ciu watchdog
 */ 
void
pat_ciu_watchdog (void)
{
    uint64_t val = 0;

    cvmx_write_csr(CVMX_CIU_PP_POKEX(CIU_PP_POKE0_OFFSET), val);
}

/*
 * functions dumps ciu watchdog cout register
 */ 
void
dump_ciu_watchdog_reg (void)
{
    uint64_t val;
    uint32_t valu, vall;
    val = cvmx_read_csr(CVMX_CIU_WDOGX(CIU_WATCHDOG0_OFFSET));
    vall = (val & WATCH_DOG_LOWER_MASK);
    valu = (val >> 32);
    printf("watchdog reg 0 value: = %08x%08x\n", valu, vall);
}

/*
 * FIXME: POST framework while running from flash uses a reserved 
 * space in RAM for its flags. These flags are about boot mode
 * and we don't depend on them. So we are not using them right now.
 */
#ifdef CONFIG_POST && CONFIG_JSRXNLE
DECLARE_GLOBAL_DATA_PTR;
void 
post_word_store (ulong a)
{
#ifdef FIXME_LATER
    volatile long *save_addr_word = (volatile ulong *)(POST_STOR_WORD);
    *save_addr_word = a;
#endif
}

ulong 
post_word_load (void)
{
#ifndef FIXME_LATER
    ulong a = 0;
    return a;
#else
    volatile long *save_addr_word = (volatile ulong *)(POST_STOR_WORD);           
    return *save_addr_word;
#endif
}

int 
post_hotkeys_pressed (void)
{
    return 0; /* No hotkeys supported */
}
#endif /* CONFIG_POST && CONFIG_JSRXNLE */

void 
enable_external_cf (void)
{
    octeon_gpio_set(SRX650_GPIO_EXT_CF_PWR);
    octeon_gpio_set(SRX650_GPIO_IDE_CF_CS);
    octeon_gpio_set(SRX650_GPIO_EXT_CF_WE);
    octeon_gpio_cfg_output(SRX650_GPIO_CF_LED_L);
    octeon_gpio_clr(SRX650_GPIO_CF_LED_L);
}

void 
disable_external_cf (void)
{
    octeon_gpio_clr(SRX650_GPIO_EXT_CF_PWR);
    octeon_gpio_clr(SRX650_GPIO_IDE_CF_CS);
    octeon_gpio_clr(SRX650_GPIO_EXT_CF_WE);
    octeon_gpio_cfg_output(SRX650_GPIO_CF_LED_L);
    octeon_gpio_set(SRX650_GPIO_CF_LED_L);
}

int 
srxle_cf_present (void)
{
    uint8_t val;

    val = cpld_read(SRX220_CPLD_CF_STATUS_REG);
    return (val & SRX220_CF_PRESENT_MASK);
}

void 
srxle_cf_enable (void)
{
    u_int8_t val;

    val = cpld_read(SRX220_CPLD_CF_CTRL_REG);

    /* Power on the CF. */
    val |= SRX220_CF_ENABLE_PWR;
    cpld_write(SRX220_CPLD_CF_CTRL_REG, val);

    /* Enable the CF for access. */
    val |= SRX220_CF_ENABLE; 
    cpld_write(SRX220_CPLD_CF_CTRL_REG, val);
}

void
srxle_cf_disable (void)
{
    u_int8_t val;

    /* Set the bits in the mask register to enable the control register */
    val = cpld_read(SRX220_CPLD_CF_CTRL_MASK_REG);
    val |= SRX220_CF_ENABLE | SRX220_CF_ENABLE_PWR;
    cpld_write(SRX220_CPLD_CF_CTRL_MASK_REG, val);

    val = cpld_read(SRX220_CPLD_CF_CTRL_REG);

    /* Disable CF access. */
    val &= ~SRX220_CF_ENABLE;
    cpld_write(SRX220_CPLD_CF_CTRL_REG, val);

    /* Power off the CF. */
    val &= ~SRX220_CF_ENABLE_PWR;
    cpld_write(SRX220_CPLD_CF_CTRL_REG, val);
}

#endif
