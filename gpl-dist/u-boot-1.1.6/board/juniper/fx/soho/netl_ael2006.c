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

#include "soho_cpld.h"
#include "soho_cpld_map.h"
#include "soho_misc.h"
#include <command.h>

struct pa_sfp_ael_map_ent
{
    uint8_t sfp_port;
    uint8_t gpio_seg;
    uint8_t mdio_addr;
};

struct pa_sfp_ael_map_ent pa_sfp_ael_map[] =
{
    {6,     4,      0},
    {7,     4,      1},
    {8,     0,      0},
    {9,     0,      1},
    {10,    0,      2},
    {11,    0,      3},
    {12,    0,      4},
    {13,    0,      5},
    {14,    0,      6},
    {15,    0,      7},
    {16,    1,      0},
    {17,    1,      1},
    {18,    1,      2},
    {19,    1,      3},
    {20,    1,      4},
    {21,    1,      5},
    {22,    1,      6},
    {23,    1,      7},
    {24,    2,      0},
    {25,    2,      1},
    {26,    2,      2},
    {27,    2,      3},
    {28,    2,      4},
    {29,    2,      5},
    {30,    2,      6},
    {31,    2,      7},
    {32,    3,      0},
    {33,    3,      1},
    {34,    3,      2},
    {35,    3,      3},
    {36,    3,      4},
    {37,    3,      5},
    {38,    3,      6},
    {39,    3,      7},
    {40,    5,      0},
    {41,    5,      1},
    {48,    6,      4},
    {49,    6,      5},
    {50,    6,      6},
    {51,    6,      7},
    {52,    6,      0},
    {53,    6,      1},
    {54,    6,      2},
    {55,    6,      3},
    {56,    7,      0},
    {57,    7,      1},
    {58,    7,      2},
    {59,    7,      3},
    {60,    7,      4},
    {61,    7,      5},
    {62,    7,      6},
    {63,    7,      7},
};

void
ael2006_read_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t phy_addr, reg_id, segment, time_para;
    uint16_t data;
    soho_cpld_status_t status;

    if (argc < 5) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 5) {
        segment = simple_strtoul(argv[1], NULL, 16);
        phy_addr = simple_strtoul(argv[2], NULL, 16);
        reg_id = simple_strtoul(argv[3], NULL, 16);
        time_para = simple_strtoul(argv[4], NULL, 16);
    }

    if (segment == 1) {
        soho_mdio_set_10G_PHY_1();
    } else if (segment == 7) {
        soho_mdio_set_10G_PHY_7();
    } else {
        printf("ERROR: invalid gpio segment\n");
        return;
    }

    status = soho_cpld_mdio_c45_read(0x1, phy_addr, reg_id, &data, time_para);

    if (status == SOHO_CPLD_OK) {
        printf("Segment: %x Phy Addr: %x Reg Id: %x Value: %x\n", 
               segment, phy_addr, reg_id, data);
    } else {
        printf("MDIO read failed 0x%x\n", status);
    }
}

U_BOOT_CMD(
           ael2006_read_reg, 
           5, 
           1, 
           ael2006_read_reg,
           "ael2006_read_reg- puma read phy register\n",
           "read a phy register <phy (1 or 7)> <phy_addr> <reg addr> <time para>"
          );

static void
pa_ael2006_read_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t port_num;
    uint32_t reg_id;
    uint32_t i;
    uint8_t found = 0;
    soho_cpld_status_t status;
    uint16_t data;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 3) {
        port_num = simple_strtoul(argv[1], NULL, 16);
        reg_id = simple_strtoul(argv[2], NULL, 16);
    }

    for (i = 0; i < sizeof (pa_sfp_ael_map) / sizeof (struct pa_sfp_ael_map_ent); i++) {
        if (pa_sfp_ael_map[i].sfp_port == port_num) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        printf("Port %d not found in the table\n", port_num);
        return;
    }

    soho_mdio_set_10G_phy(pa_sfp_ael_map[i].gpio_seg);
    status = soho_cpld_mdio_c45_read(0x1, pa_sfp_ael_map[i].mdio_addr, 
                                     reg_id, &data, 0x0);

    if (status != SOHO_CPLD_OK) {
        printf("Error: MDIO Read failed for Port %d Segment: %x Phy Addr: %x "
               "Reg Id: %x\n", 
               port_num, pa_sfp_ael_map[i].gpio_seg, pa_sfp_ael_map[i].mdio_addr, 
               reg_id);
    } else {
        printf("MDIO Read Port :%d Segment: %x Phy Addr: %x Reg Id: %x "
               "Value: %x\n", 
              port_num, pa_sfp_ael_map[i].gpio_seg, pa_sfp_ael_map[i].mdio_addr, 
              reg_id, data);
    }
}

U_BOOT_CMD(
           pa_ael2006_read_reg, 
           3, 
           1, 
           pa_ael2006_read_reg,
           "pa_ael2006_read_reg- puma read phy register\n",
           "read a phy register <port> <reg_id>"
          );


