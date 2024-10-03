/*********************************************************************

  Copyright 2003-2006 Raza Microelectronics, Inc. (RMI). All rights
  reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

  THIS SOFTWARE IS PROVIDED BY Raza Microelectronics, Inc. ``AS IS'' AND
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

  *****************************#RMI_2#**********************************/

//#define A_CONFIG_USB_EHCI
//#ifdef A_CONFIG_USB_EHCI

#define USB_MAX_PAGES (10)
#define USB_PAGE_SIZE (4096UL)  /*4K*/
#define USB_POOL_SIZE (USB_MAX_PAGES*USB_PAGE_SIZE) //1K Page each of 4K

#define USB_MAX_QH_PAGES  (10)
#define USB_QH_PAGE_SIZE (4096) /*4K*/
#define USB_QH_POOL_SIZE (USB_QH_PAGE_SIZE*USB_MAX_QH_PAGES)

#define USB_MAX_QTD_PAGES  (50)
#define USB_QTD_PAGE_SIZE (4096) /*4K*/
#define USB_QTD_POOL_SIZE (USB_QH_PAGE_SIZE*USB_MAX_QTD_PAGES)

#undef XLS_USB_DEBUG
#ifdef XLS_USB_DEBUG
#define Message(a,b...) printf("")//printf("\n[%s]@[%d]" a"\n",__FUNCTION__,__LINE__,##b)
#else
#define Message(a,b...) 
#endif
#define ErrorMsg(a,b...) printf("\n[%s]@[%d]" a"\n",__FUNCTION__,__LINE__,##b)

#define XLS_USB_DEBUG 1
#include <config.h>
#include <malloc.h>
#include <common.h>
#include <part.h>

#include "rmi/types.h"
#include "rmi/rmi_processor.h"
#include "rmi/on_chip.h"
#include "rmi/xlr_cpu.h"

#include <usb.h>
#include "usb_ehci.h"



/*Forward declaration goes here.*/
void put_usb_buf(ehci_t *ehci, unsigned char *buf);
unsigned char *get_usb_buf(ehci_t *ehci);
void ehci_qtd_free(ehci_t *ehci, struct ehci_qtd *qtd);
struct ehci_qtd *ehci_qtd_alloc(ehci_t *ehci);
void ehci_qh_free(ehci_t *ehci, struct ehci_qh *node);
struct ehci_qh * ehci_qh_alloc(ehci_t *ehci);

static int no_of_qh_allocated = 0;

/*********************************************************/
/*********************BUF POOL****************************/
/*********************************************************/

/*Below routine is not MP SAFE*/
unsigned char * get_usb_buf(ehci_t *ehci)
{
	struct usb_buf *node;
	if(list_empty(&(ehci->buf_pool_head.list)))
		return NULL;
	Message("\nTaking out %#lx\n",(unsigned long)ehci->buf_pool_head.list.next);
	node = list_entry(ehci->buf_pool_head.list.next,struct usb_buf , list);
	list_del_init(ehci->buf_pool_head.list.next);
	Message("\nNext Entry %#lx\n",(unsigned long)ehci->buf_pool_head.list.next);
	return node->buf;
}

/*Below routine is not MP SAFE*/
void put_usb_buf(ehci_t *ehci, unsigned char *buf)
{
    struct usb_buf *node = (struct usb_buf *)buf;
	list_add_tail(&node->list, &(ehci->buf_pool_head.list));
}

void test_usb_pool(ehci_t *ehci)
{
	int i;
    unsigned char *usb_node[USB_MAX_PAGES];

    printf("\nStartin Sanity Test, Max Page %d\n",USB_MAX_PAGES);
	for(i=0; i<USB_MAX_PAGES; i++){
		usb_node[i] = get_usb_buf(ehci);
		if(!usb_node[i]){
			printf("\nCouldnt get the usb buf\n");
		}else{
			printf("\nGot the buffer Address %#lx\n",(unsigned long)usb_node[i]);
		}
	}

	/*Empty check*/
	if(!get_usb_buf(ehci)){
		printf("\nGood!!! empty check passed\n");
	}else{
		printf("\nSomething went wrong.\n");
        while(1);
	}

	for(i=0; i<USB_MAX_PAGES; i++){
		put_usb_buf(ehci,usb_node[i]);
	}

	for(i=0; i<USB_MAX_PAGES; i++){
		usb_node[i] = get_usb_buf(ehci);
		if(!usb_node[i]){
			printf("\nCouldnt get the usb buf\n");
		}else{
			printf("\nGot the buffer Address %#lx\n",(unsigned long)usb_node[i]);
		}
	}

	for(i=0; i<USB_MAX_PAGES; i++){
		put_usb_buf(ehci,usb_node[i]);
	}
}

