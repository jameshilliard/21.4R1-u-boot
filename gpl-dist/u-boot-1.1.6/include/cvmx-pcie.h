/***********************license start***************
 * Copyright (c) 2003-2008  Cavium Networks (support@cavium.com). All rights
 * reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Cavium Networks nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * This Software, including technical data, may be subject to U.S.  export
 * control laws, including the U.S.  Export Administration Act and its
 * associated regulations, and may be subject to export or import regulations
 * in other countries.  You warrant that You will comply strictly in all
 * respects with all such regulations and acknowledge that you have the
 * responsibility to obtain licenses to export, re-export or import the
 * Software.
 *
 * TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 * AND WITH ALL FAULTS AND CAVIUM NETWORKS MAKES NO PROMISES, REPRESENTATIONS
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 * RESPECT TO THE SOFTWARE, INCLUDING ITS CONDITION, ITS CONFORMITY TO ANY
 * REPRESENTATION OR DESCRIPTION, OR THE EXISTENCE OF ANY LATENT OR PATENT
 * DEFECTS, AND CAVIUM SPECIFICALLY DISCLAIMS ALL IMPLIED (IF ANY) WARRANTIES
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR
 * PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET
 * POSSESSION OR CORRESPONDENCE TO DESCRIPTION.  THE ENTIRE RISK ARISING OUT
 * OF USE OR PERFORMANCE OF THE SOFTWARE LIES WITH YOU.
 *
 *
 * For any questions regarding licensing please contact marketing@caviumnetworks.com
 *
 ***********************license end**************************************/






/**
 * @file
 *
 * Interface to PCIe as a host(RC) or target(EP)
 *
 * <hr>$Revision: 1.2 $<hr>
 */

#ifndef __CVMX_PCIE_H__
#define __CVMX_PCIE_H__

#define CVMX_PCIE_BAR1_RC_BASE (1ull << 41)
#define CVMX_PCIE_BAR1_PHYS_BASE ((1ull << 32) - (1ull << 28))

typedef union
{
    uint64_t    u64;
    struct
    {
        uint64_t    upper           : 2;    /* Normally 2 for XKPHYS */
        uint64_t    reserved_49_61  : 13;   /* Must be zero */
        uint64_t    io              : 1;    /* 1 for IO space access */
        uint64_t    did             : 5;    /* PCIe DID = 3 */
        uint64_t    subdid          : 3;    /* PCIe SubDID = 1 */
        uint64_t    reserved_36_39  : 4;    /* Must be zero */
        uint64_t    es              : 2;    /* Endian swap = 1 */
        uint64_t    port            : 2;    /* PCIe port 0,1 */
        uint64_t    reserved_29_31  : 3;    /* Must be zero */
        uint64_t    ty              : 1;    /* Selects the type of the configuration request (0 = type 0, 1 = type 1). */
        uint64_t    bus             : 8;    /* Target bus number sent in the ID in the request. */
        uint64_t    dev             : 5;    /* Target device number sent in the ID in the request. Note that Dev must be
                                                zero for type 0 configuration requests. */
        uint64_t    func            : 3;    /* Target function number sent in the ID in the request. */
        uint64_t    reg             : 12;   /* Selects a register in the configuration space of the target. */
    } config;
    struct
    {
        uint64_t    upper           : 2;    /* Normally 2 for XKPHYS */
        uint64_t    reserved_49_61  : 13;   /* Must be zero */
        uint64_t    io              : 1;    /* 1 for IO space access */
        uint64_t    did             : 5;    /* PCIe DID = 3 */
        uint64_t    subdid          : 3;    /* PCIe SubDID = 2 */
        uint64_t    reserved_36_39  : 4;    /* Must be zero */
        uint64_t    es              : 2;    /* Endian swap = 1 */
        uint64_t    port            : 2;    /* PCIe port 0,1 */
        uint64_t    address         : 32;   /* PCIe IO address */
    } io;
    struct
    {
        uint64_t    upper           : 2;    /* Normally 2 for XKPHYS */
        uint64_t    reserved_49_61  : 13;   /* Must be zero */
        uint64_t    io              : 1;    /* 1 for IO space access */
        uint64_t    did             : 5;    /* PCIe DID = 3 */
        uint64_t    subdid          : 3;    /* PCIe SubDID = 3-6 */
        uint64_t    reserved_36_39  : 4;    /* Must be zero */
        uint64_t    address         : 36;   /* PCIe Mem address */
    } mem;
} cvmx_pcie_address_t;


