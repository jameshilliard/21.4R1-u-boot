/***********************license start************************************
 * Copyright (c) 2005-2007  Cavium Networks (support@cavium.com). 
 * All rights reserved.
 * 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of Cavium Networks nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.  
 * 
 * This Software, including technical data, may be subject to U.S.  export 
 * control laws, including the U.S.  Export Administration Act and its 
 * associated regulations, and may be subject to export or import regulations 
 * in other countries.  You warrant that You will comply strictly in all 
 * respects with all such regulations and acknowledge that you have the 
 * responsibility to obtain licenses to export, re-export or import the 
 * Software.  
 * 
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS" 
 * AND WITH ALL FAULTS AND CAVIUM NETWORKS MAKES NO PROMISES, REPRESENTATIONS 
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH 
 * RESPECT TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY 
 * REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT 
 * DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES 
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR 
 * PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET 
 * POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT 
 * OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 * 
 ***********************license end**************************************/

/**
 *
 * $Id$
 * 
 */
 
 
#include <common.h>
#include <command.h>
#include <exports.h>
#include <linux/ctype.h>
#include <lib_octeon.h>
#include <octeon_eeprom_types.h>
#include <net.h>
#include <miiphy.h>
#include <dxj106.h>
#include "octeon_boot.h"
#include "octeon_eeprom_types.h"
#include "lib_octeon_shared.h"
#include "cvmx-config.h"
#include "cvmx-wqe.h"
#include "cvmx-pow.h"
#include "cvmx-pko.h"
#include "cvmx-ipd.h"
#include "cvmx-pip.h"
#include "cvmx-spi.h"
#include "cvmx-mdio.h"
#include "cvmx-helper.h"
#include "cvmx-mgmt-port.h"
#include "cvmx-helper-util.h"
#include "cvmx-bootmem.h"

#ifdef DEBUG
#define dprintf printf
#else
#define dprintf(...)
#endif

#ifdef CONFIG_JSRXNLE
#include "configs/jsrxnle.h"
static int rx_sync_flag = 0;
int   loader_init = SRX240_LOADER_NOT_STARTED;
int   ethact_init = 0;

#define MRVL_ADJ_BYTES  12
#endif

/* To print Xaui rx/tx statistics */
typedef struct {
    uint32_t    packets;
    uint32_t    controls;
    uint32_t    dmacs;
    uint32_t    drops;
    uint32_t    errors;
    uint32_t    rsvrd;        /* To preserve 64-bit alignment */
    uint64_t    octets;
    uint64_t    controlOctets;
    uint64_t    dmacOctets;
    uint64_t    dropOctets;
} cvmx_gmx_rx_status_t;

typedef struct {
    uint64_t    octets;
    uint32_t    packets;
    uint32_t    multicasts;
    uint32_t    broadcasts;
    uint32_t    underflows;
    uint32_t    controls;
    uint32_t    singleCollides;
    uint32_t    multiCollides;
    uint32_t    dropCollides;
    uint32_t    dropDefers;
    uint32_t    histSmall;
    uint32_t    hist64;
    uint32_t    hist127;
    uint32_t    hist255;
    uint32_t    hist511;
    uint32_t    hist1023;
    uint32_t    hist1518;
    uint32_t    histLarge;
} cvmx_gmx_tx_status_t;

#ifdef CONFIG_MAG8600
#include "configs/mag8600.h"
#endif

/* Global flag indicating common hw has been set up */
int octeon_global_hw_inited = 0;

/* pointer to device to use for loopback.  Only needed for IPD reset workaround */
struct eth_device *loopback_dev;

int octeon_eth_send(struct eth_device *dev, volatile void *packet, int length);
int octeon_ebt3000_rgmii_init(struct eth_device *dev, bd_t * bis);
void octeon_eth_rgmii_enable(struct eth_device *dev);


/************************************************************************/
/* Ethernet device private data structure for octeon ethernet */
typedef struct
{
    uint32_t port;
    uint32_t interface;
    uint32_t index;
    uint32_t queue;
    uint64_t packets_sent;
    uint64_t packets_received;
    uint32_t initted_flag;
    struct eth_device *ethdev;	/** Eth device this priv is part of */
    uint32_t link_speed:2;   /* current link status, use to reconfigure on status changes */
    uint32_t link_duplex:1;
    uint32_t link_status:1;
    uint32_t loopback:1;
    uint32_t enabled:1;
    uint64_t gmx_base;	/** Base address to access GMX CSRs */
} octeon_eth_info_t;

#if CONFIG_JSRXNLE
typedef union ext_dsa_tag_ {

    uint64_t  val;

    struct dsa_tag_ {
        /* word 0 */
        uint64_t tag_cmd      : 2;
        uint64_t trg_tagd     : 1;
        uint64_t trg_dev      : 5;
        uint64_t trg_port     : 5;
        uint64_t use_vidx     : 1;
        uint64_t tc0          : 1;
        uint64_t cfi          : 1;
        uint64_t up           : 3;
        uint64_t extend       : 1;
        uint64_t vid          : 12;
        /* word 1 */
        uint64_t ext          : 1;
        uint64_t eg_flt_en    : 1;
        uint64_t casc_ctrl    : 1;
        uint64_t eg_flt_reg   : 1;
        uint64_t tc2          : 1;
        uint64_t dp_10        : 2;
        uint64_t src_id       : 5;
        uint64_t src_dev      : 5;
        uint64_t tc1          : 1;
        uint64_t mb_nbr_cpu   : 1;
        uint64_t res2         : 2;
        uint64_t trg_port5    : 1;
        uint64_t res1         : 10;

    } dsa_tag;

}ext_dsa_tag;
#endif

#if  defined(OCTEON_INTERNAL_ENET) || defined(OCTEON_MGMT_ENET)

/* Variable shared between ethernet routines. */
static int card_number;
static int agl_number;
static int available_mac_count = -1;
static uint8_t mac_addr[6];
static uint64_t port_state[50];  /* Save previous link state so we only configure/print on transitions */


#define USE_HW_TCPUDP_CHECKSUM  0

/* Make sure that we have enough buffers to keep prefetching blocks happy.
** Absolute minimum is probably about 200. */
#define NUM_PACKET_BUFFERS  1000

#define PKO_SHUTDOWN_TIMEOUT_VAL     100
/****************************************************************/
#define GMX_TX_OVR_BP			0x4c8


static void cvm_oct_fill_hw_memory(uint64_t pool, uint64_t size, uint64_t elements)
{

    int64_t memory;
    static int alloc_count = 0;
    char tmp_name[64];

    dprintf("cvm_oct_fill_hw_memory: pool: 0x%qx, size: 0xx%qx, count: 0x%qx\n", pool, size, elements);
    sprintf(tmp_name, "%s_fpa_alloc_%d", OCTEON_BOOTLOADER_NAMED_BLOCK_TMP_PREFIX, alloc_count++);
    memory = cvmx_bootmem_phy_named_block_alloc(size * elements, 0, 0x40000000, 128, tmp_name, CVMX_BOOTMEM_FLAG_END_ALLOC);
    if (memory < 0)
    {
#ifndef CONFIG_JSRXNLE
        printf("Unable to allocate %lu bytes for FPA pool %ld\n", elements*size, pool);
#endif
        return;
    }
    while (elements--)
    {
        cvmx_fpa_free((void *)(uint32_t)(memory + elements*size), pool, 0);
    }
    dprintf("cvm_oct_fill_hw_memory: pool: done \n");
}


/**
 * Set the hardware MAC address for a device
 *
 * @param interface    interface of port to set
 * @param index    index of port to set MAC address for
 * @param addr   Address structure to change it too. 
 * @return Zero on success
 */
static int cvm_oct_set_mac_address(int interface, int index, void *addr)
{
    cvmx_gmxx_prtx_cfg_t gmx_cfg;


    int i;
    uint8_t *ptr = addr;
    uint64_t mac = 0;
    for (i=0; i<6; i++)
        mac = (mac<<8) | (uint64_t)(ptr[i]);

    /* Disable interface */
    gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, interface));
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmx_cfg.u64 & ~1ull);

    cvmx_write_csr(CVMX_GMXX_SMACX(index, interface), mac);
    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM0(index, interface), ptr[0]);
    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM1(index, interface), ptr[1]);
    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM2(index, interface), ptr[2]);
    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM3(index, interface), ptr[3]);
    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM4(index, interface), ptr[4]);
    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM5(index, interface), ptr[5]);


    cvmx_gmxx_rxx_adr_ctl_t control;
    control.u64 = 0;
    control.s.bcst = 1;     /* Allow broadcast MAC addresses */
    control.s.mcst = 1; /* Force reject multicat packets */
    control.s.cam_mode = 1; /* Filter packets based on the CAM */
    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CTL(index, interface), control.u64);

    cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM_EN(index, interface), 1);


    /* Return interface to previous enable state */
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, interface), gmx_cfg.u64);

    return 0;
}

/**
 * Configure common hardware for all interfaces
 */
