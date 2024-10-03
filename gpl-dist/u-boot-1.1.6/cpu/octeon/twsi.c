/*
 * Copyright (c) 2014, Juniper Networks, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not ,see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0,html>  
 */

#include <config.h>
#include <common.h>
#include <octeon_hal.h>
#include <i2c.h>

int
i2c_write8_generic (uint8_t device, uint16_t offset, 
                        uint8_t val, uint8_t twsii_bus)
{
    uint64_t val_1;
    
    val_1= 0x8000000000000000ull | ( 0x0ull << 57) | ( 0x1ull << 55) | 
        ( 0x1ull << 52) | (((uint64_t)device) << 40) | 
        (((uint64_t)offset) << 8) | val;

    cvmx_write_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus), val_1 ); // tell twsii to do the write
    while (cvmx_read_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus))&0x8000000000000000ull)
        ;
    val_1 = cvmx_read_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus));
    if (!(val_1 & 0x0100000000000000ull)) {
        printf("twsii write (1byte) failed\n");
        return -1;
    }
    return(0);
}

int
i2c_read8_generic (uint8_t device, uint16_t offset, 
                   uint8_t *val, uint8_t twsii_bus) 
{
    uint64_t res;

    cvmx_write_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus),
                   0x8000000000000000ull | (((uint64_t)device) << 40) | offset);
    while(cvmx_read_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus))&0x8000000000000000ull )
        ;
    res = cvmx_read_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus));
    if (!(res & 0x0100000000000000ull)) {
        printf("twsii read (1byte) failed\n");
        return -1;
    }

    cvmx_write_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus),0x8100000000000000ull | (((uint64_t)device) << 40));
    /* tell twsii to do the read */
    while (cvmx_read_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus))&0x8000000000000000ull)
        ;
    res = cvmx_read_csr(CVMX_MIO_TWSX_SW_TWSI(twsii_bus));
    if (!(res & 0x0100000000000000ull)) {
        printf("twsii read (1byte) failed\n");
        return -1;
    } else {
        *val = (uint8_t)(res & 0xFF);
    }
    return 0;

}
