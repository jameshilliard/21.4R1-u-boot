/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/

/*
	for PHY utils
*/

#include "rmi/rmigmac.h"
#include "rmi/gmac.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"

#define PHY_STATUS_RETRIES 25000
#define R_INT_MASK_REG 25
#define RMI_GMAC_PORTS_PER_CTRL 4
#define RMI_GMAC_TOTAL_PORTS 8
#define READ_COMMAND 1
#define MGMT_CLOCK_SELECT 0x07   /* divisor = 28 */

extern struct rmi_board_info rmi_board;

enum {
   SERDES_CTRL_REG_PHY_ID = 26,
   SGMII_PCS_GMAC0_PHY_ID,
   SGMII_PCS_GMAC1_PHY_ID,
   SGMII_PCS_GMAC2_PHY_ID,
   SGMII_PCS_GMAC3_PHY_ID,
};

enum {
	SGMII_GMAC0_PHY_ID = 16,
	SGMII_GMAC1_PHY_ID,
	SGMII_GMAC2_PHY_ID,
	SGMII_GMAC3_PHY_ID,
	SGMII_GMAC4_PHY_ID,
	SGMII_GMAC5_PHY_ID,
	SGMII_GMAC6_PHY_ID,
	SGMII_GMAC7_PHY_ID,
};

#define NR_PHYS 32
static gmac_eth_info_t *phys[NR_PHYS]; 

static unsigned int serdes_ctrl_regs[] = {
   [0] = 0x6db0,
   [1] = 0xffff,
   [2] = 0xb6d0,
   [3] = 0x00ff,
   [4] = 0x0000,
   [5] = 0x0000,
   [6] = 0x0005,
   [7] = 0x0001,
   [8] = 0x0000,
   [9] = 0x0000,
   [10] = 0x0000,
};

#define NUM_SERDES_CTRL_REGS ((int)sizeof(serdes_ctrl_regs)/sizeof(unsigned int))

static unsigned long gmac_mmio[RMI_GMAC_TOTAL_PORTS] = {
	[0] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_0_OFFSET), 
	[1] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_1_OFFSET),	
	[2] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_2_OFFSET),	
	[3] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_3_OFFSET),	
	[4] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_4_OFFSET),	
	[5] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_5_OFFSET),	
	[6] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_6_OFFSET),	
	[7] = (DEFAULT_RMI_PROCESSOR_IO_BASE + PHOENIX_IO_GMAC_7_OFFSET),	
};


uint32_t rmi_get_gmac_phy_id(unsigned int mac, int type)
{   
   uint32_t addr = 0;
	
	if (chip_processor_id_xls()) {
		if ((mac == 0) && (type == RMI_GMAC_IN_RGMII)) {
			addr = 0;
		} else {
			addr = mac + SGMII_GMAC0_PHY_ID;
		}
	} else {
		addr = mac;
	}
    
   return addr;
}


uint32_t rmi_get_gmac_mii_addr(unsigned int mac, int type)
{
   uint32_t addr = 0;
	struct rmi_board_info * rmib = &rmi_board;

	if ((chip_processor_id_xls())    &&
       (rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_VI)       &&
       (mac == 0)                   &&
       (type == RMI_GMAC_IN_RGMII)) {
				addr = gmac_mmio[4];
	} 
	
	/* for all other boards */
	if (addr == 0)
		addr = gmac_mmio[0];
    
   return addr;
}


/* called only for sgmii ports */

unsigned long rmi_get_gmac_serdes_addr(unsigned int mac)
{
   unsigned long addr;

	if (mac < RMI_GMAC_PORTS_PER_CTRL) {
		addr = gmac_mmio[0];
	} else {
		addr = gmac_mmio[4];
	}
   return addr;
}


uint32_t rmi_get_gmac_pcs_addr(unsigned int mac)
{
	return (rmi_get_gmac_serdes_addr(mac));
}


/* args : gmac index 0 to 7 */
unsigned long rmi_get_gmac_mmio_addr(unsigned int mac)
{
   return (gmac_mmio[mac]);
}


