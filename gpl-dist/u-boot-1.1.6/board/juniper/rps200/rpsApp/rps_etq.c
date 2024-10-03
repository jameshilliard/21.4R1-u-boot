/*
 * Copyright (c) 2008-2010, Juniper Networks, Inc.
 * All rights reserved.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "rps.h"
#include <exports.h>
#include "rps_command.h"
#include "rps_etq.h"

#undef ETQ_DEBUG
#define MAX_TIMER_EVENTS 30

long gTid = 0;

typedef struct
{
    void (*pFunc)(struct tdata *);
    tData data;
    int32_t tid;
    int32_t ttype;
    int32_t start;
    int32_t cycle;
    int32_t delta;
} tTimer, *ptTimer;

typedef struct tnode
{
    struct tnode *pPrev, *pNext;
    int32_t flag;
    tTimer timeEvent; 
} tNode, *ptNode;

typedef struct tqueue
{
    ptNode head;
    ptNode tail;
} tQueue, *ptQueue;

static tNode gNode[MAX_TIMER_EVENTS];
static tQueue activeQ, freeQ;

int32_t 
getNextId (void)
{
    return (++gTid);
}


int32_t 
qEmpty (tQueue queue)
{
    return (queue.head == NULL);
}

int32_t 
qAvail (tQueue queue)
{
    return (queue.head != queue.tail);
}

ptNode 
qPeek (ptQueue queue)
{
    ptNode np = NULL;

    if (queue->head == NULL)
        return (NULL);

    np = queue->head;

    return (np);
}

ptNode 
qPop (ptQueue queue)
{
    ptNode np = NULL;

    if (queue->head == NULL)
        return (NULL);

    np = queue->head;
    queue->head = queue->head->pNext;

    if (queue->head == NULL)
        queue->tail = NULL;
    else
        queue->head->pPrev = NULL;

    np->pPrev = np->pNext = NULL;

    return (np);
}

void 
qAppend (ptQueue queue, ptNode pn)
{
    if (queue->head == NULL)
    {
        queue->head = queue->tail = pn;
        pn->pPrev = pn->pNext = 0;
    }
    else
    {
        pn->pPrev = queue->tail;
        queue->tail = pn->pPrev->pNext = pn;
    }
}

int32_t 
qDelete (ptQueue queue, int32_t id)
{
    ptNode pn, pn1;
    
    if (queue->head == NULL)
    {
        return (0);
    }
    else
    {
        pn = queue->head;
        while (pn != NULL)
        {
            if (pn->timeEvent.tid == id)
            {
                if (pn == queue->head)
                {
                    if (pn->pNext != NULL)
                    {
                        pn->pNext->timeEvent.delta += pn->timeEvent.delta;
                    }
                    pn = qPop(queue);
                    pn->pPrev = pn->pNext = NULL;
                    qAppend ((ptQueue)&freeQ, pn);
                }
                else
                {
                    pn->pPrev->pNext = pn->pNext;
                    pn->pNext->pPrev = pn->pPrev;
                    pn1 = pn;
                    pn = pn->pNext;
                    if (pn != NULL)
                    {
                        pn->timeEvent.delta += pn1->timeEvent.delta;
                    }
                    pn1->pPrev = pn1->pNext = NULL;
                    qAppend ((ptQueue)&freeQ, pn1);
                }
                return (0);
            }
            else
            {
                pn = pn->pNext;
            }
        }
    }
    return (0);
}

int32_t 
qInsert (ptQueue queue, ptNode pn)
{
    ptNode ptn;
    
    if (queue->head == NULL)
    {
        pn->pPrev = pn->pNext = NULL;
        qAppend (queue, pn);
        return (0);
    }
    else
    {
        ptn = queue->head;
        while (ptn != NULL)
        {
            if (pn->timeEvent.delta >= ptn->timeEvent.delta)
            {
                pn->timeEvent.delta -= ptn->timeEvent.delta;
                ptn = ptn->pNext;
            }
            else
            {
                pn->pPrev = ptn->pPrev;
                pn->pNext = ptn;
                ptn->pPrev->pNext = pn;
                ptn->pPrev = pn;
                ptn->timeEvent.delta -= pn->timeEvent.delta;
                if (queue->head == ptn)
                {
                    queue->head = pn;
                }
                return (0);
            }
        }
        pn->pPrev = pn->pNext = NULL;
        qAppend (queue, pn);
    }
    return (0);
}

void
qInit (void)
{
    int i;
    
    activeQ.head = NULL;
    activeQ.tail = NULL;

    freeQ.head = (ptNode)&gNode[0];
    freeQ.tail = (ptNode)&gNode[MAX_TIMER_EVENTS-1];
    gNode[0].pPrev = NULL;
    gNode[MAX_TIMER_EVENTS-1].pNext = NULL;
    for (i = 0; i < MAX_TIMER_EVENTS; i++)
    {
        gNode[i].flag = i;
        gNode[i].timeEvent.tid = 0;
        gNode[i].timeEvent.ttype = 0;
        gNode[i].timeEvent.start = 0;
        gNode[i].timeEvent.cycle = 0;
        gNode[i].timeEvent.delta = 0;
    }
    for (i = 0; i < MAX_TIMER_EVENTS-1; i++ )
    {
        if (MAX_TIMER_EVENTS-1 > i)
        {
            gNode[i].pNext = (ptNode)&gNode[i+1];
            gNode[i+1].pPrev = (ptNode)&gNode[i];
        }
    }

}

int32_t
addTimer (tData tdata, int32_t *tid, int32_t ttype, int32_t start,
           int32_t cycle, void (*pFunc)(ptData))
{
    int i;
    ptNode pn = qPop((ptQueue)&freeQ);

    if (pn == NULL)
        return (-1);

    pn->timeEvent.tid = *tid = getNextId();
    pn->timeEvent.ttype = ttype;
    pn->timeEvent.cycle = cycle;
    pn->timeEvent.start = pn->timeEvent.delta = start;
    pn->timeEvent.pFunc = pFunc;
    for (i = 0; i < MAX_TIMER_PARAMS-1; i++)
    {
        pn->timeEvent.data.param[i] = tdata.param[i];
    }
    
    return (qInsert((ptQueue)&activeQ, pn));
}

int32_t 
delTimer (int32_t tid)
{
    return (qDelete((ptQueue)&activeQ, tid));
}

void 
updateTimer (int32_t ticks)
{
    ptNode pn;
#if defined(ETQ_DEBUG)
    uint32_t start_ms, end_ms, diff_ms;
#endif

    if (qEmpty(activeQ))
        return;

    pn = qPeek((ptQueue)&activeQ);
    pn->timeEvent.delta -= ticks;
    if (pn->timeEvent.delta < 0)
        pn->timeEvent.delta = 0;
        
    while ((pn != NULL) && (pn->timeEvent.delta == 0))
    {
        pn = qPop((ptQueue)&activeQ);
#if defined(ETQ_DEBUG)
        start_ms = get_timer(0)/gTicksPerMS;
        (pn->timeEvent.pFunc)((ptData)&pn->timeEvent.data);
        end_ms = get_timer(0)/gTicksPerMS;
        if (end_ms < start_ms)
            diff_ms= (gMaxMS - start_ms) + end_ms;
        else
            diff_ms = end_ms - start_ms;
            
        if (diff_ms >= 100)
            printf("pFunc %08x  %d ms\n", pn->timeEvent.pFunc, diff_ms);
#else
        (pn->timeEvent.pFunc)((ptData)&pn->timeEvent.data);
#endif
        if (pn->timeEvent.ttype == PERIDICALLY)
        {
            pn->timeEvent.delta = pn->timeEvent.cycle;
            qInsert((ptQueue)&activeQ, pn);
        }
        else
        {
            pn->pPrev = pn->pNext = NULL;
            qAppend((ptQueue)&freeQ, pn);
        }
        pn = qPeek((ptQueue)&activeQ);
    }
}

void 
qDump (void)
{
    ptNode pn;
    int i;
    
    printf("activeQ = 0x%08x head = 0x%08x, tail = 0x%08x\n",
           &activeQ, activeQ.head, activeQ.tail);
    printf("freeQ = 0x%08x head = 0x%08x, tail = 0x%08x\n",
           &freeQ, freeQ.head, freeQ.tail);
    for (i = 0; i < MAX_TIMER_EVENTS; i++ )
    {
        printf("gNode[%d] = 0x%08x, pNext = 0x%08x, pPrev = 0x%08x\n",
               i, &gNode[i], gNode[i].pNext, gNode[i].pPrev);
    }
    
    pn = activeQ.head;
    printf("activeQ: %s\n", (activeQ.head == NULL) ? "empty" : "not empty");
    while (pn != NULL)
    {
        printf("f = %d, s/c/d = %d/%d/%d, id = %d, type = %d, pf = 0x%08x\n",
                pn->flag, 
                pn->timeEvent.start, 
                pn->timeEvent.cycle, 
                pn->timeEvent.delta, 
                pn->timeEvent.tid, 
                pn->timeEvent.ttype,
                pn->timeEvent.pFunc);
                pn = pn->pNext;
    }
    
    pn = freeQ.head;
    printf("freeQ: %s\n", (freeQ.head == NULL) ? "empty" : "not empty");
    while (pn != NULL)
    {
        printf("f = %d, s/c/d = %d/%d/%d, id = %d, type = %d\n",
                pn->flag, 
                pn->timeEvent.start, 
                pn->timeEvent.cycle, 
                pn->timeEvent.delta, 
                pn->timeEvent.tid, 
                pn->timeEvent.ttype);
                pn = pn->pNext;
    }
}

void 
timeExpired (ptData ptdata)
{
    static int count = 0;

    
    printf ("%d\n", ++count);
}

/* 
 *
 * event time queue commands
 * 
 */
int 
do_etq (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    char *cmd = "dump";  /* default command */
    extern unsigned long oldticks;
    
    if (argc <= 1)
        goto usage;
    
    if (argc >= 2) cmd   = argv[1];

    if (!strncmp(cmd,"dump", 3)) {
        printf("ticks = 0x%08x\n", oldticks);
        qDump(); 
    }
    else goto usage;
    return (1);

 usage:
    printf ("Usage:\n%s\n", cmdtp->usage);
    return (1);
}

U_BOOT_CMD(
    etq,    2,  1,  do_etq,
    "etq     - event time queue\n",
    "\n"
    "etq dump\n"
    "    - dump event time queue\n"
);

