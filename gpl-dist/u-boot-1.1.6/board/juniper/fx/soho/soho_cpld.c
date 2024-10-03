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

/* Chun Ng (cng@juniper.net) */

#include "soho_cpld.h"
#include "soho_cpld_map.h"
#include "soho_lcd.h"
#include "soho_board.h"

extern uint8_t fx_is_pa();
extern uint8_t fx_is_wa();
extern uint8_t *cpld_iobase;

uint8_t *mcpld_base;
uint8_t *scpld_base;
uint8_t *reset_cpld_base;

void
soho_cpld_init (struct pio_dev *mcpld, struct pio_dev *scpld, struct pio_dev *acc_fpga,
                struct pio_dev *reset_cpld)
{
    phoenix_reg_t *flash_mmio = (phoenix_reg_t *)(DEFAULT_RMI_PROCESSOR_IO_BASE +
                                            PHOENIX_IO_FLASH_OFFSET);
    volatile uint32_t reg_val;

    /* master cpld, 8 bit and demux access */
    phoenix_write_reg(flash_mmio, 0x20 + mcpld->chip_sel, 0);

    /* aligned OE and CS to be at the same clock signal */
    reg_val = 0x4f240e22;
    reg_val = phoenix_write_reg(flash_mmio, 0x30 + mcpld->chip_sel, reg_val);

    reg_val = phoenix_read_reg(flash_mmio, 0x40 + mcpld->chip_sel);
    reg_val &= ~(0xFFF);
    reg_val |= 0x208;  
    phoenix_write_reg(flash_mmio, 0x40 + mcpld->chip_sel, reg_val);
    mcpld_base = (uint8_t *)mcpld->iobase;
    debug("%s: mcpld_base=0x%x\n", __func__, mcpld_base);

    cpld_iobase = mcpld_base;

    /* slave cpld, 8 bit and demux access */
    phoenix_write_reg(flash_mmio, 0x20 + scpld->chip_sel, 0);

    /* aligned OE and CS to be at the same clock signal */
    reg_val = 0x4f240e22;
    reg_val = phoenix_write_reg(flash_mmio, 0x30 + scpld->chip_sel, reg_val);

    reg_val = phoenix_read_reg(flash_mmio, 0x40 + scpld->chip_sel);
    reg_val &= ~(0xFFF);
    reg_val |= 0xCB2;
    phoenix_write_reg(flash_mmio, 0x40 + scpld->chip_sel, reg_val);

    scpld_base = (uint8_t *)scpld->iobase;
    debug("%s: scpld_base=0x%x\n", __func__, scpld_base);

    if (fx_is_pa()) {
        /* acc fpag 8 bit and demux access */
        phoenix_write_reg(flash_mmio, 0x20 + acc_fpga->chip_sel, 0);

        /* aligned OE and CS to be at the same clock signal */
        reg_val = 0x4f240e22;
        reg_val = phoenix_write_reg(flash_mmio, 0x30 + acc_fpga->chip_sel, reg_val);

        reg_val = phoenix_read_reg(flash_mmio, 0x40 + acc_fpga->chip_sel);
        reg_val &= ~(0xFFF);
        reg_val |= 0xCB2;
        phoenix_write_reg(flash_mmio, 0x40 + acc_fpga->chip_sel, reg_val);
    }

    if (fx_has_reset_cpld()) {
        /* reset cpld 8 bit and demux access */
        phoenix_write_reg(flash_mmio, 0x20 + reset_cpld->chip_sel, 0);

        /* aligned OE and CS to be at the same clock signal */
        if (fx_is_pa()) {
            reg_val = 0xFFC00E22;
        } else {
            reg_val = 0x4F240E22;
        }
        reg_val = phoenix_write_reg(flash_mmio, 0x30 + reset_cpld->chip_sel, reg_val);

        reg_val = phoenix_read_reg(flash_mmio, 0x40 + reset_cpld->chip_sel);
        reg_val &= ~(0xFFF);
        if (fx_is_pa()) {
            reg_val |= 0x3CF;
        } else {
            reg_val |= 0xCB2;
        }
        phoenix_write_reg(flash_mmio, 0x40 + reset_cpld->chip_sel, reg_val);
        reset_cpld_base = (uint8_t *)reset_cpld->iobase;
        debug("%s: reset_cpld_base=0x%x\n", __func__, reset_cpld_base);
    }
}

