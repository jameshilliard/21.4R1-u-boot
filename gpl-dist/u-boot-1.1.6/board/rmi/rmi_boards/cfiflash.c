/***********************************************************************
Copyright 2003-2006 Raza Microelectronics, Inc.(RMI). All rights
reserved.
Use of this software shall be governed in all respects by the terms and
conditions of the RMI software license agreement ("SLA") that was
accepted by the user as a condition to opening the attached files.
Without limiting the foregoing, use of this software in source and
binary code forms, with or without modification, and subject to all
other SLA terms and conditions, is permitted.
Any transfer or redistribution of the source code, with or without
modification, IS PROHIBITED,unless specifically allowed by the SLA.
Any transfer or redistribution of the binary code, with or without
modification, is permitted, provided that the following condition is
met:
Redistributions in binary form must reproduce the above copyright
notice, the SLA, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution:
THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. `AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RMI OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*****************************#RMI_3#***********************************/

#include "rmi/byteorder.h"
#include "rmi/flashdev.h"
#include "rmi/cfiflash.h"
#include "rmi/cpld.h"
#include "rmi/system.h"
#include <common.h>


#ifdef __MIPSEB__
#define CFI_SWAP16(x) __swab16(x)
#else
#define CFI_SWAP16(x) (x)
#endif

#define CFI_TIMEOUT 0xffffff

struct cfiflash;

struct algorithm
{
	char *name;
	int (*reset) (struct cfiflash *this_cfi);
	int (*check_status) (struct cfiflash *this_cfi);

	int (*lock_block) (struct cfiflash *this_cfi, unsigned long addr,
			unsigned char lock);
	int (*block_erase)     (struct cfiflash *this_cfi, unsigned long addr);
	int (*block_write)     (struct cfiflash *this_cfi, unsigned long addr,
			  unsigned char *val);
	int (*block_write_buf) (struct cfiflash *this_cfi, unsigned long addr,
				unsigned char *buf);
};


static int intel_reset(struct cfiflash *this_cfi);
static int intel_check_status(struct cfiflash *this_cfi);
static int intel_lock_block(struct cfiflash *this_cfi, unsigned long addr,unsigned char lock);
static int intel_block_erase(struct cfiflash *this_cfi, unsigned long addr);
static int intel_block_write(struct cfiflash *this_cfi, unsigned long addr,unsigned char *val);
static int intel_block_write_buf(struct cfiflash *this_cfi, unsigned long addr,unsigned char *buf);

static int amd_reset(struct cfiflash *this_cfi);
static int amd_block_erase(struct cfiflash *this_cfi, unsigned long addr);
static int amd_block_write(struct cfiflash *this_cfi, unsigned long addr,unsigned char *val);

static struct algorithm algos[] = 
{
    {
        .name              = "Intel/Sharp Ext.",
        .reset             = intel_reset,
        .check_status      = intel_check_status,
        .lock_block        = intel_lock_block,
        .block_erase       = intel_block_erase,
        .block_write       = intel_block_write,
        .block_write_buf   = intel_block_write_buf
    },
    {
        .name              = "AMD/Fujitsu",
        .reset             = amd_reset,
        .block_erase       = amd_block_erase,
        .block_write       = amd_block_write
    },
    {}
};

/* Intel Algorithm.
 *
 * The Status Register may be read to determine when a block erase,
 * program, or lock-bit configuration is complete and whether the
 * operation completed successfully. It may be read only after the
 * specified time W12 (see Table9, Write Operations on page 25).
 * After writing this command, all subsequent read operations output
 * data from the Status Register until another valid command is written.
*/
static int intel_reset(struct cfiflash *this_cfi)
{
    unsigned long iobase    = this_cfi->iobase;
    phoenix_outw(iobase,CFI_SWAP16(0xff));
    udelay(1000);

    return 0;
}