static void cvm_oct_configure_common_hw(void)
{

    if (getenv("disable_spi"))
    {

        /* Do this here so that we can disabled this after boot but before
        ** we use networking.
        */
        cvmx_spi_callbacks_t spi_callbacks_struct;
        cvmx_spi_get_callbacks(&spi_callbacks_struct);
        spi_callbacks_struct.reset_cb            = 0;
        spi_callbacks_struct.calendar_setup_cb   = 0;
        spi_callbacks_struct.clock_detect_cb     = 0;
        spi_callbacks_struct.training_cb         = 0;
        spi_callbacks_struct.calendar_sync_cb    = 0;
        spi_callbacks_struct.interface_up_cb     = 0;
        printf("SPI interface disabled with 'disable_spi' environment variable\n");
        cvmx_spi_set_callbacks(&spi_callbacks_struct);
    }
    /* Setup the FPA */
    cvmx_fpa_enable();

    cvm_oct_fill_hw_memory(CVMX_FPA_WQE_POOL, CVMX_FPA_WQE_POOL_SIZE, NUM_PACKET_BUFFERS);
#if CVMX_FPA_OUTPUT_BUFFER_POOL != CVMX_FPA_PACKET_POOL
    cvm_oct_fill_hw_memory(CVMX_FPA_OUTPUT_BUFFER_POOL, CVMX_FPA_OUTPUT_BUFFER_POOL_SIZE, 128);
#endif
    cvm_oct_fill_hw_memory(CVMX_FPA_PACKET_POOL, CVMX_FPA_PACKET_POOL_SIZE, NUM_PACKET_BUFFERS);

    cvmx_helper_initialize_packet_io_global();
    cvmx_helper_initialize_packet_io_local();

    /* Set POW get work timeout to maximum value */
    cvmx_write_csr(CVMX_POW_NW_TIM, 0x3ff);
}

/**
 * Packet transmit
 *
 * @param skb    Packet to send
 * @param dev    Device info structure
 * @return Always returns zero
 */
static int cvm_oct_xmit(struct eth_device *dev, void *packet, int len)
{
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;
    cvmx_pko_command_word0_t    pko_command;
    cvmx_buf_ptr_t              hw_buffer;


    dprintf("cvm_oct_xmit addr: %p, len: %d\n", packet, len);


    /* Build the PKO buffer pointer */
    hw_buffer.u64 = 0;
    hw_buffer.s.addr = cvmx_ptr_to_phys(packet);
    hw_buffer.s.pool = CVMX_FPA_PACKET_POOL;
    hw_buffer.s.size = CVMX_FPA_PACKET_POOL_SIZE;

    /* Build the PKO command */
    pko_command.u64 = 0;
    pko_command.s.subone0 = 1;
    pko_command.s.dontfree = 0;
    pko_command.s.segs = 1;
    pko_command.s.total_bytes = len;
    /* Send the packet to the output queue */

    cvmx_pko_send_packet_prepare(priv->port, priv->queue, 0);
    if (cvmx_pko_send_packet_finish(priv->port, priv->queue, pko_command, hw_buffer, 0))
    {
#ifndef CONFIG_JSRXNLE
        printf("Failed to send the packet\n");
#endif
    }
	return 0;
}

/**
 * Configure the RGMII port for the negotiated speed
 *
 * @param dev    Linux device for the RGMII port
 */
static void cvm_oct_configure_rgmii_speed(struct eth_device *dev)
{
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;
    cvmx_helper_link_info_t link_state = cvmx_helper_link_get(priv->port);

    if (link_state.u64 != port_state[priv->port])
    {

        dprintf("%s: ", dev->name);
        if (!link_state.s.link_up)
            dprintf("Down ");
        else
        {
            dprintf("Up ");
            dprintf("%d Mbps ", link_state.s.speed);
            if (link_state.s.full_duplex)
                dprintf("Full duplex ");
            else
                dprintf("Half duplex ");
        }
        dprintf("(port %2d)\n", priv->port);
        cvmx_helper_link_set(priv->port, link_state);
        port_state[priv->port] = link_state.u64;
    }
}
int octeon_network_hw_shutdown_internal(void)
{
    int port, queue;
    int retval = 0;
    int i;
    int num_interfaces;

    num_interfaces = cvmx_helper_get_number_of_interfaces();
    if (num_interfaces > 2)
        num_interfaces = 2;

    octeon_eth_info_t *oct_eth_info = NULL;

    /* Don't need to do anything if networking not used */
    if (!octeon_global_hw_inited)
        return(retval);
    if (octeon_is_model(OCTEON_CN38XX_PASS1))
    {
        printf("\n");
        printf("\n");
        printf("ERROR: TFTP network reset not supported on PASS 1\n");
        printf("\n");
        return -1;
    }
    /* For pass 2 we need to make sure the device that we selected to loop back packets
    ** on has been initialized */
    if (octeon_is_model(OCTEON_CN38XX_PASS2))
    {
        if (!loopback_dev)
        {
            printf("Neither interface is RGMII, unable to reset network HW\n");
            return -1;
        }
        oct_eth_info = (octeon_eth_info_t *)loopback_dev->priv;
        dprintf("using port %d to loopback packets for ipd workaround\n", oct_eth_info->port);
    }


    /* Disable any further packet reception */
    for (i = 0; i < num_interfaces; i++)
    {
        if (CVMX_HELPER_INTERFACE_MODE_RGMII == cvmx_helper_interface_get_mode(i)
            || CVMX_HELPER_INTERFACE_MODE_GMII == cvmx_helper_interface_get_mode(i)
            || CVMX_HELPER_INTERFACE_MODE_SGMII == cvmx_helper_interface_get_mode(i)
            || CVMX_HELPER_INTERFACE_MODE_XAUI == cvmx_helper_interface_get_mode(i))
        {
            cvmx_write_csr(CVMX_ASXX_RX_PRT_EN(i), 0x0);   /* Disable the RGMII RX ports */
            cvmx_write_csr(CVMX_ASXX_TX_PRT_EN(i), 0x0);   /* Disable the RGMII TX ports */
            int num_ports = cvmx_helper_ports_on_interface(i);
            int j;
            for (j = 0; j < num_ports;j++)
            {
                /* Disable MAC filtering */
                cvmx_gmxx_prtx_cfg_t gmx_cfg;
                gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(j, i));
                cvmx_write_csr(CVMX_GMXX_PRTX_CFG(j, i), gmx_cfg.u64 & ~1ull);
                cvmx_write_csr(CVMX_GMXX_RXX_ADR_CTL(j, i), 1);
                cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM_EN(j, i), 0);
                cvmx_write_csr(CVMX_GMXX_PRTX_CFG(j, i), gmx_cfg.u64);
            }
        }
    }
    /* TODO: If SPI interface is used, the SPI device should be configured to stop receiving packets*/


    /* Make sure PKO is empty.  Read doorbell count
    ** for configured output queues.  We at most have configured
    ** ports 0-9, 16-25 */
    int timeout;
    uint32_t count;
    for (i = 0; i < num_interfaces; i++)
    {
        int ports_per_interface = cvmx_helper_ports_on_interface(i);
        for (port = i*16; port < i*16 + ports_per_interface; port++)
        {
            queue = cvmx_pko_get_base_queue(port);
            timeout = PKO_SHUTDOWN_TIMEOUT_VAL;
            /* Check doorbell count */
            while (0 != cvmx_cmd_queue_length(CVMX_CMD_QUEUE_PKO(queue)) && timeout-- > 0)
                udelay(10000);
            if ((count = cvmx_cmd_queue_length(CVMX_CMD_QUEUE_PKO(queue))))
            {
                printf("Error draining PKO: queue %d doorbell count: %d is not zero\n", queue, count);
                retval = -1;
            }

        }
    }

    /* Drain POW, free buffers to FPA in case they are needed for loopback workaround. */
    cvmx_wqe_t *work;
    while ((work = cvmx_pow_work_request_sync(1)))
    {
        cvmx_helper_free_packet_data(work);
        cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
        dprintf("Freeing packet in network shutdown!\n");
    }

    /* In Octeon CN38XX, the IPD reset did not reset a fifo ptr,
    ** so this is set to zero by using an RGMII loopback.  If neither interface
    ** is RGMII, then this workaround won't work.
    ** All other chips don't need this step - the reset is sufficient.
    */
    if (octeon_is_model(OCTEON_CN38XX_PASS2))
    {
        oct_eth_info = (octeon_eth_info_t *)loopback_dev->priv;

        /* Set loopback_dev port into loopback mode*/
        cvmx_helper_rgmii_internal_loopback(oct_eth_info->port);

        /* Now read IPD_PTR_COUNT register */

        cvmx_ipd_ptr_count_t ipd_cnt;
        ipd_cnt.u64 = cvmx_read_csr(CVMX_IPD_PTR_COUNT);
        int to_add = (ipd_cnt.s.wqev_cnt + ipd_cnt.s.wqe_pcnt) & 0x7;
        int ptr = (-to_add) & 0x7;

        dprintf("Ptr is %d, to add count is: %d\n", ptr, to_add);

        /* Send enough packets to wrap internal ipd pointer to 0 */
        while (to_add--)
        {
            char buffer[100];
            /* Send a packet through loopback to be processed by IPD */
            octeon_eth_send(loopback_dev, buffer, 100);
            udelay(50000);
        }

        udelay(200000);

        /* Check pointer to make sure that our loopback packets moved it to 0 */
        ipd_cnt.u64 = cvmx_read_csr(CVMX_IPD_PTR_COUNT);
        to_add = (ipd_cnt.s.wqev_cnt + ipd_cnt.s.wqe_pcnt) & 0x7;
        ptr = (-to_add) & 0x7;
        dprintf("(After) Ptr is %d, to add count is: %d\n", ptr, to_add);
        if (to_add)
        {
            printf("ERROR: unable to reset packet input!\n");
            return -1;
        }

        /* Disable loopback and leave port disabled. */
        cvmx_write_csr(CVMX_ASXX_RX_PRT_EN(oct_eth_info->interface), 0);
        cvmx_write_csr(CVMX_ASXX_TX_PRT_EN(oct_eth_info->interface), 0);
        cvmx_write_csr(CVMX_ASXX_PRT_LOOP(oct_eth_info->interface), 0);
        oct_eth_info->loopback = 0;
        cvmx_read_csr(CVMX_MIO_BOOT_BIST_STAT);  /* Make sure writes are complete */
        /* End workaround */
    }



    /* Drain POW  */
    while ((work = cvmx_pow_work_request_sync(1)))
    {
        cvmx_helper_free_packet_data(work);
        cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
        dprintf("Freeing packet in network shutdown!\n");
    }


    /* Drain FPA - this may not be necessary */
    for (i = 0; i < 8; i++)
    {
        count = 0;
        while (cvmx_fpa_alloc(i))
        {
	     count++;
      	     if (count > 1024)
		break; 
	} 
    }

    /* Since we are ingoring flow control, we assume that if PKO is empty
    ** the RGMII/SPI buffers are empty by now.  Since all we did was DHCP/TFTP,
    ** we should not have any outstanding packets at this point.
    ** In other situations, the GMX/SPX blocks would need to be checked to make sure they are
    ** empty before resetting.
    */
    /* Delay to let any data in GMX/SPX drain */
    udelay(50000);

    /* Reset the FPA, IPD, PKO.  These bits are new
    ** in pass 2 silicon. */
    cvmx_ipd_ctl_status_t ipd_ctl_status;
    ipd_ctl_status.u64 = cvmx_read_csr(CVMX_IPD_CTL_STATUS);
    ipd_ctl_status.s.reset = 1;
    cvmx_write_csr(CVMX_IPD_CTL_STATUS, ipd_ctl_status.u64);

    if (!octeon_is_model(OCTEON_CN38XX_PASS2))
    {
        cvmx_pip_sft_rst_t pip_reset;
        pip_reset.u64 = cvmx_read_csr(CVMX_PIP_SFT_RST);
        pip_reset.s.rst = 1;
        cvmx_write_csr(CVMX_PIP_SFT_RST, pip_reset.u64);
    }

    cvmx_pko_reg_flags_t pko_reg_flags;
    pko_reg_flags.u64 = cvmx_read_csr(CVMX_PKO_REG_FLAGS);
    pko_reg_flags.s.reset = 1;
    cvmx_write_csr(CVMX_PKO_REG_FLAGS, pko_reg_flags.u64);

    cvmx_fpa_ctl_status_t fpa_ctl_status;
    fpa_ctl_status.u64 = cvmx_read_csr(CVMX_FPA_CTL_STATUS);
    fpa_ctl_status.s.reset = 1;
    cvmx_write_csr(CVMX_FPA_CTL_STATUS, fpa_ctl_status.u64);

    return(retval);
}