static boolean
soho_cpld_is_reset_cpld_reg (uint8_t reg_id)
{
    if (fx_is_soho()) {
        switch (reg_id) {
        case SOHO_RCPLD_CPU_RST ... SOHO_RCPLD_MISC:
            return TRUE;

        case SOHO_RCPLD_SCRATCH:
            return TRUE;

        default:
            return FALSE;
        }
    }

    if (fx_is_pa()) {
        switch (reg_id) {
        case QFX3500_RST_REG_0 ... QFX3500_RST_REG_3:
            return TRUE;

        default:
            return FALSE;
        }
    }
}

static boolean
soho_cpld_is_mcpld_reg (uint8_t reg_id)
{
    if (reg_id >= SOHO_MCPLD_REG_FIRST && reg_id <= SOHO_MCPLD_REG_LAST) {
        return TRUE;
    }

    return FALSE;
}

static boolean
soho_cpld_is_scpld_reg (uint8_t reg_id)
{
    if ((reg_id >= SOHO_CPLD_REG_FIRST) && (reg_id <= SOHO_CPLD_REG_LAST)) {
        if (!soho_cpld_is_mcpld_reg(reg_id)) {
            return TRUE;
        }
    }

    return FALSE;
}

soho_cpld_status_t
soho_cpld_read (uint8_t reg_id, uint8_t *reg_val)
{
    if (reg_val == NULL) {
        return (SOHO_CPLD_NULL);
    }

    *reg_val = 0;

    if (fx_has_reset_cpld()) {
        if (soho_cpld_is_reset_cpld_reg(reg_id)) {
            *reg_val = fx_cpld_read(reset_cpld_base, reg_id);
            return SOHO_CPLD_OK;
        }
    }

    if (soho_cpld_is_mcpld_reg(reg_id)) {
        *reg_val = fx_cpld_read(mcpld_base, reg_id);
        return SOHO_CPLD_OK;
    } 

    if (soho_cpld_is_scpld_reg(reg_id)) {
        *reg_val = fx_cpld_read(scpld_base, reg_id);
        return SOHO_CPLD_OK;
    } 

    return SOHO_CPLD_INVALID_REG;
}

soho_cpld_status_t
soho_cpld_write (uint8_t reg_id, uint8_t reg_val)
{
    if (fx_has_reset_cpld()) {
        if (soho_cpld_is_reset_cpld_reg(reg_id)) {
            fx_cpld_write(reset_cpld_base, reg_id, reg_val);
            return SOHO_CPLD_OK;
        }
    }

    if (soho_cpld_is_mcpld_reg(reg_id)) {
        fx_cpld_write(mcpld_base, reg_id, reg_val);
        return SOHO_CPLD_OK;
    } 

    if (soho_cpld_is_scpld_reg(reg_id)) {
        fx_cpld_write(scpld_base, reg_id, reg_val);
        return SOHO_CPLD_OK;
    } 

    return SOHO_CPLD_INVALID_REG;
}
    
uint8_t
soho_cpld_get_version (void)
{
    uint8_t version;

    soho_cpld_read(SOHO_MCPLD_VERSION, &version);
    return version;
}

static soho_cpld_status_t
soho_cpld_init_mgmt_phy (void)
{
    uint8_t data;
    soho_cpld_status_t status;
    
    /* slow down the MDC to 500KHZ */
    *(volatile uint8_t *)(0xbd90005a) = 0xC1;

    status = soho_cpld_read(SOHO_MCPLD_EXT_RST_0, &data);

    if (status != SOHO_CPLD_OK) {
        return status;
    }

    if (fx_is_soho()) {
        /* leave ASICs in reset */
        data |= ((1 << 4) | (1 << 5) | 0x7);
    } else {
        data |= ((1 << 4) | (1 << 5));
    }

    status = soho_cpld_write(SOHO_MCPLD_EXT_RST_0, data);
    udelay (2000 * 10);

    data &= ~((1 << 4) | (1 << 5));
    status = soho_cpld_write(SOHO_MCPLD_EXT_RST_0, data);

    return status;
}

static void 
soho_cpld_init_10g_phy (void)
{
    soho_cpld_write(SOHO_SCPLD_10G_S1_PHY_RST, 0xC0);          
    soho_cpld_write(SOHO_SCPLD_10G_S1_PHY_RST, 0x0);          

    soho_cpld_write(SOHO_SCPLD_10G_S7_PHY_RST, 0xC0);          
    soho_cpld_write(SOHO_SCPLD_10G_S7_PHY_RST, 0x0);          
}