int alloc_usb_pool(ehci_t *ehci)
{
	unsigned char *tmp;
	struct usb_buf *node;
    int i=0;

	/*Allocate a 4K Page pool for later use.*/
	ehci->orig_buf = tmp = (unsigned char *)malloc(USB_POOL_SIZE+USB_PAGE_SIZE);

   	if(tmp){
		Message("\nMemory allocated at %#x\n",(unsigned long)tmp);
	}else {
		printf("\nCouldnt Allocate %#lx Memory\n",(unsigned long)USB_POOL_SIZE);
	}

	Message("\nOriginal Memory Allocated @ %#lx\n",(unsigned long)ehci->orig_buf);
	if((unsigned long)tmp & (USB_PAGE_SIZE-1)){
		tmp += (USB_PAGE_SIZE);
		tmp = (unsigned char *)((unsigned long)tmp & ~(USB_PAGE_SIZE-1));
		Message("\nNew Aligned Address %#x\n",(unsigned long)tmp);
	}

	ehci->aligned_buf = tmp;
	
	/*Now form the list*/
	INIT_LIST_HEAD(&(ehci->buf_pool_head.list));
    Message("\nUSB MAX PAGES ARE %d\n",USB_MAX_PAGES);
	for(i=0; i<USB_MAX_PAGES; i++){
		node = (struct usb_buf *)(tmp + (i*USB_PAGE_SIZE));
		node->buf = (unsigned char *)node;
		put_usb_buf(ehci, node->buf);
		Message("\nAdded Buf [%#x]",(unsigned long)node->buf);
	}
#if 0
    test_usb_pool(ehci);
#endif
	return 0;
}

/*********************************************************/
/*********************QH POOL*****************************/
/*********************************************************/

struct ehci_qh * ehci_qh_alloc(ehci_t *ehci)
{
	struct qh_pool *node;
    struct ehci_qh *qh = NULL;
	if(list_empty(&(ehci->qh_pool_head.list)))
		return NULL;
	node = list_entry(ehci->qh_pool_head.list.next,struct qh_pool, list);
	list_del_init(ehci->qh_pool_head.list.next);
    qh = node->qh;
    /*Reset QH*/
    memset (qh, 0, sizeof *qh);
    /*Initialization*/
    qh->ehci = ehci; 
    qh->qh_dma = virt_to_phys(qh);
    qh->index = no_of_qh_allocated++;
    INIT_LIST_HEAD(&qh->qtd_list);
    /* dummy qtd enables safe urb queuing */
    qh->dummy = ehci_qtd_alloc(ehci);
    if(!qh->dummy){
        printf("\nOoops..!!! QTD Allocation Failed.\n");
        /*Free QH*/
        ehci_qh_free(ehci,qh);
    }
    Message("\nAllocated QH @ %#lx\n",(unsigned long)qh);
	return qh;
}

void ehci_qh_free(ehci_t *ehci, struct ehci_qh *qh)
{
    struct qh_pool *node = (struct qh_pool *)qh;
    
    /*Free the dummy QTD linked with this QH*/
    if(qh->dummy)
        ehci_qtd_free(ehci,qh->dummy);

    node->qh = (struct ehci_qh *)node;
	list_add_tail(&node->list, &(ehci->qh_pool_head.list));
}