int octeon_network_hw_shutdown(void)
{
    int retval = octeon_network_hw_shutdown_internal();

    /* Free command queue named block, as we have reset the hardware */
    cvmx_bootmem_phy_named_block_free("cvmx_cmd_queues", 0);
    if (retval < 0)
    {
        /* Make this a fatal error since the error message is easily missed and ignoring
        ** it can lead to very strange networking behavior in the application.
        */
        printf("FATAL ERROR: Network shutdown failed.  Please reset the board.\n");
        while (1)
            ;
    }
    return(retval);
}

static void
octeon_network_hw_shutdown1 (void)
{
    cvmx_fpa_ctl_status_t status; 
    cvmx_ipd_ctl_status_t ipd_ctl_status;
    int i; 
    int num_interfaces;

    num_interfaces = cvmx_helper_get_number_of_interfaces();
    /* Disable any further packet reception */
    for (i = 0; i < num_interfaces; i++)
    {
        if (CVMX_HELPER_INTERFACE_MODE_RGMII == cvmx_helper_interface_get_mode(i)
            || CVMX_HELPER_INTERFACE_MODE_GMII == cvmx_helper_interface_get_mode(i)
            || CVMX_HELPER_INTERFACE_MODE_SGMII == cvmx_helper_interface_get_mode(i)
            || CVMX_HELPER_INTERFACE_MODE_XAUI == cvmx_helper_interface_get_mode(i))
        {
            cvmx_write_csr(CVMX_ASXX_RX_PRT_EN(i), 0x0);   /* Disable the RGMII RX ports */
            cvmx_write_csr(CVMX_ASXX_TX_PRT_EN(i), 0x0);   /* Disable the RGMII TX ports */
            int num_ports = cvmx_helper_ports_on_interface(i);
            int j;
            for (j = 0; j < num_ports;j++)
            {
                /* Disable MAC filtering */
                cvmx_gmxx_prtx_cfg_t gmx_cfg;
                gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(j, i));
                cvmx_write_csr(CVMX_GMXX_PRTX_CFG(j, i), gmx_cfg.u64 & ~1ull);
                cvmx_write_csr(CVMX_GMXX_RXX_ADR_CTL(j, i), 1);
                cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM_EN(j, i), 0);
                cvmx_write_csr(CVMX_GMXX_PRTX_CFG(j, i), gmx_cfg.u64);
            }
        }
    }
    ipd_ctl_status.u64 = cvmx_read_csr(CVMX_IPD_CTL_STATUS);
    ipd_ctl_status.s.reset = 1;
    cvmx_write_csr(CVMX_IPD_CTL_STATUS, ipd_ctl_status.u64);

    if (!octeon_is_model(OCTEON_CN38XX_PASS2))
    {
        cvmx_pip_sft_rst_t pip_reset;
        pip_reset.u64 = cvmx_read_csr(CVMX_PIP_SFT_RST);
        pip_reset.s.rst = 1;
        cvmx_write_csr(CVMX_PIP_SFT_RST, pip_reset.u64);
    }

    cvmx_pko_reg_flags_t pko_reg_flags;
    pko_reg_flags.u64 = cvmx_read_csr(CVMX_PKO_REG_FLAGS);
    pko_reg_flags.s.reset = 1;
    cvmx_write_csr(CVMX_PKO_REG_FLAGS, pko_reg_flags.u64);

    cvmx_fpa_ctl_status_t fpa_ctl_status;
    fpa_ctl_status.u64 = cvmx_read_csr(CVMX_FPA_CTL_STATUS);
    fpa_ctl_status.s.reset = 1;
    cvmx_write_csr(CVMX_FPA_CTL_STATUS, fpa_ctl_status.u64);


}
/*******************  Octeon networking functions ********************/
int octeon_ebt3000_rgmii_init(struct eth_device *dev, bd_t * bis)         /* Initialize the device	*/
{
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;
    int interface = priv->interface;
#ifdef CONFIG_JSRXNLE
    DECLARE_GLOBAL_DATA_PTR;
    char *s = NULL;
    char *oct_net_init = NULL;
#endif

    dprintf("octeon_ebt3000_rgmii_init(), dev_ptr: %p, port: %d, queue: %d\n", dev, priv->port, priv->queue);
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX240_MODELS
        s = getenv(UB_ENV_LOADER_INIT);
        if (s && !strcmp(s, UB_ETH_INIT)) {
            loader_init = SRX240_LOADER_INIT_PROGRESS;
            /* 
             * If network already initialized earlier 
             * then skip mdk initialization here
             */
            oct_net_init = getenv("oct.net.init");
            if (oct_net_init && !strcmp(oct_net_init, "1")) {
                loader_init = SRX240_LOADER_INIT_COMPLETE;
                rx_sync_flag = 1;
            } else {
                (void)mdk_init();
                udelay(100000); /* 100 ms */
            }
        }
        break;
    default:
        break;
    }
#endif
    if (priv->initted_flag) {
#ifdef CONFIG_JSRXNLE
        /* Update the link to take care of any link changes */
        do_link_update();
#endif
        return 1;
    }

    if (!octeon_global_hw_inited)
    {
        cvm_oct_configure_common_hw();
    }

    /* Ignore backpressure on RGMII ports */
#if defined(CONFIG_ACX500_SVCS)
    cvmx_write_csr(priv->gmx_base + GMX_TX_OVR_BP, 0xf << 8 | 0xf);
#else
    cvmx_write_csr(CVMX_GMXX_TX_OVR_BP(interface), 0xf << 8 | 0xf);
#endif

    cvm_oct_set_mac_address(interface, interface & 0x3, dev->enetaddr);
    if (!octeon_global_hw_inited)
    {
        cvmx_helper_ipd_and_packet_input_enable();
        octeon_global_hw_inited = 1;
    }
    priv->initted_flag = 1;
#ifdef CONFIG_JSRXNLE
    /* Update the link to take care of any link changes */
    do_link_update();
#endif
    return(1);
}




