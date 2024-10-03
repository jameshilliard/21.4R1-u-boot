/*
 * $Id: ls609x_uboot_drv.c,v 1.4.64.1 2009-08-27 06:23:45 dtang Exp $
 *
 * ls609x_uboot_drv.c - 88E609x basic driver to 
 * enable tftpboot
 *
 * Richa Singla, January-2009
 *
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
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
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>
 */

#include "ls609x_uboot_drv.h"


/*
 * ls609x_octeon_smi_set_clock
 *    Sets the clock speed for SMI transactions
 *
 * Arguments:
 *   None
 *
 * Returns:
 *    EOK on sucess, error on failure
 */
static int 
ls609x_octeon_smi_set_clock (void)
{
    DECLARE_GLOBAL_DATA_PTR;
    int cpu_clock_mhz, phase=0;
    uint64_t smi_clk_reg = 0;

    cpu_clock_mhz = gd->cpu_clock_mhz;

    /* 
     * MDI frequency is computed from core frequency as follows:
     *                    Core frequency
     * MDI Frequency = --------------------     2 < SMI_CLK[PHASE] < 128
     *                 (SMI_CLK[PHASE] x 2)
     *
     * So SMI_CLK[PHASE] = Core Frequency / (MDI Frequency * 2) <Round Up>
     * Given the maximum SMI frequency supported on the system, we compute
     * SMI_CLK[PHASE] value that gives us a frequency just less than the
     * maximum
     */
    phase = (cpu_clock_mhz * 1000) / (LS609X_OCTEON_SW_MAX_SMI_CLK_KHZ * 2)
            + 1;

    /* 
     * In the unlikely case that the divider is beyond the allowable range,
     * use the best value within the range
     */
    if (phase < 3) {
        phase = 3;
    } else if (phase > 127) {
        phase = 127;
    }

    smi_clk_reg = LS609X_OCTEON_SW_SAMPLE | phase;
    ls609x_octeon_write_csr(LS609X_OCTEON_SMI_CLK, smi_clk_reg);    

    return EOK;
}

/*
 * ls609x_octeon_smi_init
 *    Initialize the Octeon SMI block.
 *
 * Arguments:
 *   None
 *
 * Returns:
 *    EOK on sucess, error on failure
 *
 */
static int 
ls609x_octeon_smi_init (void)
{
    uint64_t val = 0;

    val = (uint64_t)ls609x_octeon_read_csr(LS609X_OCTEON_SMI_EN);
    val &= ~(0x1); /* Disable SMI transcations */
    ls609x_octeon_write_csr(LS609X_OCTEON_SMI_EN, val);

    ls609x_octeon_smi_set_clock();

    val = (uint64_t)ls609x_octeon_read_csr(LS609X_OCTEON_SMI_EN);
    val |= 0x1; /* Enable SMI transcations */
    ls609x_octeon_write_csr(LS609X_OCTEON_SMI_EN, val);

    return EOK;
}

/*
 * ls609x_wr_reg
 *    Function implementing SMI write operation.
 *
 * Arguments:
 *   phy_adr - SMI device address
 *   reg_adr - Register to write
 *   dat     - Data to be written
 *
 * Returns:
 *    EOK on sucess, error on failure
 *
 */
int 
ls609x_wr_reg (uint16_t phy_adr, uint16_t reg_adr,
               uint16_t dat)
{
    uint16_t                   smi_retry_count = 0;
    ls609x_octeon_smi_cmd_t    smi_cmd;
    ls609x_octeon_smi_rd_dat_t smi_rd_data;
    ls609x_octeon_smi_wr_dat_t smi_wr_data;

    /* Check register address validity */
    if (reg_adr > LS609X_OCTEON_MAX_VALID_SMI_REG_ADDR) {
        printf("SMI: Invalid register 0x%x to write\n", reg_adr);
        return -1;
    }

    /* write data to SMI_WR_DAT */
    smi_wr_data.s.dat = dat;
    ls609x_octeon_write_csr(LS609X_OCTEON_SMI_WR_DAT, smi_wr_data.u64);

    /* write the SMI_CMD register */
    smi_cmd.s.phy_adr = phy_adr;
    smi_cmd.s.reg_adr = reg_adr;
    smi_cmd.s.phy_op  = 0; /* write op */

    ls609x_octeon_write_csr(LS609X_OCTEON_SMI_CMD, smi_cmd.u64);

    /* check for write data valid */
    do {
        udelay(LS609X_WAIT_USECS);
        smi_retry_count++;
        smi_rd_data.u64 = 
            (uint64_t)ls609x_octeon_read_csr(LS609X_OCTEON_SMI_WR_DAT);
        if (smi_rd_data.s.val) {
            break;
        }
    } while (smi_retry_count < LS609X_OCTEON_MAX_SMI_RETRY_COUNT);

    if (smi_retry_count >= LS609X_OCTEON_MAX_SMI_RETRY_COUNT) {
        printf("SMI: Write operation timed out for register 0x%x\n", 
               reg_adr);
        return -1;
    }
    return 0;
}