void test_qh_pool(ehci_t *ehci)
{
    int i;
	struct ehci_qh *qh_desc[USB_MAX_QH_PAGES];

    Message("\nStartin Sanity Test, Max Page %d\n",USB_MAX_QH_PAGES);
	for(i=0; i<USB_MAX_QH_PAGES; i++){
		qh_desc[i] = ehci_qh_alloc(ehci);
		if(!qh_desc[i]){
			printf("\nCouldnt get the usb buf\n");
            while(1);
		}else{
			Message("\nGot the buffer Address %#x\n",(unsigned long)qh_desc[i]);
            memset(qh_desc[i],0,USB_QH_PAGE_SIZE);
		}
	}
	/*Empty check*/
	if(!ehci_qh_alloc(ehci)){
		Message("\nGood!!! empty check passed\n");
	}else{
		printf("\nSomething went wrong.\n");
        while(1);
	}
	for(i=0; i<USB_MAX_QH_PAGES; i++){
        ehci_qh_free(ehci,qh_desc[i]);
	}
	for(i=0; i<USB_MAX_QH_PAGES; i++){
		qh_desc[i] = ehci_qh_alloc(ehci);
		if(!qh_desc[i]){
			printf("\nCouldnt get the usb buf\n");
            while(1);
		}else{
			Message("\nGot the buffer Address %#x\n",(unsigned long)qh_desc[i]);
            memset(qh_desc[i],0,USB_QH_PAGE_SIZE);
		}
	}
	for(i=0; i<USB_MAX_QH_PAGES; i++){
		ehci_qh_free(ehci,qh_desc[i]);
	}
}

int ehci_create_qh_pool(ehci_t *ehci)
{
    unsigned char *tmp;
    struct qh_pool *node;
    int i=0;
    tmp = (unsigned char *)malloc(USB_QH_POOL_SIZE+USB_QH_PAGE_SIZE);
    memset(tmp,0,USB_QH_POOL_SIZE+USB_QH_PAGE_SIZE);
    if(!tmp){
        printf("\nCouldnt Allocate QH Pool for EHCI\n");
        return -1;
    }
    Message("\n#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#");
    Message("\n#@#@#@@#QH STUFF STARTS#@#@#@#@#@");
    Message("\n#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#");
    ehci->qh_orig_buf = tmp;
    Message("\nOriginal Memory Allocated @ %#lx\n",(unsigned long)tmp);
    if((unsigned long)tmp & (USB_QH_PAGE_SIZE-1)){
		tmp += USB_QH_PAGE_SIZE;
		tmp = (unsigned char *)((unsigned long)tmp & ~(USB_QH_PAGE_SIZE-1));
		Message("\nNew Aligned Address %#x\n",(unsigned long)tmp);
	}
    ehci->qh_aligned_buf = tmp;

    /*Ok.. Now form the link list... */
    INIT_LIST_HEAD(&(ehci->qh_pool_head.list));

	for(i=0; i<USB_MAX_QH_PAGES; i++){
		node = (struct qh_pool *)(tmp + (i*USB_QH_PAGE_SIZE));
		ehci_qh_free(ehci, (struct ehci_qh *)node);
		Message("\nAdded QH Buf %#x",(unsigned long)node->qh);
	}
#ifdef XLS_USB_DEBUG
//  test_qh_pool(ehci);
#endif
    Message("\n#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#");
    Message("\n#@#@#@#@#QH STUFF ENDS#@#@#@#@#@#");
    Message("\n#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#@#");
    return 0;
}

/*********************************************************/
/*********************QTD POOL****************************/
/*********************************************************/

void test_qtd_pool(ehci_t *ehci)
{
    int i;
	struct ehci_qtd *qtd_desc[USB_MAX_QTD_PAGES];

    Message("\nStartin Sanity Test, Max Page %d\n",USB_MAX_QTD_PAGES);

	for(i=0; i<USB_MAX_QTD_PAGES; i++){
		qtd_desc[i] = ehci_qtd_alloc(ehci);
		if(!qtd_desc[i]){
			printf("\nCouldnt get the usb buf\n");
		}else{
			Message("\nGot the buffer Address %#x\n",(unsigned long)qtd_desc[i]);
            memset(qtd_desc[i],0,USB_QTD_PAGE_SIZE);
		}
	}

	/*Empty check*/
	if(!ehci_qtd_alloc(ehci)){
		Message("\nGood!!! empty check passed\n");
	}else{
		printf("\nSomething went wrong.\n");
        while(1);
	}

	for(i=0; i<USB_MAX_QTD_PAGES; i++){
        ehci_qtd_free(ehci,qtd_desc[i]);
	}

	for(i=0; i<USB_MAX_QTD_PAGES; i++){
		qtd_desc[i] = ehci_qtd_alloc(ehci);
		if(!qtd_desc[i]){
			printf("\nCouldnt get the usb buf\n");
		}else{
			Message("\nGot the buffer Address %#x\n",(unsigned long)qtd_desc[i]);
		}
	}
	for(i=0; i<USB_MAX_PAGES; i++){
		ehci_qtd_free(ehci,qtd_desc[i]);
	}
}

