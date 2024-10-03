#
# Copyright (c) 2007-2010, Juniper Networks, Inc.
# All rights reserved.
#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

LOAD_ADDR = 0x400000

include $(TOPDIR)/config.mk

ELF	= rps
SREC	= rps.srec
BIN	= rps.bin

ifeq ($(BIG_ENDIAN),y)
EX_LDFLAGS += -EB
endif

LIBARMDIR := $(SRCTREE)/lib_arm/

COBJS	:= $(SREC:.srec=.o)

LIB	= $(obj)libstubs.a
LIBAOBJS=_divsi3.o _udivsi3.o _modsi3.o \
	_umodsi3.o
LIBACOBJS= div0.o
LIBCOBJS= stubs.o hang.o rps_etq.o rps_envMon.o rps_cli.o rps_command.o \
	rps_string.o rps_cpld.o rps_led.o rps_mbox.o rps_psMon.o rps_portMon.o rps_priority.o \
	rps_msg.o cmd_rpsApp.o rps.o

LIBOBJS	= $(addprefix $(obj),$(LIBAOBJS) $(LIBCOBJS))

SRCS	:= $(COBJS:.o=.c) $(LIBCOBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(LIBCOBJS))
LOBJS	:= $(addprefix $(obj),$(LIBAOBJS))
ACOBJS	:= $(addprefix $(obj),$(LIBACOBJS))
ELF	:= $(addprefix $(obj),$(ELF))
BIN	:= $(addprefix $(obj),$(BIN))
SREC	:= $(addprefix $(obj),$(SREC))

gcclibdir := $(SRCTREE)/junos/lib/arm
#gcclibdir := $(shell dirname `$(CC) -print-libgcc-file-name`)
#clibdir := $(shell dirname `$(CC) $(CFLAGS) -print-file-name=libc.a`)

CPPFLAGS += -I..

all:	$(obj).depend $(LIB) $(SREC) $(BIN) $(ELF)

#########################################################################
$(LIB):	$(obj).depend $(OBJS) $(LOBJS) $(ACOBJS)
		$(AR) $(ARFLAGS) $@ $(OBJS) $(LOBJS) $(ACOBJS)

$(obj)_divsi3.o:	$(LIBARMDIR)_divsi3.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)_udivsi3.o:	$(LIBARMDIR)_udivsi3.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)_modsi3.o:	$(LIBARMDIR)_modsi3.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)_umodsi3.o:	$(LIBARMDIR)_umodsi3.S
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(obj)div0.o:	$(LIBARMDIR)div0.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(ELF):
$(obj)%:	$(obj)%.o $(LIB)
		$(LD) -g $(EX_LDFLAGS) -Ttext $(LOAD_ADDR) \
			-o $@ -e $(notdir $(<:.o=)) $< $(LIB) \
			-L$(gcclibdir) -lgcc -Map rps.map

$(SREC):
$(obj)%.srec:	$(obj)%
		$(OBJCOPY) -O srec $< $@ 2>/dev/null

$(BIN):
$(obj)%.bin:	$(obj)%
		$(OBJCOPY) -O binary $< $@ 2>/dev/null

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
