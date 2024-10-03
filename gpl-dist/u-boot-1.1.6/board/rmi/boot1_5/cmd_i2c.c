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
#include "rmi/bridge.h"
#include "rmi/i2c_spd.h"
#include "rmi/board_i2c.h"
#include "rmi/pioctrl.h"
#include "rmi/gpio.h"
#include "rmi/on_chip.h"
#include "rmi/mem_ctrl.h"
#include "rmi/timer.h"
#include "asm/string.h"
#include "rmi/debug.h"

#include "rmi/psb.h" 
#include "rmi/boardconfig.h"

#ifdef CFG_BOARD_RMIREF
extern void printf (const char *fmt, ...);

void somedelay(void);

/* This file has routines that access the EEPROM
 * specific to RMI Reference Boards. Enable these
 * only based on this global board-define.
 */

#define MAC_OFFSET ARIZONA_EEPROM_ETH_MAC_OFFSET

extern int spd_i2c_tx(unsigned short addr, unsigned short offset, 
                  unsigned short bus, unsigned short dataout);
    

extern int spd_i2c_rx(unsigned short addr, unsigned short offset,
                  unsigned short bus, unsigned short *retval);

extern void uart_flush_tx_buf(void);
extern int tstbyte(void);
extern char inbyte(void);
extern int outbyte(char c);
            
unsigned short eeprom_data[0x30] = {
    0x60,0x0d,0xbe,0xef,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x07,
    0x00,0x0f,0x30,0xff,0xff,0xff,0x01,0x04,
    0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff
};

struct boardz {
    int major;
    int minor;
    const char name[7];
};

static const struct boardz supportedBoards[] = {
	 {.major = 1,    .minor = MINOR_A,      .name = "ARIZONA"},
	 {.major = 1,    .minor = MINOR_B,      .name = "ARIZONA"},
	 {.major = 1,    .minor = MINOR_C,      .name = "ARIZONA"},
	 {.major = 1,    .minor = MINOR_XLS,    .name = "ARIZONA"},
	 {.major = 2,    .minor = MINOR_A,      .name = "ARIZONA"},
	 {.major = 2,    .minor = MINOR_B,      .name = "ARIZONA"},
	 {.major = 3,    .minor = MINOR_A,      .name = "PCIX   "},
	 {.major = 3,    .minor = MINOR_B,      .name = "PCIX   "},
	 {.major = 4,    .minor = MINOR_A,      .name = "ARIZONA"},
	 {.major = 5,    .minor = MINOR_A,      .name = "PCIX   "},
	 {.major = 6,    .minor = MINOR_A,      .name = "ARIZONA"},
	 {.major = 7,    .minor = MINOR_XLS,    .name = "ARIZONA"},
	 {.major = 8,    .minor = MINOR_XLS,    .name = "ARIZONA"},
	 {.major = 11,   .minor = MINOR_XLS,    .name = "ARIZONA"},
	 {.major = 11,   .minor = MINOR_XLS_B,  .name = "ARIZONA"},
	 {.major = 12,   .minor = MINOR_XLS,    .name = "ARIZONA"},
	 {.major = 12,   .minor = MINOR_XLS_B,  .name = "ARIZONA"},
	 {.major = 0xff, .minor = 0xff,         .name = "UNKNOWN"}
};

static unsigned short atod(char data_in){
	unsigned short rc=0;
	if((data_in>=0x30)&&(data_in<=0x39)) 
		rc=data_in-0x30;
	if((data_in>=0x41)&&(data_in<=0x46)) 
		rc=data_in-0x37;
	if((data_in>=0x61)&&(data_in<=0x66)) 
		rc=data_in-0x57;
	return rc;
}  