static int intel_check_status(struct cfiflash *this_cfi)
{
    unsigned long iobase    = this_cfi->iobase;
    unsigned short status   = 0;
    status = CFI_SWAP16(phoenix_inw(iobase));
    if(status & ~0x80)
    {
        printf("Operation might have failed. Status 0x%x\n",status);
    }

    phoenix_outw(iobase, CFI_SWAP16(0x50)); // Clear Status
    return (status & ~0x80);
}

static int intel_lock_block(struct cfiflash *this_cfi, unsigned long addr,unsigned char lock)
{
    volatile unsigned short status = 0;
    int count = 0;
    phoenix_outw(addr, CFI_SWAP16(0x50));
    phoenix_outw(addr, CFI_SWAP16(0x60));

    if(lock)
        phoenix_outw(addr, CFI_SWAP16(0x01));
    else
        phoenix_outw(addr, CFI_SWAP16(0xd0));

    for(count = 0; count< CFI_TIMEOUT; count++)
    {
        status = CFI_SWAP16(phoenix_inw(addr));
        if(status & 0x80)
            break;
    }
    if(count == CFI_TIMEOUT)
    {
        printf("%s :Timedout. address 0x%08x status: %x\n",__FUNCTION__,addr,status);
        return -1;
    }
    return 0;
}

/*
 * Block Erase           [Write BusAdd 0x20] [Write BusAdd 0xD0]
 * Check Status
 * Move to ReadArry Mode [ Write BusAdd 0xff]
 */
static int intel_block_erase(struct cfiflash *this_cfi, unsigned long addr)
{
    volatile unsigned short status = 0;
    int count = 0;
    phoenix_outw(addr, CFI_SWAP16(0x50)); // Clear Status Reg.
    phoenix_outw(addr, CFI_SWAP16(0x20));
    phoenix_outw(addr, CFI_SWAP16(0xd0));

    for(count = 0; count <CFI_TIMEOUT;count++)
    {
        status = CFI_SWAP16(phoenix_inw(addr));
        if(status & 0x80)
        {	
#ifdef CF_DEBUG
            printf("Erase completed in %d count. status: %x\n",
                    count,status);
#endif
            break;
        }
    }

    if(count == CFI_TIMEOUT)
    {
        printf("%s :Timedout. address 0x%08x status: %x\n",__FUNCTION__,addr,status);
        return -1;
    }
    return 0;
}

/*
 *  [Write X 0x40 or 0x10] [Write PA PD]
 */
static int intel_block_write(struct cfiflash *this_cfi, unsigned long addr,unsigned char *val)
{
    volatile unsigned short *faddr = (unsigned short *)addr;
    volatile unsigned short *fdata = (unsigned short *)val;
    volatile unsigned short status = 0;
    int count = 0;
    phoenix_outw(addr, CFI_SWAP16(0x50)); // Clear Status
    phoenix_outw(addr, CFI_SWAP16(0x40));
    phoenix_outw((unsigned long)faddr,(unsigned short)*fdata);

    for(count = 0; count <CFI_TIMEOUT;count++)
    {
        status = CFI_SWAP16(phoenix_inw(addr));
        if(status & 0x80)
        {	
#ifdef CF_DEBUG
            printf("Program completed in %d count. address: %p fdata %x status: %x\n",
                    count,faddr,*fdata,status);
#endif
            break;
        }
    }

    if(count == CFI_TIMEOUT)
    {
        printf("%s :Timedout. status %x addr %p fdata %x\n",__FUNCTION__,status,faddr,*fdata);
        return -1;
    }
    return 0;
}

/*
 * [Write BA 0xE8] [Write BA N]
 * After the Write to Buffer command is issued check the XSR to 
 * make sure a buffer is available for writing.
 * The number of bytes/words to be written to the Write Buffer = N + 1,
 * where N = byte/word count argument. Count ranges on this device
 * for byte mode are N = 00H to N = 1FH and for word mode are N = 0x00
 * to N = 0x0F. The third and consecutive bus cycles, as determined by N,
 * are for writing data into the Write Buffer. The Confirm command (0xD0)
 * is expected after exactly N + 1 write cycles; any other command at that
 * point in the sequence aborts the write to buffer operation.
 * The write to buffer or erase operation does not begin until a 
 * Confirm command (0xD0) is issued.
 */
