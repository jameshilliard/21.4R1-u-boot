include mvRules.mk


# Objects list

COMMON_OBJS     = mvCommon.o

OSSERVICES_OBJS    = \
		mvOs.o

BOARD_OBJS = \
		mvBoardEnvLib.o \
		mvBoardEnvSpec.o

HAL_OBJS =	mvFlash.o \
		mvAmdFlash.o \
		mvIntelFlash.o \
		mvFlashCom.o \
		mvDram.o \
		mvDramIf.o \
		mvCntmr.o \
		mvTwsi.o \
		mvUart.o \
		mvGpp.o       \
		mvMFlash.o \
		mvSpi.o \
		mvSMFlash.o \
		mvPMFlash.o \
		mvSpiCmnd.o \
		mvEth.o \
		mvEthPhy.o

SOC_OBJS = \
		mv_cmd.o \
		mv_dram.o \
		mvCtrlEnvLib.o \
		mvCtrlEnvAddrDec.o \
		mvAhbToMbus.o \
		mvCpuIf.o \
		mvCpuArm.o \
		mvDevice.o \
		mvSysGbe.o

ifneq ($(MV_TINY_IMAGE), y) 
#SOC_OBJS     += $(HAL_PCIIF_DIR)/pci_util/mvPciUtils.o
endif

USP_OBJS        = mv_flash.o \
		mv_serial.o \
		mv_pageTable.o \
		mv_mon_init.o \
		mv_egiga.o \
		mv_protectionUnit.o \
		mv_i2c.o
                  
SOBJS           = jump.o \
		platform.o \
		mvDramIfBasicInit.o \
		mvDramIfConfig.o \
		mvTwsiEeprom.o \
		mvCpuIfInit.o

COBJS = \
		rps200.o \
		cmd_rps200.o
		
LOBJS = cmd_jmem.o

MOBJS = $(COMMON_OBJS) $(OSSERVICES_OBJS) $(HAL_OBJS) $(SOC_OBJS) $(BOARD_OBJS) $(USP_OBJS) 

LIB             = $(obj)lib$(BOARD).a

SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
LOBJS	:= $(addprefix $(obj),$(LOBJS))
MOBJS	:= $(addprefix $(obj),$(MOBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

CPPFLAGS += $(CFLAGS)

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

$(obj)mvDram.o:	$(HAL_DRAM_DIR)/mvDram.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvDramIf.o:	$(HAL_DRAM_DIR)/mvDramIf.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvCntmr.o:	$(HAL_CNTMR_DIR)/mvCntmr.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvTwsi.o:	$(HAL_TWSI_DIR)/mvTwsi.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvUart.o:	$(HAL_UART_DIR)/mvUart.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvGpp.o:	$(HAL_GPP_DIR)/mvGpp.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvMFlash.o:	$(HAL_MFLASH_DIR)/mvMFlash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSpi.o:	$(HAL_SPI_DIR)/mvSpi.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSMFlash.o:	$(HAL_MFLASH_DIR)/mvSMFlash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvPMFlash.o:	$(HAL_MFLASH_DIR)/mvPMFlash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSpiCmnd.o:	$(HAL_SPI_DIR)/mvSpiCmnd.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvEth.o:	$(HAL_ETH_GBE_DIR)/mvEth.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvEthPhy.o:	$(HAL_ETHPHY_DIR)/mvEthPhy.c
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

$(obj)mvCpuArm.o:	$(SOC_CPU_DIR)/mvCpuArm.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvDevice.o:	$(SOC_DEVICE_DIR)/mvDevice.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mvSysGbe.o:	$(SOC_SYS_DIR)/mvSysGbe.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_flash.o:	$(USP_DIR)/mv_flash.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_serial.o:	$(USP_DIR)/mv_serial.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_pageTable.o:	$(USP_DIR)/mv_pageTable.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_mon_init.o:	$(USP_DIR)/mv_mon_init.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_egiga.o:	$(USP_DIR)/mv_egiga.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_protectionUnit.o:	$(USP_DIR)/mv_protectionUnit.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)mv_i2c.o:	$(USP_DIR)/mv_i2c.c
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

#########################################################################

include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