static void
pa_ael2006_write_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t port_num;
    uint32_t reg_id;
    uint32_t i;
    uint8_t found = 0;
    soho_cpld_status_t status;
    uint16_t data;

    if (argc < 4) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 4) {
        port_num = simple_strtoul(argv[1], NULL, 16);
        reg_id = simple_strtoul(argv[2], NULL, 16);
        data = simple_strtoul(argv[3], NULL, 16);
    }

    for (i = 0; i < sizeof(pa_sfp_ael_map) / sizeof (struct pa_sfp_ael_map_ent); i++) {
        if (pa_sfp_ael_map[i].sfp_port == port_num) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        printf("Port %d not found in the table\n", port_num);
        return;
    }
    soho_mdio_set_10G_phy(pa_sfp_ael_map[i].gpio_seg);
    status = soho_cpld_mdio_c45_write(0x1, pa_sfp_ael_map[i].mdio_addr, 
                                      reg_id, data, 0);

    if (status != SOHO_CPLD_OK) {
        printf("Error: MDIO Write failed for Port %d Segment: %x Phy Addr: %x "
               "Reg Id: %x\n", 
               port_num, pa_sfp_ael_map[i].gpio_seg, 
               pa_sfp_ael_map[i].mdio_addr, reg_id);
    } else {
        printf("MDIO Write Port :%d Segment: %x Phy Addr: %x Reg Id: %x"
               "Value: %x\n", 
               port_num, pa_sfp_ael_map[i].gpio_seg, 
               pa_sfp_ael_map[i].mdio_addr, reg_id, data);
    }
}

U_BOOT_CMD(
           pa_ael2006_write_reg, 
           4, 
           1, 
           pa_ael2006_write_reg,
           "pa_ael2006_write_reg- puma write phy register\n",
           "write a phy register <port> <reg_id> <value>"
          );

void
ael2006_write_reg (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t phy_addr, reg_id, segment, time_para;
    uint16_t data;
    soho_cpld_status_t status;

    if (argc < 6) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 6) {
        segment = simple_strtoul(argv[1], NULL, 16);
        phy_addr = simple_strtoul(argv[2], NULL, 16);
        reg_id = simple_strtoul(argv[3], NULL, 16);
        data = simple_strtoul(argv[4], NULL, 16);
        time_para = simple_strtoul(argv[5], NULL, 16);
    }


    if (segment == 1) {
        soho_mdio_set_10G_PHY_1();
    } else if (segment == 7) {
        soho_mdio_set_10G_PHY_7();
    } else {
        printf("ERROR: Incorrect Segment: %d\n", segment);
    }

    status = soho_cpld_mdio_c45_write(0x1, phy_addr, reg_id, data, time_para);

    if (status == SOHO_CPLD_OK) {
        printf("Write to: Segment: %x Phy Addr: %x Reg Id: %x Value: %x\n", 
               segment, phy_addr, reg_id, data);
    } else {
        printf("MDIO write failed 0x%x\n", status);
    }
}

U_BOOT_CMD(
           ael2006_write_reg, 
           6, 
           1, 
           ael2006_write_reg,
           "ael2006_write_reg- puma write phy register\n",
           "write a puma phy register <phy (1 or 7)> <phy_addr> <reg addr> <value> <time para>"
          );

void
ael2006_prbs_ctrl (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t segment;
    uint32_t enable;
    uint32_t phy_addr0, phy_addr1, phy_addr2, phy_addr3;

    if (argc < 3) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 3) {
        enable = simple_strtoul(argv[1], NULL, 16);
        segment = simple_strtoul(argv[2], NULL, 16);
    }

    if (segment == 1) {
        soho_mdio_set_10G_PHY_1();
        phy_addr0 = 0;
        phy_addr1 = 1;
        phy_addr2 = 2;
        phy_addr3 = 3;
    } else if (segment == 7) {
        soho_mdio_set_10G_PHY_7();
        phy_addr0 = 4;
        phy_addr1 = 5;
        phy_addr2 = 6;
        phy_addr3 = 7;
    } else {
        printf("ERROR: Incorrect Segment: %d\n", segment);
        return;
    }

    if (enable == 1) {
        soho_cpld_mdio_c45_write(0x1, phy_addr0, 0xCCC3, 0x3, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr1, 0xCCC3, 0x3, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr2, 0xCCC3, 0x3, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr3, 0xCCC3, 0x3, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr0, 0xCC83, 0x3, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr1, 0xCC83, 0x3, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr2, 0xCC83, 0x3, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr3, 0xCC83, 0x3, 0x0); 
    } else if (enable == 0) {
        soho_cpld_mdio_c45_write(0x1, phy_addr0, 0xCCC3, 0x0, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr1, 0xCCC3, 0x0, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr2, 0xCCC3, 0x0, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr3, 0xCCC3, 0x0, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr0, 0xCC83, 0x0, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr1, 0xCC83, 0x0, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr2, 0xCC83, 0x0, 0x0); 
        soho_cpld_mdio_c45_write(0x1, phy_addr3, 0xCC83, 0x0, 0x0); 
    } else {
        printf("ctrl should be either 1 (enable) or 0 (disable)\n");
    }
}

