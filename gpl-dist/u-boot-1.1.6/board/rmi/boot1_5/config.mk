PLATFORM_LDFLAGS=

#Keep 128K space from reset vector
PAD_TO = 0x1FC20000
#BOOT2_START MUST be same as the uboot (stage 2) start address specified in 
#board/rmi/config.mk TEXT_BASE
BOOT2_START=0x8c100000
#Start of the boot2 image in flash (128K from reset vector)
FLASH_BOOT2_ADDR=0xBFC20000
FLASH_BOOT2_END=0xBFE20000



AFLAGS += -D__ASSEMBLY__ -I$(TOPDIR)/board/$(BOARDDIR)/../boot1_5/include 

AFLAGS += -DPSB_MAJOR_VER=$(PSB_MAJOR_VER) -DPSB_MINOR_VER=$(PSB_MINOR_VER)
AFLAGS += -DPSB_COMPILE_TIME=$(PSB_COMPILE_TIME)
AFLAGS += -DBOOT2_ENTRY=$(BOOT2_START)
AFLAGS += -DFLASH_BOOT2_START_ADDR=$(FLASH_BOOT2_ADDR)
AFLAGS += -DFLASH_BOOT2_END_ADDR=$(FLASH_BOOT2_END)
AFLAGS += -DBOOT2_LOADADDR=$(BOOT2_START)

AFLAGS += -march=xlr -mabi=o64
CFLAGS += -march=xlr -mabi=o64
AFLAGS += -G0 -mno-abicalls -fno-pic -fomit-frame-pointer
CFLAGS += -G0 -mno-abicalls -fno-pic -fomit-frame-pointer
LDFLAGS += -G0