#ifdef OCTEON_SPI4000_ENET
int octeon_spi4000_init(struct eth_device *dev, bd_t * bis)         /* Initialize the device	*/
{
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;
    static char spi4000_inited[2] = {0};
    dprintf("octeon_spi4000_init(), dev_ptr: %p, port: %d, queue: %d\n", dev, priv->port, priv->queue);


    if (priv->initted_flag)
        return 1;

    if (!octeon_global_hw_inited)
    {
        cvm_oct_configure_common_hw();
    }

    if (!spi4000_inited[priv->interface])
    {
        spi4000_inited[priv->interface] = 1;
        if (__cvmx_helper_spi_enable(priv->interface) < 0)
        {
            printf("ERROR initializing spi4000 on Octeon Interface %d\n", priv->interface);
            return 0;
        }
    }

    if (!octeon_global_hw_inited)
    {
        cvmx_helper_ipd_and_packet_input_enable();
        octeon_global_hw_inited = 1;
    }
    priv->initted_flag = 1;
    return(1);
}
#endif

#if defined(CONFIG_ACX500_SVCS)
/**
 * Enables a SGMII interface
 *
 * @param dev - Ethernet device to initialize
 */
static void octeon_eth_sgmii_enable(struct eth_device *dev)
{
    octeon_eth_info_t *oct_eth_info;
    cvmx_gmxx_prtx_cfg_t gmx_cfg;
    int index, interface;
    cvmx_helper_interface_mode_t if_mode;

    oct_eth_info = (octeon_eth_info_t *) dev->priv;

    interface = oct_eth_info->interface;

    index = oct_eth_info->index;

    if_mode = cvmx_helper_interface_get_mode(interface);
    /* Normal operating mode. */

    if (if_mode == CVMX_HELPER_INTERFACE_MODE_SGMII ||
            if_mode == CVMX_HELPER_INTERFACE_MODE_QSGMII) {
        cvmx_pcsx_miscx_ctl_reg_t pcsx_miscx_ctl_reg;
        pcsx_miscx_ctl_reg.u64 =
            cvmx_read_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface));
        pcsx_miscx_ctl_reg.s.gmxeno = 0;
        cvmx_write_csr(CVMX_PCSX_MISCX_CTL_REG(index, interface),
                pcsx_miscx_ctl_reg.u64);
    }

    gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, oct_eth_info->interface));
    gmx_cfg.s.en = 1;
    cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, oct_eth_info->interface), gmx_cfg.u64);
    gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, oct_eth_info->interface));
}
#endif /* CONFIG_ACX500_SVCS */

/**
 * Sets the hardware MAC address of the Ethernet device
 *
 * @param dev - Ethernet device
 *
 * @return 0 for success
 */
int octeon_eth_write_hwaddr(struct eth_device *dev)
{
	octeon_eth_info_t *oct_eth_info;
	oct_eth_info = (octeon_eth_info_t *)dev->priv;

	/* Skip if the interface isn't yet enabled */
	if (!oct_eth_info->enabled) {
		dprintf("%s: Interface not enabled, not setting MAC address\n",
		      __func__);
		return 0;
	}

	dprintf("%s: Setting %s address to %02x:%02x:%02x:%02x:%02x:%02x\n",
            __func__, dev->name,
	      dev->enetaddr[0], dev->enetaddr[1], dev->enetaddr[2],
	      dev->enetaddr[3], dev->enetaddr[4], dev->enetaddr[5]);

	return cvm_oct_set_mac_address(oct_eth_info->interface,
				       oct_eth_info->index,
				       dev->enetaddr);
}




void octeon_eth_rgmii_enable(struct eth_device *dev)
{
    octeon_eth_info_t *oct_eth_info;

    oct_eth_info = (octeon_eth_info_t *)dev->priv;

    if (CVMX_HELPER_INTERFACE_MODE_RGMII == cvmx_helper_interface_get_mode(oct_eth_info->interface)
        || CVMX_HELPER_INTERFACE_MODE_GMII == cvmx_helper_interface_get_mode(oct_eth_info->interface))
    {
        uint64_t tmp;
        tmp = cvmx_read_csr(CVMX_ASXX_RX_PRT_EN(oct_eth_info->interface));
        tmp |= (1ull << (oct_eth_info->port & 0x3));
        cvmx_write_csr(CVMX_ASXX_RX_PRT_EN(oct_eth_info->interface), tmp);
        tmp = cvmx_read_csr(CVMX_ASXX_TX_PRT_EN(oct_eth_info->interface));
        tmp |= (1ull << (oct_eth_info->port & 0x3));
        cvmx_write_csr(CVMX_ASXX_TX_PRT_EN(oct_eth_info->interface), tmp);
    }


#if defined(CONFIG_ACX500_SVCS)
    int interface;
    cvmx_helper_interface_mode_t if_mode;

    interface = oct_eth_info->interface;
    if_mode = cvmx_helper_interface_get_mode(interface);

    switch (if_mode) {
    case CVMX_HELPER_INTERFACE_MODE_SGMII:
    case CVMX_HELPER_INTERFACE_MODE_XAUI:
    case CVMX_HELPER_INTERFACE_MODE_RXAUI:
    case CVMX_HELPER_INTERFACE_MODE_AGL:
        octeon_eth_sgmii_enable(dev);
        oct_eth_info->enabled = 1;
        octeon_eth_write_hwaddr(dev);
        break;

    default:
        return;
    }
#endif

}




#define DSA_TAG_LENGTH 8
/* Keep track of packets sent on each interface so we can
** autonegotiate before we send the first one */
int packets_sent;  
int octeon_eth_send(struct eth_device *dev, volatile void *packet, int length)	   /* Send a packet	*/
{
#ifdef CONFIG_JSRXNLE
    uchar dsa_tagged_pkt[PKTSIZE_ALIGN + DSA_TAG_LENGTH + PKTALIGN] ;
    ext_dsa_tag dsa_tag;
    DECLARE_GLOBAL_DATA_PTR;
    static int rx_sync_count = RX_SYNC_COUNT; /* Good enough value */
    uint64_t val;
    uint64_t addr;
#endif

    dprintf("ethernet TX! ptr: %p, len: %d\n", packet, length);
    /* We need to copy this to a FPA buffer, then give that to TX */

    /* This is needed since we disable rx/tx in halt */
    octeon_eth_rgmii_enable(dev);
    if (packets_sent++ == 0)
        cvm_oct_configure_rgmii_speed(dev);

    /* HACK to get away with SRX240 Rx Sync issue. When loader wants
     * to exchange some traffic on network it uses uboot function to
     * initialize octeon network and send packets on network. During
     * its few initial packets transfer if we update the link connecting
     * octeon and BCM then Rx Sync issue doesn't happen
     */
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
      CASE_ALL_SRX240_MODELS
	{
            if ((loader_init == SRX240_LOADER_NOT_STARTED) || rx_sync_flag)
                break;

            if (rx_sync_count) {
                do_link_update();
                udelay(50000); /* 50ms */
    	        --rx_sync_count;
            } else {
                addr = CVMX_PCSX_RXX_SYNC_REG(0, 0);
                val = cvmx_read_csr(addr);
                /* If after few attempts Rx is not in sync, then we cannot
                 * recover from here. So better to reboot the box
                 */
                if (!(val & 0x2)) {
                    printf("Octeon Rx Synchronization failed... Reboot the box\n");
                }
    	        rx_sync_flag = 1;
                loader_init = SRX240_LOADER_INIT_COMPLETE;
    	    }
        }
	break;
      default:
        break;
    }
#endif
    void *fpa_buf = cvmx_fpa_alloc(CVMX_FPA_PACKET_POOL);
    if (!fpa_buf)
    {
#ifndef CONFIG_JSRXNLE
        printf("ERROR allocating buffer for packet!\n");
#endif
    }

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
      CASE_ALL_SRX210_MODELS
        dsa_tag.val = 0;

        dsa_tag.dsa_tag.extend   = 1; /* Using extended DSA tag - 8 bytes */
        dsa_tag.dsa_tag.tag_cmd  = 1; /* FROM_CPU command */
        dsa_tag.dsa_tag.trg_port = DXJ106_UBOOT_PORT; /* tftpboot port */

        memcpy(dsa_tagged_pkt, (void *)packet, MRVL_ADJ_BYTES);
        memcpy((dsa_tagged_pkt + MRVL_ADJ_BYTES),
                (void *)&dsa_tag, sizeof(ext_dsa_tag));
        memcpy((dsa_tagged_pkt + MRVL_ADJ_BYTES + sizeof(ext_dsa_tag)),
               (void *)packet + MRVL_ADJ_BYTES, length - MRVL_ADJ_BYTES);
        length += DSA_TAG_LENGTH;//sizeof(ext_dsa_tag);

        memcpy(fpa_buf, (void *)dsa_tagged_pkt, length);
        break;
      CASE_ALL_SRX220_MODELS
        dsa_tag.val = 0;

        dsa_tag.dsa_tag.extend   = 1; /* Using extended DSA tag - 8 bytes */
        dsa_tag.dsa_tag.tag_cmd  = 1; /* FROM_CPU command */
        dsa_tag.dsa_tag.trg_port = DXJ106_SRX220_UBOOT_PORT; /* tftpboot port */

        memcpy(dsa_tagged_pkt, (void *)packet, MRVL_ADJ_BYTES);
        memcpy((dsa_tagged_pkt + MRVL_ADJ_BYTES),
                (void *)&dsa_tag, sizeof(ext_dsa_tag));
        memcpy((dsa_tagged_pkt + MRVL_ADJ_BYTES + sizeof(ext_dsa_tag)),
               (void *)packet + MRVL_ADJ_BYTES, length - MRVL_ADJ_BYTES);
        length += DSA_TAG_LENGTH;//sizeof(ext_dsa_tag);

        memcpy(fpa_buf, (void *)dsa_tagged_pkt, length);
        break;

      default:
        memcpy(fpa_buf, (void *)packet, length);
        break;
    }
