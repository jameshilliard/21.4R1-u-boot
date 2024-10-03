/*
 * Copyright (c) 2009-2010, Juniper Networks, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/************************************************************************
   Author: Bilahari Akkriaju
 
   Description: This file contains the code to 
         * Read   from eeprom thats on i2c
         * write  to  eeprom thats  on i2c
         * erase eeprom over i2c
*************************************************************************/
#include "rmi/types.h"
#include "rmi/board_i2c.h"
#include <exports.h>
#include <common.h>
#include <stdarg.h>

#define MAX_EEPROM_DEVICES 1


#define EEPROM_LOG_INFO_OFFSET  0x70
#define EEPROM_LOG_AREA_START   0x80
#define EEPROM_LOG_AREA_SIZE    1004 
#define EEPROM_LOG_AREA_END    (EEPROM_LOG_AREA_START + EEPROM_LOG_AREA_SIZE) 


extern int i2c_read(uint i2c_bus, uchar chip, uint addr,  uchar *buffer, int len);
extern int i2c_write(uint i2c_bus, uchar chip, uint addr, uchar *buffer, int len);
extern int i2c_write_fill(uint i2c_bus, uchar chip, uint addr, uchar *ch, int count);
extern void *memset(void * s,int c,size_t count);

typedef struct
{
   int nfb_offset;    // next free byte offset
   int nom;           // number of msgs
   char resrvd[8];   // Reserved for future use
}tlogInfo_ST;

typedef struct eeprom_dev_api
{
    int  (*read)(uint index, int offset, char *,int nbytes);
    int  (*write)(uint index,  int offset,char *, int nbytes);
    int  (*erase)(uint index, int start, int nbytes);
}tEEPROM_i2c_API_ST;

typedef struct eeprom_on_i2c
{
    uint     i2c_bus;
    uint16_t i2c_addr;
    void    *eeprom_api;
}tEEPROM_i2c_DEV_ST;


tEEPROM_i2c_DEV_ST eeprom[MAX_EEPROM_DEVICES];

tEEPROM_i2c_API_ST  api;

#ifdef CONFIG_FX
#define     EEPROM_BUS      0x00 
/* Chnage this back to 51 once the 16bit EEPROM is ready*/
#define     EEPROM_ADDR     0x51
#define     ERR_LOG_EEPROM  0 
#else
#define     EEPROM_BUS      0x1
#define     EEPROM_ADDR     ARIZONA_EEPROM_ADDR
#define     ERR_LOG_EEPROM  0
#endif

/*********************************************************************
  Desc : Reading eeprom over i2c
  Input: index - eeprom number
         offset - offset byte in eeprom
         get_buffer - buffer to which eeprom contents are fetched
          nbytes    - number of bytes to write

  output:  Returns -1 if it fails. 
           Returns nbytes  if it passes.

*********************************************************************/

int i2c_eeprom_read(uint index, int offset, char *get_buffer, int nbytes)
{

   if(i2c_read(eeprom[index].i2c_bus,eeprom[index].i2c_addr, offset, get_buffer, nbytes) != 0 )
   {
      printf("\nReading eeprom over i2c failed"); 
      return -1;
   }
   else
   {
      return nbytes;
   }
}

/**********************************************************************
  Desc :  Writing eeprom over i2c
  Input: index - eeprom number
         offset - offset byte in eeprom
         put_buffer - buffer of contents being written 
         nbytes    - number of bytes to write

  output:  Returns -1 if it fails. 
           Returns nbytes  if it passes.
**********************************************************************/
int i2c_eeprom_write(uint index, int offset, char *put_buffer, int nbytes)
{
   if(i2c_write(eeprom[index].i2c_bus,eeprom[index].i2c_addr, offset, put_buffer, nbytes) != 0 )
   {
      printf("\nWriting to eeprom over i2c failed");
      return -1;
   }
   else
   {
     return nbytes;
   }
}

/**********************************************************************
  Desc :  Erasing eeprom over i2c
  Input:  index - eeprom number
          start - start offset in eeprom
          nbytes - number of bytes to erase from start 
 
  output:  Returns -1 if it fails. 
           Returns nbytes  if it passes.
**********************************************************************/
int i2c_eeprom_erase(uint index, int start,int nbytes)
{
    char erase_ch = 0xff;  

    printf("\nErasing eeprom log");

    if(i2c_write_fill(eeprom[index].i2c_bus,eeprom[index].i2c_addr,start,&erase_ch,nbytes) != 0 )
    {
        printf("\nFailed to erase the eeprom"); 
        return -1;
    }
    else
    {
       return nbytes;
    } 
}