/*
 * ls609x_rd_reg
 *    Function implementing SMI read operation.
 *
 * Arguments:
 *   phy_adr - SMI device address
 *   reg_adr - Register to be read
 *   val     - Value read from register
 *
 * Returns:
 *    EOK on sucess, error on failure
 *
 */
int
ls609x_rd_reg (uint16_t phy_adr, uint16_t reg_adr,
               uint16_t *val)
{
    ls609x_octeon_smi_cmd_t    smi_cmd;
    ls609x_octeon_smi_rd_dat_t smi_rd_data;
    uint16_t                   smi_retry_count = 0;

    /* Check register address validity */
    if (reg_adr > LS609X_OCTEON_MAX_VALID_SMI_REG_ADDR) {
        printf("SMI: Invalid register 0x%x to read\n", reg_adr);
        return -1;
    }
    
    smi_cmd.s.phy_adr = phy_adr;
    smi_cmd.s.reg_adr = reg_adr;
    smi_cmd.s.phy_op  = 1; /* read op */

    ls609x_octeon_write_csr(LS609X_OCTEON_SMI_CMD, smi_cmd.u64);
    
    do {
        udelay(LS609X_WAIT_USECS);
        smi_retry_count++;
        smi_rd_data.u64 = 
            (uint64_t)ls609x_octeon_read_csr(LS609X_OCTEON_SMI_RD_DAT);
        if (smi_rd_data.s.val) {
            break;
        }
    } while (smi_retry_count < LS609X_OCTEON_MAX_SMI_RETRY_COUNT);

    if (smi_retry_count >= LS609X_OCTEON_MAX_SMI_RETRY_COUNT) {
        printf("SMI: Read operation timed out for register 0x%x\n", 
               reg_adr);
        return -1;
    }

    *val =  (uint16_t)smi_rd_data.s.dat;

    return EOK;
}


/*
 * ls609x_drv_frontend_port_init :
 * It initializes the 88E609x uboot port i.e port 0
 * Disable the other i.e port 1 to port 7
 *
 * Returns:
 *   int error code
 */
static int
ls609x_drv_frontend_port_init (void)
{
    int             err         = EOK;
    uint16_t        sword       = 0;
    uint8_t        ls609x_port = 0;
    
    /*
     *First initialize the uboot port
     */
     ls609x_port = LS609X_UBOOT_PORT;

    /* 
     * Set the Egress mode to default mode 
     * i.e packets are transmitted unmodified 
     */
    ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL, &sword);

    LS609X_PORT_EGRESS_UNMODIFIED(sword); 

    ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL, sword);

    /* 
     * Set the Frame mode to Normal Mode 
     */
    ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL, &sword);

    LS609X_PORT_FRAME_NORMAL(sword);

    ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL, sword);

    /*
     * Disable learning on this port
     */
    ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_VLAN_MAP, &sword);

    sword |= LS609X_PORT_LEARN_DISABLE; 

    ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_VLAN_MAP, sword);

    /* Disable 802.1Q Mode */
    ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL2, &sword);

    LS609X_DISABLE_802_1Q(sword);

    ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL2, sword);

    /* 
     * Set the Port State as forwarding 
     */ 
    ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL, &sword);

    sword |=  LS609X_PORT_STATE_FORWARDING;

    ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_CONTROL, sword);

    /*
     * Forward All packets to CPU Port  
     */
    ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_VLAN_MAP, &sword);

    LS609X_FWD_CPU_PORT(sword);

    ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                  LS609X_PORT_VLAN_MAP, sword);

    /*
     * Disable all other frontend ports
     */
    for(ls609x_port = LS609X_FRONTEND_PORT1; 
        ls609x_port <= LS609X_FRONTEND_PORT7; ls609x_port++) {

        /* 
         * Set the Port State as disable 
         */ 
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                       LS609X_PORT_CONTROL, &sword);

        LS609X_PORT_STATE_DISABLE(sword); 

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                       LS609X_PORT_CONTROL, sword);
    }

    return err;
}

