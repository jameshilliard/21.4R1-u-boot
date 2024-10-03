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
#include "common.h"
#include "rmi/packet.h"
#include "rmi/cache.h"
#include "rmi/shared_structs_func.h"
#include "rmi/smp.h"


struct packet packets[MAX_PACKETS];

/*
struct packet packets[MAX_PACKETS] __section(".packets");

struct fr_desc frin_spill_packets[MAX_FRIN_SPILL_ENTRIES] __section(".packets")
    ____cacheline_aligned;

struct fr_desc frout_spill_packets[MAX_FROUT_SPILL_ENTRIES]
__section(".packets") ____cacheline_aligned;

union rx_tx_desc rx_spill_packets[MAX_RX_SPILL_ENTRIES] __section(".packets")
    ____cacheline_aligned;
*/

static unsigned int pkt_bitmap[MAX_PACKETS / (sizeof(unsigned int))];
static spinlock_t packet_lock;
static unsigned int pbuf_dbg = 0;
struct packet *alloc_pbuf(void)
{
	int num_chunks = MAX_PACKETS / 32;
	int i = 0, j = 0;

	smp_lock(&packet_lock);
	for (i = 0; i < num_chunks; i++) {
		if (pkt_bitmap[i] != 0xffffffff) {
			for (j = 0; j < 32; j++) {	// there is atleast on bit available
				if (!(pkt_bitmap[i] & (1 << j))) {
					pkt_bitmap[i] |= (1 << j);
					goto available;
				}
			}
		}
	}
	smp_unlock(&packet_lock);

	printf("Unable to allocate a pbuf!\n");
	return 0;

      available:
	smp_unlock(&packet_lock);
	return &packets[i * 32 + j];
}

void free_pbuf(struct packet *pbuf)
{
	int index = 0;
	//struct packet *tmp = &packets[0];

	if (pbuf < &packets[0] || pbuf >= &packets[MAX_PACKETS]) {
		printf
		    ("[%s]: bad packet! pbuf=%p, &packets[0]=%p, &packets[MAX_PACKETS]=%p\n",
		     __FUNCTION__, pbuf, &packets[0], &packets[MAX_PACKETS]);
		return;
	}
	// TODO: do a pointer diff, for some reason it isn't working. may be because they are
	// in diff sections?

	index =
	    ((unsigned long)pbuf -
	     (unsigned long)&packets[0]) / sizeof(struct packet);
	smp_lock(&packet_lock);
	pkt_bitmap[index >> 5] &= ~(1 << (index & 0x1f));
	smp_unlock(&packet_lock);
}

void packet_init(void)
{
	int size = sizeof(struct packet);

	smp_lock_init(&packet_lock);

	if (size & 0x1f) {
		printf
		    ("size of struct packet (0x%x) is not a multiple of cacheline size\n",
		     size);
	}
}

void enable_pbuf_dbg(void)
{
	smp_lock(&packet_lock);
	pbuf_dbg = 1;
	smp_unlock(&packet_lock);
}
void disable_pbuf_dbg(void)
{
	smp_lock(&packet_lock);
	pbuf_dbg = 0;
	smp_unlock(&packet_lock);
}
unsigned int get_pbuf_dbg(void)
{
	return pbuf_dbg;
}
void print_packet_pci(const char *pkt_buf, int len)
{
	int i = 0, j = 0;
	unsigned short *p = (unsigned short *)pkt_buf;
	unsigned int   nWords = len/2;
	smp_lock(&packet_lock);
	if(pbuf_dbg)
	{
		printf("Dumping %d bytes\n",len);
		for (i = 0; i < nWords; ) {
			printf("\t0x%04x:  ",2*i);
			for(j = 0; j <8 ; j++)
			{
				if(i==nWords) break;
				printf("%04x ", *p);
				p++;	i++;
			}
		printf("\n");
		}
	}
	smp_unlock(&packet_lock);
}
