/*
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>
 *
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * aloong with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _PCIE_H
#define _PCIE_H



/*
 *	PCI Class, Vendor and Device IDs
 *
 *	Please keep sorted.
 */

#define PCIE_VENDOR_ID_MELLANOX		0x15b3
#define PCIE_DEVICE_ID_MELLANOX_TAVOR	0x5a44
#define PCIE_DEVICE_ID_MELLANOX_ARBEL_COMPAT 0x6278
#define PCIE_DEVICE_ID_MELLANOX_ARBEL	0x6282
#define PCIE_DEVICE_ID_MELLANOX_SINAI_OLD 0x5e8c
#define PCIE_DEVICE_ID_MELLANOX_SINAI	0x6274


/* Device classes and subclasses */

#define PCIE_CLASS_NOT_DEFINED		0x0000
#define PCIE_CLASS_NOT_DEFINED_VGA	0x0001

#define PCIE_BASE_CLASS_STORAGE		0x01
#define PCIE_CLASS_STORAGE_SCSI		0x0100
#define PCIE_CLASS_STORAGE_IDE		0x0101
#define PCIE_CLASS_STORAGE_FLOPPY	0x0102
#define PCIE_CLASS_STORAGE_IPI		0x0103
#define PCIE_CLASS_STORAGE_RAID		0x0104
#define PCIE_CLASS_STORAGE_OTHER		0x0180

#define PCIE_BASE_CLASS_NETWORK		0x02
#define PCIE_CLASS_NETWORK_ETHERNET	0x0200
#define PCIE_CLASS_NETWORK_TOKEN_RING	0x0201
#define PCIE_CLASS_NETWORK_FDDI		0x0202
#define PCIE_CLASS_NETWORK_ATM		0x0203
#define PCIE_CLASS_NETWORK_OTHER		0x0280

#define PCIE_BASE_CLASS_DISPLAY		0x03
#define PCIE_CLASS_DISPLAY_VGA		0x0300
#define PCIE_CLASS_DISPLAY_XGA		0x0301
#define PCIE_CLASS_DISPLAY_3D		0x0302
#define PCIE_CLASS_DISPLAY_OTHER		0x0380

#define PCIE_BASE_CLASS_MULTIMEDIA	0x04
#define PCIE_CLASS_MULTIMEDIA_VIDEO	0x0400
#define PCIE_CLASS_MULTIMEDIA_AUDIO	0x0401
#define PCIE_CLASS_MULTIMEDIA_PHONE	0x0402
#define PCIE_CLASS_MULTIMEDIA_OTHER	0x0480

#define PCIE_BASE_CLASS_MEMORY		0x05
#define PCIE_CLASS_MEMORY_RAM		0x0500
#define PCIE_CLASS_MEMORY_FLASH		0x0501
#define PCIE_CLASS_MEMORY_OTHER		0x0580

#define PCIE_BASE_CLASS_BRIDGE		0x06
#define PCIE_CLASS_BRIDGE_HOST		0x0600
#define PCIE_CLASS_BRIDGE_ISA		0x0601
#define PCIE_CLASS_BRIDGE_EISA		0x0602
#define PCIE_CLASS_BRIDGE_MC		0x0603
#define PCIE_CLASS_BRIDGE_PCI		0x0604
#define PCIE_CLASS_BRIDGE_PCMCIA		0x0605
#define PCIE_CLASS_BRIDGE_NUBUS		0x0606
#define PCIE_CLASS_BRIDGE_CARDBUS	0x0607
#define PCIE_CLASS_BRIDGE_RACEWAY	0x0608
#define PCIE_CLASS_BRIDGE_OTHER		0x0680

#define PCIE_BASE_CLASS_COMMUNICATION	0x07
#define PCIE_CLASS_COMMUNICATION_SERIAL	0x0700
#define PCIE_CLASS_COMMUNICATION_PARALLEL 0x0701
#define PCIE_CLASS_COMMUNICATION_MULTISERIAL 0x0702
#define PCIE_CLASS_COMMUNICATION_MODEM	0x0703
#define PCIE_CLASS_COMMUNICATION_OTHER	0x0780

