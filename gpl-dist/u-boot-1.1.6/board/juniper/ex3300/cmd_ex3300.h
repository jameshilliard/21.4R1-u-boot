/*
 * Copyright (c) 2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef __CMD_EX3300__
#define __CMD_EX3300__

#define MAX_BUF_LEN         256
#define BASIC_HDR_LEN       4 
#define SEC_IC_NO_OFFSET    -1

/* secure eeprom commands */
#define GET_M2_VERSION            0xC0
#define WRITE_EEPROM_DATA         0xC1
#define HW_AUTHENTICATE           0xC2
#define SW_AUTHENTICATE           0xC3
#define GET_HW_PUB_INFO           0xC4
#define GET_HW_PUB_SIGN           0xC5
#define GET_HW_UID                0xC6 
#define GET_CHIP_STATUS           0xC7

/* error commands */
#define STATUS_SUCCESS       0xE0
#define STATUS_ERROR         0xE1
#define STATUS_NOT_AVAILABLE 0xE2   /* secure eeprom is busy */ 
#define STATUS_WRITE_ERROR   0xE3
#define STATUS_READ_ERROR    0xE4 

#define CPU_TIMER_CONTROL			0x20300
#define WATCHDOG_ENABLE_BIT         BIT4
#define CPU_WATCHDOG_RELOAD			0x20320
#define CPU_WATCHDOG_TIMER			0x20324

/* i2c command structures */
typedef struct _hw_cmd_ {
    unsigned char i2c_cmd;
    unsigned char flags;
    unsigned short len;
} hw_cmd_t;

typedef struct _eeprom_cmd_ {
    hw_cmd_t cmd;
    unsigned short eeprom_addr;
    unsigned short eeprom_len;
    unsigned char buf[MAX_BUF_LEN];
} eeprom_cmd_t;

#define MAX_PACKET_LENGTH 1518
typedef struct _phy_mii_t {
    uint8_t reg;
    uint16_t page;
    char * name;
} phy_mii_t;


extern char console_buffer[];

#define GPP_DEFAULT_GROUP_0 0x480C20A0
#define GPP_DEFAULT_GROUP_1 0x00000001

#if !defined(CONFIG_PRODUCTION)
extern unsigned long read_p15_c1 (void);
extern void icache_enable (void);
extern void icache_disable (void);
extern void dcache_enable (void);
extern void dcache_disable (void);
extern int dcache_status (void);
extern int icache_status (void);
#endif

extern int lcd_debug;
extern struct pcie_controller pcie_hose[];
extern int board_idtype(void);
extern int board_pci(void);

extern int eth_receive(volatile void *packet, int length);
extern int cmd_get_data_size(char* arg, int default_size);

#define DISP_LINE_LEN   16

#define PRINT(msg, type, reg) \
    pcie_hose_read_config_##type(&pcie_hose[controller], dev, reg, &_##type); \
    printf(msg, _##type)

void pcieinfo(int, int , int );
void pciedump(int, int );
static char *pcie_classes_str(unsigned char );
void pcie_header_show_brief(int, pcie_dev_t );
void pcie_header_show(int, pcie_dev_t );

#if !defined(CONFIG_PRODUCTION)
extern char console_buffer[];
extern int i2c_read_norsta(uint8_t chanNum, uchar dev_addr, 
                       unsigned int offset, int alen, uint8_t* data, int len);
#endif

extern flash_info_t flash_info[];	/* info for FLASH chips */
extern int mv_flash_real_protect (flash_info_t *info, long sector, int prot);
extern int mv_flash_erase (flash_info_t *info, int s_first, int s_last);
extern int mv_write_buff (flash_info_t *info, uchar *src, ulong addr, 
            ulong cnt);

#if defined(CONFIG_SPI_SW_PROTECT)
extern int ex_mvStatusRegGet (uint8_t * pStatReg);
#endif

#endif /*__CMD_EX3300__*/
