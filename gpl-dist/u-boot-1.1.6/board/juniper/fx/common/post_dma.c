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

/* 
	implemention of DMA post  
*/


#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <rmi/pci-phoenix.h>
#include <rmi/on_chip.h>
#include <rmi/byteorder.h>
#include <rmi/gpio.h>
#include <rmi/bridge.h>
#include <malloc.h>
#include <rmi/msgring.h>
#include "post_dma.h"

#include <common/post.h>

#undef DEBUG


static char diag_xls_cc_table_dma[128] = {

                4, 4 , 4 , 4 , 4 , 4 , 4 , 4 ,
                4, 4 , 4 , 4 , 4 , 4 , 4 , 4 ,
                4, 4 , 4 , 4 , 0 , 0 , 0 , 0 ,
                4, 4 , 4 , 4 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 2 , 4 , 4 , 4 , 4 , 0 , 2 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 2 , 4 , 4 , 4 , 4 , 0 , 2 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                0, 0 , 0 , 0 , 0 , 0 , 0 , 0 
        };


int diag_post_dma_init(void);

static struct dma_config *dma_init(int msg_type, int channel, int rmaxcr, int wmaxcr)
{
  uint32_t control;
  uint32_t old_control_reg_val;
  uint32_t val = 0x0;

  if (channel < 0 && channel >= 4)
  {
    post_printf(POST_VERB_STD,"Error:%s: Wrong channel id: %d\n",__FUNCTION__, channel);
    return NULL;
  }
  if (rmaxcr < 0 && rmaxcr >= 8)
  {
    post_printf(POST_VERB_STD,"Error:%s: Wrong max credits for read fifo: %d\n",__FUNCTION__, rmaxcr);
    return NULL;
  }
  if (wmaxcr < 0 && wmaxcr >= 8)
  {
    post_printf(POST_VERB_STD,"Error:%s: Wrong max credits for write fifo: %d\n",__FUNCTION__, wmaxcr);
    return NULL;
  }

  if (channel == 0)
    control = (DMA_CONTROL_CHANNEL0);
  else if (channel == 1)
    control = (DMA_CONTROL_CHANNEL1);
  else if (channel == 2)
    control = (DMA_CONTROL_CHANNEL2);
  else
    control = (DMA_CONTROL_CHANNEL3);
  
  old_control_reg_val = dma_read_reg(control);
  val = val | 1 << 24;         /* report errors */ 
  val = val | DMA_DFLT_FIFO_SIZE << 12; /* Default section size for the channel */
  val = val | (rmaxcr << 8); /* Setup max credits for DMA read fifo */
  val = val | (wmaxcr << 5); /* Setup max credits for DMA write fifo */
  val = val | (1 << 4);      /* Enable the channel */
  dma_write_reg(control, val);

  read_32bit_cp2_register_jack(MSGRNG_MSG_STATUS_REG_JACK);

  return xlr_dma_config(msg_type, 0, 0, 0, 0, 0, 0, 0);
}


int diag_post_dma_init()
{
    int i;

    debug("%s:\n", __FUNCTION__);

// **********************************************************
    rmiboard.rmichip.dma_mmio =
                (volatile uint32_t *)
                (phoenix_io_base+PHOENIX_IO_DMA_OFFSET);


// **********************************************************
    dma_write_reg(DMA0_BUCKET_SIZE, 32);
    dma_write_reg(DMA1_BUCKET_SIZE, 32);
    dma_write_reg(DMA2_BUCKET_SIZE, 32);
    dma_write_reg(DMA3_BUCKET_SIZE, 32);

    for(i=0;i<128;i++) {
        dma_write_reg( DMA_CC_CPU0_0 + i,
                 diag_xls_cc_table_dma[i]);
    }


// **********************************************************

    for (i=0; i<12; i++) {
        /* Create two buffers */
        if ((buf1[i] = (unsigned char*) malloc (DMA_BUF_LENGTH)) == NULL) {
            post_printf(POST_VERB_STD,"Error:[%s]: malloc failed\n",__FUNCTION__);
            return 0;   
        }

        if ((buf2[i] = (unsigned char*) malloc (DMA_BUF_LENGTH)) == NULL) {
            post_printf(POST_VERB_STD,"Error:[%s]: malloc failed\n",__FUNCTION__);
            return 0;
        }
 
        memset(buf1[i], '\0', DMA_BUF_LENGTH);
        memset(buf2[i], '\0', DMA_BUF_LENGTH);
    }

// **********************************************************

  msgrng_write_cc(MSGRNG_CC_13_REG, 8, 0);
  msgrng_write_cc(MSGRNG_CC_13_REG, 8, 1);
  msgrng_write_cc(MSGRNG_CC_13_REG, 8, 2);
  msgrng_write_cc(MSGRNG_CC_13_REG, 8, 3);

    return 1;
}