static int gmac_phy_mdio_read
	(phoenix_reg_t *mmio, int phy_id, int reg, uint32_t *data)
{
   int retVal = 0;
   int i = 0;
   
   /* setup the phy reg to be read */
   phoenix_write_reg(mmio, MIIM_ADDR,(phy_id << 8) | reg);
   
   /* perform a read cycle */ 
   phoenix_write_reg(mmio, MIIM_CMD, READ_COMMAND);

   /* poll for the read cycle to complete */
   for (i = 0; i < PHY_STATUS_RETRIES; i++) {
      udelay(10 * 1000);
      if (phoenix_read_reg(mmio, MIIM_IND) == 0)
         break;
   }
   
   /* clear the read cycle */
   phoenix_write_reg(mmio, MIIM_CMD, 0);

   if (i == PHY_STATUS_RETRIES) {
      *data = 0xffffffff;
      printf("Retries failed.\n");
      goto out;
   }
   
   /* return the read data */
   *data = (uint32_t) phoenix_read_reg(mmio, MIIM_STAT);
   retVal = 1;
out:
   return retVal;
}

static int gmac_phy_mdio_write
	(phoenix_reg_t *mmio, int phy_id, int reg, uint32_t data)
{
   int i = 0;

   /* setup the phy reg to be read */
   phoenix_write_reg(mmio, MIIM_ADDR, (phy_id << 8) | reg);

   /* write the data which starts the write cycle */
   phoenix_write_reg(mmio, MIIM_CTRL, data);

   /* poll for the write cycle to complete */
   for (i = 0; i < PHY_STATUS_RETRIES; i++) {
      udelay(10 * 1000);
      if (phoenix_read_reg(mmio, MIIM_IND) == 0)
         break;
   }

   /* clear the write data/cycle */
   /* phoenix_write_reg(mmio, MIIM_CTRL, 0); */

   if (i == PHY_STATUS_RETRIES) {
      return 0;
   }

   return 1;
}

int gmac_phy_read(gmac_eth_info_t *this_phy, int reg, uint32_t *data)
{
	return (gmac_phy_mdio_read((phoenix_reg_t *)this_phy->mii_addr,
										this_phy->phy_id,
										reg,
										data));
}

int gmac_phy_write(gmac_eth_info_t *this_phy, int reg, uint32_t data)
{
	return (gmac_phy_mdio_write((phoenix_reg_t *)this_phy->mii_addr,
										 this_phy->phy_id,
										 reg,
										 data));
}


void gmac_clear_phy_interrupt_mask(gmac_eth_info_t *this_phy) 
{
	uint32_t value;
   gmac_phy_read(this_phy, 26, &value);
}

void gmac_set_phy_interrupt_mask(gmac_eth_info_t *this_phy, uint32_t mask) 
{
   gmac_phy_write(this_phy, 25, mask);
}

static void gmac_init_serdes_ctrl_regs(uint32_t mac, int gmac_type)
{
   uint32_t data=0, mmio;
   int phy_id = SERDES_CTRL_REG_PHY_ID;
   int reg = 0;

   mmio = rmi_get_gmac_serdes_addr(mac);

   /* If port 0 is in RGMII mode */
   if (chip_processor_id_xls() && 
		 (gmac_type == RMI_GMAC_IN_RGMII)) 
	{
        reg = 7;
        data = 1;
        gmac_phy_mdio_write((phoenix_reg_t *)mmio, phy_id, reg, data);
        return;
    }

    /* initialize SERDES CONTROL Registers */
    for (reg = 0; reg < NUM_SERDES_CTRL_REGS; reg++) {
       data = serdes_ctrl_regs[reg];
       gmac_phy_mdio_write((phoenix_reg_t *)mmio, phy_id, reg, data);
    }

   return;
}


