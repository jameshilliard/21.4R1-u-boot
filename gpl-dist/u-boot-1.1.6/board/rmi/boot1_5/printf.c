
#include <stdarg.h>
#include <rmi/mipscopreg.h>
#include <rmi/mips-exts.h>

int vsprintf(char *buf, const char *fmt, va_list args);
extern void asm_printf(char *);
extern void uart_putchar(const char);

void printf (const char *fmt, ...)
{
	va_list args;
	unsigned int i;
	char printbuffer[1024];

	va_start (args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, fmt, args);
	va_end (args);

	/* Print the string */
	asm_printf(printbuffer);
}

void putc (const char c)
{
	uart_putchar(c);
}


void wwait(int nanosecs) {
     volatile int tmp= read_32bit_cp0_register($9)+nanosecs;
     while (tmp > read_32bit_cp0_register($9)) {
     }
}


void udelay (unsigned long usec)
{
    wwait(usec*1000); 
}

void vprintf (const char *fmt, va_list args)
{
	unsigned int i;
	char printbuffer[1024];

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, fmt, args);

	/* Print the string */
	asm_printf(printbuffer);
}

void do_reset(void)
{
}
