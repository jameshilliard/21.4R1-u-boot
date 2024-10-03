/***********************license start***************
 * Copyright (c) 2003-2008  Cavium Networks (support@cavium.com). All rights
 * reserved.
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
 *
 * For any questions regarding licensing please contact marketing@caviumnetworks.com
 *
 ***********************license end**************************************/






/**
 * @file
 *
 * Interface to the SMI/MDIO hardware.
 *
 * <hr>$Revision: 1.2 $<hr>
 */

#ifndef __CVMX_MIO_H__
#define __CVMX_MIO_H__

#ifdef	__cplusplus
extern "C" {
#endif


/**
 * Perform an MII read. This function is used to read PHY
 * registers controlling auto negotiation.
 *
 * @param bus_id   MDIO bus number. Zero on most chips, but some chips (ex CN56XX)
 *                 support multiple busses.
 * @param phy_id   The MII phy id
 * @param location Register location to read
 *
 * @return Result from the read or -1 on failure
 */
static inline int cvmx_mdio_read(int bus_id, int phy_id, int location)
{
    cvmx_smix_cmd_t smi_cmd;
    cvmx_smix_rd_dat_t smi_rd;
    int timeout = 1000;

    smi_cmd.u64 = 0;
    smi_cmd.s.phy_op = 1;
    smi_cmd.s.phy_adr = phy_id;
    smi_cmd.s.reg_adr = location;
    cvmx_write_csr(CVMX_SMIX_CMD(bus_id), smi_cmd.u64);

    do
    {
        cvmx_wait(1000);
        smi_rd.u64 = cvmx_read_csr(CVMX_SMIX_RD_DAT(bus_id));
    } while (smi_rd.s.pending && timeout--);

    if (smi_rd.s.val)
        return smi_rd.s.dat;
    else
        return -1;
}


/**
 * Perform an MII write. This function is used to write PHY
 * registers controlling auto negotiation.
 *
 * @param bus_id   MDIO bus number. Zero on most chips, but some chips (ex CN56XX)
 *                 support multiple busses.
 * @param phy_id   The MII phy id
 * @param location Register location to write
 * @param val      Value to write
 *
 * @return -1 on error
 *         0 on success
 */
static inline int cvmx_mdio_write(int bus_id, int phy_id, int location, int val)
{
    cvmx_smix_cmd_t smi_cmd;
    cvmx_smix_wr_dat_t smi_wr;
    int timeout = 1000;

    smi_wr.u64 = 0;
    smi_wr.s.dat = val;
    cvmx_write_csr(CVMX_SMIX_WR_DAT(bus_id), smi_wr.u64);

    smi_cmd.u64 = 0;
    smi_cmd.s.phy_op = 0;
    smi_cmd.s.phy_adr = phy_id;
    smi_cmd.s.reg_adr = location;
    cvmx_write_csr(CVMX_SMIX_CMD(bus_id), smi_cmd.u64);

    do
    {
        cvmx_wait(1000);
        smi_wr.u64 = cvmx_read_csr(CVMX_SMIX_WR_DAT(bus_id));
    } while (smi_wr.s.pending && --timeout);
    if (timeout <= 0)
        return -1;

    return 0;
}

#ifdef	__cplusplus
}
#endif

#endif

