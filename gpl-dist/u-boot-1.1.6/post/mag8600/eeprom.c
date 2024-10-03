/*
 * Copyright (c) 2011, Juniper Networks, Inc.
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

#include <common.h>

/*
 * EEPROM test
 *
 */

#ifdef CONFIG_POST

#include <post.h>

#if CONFIG_POST & CFG_POST_EEPROM

static enum { jedec_hi_bits, jedec_lo_bits };

static struct juniper_jedec_codes {
    unsigned char offset;
    unsigned char id;
} jsrx_jedec_codes[]  = {
    {0x00, 0x7F},
    {0x01, 0xB0}
};


/* PASS: Clear the bit. Fail: Set it */
#define SET_EEPROM_POST_RESULT(reg, result) \
	reg = (result == POST_PASSED \
	? (reg & (~EEPROM_POST_RESULT_MASK)) \
	: (reg | EEPROM_POST_RESULT_MASK))

#define INTER_BYTE_DELAY_MS	5000	//ms

int 
eeprom_post_test (int flags)
{
    DECLARE_GLOBAL_DATA_PTR;
    uint8_t offset = 0;
    uint8_t hi_code, lo_code;
    
    
    offset = jsrx_jedec_codes[jedec_hi_bits].offset;
    hi_code = mag8600_read_eeprom(offset); 
    
    udelay(INTER_BYTE_DELAY_MS);
    
    offset = jsrx_jedec_codes[jedec_lo_bits].offset;
    lo_code = mag8600_read_eeprom(offset);
    
    if ((hi_code != jsrx_jedec_codes[jedec_hi_bits].id ||
	 lo_code != jsrx_jedec_codes[jedec_lo_bits].id)) {
        post_log("EEPROM Test: Failed\n");
	SET_EEPROM_POST_RESULT(gd->post_result, POST_FAILED);
	return -1;
    }

    /* set result */
    SET_EEPROM_POST_RESULT(gd->post_result, POST_PASSED);

    return 0;
}

#endif /* CONFIG_POST & CFG_POST_EEPROM */
#endif /* CONFIG_POST */ 

