/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
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

#ifndef __OCTEON_SMI_H__
#define __OCTEON_SMI_H__

#define OCTEON_MAX_SMI_RETRY_COUNT      10
#define OCTEON_MAX_VALID_SMI_REG_ADDR   0x001F
#define OCTEON_SMI_CLOCK                25
#define OCTEON_SMI_CLOCK_VAL            0x1219
#define OCTEON_SMI_SEL_BIT              14
#define OCTEON_GPIO_BIT_CFG_TX_OE       0x1
#define OCTEON_SMI_SEL_BIT_MASK         (0x1 << OCTEON_SMI_SEL_BIT)

#define OCTEON_SRX220_SMI_CLOCK_VAL     0x1223

typedef enum {
    OCTEON_SMI_DEV_SWITCH,
    OCTEON_SMI_DEV_mPIM,
} smi_dev_t;

#define MPIM_OCTEON_SW_MAX_SMI_CLK_KHZ   (16667)
#define MPIM_OCTEON_SAMPLE_LOW           (2 << 8)
#define MPIM_OCTEON_SAMPLE_HIGH          (0 << 16)
#define MPIM_OCTEON_SAMPLE \
        (MPIM_OCTEON_SAMPLE_HIGH | MPIM_OCTEON_SAMPLE_LOW)

int octeon_smi_init(void);
uint16_t octeon_smi_write(uint16_t reg_adr, uint16_t dat);
uint16_t octeon_smi_read(uint16_t reg_adr, uint16_t *val);
#endif /* __OCTEON_SMI_H__ */