/**
 * Return the Core virtual base address for PCIe IO access. IOs are
 * read/written as an offset from this address.
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return 64bit Octeon IO base address for read/write
 */
uint64_t cvmx_pcie_get_io_base_address(int pcie_port);

/**
 * Size of the IO address region returned at address
 * cvmx_pcie_get_io_base_address()
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return Size of the IO window
 */
uint64_t cvmx_pcie_get_io_size(int pcie_port);

/**
 * Return the Core virtual base address for PCIe MEM access. Memory is
 * read/written as an offset from this address.
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return 64bit Octeon IO base address for read/write
 */
uint64_t cvmx_pcie_get_mem_base_address(int pcie_port);

/**
 * Size of the Mem address region returned at address
 * cvmx_pcie_get_mem_base_address()
 *
 * @param pcie_port PCIe port the IO is for
 *
 * @return Size of the Mem window
 */
uint64_t cvmx_pcie_get_mem_size(int pcie_port);

/**
 * Initialize a PCIe port for use in host(RC) mode. It doesn't enumerate the bus.
 *
 * @param pcie_port PCIe port to initialize
 *
 * @return Zero on success
 */
int cvmx_pcie_rc_initialize(int pcie_port);

/**
 * Shutdown a PCIe port and put it in reset
 *
 * @param pcie_port PCIe port to shutdown
 *
 * @return Zero on success
 */
int cvmx_pcie_rc_shutdown(int pcie_port);

/**
 * Read 8bits from a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 *
 * @return Result of the read
 */
uint8_t cvmx_pcie_config_read8(int pcie_port, int bus, int dev, int fn, int reg);

/**
 * Read 16bits from a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 *
 * @return Result of the read
 */
uint16_t cvmx_pcie_config_read16(int pcie_port, int bus, int dev, int fn, int reg);

/**
 * Read 32bits from a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 *
 * @return Result of the read
 */
uint32_t cvmx_pcie_config_read32(int pcie_port, int bus, int dev, int fn, int reg);

/**
 * Write 8bits to a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 * @param val       Value to write
 */
void cvmx_pcie_config_write8(int pcie_port, int bus, int dev, int fn, int reg, uint8_t val);

/**
 * Write 16bits to a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 * @param val       Value to write
 */
void cvmx_pcie_config_write16(int pcie_port, int bus, int dev, int fn, int reg, uint16_t val);

/**
 * Write 32bits to a Device's config space
 *
 * @param pcie_port PCIe port the device is on
 * @param bus       Sub bus
 * @param dev       Device ID
 * @param fn        Device sub function
 * @param reg       Register to access
 * @param val       Value to write
 */
void cvmx_pcie_config_write32(int pcie_port, int bus, int dev, int fn, int reg, uint32_t val);

/**
 * Read a PCIe config space register indirectly. This is used for
 * registers of the form PCIEEP_CFG??? and PCIERC?_CFG???.
 *
 * @param pcie_port  PCIe port to read from
 * @param cfg_offset Address to read
 *
 * @return Value read
 */
uint32_t cvmx_pcie_cfgx_read(int pcie_port, uint32_t cfg_offset);

/**
 * Write a PCIe config space register indirectly. This is used for
 * registers of the form PCIEEP_CFG??? and PCIERC?_CFG???.
 *
 * @param pcie_port  PCIe port to write to
 * @param cfg_offset Address to write
 * @param val        Value to write
 */