#else
    memcpy(fpa_buf, (void *)packet, length);
#endif
#if defined(DEBUG) && 0
    /* Dump out packet contents */
    {
        printf("xmit: \n");
	int i, j;
	unsigned char *up = fpa_buf;

	for (i = 0; (i + 16) < length; i += 16)
	{
	    printf("%04x ", i);
	    for (j = 0; j < 16; ++j)
	    {
		printf("%02x ", up[i+j]);
	    }
	    printf("    ");
	    for (j = 0; j < 16; ++j)
	    {
		printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	    }
	    printf("\n");
	}
	printf("%04x ", i);
	for (j = 0; i+j < length; ++j)
	{
	    printf("%02x ", up[i+j]);
	}
	for (; j < 16; ++j)
	{
	    printf("   ");
	}
	printf("    ");
	for (j = 0; i+j < length; ++j)
	{
	    printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	}
	printf("\n");
    }
#endif
    cvm_oct_xmit(dev, fpa_buf, length);
    return(0);
}

int octeon_eth_recv(struct eth_device *dev)         /* Check for received packets	*/
{
#ifdef CONFIG_JSRXNLE
    uchar stripped_pkt[PKTSIZE_ALIGN + PKTALIGN];
    DECLARE_GLOBAL_DATA_PTR;
#endif

    cvmx_wqe_t *work = cvmx_pow_work_request_sync(1);
    if (!work)
    {
        /* Poll for link status change, only if work times out.
        ** On some interfaces this is really slow.
        ** getwork timeout is set to maximum. */
        cvm_oct_configure_rgmii_speed(dev);
        return(0);
    }
    
    if (work->word2.s.rcv_error)
    {
#if defined(DEBUG) && 0
    /* Dump out packet contents */
    {
    void *packet_data = cvmx_phys_to_ptr(work->packet_ptr.u64 & 0xffffffffffull);
    int length = work->len;
	int i, j;
	unsigned char *up = packet_data;

        printf("rcv: \n");
	for (i = 0; (i + 16) < length; i += 16)
	{
	    printf("%04x ", i);
	    for (j = 0; j < 16; ++j)
	    {
		printf("%02x ", up[i+j]);
	    }
	    printf("    ");
	    for (j = 0; j < 16; ++j)
	    {
		printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	    }
	    printf("\n");
	}
	printf("%04x ", i);
	for (j = 0; i+j < length; ++j)
	{
	    printf("%02x ", up[i+j]);
	}
	for (; j < 16; ++j)
	{
	    printf("   ");
	}
	printf("    ");
	for (j = 0; i+j < length; ++j)
	{
	    printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	}
	printf("\n");
    }
#endif
        /* Work has error, so drop */
#ifndef CONFIG_JSRXNLE
        printf("Error packet received, dropping\n");
#endif
        cvmx_helper_free_packet_data(work);
        cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
        return(0);
    }


    void *packet_data = cvmx_phys_to_ptr(work->packet_ptr.u64 & 0xffffffffffull);
    int length = work->len;

    dprintf("############# got work: %p, len: %d, packet_ptr: %p\n",
            work, length, packet_data);
#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type)
    {
      CASE_ALL_SRX210_MODELS
      CASE_ALL_SRX220_MODELS
        memcpy(stripped_pkt, (uchar *)packet_data, 12);
        memcpy((stripped_pkt + 12), (uchar *)(packet_data + 12 + sizeof(ext_dsa_tag)),
              (length - sizeof(ext_dsa_tag)));
        length -= sizeof(ext_dsa_tag);
#if defined(DEBUG) && 0
    /* Dump out packet contents */
    {
	int i, j;
#ifdef JSRXNLE_PKT_DUMP
	unsigned char *up = stripped_pkt;
#else
	unsigned char *up = packet_data;
#endif

	for (i = 0; (i + 16) < length; i += 16)
	{
	    printf("%04x ", i);
	    for (j = 0; j < 16; ++j)
	    {
		printf("%02x ", up[i+j]);
	    }
	    printf("    ");
	    for (j = 0; j < 16; ++j)
	    {
		printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	    }
	    printf("\n");
	}
	printf("%04x ", i);
	for (j = 0; i+j < length; ++j)
	{
	    printf("%02x ", up[i+j]);
	}
	for (; j < 16; ++j)
	{
	    printf("   ");
	}
	printf("    ");
	for (j = 0; i+j < length; ++j)
	{
	    printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	}
	printf("\n");
    }
#endif
        NetReceive (stripped_pkt, length);
        break;

      default:
        NetReceive (packet_data, length);
        break;
    }
#else
    NetReceive (packet_data, length);
#endif
    cvmx_helper_free_packet_data(work);
    cvmx_fpa_free(work, CVMX_FPA_WQE_POOL, 0);
    /* Free WQE and packet data */
    return(length);
}
void octeon_eth_halt(struct eth_device *dev)			/* stop SCC			*/
{
    /* Disable reception on this port at the GMX block */
    octeon_eth_info_t *oct_eth_info;

    oct_eth_info = (octeon_eth_info_t *)dev->priv;

    if (CVMX_HELPER_INTERFACE_MODE_RGMII == cvmx_helper_interface_get_mode(oct_eth_info->interface)
        || CVMX_HELPER_INTERFACE_MODE_GMII == cvmx_helper_interface_get_mode(oct_eth_info->interface))
    {
        int index = oct_eth_info->interface & 0x3;
        uint64_t tmp;
        cvmx_gmxx_prtx_cfg_t gmx_cfg;
        tmp = cvmx_read_csr(CVMX_ASXX_RX_PRT_EN(oct_eth_info->interface));
        tmp &= ~(1ull << (oct_eth_info->port & 0x3));
        cvmx_write_csr(CVMX_ASXX_RX_PRT_EN(oct_eth_info->interface), tmp);   /* Disable the RGMII RX ports */
        tmp = cvmx_read_csr(CVMX_ASXX_TX_PRT_EN(oct_eth_info->interface));
        tmp &= ~(1ull << (oct_eth_info->port & 0x3));
        cvmx_write_csr(CVMX_ASXX_TX_PRT_EN(oct_eth_info->interface), tmp);   /* Disable the RGMII TX ports */

        /* Disable MAC filtering */
        gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, oct_eth_info->interface));
        cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, oct_eth_info->interface), gmx_cfg.u64 & ~1ull);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CTL(index, oct_eth_info->interface), 1);
        cvmx_write_csr(CVMX_GMXX_RXX_ADR_CAM_EN(index, oct_eth_info->interface), 0);
        cvmx_write_csr(CVMX_GMXX_PRTX_CFG(index, oct_eth_info->interface), gmx_cfg.u64);
    }

}