static int intel_block_write_buf(struct cfiflash *this_cfi, 
        unsigned long addr,
        unsigned char *buf)
{
    volatile unsigned short *faddr = (unsigned short *)addr;
    volatile unsigned short *fdata = (unsigned short *)buf;
    int nWords = this_cfi->wbs/2;
    volatile unsigned short status = 0;
    int count = 0;
    phoenix_outw(addr, CFI_SWAP16(0x50)); // Clear Status

    phoenix_outw(addr, CFI_SWAP16(0xe8));

    for(count = 0; count <CFI_TIMEOUT;count++)
    {
        status = CFI_SWAP16(phoenix_inw(addr));
        if(status & 0x80)
        {	
#ifdef CF_DEBUG
            printf("Pre-Write completed in %d count. status: %x\n",
                    count,status);
#endif
            break;
        }
    }
    if(count == CFI_TIMEOUT)
    {
        printf("%s :Pre-Write Timedout. address 0x%08x status: %x\n",__FUNCTION__,addr,status);
        return -1;
    }
    phoenix_outw(addr, CFI_SWAP16((nWords-1)));
    while(nWords --)
    {
        phoenix_outw((unsigned long)faddr,(unsigned short)*fdata);
        faddr++;
        fdata++;
    }
    phoenix_outw(addr, CFI_SWAP16(0xd0));
    for(count = 0; count <CFI_TIMEOUT;count++)
    {
        status = CFI_SWAP16(phoenix_inw(addr));
        if(status & 0x80)
        {	
#ifdef CF_DEBUG
            printf("Status check completed in %d count. status: %x\n",
                    count,status);
#endif
            break;
        }
    }
    if(count == CFI_TIMEOUT)
    {
        printf("%s :Timedout. address 0x%08x status: %x\n",__FUNCTION__,addr,status);
        return -1;
    }
    return 0;
}
/* End of Intel Algorithm 
 */

/* AMD Algorithm 
 */
static int amd_reset(struct cfiflash *this_cfi)
{
    unsigned long iobase    = this_cfi->iobase;
    phoenix_outw(iobase,CFI_SWAP16(0xf0));
    udelay(1000);
    return 0;
}

/* 
 * Sector Erase [555 AA] [2AA 55] [555 80] [555 AA] [2AA 55] [SA 30]
 */
static int amd_block_erase(struct cfiflash *this_cfi, unsigned long addr)
{
    unsigned long iobase = this_cfi->iobase;
    volatile unsigned short status = 0;
    volatile unsigned short old_status = 0;
    int count = 0;

    phoenix_outw( (iobase |(0x555<<1)), CFI_SWAP16(0xAA));
    phoenix_outw( (iobase |(0x2AA<<1)), CFI_SWAP16(0x55));

    phoenix_outw( (iobase |(0x555<<1)), CFI_SWAP16(0x80));

    phoenix_outw( (iobase |(0x555<<1)), CFI_SWAP16(0xAA));
    phoenix_outw( (iobase |(0x2AA<<1)), CFI_SWAP16(0x55));

    phoenix_outw(addr, CFI_SWAP16(0x30));

    old_status = CFI_SWAP16(phoenix_inw((unsigned long)addr));
    for(count = 0; count <CFI_TIMEOUT;count++)
    {
        status = CFI_SWAP16(phoenix_inw(addr));
        if((status & 0x40) == (old_status & 0x40))
        {
#ifdef CF_DEBUG
            printf("Erase completed in %d count. status: %x\n",
                    count,status);
#endif
            break;
        }
        else
        {
            old_status = status;
        }
    }
    if(count == CFI_TIMEOUT)
    {
        printf("%s :Timedout. address 0x%08x status: %x\n",__FUNCTION__,addr,status);
        return -1;
    }
    return 0;
}