void cvmx_pcie_cfgx_write(int pcie_port, uint32_t cfg_offset, uint32_t val);

/**
 * Write a 32bit value to the Octeon NPEI register space
 *
 * @param address Address to write to
 * @param val     Value to write
 */
static inline void cvmx_pcie_npei_write32(uint64_t address, uint32_t val)
{
	cvmx_write64_uint32(address ^ 4, val);
	cvmx_read64_uint32(address ^ 4);
}

/**
 * Read a 32bit value from the Octeon NPEI register space
 *
 * @param address Address to read
 * @return The result
 */
static inline uint32_t cvmx_pcie_npei_read32(uint64_t address)
{
	return cvmx_read64_uint32(address ^ 4);
}

#define CONFIG_PRIMARY_BUS      0x18
#define CONFIG_SECONDARY_BUS    0x19
#define CONFIG_SUBORDINATE_BUS  0x1a
#define PCIM_HDRTYPE          0x7f
#define PCI_MAXHDRTYPE        2
#define PCIM_MFDEV            0x80
#define PCI_FUNCMAX   7
#define CONFIG_VENDOR_DEVICE_ID 0x0
#define CONFIG_CLASS_CODE_HIGH  0xb

/* config registers for header type 0 devices */

#define PCIR_VENDOR   0x0
#define PCIR_DEVICE   0x2
#define PCIR_MAPS     0x10
#define PCIR_BARS     PCIR_MAPS
#define PCIR_BAR(x)     (PCIR_BARS + (x) * 4)
#define PCIM_CMD_PORTEN               0x0001
#define PCIM_CMD_MEMEN                0x0002
#define PCIM_CMD_BUSMASTEREN  0x0004
#define PCIR_COMMAND  0x04
#define PCIM_CMD_PORTEN               0x0001
#define PCIM_CMD_MEMEN                0x0002

#define PCIR_PRIBUS_1 0x18
#define PCIR_SECBUS_1 0x19
#define PCIR_SUBBUS_1 0x1a
#define PCIR_SECLAT_1 0x1b


#define PCIR_IOBASEL_1        0x1c
#define PCIR_IOLIMITL_1       0x1d
#define PCIR_IOBASEH_1        0x30
#define PCIR_IOLIMITH_1       0x32

#define PCIR_MEMBASE_1        0x20
#define PCIR_MEMLIMIT_1       0x22

#define PCIR_PMBASEL_1        0x24
#define PCIR_PMLIMITL_1       0x26
#define PCIR_PMBASEH_1        0x28
#define PCIR_PMLIMITH_1       0x2c

/*
 * Bus address and size types
 */
typedef uint64_t bus_addr_t;
typedef uint32_t bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef       uint32_t bus_space_tag_t;
typedef       uint64_t bus_space_handle_t;

typedef struct b4_ {
    unsigned char uc[4];
} b4;

typedef struct us_2_ {
    unsigned short us[2];
} us_2;

typedef union ata_prb_entry_ {
    unsigned long wd;
    us_2          u16;
    b4            u8;
} ata_prb_entry;

typedef struct ata_prb_ {
    ata_prb_entry  entry[16];
} ata_prb;

typedef struct sg_ {
    unsigned long addr_low;
    unsigned long addr_high;
    unsigned long data_cnt;
    unsigned long flags;
} sg_t;

typedef struct sg_entry_ {
    sg_t  sg[4];
} sg_entry;

#define CVMX_IO_SEG CVMX_MIPS_SPACE_XKPHYS

/* These macros simplify the process of creating common IO addresses */
#define CVMX_ADD_SEG(segment, add)          ((((uint64_t)segment) << 62) | (add))
#ifndef CVMX_ADD_IO_SEG
#define CVMX_ADD_IO_SEG(add)                CVMX_ADD_SEG(CVMX_IO_SEG, (add))
#endif