static void
pa_cpld_init_ml_phy (void)
{
    soho_cpld_write(SOHO_SCPLD_ML_PHY_RST, 0xFF);          
    udelay(100); /* 0.1ms delay */
    soho_cpld_write(SOHO_SCPLD_ML_PHY_RST, 0x0);          
    udelay(500000); /* 0.5 second delay */
}

static void
pa_cpld_init_ael_phy (void)
{
    uint8_t i;

    for (i = 0; i < 3; i++) {
        soho_cpld_write(SOHO_SCPLD_AEL_0_PHY_RST + i, 0xFF);
    }

    soho_cpld_write(SOHO_SCPLD_AEL_3_PHY_RST, 0xFF);
}

static void 
soho_cpld_set_left_fan_led (void)
{
    uint8_t reg_val;
    uint8_t fan_led_val;

    soho_cpld_read(SOHO_MCPLD_INTR_FAN_0, &reg_val);
    soho_cpld_write(SOHO_MCPLD_INTR_FAN_0, reg_val);
    soho_cpld_read(SOHO_MCPLD_INTR_FAN_0, &reg_val);

    soho_cpld_read(SOHO_MCPLD_FAN_LED, &fan_led_val);
    if (reg_val & 0xF) {
        fan_led_val &= ~(0x1 << 4);
        fan_led_val |= 0x1 << 5;
    } else {
        fan_led_val &= ~(0x1 << 5);
        fan_led_val |= 0x1 << 4;
    }
    soho_cpld_write(SOHO_MCPLD_FAN_LED, fan_led_val);
}

static void 
soho_cpld_set_right_fan_led (void)
{
    uint8_t reg_val;
    uint8_t fan_led_val;

    soho_cpld_read(SOHO_MCPLD_INTR_FAN_0, &reg_val);
    soho_cpld_write(SOHO_MCPLD_INTR_FAN_0, reg_val);
    soho_cpld_read(SOHO_MCPLD_INTR_FAN_0, &reg_val);

    soho_cpld_read(SOHO_MCPLD_FAN_LED, &fan_led_val);
    if (reg_val & 0xF0) {
        fan_led_val &= ~(0x1 << 0);
        fan_led_val |= 0x1 << 1;
    } else {
        fan_led_val &= ~(0x1 << 1);
        fan_led_val |= 0x1 << 0;
    }
    soho_cpld_write(SOHO_MCPLD_FAN_LED, fan_led_val);
}

static void 
soho_cpld_set_middle_fan_led (void)
{
    uint8_t reg_val;
    uint8_t fan_led_val;

    soho_cpld_read(SOHO_MCPLD_INTR_FAN_1, &reg_val);
    soho_cpld_write(SOHO_MCPLD_INTR_FAN_1, reg_val);
    soho_cpld_read(SOHO_MCPLD_INTR_FAN_1, &reg_val);

    soho_cpld_read(SOHO_MCPLD_FAN_LED, &fan_led_val);
    if (reg_val & 0xF) {
        fan_led_val &= ~(0x1 << 2);
        fan_led_val |= 0x1 << 3;
    } else {
        fan_led_val &= ~(0x1 << 3);
        fan_led_val |= 0x1 << 2;
    }
    soho_cpld_write(SOHO_MCPLD_FAN_LED, fan_led_val);
}

static void
soho_cpld_qfx3600_set_fan_led (void)
{
    uint8_t reg_val;

    soho_cpld_read(SOHO_MCPLD_FAN_PWR_PRE, &reg_val);

    if (reg_val & 0x10) {
        soho_cpld_set_right_fan_led();
    }

    if (reg_val & 0x20) {
        soho_cpld_set_middle_fan_led();
    }

    if (reg_val & 0x40) {
        soho_cpld_set_left_fan_led();
    }
}

soho_cpld_set_fan_led (void)
{
    uint8_t reg_val;

    soho_cpld_read(SOHO_MCPLD_FAN_PWR_PRE, &reg_val);

    if (reg_val & 0x10) {
        soho_cpld_set_left_fan_led();
    }

    if (reg_val & 0x20) {
        soho_cpld_set_middle_fan_led();
    }

    if (reg_val & 0x40) {
        soho_cpld_set_right_fan_led();
    }
}

static void
soho_cpld_set_pwr_status_led (void)
{
    uint8_t pwr_status_led;

    pwr_status_led = (1 << 1 | 1 << 2);
    soho_cpld_write(SOHO_SCPLD_LCD_LED, pwr_status_led);
}

