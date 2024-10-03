#
# (C) Copyright 2003
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

VPATH   = $(TOPDIR)

include $(TOPDIR)/config.mk

LIB     = $(obj)lib$(BOARD).a
COBJS  := \
	$(BOARD)_board.o \
	cmd_bootloader.o \
	cmd_cpld.o \
	cmd_gpio.o \
	cmd_i2c.o \
	cmd_id_eeprom.o \
	cmd_mdk.o \
	cmd_nextboot.o \
	cmd_smi.o \
	cmd_watchdog.o \
	dram-template.o \
	dxj106.o \
	ls609x_uboot_drv.o \
	ls609x_uboot_drv_cmd.o \
	platform_srxsme.o

SOBJS	= 

SRCS    := $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS    := $(addprefix $(obj),$(COBJS))
SOBJS   := $(addprefix $(obj),$(SOBJS))

$(LIB): $(obj).depend $(OBJS) $(SOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
	rm -f $(OBJS) $(SOBJS)

distclean:      clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