/**************************************************************************
    Desc: Initializes the Log area
    Input: void
    Output: void
**************************************************************************/
void eeprom_log_init()
{
   tlogInfo_ST log_head;

   memset((char *)&log_head,0,sizeof(tlogInfo_ST));

   api.read(ERR_LOG_EEPROM, EEPROM_LOG_INFO_OFFSET, (char *)&log_head, sizeof(tlogInfo_ST));  

   /* Fresh log - number of messages equal to zero */
   if(log_head.nom == 0x00 )
   {
      log_head.nfb_offset = EEPROM_LOG_AREA_START;
      api.write(ERR_LOG_EEPROM, EEPROM_LOG_INFO_OFFSET, (char *)&log_head, sizeof(tlogInfo_ST));
   }
}

/**************************************************************************
  Desc:  Dump the messages in the log to console
  Input:  void
  output: void 
 
**************************************************************************/
int eeprom_log_dump()
{
   unsigned char buffer[EEPROM_LOG_AREA_SIZE];

   unsigned int nb,i = 0;

   tlogInfo_ST log_head;

   memset((char *)&log_head,0,sizeof(tlogInfo_ST));

   if(i2c_eeprom_read(ERR_LOG_EEPROM, EEPROM_LOG_INFO_OFFSET, 
                                 (char *)&log_head,sizeof(tlogInfo_ST) ) < 0)
   {
       printf("\nError reading log head from eeprom in %s",__func__);
       return -1;
   }

   nb = (log_head.nfb_offset -  EEPROM_LOG_AREA_START);
 
   //printf("\nlog nfb offset is %x %x",log_head.nfb_offset,nb);

   if(i2c_eeprom_read(ERR_LOG_EEPROM, EEPROM_LOG_AREA_START, buffer,nb) < 0)
   {
      printf("\nError reading log contents from eeprom in %s",__func__); 
       return -1;
   }
 
   while( i <= nb )
   {
       if(buffer[i] == '\0')
           printf("\n");
       else 
           printf("%c",buffer[i]);

       i++;
   }

  return 0;
}

/**************************************************************************
  Desc:   init of the eeprom device data structure
  Input:  void
  Output: void
**************************************************************************/
void eeprom_init()
{
   eeprom[ERR_LOG_EEPROM].i2c_bus   = EEPROM_BUS;
   eeprom[ERR_LOG_EEPROM].i2c_addr  = EEPROM_ADDR;

   api.read  = i2c_eeprom_read;
   api.write = i2c_eeprom_write;
   api.erase = i2c_eeprom_erase;

   eeprom[ERR_LOG_EEPROM].eeprom_api = (tEEPROM_i2c_API_ST *) &api;
}

/*************************************************************************
  Desc:  Erase eeprom log
  Input: void
  Output : number of bytes erased 
*************************************************************************/
int erase_eeprom_log()
{
  
   eeprom_log_init();
   
   return (((tEEPROM_i2c_API_ST *)(eeprom[ERR_LOG_EEPROM].eeprom_api))->
                        erase(ERR_LOG_EEPROM, EEPROM_LOG_AREA_START,
                        (EEPROM_LOG_AREA_END-EEPROM_LOG_AREA_START)+1));
}


/*************************************************************************
  Desc: print to eeprom, works exactly like printf        
*************************************************************************/

int print_2_eeprom(const char *printbuffer, unsigned int nb)
{
     tlogInfo_ST log_head;  
     unsigned int i=0; 

     memset((char *)&log_head,0,sizeof(tlogInfo_ST));

     ((tEEPROM_i2c_API_ST *)(eeprom[ERR_LOG_EEPROM].eeprom_api))->
                 read(ERR_LOG_EEPROM, EEPROM_LOG_INFO_OFFSET,(char *)&log_head,sizeof(tlogInfo_ST));


     if( (log_head.nfb_offset+nb+1)  > EEPROM_LOG_AREA_END )
     {  
          printf("Nfb offset is %x",log_head.nfb_offset+nb+1);  
          printf("\nInsuficient space in err log, erase old messages");
          return -1;
     }

     /* Write the buffer */
     i= ((tEEPROM_i2c_API_ST *)(eeprom[ERR_LOG_EEPROM].eeprom_api))->
                   write(ERR_LOG_EEPROM,log_head.nfb_offset,printbuffer,nb+1);
          
     if( i > 0 )
     {
         log_head.nfb_offset += (nb+1); 
         log_head.nom += 1;
      
         ((tEEPROM_i2c_API_ST *)(eeprom[ERR_LOG_EEPROM].eeprom_api))->
                    write(ERR_LOG_EEPROM,EEPROM_LOG_INFO_OFFSET,(char *)&log_head,sizeof(tlogInfo_ST));
     }

     return i; 
}

int eeprom_printf(const char *fmt, ...)
{
        va_list args;
        unsigned int nb;
        char printbuffer[120];

        va_start (args, fmt);
        
        nb = vsprintf (printbuffer, fmt, args);
        
        va_end (args);
        
        printbuffer[nb] = '\0';      //can be any delimiter 

       return( print_2_eeprom(printbuffer,nb));        
}