static __inline__ int dma_set_inform_src(struct dma_config *dmacfg, int inform_src)
{                         
  int old_val;
  old_val = dmacfg->inform_src;
  dmacfg->inform_src = inform_src;
  return old_val;
}


static __inline__ int dma_get_tid(struct dma_config *dmacfg)
{  
  return dmacfg->tid;
}   

static __inline__ int dma_get_cache_w_coh(struct dma_config *dmacfg)
{
  return dmacfg->cache_w_coh;
}


static __inline__ int dma_get_cache_r_coh(struct dma_config *dmacfg)
{
  return dmacfg->cache_r_coh;
}

static __inline__ int dma_get_l2_alloc(struct dma_config *dmacfg)
{
  return dmacfg->l2_alloc;
}

static __inline__ int dma_get_use_old_crc(struct dma_config *dmacfg)
{
  return dmacfg->use_old_crc;
}

static __inline__ int dma_get_inform_src(struct dma_config *dmacfg)
{
  return dmacfg->inform_src;
}

// Transaction id it is not 
static __inline__ int dma_incr_tid(struct dma_config *dmacfg)
{                         
  int old_val;
  old_val = dmacfg->tid;
  dmacfg->tid++;
  return old_val;
}

#define barrier() __asm__ __volatile__("sync": : :"memory")
#define msgrng_read_status() read_32bit_cp2_register(MSGRNG_MSG_STATUS_REG)

static inline void msg_status_perror (unsigned int status)
{
  if (status & cpu_to_be32(0x1))
    post_printf(POST_VERB_STD,"Error:Send Message Pending. A send message was accepted but has not be transferred into the transmit FIFO\n");
  if (status & cpu_to_be32(0x2))
    post_printf(POST_VERB_STD,"Error:Send Message Pending Fail. Send Message failed because another one of the same thread is pending\n");
  if (status & cpu_to_be32(0x4))
    post_printf(POST_VERB_STD,"Error:Send Message Credit Failed. No space in destination\n");
  if (status & cpu_to_be32(0x8))
    post_printf(POST_VERB_STD,"Error:Load Message Pending. Load message was accepted, but hasnt completed\n");
  if (status & cpu_to_be32(0x10))
    post_printf(POST_VERB_STD,"Error:Load Message Pending Fail. Load message failed because another one of the same thread is pending\n");
  if (status & cpu_to_be32(0x20))
    post_printf(POST_VERB_STD,"Error:Load Message Empty Fail. Receive FIFO is empty\n");

  post_printf(POST_VERB_STD,"Error:Recieve message size = 0x%x\n",(status & cpu_to_be32(0xc0))+1);
  post_printf(POST_VERB_STD,"Error:Recieve message software code = 0x%x\n",(status & cpu_to_be32(0xff00)));
  post_printf(POST_VERB_STD,"Error:Recieve message source id = 0x%x\n",(status & cpu_to_be32(0x7f0000)));
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 0 is %s\n",(status & cpu_to_be32(0x1000000))?"empty":"not-empty");
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 1 is %s\n",(status & cpu_to_be32(0x2000000))?"empty":"not-empty");
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 2 is %s\n",(status & cpu_to_be32(0x4000000))?"empty":"not-empty");
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 3 is %s\n",(status & cpu_to_be32(0x8000000))?"empty":"not-empty");
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 4 is %s\n",(status & cpu_to_be32(0x10000000))?"empty":"not-empty");
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 5 is %s\n",(status & cpu_to_be32(0x20000000))?"empty":"not-empty");
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 6 is %s\n",(status & cpu_to_be32(0x40000000))?"empty":"not-empty");
  post_printf(POST_VERB_STD,"Error:Recieve FIFO bucket 7 is %s\n",(status & cpu_to_be32(0x80000000))?"empty":"not-empty");
}