static void read_eeprom_all(unsigned short which_eeprom) {
	unsigned short res;
	unsigned int i;
	printf("\n");
	for (i=0;i<0x30;i++) {
		spd_i2c_rx(which_eeprom,i,I2C_EEPROM_BUS,&res);
		if((i>=0x0)&&(i<=0x3)){

			if(i==0)
				printf("signature value (0~3) is 0x%02x",res);
			else if (i==3)
				printf("%02x\n",res);
			else
				printf("%02x",res);
    
		}else if((i>=0x10)&&(i<=0x17)&&(res!=0xff)){
			if(i==0x10)
				printf("board type string (0x10~0x17) is %c", res);
			else if(i==0x17)
				printf("%c\n",res);
			else
				printf("%c",res);

		}else if((i>=0x20)&&(i<=0x25)){
			if(i==0x20)
				printf("mac address (0x20~0x25) is %02x:", res);
			else if (i==0x25)
				printf("%02x\n", res);
			else
				printf("%02x:",res);
    
		}else if(i==0x18){
			printf("eeprom major (0x18) is 0x%02x\n", res);
		}else if(i==0x19){
			printf("eeprom minor (0x19) is 0x%02x\n", res);
		}else if(i==0x1a){
			printf("board rev# string (0x1a 0x1b) is %c", res); 
		}else if(i==0x1b){
			printf("%c\n",res);
		}else if(i==0x1c){
			printf("board serial# string (0x1c 0x1d) is %c", res); 
		}else if(i==0x1d){
			printf("%c\n",res);
		}else if(i==0x1f){
			printf("eeprom offset 0x1f is 0x%02x\n", res);
		}else if(i==0x26){
			printf("qdr2 (0x26) is 0x%02x\n", res);
		}else if(i==0x27){
			printf("qdr2_size (0x27) is 0x%02x\n", res);
		}else{
			printf("  0x%02x:0x%02x\n", i, res);
		}
	}
	printf("\n");
}

void printf_usage(void) {
	printf("DO NOT hit return key after entering digit\n\r");
	printf("Type ESC to quit EEPROM\n\r");
}

void do_i2c_eeprom(int argc, char* argv[]) {
	I2C_REGSIZE *iobase_i2c_regs = 0;
	unsigned int i, j;
	iobase_i2c_regs = get_i2c_base(I2C_EEPROM_BUS);

	char datains[6]={'\0'};
	char datain='\0';
	unsigned short temp=0xff;
    unsigned int binaryIn = 0;
    struct boardz *this_board = (struct boardz *)&supportedBoards[0];

	memset(datains, '\0', sizeof(datains));
  
    printf("\nEEPROM table contents before programming...\n");
	read_eeprom_all(I2C_EEPROM_ADDR);

reenter_boardtype:
    printf("\nEnter 2-DIGIT Board Major Number:\n\r");
    printf("[Valid Values: 01/02/03/04/05/06/07/08/11/12]\n\r");
	printf_usage();
  
    this_board = (struct boardz *)&supportedBoards[0];

    for(i=0; i<2; i++) {
        datains[i] = inbyte();
        if((datains[i] < 0x30) || (datains[i] > 0x39)) {
            if(datains[i]==0x1b){
                printf("\n==== Exiting EEPROM Programming ====\n");
                return;
            }
            else {
                printf("\nUnsupported Board Major Number.\n");
                goto reenter_boardtype;
            }
        }
        outbyte(datains[i]);
    }

    /* Validate */
    binaryIn = (datains[0] - 0x30) * 10 + (datains[1] - 0x30);
    for(; ((this_board->major) && (this_board->major != 0xff)); ) {
        if(binaryIn == this_board->major) {
            goto major_match;
        }
        this_board++;
    }
    goto reenter_boardtype;

major_match:    
    if (datains[0] == '0') {
        eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+7] = this_board->major + 0x30;
    }
    else {
        eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+7] = this_board->major + 0x37;
    }
    
    switch (eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+7]) {
        case '1':
        case '2':
        case '4':
        case '6':
        case '7':
        case '8':
        case 'B':
        case 'C':
            {
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+0]='A';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+1]='R';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+2]='I';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+3]='Z';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+4]='O';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+5]='N';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+6]='A';
                if ((eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+7]=='B') ||
                        (eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+7]=='C')) {
                    eeprom_data[ARIZONA_EEPROM_MAJOR_OFFSET]=this_board->major + 0x37;
                }
                else {
                    eeprom_data[ARIZONA_EEPROM_MAJOR_OFFSET]=this_board->major + 0x30;
                }
