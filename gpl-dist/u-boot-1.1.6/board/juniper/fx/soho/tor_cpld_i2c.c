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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/* Chun Ng, cng@juniper.net */


#include <command.h>
#include <common.h>
#include <common/post.h>
#include <common/i2c.h>
#include <common/fx_common.h>
#include "soho_board.h"
#include "soho_cpld.h"
#include "soho_cpld_map.h"

#include "tor_cpld_i2c.h"

static uint32_t tor_cpld_set_nop(volatile uint32_t *mem_addr);
static uint32_t tor_cpld_i2c_get_rw_type(struct tor_cpld_i2c_req *i2c_req, 
                                          uint8_t *rw_type);
static uint32_t tor_cpld_i2c_set_r(struct tor_cpld_i2c_req *i2c_req, 
                                    uint8_t rw_type, volatile uint32_t *mem_addr, 
                                    uint32_t *data_addr);
static uint32_t tor_cpld_i2c_wait(void);
static void tor_cpld_i2c_go(void);
static void tor_cpld_i2c_stop(void);
static uint32_t tor_cpld_i2c_set_mux(struct tor_cpld_i2c_req *i2c_req, 
                                      volatile uint32_t *mem_addr);
static uint32_t tor_cpld_i2c_set_wr(struct tor_cpld_i2c_req *i2c_req, 
                                    uint8_t rw_type, 
                                    volatile uint32_t *mem_addr,
                                    volatile uint32_t *data_addr);
static uint32_t tor_cpld_i2c_set_eos(volatile uint32_t *mem_addr);
static uint32_t tor_cpld_i2c_set_os(struct tor_cpld_i2c_req *i2c_req, 
                                     volatile uint32_t *mem_addr,
                                     uint32_t *data_addr);
static uint32_t tor_cpld_i2c (struct tor_cpld_i2c_req *i2c_req);
static uint32_t tor_cpld_i2c_fetch_result(struct tor_cpld_i2c_req *i2c_req, 
                                           uint32_t data_addr);
static uint32_t tor_cpld_i2c_set_delay(struct tor_cpld_i2c_req *i2c_req, 
                                        volatile uint32_t *mem_addr);

extern uint8_t *cpld_iobase;
uint32_t tor_cpld_i2c_debug_flag = 0;

#ifdef __RMI_BOOT2__
#define tor_cpld_i2c_debug(fmt, args...)                    \
    do {                                                    \
        if (getenv("i2c_boot_verbose") || tor_cpld_i2c_debug_flag) {   \
            printf(fmt ,##args);                            \
        }                                                   \
    } while (0)
#else
#define tor_cpld_i2c_debug(fmt, args...)                    \
    do {                                                    \
        if (tor_cpld_i2c_debug_flag) {                      \
            printf(fmt ,##args);                            \
        }                                                   \
    } while (0)
#endif

/* read from the i2c controller */
uint32_t
tor_cpld_i2c_controller_read (uint32_t addr, uint32_t *data)
{
    uint8_t addr1_val, tmp_buf;

    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_CMD_ADDR_0, addr & 0xFF); 

    addr1_val = (addr >> 8) & 0x3F;
    addr1_val |= TOR_CPLD_I2C_CONTROLLER_R;
    addr1_val |= TOR_CPLD_I2C_CONTROLLER_GO;
    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_CMD_ADDR_1, addr1_val);

    tmp_buf = fx_cpld_read(cpld_iobase, TOR_MCPLD_I2C_DATA_0);
    *data = tmp_buf & 0xFF;
    tmp_buf = fx_cpld_read(cpld_iobase, TOR_MCPLD_I2C_DATA_1);
    *data |= (tmp_buf << 8) & 0xFF00;
    tmp_buf = fx_cpld_read(cpld_iobase, TOR_MCPLD_I2C_DATA_2);
    *data |= (tmp_buf << 16) & 0xFF0000;
    tmp_buf = fx_cpld_read(cpld_iobase, TOR_MCPLD_I2C_DATA_3);
    *data |= (tmp_buf << 24) & 0xFF000000;

    return TOR_CPLD_OK;
}
    