#define PCIE_BASE_CLASS_SYSTEM		0x08
#define PCIE_CLASS_SYSTEM_PIC		0x0800
#define PCIE_CLASS_SYSTEM_DMA		0x0801
#define PCIE_CLASS_SYSTEM_TIMER		0x0802
#define PCIE_CLASS_SYSTEM_RTC		0x0803
#define PCIE_CLASS_SYSTEM_PCIE_HOTPLUG	0x0804
#define PCIE_CLASS_SYSTEM_OTHER		0x0880

#define PCIE_BASE_CLASS_INPUT		0x09
#define PCIE_CLASS_INPUT_KEYBOARD	0x0900
#define PCIE_CLASS_INPUT_PEN		0x0901
#define PCIE_CLASS_INPUT_MOUSE		0x0902
#define PCIE_CLASS_INPUT_SCANNER		0x0903
#define PCIE_CLASS_INPUT_GAMEPORT	0x0904
#define PCIE_CLASS_INPUT_OTHER		0x0980

#define PCIE_BASE_CLASS_DOCKING		0x0a
#define PCIE_CLASS_DOCKING_GENERIC	0x0a00
#define PCIE_CLASS_DOCKING_OTHER		0x0a80

#define PCIE_BASE_CLASS_PROCESSOR	0x0b
#define PCIE_CLASS_PROCESSOR_386		0x0b00
#define PCIE_CLASS_PROCESSOR_486		0x0b01
#define PCIE_CLASS_PROCESSOR_PENTIUM	0x0b02
#define PCIE_CLASS_PROCESSOR_ALPHA	0x0b10
#define PCIE_CLASS_PROCESSOR_POWERPC	0x0b20
#define PCIE_CLASS_PROCESSOR_MIPS	0x0b30
#define PCIE_CLASS_PROCESSOR_CO		0x0b40

#define PCIE_BASE_CLASS_SERIAL		0x0c
#define PCIE_CLASS_SERIAL_FIREWIRE	0x0c00
#define PCIE_CLASS_SERIAL_ACCESS		0x0c01
#define PCIE_CLASS_SERIAL_SSA		0x0c02
#define PCIE_CLASS_SERIAL_USB		0x0c03
#define PCIE_CLASS_SERIAL_FIBER		0x0c04
#define PCIE_CLASS_SERIAL_SMBUS		0x0c05

#define PCIE_BASE_CLASS_INTELLIGENT	0x0e
#define PCIE_CLASS_INTELLIGENT_I2O	0x0e00

#define PCIE_BASE_CLASS_SATELLITE	0x0f
#define PCIE_CLASS_SATELLITE_TV		0x0f00
#define PCIE_CLASS_SATELLITE_AUDIO	0x0f01
#define PCIE_CLASS_SATELLITE_VOICE	0x0f03
#define PCIE_CLASS_SATELLITE_DATA	0x0f04

#define PCIE_BASE_CLASS_CRYPT		0x10
#define PCIE_CLASS_CRYPT_NETWORK		0x1000
#define PCIE_CLASS_CRYPT_ENTERTAINMENT	0x1001
#define PCIE_CLASS_CRYPT_OTHER		0x1080

#define PCIE_BASE_CLASS_SIGNAL_PROCESSING 0x11
#define PCIE_CLASS_SP_DPIO		0x1100
#define PCIE_CLASS_SP_OTHER		0x1180

#define PCIE_CLASS_OTHERS		0xff








/*
 * Under PCI, each device has 256 bytes of configuration address space,
 * of which the first 64 bytes are standardized as follows:
 */
#define PCIE_VENDOR_ID		0x00	/* 16 bits */
#define PCIE_DEVICE_ID		0x02	/* 16 bits */
#define PCIE_COMMAND		0x04	/* 16 bits */
#define  PCIE_COMMAND_IO		0x1	/* Enable response in I/O space */
#define  PCIE_COMMAND_MEMORY	0x2	/* Enable response in Memory space */
#define  PCIE_COMMAND_MASTER	0x4	/* Enable bus mastering */
#define  PCIE_COMMAND_SPECIAL	0x8	/* Enable response to special cycles */
#define  PCIE_COMMAND_INVALIDATE 0x10	/* Use memory write and invalidate */
#define  PCIE_COMMAND_VGA_PALETTE 0x20	/* Enable palette snooping */
#define  PCIE_COMMAND_PARITY	0x40	/* Enable parity checking */
#define  PCIE_COMMAND_WAIT	0x80	/* Enable address/data stepping */
#define  PCIE_COMMAND_SERR	0x100	/* Enable SERR */
#define  PCIE_COMMAND_FAST_BACK	0x200	/* Enable back-to-back writes */
#define  PCIE_LTSSM				0x404   /* PCIe link statuss */
#define  PCIE_LTSSM_L0			0x16    /* L0 state */
#define  PCIE_LTSR              0x5E    /* Link status reg */