/* Generic message send API NON blocking */
static inline int message_send(unsigned int size, unsigned int code,
                               unsigned int stid, struct msgrng_msg *msg)
{ 
        unsigned long long  status = 0;
        //unsigned int  cp0_status = read_c0_status();
        unsigned int dest = 0;
        int i=0;

        barrier();   

        status = msgrng_read_status();

        msgrng_load_tx_msg0(msg->msg0);
        msgrng_load_tx_msg1(msg->msg1);
        msgrng_load_tx_msg2(msg->msg2);
        msgrng_load_tx_msg3(msg->msg3);

        dest = ((size-1)<<16)|(code<<8)|(stid);

#ifdef MSGRING_DUMP_MESSAGES
        post_printf(POST_VERB_DBG,"Sending msg<%llx,%llx,%llx,%llx> to dest = %x\n",
                msg->msg0, msg->msg1, msg->msg2, msg->msg3, dest);
#endif


        for(i=0;i<MAX_MSGSND_TRIES;i++) {
                msgrng_send(dest);
                /* Check the status */
                status = msgrng_read_status();
                status = status & 0x7;
                if (status & 0x6) {
                  /* either previous send failed or no credits
                   * return error
                   */
                  continue;
                }
                else break;
        }
        if (i==MAX_MSGSND_TRIES) {
                post_printf(POST_VERB_STD,"Error: Unable to send msg to %Lx\n", dest);
                return status & 0x6;
        }

        return 0;
}


int dma_make_desc_tx(struct dma_config *dmacfg, int channel, int fr_stid, struct msgrng_msg *msg, int type,
           unsigned long src_addr, unsigned long dest_addr, unsigned int tid, int len)
{
  int tx_stid = 0;

        /* Note: We need to send the message to different buckets to
         * perform DMA on any of the 4 channels
         */
        if (channel == 0)
                tx_stid = MSGRNG_STNID_DMA_0;
        else if (channel == 1)
                tx_stid = MSGRNG_STNID_DMA_1;
        else if (channel == 2)
                tx_stid = MSGRNG_STNID_DMA_2;
        else if (channel == 3)
                tx_stid = MSGRNG_STNID_DMA_3;
        else
        {
                post_printf(POST_VERB_STD,"Error:Wrong channel\n");
        }

  msg->msg0 = 0;
  msg->msg1 = 0;
  msg->msg2 = 0;
  msg->msg3 = 0;
  switch (type)
  {
    case SIMPLE_XFER:
        msg->msg0 = ( ((uint64_t)1 << 63) |                     /* SOP                  */
          ((uint64_t)0 << 61) |                                 /* SIMPLE_XFER: 00      */
          ((uint64_t)len<<40) |                                 /* Length               */
          ((uint64_t)src_addr <<0)                              /* Src Addr             */
        );
        msg->msg1 = ( ((uint64_t)0 << 63) |                     /* ~SOP                 */
          ((uint64_t)dma_get_cache_w_coh(dmacfg) << 62) |  /* cache write coherent */
          ((uint64_t)dma_get_cache_r_coh(dmacfg) << 61) |  /* cache read coherent  */
          ((uint64_t)dma_get_l2_alloc(dmacfg) << 60) |     /* l2 Allocate          */
          ((uint64_t)dma_get_use_old_crc(dmacfg) << 59) |  /* use old CRC          */
          ((uint64_t)dma_get_inform_src(dmacfg) << 58) |   /* inform src           */
          ((uint64_t)tid<<48) |                                 /* Transaction Id       */
          ((uint64_t)fr_stid<<40) |                             /* Message Src Stn id   */
          ((uint64_t)dest_addr <<0)                             /* Dest Addr            */
        );
        break;
    default:
        post_printf(POST_VERB_STD,"Error:%s: Unknown type: %d\n",__FUNCTION__, type);
        break;
  }

  return tx_stid;
}