#define MPS_CN5XXX 0 
#define MPS_CN6XXX 0 
#define MRRS_CN5XXX 0 
#define MRRS_CN6XXX 3

uint64_t sc_io_poffset = 0;
uint64_t sc_mem_poffset = 0;

#define MAX_NUM_PCIE 2
#define MAX_NUM_BUSSES 16
#define MAX_NUM_DEVICES 32
#define MAX_NUM_FUNCS 8
#define CONFIG_CLASS_BRIDGE     0x6

#define PROTOCOL_READ  0x8
#define SATA_FIS       0x27
#define ATA_CMD_READ   0x20

/*
 * PCI-E switch device/vendor ids
 * 
 */
#define PEX_VENDOR_ID       0x10b5
#define PEX_8509_DEVICE_ID  0x8509
#define PEX_8614_DEVICE_ID  0x8614

#define CONFIG_VENDOR_DEVICE_ID 0x0
#define CONFIG_CLASS_CODE_HIGH  0xb
#define CONFIG_PCI_HEADER_TYPE  0xe
#define CONFIG_PRIMARY_BUS      0x18
#define CONFIG_SECONDARY_BUS    0x19
#define CONFIG_SUBORDINATE_BUS  0x1a

#define CONFIG_CLASS_BRIDGE     0x6

static inline uint16_t cvmx_swap16(uint16_t x)
{
    return ((uint16_t)((((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |
                       (((uint16_t)(x) & (uint16_t)0xff00U) >> 8) ));
}

/**
 * Byte swap a 32 bit number
 *
 * @param x      32 bit number
 * @return Byte swapped result
 */
static inline uint32_t cvmx_swap32(uint32_t x)
{
    return ((uint32_t)((((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |
                       (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |
                       (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |
                       (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ));
}

/**
 * Byte swap a 64 bit number
 *
 * @param x      64 bit number
 * @return Byte swapped result
 */
static inline uint64_t cvmx_swap64(uint64_t x)
{
    return ((x >> 56) |
            (((x >> 48) & 0xfful) << 8) |
            (((x >> 40) & 0xfful) << 16) |
            (((x >> 32) & 0xfful) << 24) |
            (((x >> 24) & 0xfful) << 32) |
            (((x >> 16) & 0xfful) << 40) |
            (((x >>  8) & 0xfful) << 48) |
            (((x >>  0) & 0xfful) << 56));
}

#if __BYTE_ORDER == __BIG_ENDIAN
 
#define cvmx_cpu_to_le16(x) cvmx_swap16(x)
#define cvmx_cpu_to_le32(x) cvmx_swap32(x)
#define cvmx_cpu_to_le64(x) cvmx_swap64(x)
 
#define cvmx_cpu_to_be16(x) (x)
#define cvmx_cpu_to_be32(x) (x)
#define cvmx_cpu_to_be64(x) (x)

#else

#define cvmx_cpu_to_le16(x) (x)
#define cvmx_cpu_to_le32(x) (x)
#define cvmx_cpu_to_le64(x) (x)

#define cvmx_cpu_to_be16(x) cvmx_swap16(x)
#define cvmx_cpu_to_be32(x) cvmx_swap32(x)
#define cvmx_cpu_to_be64(x) cvmx_swap64(x)

#endif

#define cvmx_le16_to_cpu(x) cvmx_cpu_to_le16(x)
#define cvmx_le32_to_cpu(x) cvmx_cpu_to_le32(x)
#define cvmx_le64_to_cpu(x) cvmx_cpu_to_le64(x)

#define cvmx_be16_to_cpu(x) cvmx_cpu_to_be16(x)
#define cvmx_be32_to_cpu(x) cvmx_cpu_to_be32(x)
#define cvmx_be64_to_cpu(x) cvmx_cpu_to_be64(x)

#endif