#define PCIE_STATUS		0x06	/* 16 bits */
#define  PCIE_STATUS_CAP_LIST	0x10	/* Support Capability List */
#define  PCIE_STATUS_66MHZ	0x20	/* Support 66 Mhz PCI 2.1 bus */
#define  PCIE_STATUS_UDF		0x40	/* Support User Definable Features [obsolete] */
#define  PCIE_STATUS_FAST_BACK	0x80	/* Accept fast-back to back */
#define  PCIE_STATUS_PARITY	0x100	/* Detected parity error */
#define  PCIE_STATUS_DEVSEL_MASK 0x600	/* DEVSEL timing */
#define  PCIE_STATUS_DEVSEL_FAST 0x000
#define  PCIE_STATUS_DEVSEL_MEDIUM 0x200
#define  PCIE_STATUS_DEVSEL_SLOW 0x400
#define  PCIE_STATUS_SIG_TARGET_ABORT 0x800 /* Set on target abort */
#define  PCIE_STATUS_REC_TARGET_ABORT 0x1000 /* Master ack of " */
#define  PCIE_STATUS_REC_MASTER_ABORT 0x2000 /* Set on master abort */
#define  PCIE_STATUS_SIG_SYSTEM_ERROR 0x4000 /* Set when we drive SERR */
#define  PCIE_STATUS_DETECTED_PARITY 0x8000 /* Set on parity error */

#define PCIE_CLASS_REVISION	0x08	/* High 24 bits are class, low 8
					   revision */
#define PCIE_REVISION_ID		0x08	/* Revision ID */
#define PCIE_CLASS_PROG		0x09	/* Reg. Level Programming Interface */
#define PCIE_CLASS_DEVICE	0x0a	/* Device class */
#define PCIE_CLASS_CODE		0x0b	/* Device class code */
#define PCIE_CLASS_SUB_CODE	0x0a	/* Device sub-class code */

#define PCIE_CACHE_LINE_SIZE	0x0c	/* 8 bits */
#define PCIE_LATENCY_TIMER	0x0d	/* 8 bits */
#define PCIE_HEADER_TYPE		0x0e	/* 8 bits */
#define  PCIE_HEADER_TYPE_NORMAL 0
#define  PCIE_HEADER_TYPE_BRIDGE 1
#define  PCIE_HEADER_TYPE_CARDBUS 2

#define PCIE_BIST		0x0f	/* 8 bits */
#define PCIE_BIST_CODE_MASK	0x0f	/* Return result */
#define PCIE_BIST_START		0x40	/* 1 to start BIST, 2 secs or less */
#define PCIE_BIST_CAPABLE	0x80	/* 1 if BIST capable */

/*
 * Base addresses specify locations in memory or I/O space.
 * Decoded size can be determined by writing a value of
 * 0xffffffff to the register, and reading it back.  Only
 * 1 bits are decoded.
 */
#define PCIE_BASE_ADDRESS_0	0x10	/* 32 bits */
#define PCIE_BASE_ADDRESS_1	0x14	/* 32 bits [htype 0,1 only] */
#define PCIE_BASE_ADDRESS_2	0x18	/* 32 bits [htype 0 only] */
#define PCIE_BASE_ADDRESS_3	0x1c	/* 32 bits */
#define PCIE_BASE_ADDRESS_4	0x20	/* 32 bits */
#define PCIE_BASE_ADDRESS_5	0x24	/* 32 bits */
#define  PCIE_BASE_ADDRESS_SPACE 0x01	/* 0 = memory, 1 = I/O */
#define  PCIE_BASE_ADDRESS_SPACE_IO 0x01
#define  PCIE_BASE_ADDRESS_SPACE_MEMORY 0x00
#define  PCIE_BASE_ADDRESS_MEM_TYPE_MASK 0x06
#define  PCIE_BASE_ADDRESS_MEM_TYPE_32	0x00	/* 32 bit address */
#define  PCIE_BASE_ADDRESS_MEM_TYPE_1M	0x02	/* Below 1M [obsolete] */
#define  PCIE_BASE_ADDRESS_MEM_TYPE_64	0x04	/* 64 bit address */
#define  PCIE_BASE_ADDRESS_MEM_PREFETCH	0x08	/* prefetchable? */
#define  PCIE_BASE_ADDRESS_MEM_MASK	(~0x0fUL)
#define  PCIE_BASE_ADDRESS_IO_MASK	(~0x03UL)
/* bit 1 is reserved if address_space = 1 */

