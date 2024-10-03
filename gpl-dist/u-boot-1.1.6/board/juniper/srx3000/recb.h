/*
 * Copyright (c) 2007-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * $Id: recb.h,v 1.1.96.1 2009-09-01 14:41:47 jyan Exp $
 * 
 *
 * recb.h -- recb defines
 *
 * Copyright (c) 2005-2006, Juniper Networks, Inc.
 * All rights reserved.
 */

#ifndef __recb_H__
#define __recb_H__

#include <linux/types.h>

typedef struct recb_master_slave_sm_ {
    volatile u_int32_t recb_mstr_config;		/* 0x0 */
    volatile u_int32_t recb_mstr_alive;		/* 0x4 */
    volatile u_int32_t recb_mstr_alive_cnt;	/* 0x8 */
    volatile u_int32_t recb_mas_slv_sm_sw_disable; /* 0xc */
} recb_master_slave_sm_t;

typedef struct recb_slow_bus_ {
    volatile u_int32_t recb_slow_bus_addr;		/* 0x0 */
    volatile u_int32_t recb_slow_bus_data;		/* 0x4 */
    volatile u_int32_t recb_slow_bus_ctrl;		/* 0x8 */
    volatile u_int32_t recb_slow_bus_i2c_sel;		/* 0xc */
} recb_slow_bus_t;

typedef struct uart_regs {
    volatile u_int32_t data_buf; /* 0x0 */
    volatile u_int32_t int_en; /* 0x4 */
    volatile u_int32_t int_id_fifo_cntl; /* 0x8 */
    volatile u_int32_t line_cntl; /* 0xc */
    volatile u_int32_t modem_cntl; /* 0x10 */
    volatile u_int32_t line_status; /* 0x14 */
    volatile u_int32_t modem_status; /* 0x18 */
    volatile u_int32_t scratch; /* 0x1c */
} uart_regs;

typedef struct SPI {
    volatile u_int8_t spi_ctl; /* 0x0 */
    unsigned char    FILLER_0x1[0x3];
    volatile u_int8_t command; /* 0x4 */
    unsigned char    FILLER_0x5[0x3];
    volatile u_int8_t address_high; /* 0x8 */
    unsigned char    FILLER_0x9[0x3];
    volatile u_int8_t address_mid; /* 0xc */
    unsigned char    FILLER_0xd[0x3];
    volatile u_int8_t address_low; /* 0x10 */
    unsigned char    FILLER_0x11[0x3];
    volatile u_int8_t write_data; /* 0x14 */
    unsigned char    FILLER_0x15[0x3];
    volatile u_int8_t read_data; /* 0x18 */
    unsigned char    FILLER_0x19[0x7];
} SPI;

/*
 * The recb FPGA structure
 */