void
soho_cpld_init_dev(void) 
{
    /* reset all slave CPLD to clear up
     * previous settings
     */
    if (fx_is_tor()) {
        soho_cpld_write(SOHO_MCPLD_EXT_RST_1, 0xFF);
        soho_cpld_write(SOHO_MCPLD_EXT_RST_1, 0x0);
        if(fx_is_wa())
            soho_cpld_qfx3600_set_fan_led();
        else
            soho_cpld_set_fan_led();
        soho_cpld_set_pwr_status_led();
    }

    if (fx_is_pa() || fx_is_wa()) {
        pa_cpld_init_ml_phy();
        pa_cpld_init_ael_phy();
    }
    soho_cpld_init_mgmt_phy();
#ifdef __RMI_BOOT2__
    soho_lcd_init();
#endif
}

static soho_cpld_status_t
soho_cpld_mdio_poll_busy (void)
{
    uint32_t wait_cnt = 0;
    uint8_t data;

    soho_cpld_read(SOHO_MCPLD_MDIO_CTRL_ADDR, &data);
    while ((data & 0x2) != 0x0) {
        udelay (1000 * 10);
        if (wait_cnt++ > 0xFF) {
            printf("ERROR: CPLD BUSY FLAG NOT CLEARED 0x%x\n", data);
            return SOHO_CPLD_MDIO_OP_BUSY;
        }
        soho_cpld_read(SOHO_MCPLD_MDIO_CTRL_ADDR, &data);
    }

    if ((data & 0x4) != 0x0) {
        printf("ERROR: Device not ack\n");
        return SOHO_CPLD_MDIO_NO_ACK;
    }

    return SOHO_CPLD_OK;
}