/* Header type 0 (normal devices) */
#define PCIE_CARDBUS_CIS		0x28
#define PCIE_SUBSYSTEM_VENDOR_ID 0x2c
#define PCIE_SUBSYSTEM_ID	0x2e
#define PCIE_ROM_ADDRESS		0x30	/* Bits 31..11 are address, 10..1 reserved */
#define  PCIE_ROM_ADDRESS_ENABLE 0x01
#define PCIE_ROM_ADDRESS_MASK	(~0x7ffUL)

#define PCIE_CAPABILITY_LIST	0x34	/* Offset of first capability list entry */

/* 0x35-0x3b are reserved */
#define PCIE_INTERRUPT_LINE	0x3c	/* 8 bits */
#define PCIE_INTERRUPT_PIN	0x3d	/* 8 bits */
#define PCIE_MIN_GNT		0x3e	/* 8 bits */
#define PCIE_MAX_LAT		0x3f	/* 8 bits */

/* Header type 1 (PCI-to-PCI bridges) */
#define PCIE_PRIMARY_BUS		0x18	/* Primary bus number */
#define PCIE_SECONDARY_BUS	0x19	/* Secondary bus number */
#define PCIE_SUBORDINATE_BUS	0x1a	/* Highest bus number behind the bridge */
#define PCIE_SEC_LATENCY_TIMER	0x1b	/* Latency timer for secondary interface */
#define PCIE_IO_BASE		0x1c	/* I/O range behind the bridge */
#define PCIE_IO_LIMIT		0x1d
#define  PCIE_IO_RANGE_TYPE_MASK 0x0f	/* I/O bridging type */
#define  PCIE_IO_RANGE_TYPE_16	0x00
#define  PCIE_IO_RANGE_TYPE_32	0x01
#define  PCIE_IO_RANGE_MASK	~0x0f
#define PCIE_SEC_STATUS		0x1e	/* Secondary status register, only bit 14 used */
#define PCIE_MEMORY_BASE		0x20	/* Memory range behind */
#define PCIE_MEMORY_LIMIT	0x22
#define  PCIE_MEMORY_RANGE_TYPE_MASK 0x0f
#define  PCIE_MEMORY_RANGE_MASK	~0x0f
#define PCIE_PREF_MEMORY_BASE	0x24	/* Prefetchable memory range behind */
#define PCIE_PREF_MEMORY_LIMIT	0x26
#define  PCIE_PREF_RANGE_TYPE_MASK 0x0f
#define  PCIE_PREF_RANGE_TYPE_32 0x00
#define  PCIE_PREF_RANGE_TYPE_64 0x01
#define  PCIE_PREF_RANGE_MASK	~0x0f
#define PCIE_PREF_BASE_UPPER32	0x28	/* Upper half of prefetchable memory range */
#define PCIE_PREF_LIMIT_UPPER32	0x2c
#define PCIE_IO_BASE_UPPER16	0x30	/* Upper half of I/O addresses */
#define PCIE_IO_LIMIT_UPPER16	0x32
/* 0x34 same as for htype 0 */
/* 0x35-0x3b is reserved */
#define PCIE_ROM_ADDRESS1	0x38	/* Same as PCIE_ROM_ADDRESS, but for htype 1 */
/* 0x3c-0x3d are same as for htype 0 */
#define PCIE_BRIDGE_CONTROL	0x3e
#define  PCIE_BRIDGE_CTL_PARITY	0x01	/* Enable parity detection on secondary interface */
#define  PCIE_BRIDGE_CTL_SERR	0x02	/* The same for SERR forwarding */
#define  PCIE_BRIDGE_CTL_NO_ISA	0x04	/* Disable bridging of ISA ports */
#define  PCIE_BRIDGE_CTL_VGA	0x08	/* Forward VGA addresses */
#define  PCIE_BRIDGE_CTL_MASTER_ABORT 0x20  /* Report master aborts */
#define  PCIE_BRIDGE_CTL_BUS_RESET 0x40	/* Secondary bus reset */
#define  PCIE_BRIDGE_CTL_FAST_BACK 0x80	/* Fast Back2Back enabled on secondary interface */