void gmac_phy_init(gmac_eth_info_t * this_phy)
{
	uint32_t phy_id, mii_mmio;
	uint32_t mac, pcs_mmio, value;
	int nr_val, isxls = chip_processor_id_xls();
	int gmac_type, iteration_count  = 0x0;

	phy_id = this_phy->phy_id;
	mii_mmio = this_phy->mii_addr;
	mac = this_phy->port_id;
	gmac_type = this_phy->phy_mode;

	if (isxls)
		nr_val = NR_PHYS;
	else
		nr_val = 4;

	if(phy_id < nr_val) {
		/* lower the mgmt clock freq to minimum possible freq */
      phoenix_write_reg((phoenix_reg_t *)mii_mmio, MIIM_CONF, MGMT_CLOCK_SELECT);
      /* Bring out the MAC controlling this phy out of reset. */
      phoenix_write_reg((phoenix_reg_t *)mii_mmio,MAC_CONF1,0x35);
      phys[phy_id] = this_phy;
	} else {
		printf("Cannot add phy %d to the list.\n",phy_id);
	}

	/*Set Phy Mask*/
   gmac_phy_mdio_write((phoenix_reg_t *)mii_mmio, phy_id, R_INT_MASK_REG, 0xfffffffe);

   if(!isxls)
      return;

   gmac_init_serdes_ctrl_regs(mac, gmac_type);

   if(mac >= RMI_GMAC_PORTS_PER_CTRL) {
      mac -= RMI_GMAC_PORTS_PER_CTRL;
   }

	pcs_mmio = this_phy->pcs_addr;

	if (!pcs_mmio)
		return;

   gmac_phy_mdio_write((phoenix_reg_t *)pcs_mmio, SGMII_PCS_GMAC0_PHY_ID + mac, 0, 0x1000);
   udelay(100 * 1000);
   gmac_phy_mdio_write((phoenix_reg_t *)pcs_mmio, SGMII_PCS_GMAC0_PHY_ID + mac, 0, 0x0200);

   for(;;) {
      gmac_phy_mdio_read((phoenix_reg_t *)pcs_mmio,  SGMII_PCS_GMAC0_PHY_ID + mac, 1, &value);
      if ( (value & 0x04) == 0x04) {
         break;
      }

      if (iteration_count++ == 10) {
         printf("[%s]: Timed-out waiting for the SGMII Link Status!!\n", __FUNCTION__);
         break;
      }
   }
   return;
}

void gmac_clear_phy_intr(gmac_eth_info_t *this_phy)
{
   phoenix_reg_t *mmio = NULL;
	gmac_eth_info_t * phy;
	int i, nr_val; 
	int phy_value = 0;
	int isxls = chip_processor_id_xls();

	if (isxls)
		nr_val = NR_PHYS;
	else
		nr_val = 4;

   for(i=0; i<nr_val; i++)
   {
      phy = phys[i];
      if(phy) {
         mmio = (phoenix_reg_t *)this_phy->mmio_addr;
			gmac_phy_read (phy, 26, &phy_value);
		}
		
		if (i==0 && chip_processor_id_xls2xx()) {
			/* LTE Board has no RGMII */
			struct rmi_board_info * rmib = &rmi_board;
			if (!(rmib->maj == RMI_PHOENIX_BOARD_ARIZONA_VIII)) {
				/* Clear RGMII phy interrupt.
               2XX has a common line for RGMII and SGMII */
				gmac_eth_info_t rgmii_phy = *phy;
            rgmii_phy.phy_id = 0;
				gmac_phy_read (&rgmii_phy, 26, &phy_value);
			}
		}
	}	
	phoenix_write_reg(mmio,IntReg,0xffffffff);
}