/* write to the i2c controller */
uint32_t 
tor_cpld_i2c_controller_write (uint32_t addr, uint32_t cmd_data)
{
    uint8_t addr1_val;

    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_DATA_0, cmd_data & 0xFF);
    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_DATA_1, (cmd_data >> 8) & 0xFF);
    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_DATA_2, (cmd_data >> 16) & 0xFF);
    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_DATA_3, (cmd_data >> 24) & 0xFF);

    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_CMD_ADDR_0, addr & 0xFF); 

    addr1_val = (addr >> 8) & 0x3F;
    addr1_val |= TOR_CPLD_I2C_CONTROLLER_W;
    addr1_val |= TOR_CPLD_I2C_CONTROLLER_GO;
    fx_cpld_write(cpld_iobase, TOR_MCPLD_I2C_CMD_ADDR_1, addr1_val);

    return TOR_CPLD_OK;
}

/* find out which i2c read/write type base on i2c mode/length */
static uint32_t
tor_cpld_i2c_get_rw_type (struct tor_cpld_i2c_req *i2c_req, uint8_t *rw_type)
{
    if (i2c_req->read_op) {
        if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF ||
            i2c_req->i2c_mode & TOR_CPLD_I2C_32_OFF) {
            *rw_type = TOR_CPLD_I2C_R_5;
            return TOR_CPLD_OK;
        }

        switch (i2c_req->len) {
        case 1:
            if (i2c_req->i2c_mode & TOR_CPLD_I2C_RSTART) {
                *rw_type = TOR_CPLD_I2C_R_0;
            } else {
                *rw_type = TOR_CPLD_I2C_R_2;
            }
            return TOR_CPLD_OK;

        case 2:
            if (i2c_req->i2c_mode & TOR_CPLD_I2C_RSTART) {
                *rw_type = TOR_CPLD_I2C_R_1;
            } else {
                *rw_type = TOR_CPLD_I2C_R_3;
            }
            return TOR_CPLD_OK;

        case 3 ... TOR_CPLD_I2C_MAX_BYTE:
            if (i2c_req->i2c_mode & TOR_CPLD_I2C_RSTART) {
                printf("%s: not support len: %d for TOR_CPLD_I2C_RSTAR\n", 
                        __func__, i2c_req->len);
                return TOR_CPLD_ERR;
            }
            *rw_type = TOR_CPLD_I2C_R_5;
            return TOR_CPLD_OK;

        default:
            printf("%s: not support len: %d\n", __func__, i2c_req->len);
            return TOR_CPLD_ERR;
        }
    } else {
        if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF ||
            i2c_req->i2c_mode & TOR_CPLD_I2C_32_OFF) {
            *rw_type = TOR_CPLD_I2C_W_5;
            return TOR_CPLD_OK;
        }

        if (i2c_req->i2c_mode & TOR_CPLD_I2C_NO_OFF) {
            i2c_req->offset = i2c_req->data_buf[0];
            i2c_req->data_buf = &(i2c_req->data_buf[1]);
            i2c_req->len -= 1;
        }

        switch (i2c_req->len) {
        case 0:
            *rw_type = TOR_CPLD_I2C_W_0;
            return TOR_CPLD_OK;

        case 1:
            *rw_type = TOR_CPLD_I2C_W_1;
            return TOR_CPLD_OK;

        case 2:
            *rw_type = TOR_CPLD_I2C_W_2;
            return TOR_CPLD_OK;

        case 3 ... TOR_CPLD_I2C_MAX_BYTE:
            *rw_type = TOR_CPLD_I2C_W_5;
            return TOR_CPLD_OK;

        default:
            printf("%s: not support len: %d\n", __func__, i2c_req->len);
            return TOR_CPLD_ERR;
        }
    }

    return TOR_CPLD_ERR;
}

