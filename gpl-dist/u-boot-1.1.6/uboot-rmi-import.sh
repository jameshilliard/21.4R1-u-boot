#!/bin/sh
# $Id: uboot-rmi-import.sh,v 1.1.2.1 2009-09-25 00:10:45 kirant Exp $
#
# Copyright (c) 2009-2010, Juniper Networks, Inc.
# All rights reserved.

# Before running this script:
#   Unzip and untar the tarball
#   cd juniper/contrib/u-boot
mkdir -p board/rmi/boot2 board/rmi/rmi_boards

mv board/rmi_boards/board.c         board/rmi/boot2/board.c
mv board/rmi_boards/cpld.c          board/rmi/boot2/cpld.c
mv board/rmi_boards/hooks.c         board/rmi/boot2/hooks.c
mv board/rmi_boards/packet.c        board/rmi/boot2/packet.c
mv board/rmi_boards/pci-phoenix.c   board/rmi/boot2/pci-phoenix.c
mv board/rmi_boards/pcie-condor.c   board/rmi/boot2/pcie-condor.c
mv board/rmi_boards/pcmcia.c        board/rmi/boot2/pcmcia.c
mv board/rmi_boards/piobus.c        board/rmi/boot2/piobus.c
mv board/rmi_boards/serial.c        board/rmi/boot2/serial.c
mv board/rmi_boards/uart.c          board/rmi/boot2/uart.c
mv board/rmi_boards/yukon.c         board/rmi/boot2/yukon.c
mv board/rmi_boards/cfiflash.c      board/rmi/rmi_boards/cfiflash.c
mv board/rmi_boards/flash_intf.c    board/rmi/rmi_boards/flash_intf.c
mv board/rmi_boards/micron_nand.c   board/rmi/rmi_boards/micron_nand.c
mv board/rmi_boards/nand_ecc.c      board/rmi/rmi_boards/nand_ecc.c
mv board/rmi_boards/nand_flash.c    board/rmi/rmi_boards/nand_flash.c
mv board/rmi_boards/rmi_cfi_intf.c  board/rmi/rmi_boards/rmi_cfi_intf.c
mv board/rmi_boards/rmi_nand_intf.c board/rmi/rmi_boards/rmi_nand_intf.c
mv board/rmi_boards/rmigmac.c       board/rmi/rmi_boards/rmigmac.c
mv board/rmi_boards/rmigmac_phy.c   board/rmi/rmi_boards/rmigmac_phy.c
mv board/rmi_boards/usb_ohci.c      board/rmi/rmi_boards/usb_ohci.c
mv board/rmi_boards/usb_ohci.h      board/rmi/rmi_boards/usb_ohci.h

#Done with boot2 source files.... Moving boot1_5
mv nand_spl/board/rmi_boards        board/rmi/boot1_5

#remove the files that are not necessary for us as they are duplicated
rm -f board/rmi/boot1_5/ld.rmi.s board/rmi/boot1_5/ld.rmi.S

#move files from gnusys/u-boot directory and to contrib directory
#so that all the required files are in one location.
#Actually moving to the juniper directory to move files from gnusys to contrib

cd ../../