/*
 * ls609x_drv_phy_init:
 *   Do the necessary initialization for internal phy
 *
 * Arguments:
 *   None
 *
 * Returns:
 *   int error code
 */
static int
ls609x_drv_phy_init (void)
{
    int       err   = EOK;
    uint16_t sword  = 0;
    uint32_t ls609x_phy;
    
    /* 
     * Disable the PPU
     */
    ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, &sword);

    sword &= ~BIT_14S;

    ls609x_wr_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, sword);
    
    /* 
     * Initialize phy connected to uboot port
     */

    /* 
     * Advertise the phy capability
     */
    ls609x_rd_reg(LS609X_SMI_PHY_0, LS609X_PHY_AUTONEGO_AD, &sword);

    sword |= LS609X_PHY_AUTONEGO_AD_100_FULL | LS609X_PHY_AUTONEGO_AD_100_HALF;
    sword |= LS609X_PHY_AUTONEGO_AD_10_FULL | LS609X_PHY_AUTONEGO_AD_10_HALF;

    ls609x_wr_reg(LS609X_SMI_PHY_0, LS609X_PHY_AUTONEGO_AD, sword);

    /* 
     * Enable Auto-negotiation
     */

    ls609x_rd_reg(LS609X_SMI_PHY_0, LS609X_PHY_CONTROL, &sword);

    sword |= LS609X_PHY_CONTROL_SW_RESET | LS609X_PHY_CONTROL_AUTONEGO; 
    sword |= LS609X_PHY_CONTROL_SPEED_LSB | LS609X_PHY_CONTROL_DUPLEX;
    LS609X_PHY_POWER_DOWN_NORMAL(sword);
    LS609X_PHY_CONTROL_SPEED_MSB(sword);

    ls609x_wr_reg(LS609X_SMI_PHY_0, LS609X_PHY_CONTROL, sword);

    /* 
     * Program the two LED's 
     * LED1: inactive 
     * LED2: on left side on front panel is used for link 
     * and link activity
     */

    ls609x_rd_reg(LS609X_SMI_PHY_0, LS609X_PHY_PLED_SELECT, &sword);

    LS609X_PHY_PRG_LED1(sword, LS609X_PHY_LED_INACTIVE); 
    LS609X_PHY_PRG_LED2(sword, LS609X_PHY_LED_LINK_ACT); 

    ls609x_wr_reg(LS609X_SMI_PHY_0, LS609X_PHY_PLED_SELECT, sword);

    /* Program the LED's blinking rate */
    ls609x_rd_reg(LS609X_SMI_PHY_0, LS609X_PHY_LED_CONTROL, &sword);

    LS609X_PHY_PRG_BLINK_RATE(sword, LS609X_PHY_LED_BLINK_RATE_170); 
    LS609X_PHY_PRG_SR_STR_UP(sword, LS609X_PHY_LED_SR_STR_UP_170);

    ls609x_wr_reg(LS609X_SMI_PHY_0, LS609X_PHY_LED_CONTROL, sword);

    /* Manually overide and turn off the link led of PHYs other than PHY0 */
    for (ls609x_phy = LS609X_FRONTEND_PORT1; 
         ls609x_phy <= LS609X_FRONTEND_PORT7; ls609x_phy++) {

         ls609x_rd_reg(ls609x_phy, LS609X_PHY_LED_MANUAL_OVERRIDE, &sword);
         LS609X_PHY_PRG_LED0_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_OFF);
         LS609X_PHY_PRG_LED1_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_OFF);
         LS609X_PHY_PRG_LED2_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_OFF);
         ls609x_wr_reg(ls609x_phy, LS609X_PHY_LED_MANUAL_OVERRIDE, sword);
    }

    return err;
}