/* the detail of forming instruction portion of a i2c read */
static uint32_t
tor_cpld_i2c_form_rd_instr (struct tor_cpld_i2c_req *i2c_req, uint8_t rw_type)
{
    uint32_t instruction = 0;

    switch (rw_type) {
    case TOR_CPLD_I2C_R_0:
    case TOR_CPLD_I2C_R_1:
    case TOR_CPLD_I2C_R_2:
    case TOR_CPLD_I2C_R_3:
    case TOR_CPLD_I2C_R_5:
        instruction |= (i2c_req->phy_addr & TOR_CPLD_I2C_ADDR_MASK) << 
                        TOR_CPLD_I2C_ADDR_SHIFT;

        if (rw_type == TOR_CPLD_I2C_R_0 || rw_type == TOR_CPLD_I2C_R_1) {  
            instruction |= (i2c_req->offset & TOR_CPLD_I2C_CMD_OFF_MASK) << 
                            TOR_CPLD_I2C_CMD_OFF_SHIFT;
        }

        if (rw_type == TOR_CPLD_I2C_R_5) {
            instruction |= (i2c_req->len & TOR_CPLD_I2C_BYTE_CNT_MASK) << 
                           TOR_CPLD_I2C_BYTE_CNT_SHIFT;

            instruction |= ((i2c_req->len >> 6) & TOR_CPLD_I2C_BYTE_CNT_1_MASK) << 
                           TOR_CPLD_I2C_BYTE_CNT_1_SHIFT;
        }
        break;

    case TOR_CPLD_I2C_R_4:
        break;

    default:
        panic("%s[%d]: Not support RW type %d\n", __func__, __LINE__, rw_type);
    }

    instruction |= rw_type << TOR_CPLD_I2C_RW_TYPE_SHIFT;
    instruction |= TOR_CPLD_I2C_VALID_BIT;

    return instruction;
}

/* set a data portion for a i2c read operation */
static uint32_t
tor_cpld_i2c_form_r_data (struct tor_cpld_i2c_req *i2c_req, uint8_t rw_type)
{
    uint32_t data = 0;

    return data;
}

/* set both instruction and data portion for a i2c read operation*/
static uint32_t
tor_cpld_i2c_set_r (struct tor_cpld_i2c_req *i2c_req, uint8_t rw_type, 
                     volatile uint32_t *mem_addr, uint32_t *data_addr)
{
    uint32_t instruction = 0, data = 0, i;

    if (*mem_addr % 2) {
        panic("%s[%d]: mem_addr 0x%x is not an even number\n", 
                __func__, __LINE__, *mem_addr); 
    }
    data = tor_cpld_i2c_form_r_data(i2c_req, rw_type);

    tor_cpld_i2c_controller_write(*mem_addr, data);

    *data_addr = *mem_addr;

    (*mem_addr)++;

    instruction = tor_cpld_i2c_form_rd_instr(i2c_req, rw_type);

    tor_cpld_i2c_controller_write(*mem_addr, instruction);

    (*mem_addr)++;

    tor_cpld_i2c_debug("%s[%d]: mem_addr=0x%x\n", 
                 __func__, __LINE__, *mem_addr);

    for (i = 0; i < i2c_req->len; i+=2) {
        tor_cpld_set_nop(mem_addr); 
    }

    return TOR_CPLD_OK;
}
        
/* form a data portion of the i2c write instruction */
static uint32_t
tor_cpld_i2c_form_wr_data (struct tor_cpld_i2c_req *i2c_req, uint8_t rw_type)
{
    uint32_t data = 0;

    switch (rw_type) {
    case TOR_CPLD_I2C_W_0:
        data = 0;
        break;

    case TOR_CPLD_I2C_W_1:
        data = i2c_req->data_buf[0] & 0xFF;
        break;

    case TOR_CPLD_I2C_W_2:
        data = i2c_req->data_buf[0] & 0xFF;
        data |= (i2c_req->data_buf[1] << 8) & 0xFF00;
        break;

    case TOR_CPLD_I2C_W_4:
        data |= (i2c_req->i2c_mux.bus) << 8;
        data |= (i2c_req->i2c_mux.sseg) << 4;
        data |= (i2c_req->i2c_mux.pseg);
        break;

    case TOR_CPLD_I2C_W_5:
        if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF) {
            data = i2c_req->offset & 0xFF;
            data |= (i2c_req->data_buf[0] << 8) & 0xFF00;
        } else {
            data = i2c_req->data_buf[0] & 0xFF;
            data |= (i2c_req->data_buf[1] << 8) & 0xFF00;
        }
        break;

    default:
        panic("%s[%d]: Unknown Write type %d\n", __func__, __LINE__, rw_type);
    }

    return data;
}