/* From 440ep */
#define PCIE_ERREN       0x48     /* Error Enable */
#define PCIE_ERRSTS      0x49     /* Error Status */
#define PCIE_BRDGOPT1    0x4A     /* PCI Bridge Options 1 */
#define PCIE_PLBSESR0    0x4C     /* PCI PLB Slave Error Syndrome 0 */
#define PCIE_PLBSESR1    0x50     /* PCI PLB Slave Error Syndrome 1 */
#define PCIE_PLBSEAR     0x54     /* PCI PLB Slave Error Address */
#define PCIE_CAPID       0x58     /* Capability Identifier */
#define PCIE_NEXTITEMPTR 0x59     /* Next Item Pointer */
#define PCIE_PMC         0x5A     /* Power Management Capabilities */
#define PCIE_PMCSR       0x5C     /* Power Management Control Status */
#define PCIE_PMCSRBSE    0x5E     /* PMCSR PCI to PCI Bridge Support Extensions */
#define PCIE_BRDGOPT2    0x60     /* PCI Bridge Options 2 */
#define PCIE_PMSCRR      0x64     /* Power Management State Change Request Re. */

/* Header type 2 (CardBus bridges) */
#define PCIE_CB_CAPABILITY_LIST	0x14
/* 0x15 reserved */
#define PCIE_CB_SEC_STATUS	0x16	/* Secondary status */
#define PCIE_CB_PRIMARY_BUS	0x18	/* PCI bus number */
#define PCIE_CB_CARD_BUS		0x19	/* CardBus bus number */
#define PCIE_CB_SUBORDINATE_BUS	0x1a	/* Subordinate bus number */
#define PCIE_CB_LATENCY_TIMER	0x1b	/* CardBus latency timer */
#define PCIE_CB_MEMORY_BASE_0	0x1c
#define PCIE_CB_MEMORY_LIMIT_0	0x20
#define PCIE_CB_MEMORY_BASE_1	0x24
#define PCIE_CB_MEMORY_LIMIT_1	0x28
#define PCIE_CB_IO_BASE_0	0x2c
#define PCIE_CB_IO_BASE_0_HI	0x2e
#define PCIE_CB_IO_LIMIT_0	0x30
#define PCIE_CB_IO_LIMIT_0_HI	0x32
#define PCIE_CB_IO_BASE_1	0x34
#define PCIE_CB_IO_BASE_1_HI	0x36
#define PCIE_CB_IO_LIMIT_1	0x38
#define PCIE_CB_IO_LIMIT_1_HI	0x3a
#define  PCIE_CB_IO_RANGE_MASK	~0x03
/* 0x3c-0x3d are same as for htype 0 */
#define PCIE_CB_BRIDGE_CONTROL	0x3e
#define  PCIE_CB_BRIDGE_CTL_PARITY	0x01	/* Similar to standard bridge control register */
#define  PCIE_CB_BRIDGE_CTL_SERR		0x02
#define  PCIE_CB_BRIDGE_CTL_ISA		0x04
#define  PCIE_CB_BRIDGE_CTL_VGA		0x08
#define  PCIE_CB_BRIDGE_CTL_MASTER_ABORT 0x20
#define  PCIE_CB_BRIDGE_CTL_CB_RESET	0x40	/* CardBus reset */
#define  PCIE_CB_BRIDGE_CTL_16BIT_INT	0x80	/* Enable interrupt for 16-bit cards */
#define  PCIE_CB_BRIDGE_CTL_PREFETCH_MEM0 0x100	/* Prefetch enable for both memory regions */
#define  PCIE_CB_BRIDGE_CTL_PREFETCH_MEM1 0x200
#define  PCIE_CB_BRIDGE_CTL_POST_WRITES	0x400
#define PCIE_CB_SUBSYSTEM_VENDOR_ID 0x40
#define PCIE_CB_SUBSYSTEM_ID	0x42
#define PCIE_CB_LEGACY_MODE_BASE 0x44	/* 16-bit PC Card legacy mode base address (ExCa) */
/* 0x48-0x7f reserved */