int dma_simple_xfer(struct dma_config *dmacfg, int channel, int fr_stid, unsigned long dest_addr, uint8_t *pbuf, unsigned int len, unsigned int offset)
{
  uint8_t *pkt_buf ;
  struct msgrng_msg msg;
  int stid = 0;
  int status;
  int temp;

  dma_set_inform_src(dmacfg, 1);
  
  pkt_buf = (uint8_t *)pbuf + (offset * sizeof(unsigned char));
 
  stid = dma_make_desc_tx(dmacfg, channel, fr_stid, &msg, SIMPLE_XFER,
                          virt_to_phys(pkt_buf), dest_addr, dma_get_tid(dmacfg), len);
  /* Send the packet to DMA */
  dma_incr_tid(dmacfg);
        


  /* Note: We need to send only two words for dma simple_xfer */
  if (((status = message_send(2, MSGRNG_CODE_DMA, stid, &msg)) & 0x7) > 0)
  {
    post_printf(POST_VERB_STD,"Error:Unable to send pkt to DMA\n");
    msg_status_perror(status);
  }

  debug("[%s]: Done sending the pkt\n",__FUNCTION__);  

#ifdef FX_COMMON_DEBUG
  /* CPU0 bucket size */
   temp = msgrng_read_bucksize( 0);
   debug("CPU0 bucketsize:0x%x\n",temp);
  /*CPU0 credits for sending messages to DMA*/ 
  temp =  msgrng_read_cc(MSGRNG_CC_13_REG,0);
   debug("CPU0 Credits for DMA bucket0:0x%x\n",temp);
#endif


  return 0;
}


int dma_rx(int bucket, unsigned int *channel, unsigned int *tid, unsigned int *csum, unsigned int *crc)
{
  int size = 0;
  int code = 0;
  int stid = 0;
  struct msgrng_msg msg;
  unsigned long long mask;
  unsigned long long ret;
 
  debug("bucket mask: %d\n",bucket);

  msgrng_wait((1 << bucket));
  while (code != MSGRNG_CODE_DMA)
  {
    message_receive(bucket, &size, &code, &stid, &msg);
  
                if (stid == MSGRNG_STNID_DMA_0 || stid == MSGRNG_STNID_DMA_1 ||
                          stid == MSGRNG_STNID_DMA_2 || stid == MSGRNG_STNID_DMA_3)
                        break;
                else
                        post_printf(POST_VERB_DBG,"DMA Station Id stid = %d\n",stid);
  }

  /* Check return code */
  mask = ( (unsigned long long) 3 << 62 );      /* Return code is in bits 62:63 */
  ret = (msg.msg0 & mask)>>62;
  if (ret != 3)
  {
    post_printf(POST_VERB_STD,"Error:Bad Return code 0x%llx\n", ret);
    return 0;
  }

  /* Check for errors */
  mask = ( (unsigned long long) 1 << 61 );      /* Message format error is in bit 61 */
  ret = (msg.msg0 & mask)>>61;
  if (ret != 0)
  {
    post_printf(POST_VERB_STD,"Error:Message format error\n");
    return 0;
  }


  mask = ( (unsigned long long) 1 << 60 );      /* Bus error is in bit 60 */
  ret = (msg.msg0 & mask)>>60;
  if (ret != 0)
  {
    post_printf(POST_VERB_STD,"Error:Bus error\n");
    return 0;
  }

  /* Get channel id */
  mask = ( (unsigned long long) 3 << 58 );     /* Channel id 58:59 */
  *channel = (msg.msg0 & mask)>>58;

  /* Get Transaction id */
  mask = ( (unsigned long long) 1023 << 48 );     /* Transaction id 48:57 */
  *tid = (msg.msg0 & mask)>>48;

  /* Get csum */
  mask = ( (unsigned long long) 65535 << 32 );     /* csum 32:47 */
  *csum = (msg.msg0 & mask)>>32;

  /* Get crc */
  mask = ( (unsigned long long) 0xffffffff << 0 );     /* crc 0:31 */
  *crc = (msg.msg0 & mask)>>0;

  return 1;
}



int diag_post_dma_run(void);