reenter_minor:
                printf("\nEnter 1-DIGIT Board Minor Nr.\n\r");
                printf_usage();
                memset(datains, '\0', sizeof(datains));
                datain='\0';
                while(!datain) {
                    datain = inbyte();
                }

                if ((datain>=0x30) && (datain<=0x39)) {
                    eeprom_data[ARIZONA_EEPROM_MINOR_OFFSET] = datain;
                    printf("\nRegistering Board Minor# [%c]\n", datain);
                } else {
                    printf("\nUnsupported Board Minor Number.\n");
                    if(datain == 0x1b) {
                        printf("\n\n\n==== Exit EEPROM Programming ====\n\n\n");
                        return;
                    }
                    goto reenter_minor;
                }
            }
            break;

        case '3':
        case '5':
            {
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+0]='P';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+1]='C';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+2]='I';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+3]='X';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+4]=' ';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+5]=' ';
                eeprom_data[ARIZONA_EEPROM_BNAME_OFFSET+6]=' ';
                eeprom_data[ARIZONA_EEPROM_MAJOR_OFFSET]=datain;
reenter_memsize:
                printf("\nEnter PCIX-card Memory Size, [512M/256M]\n\r");
                printf_usage();
                memset(datains, '\0', sizeof(datains));
                for(i=0;i<3;i++) {
                    datains[i]=inbyte();
                    if(datains[i]==0x1b) {
                        printf("\n\n\n==== Exit EEPROM Programming ====\n\n\n");
                        return;
                    }
                    printf("%c",datains[i]);
                    outbyte(datains[i]);
                }

                if(((datains[0]!='5')||(datains[1]!='1')||(datains[2]!='2')) &&
                   ((datains[0]!='2')||(datains[1]!='5')||(datains[2]!='6'))) {
                    printf("\n\ninvalid memory size\n");
                    goto reenter_memsize;
                }

                if(datains[0]=='5') {
                    eeprom_data[ARIZONA_EEPROM_MINOR_OFFSET]='1';
                } else if(datains[0]=='2') {
                    eeprom_data[ARIZONA_EEPROM_MINOR_OFFSET]='0';
                } else {;/* Do nothing */}
            }
            break;
        default:
            printf("Unsupported Board Major Nr.\n");
            break;
    }

reenter_board_revison:
	printf("\n\nEnter two character Board Revision Number,\n\r");
	printf("E.g. 'c2', or space+char if Revision is One Character\n\r");
	printf_usage();
	memset(datains, '\0', sizeof(datains));
	for(i=0;i<2;i++){
		datains[i] = inbyte();
		if(datains[i]==0x1b){
			printf("\n\n\n===== exit eeprom =====\n\n\n");
			return;
		}
		outbyte(datains[i]);
		if((datains[i]<0x30)||((datains[i]>0x39)&&(datains[i]<0x41))||
		   ((datains[i]>0x5a)&&(datains[i]<0x61))||(datains[i]>0x7a)){

			printf("\n\nInvalid Revision Number\n");
			goto reenter_board_revison;
		}
    
	}
	eeprom_data[ARIZONA_EEPROM_BOARDREV_OFFSET]=datains[0];
	eeprom_data[ARIZONA_EEPROM_BOARDREV_OFFSET+1]=datains[1];
  
reenter_board_serial:
	printf("\n\nEnter 2 digit Board Decimal Serial Nr.\n\r");
	printf_usage();
	memset(datains, '\0', sizeof(datains));
	for(i=0;i<2;i++){
		datains[i]=inbyte();
		if(datains[i]==0x1b){
			printf("\n\n\n===== exit eeprom =====\n\n\n");
			return;
		}
		outbyte(datains[i]);
		if((datains[i]<0x30)||(datains[i]>0x39)){

			printf("\n\nInvalid Serial Number\n");
			goto reenter_board_serial;
		}
		if(datains[i]==0x0a){
			break;
		}
    
	}
	eeprom_data[ARIZONA_EEPROM_BOARD_ID_OFFSET0]=datains[0];
	eeprom_data[ARIZONA_EEPROM_BOARD_ID_OFFSET1]=datains[1];
  