/* Capability lists */

#define PCIE_CAP_LIST_ID		0	/* Capability ID */
#define  PCIE_CAP_ID_PM		0x01	/* Power Management */
#define  PCIE_CAP_ID_AGP		0x02	/* Accelerated Graphics Port */
#define  PCIE_CAP_ID_VPD		0x03	/* Vital Product Data */
#define  PCIE_CAP_ID_SLOTID	0x04	/* Slot Identification */
#define  PCIE_CAP_ID_MSI		0x05	/* Message Signalled Interrupts */
#define  PCIE_CAP_ID_CHSWP	0x06	/* CompactPCI HotSwap */
#define PCIE_CAP_LIST_NEXT	1	/* Next capability in the list */
#define PCIE_CAP_FLAGS		2	/* Capability defined flags (16 bits) */
#define PCIE_CAP_SIZEOF		4

/* Power Management Registers */

#define  PCIE_PM_CAP_VER_MASK	0x0007	/* Version */
#define  PCIE_PM_CAP_PME_CLOCK	0x0008	/* PME clock required */
#define  PCIE_PM_CAP_AUX_POWER	0x0010	/* Auxilliary power support */
#define  PCIE_PM_CAP_DSI		0x0020	/* Device specific initialization */
#define  PCIE_PM_CAP_D1		0x0200	/* D1 power state support */
#define  PCIE_PM_CAP_D2		0x0400	/* D2 power state support */
#define  PCIE_PM_CAP_PME		0x0800	/* PME pin supported */
#define PCIE_PM_CTRL		4	/* PM control and status register */
#define  PCIE_PM_CTRL_STATE_MASK 0x0003	/* Current power state (D0 to D3) */
#define  PCIE_PM_CTRL_PME_ENABLE 0x0100	/* PME pin enable */
#define  PCIE_PM_CTRL_DATA_SEL_MASK	0x1e00	/* Data select (??) */
#define  PCIE_PM_CTRL_DATA_SCALE_MASK	0x6000	/* Data scale (??) */
#define  PCIE_PM_CTRL_PME_STATUS 0x8000	/* PME pin status */
#define PCIE_PM_PPB_EXTENSIONS	6	/* PPB support extensions (??) */
#define  PCIE_PM_PPB_B2_B3	0x40	/* Stop clock when in D3hot (??) */
#define  PCIE_PM_BPCC_ENABLE	0x80	/* Bus power/clock control enable (??) */
#define PCIE_PM_DATA_REGISTER	7	/* (??) */
#define PCIE_PM_SIZEOF		8

/* AGP registers */

#define PCIE_AGP_VERSION		2	/* BCD version number */
#define PCIE_AGP_RFU		3	/* Rest of capability flags */
#define PCIE_AGP_STATUS		4	/* Status register */
#define  PCIE_AGP_STATUS_RQ_MASK 0xff000000	/* Maximum number of requests - 1 */
#define  PCIE_AGP_STATUS_SBA	0x0200	/* Sideband addressing supported */
#define  PCIE_AGP_STATUS_64BIT	0x0020	/* 64-bit addressing supported */
#define  PCIE_AGP_STATUS_FW	0x0010	/* FW transfers supported */
#define  PCIE_AGP_STATUS_RATE4	0x0004	/* 4x transfer rate supported */
#define  PCIE_AGP_STATUS_RATE2	0x0002	/* 2x transfer rate supported */
#define  PCIE_AGP_STATUS_RATE1	0x0001	/* 1x transfer rate supported */
#define PCIE_AGP_COMMAND		8	/* Control register */
#define  PCIE_AGP_COMMAND_RQ_MASK 0xff000000  /* Master: Maximum number of requests */
#define  PCIE_AGP_COMMAND_SBA	0x0200	/* Sideband addressing enabled */
#define  PCIE_AGP_COMMAND_AGP	0x0100	/* Allow processing of AGP transactions */
#define  PCIE_AGP_COMMAND_64BIT	0x0020	/* Allow processing of 64-bit addresses */
#define  PCIE_AGP_COMMAND_FW	0x0010	/* Force FW transfers */
#define  PCIE_AGP_COMMAND_RATE4	0x0004	/* Use 4x rate */
#define  PCIE_AGP_COMMAND_RATE2	0x0002	/* Use 4x rate */
#define  PCIE_AGP_COMMAND_RATE1	0x0001	/* Use 4x rate */
#define PCIE_AGP_SIZEOF		12