/* form instruction portion for a i2c write operation */
static uint32_t
tor_cpld_i2c_form_wr_instr (struct tor_cpld_i2c_req *i2c_req, uint8_t rw_type)
{
    uint32_t instruction = 0, byte_cnt;
    uint8_t offset;

    switch (rw_type) {
    case TOR_CPLD_I2C_W_0:
    case TOR_CPLD_I2C_W_1:
    case TOR_CPLD_I2C_W_2:
    case TOR_CPLD_I2C_W_5:
        instruction |= (i2c_req->phy_addr & TOR_CPLD_I2C_ADDR_MASK) << 
                        TOR_CPLD_I2C_ADDR_SHIFT;

        if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF) {
            offset = (i2c_req->offset >> 8) & 0xFF;
        } else {
            offset = (i2c_req->offset) & 0xFF;
        }
        instruction |= (offset & TOR_CPLD_I2C_CMD_OFF_MASK) << 
                       TOR_CPLD_I2C_CMD_OFF_SHIFT;

        /* form instruction portion for a i2c write operation */
        if (rw_type == TOR_CPLD_I2C_W_5) {
            if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF) {
                byte_cnt = i2c_req->len  + 1;
            } else {
                byte_cnt = i2c_req->len;
            }
            instruction |= (byte_cnt & TOR_CPLD_I2C_BYTE_CNT_MASK)  <<
                            TOR_CPLD_I2C_BYTE_CNT_SHIFT;
            instruction |= ((byte_cnt >> 6) & TOR_CPLD_I2C_BYTE_CNT_1_MASK) << 
                           TOR_CPLD_I2C_BYTE_CNT_1_SHIFT;
        }
        break;

    case TOR_CPLD_I2C_W_4:
        instruction = 0;
        break;

    default:
        panic("%s[%d]: Unknown Write type %d\n", __func__, __LINE__, rw_type);
    }

    instruction |= rw_type << TOR_CPLD_I2C_RW_TYPE_SHIFT;
    instruction |= TOR_CPLD_I2C_VALID_BIT;

    return instruction;
}

static uint32_t
tor_cpld_i2c_w5_write_data (struct tor_cpld_i2c_req *i2c_req, uint8_t rw_type,
                            volatile uint32_t *mem_addr)
{
    uint32_t i, data;

    if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF) {
        i = 1;
    } else {
        i = 2;
    }

    for (; i < i2c_req->len; i+=2) {
        data = (i2c_req->data_buf[i]) & 0xFF;

        if (i + 1 < i2c_req->len) {
            data |= (i2c_req->data_buf[i+1] << 8) & 0xFF00;
        }
        tor_cpld_i2c_controller_write(*mem_addr, data);
        (*mem_addr)++;

        tor_cpld_i2c_controller_write(*mem_addr, 0);
        (*mem_addr)++;
    }

    return TOR_CPLD_OK;
}
    
/* set up data and instruction for a i2c write operation */
static uint32_t
tor_cpld_i2c_set_wr (struct tor_cpld_i2c_req *i2c_req, uint8_t rw_type,
                     volatile uint32_t *mem_addr, volatile uint32_t *data_addr)
{
    uint32_t instruction = 0, data = 0;

    if (*mem_addr % 2) {
        panic("%s[%d]: mem_addr 0x%x is not an even number\n", 
                __func__, __LINE__, *mem_addr);
    }

    data = tor_cpld_i2c_form_wr_data(i2c_req, rw_type);

    tor_cpld_i2c_controller_write(*mem_addr, data);

    *data_addr = *mem_addr;

    (*mem_addr)++;

    instruction = tor_cpld_i2c_form_wr_instr(i2c_req, rw_type);

    tor_cpld_i2c_controller_write(*mem_addr, instruction);

    (*mem_addr)++;

    if (rw_type == TOR_CPLD_I2C_W_5) {
        tor_cpld_i2c_w5_write_data(i2c_req, rw_type, mem_addr);
    }

    return TOR_CPLD_OK;
}

/* set up data and instruction for eos */
static uint32_t
tor_cpld_i2c_set_eos (volatile uint32_t *mem_addr)
{
    uint32_t instruction = 0, data = 0;

    tor_cpld_i2c_controller_write(*mem_addr, data);
    (*mem_addr)++;

    instruction = TOR_CPLD_I2C_EOS << TOR_CPLD_I2C_RW_TYPE_SHIFT;
    instruction |= TOR_CPLD_I2C_VALID_BIT; 

    tor_cpld_i2c_controller_write(*mem_addr, instruction);
    (*mem_addr)++;


    return TOR_CPLD_OK;
}