int ehci_create_qtd_pool(ehci_t *ehci)
{
    unsigned char *tmp;
    struct qtd_pool *node;
    int i=0;
    tmp = (unsigned char *)malloc(USB_QTD_POOL_SIZE+USB_QTD_PAGE_SIZE);
    Message("\n*****************************************\n");
    Message("\n*************QTD STUFF STARTS************\n");
    Message("\n*****************************************\n");
    if(!tmp){
        printf("\nCouldnt Allocate QTD Pool for EHCI\n");
        return -1;
    }
    ehci->qtd_orig_buf = tmp;
    Message("\nOriginal Memory Allocated @ %#lx\n",(unsigned long)tmp);
    if((unsigned long)tmp & (USB_QTD_PAGE_SIZE-1)){
		tmp += USB_QTD_PAGE_SIZE;
		tmp = (unsigned char *)((unsigned long)tmp & ~(USB_QTD_PAGE_SIZE-1));
		Message("\nNew Aligned Address %#x\n",(unsigned long)tmp);
	}
    ehci->qtd_aligned_buf = tmp;

    /*Ok.. Now form the link list... */
    INIT_LIST_HEAD(&(ehci->qtd_pool_head.list));

	for(i=0; i<USB_MAX_QTD_PAGES; i++){
		node = (struct qtd_pool *)(tmp + (i*USB_QTD_PAGE_SIZE));
		node->qtd = (struct ehci_qtd *)node;
		ehci_qtd_free(ehci, node->qtd);
//		list_add_tail(&node->list,&qtd_pool_head.list);
		Message("\nAdded QTD Buf %#x\n",(unsigned long)node->qtd);
	}
#ifdef XLS_USB_DEBUG
//  test_qtd_pool(ehci);
#endif
    Message("\n*****************************************\n");
    Message("\n*************QTD STUFF ENDS**************\n");
    Message("\n*****************************************\n");
    return 0;
}

/* Allocate the key transfer structures from the previously allocated pool */

void ehci_qtd_init (struct ehci_qtd *qtd, unsigned long dma)
{
    memset (qtd, 0, sizeof *qtd);
    qtd->qtd_dma = dma;
    qtd->hw_token = cpu_to_le32 (QTD_STS_HALT);
    qtd->hw_next = EHCI_LIST_END;
    qtd->hw_alt_next = EHCI_LIST_END;
    INIT_LIST_HEAD (&qtd->qtd_list);
}



struct ehci_qtd * ehci_qtd_alloc(ehci_t *ehci)
{
	struct qtd_pool *node;
    struct ehci_qtd *qtd;

	if(list_empty(&(ehci->qtd_pool_head.list)))
		return NULL;

	node = list_entry(ehci->qtd_pool_head.list.next,struct qtd_pool, list);
	list_del_init(ehci->qtd_pool_head.list.next);
    qtd = node->qtd;
    memset (qtd, 0, sizeof *qtd);
    qtd->qtd_dma = virt_to_phys(qtd);
    qtd->hw_token = cpu_to_le32 (QTD_STS_HALT);
    qtd->hw_next = EHCI_LIST_END;
    qtd->hw_alt_next = EHCI_LIST_END;
    INIT_LIST_HEAD (&qtd->qtd_list);
    Message("\nAllocated QTD @ %#lx\n",(unsigned long)qtd);
	return qtd;
}

void ehci_qtd_free(ehci_t *ehci, struct ehci_qtd *qtd)
{
    struct qtd_pool *node = (struct qtd_pool *)qtd;
    node->qtd = (struct ehci_qtd *)node;
	list_add_tail(&node->list, &(ehci->qtd_pool_head.list));
}

int ehci_mem_init(ehci_t *ehci)
{
    /*Create QTD Pool*/
    if(ehci_create_qtd_pool(ehci))
        return -1;

    /*Create QH Pool*/
    if(ehci_create_qh_pool(ehci))
        return -1;

    ehci->async = ehci_qh_alloc(ehci);

    if(!ehci->async){
        printf("\nAsync QH Allocation Failed.\n");
        return -1;
    }
    
    return 0;
}

//#endif