/* Slot Identification */

#define PCIE_SID_ESR		2	/* Expansion Slot Register */
#define  PCIE_SID_ESR_NSLOTS	0x1f	/* Number of expansion slots available */
#define  PCIE_SID_ESR_FIC	0x20	/* First In Chassis Flag */
#define PCIE_SID_CHASSIS_NR	3	/* Chassis Number */

/* Message Signalled Interrupts registers */

#define PCIE_MSI_FLAGS		2	/* Various flags */
#define  PCIE_MSI_FLAGS_64BIT	0x80	/* 64-bit addresses allowed */
#define  PCIE_MSI_FLAGS_QSIZE	0x70	/* Message queue size configured */
#define  PCIE_MSI_FLAGS_QMASK	0x0e	/* Maximum queue size available */
#define  PCIE_MSI_FLAGS_ENABLE	0x01	/* MSI feature enabled */
#define PCIE_MSI_RFU		3	/* Rest of capability flags */
#define PCIE_MSI_ADDRESS_LO	4	/* Lower 32 bits */
#define PCIE_MSI_ADDRESS_HI	8	/* Upper 32 bits (if PCIE_MSI_FLAGS_64BIT set) */
#define PCIE_MSI_DATA_32		8	/* 16 bits of data for 32-bit devices */
#define PCIE_MSI_DATA_64		12	/* 16 bits of data for 64-bit devices */

#define PCIE_MAX_PCIE_DEVICES	32
#define PCIE_MAX_PCIE_FUNCTIONS	8

typedef struct 
	{
		unsigned long long pcie_mem_base;
		unsigned long long pcie_mem_size;
	} PARAM_PCIE;





struct pcie_region {
	unsigned long long bus_start;		/* Start on the bus */
	unsigned long long phys_start;		/* Start in physical address space */
	unsigned long size;			/* Size */
	unsigned long flags;			/* Resource flags */

	unsigned long long bus_lower;
};


#define PCIE_REGION_MEM		0x00000000	/* PCI memory space */
#define PCIE_REGION_IO		0x00000001	/* PCI IO space */
#define PCIE_REGION_TYPE		0x00000001

#define PCIE_REGION_MEMORY	0x00000100	/* System memory */
#define PCIE_REGION_RO		0x00000200	/* Read-only memory */

extern __inline__ void pcie_set_region(struct pcie_region *reg,
				      unsigned long long bus_start,
				      unsigned long long phys_start,
				      unsigned long size,
				      unsigned long flags) {
	reg->bus_start	= bus_start;
	reg->phys_start = phys_start;
	reg->size	= size;
	reg->flags	= flags;
}

typedef int pcie_dev_t;
#define PCIE_BUS(d)	(((d) >> 16) & 0xff)
#define PCIE_DEV(d)	(((d) >> 11) & 0x1f)
#define PCIE_FUNC(d)	(((d) >> 8) & 0x7)
#define PCIE_BDF(b,d,f)	((b) << 16 | (d) << 11 | (f) << 8)

#define PCIE_ANY_ID (~0)

/*
 * Structure of a PCI controller (host bridge)
 */
#define MAX_PCIE_REGIONS		7

struct pcie_device_id {
	unsigned int vendor;
        unsigned int device;	/* Vendor and device ID or PCIE_ANY_ID */
};

struct pcie_controller {
	struct pcie_controller *next;

	int first_busno;
	int last_busno;
	struct pcie_region regions[MAX_PCIE_REGIONS];
	int region_count;
	int current_busno;

	volatile unsigned int *cfg_addr;
	volatile unsigned char *cfg_data;
/* Used by auto config */
	struct pcie_region *pcie_mem, *pcie_io;


	/* Low-level architecture-dependent routines */
	int (*read_byte)(struct pcie_controller*, pcie_dev_t, int where, unsigned char *);
	int (*read_word)(struct pcie_controller*, pcie_dev_t, int where, unsigned short *);
	int (*read_dword)(struct pcie_controller*, pcie_dev_t, int where, unsigned int *);
	int (*write_byte)(struct pcie_controller*, pcie_dev_t, int where, unsigned char);
	int (*write_word)(struct pcie_controller*, pcie_dev_t, int where, unsigned short);
	int (*write_dword)(struct pcie_controller*, pcie_dev_t, int where, unsigned int);

};

