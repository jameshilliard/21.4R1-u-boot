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
#ifndef _FIFO_H
#define _FIFO_H

#define FIFO_SIZE 32

struct fifo {
	int data[FIFO_SIZE];
	int size;
	int head;
	int tail;
};

static __inline__ int fifo_next_head(struct fifo *fifo)
{
	return (fifo->head + 1) % fifo->size;
}

static __inline__ int fifo_next_tail(struct fifo *fifo)
{
	return (fifo->tail + 1) % fifo->size;
}

static __inline__ int fifo_empty(struct fifo *fifo)
{
	return fifo->head == fifo->tail ? 1 : 0;
}

static __inline__ int fifo_full(struct fifo *fifo)
{
	return fifo_next_tail(fifo) == fifo->head ? 1 : 0;
}

static __inline__ int fifo_size(struct fifo *fifo)
{
	if (fifo->head <= fifo->tail)
		return fifo->tail - fifo->head;
	else
		return (fifo->size - fifo->head) + (fifo->tail - 0);
}

static __inline__ int fifo_dequeue(struct fifo *fifo, int *data)
{
	if (fifo_empty(fifo))
		return 0;
	*data = (fifo->data)[fifo->head];
	fifo->head = fifo_next_head(fifo);
	return 1;
}

static __inline__ int fifo_enqueue(struct fifo *fifo, int data)
{
	if (fifo_full(fifo))
		return 0;
	fifo->data[fifo->tail] = data;
	fifo->tail = fifo_next_tail(fifo);
	return 1;
}

static __inline__ void fifo_init(struct fifo *f)
{
	f->head = f->tail = 0;
	f->size = FIFO_SIZE;
}

#endif
