/*
 * $Id: $
 *
 * tor_bcpld_i2c.h
 *
 * Chun Ng, March 2011
 *
 * Copyright (c) 2011, Juniper Networks, Inc.
 * All rights reserved.
 */

#ifndef _TOR_I2C_H_
#define _TOR_I2C_H_

#define TOR_CPLD_I2C_MAX_DATA                  0x200
#define TOR_CPLD_I2C_BASE_MEM                  0x2000
#define TOR_CPLD_I2C_MAX_RETRY                 0xFF
#define TOR_CPLD_I2C_OP_DONE_BIT               (0x1 << 31)
#define TOR_CPLD_I2C_NACK_BIT                  (0x1 << 24)
#define TOR_CPLD_I2C_RBERR_BIT                 (0x1 << 25)

#define TOR_CPLD_I2C_CONTROLLER_GO             (0x1 << 7)
#define TOR_CPLD_I2C_CTRL_RW_SHIFT             6
#define TOR_CPLD_I2C_CONTROLLER_R              (0x1 << TOR_CPLD_I2C_CTRL_RW_SHIFT)
#define TOR_CPLD_I2C_CONTROLLER_W              (0x0 << TOR_CPLD_I2C_CTRL_RW_SHIFT)
#define TOR_CPLD_I2C_MAX_BYTE                  512


#define TOR_CPLD_I2C_USEC                      (0 << 16)
#define TOR_CPLD_I2C_MSEC                      (1 << 16)
#define TOR_CPLD_I2C_CMD_OFF_SHIFT             0
#define TOR_CPLD_I2C_CMD_OFF_MASK              0xFF

#define TOR_CPLD_I2C_ADDR_SHIFT                0x8
#define TOR_CPLD_I2C_ADDR_MASK                 0xFF

#define TOR_CPLD_I2C_BYTE_CNT_SHIFT            16
#define TOR_CPLD_I2C_BYTE_CNT_MASK             0x3F

#define TOR_CPLD_I2C_BYTE_CNT_1_SHIFT          28
#define TOR_CPLD_I2C_BYTE_CNT_1_MASK           0x7

#define TOR_CPLD_I2C_RW_TYPE_SHIFT             24
#define TOR_CPLD_I2C_RW_TYPE_MASK              0xF
#define TOR_CPLD_I2C_W_0                       0x0
#define TOR_CPLD_I2C_W_1                       0x1
#define TOR_CPLD_I2C_W_2                       0x2
#define TOR_CPLD_I2C_W_4                       0x4
#define TOR_CPLD_I2C_W_5                       0x5
#define TOR_CPLD_I2C_DELAY                     0x6
#define TOR_CPLD_I2C_R_0                       0x8
#define TOR_CPLD_I2C_R_1                       0x9
#define TOR_CPLD_I2C_R_2                       0xA
#define TOR_CPLD_I2C_R_3                       0xB
#define TOR_CPLD_I2C_R_4                       0xC
#define TOR_CPLD_I2C_R_5                       0xD
#define TOR_CPLD_I2C_EOS                       0xE
#define TOR_CPLD_I2C_LOOP                      0xF

#define TOR_CPLD_I2C_VALID_BIT                 (1 << 31)

#define TOR_CPLD_I2C_CTRL_OFF                  0x0
#define     TOR_CPLD_I2C_GO                    0x1
#define     TOR_CPLD_I2C_BUSY                  (0x1 << 1)
#define     TOR_CPLD_I2C_CTRL_RESET_SHIFT      31

#define TOR_CPLD_I2C_INT_SRC_OFF               0x1
#define     TOR_CPLD_I2C_INT_OS_RE             (0x1 << 1)
#define     TOR_CPLD_I2C_INT_ERR               (0x1 << 2)
#define     TOR_CPLD_I2C_INT_TIMEOUT           (0x1 << 24)

#define TOR_CPLD_I2C_INT_EN_OFF                0x8

#define TOR_CPLD_I2C_NO_OFF                     (1 << 0)
#define TOR_CPLD_I2C_8_OFF                      (1 << 1)
#define TOR_CPLD_I2C_16_OFF                     (1 << 2)
#define TOR_CPLD_I2C_32_OFF                     (1 << 3)
#define TOR_CPLD_I2C_RSTART                     (1 << 4)

struct tor_cpld_i2c_req {
    uint32_t    read_op;
    uint32_t    phy_addr;
    uint32_t    offset;
    uint32_t    len;
    uint32_t    i2c_mode;
    uint32_t    delay;
    char        *data_buf;
    struct      bcpld_i2c_mux {
        uint32_t bus    :4;
        uint32_t sseg   :4;
        uint32_t pseg   :4;
    } i2c_mux;
    uint32_t rw_type;
};

struct tor_cpld_i2c_mem_dump_req {
    uint32_t    mem_addr;
    uint32_t    entry;
};

extern uint32_t tor_cpld_i2c_debug_flag; 
uint32_t tor_cpld_i2c_op(struct tor_cpld_i2c_req *i2c_req);
uint32_t tor_cpld_i2c_mem_dump(struct tor_cpld_i2c_mem_dump_req *i2c_dump_req);
uint32_t tor_cpld_i2c_controller_reset(void);
uint32_t tor_cpld_i2c_read(uint32_t mux, uint32_t phy_addr, uint32_t off, uint32_t alen,
                   uint8_t *buf, uint32_t len);
uint32_t tor_cpld_i2c_write(uint32_t mux, uint32_t phy_addr, uint8_t off, int alen,
                            uint8_t *buf, uint32_t len);
uint32_t tor_cpld_i2c_controller_read(uint32_t addr, uint32_t *data);
uint32_t tor_cpld_i2c_controller_write(uint32_t addr, uint32_t cmd_data);



#define TOR_CPLD_I2C_GET_PSEG(mux)  (mux & 0xFF)
#define TOR_CPLD_I2C_GET_SSEG(mux)  ((mux >> 4) & 0xFF)
#define TOR_CPLD_I2C_GET_BUS(mux)    ((mux >> 8) & 0xFF)

#define TOR_CPLD_I2C_CREATE_MUX(bus, sseg, pseg) \
    (((pseg) & 0xFF) | (((sseg) & 0xFF) << 4) | (((bus) & 0xFF) << 8))

#endif