U_BOOT_CMD(
           ael2006_prbs_ctrl, 
           3, 
           1, 
           ael2006_prbs_ctrl,
           "ael2006_prbs_ctrl- enable prbs in all ports \n",
           "ael2006_prbs_ctrl <1 or enable and 0 for disable> <segment>"
          );

void
ael2006_prbs_err_cnt_clr (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint32_t segment;
    uint32_t phy_addr0, phy_addr1, phy_addr2, phy_addr3;

    if (argc < 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 2) {
        segment = simple_strtoul(argv[1], NULL, 16);
    }


    if (segment == 1) {
        soho_mdio_set_10G_PHY_1();
        phy_addr0 = 0;
        phy_addr1 = 1;
        phy_addr2 = 2;
        phy_addr3 = 3;
    } else if (segment == 7) {
        soho_mdio_set_10G_PHY_7();
        phy_addr0 = 4;
        phy_addr1 = 5;
        phy_addr2 = 6;
        phy_addr3 = 7;
    } else {
        printf("ERROR: Incorrect Segment: %d\n", segment);
        return;
    }

    soho_cpld_mdio_c45_write(0x1, phy_addr0, 0xCCC8, 0x1, 0);
    soho_cpld_mdio_c45_write(0x1, phy_addr1, 0xCCC8, 0x1, 0);
    soho_cpld_mdio_c45_write(0x1, phy_addr2, 0xCCC8, 0x1, 0);
    soho_cpld_mdio_c45_write(0x1, phy_addr3, 0xCCC8, 0x1, 0);

    soho_cpld_mdio_c45_write(0x1, phy_addr0, 0xCC88, 0x1, 0);
    soho_cpld_mdio_c45_write(0x1, phy_addr1, 0xCC88, 0x1, 0);
    soho_cpld_mdio_c45_write(0x1, phy_addr2, 0xCC88, 0x1, 0);
    soho_cpld_mdio_c45_write(0x1, phy_addr3, 0xCC88, 0x1, 0);
}

U_BOOT_CMD(
           ael2006_prbs_err_cnt_clr, 
           2, 
           1, 
           ael2006_prbs_err_cnt_clr,
           "ael2006_prbs_err_cnt_clr- clear prbs error counter in all ports \n",
           "ael2006_prbs_err_cnt_clr <segment>"
          );

void
ael2006_prbs_err_cnt (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    uint16_t err_cnt_hi, err_cnt_low;
    uint32_t segment;
    uint32_t phy_addr0, phy_addr1, phy_addr2, phy_addr3;

    if (argc < 2) {
        printf("Usage:\n%s\n", cmdtp->usage);
        return;
    }

    if (argc == 2) {
        segment = simple_strtoul(argv[1], NULL, 16);
    }


    if (segment == 1) {
        soho_mdio_set_10G_PHY_1();
        phy_addr0 = 0;
        phy_addr1 = 1;
        phy_addr2 = 2;
        phy_addr3 = 3;
    } else if (segment == 7) {
        soho_mdio_set_10G_PHY_7();
        phy_addr0 = 4;
        phy_addr1 = 5;
        phy_addr2 = 6;
        phy_addr3 = 7;
    } else {
        printf("ERROR: Incorrect Segment: %d\n", segment);
        return;
    }

    soho_cpld_mdio_c45_read(0x1, phy_addr0, 0xCCC6, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr0, 0xCCC7, &err_cnt_low, 0);
    printf("Port 0 E2O PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));

    soho_cpld_mdio_c45_read(0x1, phy_addr1, 0xCCC6, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr1, 0xCCC7, &err_cnt_low, 0);
    printf("Port 1 E2O PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));

    soho_cpld_mdio_c45_read(0x1, phy_addr2, 0xCCC6, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr2, 0xCCC7, &err_cnt_low, 0);
    printf("Port 2 E2O PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));

    soho_cpld_mdio_c45_read(0x1, phy_addr3, 0xCCC6, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr3, 0xCCC7, &err_cnt_low, 0);
    printf("Port 3 E2O PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));

    soho_cpld_mdio_c45_read(0x1, phy_addr0, 0xCC86, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr0, 0xCC87, &err_cnt_low, 0);
    printf("Port 0 O2E PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));

    soho_cpld_mdio_c45_read(0x1, phy_addr1, 0xCC86, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr1, 0xCC87, &err_cnt_low, 0);
    printf("Port 1 O2E PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));

    soho_cpld_mdio_c45_read(0x1, phy_addr2, 0xCC86, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr2, 0xCC87, &err_cnt_low, 0);
    printf("Port 2 O2E PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));

    soho_cpld_mdio_c45_read(0x1, phy_addr3, 0xCC86, &err_cnt_hi, 0);
    soho_cpld_mdio_c45_read(0x1, phy_addr3, 0xCC87, &err_cnt_low, 0);
    printf("Port 3 O2E PRBS error count=0x%x\n", ((err_cnt_hi << 16) | err_cnt_low));
}

U_BOOT_CMD(
           ael2006_prbs_err_cnt, 
           2, 
           1, 
           ael2006_prbs_err_cnt,
           "ael2006_prbs_err_cnt - show prbs error counter in all ports \n",
           "ael2006_prbs_err_cnt <segment>"
          );