extern __inline__ void pcie_set_ops(struct pcie_controller *hose,
				   int (*read_byte)(struct pcie_controller*,
						    pcie_dev_t, int where, unsigned char *),
				   int (*read_word)(struct pcie_controller*,
						    pcie_dev_t, int where, unsigned short *),
				   int (*read_dword)(struct pcie_controller*,
						     pcie_dev_t, int where, unsigned int *),
				   int (*write_byte)(struct pcie_controller*,
						     pcie_dev_t, int where, unsigned char),
				   int (*write_word)(struct pcie_controller*,
						     pcie_dev_t, int where, unsigned short),
				   int (*write_dword)(struct pcie_controller*,
						      pcie_dev_t, int where, unsigned int)) {
	hose->read_byte   = read_byte;
	hose->read_word   = read_word;
	hose->read_dword  = read_dword;
	hose->write_byte  = write_byte;
	hose->write_word  = write_word;
	hose->write_dword = write_dword;
}
extern int pcie_hose_read_config_byte(struct pcie_controller *hose,
				     pcie_dev_t dev, int where, unsigned char *val);
extern int pcie_hose_read_config_word(struct pcie_controller *hose,
				     pcie_dev_t dev, int where, unsigned short *val);
extern int pcie_hose_read_config_dword(struct pcie_controller *hose,
				      pcie_dev_t dev, int where, unsigned int *val);
extern int pcie_hose_write_config_byte(struct pcie_controller *hose,
				      pcie_dev_t dev, int where, unsigned char val);
extern int pcie_hose_write_config_word(struct pcie_controller *hose,
				      pcie_dev_t dev, int where, unsigned short val);
extern int pcie_hose_write_config_dword(struct pcie_controller *hose,
				       pcie_dev_t dev, int where, unsigned int val);

extern int pcie_read_config_byte(pcie_dev_t dev, int where, unsigned char *val);
extern int pcie_read_config_word(pcie_dev_t dev, int where, unsigned short *val);
extern int pcie_read_config_dword(pcie_dev_t dev, int where, unsigned int *val);
extern int pcie_write_config_byte(pcie_dev_t dev, int where, unsigned char val);
extern int pcie_write_config_word(pcie_dev_t dev, int where, unsigned short val);
extern int pcie_write_config_dword(pcie_dev_t dev, int where, unsigned int val);

extern int pcie_hose_read_config_byte_via_dword(struct pcie_controller *hose,
					       pcie_dev_t dev, int where, unsigned char *val);
extern int pcie_hose_read_config_word_via_dword(struct pcie_controller *hose,
					       pcie_dev_t dev, int where, unsigned short *val);
extern int pcie_hose_write_config_byte_via_dword(struct pcie_controller *hose,
						pcie_dev_t dev, int where, unsigned char val);
extern int pcie_hose_write_config_word_via_dword(struct pcie_controller *hose,
						pcie_dev_t dev, int where, unsigned short val);

extern void pcie_register_hose(struct pcie_controller* hose);
extern struct pcie_controller* pcie_bus_to_hose(int bus);

extern int pcie_hose_scan(struct pcie_controller *hose);
extern int pcie_hose_scan_bus(struct pcie_controller *hose, int bus);

void pcie_auto_region_init(struct pcie_region* res);
void pcie_auto_config_init(struct pcie_controller *hose);
int pcie_auto_config_device(struct pcie_controller *hose, pcie_dev_t dev);
void pcie_auto_setup_device(struct pcie_controller *hose,
			  pcie_dev_t dev, int bars_num,
			  struct pcie_region *mem,
			  struct pcie_region *io);


int pcie_init( void );
pcie_dev_t pcie_find_devices(struct pcie_device_id *ids, int index);
pcie_dev_t pcie_find_device(unsigned int vendor, unsigned int device, int index);

#ifdef CONFIG_MPC8548
void pcie_mpc85xx_init(struct pcie_controller *hose, PARAM_PCIE *pcie_info);
#endif

#endif	/* _PCIE_H */
