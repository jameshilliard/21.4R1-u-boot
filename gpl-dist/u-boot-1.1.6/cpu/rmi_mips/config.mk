MIPSFLAGS="-march=xlr"
ENDIANNESS=

MIPSFLAGS += $(ENDIANNESS)
PLATFORM_CPPFLAGS += $(MIPSFLAGS)
AFLAGS += -march=xlr -mabi=o64
PLATFORM_CPPFLAGS += -march=xlr -mabi=o64
AFLAGS += -G0 -mno-abicalls -fno-pic -fomit-frame-pointer
PLATFORM_CPPFLAGS += -G0 -mno-abicalls -fno-pic -fomit-frame-pointer
PLATFORM_CPPFLAGS += -I$(ALTSRCTREE)/board/$(BOARDDIR)/boot1_5/include
PLATFORM_CPPFLAGS += -I$(ALTSRCTREE)/board/$(BOARDDIR)/boot1_5/nand
AFLAGS += -I$(ALTSRCTREE)/board/$(BOARDDIR)/boot1_5/include
AFLAGS += -I$(ALTSRCTREE)/board/$(BOARDDIR)/boot1_5/nand