static uint32_t
tor_cpld_i2c_send_multi_byte_off (struct tor_cpld_i2c_req *i2c_req,
                                   volatile uint32_t *mem_addr)
{
    uint32_t dummy;
    char data_buf[1];
    struct tor_cpld_i2c_req tmp_i2c_req;

    memcpy(&tmp_i2c_req, i2c_req, sizeof (struct tor_cpld_i2c_req));
    data_buf[0] = (i2c_req->offset) & 0xFF;
    tmp_i2c_req.data_buf = data_buf;
    tmp_i2c_req.len = sizeof (data_buf);
    if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF) {
        tor_cpld_i2c_set_wr(&tmp_i2c_req, TOR_CPLD_I2C_W_1, mem_addr, &dummy);
    }
    
    return TOR_CPLD_OK;
}


static uint32_t
tor_cpld_i2c_set_delay (struct tor_cpld_i2c_req *i2c_req, 
                         volatile uint32_t *mem_addr)
{
    uint32_t instruction = 0, data = 0;

    data |= TOR_CPLD_I2C_USEC;
    data |= 0x1F4 & 0xFFFF;
    tor_cpld_i2c_controller_write(*mem_addr, data);
    (*mem_addr)++;

    instruction = TOR_CPLD_I2C_DELAY << TOR_CPLD_I2C_RW_TYPE_SHIFT;
    instruction |= TOR_CPLD_I2C_VALID_BIT; 

    tor_cpld_i2c_controller_write(*mem_addr, instruction);
    (*mem_addr)++;

    return TOR_CPLD_OK;
}

static uint32_t 
tor_cpld_i2c_set_os (struct tor_cpld_i2c_req *i2c_req, 
                      volatile uint32_t *mem_addr,
                      uint32_t *data_addr)
{
    uint8_t  rw_type = 0;
    uint32_t result;

    /* first decides the cpld i2c rw type */
    result = tor_cpld_i2c_get_rw_type(i2c_req, &rw_type);
    i2c_req->rw_type = rw_type;

    if (result != TOR_CPLD_OK) {
        printf("%s[%d]: get rw type failed\n", __func__, __LINE__);
        return result;
    }

    /* sets up the entries according to the rw type. Some of the R type
     * requires a W type first */
    switch (rw_type) {
    case TOR_CPLD_I2C_R_0:
    case TOR_CPLD_I2C_R_1:
        if (!(i2c_req->i2c_mode & TOR_CPLD_I2C_NO_OFF)) {
            tor_cpld_i2c_set_wr(i2c_req, TOR_CPLD_I2C_W_0, mem_addr, data_addr);
        } 
        tor_cpld_i2c_set_r(i2c_req, rw_type, mem_addr, data_addr);
        break;

    case TOR_CPLD_I2C_R_2:
    case TOR_CPLD_I2C_R_3:
    case TOR_CPLD_I2C_R_5:
        if (i2c_req->i2c_mode & TOR_CPLD_I2C_16_OFF ||
            i2c_req->i2c_mode & TOR_CPLD_I2C_32_OFF) {
            tor_cpld_i2c_send_multi_byte_off(i2c_req, mem_addr); 
        } else if (i2c_req->i2c_mode & TOR_CPLD_I2C_8_OFF) {
            tor_cpld_i2c_set_wr(i2c_req, TOR_CPLD_I2C_W_0, mem_addr, data_addr);
        } 
        tor_cpld_i2c_set_r(i2c_req, rw_type, mem_addr, data_addr);
        break;

    case TOR_CPLD_I2C_W_0:
    case TOR_CPLD_I2C_W_1:
    case TOR_CPLD_I2C_W_2:
    case TOR_CPLD_I2C_W_4:
    case TOR_CPLD_I2C_W_5:
        tor_cpld_i2c_set_wr(i2c_req, rw_type, mem_addr, data_addr);
        break;

    default:
        break;
    }

    /* set EOS entry */
    tor_cpld_i2c_set_eos(mem_addr);

    return TOR_CPLD_OK;
}

static uint32_t
tor_cpld_i2c_wait (void)
{
    uint32_t result, retry = 0;

    do {
        tor_cpld_i2c_controller_read(TOR_CPLD_I2C_CTRL_OFF, &result);
    } while ((result & 0x3) && (++retry < TOR_CPLD_I2C_MAX_RETRY));

    if (retry >= TOR_CPLD_I2C_MAX_RETRY) {
        return TOR_CPLD_I2C_BUSY;
    } else {
        return TOR_CPLD_OK;
    }
}