/*
 * ls609x_drv_uplink_port_init:
 *   Do the necessary initialization for cpu port
 *
 * Returns:
 *   int error code
 */
static int 
ls609x_drv_uplink_port_init (void)
{
    int       err   = EOK;
    uint16_t sword  = 0;
    
    /* 
     * Set the Egress mode to default mode 
     * i.e packets are transmitted unmodified 
     */
    ls609x_rd_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_CONTROL, &sword);

    LS609X_PORT_EGRESS_UNMODIFIED(sword); 

    ls609x_wr_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_CONTROL, sword);

    /* 
     * Set the Frame mode to Normal Network 
     */
    ls609x_rd_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_CONTROL, &sword);

    LS609X_PORT_FRAME_NORMAL(sword);

    ls609x_wr_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_CONTROL, sword);
    /* 
     * Force 1000Mbps and full-duplex and disable auto-neg 
     */
    ls609x_rd_reg(LS609X_SMI_UPLINK_PORT, LS609X_PCS_CONTROL, &sword);

    LS609X_PCS_FORCE_SPEED_1000(sword);
    sword |= LS609X_PCS_FORCE_DUPLEX | LS609X_PCS_FULL_DUPLEX;  
    LS609X_PCS_DISABLE_INBAND_AUTONEG(sword); 

    ls609x_wr_reg(LS609X_SMI_UPLINK_PORT, LS609X_PCS_CONTROL, sword);

    /* 
     * Since uplink port is not connected to any external phy
     * we have to force link up
     */
    ls609x_rd_reg(LS609X_SMI_UPLINK_PORT, LS609X_PCS_CONTROL, &sword);

    sword |= LS609X_PCS_FORCE_LINK | LS609X_PCS_LINK_UP;  

    ls609x_wr_reg(LS609X_SMI_UPLINK_PORT, LS609X_PCS_CONTROL, sword);

    /* 
     * Set the Port State as forwarding 
     */ 
    ls609x_rd_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_CONTROL, &sword);

    sword |=  LS609X_PORT_STATE_FORWARDING;

    ls609x_wr_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_CONTROL, sword);

    /*
     * Forward All packets to Port 0 
     */
    ls609x_rd_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_VLAN_MAP, &sword);

    LS609X_FWD_UBOOT_PORT(sword);

    ls609x_wr_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_VLAN_MAP, sword);
    
    return err;
}

/*
 * ls609x_uboot_drv_init :
 *   performs initialization
 *
 *
 * Returns:
 *   EOK - success
 */
int
ls609x_uboot_drv_init ()
{
    int         err       = EOK;
    uint16_t   sword      = 0;

    /* Initialize SMI for 88E609x access */
    ls609x_octeon_smi_init();
    
    /* 
     * Initialize the Uplink port i.e port 10 
     */
    err = ls609x_drv_uplink_port_init();
    if (err != EOK) {
        printf("CPU port init failed\n");
        goto cleanup;
    }

    /* 
     * LED and PHY initialization
     */
    
    err = ls609x_drv_phy_init();
    if (err != EOK) {
        printf("Phy init failed\n");
        goto cleanup;
    }

    /*
     * Global Parameter Initilization
     */

    /*
     * Set MTU to 1522, as 1632 is not a standard value.
     * Note: This means that Jumbo frames will not be supported
     * and that MTU value can be set globally instead of per port.
     */
    ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, &sword);

    LS609X_SET_MAX_FRAME_1522(sword);

    ls609x_wr_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, sword);

    /*
     * Disable all interrupts */
    ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, &sword);

    LS609X_DISABLE_INTERRUPTS(sword);

    ls609x_wr_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, sword);

    /* 
     * Initialize the uboot  ports i.e port 0 and disable the others
     * i.e port 1 to port 7
     */
    err = ls609x_drv_frontend_port_init();

    if (err != EOK) {
        printf("frontend port init failed\n");
        goto cleanup;
    }
    printf("pic init done (err = %u)", err);

cleanup:

    return err;
}

