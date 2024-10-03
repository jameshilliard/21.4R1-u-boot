/*
 * Copyright (c) 2010-2013, Juniper Networks, Inc.
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */


#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <cpld.h>


DECLARE_GLOBAL_DATA_PTR;

/* boot cpld access functions on SCB and LC */
int btcpld_read(uint8_t offset, uint8_t *val)
{
	uint8_t *btcpld;

	if (EX62XX_SCB) {
		btcpld = (uint8_t *)CFG_SCB_BTCPLD_BASE;
	} else if (EX62XX_LC) {
		btcpld = (uint8_t *)CFG_LC_BTCPLD_BASE;
	} else {
		return -1;
	}

	*val = in_8(btcpld + offset);

	return 0;
}

int btcpld_write(uint8_t offset, uint8_t val)
{
	uint8_t *btcpld;

	if (EX62XX_SCB) {
		btcpld = (uint8_t *)CFG_SCB_BTCPLD_BASE;
	} else if (EX62XX_LC) {
		btcpld = (uint8_t *)CFG_LC_BTCPLD_BASE;
	} else {
		return -1;
	}

	out_8((btcpld + offset), val);

	return 0;
}

/* control cpld access functions */

int ctlcpld_read(uint8_t offset, uint8_t *val)
{
	*val = in_8((uint8_t *)(CFG_LC_CTLCPLD_BASE + offset));
	return 0;
}

int ctlcpld_write(uint8_t offset, uint8_t val)
{
	out_8((CFG_LC_CTLCPLD_BASE + offset), val);
	return 0;
}

int xcpld_read(uint8_t offset, uint8_t *val)
{
	*val = in_8((uint8_t *)(CFG_LC_XCPLD_BASE + offset));
	return 0;
}

int xcpld_write(uint8_t offset, uint8_t val)
{
	out_8((CFG_LC_XCPLD_BASE + offset), val);
	return 0;
}