static void
tor_cpld_i2c_go (void)
{
    uint32_t result;

    tor_cpld_i2c_controller_read(TOR_CPLD_I2C_CTRL_OFF, &result);

    tor_cpld_i2c_controller_write(TOR_CPLD_I2C_CTRL_OFF, result | 
                                   TOR_CPLD_I2C_GO);
}

static void
tor_cpld_i2c_stop (void)
{
    uint32_t result;
    
    tor_cpld_i2c_controller_read(TOR_CPLD_I2C_CTRL_OFF, &result);

    tor_cpld_i2c_controller_write(TOR_CPLD_I2C_CTRL_OFF, result & 
                                   ~TOR_CPLD_I2C_GO);
}

static uint32_t 
tor_cpld_i2c_set_mux (struct tor_cpld_i2c_req *i2c_req,
                       volatile uint32_t *mem_addr)
{
    uint32_t dummy;

    tor_cpld_i2c_set_wr(i2c_req, TOR_CPLD_I2C_W_4, mem_addr, &dummy);

    return TOR_CPLD_OK;
}

static uint32_t
tor_cpld_set_nop (volatile uint32_t *mem_addr)
{
    volatile uint32_t dummy;

    tor_cpld_i2c_controller_write(*mem_addr, 0);
    
    tor_cpld_i2c_controller_read(TOR_CPLD_I2C_BASE_MEM + 1, (uint32_t *)&dummy);
    if (dummy == 0) {
        printf("%s[%d]: dummy = 0 mem_addr=0x%x\n", __func__, __LINE__, *mem_addr);
    }

    (*mem_addr)++;

    tor_cpld_i2c_controller_write(*mem_addr, 0);

    tor_cpld_i2c_controller_read(TOR_CPLD_I2C_BASE_MEM + 1, (uint32_t *)&dummy);
    if (dummy == 0) {
        printf("%s[%d]: dummy = 0 mem_addr=0x%x\n", __func__, __LINE__, *mem_addr);
    }
    (*mem_addr)++;

    return TOR_CPLD_OK;
}

static uint32_t
tor_cpld_i2c_set_loop (volatile uint32_t *mem_addr)
{
    uint32_t instruction = 0, data = 0;

    tor_cpld_set_nop(mem_addr);

    tor_cpld_i2c_controller_write(*mem_addr, data);
    (*mem_addr)++;

    instruction = TOR_CPLD_I2C_LOOP << TOR_CPLD_I2C_RW_TYPE_SHIFT;
    instruction |= TOR_CPLD_I2C_VALID_BIT; 
    tor_cpld_i2c_controller_write(*mem_addr, instruction);
    (*mem_addr)++;

    return TOR_CPLD_OK;
}

static void
tor_cpld_i2c_cleanup_err (void)
{
    tor_cpld_i2c_controller_write(TOR_CPLD_I2C_INT_SRC_OFF, 
                                   TOR_CPLD_I2C_INT_OS_RE | 
                                   TOR_CPLD_I2C_INT_TIMEOUT |
                                   TOR_CPLD_I2C_INT_ERR);
}

static uint32_t
tor_cpld_i2c_handle_err (struct tor_cpld_i2c_req *i2c_req)
{
    return TOR_CPLD_OK;
}

/* check any error after i2c operation is done */
static uint32_t
tor_cpld_i2c_chk_err (uint32_t scan_result)
{
    if (scan_result & TOR_CPLD_I2C_NACK_BIT) {
        return TOR_CPLD_I2C_NACK;
    }

    if (scan_result & TOR_CPLD_I2C_RBERR_BIT) {
        return TOR_CPLD_I2C_RBERR;
    }

    if (!(scan_result & TOR_CPLD_I2C_OP_DONE_BIT)) {
        return TOR_CPLD_I2C_NOT_DONE;
    }

    return TOR_CPLD_OK;
}