/*
 * Program [555 AA] [2AA 55] [555 A0] [PA PD]
 */
static int amd_block_write(struct cfiflash *this_cfi, unsigned long addr,unsigned char *val)
{
    unsigned long iobase = this_cfi->iobase;
    volatile unsigned short *faddr = (unsigned short *)addr;
    volatile unsigned short *fdata = (unsigned short *)val;

    volatile unsigned short status = 0;
    volatile unsigned short old_status = 0;
    int count = 0;

    phoenix_outw( (iobase | (0x555 <<1)), CFI_SWAP16(0xAA));
    phoenix_outw( (iobase | (0x2AA <<1)), CFI_SWAP16(0x55));
    phoenix_outw( (iobase | (0x555<<1)), CFI_SWAP16(0xA0));

    phoenix_outw( (unsigned long)faddr, (volatile unsigned short)*fdata);

    old_status = CFI_SWAP16(phoenix_inw((unsigned long)faddr));
    for(count = 0; count <CFI_TIMEOUT;count++)
    {
        status = CFI_SWAP16(phoenix_inw((unsigned long)faddr));
        if((status & 0x40) == (old_status & 0x40))
        {
#ifdef CF_DEBUG
            printf("Program completed in %d count. address: %p fdata %x status: %x\n",
                    count,faddr,*fdata,status);
#endif
            break;
        }
        else
        {
            old_status = status;
        }
    }
    if(count == CFI_TIMEOUT)
    {
        printf("%s :Timedout. status %x addr %p fdata %x\n",__FUNCTION__,status,faddr,*fdata);
        return -1;
    }

    return 0;
}

static int cfi_flash_reset(struct flash_device *this_flash)
{
    struct cfiflash *this_cfi = (struct cfiflash *)this_flash->priv;
    struct algorithm *algo = this_cfi->algo;
    algo->reset(this_cfi);
    return 0;
}

static int cfi_flash_erase(struct flash_device *this_flash, unsigned long address, 
        unsigned long nBytes)
{
    struct cfiflash *this_cfi = (struct cfiflash *)this_flash->priv;
    unsigned long block_size = (this_cfi->size)/(this_cfi->nr_blocks);
    unsigned long nblocks  = (nBytes/block_size) + ( (nBytes%block_size)>0 ? 1:0);
    struct algorithm *algo = this_cfi->algo;

    if(address % block_size)
    {
        printf("%s Address 0x%08x is not block aligned. Cannot continue...\n",
                __FUNCTION__,address);
        return -1;
    }

    while(nblocks--)
    {
        printf("Erasing 0x%08x...\n",address);
        algo->block_erase(this_cfi,address);
        address += block_size;
    }

    if(algo->check_status)
        algo->check_status(this_cfi);
    algo->reset(this_cfi);
    printf("Done.\n");
    return 0;

}

static int cfi_flash_write(struct flash_device *this_flash, unsigned long address, 
        unsigned char *src, unsigned long nBytes)
{
    struct cfiflash *this_cfi = (struct cfiflash *)this_flash->priv;
    unsigned long wbs      = this_cfi->wbs;
    unsigned long block_size = (this_cfi->size)/(this_cfi->nr_blocks);
    struct algorithm *algo = this_cfi->algo;

    if(address % block_size)
    {
        printf("%s Address 0x%08x is not block aligned. Cannot continue...\n",
                __FUNCTION__,address);
        return -1;
    }
    if(wbs && algo->block_write_buf)
    {
        unsigned long nwbs     = (nBytes/wbs) + ( (nBytes%wbs)>0 ? 1: 0);
        while(nwbs --)
        {
            if((address % (block_size/4)) == 0)
                printf("Copying from 0x%08x to 0x%08x...\n",
                        src,address);
            algo->block_write_buf(this_cfi,address,src);
            src     += wbs;
            address += wbs;
        }
    }
    else
    {
        unsigned long nShort = nBytes/2 + ( (nBytes%2)>0 ? 1: 0);
        while(nShort--)
        {
            if(((address % (block_size/4)) == 0))
                printf("Copying from 0x%08x to 0x%08x...\n",
                        src,address);
            algo->block_write(this_cfi,address,src);
            address += 2;
            src += 2;
        }
    }

    if(algo->check_status)
        algo->check_status(this_cfi);
    algo->reset(this_cfi);
    printf("Done.\n");
    return 0;

}

