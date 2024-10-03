/*
 * Copyright (c) 2010, Juniper Networks, Inc.
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

#include "rmi/rmigmac.h"
#include "rmi/gmac.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"
#include "rmi/shared_structs.h"
#include "rmi/cpu_ipc.h"
#include "soho_misc.h"

void
soho_mdio_set_bcm5482 (void)
{
    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);

    mmio[2] |= ((1 << 6) | (0x7 << 7));
    mmio[3] =  (mmio[4] & ~((1 << 6) | (0x7 << 7)));
    udelay(5000);
}

void
soho_mdio_set_bcm5396 (void)
{
    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);
    u_int32_t reg_val;

    mmio[2] |= ((1 << 6) | (0x7 << 7));
    reg_val = mmio[4];
    reg_val  =  (reg_val & ~(1 << 6)) ;
#if 0
    /* just for a rework board */
    reg_val |= (0x3 << 7);
#else
    reg_val |= (0x1 << 7);
#endif
    mmio[3] = reg_val;
    udelay(5000);
    printf("%s: gpio init done mmio[4]=0x%x\n", __func__, mmio[4]);
}

void
soho_mdio_set_10G_PHY_1 (void)
{
    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);
    u_int32_t reg_val;

    mmio[2] |= ((1 << 6) | (0x7 << 7));
    reg_val = mmio[4];
    reg_val =  (reg_val & ~(1 << 6)) ;
    reg_val &=  ~(0x7 << 7) ;
    reg_val |= (0x1 << 7);
    mmio[3] = reg_val;
    udelay(5000);
    printf("%s: gpio init done mmio[4]=0x%x\n", __func__, mmio[4]);
}

void
soho_mdio_set_10G_PHY_7 (void)
{
    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);
    u_int32_t reg_val;

    mmio[2] |= ((1 << 6) | (0x7 << 7));
    reg_val = mmio[4];
    reg_val  =  (reg_val & ~(1 << 6)) ;
    reg_val |= (0x7 << 7);
    mmio[3] = reg_val;
    udelay(5000);
    printf("%s: gpio init done mmio[4]=0x%x\n", __func__, mmio[4]);
}

void
soho_mdio_set_10G_phy (u_int32_t value)
{
    phoenix_reg_t *mmio = (phoenix_reg_t *)
                          (DEFAULT_RMI_PROCESSOR_IO_BASE +
                           PHOENIX_IO_GPIO_OFFSET);
    u_int32_t reg_val;

    mmio[2] |= ((1 << 6) | (0x7 << 7));
    reg_val = mmio[4];
    reg_val =  (reg_val & ~(1 << 6)) ;
    reg_val &=  ~(0x7 << 7) ;
    reg_val |= ((value & 0x7) << 7);
    mmio[3] = reg_val;
    udelay(5000);
    printf("%s: gpio init done mmio[4]=0x%x\n", __func__, mmio[4]);
}