/* fetch the result from i2c controller */
static uint32_t
tor_cpld_i2c_fetch_result (struct tor_cpld_i2c_req *i2c_req,
                            uint32_t data_addr)
{
    uint32_t i, len, retry = 0;
    uint32_t scan_result, ctrller_result;
    uint32_t max_retry; 
    uint8_t error = 0, scan_error = 0;


    if (i2c_req->rw_type == TOR_CPLD_I2C_W_0) {
        len = 1;
    } else {
        len = i2c_req->len;
    }
    max_retry =  len * TOR_CPLD_I2C_MAX_RETRY;

    do {
        tor_cpld_i2c_controller_read(TOR_CPLD_I2C_INT_SRC_OFF, &ctrller_result);


        if (ctrller_result & TOR_CPLD_I2C_INT_OS_RE) {
            if (ctrller_result & TOR_CPLD_I2C_INT_ERR) {
                error = 1;
            }
            break;
        }
        udelay(1000);
    } while (++retry < max_retry);


    tor_cpld_i2c_debug("%s[%d]: retry=%d ctrller_result=0x%x data_addr=0x%x error=0x%x\n", 
                        __func__, __LINE__, retry, ctrller_result, data_addr, error);
    if (retry >= max_retry) {
        tor_cpld_i2c_debug("%s[%d]: *Error* Exceed max retry for addr=0x%x ctrller_result=0x%x\n", 
                __func__, __LINE__, i2c_req->phy_addr, ctrller_result);
        tor_cpld_i2c_cleanup_err();
        return TOR_CPLD_ERR;
    }

    if (i2c_req->read_op) {
        tor_cpld_i2c_debug("Data:\n");
    }

    for (i = 0; i < len; i+=2, data_addr+=2) {
        tor_cpld_i2c_controller_read(data_addr, &scan_result);

        if ((scan_error = tor_cpld_i2c_chk_err(scan_result)) != TOR_CPLD_OK) {
            tor_cpld_i2c_debug("*Error*: error=0x%x scan_result=0x%x data_addr=0x%x\n", 
                    error, scan_result, data_addr);
            goto Error;
        }

        if (i2c_req->read_op) {
            i2c_req->data_buf[i] = scan_result & 0xFF;
            tor_cpld_i2c_debug("0x%x ", i2c_req->data_buf[i] & 0xFF);
            if (i + 1 < i2c_req->len) {
                i2c_req->data_buf[i+1] = (scan_result >> 8) & 0xFF;
                tor_cpld_i2c_debug(" 0x%x ", i2c_req->data_buf[i+1] & 0xFF);
            }
            if (i && !(i % 20)) {
                tor_cpld_i2c_debug("\n");
            }
        }
    }
    if (i2c_req->read_op) {
        tor_cpld_i2c_debug("\n");
    }

    if (error ||scan_error) {
        goto Error;
    }

    return TOR_CPLD_OK;

Error:
    if (error || scan_error) {
        tor_cpld_i2c_debug("%s[%d]: *Error* i2c Error phy addr: 0x%x scan_result: 0x%x "
                "ctrller_result: 0x%x error=0x%x scan_error: 0x%x\n", 
                __func__, __LINE__, i2c_req->phy_addr, 
                scan_result, ctrller_result, error, scan_error);
        tor_cpld_i2c_handle_err(i2c_req);
        tor_cpld_i2c_cleanup_err();
        return scan_error;
    }
}

