#include "rmi/fifo.h"
extern int outbyte(char c);
extern char inbyte_nowait(void);
extern int tstbyte(void);
extern void rmi_uart_fifo_init(void);
int serial_init(void) 
{ 
	rmi_uart_fifo_init();
	return (0);
}

int serial_getc (void)
{
	int ch;
	while(1) {
		ch = inbyte_nowait();
		if(ch != 0)
			break;
	}
	return ch;

}

int serial_tstc (void)
{
	return tstbyte();
}

void serial_putc (const char c)
{
	outbyte(c);
}

void serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}

}
void serial_setbrg (void)
{
	/* FIXME */
}
	
