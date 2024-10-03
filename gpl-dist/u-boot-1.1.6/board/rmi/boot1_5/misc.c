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
#include "rmi/debug.h"

#include <configs/fx.h>
extern void somedelay(void);

extern void *_ftext, *_fdata, *_end;
extern unsigned char __stack[];
extern unsigned char _aligned_stack[];
extern void printf (const char *fmt, ...);
extern int i2c_write_fill(unsigned int i2c_bus, unsigned char chip, unsigned int addr, unsigned char *ch, int count);
extern int i2c_read(unsigned int i2c_bus, unsigned char chip, unsigned int addr, unsigned char *buffer, int len);

#define DISP_LINE_LEN 16

static unsigned int get_stack_size(void)
{
	return (unsigned int)((char *)__stack - (char *)_aligned_stack);
}

void prog_info(int argc, char *argv[])
{
    printf("\n");
    printf("_ftext:    %p   \n", &_ftext);
    printf("_fdata:    %p   \n", &_fdata);
    printf("_aligned_stack   %p   \n", &_aligned_stack);
    printf("_stack:    0x%lx\n", (long)__stack);
    printf("_end:      %p   \n", &_end);
    printf("Dcache lock size:      %x   \n", 
		    (unsigned long)__stack - (unsigned long) _fdata);
    printf("stacksize: 0x%x (%d KB)\n", get_stack_size(), get_stack_size()/1024);
}

int strlen (const char * s)
{
    const char *t = s;
    while (t != '\0')
    {
        t++;
    }
    return (t - s);
}    

   /*This is a simple strtoul function that converts the given hexadecimal
    * number of the form 0x12345678 to an unsigned long integer */

unsigned long strtoul (const unsigned char *c)
{
    unsigned long result = 0;
    unsigned int value = 0;


    if (!((*c == '0') && ((*(c + 1) == 'x') || (*(c + 1)) == 'X')))
    {
        printf("\n Incorrect value.. Please provide valid hex value of the form 0x12345678 \n");
        return 0;
    }
    c++; c++;
    while (*c != '\0')
    {
        if ((*c >= '0') && (*c <= '9')) 
        {
            value = ((*c) - '0');
        }
        else if ((*c >= 'a') && (*c <= 'f'))
        {
            value = (*c - 'a') + 10;
        }
        else if ((*c >= 'A') && (*c <= 'F'))
        {
            value = (*c - 'A') + 10;
        }
        else
        {
            printf("\n Unknown value detected in hex \n");
            return 0;
        }
        result = (result * 16) + value;
        c++;
    }

    return result;
}    
           
int get_data_size(char *arg)
{
    int len = strlen (arg);
    if (len > 2 && arg[len-2] == '.') 
    {
        switch(arg[len-1]) 
        {
            case 'b':
                return 1;
                break;
            case 'w':
                return 2;
                break;
            case 'l':
                return 4;
                break;
            default:
                printf("\n Unknown display quantifier. Please use .b, .w or .l \n");
                return -1;
                break;
        }
    }
    else
    {
        return 4;
    }    

}

/* reg 15, reg 16 are temperory  registers in MIPS and are
 * used in the following utilities.Inline assembly used
 * wherever boot1_5 (bootstrap) code is called.These utilities
 * run from cache and hence call bootstrap fuctions implemented
 * in assembly.
 * - Bilahari Akkiraju
 */

/* mem_read needs to be made generic, right now it only loads
 * a word from specific location to reg 15
 */

void mem_read (int argc, char **argv)
{
    int size = 0, j;
    unsigned long address;
    unsigned long length, num_bytes, linebytes;

    __asm__ __volatile__ ("lw $15, 0xffffffffa0000800 \n\t");

   /* 
    if (argc < 2)
    {
        printf("Usage: md[.b, .w, .l]  <Address> [# of objects] \n");
        return;
    }

    if ((size = get_data_size (argv[0]) < 1))
    {
        return;
    }    

    if ((address = strtoul(argv[1]) <= 0)) 
    {
        return; 
    }

    if (argc > 2)
    {
        if ((length = strtoul (argv[2]) <= 0))
            return; 
    }    
    else
    {
        length = 1;
    } 

    num_bytes = length * size;

    do 
    {
        char linebuf[DISP_LINE_LEN];
        unsigned long i = 0;
        unsigned int    *uip = (unsigned int   *)linebuf;
        unsigned short  *usp = (unsigned short *)linebuf;
        unsigned char  *ucp = (unsigned char *)linebuf;
        unsigned char *cp;
        
        printf("\n%08lx:", address);
        linebytes = (num_bytes>DISP_LINE_LEN)?DISP_LINE_LEN:num_bytes;

        for (i=0; i<linebytes; i+= size)
        {
            if (size == 4)
            {
                printf(" %08x", (*uip++ = *((unsigned int *)address)));
            }
            else if (size == 2) 
            {
                printf(" %04x", (*usp++ = *((unsigned short *)address)));
            } 
            else
            {
                printf(" %02x", (*ucp++ = *((unsigned char *)address)));
            }
            address += size;
        }

        printf("    ");
        cp = (unsigned char *)linebuf;
        
        for (i=0; i<linebytes; i++) 
        {
            if ((*cp < 0x20) || (*cp > 0x7e))
                printf (".");
            else
                printf("%c", *cp);
            cp++;
        }
        printf ("\n");
        num_bytes -= linebytes;
   
    } while (num_bytes > 0);
*/
}