/* program the controller and perform an i2c operation */ 
static uint32_t
tor_cpld_i2c (struct tor_cpld_i2c_req *i2c_req)
{
    uint32_t mem_addr = TOR_CPLD_I2C_BASE_MEM;
    uint32_t data_addr = 0;
    uint32_t result;
    struct tor_cpld_i2c_mem_dump_req i2c_dump_req;

    if (tor_cpld_i2c_wait() != TOR_CPLD_OK) {
        printf("%s[%d]: TOR_CPLD_I2C_BUSY: Retry...\n", __func__, __LINE__);
        tor_cpld_i2c_controller_reset();
        if (tor_cpld_i2c_wait() != TOR_CPLD_OK) {
            printf("%s[%d]: TOR_CPLD_I2C_BUSY: Quit...\n", __func__, __LINE__);
            return TOR_CPLD_I2C_BUSY;
        }
    }

    tor_cpld_i2c_cleanup_err();

    tor_cpld_i2c_set_mux(i2c_req, &mem_addr);

    tor_cpld_i2c_set_os(i2c_req, &mem_addr, &data_addr);

    tor_cpld_i2c_set_loop(&mem_addr);

    tor_cpld_i2c_go();

    result = tor_cpld_i2c_fetch_result(i2c_req, data_addr);


#ifdef __RMI_BOOT2__ 
    if (getenv("i2c_boot_verbose") || tor_cpld_i2c_debug_flag) {
#else
    if (tor_cpld_i2c_debug_flag) {
#endif
        i2c_dump_req.mem_addr = TOR_CPLD_I2C_BASE_MEM;
        i2c_dump_req.entry = mem_addr - TOR_CPLD_I2C_BASE_MEM;
        tor_cpld_i2c_mem_dump(&i2c_dump_req);
    }

    tor_cpld_i2c_stop();

    return result;
}

/* dumping the i2c controller memory */
uint32_t
tor_cpld_i2c_mem_dump (struct tor_cpld_i2c_mem_dump_req *i2c_dump_req)
{
    uint32_t i, data;

    printf("\nDump i2c controller memory:\n");
    for (i = 0; i < i2c_dump_req->entry; i++) {
        tor_cpld_i2c_controller_read(i2c_dump_req->mem_addr + i, &data);
        printf("mem_addr: 0x%x data: 0x%x\n", i2c_dump_req->mem_addr + i, data);
    }
    printf("\n");

    return TOR_CPLD_OK;
}

uint32_t
tor_cpld_i2c_controller_reset (void)
{
    tor_cpld_i2c_controller_write(TOR_CPLD_I2C_CTRL_OFF, 
                                  1 << TOR_CPLD_I2C_CTRL_RESET_SHIFT);

    udelay(50000);
    tor_cpld_i2c_controller_write(TOR_CPLD_I2C_CTRL_OFF, 0);
    udelay(50000);

    return TOR_CPLD_OK;
}

uint32_t
tor_cpld_i2c_op (struct tor_cpld_i2c_req *i2c_req)
{
    uint32_t result;
    uint32_t i;

    tor_cpld_i2c_debug("%s[%d]: read_op=0x%x phy_addr=0x%x len=0x%x offset=0x%x bus=0x%x "
                 "sseg=0x%x  pseg=0x%x i2c_mode=0x%x\n", __func__, __LINE__,
                i2c_req->read_op, i2c_req->phy_addr,
                i2c_req->len, i2c_req->offset, i2c_req->i2c_mux.bus,
                i2c_req->i2c_mux.sseg, i2c_req->i2c_mux.pseg, i2c_req->i2c_mode);

#ifdef __RMI_BOOT2__
    if (getenv("i2c_boot_verbose") || tor_cpld_i2c_debug_flag) {
#else
    if (tor_cpld_i2c_debug_flag) {
#endif
        if (i2c_req->read_op == 0) {
            for (i = 0; i < i2c_req->len; i++) {
                printf("0x%x ", i2c_req->data_buf[i]);
            }
        }
    }
    result = tor_cpld_i2c(i2c_req);

    return result;
}

/* interface function for performing an i2c write operation */
uint32_t
tor_cpld_i2c_write (uint32_t mux, uint32_t phy_addr, uint8_t off, int i2c_mode,
                   uint8_t *buf, uint32_t len)
{
    struct tor_cpld_i2c_req i2c_req;
    uint32_t result;

    i2c_req.read_op = 0;
    i2c_req.phy_addr = phy_addr;
    i2c_req.len = len;
    i2c_req.data_buf = buf;
    i2c_req.offset = off;
    i2c_req.i2c_mux.pseg = TOR_CPLD_I2C_GET_PSEG(mux);
    i2c_req.i2c_mux.sseg = TOR_CPLD_I2C_GET_SSEG(mux);
    i2c_req.i2c_mux.bus =  TOR_CPLD_I2C_GET_BUS(mux);
    i2c_req.i2c_mode =  i2c_mode; 

    result = tor_cpld_i2c_op(&i2c_req);

    return result;
}

/* interface function for performing an i2c read operation */
uint32_t
tor_cpld_i2c_read (uint32_t mux, uint32_t phy_addr, uint32_t off, uint32_t i2c_mode,
                   uint8_t *buf, uint32_t len)
{
    struct tor_cpld_i2c_req i2c_req;
    uint32_t result;

    i2c_req.read_op = 1;
    i2c_req.phy_addr = phy_addr;
    i2c_req.len = len;
    i2c_req.data_buf = buf;
    i2c_req.offset = off;
    i2c_req.i2c_mux.pseg = TOR_CPLD_I2C_GET_PSEG(mux);
    i2c_req.i2c_mux.sseg = TOR_CPLD_I2C_GET_SSEG(mux);
    i2c_req.i2c_mux.bus =  TOR_CPLD_I2C_GET_BUS(mux);
    i2c_req.i2c_mode = i2c_mode; 

    result = tor_cpld_i2c_op(&i2c_req);

    return result;
}