soho_cpld_status_t
soho_cpld_mdio_c22_read (uint32_t phy_addr, uint32_t reg_id, 
                         uint32_t *data, uint8_t time_para)
{
    uint8_t tmp_data = 0;
    soho_cpld_status_t status;

    soho_cpld_write(SOHO_MCPLD_MDIO_OP, ((time_para & 0xFF) | MDIO_C22_READ));
    soho_cpld_write(SOHO_MCPLD_MDIO_PHY_ADDR, (phy_addr & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_DEV_ADDR, (reg_id & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_CTRL_ADDR, 0x1);

    if ((status = soho_cpld_mdio_poll_busy()) != SOHO_CPLD_OK) {
        return status;
    }

    soho_cpld_read(SOHO_MCPLD_MDIO_HI_ADDR, &tmp_data);
    *data = (tmp_data & 0xFF) << 8;
    soho_cpld_read(SOHO_MCPLD_MDIO_LOW_ADDR, &tmp_data);
    *data |= tmp_data & 0xFF;

    return SOHO_CPLD_OK;
}

soho_cpld_status_t
soho_cpld_mdio_c22_write (uint32_t phy_addr, uint32_t reg_id, 
                          uint16_t data, uint8_t time_para)
{
    soho_cpld_write(SOHO_MCPLD_MDIO_OP, ((time_para & 0xFF) | MDIO_C22_WRITE));
    soho_cpld_write(SOHO_MCPLD_MDIO_PHY_ADDR, (phy_addr & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_DEV_ADDR, (reg_id & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_HI_ADDR, ((data >> 8) & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_LOW_ADDR, (data  & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_CTRL_ADDR, 0x1);

    return (soho_cpld_mdio_poll_busy());
}

soho_cpld_status_t
soho_cpld_mdio_c45_read (uint32_t dev_type, uint32_t phy_addr, 
                        uint32_t reg_id, uint16_t *data, uint32_t time_para)
{
    soho_cpld_status_t status;
    uint8_t tmp_data = 0;

    *data = 0;

    soho_cpld_write(SOHO_MCPLD_MDIO_OP, ((time_para & 0xFF) | MDIO_C45_ADDR));
    soho_cpld_write(SOHO_MCPLD_MDIO_PHY_ADDR, (phy_addr & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_DEV_ADDR, (dev_type & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_HI_ADDR, ((reg_id >> 8) & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_LOW_ADDR, (reg_id & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_CTRL_ADDR, 0x1);

    if ((status = soho_cpld_mdio_poll_busy()) != SOHO_CPLD_OK) {
        return status;
    }

    soho_cpld_write(SOHO_MCPLD_MDIO_OP, ((time_para & 0xFF) | MDIO_C45_READ));
    soho_cpld_write(SOHO_MCPLD_MDIO_PHY_ADDR, (phy_addr & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_DEV_ADDR, (dev_type & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_CTRL_ADDR, 0x1);

    if ((status = soho_cpld_mdio_poll_busy()) != SOHO_CPLD_OK) {
        return status;
    }

    soho_cpld_read(SOHO_MCPLD_MDIO_HI_ADDR, &tmp_data);
    *data = (tmp_data & 0xFF) << 8;
    soho_cpld_read(SOHO_MCPLD_MDIO_LOW_ADDR, &tmp_data);
    *data |= tmp_data & 0xFF;

    return SOHO_CPLD_OK;
}

soho_cpld_status_t
soho_cpld_mdio_c45_write (uint32_t dev_type, uint32_t phy_addr, 
                          uint32_t reg_id, uint16_t data, uint32_t time_para)
{
    soho_cpld_status_t status;

    soho_cpld_write(SOHO_MCPLD_MDIO_OP, ((time_para & 0xFF) | MDIO_C45_ADDR));
    soho_cpld_write(SOHO_MCPLD_MDIO_PHY_ADDR, (phy_addr & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_DEV_ADDR, (dev_type & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_HI_ADDR, ((reg_id >> 8) & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_LOW_ADDR, (reg_id & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_CTRL_ADDR, 0x1);

    if ((status = soho_cpld_mdio_poll_busy()) != SOHO_CPLD_OK) {
        return status;
    }

    soho_cpld_write(SOHO_MCPLD_MDIO_OP, ((time_para & 0xFF) | MDIO_C45_WRITE));
    soho_cpld_write(SOHO_MCPLD_MDIO_PHY_ADDR, (phy_addr & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_DEV_ADDR, (dev_type & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_HI_ADDR, ((data >> 8) & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_LOW_ADDR, (data  & 0xFF));
    soho_cpld_write(SOHO_MCPLD_MDIO_CTRL_ADDR, 0x1);

    return (soho_cpld_mdio_poll_busy());
}

void
soho_cpld_set_phy_led (uint32_t port_id, soho_phy_led_color color)
{
    uint8_t data;

    soho_cpld_read(SOHO_SCPLD_PHY_LED, &data);

    switch (port_id) {
    case 4:
        data &= ~(1 << 0 | 1 << 1);
        if (color == SOHO_PHY_LED_GREEN) {
            data |= 1 << 0;
        } else if (color == SOHO_PHY_LED_AMBER) {
            data |= 1 << 1;
        }
        break;

    case 6:
        data &= ~(1 << 4 | 1 << 5);
        if (color == SOHO_PHY_LED_GREEN) {
            data |= 1 << 4;
        } else if (color == SOHO_PHY_LED_AMBER) {
            data |= 1 << 5;
        }
        break;

    case 5:
        data &= ~(1 << 6 | 1 << 7);
        if (color == SOHO_PHY_LED_GREEN) {
            data |= 1 << 6;
        }  else if (color == SOHO_PHY_LED_AMBER) {
            data |= 1 << 7;
        }
        break;

    default:
        break;
    }

    soho_cpld_write(SOHO_SCPLD_PHY_LED, data);
}

/* 
 * Setting GPIO register so that MUX is selected to requested value.
 * This will enable read for Mgmt Board ID.
 * Parameters: mux value
 * Return value: original mux value
 *
 */
uint32_t soho_cpld_mdio_set_mux (const uint32_t mux) 
{
    uint32_t reg_val;
    uint32_t orig_mux;

    /* mux value can only be 6 bits max */
    if (mux > 0x3F) {
        printf("%s: gpio mux select 0x%x exceeds max\n",
               __func__,
               mux);
        return 0xFFFF;
    }    

    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);

    /* Using GPIO pins to set either MDIO or I2C MUX */
    mmio[2] |= ((1 << 6) | (0x7 << 7));
    reg_val = mmio[4];

    /* save original mux value */
    orig_mux = reg_val & 0x3F;

    /* delete least significant 6 bits -- MUX select */    
    reg_val &= ~(0x3F);
    /* write mux value to least significant 6 bits */
    reg_val |= mux;    

    mmio[3] = reg_val;
    udelay(5000); /* 5 ms */

    return orig_mux;
}

void
soho_cpld_select_phy_led (uint32_t port_id, int select)
{
    uint8_t data = 0;
    uint8_t reg_to_read = 0;

    switch (port_id) {
    case 6:
        reg_to_read = SOHO_SCPLD_SFP_C0_PRE;
        break;
    case 5:
        reg_to_read = SOHO_SCPLD_SFP_C1_PRE;
        break;
    default:
        return;
    }
    soho_cpld_read(reg_to_read, &data);
    if (select)
        data |= (1 << 3);
    else
        data &= ~(1 << 3);
    soho_cpld_write(reg_to_read, data);
}