reenter_macaddr:
	printf("\n\nEnter last 3 bytes of MAC address in format ######\n\r");
	printf("Do not omit 0. E.g. 0003e2, 00050a\n\r");
	printf_usage();
	memset(datains, '\0', sizeof(datains));
	for(i=0;i<6;i++){
		datains[i]=inbyte();
		if(datains[i]==0x1b){
			printf("\n\n\n===== exit eeprom =====\n\n\n");
			return;
		}
		outbyte(datains[i]);
		if((datains[i]<0x30)||((datains[i]>0x39)&&(datains[i]<0x41))||
		   ((datains[i]>0x46)&&(datains[i]<0x61))||(datains[i]>0x66)){

			printf("\n\nInvalid Mac Address\n");
			goto reenter_macaddr;
		}
    
	}

	eeprom_data[MAC_OFFSET+3]=(atod(datains[1])|(atod(datains[0])<<4));
	eeprom_data[MAC_OFFSET+4]=(atod(datains[3])|(atod(datains[2])<<4));
	eeprom_data[MAC_OFFSET+5]=(atod(datains[5])|(atod(datains[4])<<4));

	printf("\n\n==== Entered DATA ====\n\r");

	printf("board type %c, %c%c%c%c%c%c%c%c\n\r", eeprom_data[0x17],
	       eeprom_data[0x10],eeprom_data[0x11],eeprom_data[0x12],
	       eeprom_data[0x13],eeprom_data[0x14],eeprom_data[0x15],
	       eeprom_data[0x16],eeprom_data[0x17]);
	printf("mac address: %02x:%02x:%02x:%02x:%02x:%02x\n\n",
	       eeprom_data[0x20], eeprom_data[0x21],
	       eeprom_data[0x22], eeprom_data[0x23],
	       eeprom_data[0x24], eeprom_data[0x25]);
  
	datain='\0';
	printf("Ready to program, enter any key. To quit, press ESC\n\r");
	while(!datain){
		datain = inbyte();
	}
	if(datain==0x1b) {
		printf("\n\n\n==== QUIT eeprom ====\n\n\n");
		return;
	}
  

	printf("\nEEPROM programming is proceeding... ...\n\r");

  
	for(i=0;i<0x30;i++){
		temp=0xff;
		if((i!=0)&&(i%0x08==0))
			printf("\n..%02x..", i);
		else
			printf("..%02x..", i);
		somedelay();
		spd_i2c_rx(I2C_EEPROM_ADDR, i, I2C_EEPROM_BUS, &temp);
		somedelay();
		if(temp==eeprom_data[i]) continue;
		somedelay();
		if(temp!=0xff){
        
			if(spd_i2c_tx(I2C_EEPROM_ADDR, i, I2C_EEPROM_BUS, 0xff))
				printf_err("i2c tx error\n");
		}
		somedelay();
		if(spd_i2c_tx(I2C_EEPROM_ADDR, i, I2C_EEPROM_BUS, eeprom_data[i]))
			printf_err("i2c tx error\n");
		somedelay();
	}
    
	somedelay();

	for(i=0;i<0x30;i++){

		temp=0xff;
		for(j=0;j<8000000;j++);
		spd_i2c_rx(I2C_EEPROM_ADDR, i, I2C_EEPROM_BUS, &temp);
		if(temp != eeprom_data[i]){
			printf("\n\n\nProgramming Error @ EEPROM offset %02x\n\r", i);
			printf("expected value %02x, eeprom value %02x\n\r", eeprom_data[i], temp);
			printf("\n\n\n======please run this program again======\n\n\n");
			return;
		}
	}
	printf("\n\nEEPROM Programmed, Listing New Table: \n\r"); 
	read_eeprom_all(I2C_EEPROM_ADDR);
	return;
}

#endif /* CFG_BOARD_RMIREF */
void somedelay(void){
        int j;
        for(j=0;j<80000000;j++);
}

