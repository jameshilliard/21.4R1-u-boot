#include "rmi/i2c_spd.h"
#include "rmi/on_chip.h"
#include "rmi/types.h"
#include "../../juniper/fx/soho/tor_cpld_i2c.h"

#include <exports.h>
#include <common.h>

/* 'C' i2c api for boot strapper image */
extern int _i2c_prb(unsigned char bus, unsigned int slave_addr);

extern int _i2c_tx(unsigned short slave_addr, unsigned short slave_offset,
        unsigned short bus, unsigned char dataout);

extern int _i2c_rx(unsigned short slave_addr, unsigned short slave_offset,
        unsigned short bus, unsigned char *retval);

extern void _i2c_init(void) ;

/***************************************************************
 * Desc: Reads over i2c from a device and fill up the buffer
 * Input: i2c_bus - bus number on which the device is present
 * chip  - slave address of the device
 * addr  - address offset on the device
 * buffer - buffer into which the contents are read
 * len    - number of bytes to be read.
 ****************************************************************/
int i2c_read(uint i2c_bus, uchar chip, uint addr, uchar *buffer, int len)
{
    unsigned short i;
    unsigned short slave_addr = (unsigned short) chip; 
    unsigned short bus = (unsigned short ) i2c_bus;
    unsigned short retval;
    unsigned short p;
    unsigned char ch;

    for(i=0;i<len;i++)  
    {
        if(_i2c_rx(slave_addr, (addr+i), bus, &ch) != 0)
        {
            return -1;
        }
        else 
        {
            buffer[i]= ch;
            i2c_delay();
        }
    }
    return 0;
}

/*****************************************************************
 * Desc: Fills up a slave device space contiguosly with a byte value
 * Input: bus  - i2c bus number on which the device is present
 * chip - the slave device address on the bus
 * addr - offset on the slave device
 * ch   - byte value to write to the device
 * len  - number of times it should be written
 * Output: 0  - success
 * -1 - failure
 *****************************************************************/

int i2c_write_fill(uint i2c_bus, uchar chip, uint addr, uchar *ch, int count)
{
    unsigned short i;

    unsigned short slave_addr = (unsigned short) chip; 
    unsigned short bus = (unsigned short ) i2c_bus;
    unsigned short retval;

    for(i=0; i<count; i++) 
    {
        if(_i2c_tx(slave_addr,(addr+i),bus,*ch) != 0 )
        {
            return -1;
        }
        i2c_delay();
    }

    return 0;
}

/*****************************************************************
 * Desc: writes a bufferover i2c bus to a slave device 
 * Input: bus  - i2c bus number on which the device is present
 * chip - the slave device address on the bus
 * addr - offset on the slave device
 * buffer - contents to write to the device
 * len    - number of bytes to write
 * Output: 0  - success
 * -1 - failure
 *****************************************************************/

int i2c_write(uint i2c_bus, uchar chip, uint addr, uchar *buffer, int len)
{
    unsigned short i;

    unsigned short slave_addr = (unsigned short) chip; 
    unsigned short bus = (unsigned short ) i2c_bus;
    unsigned short retval;

    for(i=0; i<len; i++) 
    {
        i2c_delay();
        i2c_delay();
        if(_i2c_tx(slave_addr,(addr+i),bus,*(buffer+i)) != 0 )
        {
            return -1;
        }
    }
    return 0;
}

#define MAX_ALEN 4
#define MAX_BURST_LEN 64

/*****************************************************************
 *  Desc:   Seq Read for 16,24,32 etc bit slave devices 
 *  Input:   bus  - i2c bus number on which the device is present
 *           chip - the slave device address on the bus
 *           addr - offset on the slave device
 *           alen - Slave device word length (8-bit:0  16-bit:2 32-bit:4)
 *           buffer - contents read into this buffer
 *           len    - number of bytes to write
 *  Output:  0 if all of specified number of bytes are read successfully
 *          or -1 if failed to read
 *****************************************************************/

int 
i2c_seq_rd(uint bus, uint chip, uint addr, uint alen, u_char *tot_buffer, int len){
    u_char offset_buff[MAX_ALEN]={0};
    int i;
    uint32_t i2c_mode;
    
    if (fx_use_i2c_cpld(chip)) {        
        i2c_mode = (alen < 2)  ?  TOR_CPLD_I2C_8_OFF : TOR_CPLD_I2C_16_OFF;
        if (tor_cpld_i2c_read(bus, chip, addr, i2c_mode, tot_buffer, len) != 0){
            printf("Error reading %d bytes from 0x%x on i2c bus %x\n",len, chip,bus);
            return -1;
        }
        return 0;
    }


    /* The offsets are sent via the same buffer into which data is read back */
    for(i=0; i<alen; i++){
        offset_buff[i] = (unsigned char)(addr >> ((alen-1-i)*8));
        tot_buffer[i] = offset_buff[i];
    }

    if(_i2c_rx_alen_seq(chip, bus, alen, tot_buffer, len) != 0){
        printf("Error reading %d bytes from 0x%x on i2c bus %x\n",len, chip,bus);
        return -1;
    }

    return 0;
}

/*****************************************************************
 *  Desc:   Seq Write for 16,24,32 etc bit slave devices 
 *  Input:   bus  - i2c bus number on which the device is present
 *           chip - the slave device address on the bus
 *           addr - offset on the slave device
 *           alen - Slave device word length (8-bit:0  16-bit:2 32-bit:4)
 *           buffer - contents written out from this buffer
 *           dlen    - tot number of data bytes to write
 *  Output:  0 if all of specified number of bytes are written successfully
 *          or -1 if failed to read
 *****************************************************************/
int 
i2c_seq_wr(uint bus, uint chip, uint addr, uint alen, u_char *tot_buffer, int dlen){
    volatile int i, j = 0, nodb; /* nodb : number of data bytes */
    u_char offset_buff[4]= {0};
    uint32_t i2c_mode;
    uint32_t mux = bus;

    /*printf("%s %x %x %x %x\n", __FUNCTION__, addr, chip, bus, dlen); */

    /* number of bytes (nob) to be written should accomadate for alen */

    
    if (fx_use_i2c_cpld(chip)) {
        udelay(5000);
        i2c_mode = (alen < 2)  ?  TOR_CPLD_I2C_8_OFF : TOR_CPLD_I2C_16_OFF;

        if (tor_cpld_i2c_write(bus, chip, addr, i2c_mode, tot_buffer, dlen)) {
            printf("%s: Failed to write to addr 0x%x on device 0x%x of i2c bus %x\n",__func__, 
                    addr,chip,bus);
            return -1;
        }
        return 0;
    }

    while(dlen > 0)
    {
        for (i = 0; i < alen; i++) {
            offset_buff[i] = (u_char) (addr >> ((alen-1-i) * 8));
        }

        if(_i2c_tx_alen_seq(chip, bus, alen, offset_buff, &tot_buffer[j], 1) != 0 ) {
            printf("Failed to write to addr 0x%x on device 0x%x of i2c bus %x\n", addr, chip, bus);
            return -1;
        }

        i2c_delay(11000); /* wait for a 11 ms after a write burst */

        addr  += 1 ; /* Next address */
        dlen  -= 1;
        j++;
    }

    return 0;
}

/*****************************************************************
 * Desc: probes the i2c bus for slave device 
 * Input: bus - i2c bus number on which the device is present
 * slave_addr - the device address on the bus
 * Output: 0 - success
 *         -1 - failure
 *****************************************************************/
int i2c_probe(unsigned char bus, unsigned int slave_addr)
{
    return (_i2c_prb(bus,slave_addr));
}


