/*
 * Copyright (c) 2010-2011, Juniper Networks, Inc.
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
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.
 */

#include <common.h>

#ifdef CONFIG_POST
#include <post.h>
#include "post_jsrxnle.h"

#if CONFIG_POST & CFG_POST_SKU_ID

int
sku_id_post_test (int flags)
{
    DECLARE_GLOBAL_DATA_PTR;
    unsigned int board_sku_id = 0, expected_sku_id = 0;

    /* Verify that the SKU ID for SRX110 matches the I2C ID */
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX110_MODELS
        board_sku_id = cpld_read(SRX110_CPLD_SKU_ID);
        expected_sku_id = compute_sku_id();

        if (board_sku_id & SRX110_SKU_ID_SUPERSET) {
            /*
             * For superset SKU the ID gets programmed in early_board_init
             * Just display the settings here for convenience
             */
            printf("SRX110 superset SKU detected\n");
            printf("Programmed SKU-ID=0x%02X for I2C-ID=0x%04X\n",
                   board_sku_id, gd->board_desc.board_type);
        } else {
            /* Not superset SKU: need to check if SKU-ID matches platform */
            if (board_sku_id != expected_sku_id) {
                post_log("SKU-ID Test: Failed\n"
                         "SKU-ID=0x%02X, Expected=0x%02X, I2C-ID=0x%04X\n",
                          board_sku_id, expected_sku_id,
                          gd->board_desc.board_type);
                SET_POST_RESULT(gd->post_result, SKU_ID_POST_RESULT_MASK,
                                POST_FAILED);
                return -1;
            }
        }
        break;
    default:
        break;
    }
    SET_POST_RESULT(gd->post_result, SKU_ID_POST_RESULT_MASK, POST_PASSED);
    return 0;
}
#endif /* CONFIG_POST & CFG_POST_SKU_ID */
#endif /* CONFIG_POST */