int 
ls609x_drv_get_stat (uint8_t counter, uint32_t *dword)
{
    uint16_t   sword    = 0;
    int        loop_cnt = 0;
    int        err      = EOK;

    /* Check busy bit */
    ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_OPERATION,
                   &sword);
    if (sword & LS609X_STATS_BUSY) {
        
        printf("Stats unit busy: Unable to get stats\n");

        err = -1;
        return -1;
    }

    /* Load the specific counter (stat) */
    sword = (LS609X_STATS_BUSY | LS609X_STATS_OP_PORT_RD_CAPT | 
             LS609X_STATS_HIST_MODE_BOTH | counter);
    ls609x_wr_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_OPERATION,
                  sword);

   /* Stats busy bit should be cleared before collecting the relevant stats */
    do {
        udelay(LS609X_STATS_WAIT_BUSY);
        ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_OPERATION, 
                       &sword);

        if (loop_cnt++ >= LS609X_STATS_MAX_RETRY) {
           printf("Timed out while loading stats counter\n");
           err = -1;
           return -1;
        }

    } while (sword & LS609X_STATS_BUSY);

    /* Read the 32 bit counter value */
    ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_COUNTER3_2,
                  &sword);
    *dword = sword;

    ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_COUNTER1_0,
                  &sword);
    *dword = (*dword << 16) | sword;

    return 0;
}

/*
 * ls609x_drv_eth_getstats:
 *
 * This function reads the statistics from the 88E609x chip
 * and populates in the if_eth_stats_t structure
 * 
 */
int
ls609x_drv_eth_getstats (uint16_t port)
{
    int                 loop_cnt = 0;
    uint16_t            sword    = 0;
    uint32_t            dword    = 0;
    int                 err      = EOK;
    
    if(port > LS609X_UPLINK_PORT){
        printf("\n print the port number in hex (0...7 & A)\n");
        err = -1;
        return err;
    }
    
    printf("\n Stats for port %d \n", port);
    /* Check whether the stats unit is free */
    ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_OPERATION,
                  &sword);
    if (sword & LS609X_STATS_BUSY) {
        
        printf("Stats unit busy:Unable to get"
                   "stats for port %d \n",port);

        err = -1;
        return err;
    }
    
    /* Initiate capture all counters for port */
    sword = (LS609X_STATS_BUSY | LS609X_STATS_OP_PORT_CAPT_ALL |
             LS609X_STATS_HIST_MODE_BOTH | port);

    ls609x_wr_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_OPERATION,
                  sword);

   /* Stats busy bit should be cleared before collecting the relevant stats */
    do {
        udelay(LS609X_STATS_WAIT_BUSY);
        ls609x_rd_reg(LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_STATS_OPERATION, 
                      &sword);

        if (loop_cnt++ >= LS609X_STATS_MAX_RETRY) {
           printf("Timed out while taking stats snapshot\n");
           err = -1;
           return err;
        }

    } while (sword & LS609X_STATS_BUSY);

    /* Now collect all relevant stats */

    /* Ingress Counters */
    ls609x_drv_get_stat(LS609X_STATS_PTR_IN_UNICAST, 
                        &dword);
    printf("\n Stats RX: unicast = 0x%x \n", dword);
    ls609x_drv_get_stat(LS609X_STATS_PTR_IN_BROADCAST,
                        &dword);
    printf("Stats RX: broadcast = 0x%x \n", dword);
    ls609x_drv_get_stat(LS609X_STATS_PTR_IN_MULTICAST,
                        &dword);
    printf("Stats RX: multicast = 0x%x \n", dword);
    ls609x_drv_get_stat(LS609X_STATS_PTR_IN_RxErr,
                        &dword);
    printf("Stats RX: RX ERR = 0x%x \n", dword);
    ls609x_drv_get_stat(LS609X_STATS_PTR_IN_FCS_ERR,
                        &dword);
    printf("Stats RX: FCS ERR = 0x%x \n", dword);
    /* Egress Counter */
    ls609x_drv_get_stat(LS609X_STATS_PTR_OUT_UNICAST,
                        &dword);
    printf("Stats TX: unicast = 0x%x \n", dword);
    ls609x_drv_get_stat(LS609X_STATS_PTR_OUT_BROADCAST,
                        &dword);
    printf("Stats TX: broadcast = 0x%x \n", dword);
    ls609x_drv_get_stat(LS609X_STATS_PTR_OUT_MULTICAST,
                        &dword);
    printf("Stats TX:multicast = 0x%x \n", dword);
    ls609x_drv_get_stat(LS609X_STATS_PTR_OUT_FCS_ERR, 
                        &dword);
    printf("Stats TX:fcs err = 0x%x \n", dword);

    return 0;

}