typedef struct recb_fpga_regs_ {
    /*
     * (Offset starting from  0x0000)
     */
    volatile u_int32_t recb_version;    
    volatile u_int32_t recb_scratch_pad;          
    volatile u_int32_t recb_board_status; 
    volatile u_int32_t recb_re_reset_enable;       

    volatile u_int32_t recb_re_reset_history;  

    volatile u_int32_t  recb_re_reset_controller;         
    volatile u_int32_t  recb_control_register;   

    volatile u_int32_t  recb_timer_cnt;     

    volatile u_int32_t  recb_wdtimer;     
    volatile u_int32_t  recb_cpp_bootram_write_enable;  

    volatile u_int32_t  recb_card_presence;         
    volatile u_int32_t  recb_irq4_status;         
    volatile u_int32_t  recb_irq4_en;         
    
    volatile u_int32_t  recb_irq7_status;         
    volatile u_int32_t  recb_irq7_en;         
    
    volatile u_int32_t  recb_irq8_status;         
    volatile u_int32_t  recb_irq8_en;         

    volatile u_int32_t  recb_irq10_status;;         
    volatile u_int32_t  recb_irq10_en;         
    
    unsigned char    FILLER_0x4c[0x4];
    
    volatile u_int32_t  recb_keep_alive_fail_timer;         
    volatile u_int32_t  recb_fast_alive_fail_timer;         


    volatile u_int32_t  recb_fast_keep_alive_status;         
    volatile u_int32_t  recb_misc_led_cntrl;         
    
    recb_master_slave_sm_t ms_arb;		/* 0x60 */
    recb_slow_bus_t slow_bus;		/* 0x70 */

    unsigned char    FILLER_0x80[0x40];
    volatile u_int32_t uart_mux; /* 0xc0 */
    volatile u_int32_t ha_led; /* 0xc4 */
    volatile u_int32_t interupt_source_reg; /* 0xc8 */
    volatile u_int32_t interupt_source_en; /* 0xcc */
    unsigned char    FILLER_0xd0[0x30];
    volatile u_int32_t re_to_cpp_mbox[8]; /* 0x100 */
    volatile u_int32_t cpp_to_re_mbox[8]; /* 0x120 */
    volatile u_int32_t re_to_cpp_mbox_event; /* 0x140 */
    volatile u_int32_t cpp_to_re_mbox_event; /* 0x144 */
    volatile u_int32_t master_re_i2c_status; /* 0x148 */
    volatile u_int32_t i2c_scratch_pad; /* 0x14c */
    volatile u_int32_t nextboot_reg0; /* 0x150 */
    volatile u_int32_t nextboot_reg1; /* 0x154 */
    volatile u_int32_t recb_card_type; /* 0x158 */
    unsigned char    FILLER_0x15c[0x4];
    volatile u_int32_t watchdog_readback; /* 0x160 */
    volatile u_int32_t watchdog_counter; /* 0x164 */
    volatile u_int32_t sreset_history; /* 0x168 */
    volatile u_int32_t int_spi_flash_cmd_reg; /* 0x16c */
    volatile u_int32_t int_spi_flash_wr_data_reg; /* 0x170 */
    volatile u_int32_t int_spi_flash_rd_data_reg; /* 0x174 */
    volatile u_int32_t int_spi_flash_intr_data_reg; /* 0x178 */
    volatile u_int32_t spi_program; /* 0x17c */
    unsigned char    FILLER_0x180[0x80];
    uart_regs uart[2]; /* 0x200 */
    unsigned char    FILLER_0x240[0xc0];
    SPI internalSPI; /* 0x300 */
    unsigned char    FILLER_0x320[0xe0];

} recb_fpga_regs_t;

/*
 * Defines for Slow Bus (Flash & I2C)
 */
/* slow_bus_ctrl */
#define RECB_SLOW_BUS_CTRL_READ              (1 << 0)
#define RECB_SLOW_BUS_CTRL_WRITE             0
#define RECB_SLOW_BUS_CTRL_START             (1 << 1) 
#define RECB_SLOW_BUS_CTRL_ACTIVE            RECB_SLOW_BUS_CTRL_START
#define RECB_SLOW_BUS_CTRL_SEL_FLASH         (0 << 2)
#define RECB_SLOW_BUS_CTRL_SEL_I2C           (1 << 2)

#define RECB_SLAVE_CPLD_ADDR                0x54

/*
 *  Defines for HDRE && HA port control via BCM led proc
 */
#define HDRE_ENV_SET                '1'
#define BCM_CMIC_PCI_BASE_ADDR      0x80000000
#define BCM_PCI_READ(b, o)          (((volatile uint32_t *)(b))[(o)/4])
#define BCM_PCI_WRITE(b, o, v)      (BCM_PCI_READ(b,o) = v)
#define CMIC_LED_CTRL               0x00001000
#define CMIC_LED_PROGRAM_RAM_BASE   0x00001800
#define CMIC_LED_DATA_RAM_BASE      0x00001c00
#define CMIC_LED_PROGRAM_RAM(_a)    (CMIC_LED_PROGRAM_RAM_BASE + 4 * (_a))
#define CMIC_LED_PROGRAM_RAM_SIZE   0x100
#define CMIC_LED_DATA_RAM(_a)       (CMIC_LED_DATA_RAM_BASE + 4 * (_a))
#define CMIC_LED_DATA_RAM_SIZE      0x100

extern void init_ha_led(ulong);
extern int  cb_is_hdre();
#endif /* __recb_H__ */