/*********************************************************
Only handle built-in RGMII/SGMII/XAUI/GMII ports here
*********************************************/
int octeon_eth_initialize (bd_t * bis)
{
    DECLARE_GLOBAL_DATA_PTR;
    struct eth_device *dev;
    octeon_eth_info_t *oct_eth_info;
    int port;
    int interface;
    uint32_t *mac_inc_ptr = (uint32_t *)(&mac_addr[2]);
    int num_ports;
    int num_ints;

    dprintf("octeon_mgmt_eth_initialize called\n");
#ifdef CONFIG_RAM_RESIDENT
    octeon_network_hw_shutdown1();
#endif
    if (available_mac_count == -1)
    {
        available_mac_count = gd->mac_desc.count;
        memcpy(mac_addr, (uint8_t *)(gd->mac_desc.mac_addr_base), 6);
    }
    if (getenv("disable_networking"))
    {
        printf("Networking disabled with 'disable_networking' environment variable, skipping RGMII interface\n");
        return 0;
    }
    if (available_mac_count <= 0)
    {
        printf("No available MAC addresses for RGMII interface, skipping\n");
        return 0;
    }
    /* Initialize port state array to invalid values */
    memset(port_state, 0xff, sizeof(port_state));

    if (octeon_is_model(OCTEON_CN38XX_PASS1))
    {
        printf("\n");
        printf("\n");
        printf("WARNING:\n");
        printf("WARNING:OCTEON pass 1 does not support network hardware reset.\n");
        printf("WARNING:Bootloader networking use will cause unstable operation of any other\n");
        printf("WARNING:program or OS using the network hardware.\n");
        printf("WARNING:\n");
        printf("\n");
        printf("\n");
    }

    /* Do board specific init in early_board_init or checkboard if possible. */

    /* NOTE: on 31XX based boards, the GMXX_INF_MODE register must be set appropriately
    ** before this code is run (in checkboard for instance).  The hardware is configured based
    ** on the settings of GMXX_INF_MODE. */

    /* We only care about the first two (real packet interfaces)
    ** This will ignore PCI, loop, etc interfaces. */
    num_ints = cvmx_helper_get_number_of_interfaces();
#if !defined(CONFIG_ACX500_SVCS)
    if (num_ints > 2)
        num_ints = 2;
#endif

    /* Check to see what interface and ports we should use */
    for (interface=cvmx_helper_get_start_interface(); 
         interface < num_ints; interface++)
    {
        cvmx_gmxx_inf_mode_t mode;
        mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(interface));

        if (CVMX_HELPER_INTERFACE_MODE_RGMII == cvmx_helper_interface_get_mode(interface)
            || CVMX_HELPER_INTERFACE_MODE_GMII == cvmx_helper_interface_get_mode(interface)
            || CVMX_HELPER_INTERFACE_MODE_SGMII == cvmx_helper_interface_get_mode(interface)
            || CVMX_HELPER_INTERFACE_MODE_QSGMII == cvmx_helper_interface_get_mode(interface)
            || CVMX_HELPER_INTERFACE_MODE_AGL == cvmx_helper_interface_get_mode(interface)
            || CVMX_HELPER_INTERFACE_MODE_XAUI == cvmx_helper_interface_get_mode(interface))
        {
            cvmx_helper_interface_probe(interface);
            num_ports = cvmx_helper_ports_on_interface(interface);
            /* RGMII or SGMII */
            int index = 0;
            int pknd = cvmx_helper_get_ipd_port(interface, 0);
            int incr = 1;
            cvmx_helper_interface_mode_t if_mode;
            num_ports = pknd + (num_ports * incr);
            if_mode = cvmx_helper_interface_get_mode(interface);
            for (index = 0, port = pknd; (port < num_ports); port += incr,
                    index++)
            {
                dev = (struct eth_device *) malloc(sizeof(*dev));

#if !defined(CONFIG_ACX500_SVCS)
                /* Save pointer for port 16 or 0 to use in IPD reset workaround */
                if (port == 16 || port == 0)
                    loopback_dev = dev;
#endif

                oct_eth_info = (octeon_eth_info_t *) malloc(sizeof(octeon_eth_info_t));

                oct_eth_info->port = port;
                oct_eth_info->queue = cvmx_pko_get_base_queue(oct_eth_info->port);
                oct_eth_info->interface = interface;
                oct_eth_info->initted_flag = 0;
                oct_eth_info->index = index;
                oct_eth_info->ethdev = dev;

                dprintf("Setting up port: %d, queue: %d, int: %d\n",
                        oct_eth_info->port,
                        oct_eth_info->queue, oct_eth_info->interface);

                if (if_mode == CVMX_HELPER_INTERFACE_MODE_AGL) {
                    oct_eth_info->gmx_base = CVMX_AGL_GMX_RXX_INT_REG(0);
                    sprintf(dev->name, "octrgmii%d", agl_number++);
                } else {
                    oct_eth_info->gmx_base = CVMX_GMXX_RXX_INT_REG(index, interface);
                    sprintf (dev->name, "octeth%d", card_number);
                }
                card_number++;

                dev->priv = (void *)oct_eth_info;
                dev->iobase = 0;
                dev->init = octeon_ebt3000_rgmii_init;
                dev->halt = octeon_eth_halt;
                dev->send = octeon_eth_send;
                dev->recv = octeon_eth_recv;
                memcpy(dev->enetaddr, mac_addr, 6);
                (*mac_inc_ptr)++;  /* increment MAC address */

                eth_register (dev);

#if defined(CONFIG_ACX500_SVCS)
                /* Initialize the SGMII links here as well */
                octeon_eth_rgmii_enable(dev);
                cvm_oct_configure_rgmii_speed(dev);
#endif
            }
        }
    }

#ifdef CONFIG_JSRXNLE
    switch (gd->board_desc.board_type) {
    CASE_ALL_SRX210_MODELS
    CASE_ALL_SRX220_MODELS	
        if ((gd->flags & GD_FLG_BOARD_EEPROM_READ_FAILED) == 0) 
	    dxj106_init();
        break;
    CASE_ALL_SRX100_MODELS
    CASE_ALL_SRX110_MODELS
        if ((gd->flags & GD_FLG_BOARD_EEPROM_READ_FAILED) == 0) 
	    ls609x_uboot_drv_init();
        break;
    default:
        break;
    }
#endif
#ifdef CONFIG_MAG8600
    setenv("ethact","octeth1");
#endif

    return card_number;
}



#ifdef OCTEON_SPI4000_ENET
int octeon_spi4000_initialize(bd_t * bis)
{
    DECLARE_GLOBAL_DATA_PTR;
    struct eth_device *dev;
    octeon_eth_info_t *oct_eth_info;
    int port;
    int interface = 0;
    uint32_t *mac_inc_ptr = (uint32_t *)(&mac_addr[2]);
    cvmx_spi_callbacks_t spi_callbacks_struct;

    if (available_mac_count == -1)
    {
        available_mac_count = gd->mac_desc.count;
        memcpy(mac_addr, (void *)(gd->mac_desc.mac_addr_base), 6);
    }
    if (getenv("disable_networking"))
    {
        printf("Networking disabled with 'disable_networking' environment variable, skipping SPI interface\n");
        return 0;
    }

    /* Re-setup callbacks here, as the initialization done at compile time points
    ** to the link addresses in flash.
    */
    cvmx_spi_get_callbacks(&spi_callbacks_struct);
    spi_callbacks_struct.reset_cb            = cvmx_spi_reset_cb;
    spi_callbacks_struct.calendar_setup_cb   = cvmx_spi_calendar_setup_cb;
    spi_callbacks_struct.clock_detect_cb     = cvmx_spi_clock_detect_cb;
    spi_callbacks_struct.training_cb         = cvmx_spi_training_cb;
    spi_callbacks_struct.calendar_sync_cb    = cvmx_spi_calendar_sync_cb;
    spi_callbacks_struct.interface_up_cb     = cvmx_spi_interface_up_cb;
    cvmx_spi_set_callbacks(&spi_callbacks_struct);

    if (!cvmx_spi4000_is_present(interface))
        return 0;
    if (available_mac_count <= 0)
    {
        printf("No available MAC addresses for SPI interface, skipping\n");
        return 0;
    }

    for (port = 0; port < 10 && available_mac_count-- > 0; port++)
    {
        dev = (struct eth_device *) malloc(sizeof(*dev));
        oct_eth_info = (octeon_eth_info_t *) malloc(sizeof(octeon_eth_info_t));

        oct_eth_info->port = port;
        oct_eth_info->queue = cvmx_pko_get_base_queue(oct_eth_info->port);
        oct_eth_info->interface = interface;
        oct_eth_info->initted_flag = 0;

        dprintf("Setting up SPI4000 port: %d, queue: %d, int: %d\n", oct_eth_info->port, oct_eth_info->queue, oct_eth_info->interface);
        sprintf (dev->name, "octspi%d", port);
        card_number++;

        dev->priv = (void *)oct_eth_info;
        dev->iobase = 0;
        dev->init = octeon_spi4000_init;
        dev->halt = octeon_eth_halt;
        dev->send = octeon_eth_send;
        dev->recv = octeon_eth_recv;
        memcpy(dev->enetaddr, mac_addr, 6);
        (*mac_inc_ptr)++;  /* increment MAC address */

        eth_register (dev);
    }
    return card_number;
}
#endif  /* OCTEON_SPI4000_ENET */


#endif   /* OCTEON_INTERNAL_ENET */


#if defined(CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)

/**
 * Perform an MII read. Called by the generic MII routines
 *
 * @param dev      Device to perform read for
 * @param phy_id   The MII phy id
 * @param location Register location to read
 * @return Result from the read or zero on failure
 */
int  oct_miiphy_read(unsigned char addr, unsigned char reg, unsigned short * value)
{

    int bus_id ;
    int retval;

    /* to support clause 45 */
    bus_id = (addr & 0x80) >> 7;
    retval  = cvmx_mdio_read(bus_id, addr, reg);
    if (retval < 0)
        return -1;
    *value = (unsigned short)retval;
    return 0;
}

/* Wrapper to make non-error checking code simpler */
int miiphy_read_wrapper(unsigned char  addr, unsigned char  reg)
{
    unsigned short value;

    if (oct_miiphy_read(addr, reg, &value) < 0)
    {
        printf("ERROR: miiphy_read_wrapper(0x%x, 0x%x) timed out.\n", addr, reg);
        return (-1);
    }

    return(value);
}



int oct_miiphy_write(unsigned char  addr,
		 unsigned char  reg,
		 unsigned short value)
{
    int bus_id = 0;
    
    /* to support clause 45 */
    bus_id = (addr & 0x80) >> 7;
    if (cvmx_mdio_write(bus_id, addr, reg, value) < 0)
        return(-1);
    else
        return(0);
}
#endif /* MII */



#if defined(OCTEON_MGMT_ENET) && (CONFIG_COMMANDS & CFG_CMD_NET)