/*
 * ls609x_drv_diag_phy_init:
 *   Do the necessary initialization for internal phy
 *   for port 1 to 7, port 0 is already intialized
 *
 * Arguments:
 *   None
 *
 * Returns:
 *   int error code
 */
static int
ls609x_drv_diag_phy_init(void)
{
    int       err         = EOK;
    u_int16_t sword       = 0;
    u_int8_t  ls609x_port = 0;
    
    /* 
     * Disable the PPU
     */
    ls609x_rd_reg( LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, &sword);

    sword &= ~BIT_14S;

    ls609x_wr_reg( LS609X_SMI_GLOBAL, LS609X_SW_GLOBAL_CONTROL, sword);
    
    for(ls609x_port =   LS609X_FRONTEND_PORT1;
        ls609x_port <=  LS609X_FRONTEND_PORT7; 
        ls609x_port++){

        /* Undo Manually overide*/
        ls609x_rd_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_LED_MANUAL_OVERRIDE, &sword);

        LS609X_PHY_PRG_LED0_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_NORMAL);
        LS609X_PHY_PRG_LED1_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_NORMAL);
        LS609X_PHY_PRG_LED2_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_NORMAL);

        ls609x_wr_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_LED_MANUAL_OVERRIDE, sword);

        /* 
         * Advertise the phy capability
         */
        ls609x_rd_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_AUTONEGO_AD, &sword);

        sword |= LS609X_PHY_AUTONEGO_AD_100_FULL | LS609X_PHY_AUTONEGO_AD_100_HALF;
        sword |= LS609X_PHY_AUTONEGO_AD_10_FULL | LS609X_PHY_AUTONEGO_AD_10_HALF;

        ls609x_wr_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_AUTONEGO_AD, sword);

        /* 
         * Enable Auto-negotiation
         */

        ls609x_rd_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_CONTROL, &sword);

        sword |= LS609X_PHY_CONTROL_SW_RESET | LS609X_PHY_CONTROL_AUTONEGO; 
        sword |= LS609X_PHY_CONTROL_SPEED_LSB | LS609X_PHY_CONTROL_DUPLEX;
        LS609X_PHY_POWER_DOWN_NORMAL(sword);
        LS609X_PHY_CONTROL_SPEED_MSB(sword);

        ls609x_wr_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_CONTROL, sword);

        /* 
         * Program the two LED's 
         * LED1: inactive 
         * LED2: on left side on front panel is used for link 
         * and link activity
         */

        ls609x_rd_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_PLED_SELECT, &sword);

        LS609X_PHY_PRG_LED1(sword, LS609X_PHY_LED_INACTIVE); 
        LS609X_PHY_PRG_LED2(sword, LS609X_PHY_LED_LINK_ACT); 

        ls609x_wr_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_PLED_SELECT, sword);

        /* Program the LED's blinking rate */
        ls609x_rd_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_LED_CONTROL, &sword);

        LS609X_PHY_PRG_BLINK_RATE(sword, LS609X_PHY_LED_BLINK_RATE_170); 
        LS609X_PHY_PRG_SR_STR_UP(sword, LS609X_PHY_LED_SR_STR_UP_170);

        ls609x_wr_reg((LS609X_SMI_PHY_0 + ls609x_port), 
                      LS609X_PHY_LED_CONTROL, sword);

    }
    return err;
}

/*
 * ls609x_drv_diag_frontend_port_init :
 * It initializes the 88E6097 port i.e port 1 to 7
 * port 0 is already initialized
 *
 * Returns:
 *   int error code
 */
