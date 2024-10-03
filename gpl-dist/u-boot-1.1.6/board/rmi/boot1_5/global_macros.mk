#set the global verbosity level
#ifeq ("$(V)", "0")
CFLAGS += -DVERBOSE0
AFLAGS += -DVERBOSE0
#endif 

# parse the 'template' target
ifeq ("$(TEMPLATE)", "1")
CFLAGS += -DCFG_BOARD_TEMPLATE
AFLAGS += -DCFG_BOARD_TEMPLATE
endif


CFLAGS += -DDEBUG_MODE_WATCH_EXCEPTION
AFLAGS += -DDEBUG_MODE_WATCH_EXCEPTION

#One of the following defines needs to be set for the QDR mem frequency
#define QDR2_133 
#define QDR2_200 
#define QDR2_233 
#define QDR2_266 
CFLAGS += -DQDR2_233  
AFLAGS += -DQDR2_233

# to build spd with memtester, do: make clean; make SPD_TEST=1
# bootloader with this flag on only works for 732/532 xlr.
ifeq ("$(SPD_TEST)", "1")
CFLAGS += -DSPD_TEST
AFLAGS += -DSPD_TEST
endif

# for the bootloader to run on ssim/psim, the following macros should be set
ifeq ("$(PHOENIX_SIM)", "1")
CFLAGS +=-DPHOENIX_SIM
AFLAGS += -DPHOENIX_SIM
endif

MEMSIZE=0x3f00000
LOADADDR=0xffffffff8c100000

## For two channel mode comment the following lines
#CFLAGS += -DARIZONA_ONE_CHANNEL
#AFLAGS += -DARIZONA_ONE_CHANNEL
#ARIZONA_ONE_CHANNEL=1

BOOT1_INFO=0xffffffffac000000

CFLAGS +=  -DBOOT1_INFO=$(BOOT1_INFO) -DMEMSIZE=$(MEMSIZE)
AFLAGS += -DBOOT1_INFO=$(BOOT1_INFO) -DMEMSIZE=$(MEMSIZE)

SDK_VERSION="Uboot Development"
PSB_MAJOR_VER=0
PSB_MINOR_VER=1
PSB_COMPILE_TIME=$(shell date +%s)