int octeon_mgmt_eth_init(struct eth_device *dev, bd_t * bis)         /* Initialize the device	*/
{
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;
    uint64_t mac = 0x0;
    dprintf("octeon_mgmt_eth_init(), dev_ptr: %p\n", dev);

    if (priv->initted_flag)
        return 1;

    cvmx_mgmt_port_initialize(priv->port);
    memcpy(((char *)&mac) + 2, dev->enetaddr,6);
    cvmx_mgmt_port_set_mac(priv->port, mac);

    priv->enabled = 0;


    priv->initted_flag = 1;
    return(1);
}
int octeon_mgmt_eth_send(struct eth_device *dev, volatile void *packet, int length)	   /* Send a packet	*/
{
    int retval;
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;

    if (!priv->enabled)
    {
       priv->enabled = 1;
       cvmx_mgmt_port_enable(priv->port);
    }
    retval = cvmx_mgmt_port_send(priv->port, length, (void *)packet);
    return retval;

}
#define MGMT_BUF_SIZE   1700
int octeon_mgmt_eth_recv(struct eth_device *dev)         /* Check for received packets	*/
{
    uchar buffer[MGMT_BUF_SIZE];
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;


    if (!priv->enabled)
    {
       priv->enabled = 1;
       cvmx_mgmt_port_enable(priv->port);
    }
    int length = cvmx_mgmt_port_receive(priv->port, MGMT_BUF_SIZE, buffer);

    if (length > 0)
    {
#if defined(DEBUG) && 0
    /* Dump out packet contents */
    {
	int i, j;
	unsigned char *up = buffer;

	for (i = 0; (i + 16) < length; i += 16)
	{
	    printf("%04x ", i);
	    for (j = 0; j < 16; ++j)
	    {
		printf("%02x ", up[i+j]);
	    }
	    printf("    ");
	    for (j = 0; j < 16; ++j)
	    {
		printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	    }
	    printf("\n");
	}
	printf("%04x ", i);
	for (j = 0; i+j < length; ++j)
	{
	    printf("%02x ", up[i+j]);
	}
	for (; j < 16; ++j)
	{
	    printf("   ");
	}
	printf("    ");
	for (j = 0; i+j < length; ++j)
	{
	    printf("%c", ((up[i+j] >= ' ') && (up[i+j] <= '~')) ? up[i+j] : '.');
	}
	printf("\n");
    }
#endif
        NetReceive (buffer, length);
    }
    else if (length < 0)
    {
        printf("MGMT port rx error: %d\n", length);
    }
    return(length);
}

void octeon_mgmt_eth_halt(struct eth_device *dev)			/* stop SCC			*/
{
    octeon_eth_info_t* priv = (octeon_eth_info_t*)dev->priv;
    priv->enabled = 0;
    cvmx_mgmt_port_disable(priv->port);
}
int octeon_mgmt_eth_initialize (bd_t * bis)
{
    DECLARE_GLOBAL_DATA_PTR;
    struct eth_device *dev;
    octeon_eth_info_t *oct_eth_info;
    uint32_t *mac_inc_ptr = (uint32_t *)(&mac_addr[2]);
    int mgmt_port_count = 1;
    int cur_port;

    if (OCTEON_IS_MODEL(OCTEON_CN52XX))
        mgmt_port_count = 2;


    dprintf("octeon_mgmt_eth_initialize called\n");
    if (available_mac_count == -1)
    {
        available_mac_count = gd->mac_desc.count;
        memcpy(mac_addr, (uint8_t *)(gd->mac_desc.mac_addr_base), 6);
    }
    if (getenv("disable_networking"))
    {
        printf("Networking disabled with 'disable_networking' environment variable, skipping RGMII interface\n");
        return 0;
    }

    if (available_mac_count <= 0)
    {
        printf("No available MAC addresses for Management interface(s), skipping\n");
        return 0;
    }


    for (cur_port = 0; (cur_port < mgmt_port_count) && available_mac_count-- > 0; cur_port++)
    {
        /* Initialize port state array to invalid values */
        memset(port_state, 0xff, sizeof(port_state));

        dev = (struct eth_device *) malloc(sizeof(*dev));

        oct_eth_info = (octeon_eth_info_t *) malloc(sizeof(octeon_eth_info_t));

        oct_eth_info->port = cur_port;
        /* These don't apply to the management ports */
        oct_eth_info->queue = ~0;
        oct_eth_info->interface = ~0;
        oct_eth_info->initted_flag = 0;

        dprintf("Setting up mgmt eth port\n");
        sprintf (dev->name, "octmgmt%d", cur_port);

        dev->priv = (void *)oct_eth_info;
        dev->iobase = 0;
        dev->init = octeon_mgmt_eth_init;
        dev->halt = octeon_mgmt_eth_halt;
        dev->send = octeon_mgmt_eth_send;
        dev->recv = octeon_mgmt_eth_recv;
        memcpy(dev->enetaddr, mac_addr, 6);

        (*mac_inc_ptr)++;  /* increment MAC address */

        eth_register (dev);
    }

    return card_number;

}
#endif

#define INTERFACE(port) (port >> 4) /* Ports 0-15 are interface 0, 16-31 are interface 1 */
#define INDEX(port) (port & 0xf)

#ifdef CONFIG_JSRXNLE
void 
ethsetup (void)
{
    int rc;
    DECLARE_GLOBAL_DATA_PTR;
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
        CASE_ALL_SRX550_MODELS
            setenv("ethact","octeth0");
            break;
        CASE_ALL_SRX650_MODELS
            break;
        default:
            return;
    }
    rc = mdk_init();
    if (rc < 0) {
        printf("Switch initialization failed\n");
    }
    ethact_init = 1;
#ifdef CONFIG_NET_MULTI
    eth_set_current();
#endif
    return;
}

void
static srx550_hw_get_gmx_rx_status (cvmx_gmx_rx_status_t *stats)
{
    int    localPort, interface;

    localPort = 0;
    interface = 0;

    stats->packets  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_PKTS (localPort, interface));
    stats->controls  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_PKTS_CTL (localPort, interface));
    stats->dmacs  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_PKTS_DMAC (localPort, interface));
    stats->drops  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_PKTS_DRP (localPort, interface));
    stats->errors  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_PKTS_BAD (localPort, interface));

    stats->octets  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_OCTS (localPort, interface));
    stats->controlOctets  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_OCTS_CTL (localPort, interface));
    stats->dmacOctets  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_OCTS_DMAC (localPort, interface));
    stats->dropOctets  =
        cvmx_read_csr (CVMX_GMXX_RXX_STATS_OCTS_DRP (localPort, interface));
}

void
static srx550_hw_get_gmx_tx_status (cvmx_gmx_tx_status_t *stats)
{
    int    localPort, interface;

    localPort  = 0;
    interface =  0;

    stats->dropCollides  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT0 (localPort, interface)) >> 32;
    stats->dropDefers  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT0 (localPort, interface)) &
        0x00000000FFFFFFFFLU;
    stats->singleCollides  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT1 (localPort, interface)) >> 32;
    stats->multiCollides  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT1 (localPort, interface)) &
        0x00000000FFFFFFFFLU;
    stats->packets =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT3 (localPort, interface));
    stats->octets  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT2 (localPort, interface));
    stats->multicasts  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT8 (localPort, interface)) >> 32;
    stats->broadcasts  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT8 (localPort, interface)) &
        0x00000000FFFFFFFFLU;
    stats->underflows  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT9 (localPort, interface)) >> 32;
    stats->controls  =
        cvmx_read_csr (CVMX_GMXX_TXX_STAT9 (localPort, interface)) &
        0x00000000FFFFFFFFLU;
}


int 
static srx550_dump_stats(void)
{

    cvmx_gmx_rx_status_t    rxStats;
    cvmx_gmx_tx_status_t    txStats;

    srx550_hw_get_gmx_rx_status (&rxStats);
    srx550_hw_get_gmx_tx_status (&txStats);

    printf ("  Port%2d Tx:     sent Pkts %u (%lu octets)\n"
            "            multicast Pkts %u\n"
            "            broadcast Pkts %u\n"
            "              control Pkts %u\n"
            "            underflow Pkts %u\n"
            "        collisions: single %u, multiple %u\n"
            "         drops: collisions %u, deferrals %u\n",
            0, txStats.packets, txStats.octets, txStats.multicasts,
            txStats.broadcasts, txStats.controls, txStats.underflows,
            txStats.singleCollides, txStats.multiCollides,
            txStats.dropCollides, txStats.dropDefers);

    printf ("  Port%2d Rx: received Pkts %u (%lu octets)\n"
            "              control Pkts %u (%lu octets)\n"
            "        DMAC filtered Pkts %u (%lu octets)\n"
            "              dropped Pkts %u (%lu octets)\n"
            "                    errors %u\n",
            0, rxStats.packets, rxStats.octets,
            rxStats.controls, rxStats.controlOctets,
            rxStats.dmacs, rxStats.dmacOctets,
            rxStats.drops, rxStats.dropOctets, rxStats.errors);
    return 0;
}