static int
ls609x_drv_diag_frontend_port_init(void)
{
    int             err         = EOK;
    u_int16_t       sword       = 0;
    u_int8_t        ls609x_port = 0;
    
    for(ls609x_port = LS609X_FRONTEND_PORT1; ls609x_port <= 
        LS609X_FRONTEND_PORT7; ls609x_port++){
        
        /* 
         * Set the Egress mode to default mode 
         * i.e packets are transmitted unmodified 
         */
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                       LS609X_PORT_CONTROL, &sword);

        LS609X_PORT_EGRESS_UNMODIFIED(sword); 

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_CONTROL, sword);

        /* 
         * Set the Frame mode to Normal Mode 
         */
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_CONTROL, &sword);

        LS609X_PORT_FRAME_NORMAL(sword);

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_CONTROL, sword);

        /*
         * Disable learning on this port
         */
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_VLAN_MAP, &sword);

        sword |= LS609X_PORT_LEARN_DISABLE; 

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_VLAN_MAP, sword);

        /* Disable 802.1Q Mode */
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_CONTROL2, &sword);

        LS609X_DISABLE_802_1Q(sword);

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_CONTROL2, sword);

        /* 
         * Set the Port State as forwarding 
         */ 
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_CONTROL, &sword);
   
        sword |=  LS609X_PORT_STATE_FORWARDING;

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_CONTROL, sword);

        /*
         * Forward All packets to CPU Port  
         */
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_VLAN_MAP, &sword);

        LS609X_FWD_CPU_PORT(sword);

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                      LS609X_PORT_VLAN_MAP, sword);
    }

    return err;
}

/*
 * ls609x_uboot_drv_diag_init :
 *   performs diag initialization
 *
 *
 * Returns:
 *   EOK - success
 */
int
ls609x_uboot_drv_diag_init ()
{
    int        err        = EOK;
    uint16_t   sword      = 0;

    /*
     * Forward config
     */
    ls609x_rd_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_VLAN_MAP, &sword);

    LS609X_FWD_ALL_PORT(sword);

    ls609x_wr_reg(LS609X_SMI_UPLINK_PORT, LS609X_PORT_VLAN_MAP, sword);
    
    /* 
     * LED and PHY initialization
     */
    
    err = ls609x_drv_diag_phy_init();
    if (err != EOK) {
        printf("Phy init failed\n");
        goto cleanup;
    }

    /* 
     * Initialize the diag ports i.e port 1 to 7
     * 0 is already initilaized
     */
    err = ls609x_drv_diag_frontend_port_init();

    if (err != EOK) {
        printf("frontend port init failed\n");
        goto cleanup;
    }
    printf("diag init done (err = %u)", err);

cleanup:

    return err;
}

/*
 * ls609x_uboot_drv_diag_deinit :
 *   performs diag de-initialization
 *
 *
 * Returns:
 *   EOK - success
 */
int
ls609x_uboot_drv_diag_deinit ()
{
    int        err        = EOK;
    uint16_t   sword      = 0;
    uint32_t   ls609x_phy;
    uint8_t    ls609x_port;
    
    /* Manually overide and turn off the link led of PHYs other than PHY0 */
    for (ls609x_phy = LS609X_FRONTEND_PORT1; 
         ls609x_phy <= LS609X_FRONTEND_PORT7; ls609x_phy++) {

         ls609x_rd_reg(ls609x_phy, LS609X_PHY_LED_MANUAL_OVERRIDE, &sword);
         
         LS609X_PHY_PRG_LED0_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_OFF);
         LS609X_PHY_PRG_LED1_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_OFF);
         LS609X_PHY_PRG_LED2_MAN_OVRDRV(sword, LS609X_PHY_MANOVRDRV_LED_OFF);
         
         ls609x_wr_reg(ls609x_phy, LS609X_PHY_LED_MANUAL_OVERRIDE, sword);
    }

    /*
     * Disable all other frontend ports
     */
    for(ls609x_port = LS609X_FRONTEND_PORT1; 
        ls609x_port <= LS609X_FRONTEND_PORT7; ls609x_port++) {

        /* 
         * Set the Port State as disable 
         */ 
        ls609x_rd_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                       LS609X_PORT_CONTROL, &sword);

        LS609X_PORT_STATE_DISABLE(sword); 

        ls609x_wr_reg((LS609X_SMI_PORT_0 + ls609x_port), 
                       LS609X_PORT_CONTROL, sword);
    }
    
    return err;
} 