#ifdef RMI_GMAC_GLOBAL_DEBUG
#include <command.h>
/* debug functions */
int do_rmigmac (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int mac = 0;
	int reg = 0;
	phoenix_reg_t * mmio;

	if ((mac<0) || (mac>7)) {
		printf ("Invalid mac. \n");
		return 0;
	}

	mac = simple_strtoul (argv[1],0,10);
	reg = *argv[2];
	printf ("\nreg dump for gmac%d:\n", mac);
	
	mmio = (phoenix_reg_t *) gmac_mmio[mac];

	printf("r%x:0x%08x\n", MAC_CONF1, phoenix_read_reg(mmio,MAC_CONF1));
	printf("r%x:0x%08x\n", MAC_CONF2, phoenix_read_reg(mmio,MAC_CONF2));
	printf("r%x:0x%08x\n", TxControl, phoenix_read_reg(mmio,TxControl));
	printf("r%x:0x%08x\n", RxControl, phoenix_read_reg(mmio,RxControl));
	printf("r%x:0x%08x\n", CoreControl, phoenix_read_reg(mmio,CoreControl));
	
	printf ("\n");
	printf("r%x:0x%08x\n", SGMII_SPEED, phoenix_read_reg(mmio,SGMII_SPEED));
	printf("r%x:0x%08x\n", IntReg, phoenix_read_reg(mmio,IntReg));
	printf("r%x:0x%08x\n", IntMask, phoenix_read_reg(mmio,IntMask));
	printf("r%x:0x%08x\n", G_TX0_BUCKET_SIZE, phoenix_read_reg(mmio,G_TX0_BUCKET_SIZE));
	printf("r%x:0x%08x\n", G_RFR0_BUCKET_SIZE, phoenix_read_reg(mmio,G_RFR0_BUCKET_SIZE));
	printf("r%x:0x%08x\n", G_RFR1_BUCKET_SIZE, phoenix_read_reg(mmio,G_RFR1_BUCKET_SIZE));

	printf ("\n");
	printf("r%x:0x%08x\n", G_JFR0_BUCKET_SIZE, phoenix_read_reg(mmio,G_JFR0_BUCKET_SIZE));
	printf("r%x:0x%08x\n", G_JFR1_BUCKET_SIZE, phoenix_read_reg(mmio,G_JFR1_BUCKET_SIZE));
	printf("r%x:0x%08x\n", DmaCr0, phoenix_read_reg(mmio,DmaCr0));
	printf("r%x:0x%08x\n", DmaCr2, phoenix_read_reg(mmio,DmaCr2));
	printf("r%x:0x%08x\n", DmaCr3, phoenix_read_reg(mmio,DmaCr3));

	printf ("\n");
	printf("r%x:0x%08x\n", RegFrInSpillMemStart0, phoenix_read_reg(mmio,RegFrInSpillMemStart0));
	printf("r%x:0x%08x\n", RegFrInSpillMemSize, phoenix_read_reg(mmio,RegFrInSpillMemSize));
	printf("r%x:0x%08x\n", FrOutSpillMemStart0, phoenix_read_reg(mmio,FrOutSpillMemStart0));
	printf("r%x:0x%08x\n", FrOutSpillMemSize, phoenix_read_reg(mmio,FrOutSpillMemSize));

	printf ("\n");
	printf("r%x:0x%08x\n", MAC_ADDR0_LO, phoenix_read_reg(mmio,MAC_ADDR0_LO));
	printf("r%x:0x%08x\n", MAC_ADDR0_HI, phoenix_read_reg(mmio,MAC_ADDR0_HI));
	printf("r%x:0x%08x\n", MAC_ADDR_MASK0_LO, phoenix_read_reg(mmio,MAC_ADDR_MASK0_LO));
	printf("r%x:0x%08x\n", MAC_ADDR_MASK0_HI, phoenix_read_reg(mmio,MAC_ADDR_MASK0_HI));
	printf("r%x:0x%08x\n", MAC_ADDR_MASK1_LO, phoenix_read_reg(mmio,MAC_ADDR_MASK1_LO));
	printf("r%x:0x%08x\n", MAC_ADDR_MASK1_HI, phoenix_read_reg(mmio,MAC_ADDR_MASK1_HI));
	printf("r%x:0x%08x\n", MAC_FILTER_CONFIG, phoenix_read_reg(mmio,MAC_FILTER_CONFIG));

	printf ("\n");

	return 1;
}

U_BOOT_CMD(
   gmacdumpreg, 3, 1, do_rmigmac,
   "gmacdumpreg - dump all known registers\n",
   "testing\n"
); 
#endif