void 
static srx550_dump_xaui_registers(void)
{
        

    int interface = 0;
    cvmx_gmxx_tx_xaui_ctl_t       gmxx_tx_xaui_ctl;
    cvmx_gmxx_rx_xaui_ctl_t       gmxx_rx_xaui_ctl;
    cvmx_pcsxx_control1_reg_t     xauiCtl;
    cvmx_pcsxx_misc_ctl_reg_t     xauiMiscCtl;
    cvmx_pcsxx_status1_reg_t pcsxx_status1_reg;
    cvmx_gmxx_xaui_ext_loopback_t gmxx_xaui_ext_loopback;


    gmxx_tx_xaui_ctl.u64 = cvmx_read_csr(CVMX_GMXX_TX_XAUI_CTL(interface));
    gmxx_rx_xaui_ctl.u64 = cvmx_read_csr(CVMX_GMXX_RX_XAUI_CTL(interface));
    xauiCtl.u64 = cvmx_read_csr (CVMX_PCSXX_CONTROL1_REG(interface));
    xauiMiscCtl.u64 = cvmx_read_csr(CVMX_PCSXX_MISC_CTL_REG(interface));
    pcsxx_status1_reg.u64 = cvmx_read_csr(CVMX_PCSXX_STATUS1_REG(interface));
    gmxx_xaui_ext_loopback.u64 = cvmx_read_csr(CVMX_GMXX_XAUI_EXT_LOOPBACK(interface));

    printf ("GMX tx_xaui_ctl is       0x%0lu \n", gmxx_tx_xaui_ctl.u64);
    printf ("GMX rx_xaui_ctl is       0x%0lu \n", gmxx_rx_xaui_ctl.u64);
    printf ("PCS xauiCtl is           0x%0lu \n", xauiCtl.u64);
    printf ("PCS xauiMiscCtl is       0x%0lu \n", xauiMiscCtl.u64);
    printf ("PCS status1_reg is       0x%0lu \n", pcsxx_status1_reg.u64);
    printf ("GMX xaui_ext_loopback is 0x%0lu \n", gmxx_xaui_ext_loopback.u64);

}
#endif

void 
dumpstats (uint32_t port)
{
    cvmx_pip_stat_inb_pktsx_t  inb_pkts;
    cvmx_pip_stat_inb_errsx_t  inb_errs;
    int block_id = INTERFACE(port), offset = INDEX(port);
    
    printf("Port %d Statistics: block %d offset %d\n",port,block_id,offset);
    printf("CVMX_GMXX_RXX_STATS_PKTS   \t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_RXX_STATS_PKTS(offset,block_id)));
    printf("CVMX_GMXX_RXX_STATS_OCTS   \t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_RXX_STATS_OCTS(offset,block_id)));
    printf("CVMX_GMXX_RXX_STATS_PKTS_DMAC\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_RXX_STATS_PKTS_DMAC(offset,block_id)));
    printf("CVMX_GMXX_RXX_STATS_OCTS_DMAC\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_RXX_STATS_OCTS_DMAC(offset,block_id)));
    printf("CVMX_GMXX_RXX_STATS_OCTS_DRP\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_RXX_STATS_OCTS_DRP(offset,block_id)));
    printf("CVMX_GMXX_RXX_STATS_OCTS_DRP\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_RXX_STATS_OCTS_DRP(offset,block_id)));
    printf("CVMX_GMXX_RXX_STATS_PKTS_BAD\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_RXX_STATS_PKTS_BAD(offset,block_id)));
    printf("CVMX_GMXX_TXX_STAT0\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT0(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT1\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT1(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT2\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT2(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT3\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT3(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT4\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT4(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT5\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT5(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT6\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT6(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT7\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT7(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT8\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT8(offset, block_id)));
    printf("CVMX_GMXX_TXX_STAT9\t 0x%08x:%08x\n",
              cvmx_read_csr(CVMX_GMXX_TXX_STAT9(offset, block_id)));
    inb_pkts.u64 = 0;
    inb_pkts.u64 = cvmx_read_csr(CVMX_PIP_STAT_INB_PKTSX(port));
    inb_errs.u64 = 0;
    inb_errs.u64 = cvmx_read_csr(CVMX_PIP_STAT_INB_ERRSX(port));

    printf("Inbound Packets\t 0x%08x:%08x\n", inb_pkts.u64);
    printf("Inbound Errors\t  0x%08x:%08x\n", inb_errs.u64);
    return;
}

int 
do_dumpstats (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t port;

#if defined(CONFIG_JSRXNLE)
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
        CASE_ALL_SRX650_MODELS
            break;
        CASE_ALL_SRX550_MODELS
            srx550_dump_stats();
            return 0;
        default:
            printf("Command not supported on this board\n");
            return 0;
    }
#endif

    if ((argc == 2)) 
    {
        port = simple_strtoul(argv[1], NULL, 10);
        dumpstats(port);
    }
    else
    {
#if defined(CONFIG_JSRXNLE)
        switch (gd->board_desc.board_type)
        {
            CASE_ALL_SRX240_MODELS
                dumpstats(0);
                dumpstats(1);
                break;

            CASE_ALL_SRX650_MODELS
                dumpstats(0);
                dumpstats(1);
                dumpstats(2);
                dumpstats(3);
                dumpstats(16);
                dumpstats(17);
                dumpstats(18);
                dumpstats(19);
                break;

            default:
                printf("Command not supported on this board\n");
                return 0;
        }
#endif
    }
	
    return 0;
}

int 
dump_octeon_regs (uint32_t port)
{
    cvmx_pcsx_miscx_ctl_reg_t pcsx_misc_ctl;
    cvmx_pcsx_mrx_control_reg_t control_reg;
    cvmx_pcsx_mrx_status_reg_t status_reg;
    cvmx_pcsx_anx_results_reg_t pcsx_anx_result;
    cvmx_gmxx_inf_mode_t mode;
    cvmx_gmxx_prtx_cfg_t gmx_cfg;
    cvmx_pcsx_linkx_timer_count_reg_t pcsx_linkx_tmrcount;
    int index;
    int iface;
	
    index = INDEX(port);
    iface = INTERFACE(port);
    pcsx_misc_ctl.u64 = cvmx_read_csr(CVMX_PCSX_MISCX_CTL_REG(index, iface));
    control_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_CONTROL_REG(index, iface));
    status_reg.u64 = cvmx_read_csr(CVMX_PCSX_MRX_STATUS_REG(index, iface));
    pcsx_anx_result.u64 = cvmx_read_csr(CVMX_PCSX_ANX_RESULTS_REG(index, iface));
    pcsx_linkx_tmrcount.u64 = cvmx_read_csr(CVMX_PCSX_LINKX_TIMER_COUNT_REG(index, iface));
    mode.u64 = cvmx_read_csr(CVMX_GMXX_INF_MODE(iface));
    gmx_cfg.u64 = cvmx_read_csr(CVMX_GMXX_PRTX_CFG(index, iface));

    printf("OCTEON REG DUMP port %d idx %d intf %d:\n",port,index,iface);
    printf("PCS MISC Control reg 		0x%qx\n",pcsx_misc_ctl.u64);
    printf("PCS MR Control reg 		0x%qx\n",control_reg.u64);
    printf("PCS MR Status reg 		0x%qx\n",status_reg.u64);
    printf("PCS AN Result reg 		0x%qx\n",pcsx_anx_result.u64);
    printf("PCS Link Timer reg	 	0x%qx\n",pcsx_linkx_tmrcount.u64);
    printf("GMX Interface Mode reg		0x%qx\n",mode.u64);
    printf("GMX Port Config Mode reg 	0x%qx\n",gmx_cfg.u64);

    return 0 ;
}
	
int 
do_dump_octeon_regs (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    DECLARE_GLOBAL_DATA_PTR;
    uint32_t port;

#if defined(CONFIG_JSRXNLE)
    switch (gd->board_desc.board_type)
    {
        CASE_ALL_SRX240_MODELS
        CASE_ALL_SRX650_MODELS
            break;
        CASE_ALL_SRX550_MODELS
            srx550_dump_xaui_registers();
            return 0;
        default:
            printf("Command not supported on this board\n");
            return 0;
    }
#endif

    if ((argc < 2)) 
    {
	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
    }
    port = simple_strtoul(argv[1], NULL, 10);
    dump_octeon_regs(port);
    return 0;
}

U_BOOT_CMD(
	dumpstats,     2,     1,      do_dumpstats,
	"dumpstats    - dump cavium stats\n",
	"[port]      - dukp stats for this port or ALL\n"
);

U_BOOT_CMD(
	dumpoct,     3,     1,      do_dump_octeon_regs,
	"dumpoct      - dump octeon regs\n",
	" port\n  - port\n"
);


int
do_oct_loop (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    DECLARE_GLOBAL_DATA_PTR;
    int loop, port, ipd_port;
    uint64_t reg, data;
    struct eth_device *dev;
    char name[20];
    char *endptr;
    int err;

    if (argc != 3) 
    {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    port = simple_strtoull(argv[1], &endptr, 16);

    if (endptr == argv[1] || (port != 0 && port != 1)) {
        printf("Port should be either 0 or 1\n");
        return 1;
    }

    loop = simple_strtoull(argv[2], &endptr, 16);

    if (endptr == argv[2] || (loop != 1 && loop != 2)) {
        printf("Loop should be either 1 or 2\n");
        return 1;
    }

    /* port = ipd_port */
    ipd_port = port * 16;
    printf("%s: Configuring Loopback: port %d, ipd_port %d, loop %d\n",
            __func__, port, ipd_port, loop);
    cvmx_helper_sgmii_configure_loopback(ipd_port, (loop == 1), (loop == 2));

    /* Print status if loop was configured */
    return 0;
}

U_BOOT_CMD(
	octloop,     3,     1,      do_oct_loop,
	"octloop <port> <no/int/ext: 0/1/2>\n",
	"Configure a port in loopback: noloop-0/internal-1/external-2\n"
);