int diag_post_dma_run(void)
{
    int i;
    struct dma_config *cfg;
    struct dma_config *multi_cfg[4];
    unsigned int recv_channel = 0;
    unsigned int tid = 0;
      unsigned int csum = 0;
  unsigned int crc = 0;
    int prsr_id = processor_id();
    int status = 0;
    
    debug("%s:\n", __FUNCTION__);

  for (i=0; i<4; i++) {
      memset(buf1[i], 0xAA+i, 512);
      memset(buf2[i], '\0', 512);
   
      /* dma simple xfer from buffer 1 to 2 */
      cfg = (struct dma_config *)dma_init(SIMPLE_XFER, i, 2, 2);
          if (cfg == NULL) {
              post_printf(POST_VERB_STD,"Error:DMA configuration failed\n");
              post_test_status_set(POST_DMA_TXR_ID, POST_DMA_ERROR1);
              status = 1;
              goto diag_post_dma_run_exit ;
          }
   
      // send dma and tell dma engine to notify backet 6 the msg
      dma_simple_xfer(cfg, i, 6, virt_to_phys(buf2[i]), buf1[i], 512, 0);

    /* Recieve the results at bucket 7*/
    dma_rx(6, &recv_channel, &tid, &csum, &crc);
   
    /* compare buffers */
    if (memcmp(buf1[i], buf2[i], 512) != 0){
        post_printf(POST_VERB_STD,"Error:DMA Xfer test failed on channel %d\n", i);
        post_test_status_set(POST_DMA_TXR_ID, POST_DMA_ERROR1);
        status = 1;
        goto diag_post_dma_run_exit;
    } else {
        post_printf(POST_VERB_DBG,"Pass Simple DMA Test Channel=%d ! \n",i);
    }

  }


// Now issue DMA transfer multi-dma task

    post_printf(POST_VERB_DBG,"Co-current DMA Test\n");

  for (i=0; i<4; i++) {
      memset(buf1[i],  0x55+i, 512);
      memset(buf2[i], '\0', 512);
   
      /* dma simple xfer from buffer 1 to 2 */
      multi_cfg[i] = (struct dma_config *)dma_init(SIMPLE_XFER, i, 2, 2);
          if (multi_cfg[i] == NULL) {
              post_printf(POST_VERB_STD,"Error:Co-current  DMA configuration failed\n");
              post_test_status_set(POST_DMA_TXR_ID, POST_DMA_ERROR2);
              status = 1;
              goto diag_post_dma_run_exit ;
          }

    } 
   
    for (i=0; i<4; i++) {
      // Change buf2[???x???] perchannel 
      dma_simple_xfer(multi_cfg[i], i, i, virt_to_phys(buf2[i]), buf1[i], 512, 0);
    }

    for (i=0; i<4; i++) {
        /* DMA Rx */
        dma_rx(i, &recv_channel, &tid, &csum, &crc);
   
        /* compare buffers */
        if (memcmp(buf1[i], buf2[i], 512) != 0){
            post_printf(POST_VERB_STD,"Error:Concurrent DMA Xfer Failed On Channel %d\n", i);
            post_test_status_set(POST_DMA_TXR_ID, POST_DMA_ERROR2);
            status = 1;
            goto diag_post_dma_run_exit;
        } else {
            post_printf(POST_VERB_DBG,"Pass Concurrent DMA Test Channel=%d ! \n",i);
        }
    }

diag_post_dma_run_exit:

   return status;
}


int diag_post_dma_clean(void)
{
    int i;
    int status = 1;

    debug("%s:\n", __FUNCTION__);

    for (i=0; i<12; i++) {
        if (buf1[i]) {
            free (buf1[i]);
        }

        if (buf2[i]) {
            free (buf2[i]);
        }
    }


    return status;
}


typedef struct DIAG_DMA_T{
    char *test_name;
    int (*init)(void);
    int (*run)(void);
    int (*clean)(void);
    int continue_on_error;
} diag_dma_t;


diag_dma_t diag_post_dma={
    "DMA Test\n",
    diag_post_dma_init,
    diag_post_dma_run,
    diag_post_dma_clean,
    0
};


int diag_dma_post_main(void)
{
    int status = 0;

    debug("%s: \n", __FUNCTION__);

    post_printf(POST_VERB_STD,"Test Start: %s \n", diag_post_dma.test_name);

    status = diag_post_dma.init();

    if (status ==0){
        goto diag_dma_post_main_exit;
    }

    status = diag_post_dma.run();

    if (status ==0){
        goto diag_dma_post_main_exit;
    }


    

diag_dma_post_main_exit:

    diag_post_dma.clean();
    post_printf(POST_VERB_STD,"%s %s \n", diag_post_dma.test_name,status == 1 ? "PASS":"FAILURE");
    post_printf(POST_VERB_STD,"Test End: %s \n", diag_post_dma.test_name);
    return status;
}


int diag_post_dma_main(void)
{
    int status = 1;
    status = diag_dma_post_main();
    return status;
}

