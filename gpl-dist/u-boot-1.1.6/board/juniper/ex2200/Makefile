include $(TOPDIR)/config.mk
include $(TOPDIR)/include/config.mk
include mvRules.mk


# Objects list

COMMON_OBJS     = mvCommon.o

OSSERVICES_OBJS    = mvOs.o

BOARD_OBJS = \
		mvBoardEnvLib.o \
		mvBoardEnvSpec.o

HAL_OBJS = \
		mvFlash.o \
		mvAmdFlash.o \
		mvIntelFlash.o \
		mvFlashCom.o \
		mvSpd.o \
		mvDramIf.o \
		mvSFlash.o \
		mvEthPhy.o \
		mvCntmr.o \
		mvTwsi.o \
		mvUart.o \
		mvGpp.o       \
		mvSpi.o \
		mvSpiCmnd.o \
		mvUsb.o \
		mvEth.o \
		mvPciIf.o \
		mvEthDebug.o \
		mvPex.o \
		mvMD5.o

ifeq ($(MV_PRESTERA_SWITCH), y)
HAL_OBJS     += \
		mvNetIf.o \
		mvBridge.o \
		mvVlan.o \
		mvSdma.o \
		mvUtils.o \
		mvTxHandling.o \
		mvNetTransmit.o \
		mvNetReceive.o \
		mvHwIf.o \
		mvOsPrestera.o
endif

SOC_OBJS = \
		mv_cmd.o \
		mv_dram.o \
		mvCtrlEnvLib.o \
		mvCtrlEnvAddrDec.o \
		mvAhbToMbus.o \
		mvCpuIf.o \
		mvSysDram.o \
		mvSysGbe.o \
		mvSysPex.o \
		mvSysUsb.o \
		mvCpu.o \
		mvDevice.o 

ifneq ($(MV_TINY_IMAGE), y) 
SOC_OBJS     += mvPciUtils.o
endif

USP_OBJS        = \
		mv_flash.o \
		mv_serial.o \
		mv_pageTable.o \
		mv_mon_init.o \
		mv_pci.o \
		mv_egiga.o \
		mv_protectionUnit.o \
		mv_i2c.o \
		mvSwitch.o


ifeq ($(MV_PRESTERA_SWITCH), y)
USP_OBJS       += mv_prestera_switch.o
SOC_OBJS       += mv_prestera_cmd.o mv_test_cmd.o
endif

ifeq ($(NAND_BOOT), y)
USP_OBJS        += nBootloader.o
endif
                  
SOBJS           = \
		jump.o \
		platform.o \
		mvDramIfBasicInit.o \
		mvDramIfConfig.o \
		mvTwsiEeprom.o \
		mvCpuIfInit.o

ifeq ($(NAND_BOOT), y)
SOBJS        += nBootstrap.o
endif
		  
COBJS = \
		ex2200.o \
		syspld.o \
		led.o \
		pcie.o \
		pcie_indirect.o \
		pcie_auto.o \
		pci_express.o \
		cmd_ex2200.o
		
LOBJS = cmd_jmem.o

MOBJS            = $(COMMON_OBJS) $(OSSERVICES_OBJS) $(HAL_OBJS) $(SOC_OBJS) $(BOARD_OBJS) $(USP_OBJS) 

LIB             = $(obj)lib$(BOARD).a

SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
LOBJS	:= $(addprefix $(obj),$(LOBJS))
MOBJS	:= $(addprefix $(obj),$(MOBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

all:    $(LIB)

$(LIB):	$(obj).depend $(OBJS) $(LOBJS) $(MOBJS) $(SOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(LOBJS) $(MOBJS) $(SOBJS)

$(obj)cmd_jmem.o:	$(TOPDIR)/board/juniper/ex3242/cmd_jmem.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCommon.o:	$(COMMON_DIR)/mvCommon.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvOs.o:	$(OSSERVICES_DIR)/mvOs.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvBoardEnvLib.o:	$(BOARD_ENV_DIR)/mvBoardEnvLib.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvBoardEnvSpec.o:	$(BOARD_ENV_DIR)/mvBoardEnvSpec.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvFlash.o:	$(HAL_FLASH_DIR)/mvFlash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvAmdFlash.o:	$(HAL_FLASH_DIR)/mvAmdFlash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvIntelFlash.o:	$(HAL_FLASH_DIR)/mvIntelFlash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvFlashCom.o:	$(HAL_FLASH_DIR)/mvFlashCom.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSpd.o:	$(HAL_SPD_DIR)/mvSpd.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvDramIf.o:	$(HAL_DRAM_DIR)/mvDramIf.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSFlash.o:	$(HAL_SFLASH_DIR)/mvSFlash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvEthPhy.o:	$(HAL_ETHPHY_DIR)/mvEthPhy.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCntmr.o:	$(HAL_CNTMR_DIR)/mvCntmr.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvTwsi.o:	$(HAL_TWSI_DIR)/mvTwsi.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvUart.o:	$(HAL_UART_DIR)/mvUart.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvGpp.o:	$(HAL_GPP_DIR)/mvGpp.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSpi.o:	$(HAL_SPI_DIR)/mvSpi.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSpiCmnd.o:	$(HAL_SPI_DIR)/mvSpiCmnd.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvUsb.o:	$(HAL_USB_DIR)/mvUsb.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvEth.o:	$(HAL_ETH_GBE_DIR)/mvEth.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvPciIf.o:	$(HAL_PCIIF_DIR)/mvPciIf.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvEthDebug.o:	$(HAL_ETH_GBE_DIR)/mvEthDebug.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvPex.o:	$(HAL_PEX_DIR)/mvPex.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvMD5.o:	$(HAL_CESA_DIR)/mvMD5.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvNetIf.o:	$(HAL_PRESTERA_DIR)/sdma/mvNetIf.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvBridge.o:	$(HAL_PRESTERA_DIR)/sdma/mvBridge.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvVlan.o:	$(HAL_PRESTERA_DIR)/vlan/mvVlan.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSdma.o:	$(HAL_PRESTERA_DIR)/sdma/mvSdma.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvUtils.o:	$(HAL_PRESTERA_DIR)/util/mvUtils.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvTxHandling.o:	$(HAL_PRESTERA_DIR)/sdma/mvTxHandling.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvNetTransmit.o:	$(HAL_PRESTERA_DIR)/util/mvNetTransmit.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvNetReceive.o:	$(HAL_PRESTERA_DIR)/util/mvNetReceive.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvHwIf.o:	$(HAL_PRESTERA_DIR)/hwIf/mvHwIf.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvOsPrestera.o:	$(HAL_PRESTERA_DIR)/mvOsPrestera.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_cmd.o:	$(SOC_DIR)/mv_cmd.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_dram.o:	$(SOC_DIR)/mv_dram.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCtrlEnvLib.o:	$(SOC_ENV_DIR)/mvCtrlEnvLib.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCtrlEnvAddrDec.o:	$(SOC_ENV_DIR)/mvCtrlEnvAddrDec.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvAhbToMbus.o:	$(SOC_SYS_DIR)/mvAhbToMbus.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCpuIf.o:	$(SOC_SYS_DIR)/mvCpuIf.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSysDram.o:	$(SOC_SYS_DIR)/mvSysDram.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSysGbe.o:	$(SOC_SYS_DIR)/mvSysGbe.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSysPex.o:	$(SOC_SYS_DIR)/mvSysPex.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSysUsb.o:	$(SOC_SYS_DIR)/mvSysUsb.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCpu.o:	$(SOC_CPU_DIR)/mvCpu.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvDevice.o:	$(SOC_DEVICE_DIR)/mvDevice.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvPciUtils.o:	$(HAL_PCIIF_DIR)/pci_util/mvPciUtils.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_flash.o:	$(USP_DIR)/mv_flash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_serial.o:	$(USP_DIR)/mv_serial.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_pageTable.o:	$(USP_DIR)/mv_pageTable.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_mon_init.o:	$(USP_DIR)/mv_mon_init.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_pci.o:	$(USP_DIR)/mv_pci.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_egiga.o:	$(USP_DIR)/mv_egiga.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_protectionUnit.o:	$(USP_DIR)/mv_protectionUnit.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_i2c.o:	$(USP_DIR)/mv_i2c.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSwitch.o:	$(USP_ETH_SWITCH_DIR)/mvSwitch.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_prestera_switch.o:	$(USP_DIR)/mv_prestera_switch.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_prestera_cmd.o:	$(SOC_DIR)/mv_prestera_cmd.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_test_cmd.o:	$(SOC_DIR)/mv_test_cmd.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)nBootloader.o:	$(USP_DIR)/nBootloader.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)jump.o:	$(USP_DIR)/jump.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)platform.o:	$(SOC_DIR)/platform.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvDramIfBasicInit.o:	$(HAL_DRAM_DIR)/mvDramIfBasicInit.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvDramIfConfig.o:	$(HAL_DRAM_DIR)/mvDramIfConfig.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvTwsiEeprom.o:	$(HAL_TWSI_DIR)/mvTwsiEeprom.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCpuIfInit.o:	$(SOC_SYS_DIR)/mvCpuIfInit.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)nBootstrap.o:	$(SOC_DIR)/nBootstrap.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

#########################################################################

include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