/* mem_write1 & mem_write0 are specific location writes to
 * check the functionality while code is running from flash.
 * Will be expanded to generic mode in future checkins 
 */

void mem_write1 (int argc, char **argv)
{
     __asm__ __volatile__ ("li $15, 0x1  \n\t"
                           "li $16, 0xffffffffa0000800\n\t"
                           "sd $15, 0xffffffffa0000800 \n\t"
                           "cache   0x15, 0($16)\n\t" 
                          "nop"
                         );
}

void mem_write0 (int argc, char **argv)
{
    int size = 0;
    unsigned long writeval, address, count;

    __asm__ __volatile__ ("li $15, 0x0  \n\t"
                           "li $16, 0xffffffffa0000800\n\t"
                           "sd $15, 0xffffffffa0000800 \n\t"
                          "cache   0x15, 0($16)\n\t" 
                          "nop"
                         );
/*
    if ((argc < 3) || (argc > 4))
    {
        printf("\n Usage: mw [.b, .w, .l] address value [count] \n");
        return;
    }

    if ((size = get_data_size(argv[0])) < 1)
    {
        return;
    }

    if ((address = strtoul(argv[1]) == 0))
    {
        return;
    }

    if ((writeval = strtoul(argv[2])) == 0)
    {
        printf("\n Writing 0's to the address \n");
    }

    if (argc == 4)
    {
        count = strtoul(argv[3]);
    }
    else
    {
        count = 1;
    }    
 
    while (count-- > 0)
    {
        if (size ==4)
        {
            *((unsigned long *)address) = (unsigned long)writeval;
        }
        else if (size == 2)
        {
            *((unsigned short *)address) = (unsigned short)writeval;
        }
        else
        {
            *((unsigned char *)address) - (unsigned char)writeval;
        }
        address += size;
    }
*/
    return;
}

/* Full memory test runs from cache, and using a variable in
 * data cache might clear it up as we flush the data cache during 
 * a full memory test. Thats why we use register qualifier to keep 
 * track of iteration count.
 *
 *  Full memory test implemented in boot1_5 section is in Assembly
 *  as its too early in the init and there is no stack to cal C
 *  function. We reuse that code from RMI and hence have made a
 *  call to it from assembly.
 */
void fmtest(int argc, char *argv[ ]){
   register volatile unsigned long int i =0;

   if (argc == 2) {

       printf("\n Running memory test indefinitely.\n");
       printf(" Please power cycle board to end the test.\n");

       while (1) {
           __asm__ __volatile__ ("jal dram_bar_mats \n\t"
                                 "nop \n\t");
       }
   } 
   else if (argc == 1) {

       printf("\n");
       __asm__ __volatile__ ("jal dram_bar_mats \n\t"
                             "nop");
       /* Give it time before kicking the RMI reset pin */
       somedelay();
       somedelay();

       printf("All dram mats done\n");
       /* Put in a few LFs to get rid of garbage characters on reset */
       printf("\n\n");
       fx_get_board_type();
       fx_cpld_early_init();
       fx_cpld_issue_sw_reset();
   } else {
       printf("\n fmtest <loop> \n");
       printf("        If the parameter is provided, test will run forever. \n");
       printf("        If not it will run once. \n");
   }

   return;
}

void i2c_write_fill_cmd(int argc, char *argv[ ]){

  unsigned char bus, slave_addr,offset, len,ch;
 
  if(argc != 6 ) {
    printf("imd bus slave offset len\n");
    return;
  }

  bus        = (char ) strtoul(argv[1]);
  slave_addr = (char ) strtoul(argv[2]);
  offset     = (char ) strtoul(argv[3]);
  ch         = (char ) strtoul(argv[4]);
  len        = (char ) strtoul(argv[5]);

  i2c_write_fill(bus, slave_addr, offset, &ch, len); 
}

void i2c_read_cmd(int argc, char *argv[ ]){

  unsigned char i, buff[48], bus, slave_addr,offset, len;
 
  if(argc != 5 ) {
    printf("imd bus slave offset len\n");
    return;
  }

  bus        = (char ) strtoul(argv[1]);
  slave_addr = (char ) strtoul(argv[2]);
  offset     = (char ) strtoul(argv[3]);
  len        = (char ) strtoul(argv[4]);

  i2c_read(bus, slave_addr, offset, buff, len);
 
  printf("\n");
  for(i=0;i<len;i++)
   printf("0x%x ",buff[i]);
}


void dog_reset_disable(int argc, char *argv[ ]){
    fx_get_board_type();
    fx_cpld_early_init();
    fx_cpld_set_bootkey();
}