mv -f gnusys/u-boot/board/rmi_boards/config.mk         contrib/u-boot/board/rmi/boot2/
mv -f gnusys/u-boot/board/rmi_boards/Makefile          contrib/u-boot/board/rmi/boot2/
mv -f gnusys/u-boot/board/rmi_boards/ld.rmi.s          contrib/u-boot/board/rmi/boot2/ld.rmi.lds
mv -f gnusys/u-boot/common/cmd_usb.c                   contrib/u-boot/common/
mv -f gnusys/u-boot/common/Makefile                    contrib/u-boot/common/
mv -f gnusys/u-boot/common/usb_storage.c               contrib/u-boot/common/
mv -f gnusys/u-boot/Makefile                           contrib/u-boot/
mv -f gnusys/u-boot/mkconfig                           contrib/u-boot/
mv -f gnusys/u-boot/config.mk                          contrib/u-boot/
mv -f gnusys/u-boot/cpu/rmi_mips/Makefile              contrib/u-boot/cpu/rmi_mips/
mv -f gnusys/u-boot/cpu/rmi_mips/config.mk             contrib/u-boot/cpu/rmi_mips/
mv -f gnusys/u-boot/nand_spl/board/rmi_boards/Makefile contrib/u-boot/board/rmi/boot1_5/
mv -f gnusys/u-boot/nand_spl/board/rmi_boards/ld.rmi.s contrib/u-boot/board/rmi/boot1_5/ld.rmi.lds
mv -f gnusys/u-boot/net/*.c                            contrib/u-boot/net/
mv -f gnusys/u-boot/net/Makefile                       contrib/u-boot/net/
mv -f gnusys/u-boot/include/common.h                   contrib/u-boot/include
mv -f gnusys/u-boot/drivers/usb_ehci.c                 contrib/u-boot/board/rmi/boot2/
mv -f gnusys/u-boot/drivers/usb_ehci.h                 contrib/u-boot/board/rmi/boot2/

#remove the empty directories that are not necessary to be imported.
rm -rf contrib/u-boot/nand_spl/board
rm -rf contrib/u-boot/board/rmi_boards

cd contrib/u-boot

CVS=${CVS:-/volume/fwtools/cvs/1.12.13.0.2-JNPR_2008_3_1/bin/cvs}
CVSROOT=freeblend.juniper.net:/cvs/junos-2008
#CVSROOT=freeblend.juniper.net:/cvs/cvs-data2/clone/junos-2008-kirant-20090828

# Check if this is the u-boot distribution directory
if test -s lib_ppc/board.c
then
    case `cat CVS/Root 2>/dev/null` in
        *junos-2008*) echo "Must not be in a checked out JUNOS tree"; exit  1;;
        esac
else
        echo "Must import from a u-boot directory structure"; exit 1
fi


while :
do
        case "$1" in
        *=*) eval "$1"; shift;;
        --) shift; break;;
        -n) ECHO=echo; shift;;
        -u) URL=$2; shift 2;;
        -P) PR=$2; shift 2;;
        -V) VERSION=$2; shift 2;;
        -B) BRANCH=$2; shift 2;;
        -h) echo "Usage: -P <PR #> -V <VERSION #> -B <BRANCH #>"; exit 1;;
        *) break;;
        esac
done

[ "$PR" ] || { echo "We will need a PR.  Use -P <PR>." >&2; exit 1; }

[ "$BRANCH" ] || { echo "We will need a BRANCH.  Use -B <BRANCH>." >&2; exit 1; }

version_needed () {
    echo "We will need a version.  $1 not found in Makefile" >&2
    exit 1
}

# check that specified version is correct
# SRC_VERSION=`grep -m 1 "RMI version: " ./include/configs/mv_kw.h | awk '{ print $6 }' | sed 's/-.*//g' | sed 's/\./\_/g'`
# [ "SRC_VERSION" ] || version_needed SRC_VERSION

# Default the version to the one read from the files. Allow
# the '-V version' command-line to use that version as a prefix
# if necessary to allow a new timestamp suffix to be added if needed.
VERSION=${VERSION:-${SRC_VERSION}}
case $VERSION in
${SRC_VERSION}*) ;;
*) echo "Version read from sources $SRC_VERSION not a prefix of $VERSION" >&2
   echo "aborting import" >&2
   exit 1
;;
esac

VERSION_TAG=`echo RMI_UBOOT_${VERSION} | tr ' .' '-_'`

MODULE=vendor/u-boot

# make sure the tag is not already used
wcfind . -type f 2>/dev/null \
  | sed "s,^\.,$MODULE," \
  | xargs $CVS -d $CVSROOT rlog 2>/dev/null \
  | egrep "^[[:space:]]${VERSION_TAG}:[[:space:]][0-9.]+" 2>&1 >/dev/null && \
  { echo "CVS tag '$VERSION_TAG' is already being used" >&2; \
    echo "Use '-V <VERSION>' to specify another version number." >&2; \
    exit 1; }

# make sure CVS is executable and supports the -X flag
[ -x "$CVS" ] || { echo "$CVS is not executable" >&2; exit 1; }
$CVS -d $CVSROOT -H import 2>&1 | egrep '^[[:space:]]*-X' >/dev/null || \
    { echo "$CVS does not support the -X flag, it is needed for this import" >&2; \
      exit 1; }

MESSAGE=
if [ "$URL" ]; then
  MESSAGE="Source URL: $URL"
fi

$ECHO ${CVS:-cvs} -d $CVSROOT import -b $BRANCH -ko -X -I! -I CVS \
        -I uboot-rmi-import.sh -m "Import RMI u-boot $VERSION 
U-BOOT The universal boot loader 

Home Page URL: http://www.denx.de/wiki/UBoot
$MESSAGE
PR: $PR" \
        $MODULE UBOOT-RMI $VERSION_TAG