static int cfi_flash_read(struct flash_device *this_flash, unsigned long 
		address, unsigned char *dst, unsigned long nBytes)
{
        while(nBytes--)
        {
                *dst++ = phoenix_inb(address++);
        }
	return 0;
}


static void print_cfi_flash_info(struct flash_device *this_flash)
{
    struct cfiflash *this_cfi = (struct cfiflash *)this_flash->priv;
    printf("Flash Info: ID%d, Algo:%s Blocks:0x%x WBS:0x%x Size:0x%x\n",
            this_cfi->id,
            this_cfi->algo->name,
            this_cfi->nr_blocks,
            this_cfi->wbs,
            this_cfi->size);
}

struct cfiflash rmi_cfiflash;

int cfi_flash_detect(struct flash_device *this_flash)
{
    struct cfiflash *this_cfi = NULL;
    unsigned long iobase = this_flash->iobase;

    unsigned short q = 0;
    unsigned short r = 0;
    unsigned short y = 0;
    unsigned short val = 0;

    /* Send CFI QRY command */
    phoenix_outw(iobase + (0x55 << 1), CFI_SWAP16(0x98));
    val = phoenix_inw(iobase + (0x10 <<1));
    q = CFI_SWAP16(val);

    val = phoenix_inw(iobase + (0x11 <<1));
    r = CFI_SWAP16(val);

    val = phoenix_inw(iobase + (0x12 <<1));
    y = CFI_SWAP16(val);

    if((q == 'Q') && (r == 'R') && (y == 'Y')) {
        
	this_cfi = &rmi_cfiflash;

        memset(this_cfi,0,sizeof(*this_cfi));

        this_cfi->iobase = iobase;
        this_cfi->priv = (void *)this_flash;

        val = phoenix_inw(iobase + (0x13 <<1));
        this_cfi->id        = CFI_SWAP16(val);

        val = phoenix_inw(iobase + (0x27 <<1));
        this_cfi->size      = (1 << CFI_SWAP16(val));

        val = phoenix_inw(iobase + (0x2a <<1));
        this_cfi->wbs       = (1 << CFI_SWAP16(val)) & ~0x1; // 0 : Not implemented

        val = phoenix_inw(iobase + (0x2d <<1));
        this_cfi->nr_blocks = CFI_SWAP16(val) + 1;
        this_cfi->bwidth    = 16;
        this_cfi->cwidth    = 16;

        switch(this_cfi->id)
        {
            case 1:
                sprintf(this_cfi->name,"Intel");
                this_cfi->algo = &algos[0];
                break;

            case 2:
                sprintf(this_cfi->name,"AMD");
                this_cfi->algo = &algos[1];
                break;
            default:
                printf("Unknown Flash\n");
                return -1;

        }
    }

    if(this_cfi)
    {
        sprintf(this_flash->name,"%s",this_cfi->name);
        this_flash->iobase = iobase;
        this_flash->flash_type = NOR_FLASH;
        this_flash->size   = this_cfi->size;
        this_flash->reset = cfi_flash_reset;
        this_flash->erase = cfi_flash_erase;
        this_flash->write = cfi_flash_write;
        this_flash->read = cfi_flash_read;
        this_flash->iprint = print_cfi_flash_info;
        this_flash->priv  = (void *)this_cfi;
    }
    this_flash->reset(this_flash);
    cpld_enable_flash_write(1);
    this_flash->iprint(this_flash);
    udelay(100 * 1000);


    return 0;
}
