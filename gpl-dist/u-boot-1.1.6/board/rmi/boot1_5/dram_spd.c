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
#include "rmi/boardconfig.h"
#include "rmi/board_i2c.h"
#include "rmi/mem_ctrl.h"
#include "rmi/dram_eye_finder.h"
#include "rmi/on_chip.h"
#include "rmi/types.h"
#include "rmi/bridge.h"
#include "rmi/gpio.h"
#include "rmi/mips-exts.h"
#include "rmi/board.h"
#include "rmi/shared_structs_offsets.h"
#include "rmi/xlr_cpu.h"
#include "rmi/debug.h"
#ifdef CONFIG_FX
#include <configs/fx.h>
#include "../../juniper/fx/soho/tor_cpld_i2c.h"
#endif

/* Globals */
uint16_t assm_id = 0xffff;

/*
 * #define SPD_DEBUG
 */ 

extern char * strcpy(char * dest,const char *src);

struct boot1_info g_boot1_info[1] __attribute__((__aligned__(8)));

extern int register_shell_cmd(const char *name,
			      void (*func) (int argc, char *argv[]), 
                  const char *shorthelp);

typedef struct spd_entry {
	int      raw_data;
	int      whole;
	int      num;
	int      denom;
} spd_entry;

typedef struct rat_entry { /* rank allocation table entry */
	int           exists;
	unsigned long SPD_addr;
	int           CS_id;
	int           ODT_id;
	int           logical_rank_id;
} rat_entry;

enum mem_protocol {DDR1, DDR2, RLDRAM};

unsigned long phoenix_io_base = (unsigned long)(DEFAULT_PHOENIX_IO_BASE);
/* Function prototypes */
void dummy_phoenix_reg_write (unsigned long, int);
int read_spd_data (unsigned short, spd_entry *);
int channel_config(unsigned short, unsigned short, unsigned short, 
                   unsigned long, unsigned long, unsigned long, 
                   unsigned long, unsigned long, int *, enum mem_protocol *, 
                   int, int, unsigned long, int, unsigned long, int *, 
                   int *, int *, int *, int *, int, int *, int, char[]);
void init_spd_data (spd_entry *);
void print_spd(spd_entry *);
void check_spd_integrity(spd_entry *, spd_entry *, int[] );
void decode_spd_data (spd_entry *, enum mem_protocol *, int *, int *, int *, int, int *, int *);
void convert_spd_mixed_to_fraction (spd_entry *);
void convert_spd_array_to_fraction (spd_entry *);
void program_clock (spd_entry *, phoenix_reg_t *, unsigned long, unsigned long, unsigned long, unsigned long, int[], int[], int *, int *, int, int, int, char[]);
void convert_spd_fraction_to_cycles (spd_entry *, int);
void convert_spd_array_to_cycles (spd_entry *, int);
int  compute_AL(spd_entry *, enum mem_protocol *, int, int, int, int);
void program_bridge_cfg(phoenix_reg_t *, int, char[]);
void program_dyn_data_pad_ctrl(phoenix_reg_t *, int, int, char[]);
void program_glb_params(phoenix_reg_t *, spd_entry *, int, int, enum mem_protocol, char[]);
void program_strobe_configs (phoenix_reg_t *, int, char, int);
void init_RAT (unsigned short, unsigned short, spd_entry *, int [], rat_entry *, int, int);
void program_write_ODT_mask(phoenix_reg_t *, rat_entry *, int [], enum mem_protocol);
void program_read_ODT_mask(phoenix_reg_t *, rat_entry *, int [], enum mem_protocol);
void program_addr_params(phoenix_reg_t *, rat_entry *, spd_entry *, int [], int, int *, int *, int *, int *, int *, int *, int *, int *, int *);
void program_params_1(phoenix_reg_t *, spd_entry *, enum mem_protocol);
void program_params_2(phoenix_reg_t *, spd_entry *, int, int);
void program_params_3(phoenix_reg_t *, spd_entry *, int, int, int, int, int, enum mem_protocol, int);
void program_params_4(phoenix_reg_t *, spd_entry *, int, int, int, int, int, enum mem_protocol, int); 
void program_params_5(phoenix_reg_t *, spd_entry *);
void program_params_6(phoenix_reg_t *, int, int, int, int);
void program_params_7(phoenix_reg_t *, int, int, spd_entry *, enum mem_protocol);
void program_mrs(phoenix_reg_t *, spd_entry *, int, int, enum mem_protocol);
void program_rdmrs(phoenix_reg_t *, spd_entry *, int, int, enum mem_protocol);
void program_emrs1(phoenix_reg_t *, int, enum mem_protocol, int [] );
void program_tx0_therm_ctrl(phoenix_reg_t *, enum mem_protocol);
void program_rx0_therm_ctrl(phoenix_reg_t *, int []);
void dram_spd_exit(void);
void phoenix_verbose_write_reg (phoenix_reg_t *, int, uint32_t);
void program_reset_sequence(phoenix_reg_t *, enum mem_protocol);
void set_wait_func_limit(int, char[]);
void wait_func(void);
void program_eye_finder_bridge_map(phoenix_reg_t *, int, int);
int dram_page_thrash(int, int, int, int, int, int, int, int, int, int);
void run_test(int, int, int, int, int, int, int, int, int, int, int, int *, int);
int  find_center(int [], int);
void update_results(int, int, int, int);
void program_strobe_regs(phoenix_reg_t *, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
int  program_P34_activate(phoenix_reg_t *, spd_entry *, int, int, int, int, int, int, int);
void set_calibrate_range(int, int [], int, int *, int *, int *, int *, int *, int *, int *, int *, int, int);
void calibrate_channel_strobes(int, int, int, int, int, int, int, int, int, int, int, int, int, phoenix_reg_t *, phoenix_reg_t *, spd_entry *, enum mem_protocol, int, int [], int, int, char[]); 

int dummy_read_spd_eeprom(unsigned short, unsigned short, unsigned short *, int);

#ifdef SPD_DEBUG
void read_programmed_registers(phoenix_reg_t *, int, enum mem_protocol, int);
#endif

#if defined (SPD_TEST) && defined (SPD_DEBUG)
extern void memtester_main();
#endif

extern void spd_i2c_init(void);
extern int spd_i2c_tx(unsigned short slave_addr, unsigned short slave_offset,
                  unsigned short bus, unsigned short dataout);

extern char inbyte_nowait(void);

/* void prog_iobase(int argc, char *argv[]); */
int spd_i2c_rx(unsigned short i2c_bus_addr, unsigned short offset,
           unsigned short i2c_bus, unsigned short *retval);

extern void do_i2c_eeprom(int argc, char* argv[]);
int calc_mem_size(spd_entry *, int []);

int dram_read_spd_eeprom(unsigned short which_eeprom, 
        unsigned short which_param, unsigned short *retval);

static int board_spd_init(int, int);
static int program_dram_bridge(int, int, int, int, int, char[]); 
extern void shell_init(const char *prmpt);
extern void shell_run(void);
extern void shell_help(int argc, char *argv[]);
extern void read_eeprom(unsigned short offset, unsigned short *value);
extern int shell_loop;
extern int tstbyte(void);

#ifdef CFG_BOARD_RMIREF
static void read_slct_cpld_dram_config_reg(int *tck_denom, int *voltage, int xlr);
#endif

/* Note that for all boards with more than 256MB of total DRAM memory, BAR7 is reserved for   */
/* the 16MB NMI region, from 496MB to 512MB physical address.                                 */ 
/* It always maps to addresses 0x0_0000_0000 -> 0x0_00FF_FFFF in the dram address space. This */
/* mapping is programmed during boot2, in nmi.c.                                              */ 
/* Ideally, BAR7 would be programmed here along with the other DRAM BARs. However, this is not*/
/* possible since the flash interface is mapped to the 496MB -> 512MB physical address region */
/* at this point in the boot sequence. The flash interface is relocated later in the sequence.*/ 
/*                                                                                            */
/* Since the entire DRAM address space must be scrubbed prior to activating DRAM ECC, BAR7    */
/* is programmed here with a temporary mapping so that the scrubbing code in boot1_5 will not */
/* skip this region. This temporary mapping maps the same DRAM address range identified above,*/
/* but uses a physical address range dependent on the total amount of memory in the system.   */
/* The table below applies:                                                                   */
/*                            Physical address range    Physical address range DRAM address range             */
/* XLR/XLS total DRAM memory  of temporary BAR7 mapping of actual BAR7 mapping of both BAR7 mappings          */
/* =========================  ========================= ====================== ============================== */
/* 256MB                        none                    none                   none                           */ 
/* 512MB                        752M ->   768M          496MB -> 512MB         0x0_0000_0000 -> 0x0_00FF_FFFF */ 
/*   1GB                       1264M ->  1280M          496MB -> 512MB         0x0_0000_0000 -> 0x0_00FF_FFFF */ 
/*   2GB                       2288M ->  2304M          496MB -> 512MB         0x0_0000_0000 -> 0x0_00FF_FFFF */ 
/*   4GB                       4736M ->  4752M          496MB -> 512MB         0x0_0000_0000 -> 0x0_00FF_FFFF */ 
/*   8GB                       8704M ->  8720M          496MB -> 512MB         0x0_0000_0000 -> 0x0_00FF_FFFF */ 
/*  16GB                      16384M -> 16400M          496MB -> 512MB         0x0_0000_0000 -> 0x0_00FF_FFFF */ 
/*                                                                                            */
/* Following DRAM ECC scrubbing in boot1_5, the temporary mapping in BAR 7 will be erased. As */
/* mentioned above, the actual mapping is programmed in boot2.                                */
/*                                                                                            */

//base address
static	int ba[7][8]={ {   0,0x00,0x00,0x00, 0x0, 0x0,   0x0, 0x00},  //256M system
		       {   0,0x20,0x28,0x2c,0x2e, 0x0,   0x0, 0x2f},  //512M system
		       {   0,0x20,0x40,0x48,0x4c,0x4e,   0x0, 0x4f},  //  1G system
		       {0x8e,0x8c,0x88,0x80,0x00,0x20,  0x40, 0x8f},  //  2G system
		       {   0,0x20,0x40,0x80,0xe0,0x100,0x120,0x128},  //  4G system
		       {   0,0x20,0x40,0x80,0xe0,0x100,0x200,0x220},  //  8G system
		       {   0,0x20,0x40,0x80,0xe0,0x100,0x200,0x400}}; // 16G system
//address mask
static	int am[7][8]={  { 0xf, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0, 0x0},
			{ 0xf, 0x7, 0x3, 0x1, 0x0, 0x0,  0x0, 0x0},
			{ 0xf,0x1f, 0x7, 0x3, 0x1, 0x0,  0x0, 0x0},
			{ 0x0, 0x1, 0x3, 0x7, 0xf,0x1f, 0x3f, 0x0},
			{ 0xf,0x1f,0x3f,0x3f,0x1f,0x1f, 0x07, 0x0},
			{ 0xf,0x1f,0x3f,0x3f,0x1f,0xff, 0x1f, 0x0},
			{ 0xf,0x1f,0x3f,0x3f,0x1f,0xff,0x1ff, 0x0}};

static	int part[7][8]={{0,0,0,0,0,0,0,0},
			{2,2,2,2,2,0,0,0},
			{2,2,2,2,2,2,0,0},
			{2,2,2,2,2,2,2,0},
			{2,1,2,3,2,3,2,0},
			{2,1,2,3,2,2,3,0},
			{2,1,2,3,2,2,2,0}};

static	int DIV[7][8]={{0,0,0,0,0,0,0,0},
		       {1,1,1,1,1,0,0,1},
		       {1,1,1,1,1,1,0,1},
		       {1,1,1,1,1,1,1,1},
		       {1,2,2,2,2,2,1,1},
		       {1,2,2,2,2,1,2,1},
		       {1,2,2,2,2,1,1,1}};

/*  These are POS bit values BEFORE adjusting for interleaving. */
static	int pos[7][8]= {{ 0, 0, 0, 0, 0, 0, 0, 0},
			{28,27,26,25,24, 0, 0,24},
			{28,29,27,26,25,24, 0,24}, 
			{24,25,26,27,28,29,30,24},
			{28,29,30,30,29,29,27,24},
			{28,29,30,30,29,32,29,24},
			{28,29,30,30,29,32,33,24}};

static	int en[8]={0x01,
		   0x9f,
		   0xbf,
		   0xff,
		   0xff,
		   0xff,
		   0xff};

static void (*boot1_reentry_fn)(void) = 0;

/* Column 0: SPD data for RMI 256MB XLR 7xx/5xx PCIX board */ 
/* Column 1: SPD data for RMI 512MB XLR 7xx/5xx PCIX board */ 
/* Column 2: SPD data for RMI 512MB XLR 308 PCIX board     */ 

static  int fake_spd[64][4]= {

    {0x80, 0x80, 0x80, 0x80}, /* Byte  0: 128 SPD bytes used                           */
    {0x08, 0x08, 0x08, 0x08}, /* Byte  1: 256 total SPD bytes                          */
    {0x08, 0x08, 0x08, 0x08}, /* Byte  2: DDR2 memory                                  */
    {0x0D, 0x0D, 0x0D, 0x0E}, /* Byte  3: 13 row address bits                          */
    {0x0A, 0x0A, 0x0A, 0x0A}, /* Byte  4: 10 column address bits                       */
    {0x60, 0x60, 0x60, 0x61}, /* Byte  5: 30mm DIMM height, 1 rank                     */
    {0x48, 0x48, 0x48, 0x48}, /* Byte  6: 72-bit wide data + ecc bus                   */
    {0x00, 0x00, 0x00, 0x00}, /* Byte  7: Reserved                                     */

    {0x05, 0x05, 0x05, 0x05}, /* Byte  8: SSTL 1.8V I/O                                */
    {0x3D, 0x3D, 0x30, 0x3D}, /* Byte  9: Minimum period at highest CAS                */
    {0x50, 0x50, 0x45, 0x50}, /* Byte 10: tAC                                          */
    {0x02, 0x02, 0x02, 0x02}, /* Byte 11: Data ECC is supported                        */
    {0x82, 0x82, 0x82, 0x82}, /* Byte 12: 7.81 us refresh rate, SELF refresh type      */
    {0x10, 0x10, 0x10, 0x08}, /* Byte 13: x16 devices                                  */
    {0x10, 0x10, 0x10, 0x08}, /* Byte 14: x16 ECC device                               */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 15: Min. clk delay, back-to-back random accesses */

    {0x0C, 0x0C, 0x0C, 0x0C}, /* Byte 16: Burst lengths 4 and 8 supported              */
    {0x04, 0x08, 0x08, 0x08}, /* Byte 17: Number of banks                              */
    {0x18, 0x18, 0x38, 0x18}, /* Byte 18: CAS latencies supported                      */
    {0x01, 0x01, 0x01, 0x01}, /* Byte 19: Module thickness/reserved                    */
    {0x02, 0x02, 0x02, 0x02}, /* Byte 20: Unbuffered DIMM                              */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 21: SDRAM module attributes                      */
    {0x01, 0x01, 0x01, 0x01}, /* Byte 22: Supports weak driver                         */
    {0x3D, 0x50, 0x3D, 0x50}, /* Byte 23: Minimum period at second highest CAS         */

    {0x50, 0x50, 0x45, 0x50}, /* Byte 24: tAC at second highest CAS                    */
    {0x00, 0x00, 0x50, 0x00}, /* Byte 25: Minimum period at third highest CAS          */
    {0x00, 0x00, 0x45, 0x00}, /* Byte 26: tAC at third highest CAS                     */
    {0x2D, 0x3C, 0x3C, 0x3C}, /* Byte 27: tRP                                          */
    {0x28, 0x28, 0x28, 0x1E}, /* Byte 28: tRRD = 10ns                                  */
    {0x2D, 0x3C, 0x3C, 0x3C}, /* Byte 29: tRCD                                         */
    {0x28, 0x28, 0x28, 0x2D}, /* Byte 30: tRAS = 40ns                                  */
    {0x40, 0x80, 0x80, 0x01}, /* Byte 31: Rank density                                 */

    {0x25, 0x25, 0x20, 0x25}, /* Byte 32: tISb                                         */
    {0x37, 0x37, 0x27, 0x37}, /* Byte 33: tIHb                                         */
    {0x10, 0x10, 0x10, 0x10}, /* Byte 34: tDSb = 0.100 ns                              */
    {0x22, 0x22, 0x17, 0x22}, /* Byte 35: tDHb                                         */
    {0x3C, 0x3C, 0x3C, 0x3C}, /* Byte 36: tWR  = 15 ns                                 */
    {0x1E, 0x1E, 0x1E, 0x1E}, /* Byte 37: tWTR =  7.5 ns                               */
    {0x1E, 0x1E, 0x1E, 0x1E}, /* Byte 38: tRTP =  7.5 ns                               */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 39: Reserved                                     */

    {0x00, 0x06, 0x06, 0x06}, /* Byte 40: Extension for bytes 41 and 42                */
    {0x37, 0x37, 0x37, 0x3C}, /* Byte 41: tRC = 55 ns                                  */
    {0x69, 0x7F, 0x7F, 0x7F}, /* Byte 42: tRFC                                         */
    {0x80, 0x80, 0x80, 0x80}, /* Byte 43: tCKmax = 8ns(125 MHz, DDR2-250)              */
    {0x1E, 0x1E, 0x18, 0x1E}, /* Byte 44: tDQSQ                                        */
    {0x28, 0x28, 0x22, 0x28}, /* Byte 45: tQHS                                         */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 46: PLL relock time (undefined)                  */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 47: Reserved                                     */

    {0x00, 0x00, 0x00, 0x00}, /* Byte 48: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 49: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 50: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 51: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 52: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 53: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 54: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 55: Reserved                                     */

    {0x00, 0x00, 0x00, 0x00}, /* Byte 56: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 57: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 58: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 59: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 60: Reserved                                     */
    {0x00, 0x00, 0x00, 0x00}, /* Byte 61: Reserved                                     */
    {0x12, 0x12, 0x12, 0x12}, /* Byte 62: SPD revision = 1.2                           */
    {0xA6, 0x37, 0x8A, 0xAA}};/* Byte 63: Checksum                                     */

/* Global variables */
int                run_eye_finder = 0;
unsigned long int  wait_loop_index = 0;
int                c_results[C_MAX];
int                use_fake_spd = 0;
int                col_index = 0;
int                cmd_strb_default_unbuf_hv = 0;
int                cmd_strb_default_reg = 0;

typedef struct dqs_data_entry {
  int tx_sweep_data[TX_MAX/TX_STEP]; 
  int rx_sweep_data[RX_MAX];
  int tx_center;                     /* Center Tx strobe value from the tx/rx sweep data */
  int rx_center;                     /* Center Rx strobe value from the tx/rx sweep data */
  int pass_region_size;              /* Size of the latest tx/rx sweep passing region    */
  int tb_delay_sweep_data[9];        /* Pass region sizes for (tboard = 0, rx_en = Half Early/Nominal) to (tboard = 2, rx_en = Half Late/Cycle Late) */ 
  int chosen_rx_en;                  /* RxEn strobe value: 0 = Half Early/Nominal, 1 = Nominal/Half Late, 2 = Half Late/Cycle Late */
} dqs_data_entry;

dqs_data_entry        dqs_data_table[8];

spd_entry          spd_data_dimm0[SPD_DATA_NUM_BYTES];  /* Array to hold SPD data for the DIMM whose SPD EEPROM address */
                                                        /*   is given by spd_addr_dimm0 */
spd_entry          spd_data_dimm1[SPD_DATA_NUM_BYTES];  /* Array to hold SPD data for the other DIMM */

typedef struct eye_finder_info {
    int rx_en_shift_amount; 
    int min_pass_threshold;
    int calibrate_disable_and_mask;
    int calibrate_enable_or_mask;
    int calibrate_disable_reg_addr;
    int ef_cmd_strb_default_unbuf_hv;
    int ef_cmd_strb_default_unbuf_lv;
    int ef_cmd_strb_default_reg;
    int ef_cmd_strb_sweep_max;
    int ef_rxtx_strb_sweep_max;
} eye_finder_info;

eye_finder_info   ef_info;

void enable_dram_eye_finder(int argc, char* argv[]){
#ifdef CONFIG_FX
    uint8_t buf;

    i2c_read(0x0, 0x51, FX_EYE_FINDER_OFFSET, &buf, 1);
    if (buf == 1) {
        buf = 0;
        printf("\n Disabling the eyefinder!! \n");
    } else {
        buf = 1;
        printf("\n Enabling the eyefinder!! \n");
    }
    i2c_write(0x0, 0x51, FX_EYE_FINDER_OFFSET, &buf, 1);
#endif    
    run_eye_finder = 1;
    printf("\nDRAM strobe sweeper run has been enabled.\n");
    
}


/* temperory - will be removed later */
void write_binfo_eeprom()
{
    unsigned char ee_data[ ] = {0x60, 0x0d,0xbe, 0xef, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
                                   0x41,0x52,0x49,0x5a,0x4f,0x4e,0x41,0x38,0x38,0x31,0x62,0x31,0x30,0x34,0x00,0x07, 
                                   0x00,0x0f,0x30,0x04,0x3b,0x38,0x01,0x04,0x00,0x00,0x00,0x00 };
    int i=0;
   
    for(i=0;i<44;i++)
    { 
      if(spd_i2c_tx(0x50,i,1,ee_data[i]) != -1 )
       {
           printf("\nWritten %x to eeprom offset %d",ee_data[i],i);
       }
    }
}

void print_usage_iobase(void) {

    printf("\niobase [r] [addr]\n");
    printf("iobase [w] [addr] [val]\n");
    printf("     : [addr] is in the 1MB Range [0x1EF00000, 0x1EFFFFFF]\n");
}

/* defines to disable cpld watchdog when in mini shell */
#define MASTER_CPLD_CHIP_SELECT 1
#define RESET_CPLD_CHIP_SELECT 3
#define CPLD_WD_DISABLE_OFFSET 0xb0
#define CPLD_WD_MASK  0x3F

int dram_spd_main(int argc, char *argv[]) {
    int tck_denom = 0, voltage = 2;
    uint8_t get_buffer[2], buf = 0;

    /* RMI GPIO  reg for real data on pin 1 - hi, 0- lo */
    volatile uint32_t *gpio_reg_io = GPIO_IO_REG; 
    /* RMI GPIO  reg for io direction */
    volatile uint32_t *gpio_reg_cfg = GPIO_CFG_REG;

    boot1_reentry_fn = (void (*)(void))
        (unsigned long)read_64bit_cp0_register_sel(COP_0_OSSCRATCH, 1);

    spd_i2c_init();

#ifdef CONFIG_FX
    assm_id = fx_get_board_type();

    fx_cpld_early_init();

    if(i2c_read(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR, FX_EYE_FINDER_OFFSET, &buf, 1) != 0)
    {
        printf("\n ERROR: Could not read run eye finder value!!\n");
        buf = 0;
    }
    if (buf == 1) {
        run_eye_finder = 1;
        buf = 0;
        if (i2c_write(FX_EEPROM_BUS, FX_BOARD_EEPROM_ADDR, FX_EYE_FINDER_OFFSET, &buf, 1) !=0)
        {
            printf("\n ERROR: Could not write eyefinder value!!\n");
        }
            
        printf("\nRunning Eye Finder... Disabling the Boot CPLD!!!\n");

        fx_cpld_set_bootkey();
    }
    else {
        /*  Using number of "." to imply reset reason.
         *  Booting<...>
         *  .     Cold boot
         *  ..    Push buttom reset
         *  ...   Software reset
         *  ....  Watchdog timer reset
         *  ..... CPLD reset
         */
        uint8_t reset_type = 0;
        char dots[5] = {'.', '.', '.', '.', '.'};
        int i;
        
        reset_type = fx_cpld_get_reset_type();

        for (i = 4; i > 0; i--) {
            if (reset_type & (1 << i)) {
                break;
            }
            else {
                dots[i] = 0;
            }
        }
        printf("\n\nBooting%s\n", dots);
    }

    /*
     * Write to the SPD EEPROM and make it write protect.
     *
     *   These are special instructions to the DIMM Modules.
     *   ANY value written to ANY offset on device 0x30
     *   puts the EEPROM on 0x50 into a read-only state.
     *   (There is no actual device on 0x30 but the DIMM
     *   responds to this.)
     *
     *   Similarly, 0x32 is to write protect the DIMM2 which
     *   is the device 0x52.
     */
    buf = 1;
    i2c_write(FX_EEPROM_BUS, 0x30, 0x0, &buf, 1);
    i2c_write(FX_EEPROM_BUS, 0x32, 0x0, &buf, 1);
#endif 
    switch (assm_id)
    {
#ifdef CONFIG_FX    
        case QFX_CONTROL_BOARD_ID:	
            use_fake_spd = 0;
            cmd_strb_default_unbuf_hv = 0x42; 
            cmd_strb_default_reg = 0x43;      
            break;

        case QFX3500_BOARD_ID:
        case QFX3500_48S4Q_AFO_BOARD_ID:
            *gpio_reg_cfg = (0x3F | (1 << 24) | ( 1 << 22) | (1 << 19));
            *gpio_reg_io = (0x2 | (1 << 24) | ( 1 << 22) | (1 << 19));
            use_fake_spd = 0;
            cmd_strb_default_unbuf_hv = 0x40; 
            cmd_strb_default_reg = 0x40;     
            break;

        CASE_UFABRIC_QFX3600_SERIES:
        CASE_SE_QFX3600_SERIES:
            /* 
             * Enable port pins 22, 24 and 19
             * and configure the eye finder to 0x39
             */
            *gpio_reg_cfg = (0x3F | (1 << 24) | ( 1 << 22) | (1 << 19));
            *gpio_reg_io = (0x2 | (1 << 24) | ( 1 << 22) | (1 << 19));
            use_fake_spd = 0;
            cmd_strb_default_unbuf_hv = 0x39; 
            cmd_strb_default_reg = 0x39;     
            break;

        case QFX5500_BOARD_ID:
            *gpio_reg_cfg = (0x3F | (1 << 24) | ( 1 << 22) | (1 << 19));
            *gpio_reg_io = (0x2 | (1 << 24) | ( 1 << 22) | (1 << 19));
            use_fake_spd = 0;
            cmd_strb_default_unbuf_hv = 0x4b; 
            cmd_strb_default_reg = 0x4b;     
            break;
#endif
        default:
           /*
            *  use_fake_spd = 0;
            *  cmd_strb_default_unbuf_hv = 0x18;
            *  cmd_strb_default_reg = EF_CMD_STRB_DEFAULT_REG;
            */
            printf("Defaulting to SOHO board type\n"); 
            printf("Stop at boo1_5 to program board id by using imw utility (example below)\n");
            printf("boot1_5> imw 0x00 0x51 0x04 0x09 0x1\n"); 
            printf("boot1_5> imw 0x00 0x51 0x05 0xa2 0x1\n");             
            /* Soho needs gpio selection */
            *gpio_reg_cfg = (0x3F | (1 << 24) | ( 1 << 22) | (1 << 19));
            *gpio_reg_io = (0x2 | (1 << 24) |  ( 1 << 22) | (1 << 19));
            use_fake_spd = 0;
            cmd_strb_default_unbuf_hv = 0x34; 
            cmd_strb_default_reg = 0x34;     
            break;
    }

#ifdef CFG_BOARD_RMIREF 
    {
        /* The internal SLCT Board could have 
         * other values for tck_denom, voltage
         */
        unsigned short major=0;
        read_eeprom(MAJOR_OFFSET, &major);
	switch (major) {
		case '4' :
			read_slct_cpld_dram_config_reg(&tck_denom, &voltage, 1);
			break;
		default:
			/* XLS B0 */
			if (chip_pid_xls_b0()) {
				read_slct_cpld_dram_config_reg(&tck_denom, 
								&voltage, 0);
			}
			break;
	}

    }
#endif

    /* the main SPD init function */
    board_spd_init(tck_denom, voltage);
    
#if defined (SPD_TEST) && defined (SPD_DEBUG)
    memtester_main();
#endif
 
    boot1_reentry_fn();
    return 0;
}

#ifdef CFG_BOARD_RMIREF
static void read_slct_cpld_dram_config_reg(int *tck_denom, int *voltage, 
		int xlr) 
{

    phoenix_reg_t *fl_base = phoenix_io_mmio(PHOENIX_IO_FLASH_OFFSET);
    unsigned int data=0;

    /* Map CS1 (CPLD) from 0xbfb00000 to 0xbfb0ffff */
    phoenix_verbose_write_reg (fl_base, 0x1, 0xb0);
    if (xlr) {
        /* XLR has an 8-bit CPLD interface */
        unsigned char *slct_cpld_dram_cfgreg;
        slct_cpld_dram_cfgreg = (unsigned char *)0xbfb00014; 
        data = ((*slct_cpld_dram_cfgreg) & 0xf8) >> 3;                    
    }
    else {
        /* XLS has a 16-bit CPLD interface */
        unsigned short *slct_cpld_dram_cfgreg;
        slct_cpld_dram_cfgreg = (unsigned short *)0xbfb00014; 
        data = *slct_cpld_dram_cfgreg;
    }
    printf("\nSLCT CPLD DRAM CFG Reg [0x%x]\n", data);

    if (data == 0) {  
        *tck_denom = 0; /* Bootloader default DDR clock speed limit*/
        *voltage   = 2; /* High voltage */     
    }
    else if (data == 31) {
        printf_err("[Error]: SLCT DRAM CFG Reg. Values 0-30 are supported. You have %d\n", data);
        dram_spd_exit();
    }
    else {
        /*  1/11/21 = 100MHz, 2/12/22 = 133MHz, 3/13/23 = 166MHz, 
         *  4/14/24 = 200MHz, 5/15/25 = 233MHz, 6/16/26 = 266MHz,   
         *  7/17/27 = 300MHz, 8/18/28 = 333MHz, 9/19/29 = 366MHz,   
         * 10/20/30 = 400MHz 
         */  
        *tck_denom = ((data - 1) % 10) + 3; 
        /* 1-10: Low, 11-20: Medium, 21-30: High */ 
        *voltage   = ((data - 1) / 10); 
    }
}
#endif
	
static int board_spd_init(int tck_denom, int voltage) {

    int               tck_start_denom = tck_denom;
    int               bidi_board_del = 1;
    enum mem_protocol dtype;
    phoenix_reg_t     *spdinitbridgebase = phoenix_io_mmio(PHOENIX_IO_BRIDGE_OFFSET);
    unsigned int      spd_init_data;
    int               ch0_mem_size = 0;
    int               ch2_mem_size = 0;
    int               ch1_mem_size = 0;
    int               ch3_mem_size = 0;

	int               ch0_status;
	int               ch2_status;
	int               ch1_status;
	int               ch3_status;
	
	int               ch0_ecc_supported = 0; 
	int               ch1_ecc_supported = 0;
	int               ch2_ecc_supported = 0; 
	int               ch3_ecc_supported = 0;
	
	int               ch0_user_bank_loc = 0; /* Use default addressing scheme */ 
	int               ch0_user_rank_loc = 0; /* Use default addressing scheme */ 
	int               ch0_user_row_loc = 0;  /* Use default addressing scheme */ 
	int               ch0_user_col_loc = 0;  /* Use default addressing scheme */ 
	
	int               ch1_user_bank_loc = 0; /* Use default addressing scheme */ 
	int               ch1_user_rank_loc = 0; /* Use default addressing scheme */ 
	int               ch1_user_row_loc = 0;  /* Use default addressing scheme */ 
	int               ch1_user_col_loc = 0;  /* Use default addressing scheme */ 
	
	int               ch2_user_bank_loc = 0; /* Use default addressing scheme */ 
	int               ch2_user_rank_loc = 0; /* Use default addressing scheme */ 
	int               ch2_user_row_loc = 0;  /* Use default addressing scheme */ 
	int               ch2_user_col_loc = 0;  /* Use default addressing scheme */ 
	
	int               ch3_user_bank_loc = 0; /* Use default addressing scheme */ 
	int               ch3_user_rank_loc = 0; /* Use default addressing scheme */ 
	int               ch3_user_row_loc = 0;  /* Use default addressing scheme */ 
    int               ch3_user_col_loc = 0;  /* Use default addressing scheme */ 

	int               ch_width = 72;         /* SET CHANNEL WIDTH HERE */
	int               index; 
    char              chip_rev[3];
	int               chip_version = 0;

    int               dram_ch0_clock_ratio_reg_addr = 0;
    int               dram_ch1_clock_ratio_reg_addr = 0;
    int               dram_ch2_clock_ratio_reg_addr = 0;
    int               dram_ch3_clock_ratio_reg_addr = 0;

    int               ctrl_pres_vect = 0;
    u_int8_t          spd_eeprom1_val;

    phoenix_reg_t *gpiobase = phoenix_io_mmio(PHOENIX_IO_GPIO_OFFSET);
    unsigned long dram_chwidth_bit;

	/* Detect XLR revision number */
	chip_version = ((chip_processor_id() << 8) | chip_revision_id());

	if (!chip_processor_id_xls()) {
	  chip_version = chip_revision_id();
	}

	switch (chip_version) {

	case 2:
	case 3:
	case 4:
		chip_rev[0] = 'B';
		chip_rev[1] = 'x';
		chip_rev[2] = 'R';
		ctrl_pres_vect = 0xf;
		ef_info.rx_en_shift_amount           = RX_EN_SHIFT_AMOUNT;
		ef_info.min_pass_threshold           = MIN_PASS_THRESHOLD;
		ef_info.calibrate_disable_and_mask   = CALIBRATE_DISABLE_AND_MASK;
		ef_info.calibrate_enable_or_mask     = CALIBRATE_ENABLE_OR_MASK;
		ef_info.calibrate_disable_reg_addr   = CALIBRATE_DISABLE_REG_ADDR;
		ef_info.ef_cmd_strb_default_unbuf_hv = EF_CMD_STRB_DEFAULT_UNBUF_HV;
		ef_info.ef_cmd_strb_default_unbuf_lv = EF_CMD_STRB_DEFAULT_UNBUF_LV;
		ef_info.ef_cmd_strb_default_reg      = EF_CMD_STRB_DEFAULT_REG;
		ef_info.ef_cmd_strb_sweep_max	     = EF_CMD_STRB_SWEEP_MAX;
		ef_info.ef_rxtx_strb_sweep_max       = EF_RXTX_STRB_SWEEP_MAX;
		break;

	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		chip_rev[0] = 'C';
		chip_rev[1] = 'x';
		chip_rev[2] = 'R';
		ctrl_pres_vect = 0xf;
		ef_info.rx_en_shift_amount           = C0_RX_EN_SHIFT_AMOUNT;
		ef_info.min_pass_threshold           = C0_MIN_PASS_THRESHOLD;
		ef_info.calibrate_disable_and_mask   = C0_CALIBRATE_DISABLE_AND_MASK;
		ef_info.calibrate_enable_or_mask     = C0_CALIBRATE_ENABLE_OR_MASK;
		ef_info.calibrate_disable_reg_addr   = C0_CALIBRATE_DISABLE_REG_ADDR;
		ef_info.ef_cmd_strb_default_unbuf_hv = C0_EF_CMD_STRB_DEFAULT_UNBUF_HV;
		ef_info.ef_cmd_strb_default_unbuf_lv = C0_EF_CMD_STRB_DEFAULT_UNBUF_LV;
		ef_info.ef_cmd_strb_default_reg      = C0_EF_CMD_STRB_DEFAULT_REG;
		ef_info.ef_cmd_strb_sweep_max	     = C0_EF_CMD_STRB_SWEEP_MAX;
		ef_info.ef_rxtx_strb_sweep_max       = C0_EF_RXTX_STRB_SWEEP_MAX;
		break;
     
    case 0x8001: /* XLS 608 */
    case 0x4002: /* XLS B0 616 */
    case 0x4003: /* XLS B0 616 A1 */
    case 0x4a02: /* XLS B0 608 */
    case 0x4a03: /* XLS B0 608 A1 */
    case 0x4803: /* XLS B0 612 A1 */
		chip_rev[0] = 'A';
		chip_rev[1] = '6';
		chip_rev[2] = 'S';
		ctrl_pres_vect = 0xf;
 		ef_info.rx_en_shift_amount           = XLS_A0_RX_EN_SHIFT_AMOUNT;
		ef_info.min_pass_threshold           = XLS_A0_MIN_PASS_THRESHOLD;
		ef_info.calibrate_disable_and_mask   = XLS_A0_CALIBRATE_DISABLE_AND_MASK;
		ef_info.calibrate_enable_or_mask     = XLS_A0_CALIBRATE_ENABLE_OR_MASK;
		ef_info.calibrate_disable_reg_addr   = XLS_A0_CALIBRATE_DISABLE_REG_ADDR;
		ef_info.ef_cmd_strb_default_unbuf_hv = cmd_strb_default_unbuf_hv;
		ef_info.ef_cmd_strb_default_unbuf_lv = XLS_A0_EF_CMD_STRB_DEFAULT_UNBUF_LV;
		ef_info.ef_cmd_strb_default_reg      = cmd_strb_default_reg;
		ef_info.ef_cmd_strb_sweep_max	     = XLS_A0_EF_CMD_STRB_SWEEP_MAX;
		ef_info.ef_rxtx_strb_sweep_max       = XLS_A0_EF_RXTX_STRB_SWEEP_MAX;
		break;

    case 0x8801: /* XLS 408 */ 
    case 0x8c01: /* XLS 404 */
    case 0x4402: /* XLS B0 416 */
    case 0x4403: /* XLS B0 416 A1*/
    case 0x4c02: /* XLS B0 412 */
    case 0x4c03: /* XLS B0 412 A1 */
    case 0x4e02: /* XLS B0 408 */
    case 0x4e03: /* XLS B0 408 A1 */
    case 0x4f02: /* XLS B0 404 */
    case 0x4f03: /* XLS B0 404 A1 */
		chip_rev[0] = 'A';
		chip_rev[1] = '4';
		chip_rev[2] = 'S';
		ctrl_pres_vect = 0x3;
		ef_info.rx_en_shift_amount           = XLS_A0_RX_EN_SHIFT_AMOUNT;
		ef_info.min_pass_threshold           = XLS_A0_MIN_PASS_THRESHOLD;
		ef_info.calibrate_disable_and_mask   = XLS_A0_CALIBRATE_DISABLE_AND_MASK;
		ef_info.calibrate_enable_or_mask     = XLS_A0_CALIBRATE_ENABLE_OR_MASK;
		ef_info.calibrate_disable_reg_addr   = XLS_A0_CALIBRATE_DISABLE_REG_ADDR;
		ef_info.ef_cmd_strb_default_unbuf_hv = cmd_strb_default_unbuf_hv;
		ef_info.ef_cmd_strb_default_unbuf_lv = XLS_A0_EF_CMD_STRB_DEFAULT_UNBUF_LV;
		ef_info.ef_cmd_strb_default_reg      = cmd_strb_default_reg;
		ef_info.ef_cmd_strb_sweep_max	     = XLS_A0_EF_CMD_STRB_SWEEP_MAX;
		ef_info.ef_rxtx_strb_sweep_max       = XLS_A0_EF_RXTX_STRB_SWEEP_MAX;
		break;

	case 0x8e01: /* XLS 208 */
	case 0x8e00: /* XLS 208 */
	case 0x8f00: /* XLS 204 */  
	case 0x8f01: /* XLS 204 */  
	case 0xce00: /* XLS 108 */  
	case 0xce01: /* XLS 108 */  
	case 0xcf00: /* XLS 104 */  
	case 0xcf01: /* XLS 104 */  
		chip_rev[0] = 'A';
		chip_rev[1] = '2';
		chip_rev[2] = 'S';
		ctrl_pres_vect = 0x1;
		ef_info.rx_en_shift_amount           = XLS_A0_RX_EN_SHIFT_AMOUNT;
		ef_info.min_pass_threshold           = XLS_A0_MIN_PASS_THRESHOLD;
		ef_info.calibrate_disable_and_mask   = XLS_A0_CALIBRATE_DISABLE_AND_MASK;
		ef_info.calibrate_enable_or_mask     = XLS_A0_CALIBRATE_ENABLE_OR_MASK;
		ef_info.calibrate_disable_reg_addr   = XLS_A0_CALIBRATE_DISABLE_REG_ADDR;
		ef_info.ef_cmd_strb_default_unbuf_hv = cmd_strb_default_unbuf_hv;
		ef_info.ef_cmd_strb_default_unbuf_lv = XLS_A0_EF_CMD_STRB_DEFAULT_UNBUF_LV;
		ef_info.ef_cmd_strb_default_reg      = XLS_A0_EF_CMD_STRB_DEFAULT_REG;
		ef_info.ef_cmd_strb_sweep_max	     = XLS_A0_EF_CMD_STRB_SWEEP_MAX;
		ef_info.ef_rxtx_strb_sweep_max       = XLS_A0_EF_RXTX_STRB_SWEEP_MAX;
        dram_chwidth_bit = phoenix_read_reg(gpiobase, 35);
        if ((dram_chwidth_bit >> 25) & 0x01)
            ch_width = 36;
        break;

	default:
		printf_err("\nUnsupported Silicon Version [0x%x]\n", chip_version);
		dram_spd_exit();
		break;
	}

    if (chip_rev[2] == 'S'){ /* Only one DRAM PLL on XLS */
        dram_ch0_clock_ratio_reg_addr = GPIO_DRAM1_RATIO_REG;
        dram_ch1_clock_ratio_reg_addr = GPIO_DRAM1_RATIO_REG;
        dram_ch2_clock_ratio_reg_addr = GPIO_DRAM1_RATIO_REG;
        dram_ch3_clock_ratio_reg_addr = GPIO_DRAM1_RATIO_REG;
    }
    else { /* Two DRAM PLL's on XLR */
        dram_ch0_clock_ratio_reg_addr = GPIO_DRAM1_RATIO_REG;
        dram_ch1_clock_ratio_reg_addr = GPIO_DRAM1_RATIO_REG;
        dram_ch2_clock_ratio_reg_addr = GPIO_DRAM2_RATIO_REG;
        dram_ch3_clock_ratio_reg_addr = GPIO_DRAM2_RATIO_REG;
    }
    printf("\n");
    if (chip_rev[2] == 'R'){
        SPD_PRINTF("DRAM SPD: Detected XL%c version %c\n", 
                chip_rev[2], chip_rev[0]);
    }
    else{
        SPD_PRINTF("DRAM SPD: Detected XL%c %c%c%c version %c\n", 
                chip_rev[2], chip_rev[1], 'x', 'x', chip_rev[0]);
    }
	
    SPD_PRINTF("Compile-time Presumed DRAM Channel Width [%d]\n", ch_width);

	if (ch_width == 72) {
		/* channel 0 */
		SPD_PRINTF("\nConfiguring DRAM Channel 0\n");
		init_spd_data(spd_data_dimm0);
		init_spd_data(spd_data_dimm1);

    if ((PRODUCT_IS_OF_UFABRIC_QFX3600_SERIES(fx_get_board_type())) ||
        (PRODUCT_IS_OF_SE_QFX3600_SERIES(fx_get_board_type()))) {
        spd_eeprom1_val = SPD_EEPROM2;
    } else {
        spd_eeprom1_val = SPD_EEPROM1;
    }
		ch0_status = channel_config( 0,                   
					     SPD_EEPROM0, 
					     spd_eeprom1_val, 
					     PHOENIX_IO_GPIO_OFFSET,
					     GPIO_DRAM1_CNTRL_REG,
					     dram_ch0_clock_ratio_reg_addr,
					     GPIO_DRAM1_RESET_REG,
					     GPIO_DRAM1_STATUS_REG, 
					     &tck_start_denom,
					     &dtype, 
					     ch_width, 
					     NO_OVERRIDE_ADD_LAT_MAGIC_NUM, 
					     PHOENIX_IO_BRIDGE_OFFSET, 
					     0,                              
					     PHOENIX_IO_DDR2_CHN0_OFFSET, 
					     &ch0_ecc_supported,             /* Is ECC supported? */
					     &ch0_user_bank_loc,
					     &ch0_user_rank_loc,
					     &ch0_user_col_loc, 
					     &ch0_user_row_loc,
					     bidi_board_del, 
					     &ch0_mem_size, 
					     voltage,
					     chip_rev);
   
		printf("Available Memory on Channel0 [%dMB]\n", ch0_mem_size); 

		tck_start_denom = tck_denom;

                if ((ctrl_pres_vect & 0x4) == 0x4) { /* Channel 2 */
		  SPD_PRINTF("\nConfiguring DRAM Channel 2\n");
		  init_spd_data(spd_data_dimm0);
		  init_spd_data(spd_data_dimm1);
		  ch2_status = channel_config( 2,                              /* Configuring channel 2 */
					       SPD_EEPROM2,
					       SPD_EEPROM3,
					       PHOENIX_IO_GPIO_OFFSET,
					       GPIO_DRAM2_CNTRL_REG,
					       dram_ch2_clock_ratio_reg_addr,
					       GPIO_DRAM2_RESET_REG,
					       GPIO_DRAM2_STATUS_REG,
					       &tck_start_denom,
					       &dtype,
					       ch_width,
					       NO_OVERRIDE_ADD_LAT_MAGIC_NUM,
					       PHOENIX_IO_BRIDGE_OFFSET,
					       0,
					       PHOENIX_IO_DDR2_CHN2_OFFSET,
					       &ch2_ecc_supported,             /* Is ECC supported? */
					       &ch2_user_bank_loc,
					       &ch2_user_rank_loc,
					       &ch2_user_col_loc, 
					       &ch2_user_row_loc,
					       bidi_board_del,
					       &ch2_mem_size,
					       voltage,
					       chip_rev);
                                                
		  printf("Available Memory on Channel2 [%dMB]\n", ch2_mem_size); 
		} /* END if controller 2 present */

   	} /* END if ch_width == 72 */

	else if (ch_width == 36) {
		SPD_PRINTF("\nConfiguring DRAM Channel 0\n");
		init_spd_data(spd_data_dimm0);
		init_spd_data(spd_data_dimm1);
		ch0_status = channel_config( 0,                              /* Configuring channel 0 */
					     SPD_EEPROM0, 
					     SPD_EEPROM0,            /* Same DIMM address since there is only one DIMM per channel */
					     PHOENIX_IO_GPIO_OFFSET,
					     GPIO_DRAM1_CNTRL_REG,
					     dram_ch0_clock_ratio_reg_addr,
					     GPIO_DRAM1_RESET_REG,
					     GPIO_DRAM1_STATUS_REG, 
					     &tck_start_denom,
					     &dtype, 
					     ch_width, 
					     NO_OVERRIDE_ADD_LAT_MAGIC_NUM, 
					     PHOENIX_IO_BRIDGE_OFFSET, 
					     0,                              
					     PHOENIX_IO_DDR2_CHN0_OFFSET, 
					     &ch0_ecc_supported,             /* Is ECC supported? */
					     &ch0_user_bank_loc,
					     &ch0_user_rank_loc,
					     &ch0_user_col_loc, 
					     &ch0_user_row_loc,
					     bidi_board_del, 
					     &ch0_mem_size,
					     voltage, 
					     chip_rev);
   
		printf("Available Memory on Channel0 [%dMB]\n", ch0_mem_size/2); 

		tck_start_denom = tck_denom;
 
		if ((ctrl_pres_vect & 0x2) == 0x2){ /* Channel 1 */
		  SPD_PRINTF("\nConfiguring DRAM Channel 1\n");
		  init_spd_data(spd_data_dimm0);
		  init_spd_data(spd_data_dimm1);
		  ch1_status = channel_config( 1,                  
					       SPD_EEPROM1, 
					       SPD_EEPROM1,
					       PHOENIX_IO_GPIO_OFFSET,
					       GPIO_DRAM1_CNTRL_REG,
					       dram_ch1_clock_ratio_reg_addr,
					       GPIO_DRAM1_RESET_REG,
					       GPIO_DRAM1_STATUS_REG, 
					       &tck_start_denom,
					       &dtype, 
					       ch_width, 
					       NO_OVERRIDE_ADD_LAT_MAGIC_NUM, 
					       PHOENIX_IO_BRIDGE_OFFSET, 
					       0,                              
					       PHOENIX_IO_DDR2_CHN1_OFFSET, 
					       &ch1_ecc_supported, 
					       &ch1_user_bank_loc,
					       &ch1_user_rank_loc,
					       &ch1_user_col_loc, 
					       &ch1_user_row_loc,
					       bidi_board_del, 
					       &ch1_mem_size,
					       voltage,
					       chip_rev);

		  printf("Available Memory on Channel1 [%dMB]\n", ch1_mem_size/2); 
		  
		  tck_start_denom = tck_denom;
		} /* END if controller 1 present */

		if ((ctrl_pres_vect & 0x4) == 0x4){ /* channel 2 */
		  SPD_PRINTF("\nConfiguring DRAM Channel 2\n");
		  init_spd_data(spd_data_dimm0);
		  init_spd_data(spd_data_dimm1);
		  ch2_status = channel_config( 2,                              /* Configuring channel 2 */
					       SPD_EEPROM2,
					       SPD_EEPROM2,            /* Same DIMM address since there is only one DIMM per channel */
					       PHOENIX_IO_GPIO_OFFSET,
					       GPIO_DRAM2_CNTRL_REG,
					       dram_ch2_clock_ratio_reg_addr,
					       GPIO_DRAM2_RESET_REG,
					       GPIO_DRAM2_STATUS_REG,
					       &tck_start_denom,
					       &dtype,
					       ch_width,
					       NO_OVERRIDE_ADD_LAT_MAGIC_NUM,
					       PHOENIX_IO_BRIDGE_OFFSET,
					       0,
					       PHOENIX_IO_DDR2_CHN2_OFFSET,
					       &ch2_ecc_supported,             /* Is ECC supported? */
					       &ch2_user_bank_loc,
					       &ch2_user_rank_loc,
					       &ch2_user_col_loc, 
					       &ch2_user_row_loc,
					       bidi_board_del,
					       &ch2_mem_size,
					       voltage,
					       chip_rev);

		  printf("Available Memory on Channel2 [%dMB]\n", ch2_mem_size/2); 
		  
		  tck_start_denom = tck_denom;
		} /* END if controller 2 present */

		if ((ctrl_pres_vect & 0x8) == 0x8){ /* channel 3 */
		  SPD_PRINTF("\nConfiguring DRAM Channel 3\n");
		  init_spd_data(spd_data_dimm0);
		  init_spd_data(spd_data_dimm1);
		  ch3_status = channel_config( 3,
					       SPD_EEPROM3,
					       SPD_EEPROM3,
					       PHOENIX_IO_GPIO_OFFSET,
					       GPIO_DRAM2_CNTRL_REG,
					       dram_ch3_clock_ratio_reg_addr,
					       GPIO_DRAM2_RESET_REG,
					       GPIO_DRAM2_STATUS_REG,
					       &tck_start_denom,
					       &dtype,
					       ch_width,
					       NO_OVERRIDE_ADD_LAT_MAGIC_NUM,
					       PHOENIX_IO_BRIDGE_OFFSET,
					       0,
					       PHOENIX_IO_DDR2_CHN3_OFFSET,
					       &ch3_ecc_supported,
					       &ch3_user_bank_loc,
					       &ch3_user_rank_loc,
					       &ch3_user_col_loc, 
					       &ch3_user_row_loc,
					       bidi_board_del,
					       &ch3_mem_size,
					       voltage,
					       chip_rev);

		  printf("Available Memory on Channel3 [%dMB]\n", ch3_mem_size/2); 
 
		} /* END if controller 3 present */
	
		/* With our present RMI ATX I/II 36-bit DRAM channel DIMM hack, */
		/* ECC will not work on any of the channels. Hence disable it.  */
		ch0_ecc_supported = 0; 
		ch1_ecc_supported = 0;
		ch2_ecc_supported = 0; 
		ch3_ecc_supported = 0;

	} /* END if ch_width == 36 */
  
	index = program_dram_bridge(ch0_mem_size, ch1_mem_size, ch2_mem_size, ch3_mem_size, ch_width, chip_rev);

	/* Store various information in bridge scratch register 1, to  */
	/* be used by a later stage of the bootloader to enable ECC    */
	/* and the DRAM BARs.                                          */
	/* (not to be confused with OS scratch register 1 )            */
	/* Format: [0]: Set if channel 0 supports ECC                  */
	/*         [1]: Set if channel 1 supports ECC                  */
	/*         [2]: Set if channel 2 supports ECC                  */
	/*         [3]: Set if channel 3 supports ECC                  */
	/*         [4]: Set if DRAM BAR 0 should be enabled            */
	/*         [5]: Set if DRAM BAR 1 should be enabled            */
	/*         [6]: Set if DRAM BAR 2 should be enabled            */
	/*         [7]: Set if DRAM BAR 3 should be enabled            */
	/*         [8]: Set if DRAM BAR 4 should be enabled            */
	/*         [9]: Set if DRAM BAR 5 should be enabled            */
	/*        [10]: Set if DRAM BAR 6 should be enabled            */
	/*        [11]: Set if DRAM BAR 7 should be enabled            */

	spd_init_data = (en[index]         << 4) |
	                (ch3_ecc_supported << 3) | 
		        (ch2_ecc_supported << 2) |  
		        (ch1_ecc_supported << 1) |  
		        (ch0_ecc_supported << 0);

	phoenix_write_reg(spdinitbridgebase, BRIDGE_SCRATCH1, spd_init_data);
        
	return 0;
} /* END board_spd_init */

static int program_dram_bridge(int ch0_mem_size, 
			       int ch1_mem_size, 
			       int ch2_mem_size, 
			       int ch3_mem_size, 
			       int ch_width,
			       char chip_rev[]){ 

	phoenix_reg_t *mainbridgebase = phoenix_io_mmio(PHOENIX_IO_BRIDGE_OFFSET);

	int               idx = 0;
	unsigned int      data;
	unsigned int      xleave = 0;
	unsigned int      pos_adjust = 0;

    int total_mem  = 0;
	int state = 0;

	if (ch_width == 36){
		/* For ATX-1/2, in 36-bit mode DIMMs       */
		/* are hacked, disabling half their memory */
		ch0_mem_size = ch0_mem_size >> 1;
		ch1_mem_size = ch1_mem_size >> 1;
		ch2_mem_size = ch2_mem_size >> 1;
		ch3_mem_size = ch3_mem_size >> 1;
	}

	total_mem = ch0_mem_size + ch1_mem_size + ch2_mem_size + ch3_mem_size;

	/* Index values are as follows: 0 for 256MB total memory                         */ 
	/*                              1 for 512MB total memory                         */ 
	/*                              2 for   1GB total memory                         */ 
	/*                              3 for   2GB total memory                         */
	/*                              4 for   4GB total memory                         */
	/*                              5 for   8GB total memory                         */
	/*                              6 for  16GB total memory                         */
        switch (total_mem) {
        case  256:
                idx = 0;
                break;
 
        case  512:
                idx = 1;
                break;
 
        case 1024:
                idx = 2;
                break;
 
        case 2048:
                idx = 3;
                break;
 
        case 4096:
                idx = 4;
                break;
 
        case 8192:
                idx = 5;
                break;
 
        case 16384:
                idx = 6;
                break;
 
        default:
                printf_err("Invalid/Unsupported Total Memory Detected [%dMB]\n", total_mem);
                printf_err("Valid/Supported Values: 256M/512M/1G/2G/4G/8G/16G\n");
                dram_spd_exit();
                break;
        }

	state = ((ch3_mem_size != 0) << 3) |
		((ch2_mem_size != 0) << 2) |
		((ch1_mem_size != 0) << 1) |
		((ch0_mem_size != 0) << 0);

	/* ch3 ch2 ch1 ch0 xleave pos_adjust Comments       */
	/* 0   0   0   0                     Unsupported    */
	/* 0   0   0   M   0      0          Ok if 1G/2G/4G */
	/* 0   0   M   0   1      0          Ok if 1G/2G/4G */
	/* 0   0   M   M                     Unsupported    */
	/* 0   M   0   0   2      0          Ok if 1G/2G/4G */
	/* 0   M   0   M   4      1          Ok if 1G/2G/4G and if equal */
	/* 0   M   M   0                     Unsupported    */
	/* 0   M   M   M                     Unsupported    */
	/* M   0   0   0   3      0          Ok if 1G/2G/4G */
	/* M   0   0   M                     Unsupported    */
	/* M   0   M   0   5      1          Ok if 1G/2G/4G and if equal */
	/* M   0   M   M                     Unsupported    */
	/* M   M   0   0                     Unsupported    */
	/* M   M   0   M                     Unsupported    */
	/* M   M   M   0                     Unsupported    */
	/* M   M   M   M   7      2          Ok if 1G/2G/4G and if equal */

	switch (state) {
	case 1:     
		xleave = 0x0;      
		pos_adjust = 0;
		break;

	case 2:     
		xleave = 0x1;      
		pos_adjust = 0;
		break;

	case 3: 
	        if ((chip_rev[2] == 'S') && ((chip_rev[1] == '6') || (chip_rev[1] == '4')) && (ch_width == 36) ) { /* XLS6xx, XLS4xx 2x36 mode */    
		  phoenix_verbose_write_reg(mainbridgebase, BRIDGE_CFG_BAR, MEM_CTL_XLS4XX_XLS6XX_DDR36_AB_BRIDGE_CFG);
		  xleave = 0x7;      
		  pos_adjust = 1;
		}
		else{
		  printf_err("[Error]: Unsupported DRAM population pattern - {ch3,ch2,ch1,ch0} = %d %d %d %d\n", 
                        ((state & 0x8) >> 3), ((state & 0x4) >> 2), ((state & 0x2) >> 1), (state & 0x1)); 
		  dram_spd_exit();
		}
		break;
  
	case 4:     
		xleave = 0x2;      
		pos_adjust = 0;
		break;

	case 5: 
		if (ch0_mem_size == ch2_mem_size){ 
			xleave = 0x4;      
			pos_adjust = 1;
		}
		else {
			printf_err("[Error]: Channels 0 and 2 have different amounts of DRAM.\n");
			dram_spd_exit();
		}
		break;

	case 8:     
		xleave = 0x3;      
		pos_adjust = 0;
		break;

	case 10: 
		if (ch1_mem_size == ch3_mem_size){ 
			xleave = 0x5;      
			pos_adjust = 1;
		}
		else {
			printf_err("[Error]: Channels 1 and 3 have different amounts of DRAM.\n");
			dram_spd_exit();
		}
		break;

	case 15:     
		if ( (ch0_mem_size == ch1_mem_size) && 
		     (ch0_mem_size == ch2_mem_size) &&
		     (ch0_mem_size == ch3_mem_size) ){
			xleave = 0x7;      
			pos_adjust = 2;
		}
		else {
			printf_err("[Error]: Four channels populated have different amounts of DRAM.\n");
			dram_spd_exit();
		}
		break;

	default:
		printf_err("[Error]: Unsupported DRAM population pattern - {ch3,ch2,ch1,ch0} = %d %d %d %d\n", 
                        ((state & 0x8) >> 3), ((state & 0x4) >> 2), ((state & 0x2) >> 1), (state & 0x1)); 
		dram_spd_exit();
		break;
	}

	/* Note the following:                                                         */
	/* With  two 72-bit channels: total memory = 2 * ch0_mem_size.                 */
	/* With four 36-bit channels: total memory = 4 * (ch0_mem_size/2)              */
	/* (Recall that in 36-bit mode DIMMs are hacked to disable half their memory). */

	/* Because we are executing SPD code locked in the L2 at this point, the dummy */
	/* BAR (BAR 7) is still required to prevent address errors form being reported.*/
	/* Therefore, the final DRAM BAR mappings are programmed into BARs 0 - 6, but  */ 
	/* are NOT enabled at this point. They will be enabled by the assembly code    */
	/* once we return to the flash. That code will also erase the dummy BAR.       */

	/* BRIDGE BAR PROGRAMMING */
	SPD_PRINTF("Programming Bridge BARS and MTRS\n");
	/* Program BAR */
	data = (ba[idx][0] << 16)|(am[idx][0] << 4)|(xleave << 1);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_0_BAR, data);

	/* Program MTR */
	data = (part[idx][0]<<12) | (DIV[idx][0] << 8) | (pos[idx][0] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_0_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_0_BAR, data);

	/* Program BAR */
	data = (ba[idx][1] << 16)|(am[idx][1] << 4)|(xleave << 1);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_1_BAR, data);

	/* Program MTR */
	data = (part[idx][1] << 12)|(DIV[idx][1] << 8)| (pos[idx][1] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_1_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_1_BAR, data);

	/* Program BAR  */
	data = (ba[idx][2] << 16)|(am[idx][2] << 4)|(xleave << 1);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_2_BAR, data);

	/* Program MTR */
	data = (part[idx][2] << 12)|(DIV[idx][2] << 8)| (pos[idx][2] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_2_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_2_BAR, data);

	/* Program BAR  */
	data = (ba[idx][3] << 16)|(am[idx][3] << 4)|(xleave << 1);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_3_BAR, data);

	/* Program MTR */
	data = (part[idx][3] << 12)|(DIV[idx][3] << 8)| (pos[idx][3] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_3_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_3_BAR, data);

	/* Program BAR  */
	data = (ba[idx][4] << 16)|(am[idx][4] << 4)|(xleave << 1);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_4_BAR, data);

	/* Program MTR */
	data = (part[idx][4] << 12)|(DIV[idx][4] << 8)| (pos[idx][4] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_4_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_4_BAR, data);
  
	/* Program BAR  */
	data = (ba[idx][5] << 16)|(am[idx][5] << 4)|(xleave << 1); 
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_5_BAR, data);

	/* Program MTR */
	data = (part[idx][5] << 12)|(DIV[idx][5] << 8)| (pos[idx][5] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_5_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_5_BAR, data);

	/* Program BAR  */
	data = (ba[idx][6] << 16)|(am[idx][6] << 4)|(xleave << 1); 
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_6_BAR, data);

	/* Program MTR */
	data = (part[idx][6] << 12)|(DIV[idx][6] << 8)| (pos[idx][6] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_6_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_6_BAR, data);

	/* Program BAR  */
	data = (ba[idx][7] << 16)|(am[idx][7] << 4)|(xleave << 1); 
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_7_BAR, data);

	/* Program MTR */
	data = (part[idx][7] << 12)|(DIV[idx][7] << 8)| (pos[idx][7] - pos_adjust);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_7_BAR, data);
	phoenix_write_reg(mainbridgebase, BRIDGE_DRAM_CHN_1_MTR_7_BAR, data);


#ifdef SPD_DEBUG
	unsigned int      i;

	/* Wait for some time */
	for (i=0;i<1000000;i++){
	}
	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_0_BAR);
	printf("BRIDGE_DRAM_0_BAR readback value:           0x%x\n", data);
  
	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_0_BAR);
	printf("BRIDGE_DRAM_CHN_0_MTR_0_BAR readback value: 0x%x\n", data);

	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_1_BAR);
	printf("BRIDGE_DRAM_1_BAR readback value:           0x%x\n", data);

	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_1_BAR);
	printf("BRIDGE_DRAM_CHN_0_MTR_1_BAR readback value: 0x%x\n", data);

	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_2_BAR);
	printf("BRIDGE_DRAM_2_BAR readback value:           0x%x\n", data);

	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_2_BAR);
	printf("BRIDGE_DRAM_CHN_0_MTR_2_BAR readback value: 0x%x\n", data);

	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_3_BAR);
	printf("BRIDGE_DRAM_3_BAR readback value:           0x%x\n", data);

	data = phoenix_read_reg(mainbridgebase, BRIDGE_DRAM_CHN_0_MTR_3_BAR);
	printf("BRIDGE_DRAM_CHN_0_MTR_3_BAR readback value: 0x%x\n", data);
  
#endif

	return idx;
  
} /* END program_dram_bridge */

int channel_config(

	unsigned short    ch_id,                          /* Input: The channel for which the function is being called */
	unsigned short    spd_addr_dimm0,                 /* Input: DIMM0 SPD EEPROM address */
	unsigned short    spd_addr_dimm1,                 /* Input: DIMM1 SPD EEPROM address */
	unsigned long     clk_base_addr,                  /* Input: Dram clock registers base address */
	unsigned long     clk_cntrl_reg_addr,             /* Input: Dram clock control register address offset */
	unsigned long     clk_ratio_reg_addr,             /* Input: Dram clock ratio   register address offset */
	unsigned long     clk_reset_reg_addr,             /* Input: Dram controller reset register address offset */
	unsigned long     clk_status_reg_addr,            /* Input: Dram clock status  register address offset */
	int               *tck_start_denom_ptr,           /* Input/Output: Clock period upper limit. */
	enum mem_protocol *dram_type_ptr,                 /* Output: memory type (DDR1, DDR2, RLDRAM) */     
	int               ch_width,                       /* Input: Channel bit width, either 36 or 72 */ 
	int               desired_AL,                     /* Input: Desired additive latency, should be between 0 to 4, inclusive. */ 
	unsigned long     bridge_cfg_addr,                /* Input: Address of bridge config register */
	int               rld2_mode,                      /* Input: If 1, we are in RLD2 mode */
	unsigned long     ch_base_addr,                   /* Input: Base address for this channel's register space */
	int               *ecc_supported_ptr,             /* Output:If 1, dimm's support ECC */
	int               *loc_bank_ptr,                  /* Input/Output: Location of LSB of bank address bits */
	int               *loc_rank_ptr,                  /* Input/Output: Location of LSB of rank address bits */
	int               *loc_col_ptr,                   /* Input/Output: Location of LSB of col  address bits */
	int               *loc_row_ptr,                   /* Input/Output: Location of LSB of row  address bits */
	int               twoway_tboard,                  /* Input: 2-way XLR->DRAM->XLR additional delay due to board, */
	                                                  /*        dimm, package trace, in units of clock cycles. */ 
	int               *mem_size_ptr,                  /* Output: Amount of memory on this channel (in MB) */
	int               voltage,                        /* Input:  Voltage level at which this board is being operated. 0=Low, 1=Med, 2=High */ 
	char              chip_rev[]) {                   /* Input: XLR silicon revision ID. */ 

	/* Local variables */
	spd_entry         *spd_data_reference=NULL;            /* The data which will be decoded to calculate parameters */
	rat_entry         rank_allocation_table[4];            /* Array to hold SPD EEPROM address <-> logical rank <-> */
	/*   CS,ODT mapping information */ 
	int               cas_value_vect[3];                   /* Array to store DIMM-supported CAS values */
	int               cas_support_vect[3];                 /* Array to store DIMM-supported CAS vector */
	int               dimm_presence_vect[2];               /* Two bit vector to indicate if DIMM's are plugged in */
	int               cas_lat;                             /* Global CAS latency variable */
	int               treg = 0;                                /* 1 if registered DIMMs, 0 if not */
	int               blength;                             /* Burst length, i.e. 4 or 8 */
	int               chosen_AL=0;                         /* Chosen additive latency, 0 -> 4 */ 

	int               num_banks;       
	int               num_ranks;
	int               num_useful_col_bits;
	int               num_useful_cols;
	int               num_row_bits;

	/* I/O set up */
	phoenix_reg_t *dramchbase = phoenix_io_mmio(ch_base_addr);
	phoenix_reg_t *bridgebase = phoenix_io_mmio(bridge_cfg_addr);
	phoenix_reg_t *clkbase    = phoenix_io_mmio(clk_base_addr);

	if (rld2_mode == 1) {
		*dram_type_ptr = RLDRAM;
	}
	else{ 
        /* DDR1 or DDR2 mode, so use SPD */
		/* Read SPD data from DIMMs, & load data into arrays */
		dimm_presence_vect[0] = read_spd_data(spd_addr_dimm0, spd_data_dimm0);

		if (spd_addr_dimm1 != spd_addr_dimm0) {
			dimm_presence_vect[1] = read_spd_data(spd_addr_dimm1, spd_data_dimm1);
		}
		else {
			dimm_presence_vect[1] = 0;
		}

		/* If no DIMMs present, report an error */
		if ( (dimm_presence_vect[0] == 0) && (dimm_presence_vect[1] == 0) ){
			printf("No DIMMs on Channel%d\n", ch_id);
			return 1;
		}

		/* Run checksums, compare data equivalence */
		check_spd_integrity(spd_data_dimm0, spd_data_dimm1, dimm_presence_vect);

		/* Pick the SPD data to be decoded */
		if (dimm_presence_vect[0]){
			spd_data_reference = spd_data_dimm0;
		}
		else {
			spd_data_reference = spd_data_dimm1;
		}

		/* Decode SPD data */
		decode_spd_data (spd_data_reference, dram_type_ptr, cas_value_vect, cas_support_vect, &treg, ch_width, &blength,  ecc_supported_ptr);

		/* Initialize Rank Allocation Table (RAT) */
		init_RAT (spd_addr_dimm0, spd_addr_dimm1, spd_data_reference, dimm_presence_vect, rank_allocation_table, ch_width, ch_id); 

		/* Convert relevant bytes of SPD data from mixed numbers to fractions */
		convert_spd_array_to_fraction (spd_data_reference);

		/* Pick clock frequency based on DIMM SPD data, program it. */
		program_clock(spd_data_reference, clkbase, clk_cntrl_reg_addr, clk_ratio_reg_addr, clk_reset_reg_addr, clk_status_reg_addr, cas_value_vect, cas_support_vect, &cas_lat, tck_start_denom_ptr, ch_width, ch_id, treg, chip_rev);
  
		/* Convert relevant bytes of SPD data from fractions to clock cycles */
		convert_spd_array_to_cycles (spd_data_reference, *tck_start_denom_ptr);

#ifdef SPD_DEBUG
		print_spd(spd_data_reference);
#endif

		/* Pick additive latency value */
		chosen_AL = compute_AL(spd_data_reference, dram_type_ptr, desired_AL, cas_lat, twoway_tboard, treg);
		SPD_PRINTF(" Chosen AL: %d Chosen CL:%d\n", chosen_AL, cas_lat);

		/* Calculate amount of memory on this channel*/
		*mem_size_ptr = calc_mem_size(spd_data_reference, dimm_presence_vect);

	} /* END DDR1 or DDR2 mode */

	/* Program */
        program_bridge_cfg(bridgebase, ch_width, chip_rev);
        program_dyn_data_pad_ctrl(dramchbase, ch_width, ch_id, chip_rev);
	program_glb_params(dramchbase, spd_data_reference, ch_width, rld2_mode, *dram_type_ptr, chip_rev);
	program_write_ODT_mask(dramchbase, rank_allocation_table, dimm_presence_vect, *dram_type_ptr);
	program_read_ODT_mask(dramchbase, rank_allocation_table, dimm_presence_vect, *dram_type_ptr);
	program_addr_params(dramchbase, 
			    rank_allocation_table, 
			    spd_data_reference, 
			    dimm_presence_vect, 
			    ch_width, 
			    loc_bank_ptr, 
			    loc_col_ptr, 
			    loc_row_ptr, 
			    loc_rank_ptr,  
			    &num_banks, 
			    &num_ranks, 
			    &num_useful_col_bits, 
			    &num_useful_cols, 
			    &num_row_bits); 
	program_params_1(dramchbase, spd_data_reference, *dram_type_ptr); 
	program_params_2(dramchbase, spd_data_reference, blength, chosen_AL); 
	program_params_5(dramchbase, spd_data_reference); 
	program_params_6(dramchbase, blength, chosen_AL, cas_lat, *tck_start_denom_ptr ); 
	program_params_7(dramchbase, blength, cas_lat, spd_data_reference, *dram_type_ptr);
	program_mrs(dramchbase, spd_data_reference, blength, cas_lat, *dram_type_ptr);
	program_rdmrs(dramchbase, spd_data_reference, blength, cas_lat, *dram_type_ptr);
	program_emrs1(dramchbase, chosen_AL, *dram_type_ptr, dimm_presence_vect);
	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_ECC_POISON_MASK, 0);
	program_tx0_therm_ctrl(dramchbase, *dram_type_ptr);
	program_rx0_therm_ctrl(dramchbase, dimm_presence_vect);
	program_reset_sequence(dramchbase, *dram_type_ptr);
	calibrate_channel_strobes(num_banks, 
				  *loc_bank_ptr, 
				  num_ranks, 
				  *loc_rank_ptr, 
				  num_useful_col_bits, 
				  num_useful_cols, 
				  *loc_col_ptr, 
				  num_row_bits, 
				  *loc_row_ptr, 
				  chosen_AL, 
				  cas_lat, 
				  treg, 
				  blength, 
				  dramchbase, 
				  bridgebase, 
				  spd_data_reference, 
				  *dram_type_ptr,  
				  ch_id, 
				  dimm_presence_vect,
				  voltage,
				  *tck_start_denom_ptr, 
				  chip_rev);

#if SDP_DEBUG
	/* Read back and print programmed values */
	read_programmed_registers(dramchbase, ch_width, *dram_type_ptr, ch_id);
#endif
	return 0;
}

void check_spd_integrity(spd_entry *spd_data_ptr0, spd_entry *spd_data_ptr1, int presence_vect[]){

	int       dimm0_checksum = 0;
	int       dimm1_checksum = 0;
	int       i;
	enum mem_protocol mem_type = RLDRAM;

	for (i=0; i < SPD_DATA_CHECKSUM_BYTE; i++, spd_data_ptr0++, spd_data_ptr1++){
		dimm0_checksum = dimm0_checksum + spd_data_ptr0->raw_data;
		dimm1_checksum = dimm1_checksum + spd_data_ptr1->raw_data;
      
		if ((i == ARIZONA_SPD_MEM_TYPE) && (spd_data_ptr0->raw_data == SPD_DATA_ENC_MEMTYPE_DDR1)){
			mem_type = DDR1;
		}
		else if ((i == ARIZONA_SPD_MEM_TYPE) && (spd_data_ptr0->raw_data == SPD_DATA_ENC_MEMTYPE_DDR2)){
			mem_type = DDR2;
		}

		if ( presence_vect[0] && 
		     presence_vect[1] && 
		     (spd_data_ptr0->raw_data != spd_data_ptr1->raw_data)){ /* SPD byte mismatch found */

			if ( ( (mem_type == DDR1) && ( ((i > 35) && (i < 41)) || (i == 46) || ((i > 47) && (i < 63)  ) ) ) ||  /* DDR1 SPD Bytes 36 to 40, 46, 48 to 61 are reserved. 62 is SPD version number */
			     ( (mem_type == DDR2) && ( (i ==  7)              || (i == 15) || (i == 19) || (i == 62) ) ) ) {   /* DDR2 SPD Bytes  7, 15, and 19 are reserved. 62 is SPD version number*/
				printf_err("Warning: mismatched SPD data on reserved byte %d - DIMM0: %x DIMM1: %x\n", i, spd_data_ptr0->raw_data, spd_data_ptr1->raw_data );
			}
			else {
				printf_err("[Error]: mismatched SPD data on byte %d - DIMM0: %x DIMM1: %x\n", i, spd_data_ptr0->raw_data, spd_data_ptr1->raw_data );
				dram_spd_exit();
			}
		}
	} /* END for */

	dimm0_checksum = dimm0_checksum % 256;
	dimm1_checksum = dimm1_checksum % 256;

	if ( presence_vect[0] && (spd_data_ptr0->raw_data != dimm0_checksum) ){
		printf_err("[Error]: Checksum mismatch on DIMM0 SPD data - Calculated: 0x%x DIMM: 0x%x\n", dimm0_checksum, spd_data_ptr0->raw_data );
		dram_spd_exit();
	}

	if ( presence_vect[1] && (spd_data_ptr1->raw_data != dimm1_checksum) ){
		printf_err("[Error]: Checksum mismatch on DIMM1 SPD data - Calculated: 0x%x DIMM: 0x%x\n", dimm1_checksum, spd_data_ptr1->raw_data );
		dram_spd_exit();
	}
} /* END check_spd_integrity */

void init_spd_data (spd_entry *d_ptr){

	int i;

	for (i=0; i<SPD_DATA_NUM_BYTES; i++){
		d_ptr->raw_data = 0;
		d_ptr->whole   = 0;
		d_ptr->num     = 0;
		d_ptr->denom   = 1;
		d_ptr++;
	}
} /* END init_spd_data */


int dummy_read_spd_eeprom(unsigned short addr, unsigned short i, 
        unsigned short *res_ptr, int col_index) {

    /* Only 1 simulated DIMM
     * slot is supported.
     */
    if (addr == SPD_EEPROM0) {
        *res_ptr = fake_spd[i][col_index];
        return 0;
    }
    else {
        *res_ptr = 0;
        return -1;
    }
} 

int read_spd_data (unsigned short addr, spd_entry *data_ptr) {

    unsigned short i;
    int status;
    unsigned short res;
    unsigned char data;
    volatile int mux = 0;

#ifdef CFG_BOARD_RMIREF
    unsigned short major=0, minor=0;
    
    read_eeprom(MAJOR_OFFSET, &major);
    read_eeprom(MINOR_OFFSET, &minor);

    switch (major) {
        case '3':
            use_fake_spd = 1;
            if (minor == '1')
                col_index = 1;
            break;
        case '5':
            use_fake_spd = 1;
            col_index = 2;
            break;
        default:
            break;
    }
#endif  /* CFG_BOARD_RMIREF */
    for (i=0; i<SPD_DATA_NUM_BYTES; i++) {

        /* Use fake spd values for: -
         *  : RMI PCIX Boards [256/512MB Versions, 308 Card]
         *  : Template boards with no DIMM Slots
         */
        if (use_fake_spd) {
            status = dummy_read_spd_eeprom(addr, i, &res, col_index);
        }
        else {
#ifdef CONFIG_FX    
            if (fx_use_i2c_cpld(addr)) {
                mux = TOR_CPLD_I2C_CREATE_MUX(0, 0, 2);
                status = tor_cpld_i2c_read(mux, addr, i, TOR_CPLD_I2C_8_OFF | TOR_CPLD_I2C_RSTART, 
                                            &data, 1);
                /* kludge, no DIMM present */
                if ((i == 0) && (data != 0x80)) {
                    return 0;
                }
                res = data;
                if (i % 16 == 0)
                    printf("\n");
                printf("%02x ", data);
            }
            else {
                status = dram_read_spd_eeprom(addr, i, &res);
            }
#else
            status = dram_read_spd_eeprom(addr, i, &res);
#endif
        }
        if (status == -1) {
            return 0;
        }
        else {
            (data_ptr++)->raw_data = (int) res;
        }
    }
    return 1; 
}

void decode_spd_data (spd_entry *data_ptr, enum mem_protocol *mem_type_ptr, int *cas_value_vect, int *cas_support_vect, int *treg_ptr, int ch_width, int *burst_length_ptr, int *ecc_supported_ptr){

	spd_entry  *offset_ptr;
	int temp;

	/* -------------------Byte 2: Fundamental memory type (encoded) -------------------------*/
	offset_ptr = data_ptr + ARIZONA_SPD_MEM_TYPE;
	if (offset_ptr->raw_data == SPD_DATA_ENC_MEMTYPE_DDR1){
		*mem_type_ptr = DDR1;
	}
	else if (offset_ptr->raw_data == SPD_DATA_ENC_MEMTYPE_DDR2){
		*mem_type_ptr = DDR2;
	}
	else {
		printf_err("ERROR - Illegal/unsupported memory type (SPD byte 2):0x%x\n", offset_ptr->raw_data);
	}

	/* -------------------Byte 3: Number of row addresses (integer) -------------------------*/
	/* NOTE: This version of the code will not support DIMMs which have multiple ranks
	   of different sizes on the same DIMM (seems to be allowed by DDR1 SPD spec). */ 
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_NUM_ROWS;

	if (offset_ptr->raw_data == 0) {
		printf_err("ERROR - Illegal zero value for SPD byte 3\n");
		dram_spd_exit();
	}

	temp = offset_ptr->raw_data & 0xF0;

	if (temp != 0) {
		printf_err("ERROR - SPD byte 3 bits 7:4 illegally set\n");
		dram_spd_exit();
	}

	offset_ptr->whole = offset_ptr->raw_data & 0x0F; /* whole = byte3[3:0] */

	/* -------------------Byte 4: Number of column addresses (integer) -------------------------*/
	/* NOTE: This version of the code will not support DIMMs which have multiple ranks
	   of different sizes on the same DIMM (seems to be allowed by DDR1 SPD spec). */ 
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_NUM_COLS;

	if (offset_ptr->raw_data == 0) {
		printf_err("ERROR - Illegal zero value for SPD byte 4\n");
		dram_spd_exit();
	}

	temp = offset_ptr->raw_data & 0xF0;

	if (temp != 0) {
		printf_err("ERROR - SPD byte 4 bits 7:4 illegally set\n");
		dram_spd_exit();
	}

	offset_ptr->whole = offset_ptr->raw_data & 0x0F; /* whole = byte4[3:0] */

	/* --------- Byte 5: Number of ranks (DDR1), Module attributes (DDR2) (integer) ---------*/
	/* NOTE: This version of the code will not support DDR1 DIMMs which have more than
	   8 ranks on the same DIMM (assuming that they exist). */ 
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_NUM_PHYS_BANKS;

	if (*mem_type_ptr == DDR1){
		if (offset_ptr->raw_data == 0) {
			printf_err("ERROR - Illegal zero value for SPD byte 5\n");
			dram_spd_exit();
		}
		else {
			offset_ptr->whole = offset_ptr->raw_data; /* whole = byte5 */
		}
	}
	else { /* DDR 2 */
		offset_ptr->whole = (offset_ptr->raw_data & 0x07) + 1; /* whole = byte5[2:0]+1 */
	}


	/* --------- Byte 9: Minimum SDRAM cycle time @ maximum CAS  (whole & tenths, fractions) ---------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_MAX_CYCLE_TIME;

	if (offset_ptr->raw_data  == 15) {
		printf_err("ERROR - Illegal all-one value for SPD byte 9\n");
		dram_spd_exit();
	}
	temp = (offset_ptr->raw_data & 0xF0) >> 4;

	if (temp == 0) {
		printf_err("ERROR - Illegal zero value for SPD byte 9 bits [7:4]\n");
		dram_spd_exit();
	}
	else {
		offset_ptr->whole = temp; /* whole = byte9[7:4] */
	}

	temp = offset_ptr->raw_data & 0x0F;

	if ( (temp >= 0) && (temp <= 9) ) {
		offset_ptr->num   = temp; /* num = byte9[3:0] */
		offset_ptr->denom = 10;   /* denom = 10 */
	}
	else{
		if (*mem_type_ptr == DDR1){
			printf_err("ERROR - Illegal value for DDR1 SPD byte 9 bits[3:0] %x\n", temp);
			dram_spd_exit();
		}
		else if (temp == 0xA){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else if (temp == 0xB){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xC){
			offset_ptr->num   = 2; /* num   = 2 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xD){
			offset_ptr->num   = 3; /* num   = 3 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else {
			printf_err("ERROR - Illegal value for DDR2 SPD byte 9 bits[3:0] %x\n", temp);
			dram_spd_exit();
		}
	}

	/* --------- Byte 11: DIMM configuration type (special encoding for DDR1, bit vector for DDR2) ---------*/
	/* NOTE: Whether DDR1 or DDR2, enable ECC if whole = 2 */
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_DIMM_ECC_PARITY;

	if (*mem_type_ptr == DDR1){
		offset_ptr->whole = offset_ptr->raw_data; /* whole = byte11 */
	}
	else {
		offset_ptr->whole = offset_ptr->raw_data & 0x02; /* whole = byte 11, 6'b0, bit[1], 1'b0 */
	}

	*ecc_supported_ptr = 0;
	if (offset_ptr->whole == 0x02){
		*ecc_supported_ptr = 1;
	}
	else if (offset_ptr->whole != 0x02){
		printf("ECC is not supported by the DIMMs\n");
	}

	/* --------- Byte 12: Refresh rate, type  (special encoding) ---------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_DIMM_REFRESH_RATE;

	if ( (offset_ptr->raw_data & 0x7F) == 0x00){
		offset_ptr->whole =  15625;
	}
	else if ( (offset_ptr->raw_data & 0x7F) == 0x01){
		offset_ptr->whole =   3900;
	}
	else if ( (offset_ptr->raw_data & 0x7F) == 0x02){
		offset_ptr->whole =   7800;
	}
	else if ( (offset_ptr->raw_data & 0x7F) == 0x03){
		offset_ptr->whole =  31300;
	}
	else if ( (offset_ptr->raw_data & 0x7F) == 0x04){
		offset_ptr->whole =  62500;
	}
	else if ( (offset_ptr->raw_data & 0x7F) == 0x05){
		offset_ptr->whole = 125000;
	}
	else {
		printf_err("ERROR - Illegal value for SPD byte 12: 0x%x\n", offset_ptr->raw_data );   
		dram_spd_exit(); 
	}

	/* --------- Byte 13: Primary SDRAM width  (integer for DDR2) ---------*/
	/* NOTE 1: Phoenix memory controller does not support x4 devices.
	   NOTE 2: This version of software does not support DIMMs with multiple ranks of different
	   organizations on the same DIMM (e.g. one rank uses x8's, other ranks uses x16's).
	   NOTE 3: Phoenix memory controller does not support DDR1/2 DRAM chips with integrated ECC, 
	   i.e. x9, x18, x36, etc. These devices are supported only in RLDRAM mode
	*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_PRIMARY_WIDTH;

	if ( (offset_ptr->raw_data == 0x08) || (offset_ptr->raw_data == 0x10) || (offset_ptr->raw_data == 0x20) ){
		offset_ptr->whole = offset_ptr->raw_data;
	}
	else {
		printf_err("ERROR - Illegal/unsupported value for SPD byte 13: 0x%x\n", offset_ptr->raw_data );   
		dram_spd_exit(); 
	}

	/* --------- Byte 15: Minimum clock delay, back-to-back random column access [DDR1] (integer) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tCCD_MIN;

	if (*mem_type_ptr == DDR1){

		if (offset_ptr->raw_data == 0){
			printf_err("ERROR - Illegal all-zero value for DDR1 SPD data byte 15\n" ); 
			dram_spd_exit();
		}
		else {
			offset_ptr->whole = offset_ptr->raw_data; /* whole = byte 15 */
		}
	}

	/* --------- Byte 16: Supported burst lengths (bit vector) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_BL_SUPPORTED;
	offset_ptr->whole = offset_ptr->raw_data & 0x0c; /* whole = {4'b0, byte 16[3:2], 2'b0} */

	if ( (ch_width == 36) && ((offset_ptr->whole & 0x08) != 0) ){
		*burst_length_ptr = 8;
	}
	else if ( (ch_width == 72) && ((offset_ptr->whole & 0x04) != 0) ){
		*burst_length_ptr = 4;
	}
	else {
		printf_err("ERROR - Illegal channel width/BL pair:%d 0x%x\n", ch_width, offset_ptr->raw_data); 
		dram_spd_exit();
	}

	/* --------- Byte 17: Number of banks (integer) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_NUM_BANKS;

	if (offset_ptr->raw_data == 0){
		printf_err("ERROR - Illegal all-zero value for SPD data byte 17\n" );    
		dram_spd_exit();
	}
	else {
		offset_ptr->whole = offset_ptr->raw_data; /* whole = byte 17 */
	}

	/* --------- Byte 18: CAS latency (bit vector) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_CASLAT;

	if (*mem_type_ptr == DDR1){

		if (offset_ptr->raw_data & 0x40){
			*cas_value_vect++ = 4;
			*cas_value_vect++ = 0; /* 3.5 not supported */
			*cas_value_vect   = 3;
			*cas_support_vect++ = 1;
			*cas_support_vect++ = 0;
			*cas_support_vect   = (offset_ptr->raw_data & 0x10) >> 4;
		}
		else if (offset_ptr->raw_data & 0x20){
			*cas_value_vect++ = 0; /* 3.5 not supported */
			*cas_value_vect++ = 3; 
			*cas_value_vect   = 0; /* 2.5 not supported */
			*cas_support_vect++ = 0;
			*cas_support_vect++ = (offset_ptr->raw_data & 0x10) >> 4;
			*cas_support_vect   = 0;
		}
		else if (offset_ptr->raw_data & 0x10){
			*cas_value_vect++ = 3; 
			*cas_value_vect++ = 0; /* 2.5 not supported */ 
			*cas_value_vect   = 2;
			*cas_support_vect++ = 1;
			*cas_support_vect++ = 0;
			*cas_support_vect   = (offset_ptr->raw_data & 0x04) >> 2;
		}
		else if (offset_ptr->raw_data & 0x08){
			*cas_value_vect++ = 0; /* 2.5 not supported */ 
			*cas_value_vect++ = 2; 
			*cas_value_vect   = 0; /* 1.5 not supported */
			*cas_support_vect++ = 0;
			*cas_support_vect++ = (offset_ptr->raw_data & 0x04) >> 2;
			*cas_support_vect   = 0;
		}
		else if (offset_ptr->raw_data & 0x04){
			*cas_value_vect++ = 2; 
			*cas_value_vect++ = 0; /* 1.5 not supported */
			*cas_value_vect   = 1; 
			*cas_support_vect++ = 1;
			*cas_support_vect++ = 0;
			*cas_support_vect   = offset_ptr->raw_data & 0x01;
		}
		else if (offset_ptr->raw_data & 0x02){
			*cas_value_vect++  = 0; /* 1.5 not supported */ 
			*cas_value_vect++  = 1;
			*cas_value_vect    = 0; /* No value */ 
			*cas_support_vect++ = 0;
			*cas_support_vect++ = offset_ptr->raw_data & 0x01;
			*cas_support_vect   = 0;
		}
		else if (offset_ptr->raw_data & 0x01){
			*cas_value_vect++  = 1; 
			*cas_value_vect++  = 0; /* No value */
			*cas_value_vect    = 0; /* No value */ 
			*cas_support_vect++ = 1;
			*cas_support_vect++ = 0;
			*cas_support_vect   = 0;
		}
		else {
			printf_err("ERROR - No supported CAS latency settings found in DDR1 SPD data byte 18\n" );    
			dram_spd_exit();  
		}
   
		if ( (*cas_support_vect-- == 0) && (*cas_support_vect-- == 0) && (*cas_support_vect == 0) ){
			printf_err("ERROR - No XLR-compatible supported CAS latency settings found in DDR1 SPD data byte 18\n" );    
			dram_spd_exit();  
		}
	}
	else { /* DDR2 */
		if (offset_ptr->raw_data & 0x40){
			*cas_value_vect++ = 6;
			*cas_value_vect++ = 5;
			*cas_value_vect   = 4;
			*cas_support_vect++ = 1;
			*cas_support_vect++ = (offset_ptr->raw_data & 0x20) >> 5;
			*cas_support_vect   = (offset_ptr->raw_data & 0x10) >> 4;
		}
		else if (offset_ptr->raw_data & 0x20){
			*cas_value_vect++ = 5;
			*cas_value_vect++ = 4;
			*cas_value_vect   = 3;
			*cas_support_vect++ = 1;
			*cas_support_vect++ = (offset_ptr->raw_data & 0x10) >> 4;
			*cas_support_vect   = (offset_ptr->raw_data & 0x08) >> 3;
		}
		else if (offset_ptr->raw_data & 0x10){
			*cas_value_vect++ = 4;
			*cas_value_vect++ = 3;
			*cas_value_vect   = 2;
			*cas_support_vect++ = 1;
			*cas_support_vect++ = (offset_ptr->raw_data & 0x08) >> 3;
			*cas_support_vect   = (offset_ptr->raw_data & 0x04) >> 2;
		}
		else if (offset_ptr->raw_data & 0x08){
			*cas_value_vect++ = 3;
			*cas_value_vect++ = 2;
			*cas_value_vect   = 0; /* No value */
			*cas_support_vect++ = 1;
			*cas_support_vect++ = (offset_ptr->raw_data & 0x08) >> 3;
			*cas_support_vect   = 0;
		}
		else if (offset_ptr->raw_data & 0x04){
			*cas_value_vect++ = 2;
			*cas_value_vect++ = 0; /* No value */
			*cas_value_vect   = 0; /* No value */
			*cas_support_vect++ = 1;
			*cas_support_vect++ = 0;
			*cas_support_vect   = 0;
		}
		else {
			printf_err("ERROR - No supported CAS latency settings found in DDR2 SPD data byte 18\n" );    
			dram_spd_exit();  
		}
	}

	/* --------- Byte 20: WE latency [DDR1] (bit vector), DIMM type [DDR2] (bit vector) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_DIMM_TYPE;

	if (*mem_type_ptr == DDR1){
	}
	else { /* DDR2 */
		temp = (offset_ptr->raw_data) & 0x3f; /* Mask out unsupported configurations.*/
		offset_ptr->whole = temp;     

		if (temp == 0x01) {
			*treg_ptr = 1; /* DDR2 registered DIMM, or DDR2 VLP registered DIMM, so set treg = 1*/
		}
		else if (temp == 0x02) {
			*treg_ptr = 0; /* DDR2 unbuffered DIMM, or DDR2 VLP unbuffered DIMM, so set treg = 0*/   
		}
		else if (temp == 0x04) {
			*treg_ptr = DDR2_SODIMM_DEF_TYPE; /* DDR2 SO-DIMM, or DDR2 VLP SO-DIMM */   
			printf("WARNING - Detected DDR2 SO-DIMM form factor on this channel.\n");    
			printf("JEDEC DDR2 SPD specification does not identify registered vs. unbuffered.\n");
			printf("Assumed type (0 for unbuffered, 1 for registered):%d\n", *treg_ptr);
			printf("If this is incorrect, please modify DDR2_SODIMM_DEF_TYPE parameter \n");
			printf("in include/mem_ctrl.h bootloader file.\n");
		}
		else if (temp == 0x08) {
			*treg_ptr = DDR2_MICRODIMM_DEF_TYPE; /* DDR2 MicroDIMM, or DDR2 VLP MicroDIMM */   
			printf("WARNING - Detected DDR2 MicroDIMM form factor on this channel.\n");    
			printf("JEDEC DDR2 SPD specification does not identify registered vs. unbuffered.\n");
			printf("Assumed type (0 for unbuffered, 1 for registered):%d\n", *treg_ptr);
			printf("If this is incorrect, please modify DDR2_MICRODIMM_DEF_TYPE parameter\n");
			printf("in include/mem_ctrl.h bootloader file.\n");
		}
		else if (temp == 0x10) {
			*treg_ptr = 1; /* DDR2 registered mini-DIMM, or DDR2 VLP registered mini-DIMM, so set treg = 1*/   
		}
		else if (temp == 0x20) {
			*treg_ptr = 0; /* DDR2 unbuffered mini-DIMM, or DDR2 VLP unbuffered mini-DIMM, so set treg = 0*/   
		}
		else {
			printf_err("ERROR - Illegal/unsupported value for DDR2 SPD data byte 20: 0x%x\n", temp);    
			dram_spd_exit();
		}

	}

	/* --------- Byte 21: SDRAM module attributes [DDR1] (bit vector), irrelevant stuff [DDR2] --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_BUF_REG;

	if (*mem_type_ptr == DDR1){
		temp = (offset_ptr->raw_data) & 0x02; /* Mask out irrelevant stuff.*/
		offset_ptr->whole = temp;     

		if (temp == 0x02){ 
			*treg_ptr = 1; /* If registered DDR1 DIMM, set treg = 1*/   
		}
		else{
			*treg_ptr = 0; /* If unbuffered DDR1 DIMM, set treg = 0*/   
		}
	}

	/* ---------  Byte 23: Minimum SDRAM cycle time @CAS-0.5[DDR1]/@CAS-1[DDR2] (whole & tenths, fractions) --------*/
	/* NOTE 1: This code assumes [3:0] = 1 implies 1ns, not 16ns. (which is also permitted by the JEDEC SPD spec).
	   NOTE 2: This code assumes [3:0] = 2 implies 2ns, not 17ns. (which is also permitted by the JEDEC SPD spec).
	*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_HALF;

	if (offset_ptr->raw_data  == 15) {
		printf_err("ERROR - Illegal all-one value for SPD byte 23\n");
		dram_spd_exit();
	}
	temp = (offset_ptr->raw_data & 0xF0) >> 4;

	/* Removed the [7:4] = 0 error message. Apparently, this field WILL be zero if:
	   -a DDR1 DIMM does not support a CAS latency of CASmax-0.5, or
	   -a DDR2 DIMM does not support a CAS latency of CASmax-1
	*/
	offset_ptr->whole = temp; /* whole = byte23 [7:4] */

	temp = offset_ptr->raw_data & 0x0F;

	if ( (temp >= 0) && (temp <= 9) ) {
		offset_ptr->num   = temp; /* num = byte23 [3:0] */
		offset_ptr->denom = 10;   /* denom = 10 */
	}
	else{
		if (*mem_type_ptr == DDR1){
			printf_err("ERROR - Illegal value for DDR1 SPD byte 23 bits[3:0] %x\n", temp);
			dram_spd_exit();
		}
		else if (temp == 0xA){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else if (temp == 0xB){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xC){
			offset_ptr->num   = 2; /* num   = 2 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xD){
			offset_ptr->num   = 3; /* num   = 3 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else {
			printf_err("ERROR - Illegal value for DDR2 SPD byte 23 bits[3:0] %x\n", temp);
			dram_spd_exit();
		}
	}

	/* ---------  Byte 25: Minimum SDRAM cycle time @CAS-1[DDR1]/@CAS-2[DDR2] (whole & tenths, fractions) --------*/
	/* NOTE 1: This code assumes [3:0] = 1 implies 1ns, not 16ns. (which is also permitted by the JEDEC SPD spec).
	   NOTE 2: This code assumes [3:0] = 2 implies 2ns, not 17ns. (which is also permitted by the JEDEC SPD spec).
	*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_ONE;

	if (offset_ptr->raw_data  == 15) {
		printf_err("ERROR - Illegal all-one value for SPD byte 25\n");
		dram_spd_exit();
	}
	temp = (offset_ptr->raw_data & 0xF0) >> 4;

	/* Removed the [7:4] = 0 error message. Apparently, this field WILL be zero if:
	   -a DDR1 DIMM does not support a CAS latency of CASmax-1, or
	   -a DDR2 DIMM does not support a CAS latency of CASmax-2
	*/
	offset_ptr->whole = temp; /* whole = byte25 [7:4] */

	temp = offset_ptr->raw_data & 0x0F;

	if ( (temp >= 0) && (temp <= 9) ) {
		offset_ptr->num   = temp; /* num = byte25 [3:0] */
		offset_ptr->denom = 10;   /* denom = 10 */
	}
	else{
		if (*mem_type_ptr == DDR1){
			printf_err("ERROR - Illegal value for DDR1 SPD byte 25 bits[3:0] %x\n", temp);
			dram_spd_exit();
		}
		else if (temp == 0xA){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else if (temp == 0xB){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xC){
			offset_ptr->num   = 2; /* num   = 2 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xD){
			offset_ptr->num   = 3; /* num   = 3 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else {
			printf_err("ERROR - Illegal value for DDR2 SPD byte 25 bits[3:0] %x\n", temp);
			dram_spd_exit();
		}
	}

	/* ---------  Byte 27: tRP (whole & quarters) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRP;

	temp = (offset_ptr->raw_data & 0xFC) >> 2;

	if (temp == 0){
		printf_err("ERROR - Illegal all-zero value for SPD byte 27 bits[7:2] %x\n", temp);
		dram_spd_exit();
	}

	offset_ptr->whole = temp;                        /* whole = byte27 [7:2] */
	offset_ptr->num   = offset_ptr->raw_data & 0x03; /* num   = byte27 [1:0] */
	offset_ptr->denom = 4;                           /* denom = 4 */

	/* ---------  Byte 28: tRRD (whole & quarters) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRRD;

	temp = (offset_ptr->raw_data & 0xFC) >> 2;

	if (temp == 0){
		printf_err("ERROR - Illegal all-zero value for SPD byte 28 bits[7:2] %x\n", temp);
		dram_spd_exit();
	}

	offset_ptr->whole = temp;                        /* whole = byte28 [7:2] */
	offset_ptr->num   = offset_ptr->raw_data & 0x03; /* num   = byte28 [1:0] */
	offset_ptr->denom = 4;                           /* denom = 4 */

	/* ---------  Byte 29: tRCD (whole & quarters) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRCD;

	temp = (offset_ptr->raw_data & 0xFC) >> 2;

	if (temp == 0){
		printf_err("ERROR - Illegal all-zero value for SPD byte 29 bits[7:2] %x\n", temp);
		dram_spd_exit();
	}

	offset_ptr->whole = temp;                        /* whole = byte29 [7:2] */
	offset_ptr->num   = offset_ptr->raw_data & 0x03; /* num   = byte29 [1:0] */
	offset_ptr->denom = 4;                           /* denom = 4 */

	/* ---------  Byte 30: tRAS (integer) --------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRAS;

	if (offset_ptr->raw_data == 0){
		printf_err("ERROR - Illegal all-zero value for SPD byte 30.\n");
		dram_spd_exit();
	}
	offset_ptr->whole = offset_ptr->raw_data;

	/* ---------  Byte 31: Module rank density (bit vector) --------*/
	/* NOTE 1: Stores the rank size (in MB) in whole field. 
	   NOTE 2: For DDR1 DIMMs, it is assumed that [0] set implies 1GB ranks, not  4MB ranks (which is also permitted by JEDEC SPD spec).
	   NOTE 3: For DDR1 DIMMs, it is assumed that [1] set implies 2GB ranks, not  8MB ranks (which is also permitted by JEDEC SPD spec).
	   NOTE 4: For DDR1 DIMMs, it is assumed that [2] set implies 4GB ranks, not 16MB ranks (which is also permitted by JEDEC SPD spec).
	   NOTE 5: This code does not support DIMMs that contain multiple ranks of different sizes. 
	*/

	offset_ptr = data_ptr + ARIZONA_SPD_DDR_DENSITY;

	if (*mem_type_ptr == DDR1){

		if(offset_ptr->raw_data == 0x01){
			offset_ptr->whole = 1024;
		}
		else if(offset_ptr->raw_data == 0x02){
			offset_ptr->whole = 2048;
		}
		else if(offset_ptr->raw_data == 0x04){
			offset_ptr->whole = 4096;
		}
		else if(offset_ptr->raw_data == 0x08){
			offset_ptr->whole =   32;
		}
		else if(offset_ptr->raw_data == 0x10){
			offset_ptr->whole =   64;
		}
		else if(offset_ptr->raw_data == 0x20){
			offset_ptr->whole =  128;
		}
		else if(offset_ptr->raw_data == 0x40){
			offset_ptr->whole =  256;
		}
		else if(offset_ptr->raw_data == 0x80){
			offset_ptr->whole =  512;
		}
		else {
			printf_err("ERROR - Illegal/unsupported value for DDR1 SPD byte 31: 0x%x\n", offset_ptr->raw_data);
			dram_spd_exit();
		}
	}
	else { /* DDR2 */
		if(offset_ptr->raw_data == 0x01){
			offset_ptr->whole =  1024;
		}
		else if(offset_ptr->raw_data == 0x02){
			offset_ptr->whole =  2048;
		}
		else if(offset_ptr->raw_data == 0x04){
			offset_ptr->whole =  4096;
		}
		else if(offset_ptr->raw_data == 0x08){
			offset_ptr->whole =  8192;
		}
		else if(offset_ptr->raw_data == 0x10){
			offset_ptr->whole = 16384;
		}
		else if(offset_ptr->raw_data == 0x20){
			offset_ptr->whole =   128;
		}
		else if(offset_ptr->raw_data == 0x40){
			offset_ptr->whole =   256;
		}
		else if(offset_ptr->raw_data == 0x80){
			offset_ptr->whole =   512;
		}
		else {
			printf_err("ERROR - Illegal/unsupported value for DDR2 SPD byte 31: 0x%x\n", offset_ptr->raw_data);
			dram_spd_exit();
		}
	}

	/* ---------  Byte 36: tWR (whole & quarters) [DDR2 only]--------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tWR;

	if (*mem_type_ptr == DDR2){

		temp = (offset_ptr->raw_data & 0xFC) >> 2;

		if (temp == 0){
			printf_err("ERROR - Illegal all-zero value for DDR2 SPD byte 36 bits[7:2] %x\n", temp);
			dram_spd_exit();
		}

		offset_ptr->whole = temp;                        /* whole = byte36 [7:2] */
		offset_ptr->num   = offset_ptr->raw_data & 0x03; /* num   = byte36 [1:0] */
		offset_ptr->denom = 4;                           /* denom = 4 */

	}
	else{
		offset_ptr->whole = SPD_DATA_DDR1_tWR;           /* whole = 15ns */
		offset_ptr->num   = 0;                           /* num   = 0 */
		offset_ptr->denom = 1;                           /* denom = 1 */    
	}

	/* ---------  Byte 37: tWTR (whole & quarters) [DDR2 only]--------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tWTR;

	if (*mem_type_ptr == DDR2){

		temp = (offset_ptr->raw_data & 0xFC) >> 2;

		if (temp == 0){
			printf_err("ERROR - Illegal all-zero value for DDR2 SPD byte 37 bits[7:2] %x\n", temp);
			dram_spd_exit();
		}

		offset_ptr->whole = temp;                        /* whole = byte37 [7:2] */
		offset_ptr->num   = offset_ptr->raw_data & 0x03; /* num   = byte37 [1:0] */
		offset_ptr->denom = 4;                           /* denom = 4 */

	}

	/* ---------  Byte 38: tRTP (whole & quarters) [DDR2 only]--------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRTP;

	if (*mem_type_ptr == DDR2){

		temp = (offset_ptr->raw_data & 0xFC) >> 2;

		if (temp == 0){
			printf_err("ERROR - Illegal all-zero value for DDR2 SPD byte 38 bits[7:2] %x\n", temp);
			dram_spd_exit();
		}

		offset_ptr->whole = temp;                        /* whole = byte38 [7:2] */
		offset_ptr->num   = offset_ptr->raw_data & 0x03; /* num   = byte38 [1:0] */
		offset_ptr->denom = 4;                           /* denom = 4 */

	}

	/* ---------  Byte 41 & 40: tRC (integer) [with extensions in DDR2 only]--------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRC; /* Byte 41 */

	if (offset_ptr->raw_data == 0x00){
		printf_err("ERROR - Illegal all-zero value for SPD byte 41\n");
		dram_spd_exit();
	}
	else if (offset_ptr->raw_data == 0xFF){
		printf_err("ERROR - Illegal all-one value for SPD byte 41\n");
		dram_spd_exit();
	}
	else {
		offset_ptr->whole = offset_ptr->raw_data; /* whole = byte 41 */
	}

	if (*mem_type_ptr == DDR2){

		offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRFC_tRC_EXTENSION; /* Read byte 40 */
		temp = (offset_ptr->raw_data & 0x70) >> 4;

		offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRC; /* Byte 41 */

		if (temp == 0){
			offset_ptr->num   = 0;
			offset_ptr->denom = 10;  
		}
		else if (temp == 1){
			offset_ptr->num   = 1;
			offset_ptr->denom = 4;  
		}
		else if (temp == 2){
			offset_ptr->num   = 1;
			offset_ptr->denom = 3;  
		}
		else if (temp == 3){
			offset_ptr->num   = 1;
			offset_ptr->denom = 2;  
		}
		else if (temp == 4){
			offset_ptr->num   = 2;
			offset_ptr->denom = 3;  
		}
		else if (temp == 5){
			offset_ptr->num   = 3;
			offset_ptr->denom = 4;  
		}
		else {
			printf_err("ERROR - Illegal value for DDR2 SPD byte 40 extension for tRC(byte 41): 0x%x\n", temp);
			dram_spd_exit();
		}
	}

	/* ---------  Byte 42 & 40: tRFC (integer) [with extensions in DDR2 only]--------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRFC; /* Byte 42 */

	if (offset_ptr->raw_data == 0x00){
		printf_err("ERROR - Illegal all-zero value for SPD byte 42\n");
		dram_spd_exit();
	}
	else {
		offset_ptr->whole = offset_ptr->raw_data; /* whole = byte 42 */
	}

	if (*mem_type_ptr == DDR2){

		offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRFC_tRC_EXTENSION; /* Point to byte 40 */
		temp = offset_ptr->raw_data & 0x01;                         /* Extract bit [0] */

		if (temp == 1){
			offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRFC;               /* Go to byte 42 */
			(offset_ptr->whole) += 256;                                 /* whole = whole + 256 */
			offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRFC_tRC_EXTENSION; /* Go back to byte 40 */
		}

		temp = (offset_ptr->raw_data & 0x0E) >> 1;                    /* Extract byte 40 bits [3:1] */
		offset_ptr = data_ptr + ARIZONA_SPD_DDR_tRFC;                 /* Go to byte 42 */

		if (temp == 0){
			offset_ptr->num   = 0;
			offset_ptr->denom = 10;  
		}
		else if (temp == 1){
			offset_ptr->num   = 1;
			offset_ptr->denom = 4;  
		}
		else if (temp == 2){
			offset_ptr->num   = 1;
			offset_ptr->denom = 3;  
		}
		else if (temp == 3){
			offset_ptr->num   = 1;
			offset_ptr->denom = 2;  
		}
		else if (temp == 4){
			offset_ptr->num   = 2;
			offset_ptr->denom = 3;  
		}
		else if (temp == 5){
			offset_ptr->num   = 3;
			offset_ptr->denom = 4;  
		}
		else {
			printf_err("ERROR - Illegal value for DDR2 SPD byte 40 extension for tRFC(byte 42): 0x%x\n", temp);
			dram_spd_exit();
		}

	}

	/* ---------  Byte 43: tCKmax (whole & quarters)[DDR1], (whole & tenths w/fractions)[DDR2]--------*/
	offset_ptr = data_ptr + ARIZONA_SPD_DDR_tCK_MAX;

	if (*mem_type_ptr == DDR1){

		if (offset_ptr->raw_data == 0xFF){ /* Special encoding that indicates no minimum value */
			offset_ptr->whole = 32000;                      /* A large value  */
		}
		else {
			temp = (offset_ptr->raw_data & 0xFC) >> 2;

			if (temp == 0){
				printf_err("ERROR - Illegal all-zero value for DDR1 SPD byte 43 bits[7:2] %x\n", temp);
				dram_spd_exit();
			}

			offset_ptr->whole = temp;                        /* whole = byte43 [7:2] */
			offset_ptr->num   = offset_ptr->raw_data & 0x03; /* num   = byte43 [1:0] */
			offset_ptr->denom = 4;                           /* denom = 4 */
		}

	}
	else { /* DDR2 */
		if (offset_ptr->raw_data  == 15) {
			printf_err("ERROR - Illegal all-one value for DDR2 SPD byte 43\n");
			dram_spd_exit();
		}

		temp = (offset_ptr->raw_data & 0xF0) >> 4;

		if (temp == 0) {
			printf_err("ERROR - Illegal zero value for DDR2 SPD byte 43 bits [7:4]\n");
			dram_spd_exit();
		}
		else {
			offset_ptr->whole = temp; /* whole = byte43[7:4] */
		}

		temp = offset_ptr->raw_data & 0x0F;

		if ( (temp >= 0) && (temp <= 9) ) {
			offset_ptr->num   = temp; /* num = byte43[3:0] */
			offset_ptr->denom = 10;   /* denom = 10 */
		}
		else if (temp == 0xA){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else if (temp == 0xB){
			offset_ptr->num   = 1; /* num   = 1 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xC){
			offset_ptr->num   = 2; /* num   = 2 */
			offset_ptr->denom = 3; /* denom = 3 */
		}
		else if (temp == 0xD){
			offset_ptr->num   = 3; /* num   = 3 */
			offset_ptr->denom = 4; /* denom = 4 */
		}
		else {
			printf_err("ERROR - Illegal value for DDR2 SPD byte43 bits[3:0] %x\n", temp);
			dram_spd_exit();
		}

	}

} /* END decode_spd_data */


void convert_spd_mixed_to_fraction (spd_entry *spd_entry_ptr){

	spd_entry_ptr->num = (spd_entry_ptr->whole * spd_entry_ptr->denom) + spd_entry_ptr->num;
	spd_entry_ptr->whole = 0;

}

void convert_spd_array_to_fraction (spd_entry *spd_array_ptr){

	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_MAX_CYCLE_TIME);              /* Convert byte 9 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_DIMM_REFRESH_RATE);           /* Convert byte 12 from fraction to cycles */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_HALF); /* Convert byte23 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_ONE);  /* Convert byte25 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tRP);                         /* Convert byte27 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tRRD);                        /* Convert byte28 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tRCD);                        /* Convert byte29 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tRAS);                        /* Convert byte30 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tWR);                         /* Convert byte36 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tWTR);                        /* Convert byte37 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tRTP);                        /* Convert byte38 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tRC);                         /* Convert byte41 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tRFC);                        /* Convert byte42 from mixed number to fraction */
	convert_spd_mixed_to_fraction(spd_array_ptr + ARIZONA_SPD_DDR_tCK_MAX);                     /* Convert byte43 from mixed number to fraction */

}

void convert_spd_fraction_to_cycles (spd_entry *spd_entry_ptr, int clk_period_denom){

	int remainder;

	spd_entry_ptr->whole = (spd_entry_ptr->num * clk_period_denom) / (spd_entry_ptr->denom * PHOENIX_DRAM_CLK_PERIOD_NUMERATOR);
	remainder =            (spd_entry_ptr->num * clk_period_denom) % (spd_entry_ptr->denom * PHOENIX_DRAM_CLK_PERIOD_NUMERATOR);

	if (remainder){
		(spd_entry_ptr->whole)++;
	} 
	spd_entry_ptr->num   = 0;
	spd_entry_ptr->denom = 1;
}

void convert_spd_array_to_cycles (spd_entry *spd_array_ptr, int clk_period_denom){

	/* WARNING: ALL bytes converted to cycles in this function must first have been converted to fractions using 
	   the convert_spd_array_to_fraction function */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_DIMM_REFRESH_RATE, clk_period_denom); /* Convert byte 12 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tRP,               clk_period_denom); /* Convert byte 27 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tRRD,              clk_period_denom); /* Convert byte 28 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tRCD,              clk_period_denom); /* Convert byte 29 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tRAS,              clk_period_denom); /* Convert byte 30 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tWR,               clk_period_denom); /* Convert byte 36 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tWTR,              clk_period_denom); /* Convert byte 37 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tRTP,              clk_period_denom); /* Convert byte 38 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tRC,               clk_period_denom); /* Convert byte 41 from fraction to cycles */
	convert_spd_fraction_to_cycles(spd_array_ptr + ARIZONA_SPD_DDR_tRFC,              clk_period_denom); /* Convert byte 42 from fraction to cycles */
}

#ifdef SPD_DEBUG
void print_spd (spd_entry *spd_ptr){

	int i;
  
	for (i=0; i<SPD_DATA_NUM_BYTES; i++, spd_ptr++){   
		printf("SPD entry %3d: %4x %4d %4d %4d\n", i, spd_ptr->raw_data, spd_ptr->whole, spd_ptr->num, spd_ptr->denom);  
	}
}
#endif


void program_clock (spd_entry         *dimm_data_ptr,  
		    phoenix_reg_t     *clkbase,
		    unsigned long     clk_cntrl_reg_addr, 
		    unsigned long     clk_ratio_reg_addr, 
		    unsigned long     clk_reset_reg_addr,
		    unsigned long     clk_status_reg_addr,
		    int               cas_value_vect[],
		    int               cas_support_vect[],
		    int               *chosen_cas,
		    int               *chosen_period_denom,
		    int               ch_width, 
		    int               ch_id,
		    int               treg,
		    char              chip_rev[]){

        int                        temp;
	volatile unsigned long int data;
	int                        freq_found;
	int                        skip_clock_programming = 0; /* By default, we want to program the clock */

	spd_entry                  *dimm_access_ptr;
	int                        result_vect[3];

	freq_found  = 0; 

	
    /* Set the default for the DDR clock frequency upper limit*/
    if ((*chosen_period_denom == 0) && (treg == 1)) {
           /* Added logic to change DRAM frequency on platform basis */
           switch(assm_id){
            case QFX5500_BOARD_ID:
                /* Changed from 400 to 333 */
                *chosen_period_denom = PHOENIX_DRAM_CLK_PERIOD_DENOM_333;
                break;

            CASE_SE_QFX3600_SERIES:
            CASE_UFABRIC_QFX3600_SERIES:
                /* changed from 400 to 266 */
                *chosen_period_denom = PHOENIX_DRAM_CLK_PERIOD_DENOM_266;
                break;

            default: 
               *chosen_period_denom = PHOENIX_DRAM_CLK_PERIOD_DENOM_400;
          }
    }
    else if ((*chosen_period_denom == 0) && (treg == 0)){
	  *chosen_period_denom = PHOENIX_DRAM_CLK_PERIOD_DENOM_266;
	}

	while ( (freq_found == 0) && (*chosen_period_denom > 2) ) {

		result_vect[0] = 0;
		result_vect[1] = 0;
		result_vect[2] = 0;

		/* Start with lowest CAS latency */
		dimm_access_ptr = dimm_data_ptr + ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_ONE; 
		temp = (PHOENIX_DRAM_CLK_PERIOD_NUMERATOR * dimm_access_ptr->denom) - (*chosen_period_denom * dimm_access_ptr->num);
		if (temp >= 0) {
			result_vect[2] = 1;
		}
  
		/* Increase CAS */
		dimm_access_ptr = dimm_data_ptr + ARIZONA_SPD_DDR_DERATE_FREQ_CL_DERATED_HALF;
		temp = (PHOENIX_DRAM_CLK_PERIOD_NUMERATOR * dimm_access_ptr->denom) - (*chosen_period_denom * dimm_access_ptr->num);
		if (temp >= 0) {
			result_vect[1] = 1;
		}

		/* Increase CAS again */
		dimm_access_ptr = dimm_data_ptr + ARIZONA_SPD_DDR_MAX_CYCLE_TIME;
		temp = (PHOENIX_DRAM_CLK_PERIOD_NUMERATOR * dimm_access_ptr->denom) - (*chosen_period_denom * dimm_access_ptr->num);
		if (temp >= 0) {
			result_vect[0] = 1;
		}

		/* Decide what to do */
		if (result_vect[2] & cas_support_vect[2]){ /* Lowest CAS gets preference */
			*chosen_cas = cas_value_vect[2];
			freq_found = 1;
		}
		else if (result_vect[1] & cas_support_vect[1]){
			*chosen_cas = cas_value_vect[1];
			freq_found = 1;
		}
		else if (result_vect[0] & cas_support_vect[0]){
			*chosen_cas = cas_value_vect[0];
			freq_found = 1;
		}
		else {
			(*chosen_period_denom)--; /* If nothing found, try again with lower frequency, i.e. increase period by lowering denominator */
		}
	}


	if (freq_found == 0){
		printf_err("[Error]: Unable to find suitable clock frequency\n");
		dram_spd_exit();
	}

	dimm_access_ptr = dimm_data_ptr + ARIZONA_SPD_DDR_tCK_MAX;
	temp = (PHOENIX_DRAM_CLK_PERIOD_NUMERATOR * dimm_access_ptr->denom) - (*chosen_period_denom * dimm_access_ptr->num);
	if (temp >= 0) {
		printf_err("[Error]: Chosen clock period %d/%d ns greater than DIMM's tCKmax %d/%d ns\n", PHOENIX_DRAM_CLK_PERIOD_NUMERATOR, *chosen_period_denom, dimm_access_ptr->num, dimm_access_ptr->denom );
		dram_spd_exit();
	}

	if ((chip_rev[2]== 'R') && (ch_width == 36) && ((ch_id == 1) || (ch_id == 3))){

	  /* Check the clock frequency programmed for the 36-bit DRAM channel */
	  /* which shares this 36-bit DRAM channel's PLL. They should match.  */ 

	  data = phoenix_read_reg(clkbase, clk_ratio_reg_addr);

	  if (*chosen_period_denom == 3) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_100);
	  }
	  else if (*chosen_period_denom == 4) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_133);
	  }
	  else if (*chosen_period_denom == 5) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_166);
	  }
	  else if (*chosen_period_denom == 6) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_200);
	  }
	  else if (*chosen_period_denom == 7) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_233);
	  }
	  else if (*chosen_period_denom == 8) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_266);
	  }
	  else if (*chosen_period_denom == 9) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_300);
	  }
	  else if (*chosen_period_denom == 10) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_333);
	  }
	  else if (*chosen_period_denom == 11) { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_366);
	  }
	  else { 
	    skip_clock_programming = (data == PHOENIX_DRAM_PLL_RATIO_400);
	  }

	  if (skip_clock_programming == 0){
	    printf_err("[Warning]:Clock frequency mismatch for 36-bit DRAM channels %d and %d, which share a PLL.\n", ch_id-1, ch_id);
	  }
	  else {
	    SPD_PRINTF("Skipping clock programming - 36-bit DRAM channels %d and %d share a PLL.\n", ch_id-1, ch_id);
	  }
	}
	else if ((chip_rev[2]== 'S') && (ch_id != 0)){

	  /* Check the clock frequency already programmed for the single DRAM PLL. */
	  /* It should match. */
	  data = phoenix_read_reg(clkbase, clk_ratio_reg_addr);

	  if (*chosen_period_denom == 3) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_100);
	  }
	  else if (*chosen_period_denom == 4) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_133);
	  }
	  else if (*chosen_period_denom == 5) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_166);
	  }
	  else if (*chosen_period_denom == 6) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_200);
	  }
	  else if (*chosen_period_denom == 7) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_233);
	  }
	  else if (*chosen_period_denom == 8) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_266);
	  }
	  else if (*chosen_period_denom == 9) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_300);
	  }
	  else if (*chosen_period_denom == 10) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_333);
	  }
	  else if (*chosen_period_denom == 11) { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_366);
	  }
	  else { 
	    skip_clock_programming = (data == XLS_DRAM_PLL_RATIO_400);
	  }

	  if (skip_clock_programming == 0){
	    printf_err("[Warning]:Clock frequency mismatch between DRAM channel 0 and %d which share a PLL.\n", ch_id);
	  }
	  else {
	    printf("Skipping clock programming - DRAM channels 0 and %d share a PLL.\n", ch_id);
	  }

	}

	if (skip_clock_programming == 0){
	  /* Begin the actual clock frequency programming */
	  
	  /* Step 1: Disable the clock */
	  
	  phoenix_verbose_write_reg(clkbase, clk_cntrl_reg_addr, 0x0);
	  
	  /* Step 2: Reset the controller */
	  phoenix_verbose_write_reg(clkbase, clk_reset_reg_addr, 0x0);
	  
	  
	  /* Step 3: Program the frequency */	
	  if (chip_rev[2]== 'R'){

	    if (*chosen_period_denom == 3) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_100);
	    }
	    else if (*chosen_period_denom == 4) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_133);
	    }
	    else if (*chosen_period_denom == 5) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_166);
	    }
	    else if (*chosen_period_denom == 6) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_200);
	    }
	    else if (*chosen_period_denom == 7) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_233);
	    }
	    else if (*chosen_period_denom == 8) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_266);
	    }
	    else if (*chosen_period_denom == 9) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_300);
	    }
	    else if (*chosen_period_denom == 10) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_333);
	    }
	    else if (*chosen_period_denom == 11) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_366);
	    }
	    else { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, PHOENIX_DRAM_PLL_RATIO_400);
	    }
	  }
	  else { /* XLS */
	    if (*chosen_period_denom == 3) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_100);
	    }
	    else if (*chosen_period_denom == 4) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_133);
	    }
	    else if (*chosen_period_denom == 5) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_166);
	    }
	    else if (*chosen_period_denom == 6) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_200);
	    }
	    else if (*chosen_period_denom == 7) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_233);
	    }
	    else if (*chosen_period_denom == 8) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_266);
	    }
	    else if (*chosen_period_denom == 9) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_300);
	    }
	    else if (*chosen_period_denom == 10) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_333);
	    }
	    else if (*chosen_period_denom == 11) { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_366);
	    }
	    else { 
	      phoenix_verbose_write_reg(clkbase, clk_ratio_reg_addr, XLS_DRAM_PLL_RATIO_400);
	    }
	  }
	  /* Step 4: Pull the controller out of reset */
	  phoenix_verbose_write_reg(clkbase, clk_reset_reg_addr, 0x1);
	  
	  /* Step 5: Poll for PLL lock */
	  while (phoenix_read_reg(clkbase, clk_status_reg_addr) == 0){
	  }
	  
	  /*Step 6: Enable the clock */
	  phoenix_verbose_write_reg(clkbase, clk_cntrl_reg_addr, 0x1);  
	}
	
	temp = (*chosen_period_denom * 1000)/15;
	SPD_PRINTF("Memory Channel Operating at [%d] MHz\n", temp);
  

} /* END program_clock */

void phoenix_verbose_write_reg (phoenix_reg_t *base, int offset, uint32_t data){

//  printf("Programming register at address 0x%p with data 0x%x\n", base+(offset<<2), data);
	phoenix_write_reg(base, offset, data);  
}

int compute_AL(spd_entry *spd_data_reference, enum mem_protocol *dram_type_ptr, int desired_AL, int cas_lat, int board_del, int treg){

	unsigned int data;
	unsigned int read_lat;
 
	/* Pick a value for additive latency */
	if (*dram_type_ptr == DDR1){ /* AL = 0 for DDR1 */
		data = 0;
	}
	else if (desired_AL != NO_OVERRIDE_ADD_LAT_MAGIC_NUM){ /* DDR2, override AL provided */
		data = desired_AL;
	}
	else { /* DDR2, override AL not provided */
		data = ( ( (spd_data_reference + ARIZONA_SPD_DDR_tRCD)->whole) - 1);
	}

	/* Calculate the read latency for the chosen additive latency. */
	read_lat = data + cas_lat + treg + board_del;

	/* Check if chosen additive latency value is legal, If not, force it to 0 */
	if ( (data >= 0) && (data < 5) ){ 
		return(data);
	}
	else if (data > 4){
    /*
     * FX Products use a memory with a latency of 5. RMI has
     * confirmed that the following warning does not indicate
     * any serious drawback but its just a normal warning.
     * Disabling it for FX to avoid customer panic.
     */
#ifndef CONFIG_FX
		printf_err("Warning: Illegal high additive latency calculated or provided:%d, using 4 instead\n", data);
#endif
		return(4);
	}
	else {
		printf_err("[Error]: Illegal low additive latency calculated or provided:%d, using zero instead\n", data);
		return(0);
	}
}

int calc_mem_size(spd_entry *spd_data_reference, int dimm_presence_vect[] ){
	int i;

	i = ((spd_data_reference + ARIZONA_SPD_DDR_NUM_PHYS_BANKS)->whole) * /* Number of ranks */
		((spd_data_reference + ARIZONA_SPD_DDR_DENSITY)       ->whole) * /* Size of rank */
		(dimm_presence_vect[0] + dimm_presence_vect[1]);                 /* Number of DIMMs */

	return(i);
}


void program_bridge_cfg(phoenix_reg_t *bridgebase, int ch_width, char chip_rev[]){

  if((chip_rev[2] == 'S') && (chip_rev[1] == '2') && (ch_width == 72)){ /* XLS2xx in 1x72 mode */
    phoenix_verbose_write_reg(bridgebase, BRIDGE_CFG_BAR, MEM_CTL_XLS2XX_DDR72_INITIAL_BRIDGE_CFG);
  }
  else if((chip_rev[2] == 'S') && (chip_rev[1] == '2') && (ch_width == 36)){ /* XLS2xx in 1x36 mode */
    phoenix_verbose_write_reg(bridgebase, BRIDGE_CFG_BAR, MEM_CTL_XLS2XX_DDR36_INITIAL_BRIDGE_CFG);
  }
  else if (ch_width == 72){ /* XLR7xx, XLR5xx, XLR3xx, XLS6xx, or XLS4xx in 72-bit mode */
    phoenix_verbose_write_reg(bridgebase, BRIDGE_CFG_BAR, MEM_CTL_DDR72_INITIAL_BRIDGE_CFG);
  }
  else if (ch_width == 36){ /* XLR7xx, XLR5xx, XLR3xx, XLS6xx, or XLS4xx in 36-bit mode */
    phoenix_verbose_write_reg(bridgebase, BRIDGE_CFG_BAR, MEM_CTL_DDR36_INITIAL_BRIDGE_CFG);
  }
}


void program_dyn_data_pad_ctrl(phoenix_reg_t *dramchbase, int ch_width, int ch_id, char chip_rev[]){

    if((chip_rev[2] == 'S') && (chip_rev[1] == '2') && (ch_id == 0)){
        if (ch_width == 72){
            phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_DYN_DATA_PAD_CTRL,0x70417051);
        }
        else { /* Raven 1x36 mode */
            phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_DYN_DATA_PAD_CTRL,0x90419051);
        }
    }
}


void program_glb_params(phoenix_reg_t *dramchbase, spd_entry *spd_data_reference, int ch_width, int rld2_mode, enum mem_protocol mem_type, char chip_rev[]){
	unsigned int data;
	unsigned int byp_dis;

	byp_dis = 0;

	if ((chip_rev[2]== 'R') && (chip_rev[0] == 'B')){
	  data = ((ch_width == 72)                  <<10) | /* SLV_CLK_GATE_EN should be 0 if 36-bit mode */
	         (byp_dis                           << 9) |
	         (MEM_CTL_GLB_PARAMS_ODT2_MUX_EN_L  << 8) |
		 (rld2_mode                         << 7) |
		 (MEM_CTL_GLB_PARAMS_CPO            << 6) |
		 (0                                 << 5) | /* ECC is activated at a later stage */
		 (MEM_CTL_GLB_PARAMS_OPP            << 4) |
		 (MEM_CTL_GLB_PARAMS_CLK_SCM        << 3) |
		 ((mem_type == DDR1)                << 2) |
		 (MEM_CTL_GLB_PARAMS_ORWR           << 1) |
		 ((ch_width == 72)                  << 0);
	}
	else{ /* XLR version C, or XLS version A */
	  data = (0                                 <<11) | /* ECC is activated at a later stage */
                 (0                                 <<10) | /* Unused */
                 (byp_dis                           << 9) |
                 (MEM_CTL_GLB_PARAMS_ODT2_MUX_EN_L  << 8) |
                 (rld2_mode                         << 7) |
                 (MEM_CTL_GLB_PARAMS_CPO            << 6) |
                 (0                                 << 5) | /* ECC is activated at a later stage */
                 (MEM_CTL_GLB_PARAMS_OPP            << 4) |
                 (0                                 << 3) | /* Unused */
                 ((mem_type == DDR1)                << 2) |
                 (MEM_CTL_GLB_PARAMS_ORWR           << 1) |
                 ((ch_width == 72)                  << 0);

	}
	phoenix_verbose_write_reg(dramchbase, DDR2_CONFIG_REG_GLB_PARAMS, data);  

}

void init_RAT (unsigned short dimm0_spd_addr, unsigned short dimm1_spd_addr, spd_entry *spd_data_ptr, int dimm_presence_vect[], rat_entry *rat_ptr, int ch_width, int ch_id){

	int i;
	int r;

	int num_ranks = (spd_data_ptr+ARIZONA_SPD_DDR_NUM_PHYS_BANKS)->whole;

	/* Write to the SPD address fields */
	rat_ptr->SPD_addr     = dimm0_spd_addr;
	(rat_ptr+1)->SPD_addr = dimm0_spd_addr;

	if (num_ranks == 4){
	  (rat_ptr+2)->SPD_addr = dimm0_spd_addr;
	  (rat_ptr+3)->SPD_addr = dimm0_spd_addr;
	}
	else {
	  (rat_ptr+2)->SPD_addr = dimm1_spd_addr;
	  (rat_ptr+3)->SPD_addr = dimm1_spd_addr;
	}

	/* Write to the exists fields */
	rat_ptr->exists = 0;
	(rat_ptr+1)->exists = 0;
	(rat_ptr+2)->exists = 0;
	(rat_ptr+3)->exists = 0;

	if ((dimm_presence_vect[0] == 1) && (num_ranks == 1)){      /* First DIMM is single-rank */
	  rat_ptr->exists = 1;
	}
	else if ((dimm_presence_vect[0] == 1) && (num_ranks == 2)){ /* First DIMM is dual-rank */
	  rat_ptr->exists = 1;
	  (rat_ptr+1)->exists = 1;
	}
	else if ((dimm_presence_vect[0] == 1) && (num_ranks == 4)){ /* First DIMM is quad-rank */
	  rat_ptr->exists = 1;
	  (rat_ptr+1)->exists = 1;
	  (rat_ptr+2)->exists = 1;
	  (rat_ptr+3)->exists = 1;
	}

	if ((dimm_presence_vect[1] == 1) && (num_ranks == 1)){      /* Second DIMM is single-rank */
	  (rat_ptr+2)->exists = 1;
	}
	else if ((dimm_presence_vect[1] == 1) && (num_ranks == 2)){ /* Second DIMM is dual-rank */
	  (rat_ptr+2)->exists = 1;
	  (rat_ptr+3)->exists = 1;
	}

	/* Write to the CS_id field */
	if (ch_width == 72){ 
		rat_ptr->CS_id         = MEM_CTL_72_DEFAULT_SPD0_RANK0_CS_PIN;
		(rat_ptr+1)->CS_id     = MEM_CTL_72_DEFAULT_SPD0_RANK1_CS_PIN;
		(rat_ptr+2)->CS_id     = MEM_CTL_72_DEFAULT_SPD1_RANK0_CS_PIN;
		(rat_ptr+3)->CS_id     = MEM_CTL_72_DEFAULT_SPD1_RANK1_CS_PIN;
	}
	else{ /* 36-bit channels, as implemented in RMI ATX I, II */
		if ((ch_id == 0) || (ch_id == 2)){
			rat_ptr->CS_id         = MEM_CTL_36AC_DEFAULT_SPD0_RANK0_CS_PIN;
			(rat_ptr+1)->CS_id     = MEM_CTL_36AC_DEFAULT_SPD0_RANK1_CS_PIN;
			(rat_ptr+2)->CS_id     = 2; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
			(rat_ptr+3)->CS_id     = 3; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
		}
		else{ /* Channels B or D */
			rat_ptr->CS_id         = MEM_CTL_36BD_DEFAULT_SPD0_RANK0_CS_PIN;
			(rat_ptr+1)->CS_id     = MEM_CTL_36BD_DEFAULT_SPD0_RANK1_CS_PIN;
			(rat_ptr+2)->CS_id     = 0; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
			(rat_ptr+3)->CS_id     = 1; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
		}
	}

	/* Write to the ODT_id field */
	if (ch_width == 72){
		if ((dimm_presence_vect[0] == 1) && (num_ranks == 4)){ 
			/* Quad-rank DIMMs have only two ODT pins, not four. ODT pins exist        */
			/* for ranks 0 and 2. ODT pins for ranks 1 and 3 are grounded on the DIMM. */
			/* Normally, for a single DIMM on the channel, we would switch on ODT for  */
			/* the same rank being accessed. However, in this case, when writing to    */
			/* ranks 1 or 3, we will instead switch on ODT for ranks 0 and 2.          */
			/* Note that the assumption here is that the board is designed such that   */
			/* DDR[A/C]_ODT0 is connected to the quad-rank DIMM's rank0 ODT pin, and   */
			/* DDR[A/C]_ODT1 is connected to the quad-rank DIMM's rank2 ODT pin.       */
			rat_ptr->ODT_id         = MEM_CTL_72_DEFAULT_SPD0_RANK0_ODT_PIN;
			(rat_ptr+1)->ODT_id     = MEM_CTL_72_DEFAULT_SPD0_RANK0_ODT_PIN;
			(rat_ptr+2)->ODT_id     = MEM_CTL_72_DEFAULT_SPD0_RANK1_ODT_PIN;
			(rat_ptr+3)->ODT_id     = MEM_CTL_72_DEFAULT_SPD0_RANK1_ODT_PIN;
		}
		else{	/* Dual- or Single- rank DIMM(s) */
			rat_ptr->ODT_id         = MEM_CTL_72_DEFAULT_SPD0_RANK0_ODT_PIN;
			(rat_ptr+1)->ODT_id     = MEM_CTL_72_DEFAULT_SPD0_RANK1_ODT_PIN;
			(rat_ptr+2)->ODT_id     = MEM_CTL_72_DEFAULT_SPD1_RANK0_ODT_PIN;
			(rat_ptr+3)->ODT_id     = MEM_CTL_72_DEFAULT_SPD1_RANK1_ODT_PIN;
		}
	}
	else{ /* 36-bit channels, as implemented in RMI ATX I, II */
		if ((ch_id == 0) || (ch_id == 2)){
			rat_ptr->ODT_id         = MEM_CTL_36AC_DEFAULT_SPD0_RANK0_ODT_PIN;
			(rat_ptr+1)->ODT_id     = MEM_CTL_36AC_DEFAULT_SPD0_RANK1_ODT_PIN;
			(rat_ptr+2)->ODT_id     = MEM_CTL_36AC_DEFAULT_SPD1_RANK0_ODT_PIN; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
			(rat_ptr+3)->ODT_id     = MEM_CTL_36AC_DEFAULT_SPD1_RANK1_ODT_PIN; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
		}
		else{ /* Channels B or D */
			rat_ptr->ODT_id         = MEM_CTL_36BD_DEFAULT_SPD0_RANK0_ODT_PIN;
			(rat_ptr+1)->ODT_id     = MEM_CTL_36BD_DEFAULT_SPD0_RANK1_ODT_PIN;
			(rat_ptr+2)->ODT_id     = MEM_CTL_36BD_DEFAULT_SPD1_RANK0_ODT_PIN; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
			(rat_ptr+3)->ODT_id     = MEM_CTL_36BD_DEFAULT_SPD1_RANK1_ODT_PIN; /* On RMI ATX I, II, only 1 DIMM/channel in 36-bit mode */
		}
	}

	/* When it comes time to program the ADDR_PARAMS_2 register, this config software will have to set up logical rank<->CS pin mappings.
	   The actual mapping does not really matter, but it is important that logical ranks do not get allocated to ranks that do
	   not exist (e.g. single-sided DIMMs, missing DIMMs). This software can only identify DIMMs by their SPD EEPROM address, 
	   and presence/absence of ranks and DIMMs is detected by an SPD read. Therefore, this software needs some way to correlate 
	   {SPD address, presence} pairs with the XLR's CS pins. That is the purpose of the CS_id field, which is essentially statically
	   programmed by the user for each board. */ 

	/* Logical Rank allocation */
	rat_ptr->logical_rank_id = -1;
	(rat_ptr+1)->logical_rank_id = -1;
	(rat_ptr+2)->logical_rank_id = -1;
	(rat_ptr+3)->logical_rank_id = -1;

	r = 0;
	for (i=0; i<4; i++){
		if ((rat_ptr+i)->exists) {
			(rat_ptr+i)->logical_rank_id = r;
			r++;
		}
	}

	/* Print out RAT */
	SPD_PRINTF("Rank Allocation Table: \n");
	for (i=0; i<4; i++){
		if((rat_ptr+i)->exists == 1){
			SPD_PRINTF("SPD address:0x%lx Exists:%d CSid:%d ODTid:%d Logical rank ID:%d \n", (rat_ptr+i)->SPD_addr, (rat_ptr+i)->exists, (rat_ptr+i)->CS_id, (rat_ptr+i)->ODT_id, (rat_ptr+i)->logical_rank_id);
		}
	}
}

void program_write_ODT_mask(phoenix_reg_t *dramchbase, rat_entry rank_table[], int dimm_presence_vect[], enum mem_protocol mem_type ){

	int r;
	int i;
	int rat_index=0;
	int term_index;
	int odt_pin_number;
	int data = 0;
	int rank_odt_bits;

	if (mem_type != DDR1){
		for (r=0; r<4; r++){  /* For each rank */

			for (i=0; i<4; i++){ /* For each RAT entry */
				if ( (rank_table[i].exists) && (rank_table[i].logical_rank_id == r) ){
					rat_index = i;
				}
			}
    
			if ( (dimm_presence_vect[0]) && (dimm_presence_vect[1]) ){ /* 2 DIMMs in the channel */

				/* ODT source is from other DIMM: 0->2, 1->3, 2->0, 3->1*/
				/* Note that if we ever decide to support 2 mismatched DIMMs on a channel (e.g 1 2-rank and another 1-rank), this 
				   equation may fail. e.g. just if rank1 exists but rank3 does not*/ 
				term_index = (rat_index + 2) % 4; 
			}
			else {/* 1 DIMMs in the channel */
				term_index = rat_index; /* ODT source is from same DIMM */
			}

			/* Retrieve the ODT pin number */
			odt_pin_number = rank_table[term_index].ODT_id; 

			/* Decode ODT pin number */
			rank_odt_bits = 0x01 << odt_pin_number;
    
			data |= rank_odt_bits << (5 * r);
		}
	}
	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_WRITE_ODT_MASK, data);

}

void program_read_ODT_mask(phoenix_reg_t *dramchbase, rat_entry rank_table[], int dimm_presence_vect[], enum mem_protocol mem_type ){

	int r;
	int i;
	int rat_index= 0;
	int term_index;
	int odt_pin_number;
	int data = 0;
	int rank_odt_bits;


	if (mem_type != DDR1 ){
		for (r=0; r<4; r++){  /* For each rank */

			for (i=0; i<4; i++){ /* For each RAT entry */
				if ( (rank_table[i].exists) && (rank_table[i].logical_rank_id == r) ){
					rat_index = i;
				}
			}
    
			if ( (dimm_presence_vect[0]) && (dimm_presence_vect[1]) ){ /* 2 DIMMs in the channel */

				/* ODT source is from other DIMM: 0->2, 1->3, 2->0, 3->1*/
				/* Note that if we ever decide to support 2 mismatched DIMMs on a channel (e.g 1 2-rank and another 1-rank), this 
				   equation may fail. e.g. just if rank1 exists but rank3 does not*/ 
				term_index = (rat_index + 2) % 4; 

				/* Retrieve the ODT pin number */
				odt_pin_number = rank_table[term_index].ODT_id; 

				/* Decode ODT pin number */
				rank_odt_bits = 0x01 << odt_pin_number;

			}
			else {/* 1 DIMMs in the channel */
				rank_odt_bits = 0x0; /* no ODT required */
			}

			data |= rank_odt_bits << (5 * r);
		}
	}

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_READ_ODT_MASK, data);
}

void program_addr_params(phoenix_reg_t *dramchbase, 
                         rat_entry     *rank_allocation_table, 
                         spd_entry     *spd_data_reference, 
                         int           dimm_presence_vect[], 
                         int           ch_width, 
                         int           *loc_bank_ptr, 
                         int           *loc_col_ptr, 
                         int           *loc_row_ptr, 
                         int           *loc_rank_ptr,  
			 int           *num_banks_ptr,
			 int           *num_ranks_ptr,
                         int           *num_useful_col_bits_ptr,
			 int           *num_useful_cols_ptr,
			 int           *num_row_bits_ptr){
	int         cs0_rankid=0;
	int         cs1_rankid=0;
	int         cs2_rankid=0;
	int         cs3_rankid=0;

	int         num_dimms;
	int         num_bank_bits=0;
	int         num_rank_bits=0;
	int         num_row_bits_minus1;
	int         data;
	int         bank_addr_vect;
	int         rank_addr_vect;
	int         bank_rank_vect;
	int         row_vect;
	int         col_vect;
	unsigned    i;
	int         one_count;

	int         use_rank_loc;
	int         calc_bank_loc;
	int         calc_rank_loc;
	int         calc_col_addr_loc;
	int         calc_row_addr_loc; 

	/* Number of bank bits */
	if ( (spd_data_reference+ARIZONA_SPD_DDR_NUM_BANKS)->whole == 4){
		*num_banks_ptr = 4;
		num_bank_bits = 2;
	}
	else if ( (spd_data_reference+ARIZONA_SPD_DDR_NUM_BANKS)->whole == 8){
		*num_banks_ptr = 8;
		num_bank_bits = 3;
	}
	else {
		printf_err("[Error]: Unsupported number of banks: %d\n", (spd_data_reference+ARIZONA_SPD_DDR_NUM_BANKS)->whole );
	}

	/* Number of rank bits */
	if (dimm_presence_vect[0] && dimm_presence_vect[1]){
		num_dimms = 2;
	}
	else{
		num_dimms = 1;
	}

	*num_ranks_ptr = num_dimms * ( (spd_data_reference+ARIZONA_SPD_DDR_NUM_PHYS_BANKS)->whole );

	if (*num_ranks_ptr == 1) {
		num_rank_bits = 0;
	}
	else if (*num_ranks_ptr == 2) {
		num_rank_bits = 1;
	}
	else if (*num_ranks_ptr == 4) {
		num_rank_bits = 2;
	}
	else{
		printf_err("[Error]: Unsupported number of ranks: %d\n", *num_ranks_ptr );
		dram_spd_exit();
	}

	/* Number of column bits */
	if (ch_width == 36){
		*num_useful_col_bits_ptr = ( (spd_data_reference+ARIZONA_SPD_DDR_NUM_COLS)->whole - 3 );
	}
	else { /* 72 bits */
		*num_useful_col_bits_ptr = ( (spd_data_reference+ARIZONA_SPD_DDR_NUM_COLS)->whole - 2 );
	}

	*num_useful_cols_ptr = (0x1 << *num_useful_col_bits_ptr);

	/* Number of row bits */
	*num_row_bits_ptr = (spd_data_reference+ARIZONA_SPD_DDR_NUM_ROWS)->whole;
	num_row_bits_minus1 = *num_row_bits_ptr - 1;

	/* Compute default field locations, which assumes the format:<row><col><rank,bank> */
	calc_bank_loc = 0;
	calc_rank_loc = num_bank_bits;
	calc_col_addr_loc = num_rank_bits + num_bank_bits;
	calc_row_addr_loc = num_rank_bits + num_bank_bits + *num_useful_col_bits_ptr;

	/* Decide whether to use user-specified or default field locations */
	if ( ( *loc_bank_ptr == 0) && (*loc_row_ptr == 0) && (*loc_col_ptr == 0) && (*loc_rank_ptr == 0) ){
		/* Use default */
		use_rank_loc = 0;
		*loc_bank_ptr = calc_bank_loc;
		*loc_rank_ptr = calc_rank_loc;
		*loc_col_ptr  = calc_col_addr_loc;
		*loc_row_ptr  = calc_row_addr_loc;
	}
	else { /* Use user-specified bank_loc, rank_loc, row_loc, and col_loc */
		use_rank_loc = 0;
		if ( *loc_rank_ptr != (*loc_bank_ptr + num_bank_bits) ){
			use_rank_loc = 1;      
		}
	}

	/* Program ADDR_PARAMS_1 */
	data = num_bank_bits |
		(num_rank_bits               << 2) |
		( (num_rank_bits + num_bank_bits) << 4) |
		(*loc_bank_ptr              <<  7) |
		(num_row_bits_minus1        << 12) |
		(*loc_row_ptr               << 16) |
		(*num_useful_col_bits_ptr   << 21) |
		(*loc_col_ptr               << 25);

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_ADDRESS_PARAMS, data);

	/* Program ADDR_PARAMS_2 */
	for (i=0; i<4; i++){
		if((rank_allocation_table+i)->CS_id == 0){
			cs0_rankid = ( ( (rank_allocation_table+i)->logical_rank_id) & 0x3);
		}
		else if((rank_allocation_table+i)->CS_id == 1){
			cs1_rankid = ( ( (rank_allocation_table+i)->logical_rank_id) & 0x3);
		}
		else if((rank_allocation_table+i)->CS_id == 2){
			cs2_rankid = ( ( (rank_allocation_table+i)->logical_rank_id) & 0x3);
		}
		else if((rank_allocation_table+i)->CS_id == 3){
			cs3_rankid = ( ( (rank_allocation_table+i)->logical_rank_id) & 0x3);
		}
	}

	data = MEM_CTL_ADDR_PARAMS_2_AP_LOC |
		(*loc_rank_ptr <<  4) |
		(use_rank_loc <<  9) |
		(cs0_rankid   << 10) |
		(cs1_rankid   << 12) |
		(cs2_rankid   << 14) |
		(cs3_rankid   << 16) |
		(1            << 18); /* Always set USE_RANK_TO_CS_MAP to 1. For Bx silicon and rankId == CSId, 
					 it does no harm since our cs*_rankid's should be accurate anyway.
					 If Bx silicon and rankId != CSId, it is the right thing to do */ 

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_ADDRESS_PARAMS2, data);

	/* Address field overlap, underlap check */
	bank_addr_vect = ((~0)<< *loc_bank_ptr) ^ ((~0)<<(*loc_bank_ptr + num_bank_bits));
	rank_addr_vect = ((~0)<< *loc_rank_ptr) ^ ((~0)<<(*loc_rank_ptr + num_rank_bits));
	bank_rank_vect = ((~0)<< *loc_bank_ptr) ^ ((~0)<<(*loc_bank_ptr + num_rank_bits+num_bank_bits));   
	col_vect =       ((~0)<< *loc_col_ptr)  ^ ((~0)<<(*loc_col_ptr  + *num_useful_col_bits_ptr));   
	row_vect =       ((~0)<< *loc_row_ptr)  ^ ((~0)<<(*loc_row_ptr  + *num_row_bits_ptr));   

	for (i=0x01; i != 0x20000000; i=i<<1){

		one_count = 0;

		if (bank_rank_vect & i){
			one_count++;
		}
		if (col_vect & i){
			one_count++;
		}
		if (row_vect & i){
			one_count++;
		}
		if (one_count == 0){
		}
		else if (one_count > 1){
		}
	}

	for (i=0x01; i != 0x20000000; i=i<<1){

		one_count = 0;

		if (bank_addr_vect & i){
			one_count++;
		}
		if (rank_addr_vect & i){
			one_count++;
		}
		if (col_vect & i){
			one_count++;
		}
		if (row_vect & i){
			one_count++;
		}
		if (one_count == 0){
		}
		else if (one_count > 1){
		}
	}

} 


void  program_params_1(phoenix_reg_t *dramchbase, spd_entry *spd_data_reference, enum mem_protocol mem_type){
	int data;
	int tCCD;

	if (mem_type == DDR1){
		tCCD = (spd_data_reference+ARIZONA_SPD_DDR_tCCD_MIN)->whole;
	}
	else {
		tCCD = MEM_CTL_PARAMS_1_tCCD_DDR2;
	}

	data = ( (spd_data_reference+ARIZONA_SPD_DDR_tRRD)->whole - 1)           |
		( (tCCD-1)                                                <<  5 ) |
		( ( (spd_data_reference+ARIZONA_SPD_DDR_tRP)->whole - 1)  << 10 ) |
		( (MEM_CTL_PARAMS_1_tMRD-1)                               << 15 ) |
		( ( (spd_data_reference+ARIZONA_SPD_DDR_tRFC)->whole - 1) << 20 ) |
		( MEM_CTL_PARAMS_1_tRRDwindow_NUM_CMDS                    << 26 );

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_PARAMS_1, data);
} 

void program_params_2(phoenix_reg_t *dramchbase, spd_entry *spd_data_reference, int blength, int chosen_AL){
	int data;

	data =   ( (spd_data_reference+ARIZONA_SPD_DDR_tRCD)->whole - chosen_AL - 1)         |
		( ( (spd_data_reference+ARIZONA_SPD_DDR_tRC)->whole - 1)              <<  5 ) |
		( ( (spd_data_reference+ARIZONA_SPD_DDR_tRAS)->whole - 1)             << 10 ) |
		( (blength/2 - 1)                                                   << 15 ) |
		( (blength/2 - 1)                                                   << 20 );

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_PARAMS_2, data);
}


void program_params_3(phoenix_reg_t *dramchbase, spd_entry *spd_data_reference, int blength, int chosen_AL, int cas_lat, int twoway_tboard, int treg, enum mem_protocol mem_type, int late_settings_used){
	int data;
	int trtp_1;
	int trtp_2;
	int trtp_3;
	int trtp;
	int twoway_tboard_plus_one;
	int twtr;
	int twtp;
	int trtw;

	if (late_settings_used == 1){
	  twoway_tboard_plus_one = twoway_tboard + 1;
	}
	else{
	  twoway_tboard_plus_one = twoway_tboard;
	}

	trtp_1 = chosen_AL + blength/2 - 1;
	trtp_2 = chosen_AL + blength/2 + (spd_data_reference+ARIZONA_SPD_DDR_tRTP)->whole - 3;

	trtp_3 = ( (chosen_AL + cas_lat + treg + twoway_tboard_plus_one) +  
		   blength/2 -
		   ((spd_data_reference+ARIZONA_SPD_DDR_tRP)->whole - 1) - /* PARAMS1.TRP-1 */ 
		   3 );
 
	if      ( (trtp_3 >= trtp_2) && (trtp_3 >= trtp_1) ){ /* trtp_3 is the largest */
		trtp = trtp_3; 
	}
	else if ( (trtp_2 >= trtp_3) && (trtp_2 >= trtp_1) ){ /* trtp_2 is the largest */
		trtp = trtp_2;
	}
	else {                                                /* trtp_1 is the largest */
		trtp = trtp_1;
	}

	if (mem_type == DDR2){
		twtr = (cas_lat + blength/2 + (spd_data_reference+ARIZONA_SPD_DDR_tWTR)->whole - 2);
	}
	else{
		twtr = blength/2 + SPD_DATA_DDR1_tWTR;
	}

	if (mem_type == DDR2){
		twtp = chosen_AL + cas_lat + blength/2 + (spd_data_reference+ARIZONA_SPD_DDR_tWR)->whole - 2;
	}
	else{ /* DDR1 */
		twtp = blength/2 + (spd_data_reference+ARIZONA_SPD_DDR_tWR)->whole;
	}

	if (mem_type == DDR2){
		trtw = blength/2 + twoway_tboard_plus_one + 1;
	}
	else{
		trtw = cas_lat + blength/2 + twoway_tboard_plus_one - 1;
	}

	data =   trtp |
		( twtp                                           <<  5) |
		( twtr                                           << 10) |
		( trtw                                           << 15) |
	        ( (chosen_AL + cas_lat + treg + twoway_tboard)   << 20); 

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_PARAMS_3, data);
} /* End routine program_params_3 */


void program_params_4 (phoenix_reg_t *dramchbase, spd_entry *spd_data_reference, int blength, int chosen_AL, int cas_lat, int twoway_tboard, int treg, enum mem_protocol mem_type, int late_settings_used){
	int data;
	int odt_en;
	int wr_lat;
	int odt_cmd_dly;

	if (mem_type == DDR1){
		odt_en = 0;
	}
	else {
		odt_en = MEM_CTL_PARAMS_4_DDR2_ODT_EN;
	}

	if (mem_type == DDR1){
		wr_lat = 1;
	}
	else {
		wr_lat = chosen_AL + cas_lat + treg - 1;
	}

	odt_cmd_dly = MEM_CTL_PARAMS_4_tAOND + 3 + treg + twoway_tboard; /* For XLR Bx READs */ 

	data = (wr_lat - 1)                                                      |
		( ( (spd_data_reference+ARIZONA_SPD_DDR_tRP)->whole)      <<  5 ) |
		( odt_en                                                  << 10 ) |
		( odt_cmd_dly                                             << 11 ) | 
		( (blength/2 + 1)                                         << 16 ) |
		( ( MEM_CTL_PARAMS_4_tAOND + 2 + treg)                    << 20 ) | /* This value is correct for Phoenix Bx writes.*/ 
	        ( late_settings_used                                      << 28 ) | /* MEM_CTL_PARAMS_4_RL_EXTRA_CYC_EN */
		( MEM_CTL_PARAMS_4_CONTROLLER_TERM_ALWAYS_EN              << 29 ) |
		( MEM_CTL_PARAMS_4_CONTROLLER_TERM_EN                     << 30 );

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_PARAMS_4, data);

} /* END routine program_params_4 */

void program_params_5 (phoenix_reg_t *dramchbase, spd_entry *spd_data_reference){
	int data;

	data = MEM_CTL_PARAMS_5_tXSRD                                                           |
		( ( ( spd_data_reference+ARIZONA_SPD_DDR_DIMM_REFRESH_RATE)->whole - 21) <<  9 ) |
		( (MEM_CTL_PARAMS_5_REF_CMD_ISS_CNT-1)                                  << 25 );

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_PARAMS_5, data);

} 

void program_params_6(phoenix_reg_t *dramchbase, int blength, int chosen_AL, int cas_lat, int chosen_tck_denom ){
	int data;
	int trasmax;

	trasmax = (MEM_CTL_PARAMS_6_tRASMAX * chosen_tck_denom)/ PHOENIX_DRAM_CLK_PERIOD_NUMERATOR;

	data =  (trasmax-66) |
		( (MEM_CTL_PARAMS_6_tXPRD  - chosen_AL - 1) << 15) |
		( (MEM_CTL_PARAMS_6_tXARDS - chosen_AL - 1) << 18) |
		( (MEM_CTL_PARAMS_6_tCKE_PW_MIN - 1)        << 21) |
		( (chosen_AL + cas_lat + (blength/2)   - 1) << 26);

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_PARAMS_6, data);
} 

void program_params_7(phoenix_reg_t *dramchbase, int blength, int cas_lat, spd_entry *spd_data_reference, enum mem_protocol mem_type){
	int data;
	int twtr;

	if (mem_type == DDR2){
		twtr = cas_lat + blength/2 + (spd_data_reference+ARIZONA_SPD_DDR_tWTR)->whole - 2;
	}
	else{
		twtr = blength/2 + SPD_DATA_DDR1_tWTR;
	}

	data = (blength/2)                                                                                      |
		( twtr                                                                                     << 5) |
		( ( ( (spd_data_reference+ARIZONA_SPD_DDR_tRRD)->whole * 4) +1 )                          << 10) |
		( MEM_CTL_PARAMS_7_ALT_PARAM_RANK_MASK                                                    << 15);

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_PARAMS_7, data);

}

void program_mrs(phoenix_reg_t *dramchbase, spd_entry *spd_data_reference, int blength, int chosen_cas, enum mem_protocol mem_type){
	int data;
	int enc_blength=0;

	int twrite_rec=0;
	int exit_mode = 0;

	if (blength == 4){
		enc_blength = 2;
	}
	else if (blength == 8){
		enc_blength = 3;
	}

	if (mem_type == DDR2){
		twrite_rec = (spd_data_reference+ARIZONA_SPD_DDR_tWR)->whole - 1;
	}
	else{
		twrite_rec = 0; /* This field is unused in DDR1 */
	}

	if (mem_type == DDR2){
		exit_mode = MEM_CTL_MRS_PWRDWN_EXIT_MODE;
	}
	else{
		exit_mode = 0; /* This field is unused in DDR1 */
	}

	data = enc_blength |
		( MEM_CTL_MRS_BURST_TYPE                                 <<  3 ) |
		( chosen_cas                                             <<  4 ) |
		( MEM_CTL_MRS_TEST_MODE                                  <<  7 ) |
		( MEM_CTL_MRS_DLL_RESET                                  <<  8 ) |
		( twrite_rec                                             <<  9 ) |
		( exit_mode                                              << 12 );

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_MRS, data);

}

void program_rdmrs(phoenix_reg_t *dramchbase, spd_entry *spd_data_reference, int blength, int chosen_cas, enum mem_protocol mem_type){
	int data;
	int enc_blength=0;

	int twrite_rec=0;
	int exit_mode = 0;

	if (blength == 4){
		enc_blength = 2;
	}
	else if (blength == 8){
		enc_blength = 3;
	}

	if (mem_type == DDR2){
		twrite_rec = (spd_data_reference+ARIZONA_SPD_DDR_tWR)->whole - 1;
	}
	else{
		twrite_rec = 0; /* This field is unused in DDR1 */
	}

	if (mem_type == DDR2){
		exit_mode = MEM_CTL_RDMRS_PWRDWN_EXIT_MODE;
	}
	else{
		exit_mode = 0; /* This field is unused in DDR1 */
	}

	data = enc_blength |
		( MEM_CTL_RDMRS_BURST_TYPE                                 <<  3 ) |
		( chosen_cas                                               <<  4 ) |
		( MEM_CTL_RDMRS_TEST_MODE                                  <<  7 ) |
		( MEM_CTL_RDMRS_DLL_RESET                                  <<  8 ) |
		( twrite_rec                                               <<  9 ) |
		( exit_mode                                                << 12 );

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RDMRS, data);

}

void program_emrs1(phoenix_reg_t *dramchbase, int chosen_AL, enum mem_protocol mem_type, int dimm_presence_vect[] ){
	int data;

	if (mem_type == DDR1){
		data = MEM_CTL_EMRS1_DLL_EN |
			( MEM_CTL_EMRS1_OUT_DRIVE_STRENGTH <<  1);
	}

	else if ( (mem_type == DDR2) && dimm_presence_vect[0] && dimm_presence_vect[1] ){ /* Rtt = 75 Ohm */

		data = MEM_CTL_EMRS1_DLL_EN |
			( MEM_CTL_EMRS1_OUT_DRIVE_STRENGTH <<  1) |
			( 1                                <<  2) |
			( chosen_AL                        <<  3) |
			( 0                                <<  6) |
			( MEM_CTL_EMRS1_OCD_OPERATION      <<  7) |
			( 0                                << 10) |
			( MEM_CTL_EMRS1_RDQS_EN            << 11) |
			( MEM_CTL_EMRS1_OUT_EN             << 12); 

	}
	else { /* Rtt = 150 Ohm */ 
		data = MEM_CTL_EMRS1_DLL_EN |
			( MEM_CTL_EMRS1_OUT_DRIVE_STRENGTH <<  1) |
			( 0                                <<  2) |
			( chosen_AL                        <<  3) |
			( 1                                <<  6) |
			( MEM_CTL_EMRS1_OCD_OPERATION      <<  7) |
			( 0                                << 10) |
			( MEM_CTL_EMRS1_RDQS_EN            << 11) |
			( MEM_CTL_EMRS1_OUT_EN             << 12); 

	}

	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_EMRS1, data);
  
}

void program_tx0_therm_ctrl(phoenix_reg_t *dramchbase, enum mem_protocol mem_type){
	int data;

	data = MEM_CTL_TX0_THERM_CTRL_EN_CNT |
		( MEM_CTL_TX0_THERM_CTRL_PRESET_VAL << 1) |
		( MEM_CTL_TX0_THERM_CTRL_PRESET_N   << 7) |
		( MEM_CTL_TX0_THERM_CTRL_PRESET_P   << 8) |
		( (mem_type == DDR2)                << 9);

	phoenix_verbose_write_reg (dramchbase, DDR2_TX0_THERM_CTRL_WR_ADDR, data);

}

void program_rx0_therm_ctrl(phoenix_reg_t *dramchbase, int dimm_presence_vect[]){
	int data;

	data = MEM_CTL_RX0_THERM_CTRL_EN_CNT |
		( MEM_CTL_RX0_THERM_CTRL_PRESET_VAL << 1) |
		( MEM_CTL_RX0_THERM_CTRL_PRESET_N   << 7) |
		( MEM_CTL_RX0_THERM_CTRL_PRESET_P   << 8) |
		( 0                                 << 9) ;


       if ((PRODUCT_IS_OF_UFABRIC_QFX3600_SERIES(fx_get_board_type())) ||
            (PRODUCT_IS_OF_SE_QFX3600_SERIES(fx_get_board_type()))) {
              /*
             * For all QFX3600 series (XLS processor) boards,
             * specify the 150 Ohm controller termination.
             */

            data |= (MEM_CTL_PARAMS_4_CONTROLLER_TERM_EN << 11);
       } else {
            /* 75 Ohm controller termination if there is only 1 dimm */
            data |= ((dimm_presence_vect[0] & dimm_presence_vect[1]) == 0) << 10;
       }

	phoenix_verbose_write_reg (dramchbase, DDR2_RX0_THERM_CTRL_WR_ADDR, data);

}

void dram_spd_exit(){
	printf_err("\n\n=============== DRAM SPD ERROR, BAIL OUT ====================\n");
 deadloop:
	goto deadloop; 
}

void program_reset_sequence(phoenix_reg_t *dramchbase, enum mem_protocol mem_type) {
	int data;

	if (mem_type == DDR1){ /* DDR1 reset sequence */
		data = MEM_CTL_DDR1_RESET_SEQ_CMD0 |
			( MEM_CTL_DDR1_RESET_SEQ_CMD0_CNT <<  6 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD1     << 16 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD1_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD0, data );    

		data = MEM_CTL_DDR1_RESET_SEQ_CMD2 |
			( MEM_CTL_DDR1_RESET_SEQ_CMD2_CNT <<  6 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD3     << 16 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD3_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD1, data );    

		data = MEM_CTL_DDR1_RESET_SEQ_CMD4 |
			( MEM_CTL_DDR1_RESET_SEQ_CMD4_CNT <<  6 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD5     << 16 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD5_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD2, data );    

		data = MEM_CTL_DDR1_RESET_SEQ_CMD6 |
			( MEM_CTL_DDR1_RESET_SEQ_CMD6_CNT <<  6 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD7     << 16 ) |
			( MEM_CTL_DDR1_RESET_SEQ_CMD7_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD3, data );    

	}
	else { /* DDR2 reset sequence */

		data = MEM_CTL_DDR2_RESET_SEQ_CMD0 |
			( MEM_CTL_DDR2_RESET_SEQ_CMD0_CNT <<  6 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD1     << 16 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD1_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD0, data );    

		data = MEM_CTL_DDR2_RESET_SEQ_CMD2 |
			( MEM_CTL_DDR2_RESET_SEQ_CMD2_CNT <<  6 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD3     << 16 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD3_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD1, data );    

		data = MEM_CTL_DDR2_RESET_SEQ_CMD4 |
			( MEM_CTL_DDR2_RESET_SEQ_CMD4_CNT <<  6 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD5     << 16 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD5_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD2, data );    

		data = MEM_CTL_DDR2_RESET_SEQ_CMD6 |
			( MEM_CTL_DDR2_RESET_SEQ_CMD6_CNT <<  6 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD7     << 16 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD7_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD3, data );    

		data = MEM_CTL_DDR2_RESET_SEQ_CMD8 |
			( MEM_CTL_DDR2_RESET_SEQ_CMD8_CNT <<  6 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD9     << 16 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD9_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD4, data );    

		data = MEM_CTL_DDR2_RESET_SEQ_CMD10 |
			( MEM_CTL_DDR2_RESET_SEQ_CMD10_CNT <<  6 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD11     << 16 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD11_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD5, data );    

		data = MEM_CTL_DDR2_RESET_SEQ_CMD12 |
			( MEM_CTL_DDR2_RESET_SEQ_CMD12_CNT <<  6 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD13     << 16 ) |
			( MEM_CTL_DDR2_RESET_SEQ_CMD13_CNT << 22 );

		phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_CMD6, data );    
	}
  
} /* END program_reset_sequence */

void set_wait_func_limit (int chosen_period_denom, char chip_rev[]){

  phoenix_reg_t *gpiobase = phoenix_io_mmio(PHOENIX_IO_GPIO_OFFSET);

  unsigned long int cfg_reg;
  unsigned long int fb_plus_one;
  unsigned long int od_plus_one;
  unsigned long int pvDIVF_plus_one;
  unsigned long int pvDIVQ;
  unsigned long int divq = 0;

  /* Note that for the purposes of the delay function it is better to      */
  /* count a fixed number of DRAM clock cycles than CPU clock cycles,      */
  /* since the time required for various DRAM controller tasks to complete */
  /* (e.g. calibration) is proportional to the DRAM clock frequency,       */
  /* and is independent of the CPU clock frequency.                        */

  cfg_reg = phoenix_read_reg(gpiobase,GPIO_RESET_CFG);

  if (chip_rev[2] == 'R'){
    /* For XLR:                                                              */
    /* cpu  frequency = ((feedback + 1)/(output_divider + 1)) * 16.67 MHz    */
    /* DRAM frequency = (chosen_period_denom/30)              * 1     GHz    */
    /* Therefore, to convert n DRAM clock cycles to cpu cycles:              */
    /*                                                                       */
    /*                        n * (feedback + 1)                             */
    /* number of cpu cycles = ---------------------------------------------- */
    /*                        2 * (output_divider + 1) * chosen_period_denom */
    /*                                                                       */
    /* To get the delay loop index, I then divide by 3 (the smallest loop    */
    /* will be at least three assembly instructions: ld/slt/bnez             */
    fb_plus_one = ((cfg_reg >> 2) & 0x7f) + 1;
    od_plus_one = ((cfg_reg >> 9) &  0x1) + 1;
    wait_loop_index = (6250 * fb_plus_one)/(6 * od_plus_one * chosen_period_denom); 
  }
  else { 
    /* For XLS:                                                              */
    /* Note - pv(FOO) means the value programmed into the FOO register field */
    /*                                                                       */
    /* cpu  frequency = ((pv[DIVF] + 1)/2^pv[DIVQ]) * 33.33 MHz              */
    /* DRAM frequency = (chosen_period_denom/30)    * 1     GHz              */
    /* Therefore, to convert n DRAM clock cycles to cpu cycles:              */
    /*                                                                       */
    /*                        n * (pv[DIVF] + 1)                             */
    /* number of cpu cycles = -------------------------------------          */
    /*                        (2 ^(pv[DIVQ])) * chosen_period_denom          */
    /*                                                                       */
    /* To get the delay loop index, I then divide by 3 (the smallest loop    */
    /* will be at least three assembly instructions: ld/slt/bnez             */
    pvDIVF_plus_one = (cfg_reg & 0xff) + 1;
    pvDIVQ         = ((cfg_reg & 0x700) >> 8); 

    /* Calculate divq */
    if (pvDIVQ == 1){
      divq = 2;
    }
    else if (pvDIVQ == 2){
      divq = 4;
    }
    else if (pvDIVQ == 3){
      divq = 8;
    }
    else if (pvDIVQ == 4){
      divq = 16;
    }
    else{
      printf_err("[Error]: Detected illegal value of XLS CPU PLL DIVQ field:%lu\n", pvDIVQ);
      dram_spd_exit();
    }
    wait_loop_index = (6250 * pvDIVF_plus_one)/(3 * divq * chosen_period_denom); 
  }
  /* printf("Chosen wait loop index based on CPU and DRAM frequency:%lu\n", wait_loop_index); */

} /* END set_wait_func_limit */


void wait_func(){

  int i;

  for (i = 0; i< wait_loop_index; i++){
  }

} /* END function wait_func */

void program_eye_finder_bridge_map(phoenix_reg_t *bridgebase, int ch_id, int wipe){

	unsigned int data;
	unsigned int i;

	/* Begin by wiping the following registers: */
	/*   -BRIDGE_DRAM_[6 to 0]_BAR              */
	/*   -BRIDGE_DRAM_CHN_0_MTR_[6 to 0]_BAR    */
	/*   -BRIDGE_DRAM_CHN_1_MTR_[6 to 0]_BAR    */
	/* The following registers are preserved, since they cover the */
	/* SPD code address space locked in L2:                        */
	/*   -BRIDGE_DRAM_7_BAR              */
	/*   -BRIDGE_DRAM_CHN_0_MTR_7_BAR    */
	/*   -BRIDGE_DRAM_CHN_1_MTR_7_BAR    */
	/* They will be deleted once we return to flash after executing */
	/* the SPD code.                                                */

	data = 0x0;
	for (i= 0;i<7;i++){
		phoenix_verbose_write_reg(bridgebase, i, data);
	} 
	for (i= 8;i<15;i++){
		phoenix_verbose_write_reg(bridgebase, i, data);
	} 
	for (i=16;i<23;i++){
		phoenix_verbose_write_reg(bridgebase, i, data);
	} 

	/* Now, if required, program BAR0, and corresponding MTR */
	if (wipe == 0){

		/* -Enabled                                                      */
		/* -16MB so it won't overlap any other BAR                       */
		/* -0x00_0000_0000 -> 0x00_00FF_FFFF                             */
		/* -No interleaving so we don't have to worry about those bits   */
		data = (0x0 << 16) | (0x0 << 4) | (ch_id << 1) | 0x1;
		phoenix_verbose_write_reg(bridgebase, BRIDGE_DRAM_0_BAR, data);

		/* Whole, so that we don't have to worry about POS */
		data = (0x0 << 12) | (0x0 << 8) | 0x0; 
		phoenix_verbose_write_reg(bridgebase, BRIDGE_DRAM_CHN_0_MTR_0_BAR, data);
		phoenix_verbose_write_reg(bridgebase, BRIDGE_DRAM_CHN_1_MTR_0_BAR, data);

	}

} /*END routine program_eye_finder_bridge_map */

void run_test ( int num_ranks, 
                int rank_loc, 
		int num_banks, 
                int bank_loc, 
                int num_useful_col_bits, 
                int col_loc, 
                int num_row_bits, 
                int row_loc, 
                int blength, 
                int start_byte,
	        int stride,
	        int *fail_status_ptr,
		int debug){

	/* start_byte input parameter values should be as follows, for the normal XLR endian-ness: */
	/* BL=4, DQS0: 7                                                                           */
	/* BL=4, DQS1: 6                                                                           */
	/* BL=4, DQS2: 5                                                                           */
	/* BL=4, DQS3: 4                                                                           */
	/* BL=4, DQS4: 3                                                                           */
	/* BL=4, DQS5: 2                                                                           */
	/* BL=4, DQS6: 1                                                                           */
	/* BL=4, DQS7: 0                                                                           */
	/* BL=8, DQS0: 3                                                                           */
	/* BL=8, DQS1: 2                                                                           */
	/* BL=8, DQS2: 1                                                                           */
	/* BL=8, DQS3: 0                                                                           */
	/* BL=8, DQS4: 3                                                                           */
	/* BL=8, DQS5: 2                                                                           */
	/* BL=8, DQS6: 1                                                                           */
	/* BL=8, DQS7: 0                                                                           */

	/*  -Stride = 32/blength                                                                                */
	/*  -For each rank {                                                                                    */
	/*		   -Write                 0b10101010 to   10_row, 01_col, rank, 01_bank, byte n              */
	/*		   -Write                 0b01010101 to   10_row, 01_col, rank, 01_bank, byte n +    stride  */
	/*		   -Write                 0b10101010 to   10_row, 01_col, rank, 01_bank, byte n + (2*stride) */
	/*		   -Write                 0b01010101 to   10_row, 01_col, rank, 01_bank, byte n + (3*stride) */
	/*		   -If blength = 8, write 0b10101010 to   10_row, 01_col, rank, 01_bank, byte n + (4*stride) */
	/*		   -If blength = 8, write 0b01010101 to   10_row, 01_col, rank, 01_bank, byte n + (5*stride) */
	/*		   -If blength = 8, write 0b10101010 to   10_row, 01_col, rank, 01_bank, byte n + (6*stride) */
	/*		   -If blength = 8, write 0b01010101 to   10_row, 01_col, rank, 01_bank, byte n + (7*stride) */

	/*		   -Read from                 10_row, 01_col, rank, 01_bank, byte n             , check it   */
	/*		   -Read from                 10_row, 01_col, rank, 01_bank, byte n +    stride , check it   */
	/*		   -Read from                 10_row, 01_col, rank, 01_bank, byte n + (2*stride), check it   */
	/*		   -Read from                 10_row, 01_col, rank, 01_bank, byte n + (3*stride), check it   */
	/*		   -If blength = 8, read from 10_row, 01_col, rank, 01_bank, byte n + (4*stride), check it   */
	/*		   -If blength = 8, read from 10_row, 01_col, rank, 01_bank, byte n + (5*stride), check it   */
	/*		   -If blength = 8, read from 10_row, 01_col, rank, 01_bank, byte n + (6*stride), check it   */
	/*		   -If blength = 8, read from 10_row, 01_col, rank, 01_bank, byte n + (7*stride), check it   */
	/*  } for each rank                                                                                     */
	/*  -Return overall PASS/FAIL status                                                                    */

	/* Local variables */
	int rank_id;
	int bank_id;
	int w_cnt;       /* word count */

	unsigned int row_addr;
	unsigned int col_addr;

	unsigned int  seed = 0;
	unsigned char data = 0x55;
	unsigned char read_data;

	unsigned int   temp_addr;
	unsigned char *char_ptr;

	/* ----------------------------------------- Write loop ----------------------------------------------*/
	for (rank_id = 0; rank_id < num_ranks; rank_id++){

		if (rank_id & 0x1){ /* If rank is odd */
			col_addr = 0xAAAAAAAA & ((~0) ^ ((~0) << num_useful_col_bits) ); 
			row_addr = 0x55555555 & ((~0) ^ ((~0) << num_row_bits) );        
		}
		else {
			col_addr = 0x55555555 & ((~0) ^ ((~0) << num_useful_col_bits) ); 
			row_addr = 0xAAAAAAAA & ((~0) ^ ((~0) << num_row_bits) );        
		}

		for (bank_id = 0; bank_id < num_banks; bank_id++){

			for (w_cnt = 0; w_cnt < blength; w_cnt++){

				temp_addr = 0x007fffff & ( (row_addr  << ( row_loc+5)) | /* Limit this portion of the address to 23 bits for 8MB BAR*/
							   (col_addr  << ( col_loc+5)) |
							   (rank_id   << (rank_loc+5)) | 
							   (bank_id   << (bank_loc+5)) |
							   (start_byte + (w_cnt * stride)) );
 
				temp_addr = 0xa0000000 | temp_addr; /* Place it in the unmapped, uncached kseg1 region to get around caches, TLB's */

				seed = (rank_id) * 8 + bank_id; /* Will range between 0 and 31 */

				switch (w_cnt) {

				  case 7: data = ~seed;
				          break;

				  case 6: data = seed;
				          break;

				  case 5: data = ~(seed + 32); 
				          break;

				  case 4: data = seed + 32;
				          break;

				  case 3: data = ~(seed + 64);
				          break;

				  case 2: data = seed + 64;
				          break;

				  case 1: data = ~(seed + 96);
				          break;

				  case 0: data = seed + 96;
				          break;     
				}

				/* Do the byte write */
				char_ptr = (unsigned char *) temp_addr;
				if (debug){
					printf("Write: %p 0x%x\n", char_ptr, data);
				}
				*char_ptr = data;
 
			} /* for w_cnt */
		} /* for bank_id */
	} /* for rank_id */   

	/* ----------------------------------------- Read loop ----------------------------------------------*/

	for (w_cnt = 0; w_cnt < blength; w_cnt++){

		for (rank_id = 0; rank_id < num_ranks; rank_id++){

			if (rank_id & 0x1){ /* If rank is odd */
				col_addr = 0xAAAAAAAA & ((~0) ^ ((~0) << num_useful_col_bits) ); 
				row_addr = 0x55555555 & ((~0) ^ ((~0) << num_row_bits) );        
			}
			else {
				col_addr = 0x55555555 & ((~0) ^ ((~0) << num_useful_col_bits) ); 
				row_addr = 0xAAAAAAAA & ((~0) ^ ((~0) << num_row_bits) );        
			}

			for (bank_id = (num_banks-1); bank_id >=0; bank_id--){

				temp_addr = 0x007fffff & ( (row_addr  << ( row_loc+5)) | /* Limit this portion of the address to 23 bits for 8MB BAR */
							   (col_addr  << ( col_loc+5)) |
							   (rank_id   << (rank_loc+5)) | 
							   (bank_id   << (bank_loc+5)) |
							   (start_byte + (w_cnt * stride)) );
 
				temp_addr = 0xa0000000 | temp_addr; /* Place it in the unmapped, uncached kseg1 region to get around caches, TLB's */

				seed = (rank_id * 8) + bank_id; /* Will range between 0 and 31 */

				switch (w_cnt) {

				  case 7: data = ~seed;
				          break;

				  case 6: data = seed;
				          break;

				  case 5: data = ~(seed + 32); 
				          break;

				  case 4: data = seed + 32;
				          break;

				  case 3: data = ~(seed + 64);
				          break;

				  case 2: data = seed + 64;
				          break;

				  case 1: data = ~(seed + 96);
				          break;

				  case 0: data = seed + 96;
				          break;     
				}

				/* Do the byte read */
				char_ptr = (unsigned char *) temp_addr;
				read_data = *char_ptr; 
				if (read_data != data){
					*fail_status_ptr = (*fail_status_ptr) | (0x1<<start_byte);
					if (debug == 1){
						printf("Read mismatch: Address:%p Expected:0x%x Received:0x%x\n", char_ptr, data, read_data);
					}
					return;
				}
			} /* For bank_id */

		} /* for rank_id */
	} /* for w_cnt */   


} /* END function run_test */


int dram_page_thrash( int num_banks, 
		      int bank_loc,  
		      int num_ranks, 
		      int rank_loc,  
		      int num_useful_cols,
		      int col_loc,        
		      int bar_size,
		      int row_loc,
		      int seed,
		      int debug){       


	/* The purpose of this test is to cause as many DRAM page openings 
	   and closings as possible. This is achieved by repeatedly accessing 
	   different rows in the same rank and bank. Consequently, the desired
	   toggle order is:

	   Rank - changes less
	   Bank
	   Col 
	   Row  - changes more

	   Note that this has the benefit of thrashing a lot in L1 and L2 caches,
	   i.e. a lot of memory traffic is generated. This is because a given 
	   {rank_id, bank_id, col_id} value fixes at least PA[12:0] 
	   (and maybe as much as PA[18:0], depending on the memory configuration),
	   if we assume the following DRAM address ordering: 
	   {row,col,rank,bank,interleave,[4:0]}.
	   Since the L1 cache interprets addresses like this: 

	   Tag     Index  Line
	   [39:12] [11:5] [4:0] 
 
	   and the XLR 732 L2 interprets addresses like this:

	   Tag     Set    Bank  Line 
	   [39:18] [17:8] [7:5] [4:0] 

	   ...with PA[12:0] fixed, we will be hitting in the same cache set a lot.
	   Of course the 8-way associativity of each cache will take its toll, but
	   we should still get a decent amount of memory traffic, even with a data
	   set smaller than 2MB+32KB. 
	*/ 

	volatile unsigned long int L1_base_addr;
	volatile unsigned long int L2_base_addr;
	volatile unsigned long int L3_base_addr;
	volatile unsigned long int L4_base_addr;
	volatile unsigned long int L5_base_addr;
	volatile unsigned long int temp_data;

	unsigned volatile long int *addr;

	int bank_id;
	int rank_id;
	int row_id;
	int col_id;
	int word_id;

	int num_avail_rows;
	volatile unsigned long int read_data;

	int fail_status=0;

	num_avail_rows = (bar_size << 20)/(num_banks * num_ranks * num_useful_cols * 32);

	for (word_id = 0; word_id < 8; word_id++){ 

		if (debug){
			printf("Word: %d\n", word_id);
		}

#ifdef SPD_IN_L2
		/* TEMPORARY: Making test non-cacheable */ 
		L1_base_addr = 0xa0800000 | (word_id << 2); /* Start at 8MB */
#else
		L1_base_addr = 0x80800000 | (word_id << 2); /* Start at 8MB */
#endif

		SPD_PRINTF("L1_base_addr 0x%lx\n",L1_base_addr);

		for (rank_id = 0; rank_id < num_ranks; rank_id++){

			L2_base_addr = L1_base_addr | (rank_id << (rank_loc+5)); 

			for (bank_id = 0; bank_id < num_banks; bank_id++){

				L3_base_addr = L2_base_addr | (bank_id << (bank_loc+5));

				for (col_id = 0; col_id < num_useful_cols; col_id++){

					/* Construct the base address */
					L4_base_addr = L3_base_addr | (col_id  << ( col_loc+5));

					/* Write loop */
					for (row_id = 0; row_id < num_avail_rows; row_id++){

						/* Construct the address */
						L5_base_addr = L4_base_addr | (row_id << (row_loc+5));
						addr = (unsigned long int *) L5_base_addr;

						/* Construct the data */
						temp_data = ( L5_base_addr | (seed << 24) );
			   
						if (word_id & 0x2){
							*addr = temp_data;
							/* printf("Write: %p 0x%x\n", addr, temp_data); */ 
						}
						else {
							*addr = ~temp_data;
							/* printf("Write: %p 0x%x\n", addr, ~temp_data); */
						}

					}

					/* Read loop */
					for (row_id = 0; row_id < num_avail_rows; row_id++){

						/* Construct the address */
						L5_base_addr = L4_base_addr | (row_id << (row_loc+5));
						addr = (unsigned long int *) L5_base_addr;

						/* Read an integer */
						/* printf("Read : %p\n", addr); */
						read_data = *addr;

						/* Construct the expected data */
						temp_data = (L5_base_addr | (seed << 24) );

						if ( (word_id & 0x2) && (read_data != temp_data) ){
							fail_status = 1;
							/* printf("Read data mismatch: Address %p Expected: %lx Received: %lx\n", addr, L5_base_addr, read_data); */
						}
						else if ( (!(word_id & 0x2)) && (read_data != (~temp_data))  ){
							fail_status = 1;
							/* printf("Read data mismatch: Address %p Expected: %lx Received: %lx\n", addr, ~L5_base_addr, read_data); */
						}

					}
					if (fail_status){
						return(1);
					}

				} /* col_id */
			} /* bank_id */
		} /* rank_id */
	} /* word_id */
	return(0);

} /* END function dram_page_thrash */


int find_center(int d_array[], int size){

	unsigned short int i;
	unsigned long  int running_sum = 0;
	unsigned long  int success_count = 0;

	int remainder = 0;
	int quotient = 0;

	for (i=0; i<size; i++) {
		running_sum   += i*d_array[i];
		success_count += d_array[i];
	} 

	if (success_count > 0){
	  quotient  = running_sum / success_count;
	  remainder = running_sum % success_count;
	  if ((2 * remainder) >= success_count){
	    quotient++;
	  }
	}
	return(quotient);

} /* END routine find_center */

void update_results( int fail_status,
 		     int rx_strobe,
		     int tx_strobe, 
		     int stride){

	int i;

	/* Update results tables. */
	/* NOTE: This portion of the code will break if the endian-ness of the processor is changed */
	/* The following byte address -> DQS strobe mapping is assumed:*/
	/* 72-                                                                     */
	/* bit                             1st  2nd  3rd  4th  5th  6th  7th  8th  */
	/* mod DQS Pin name	           chnk chnk chnk chnk chnk chnk chnk chnk */
	/* === === ======================  ==== ==== ==== ==== ==== ==== ==== ==== */
	/* 0   0   DDR[A/B/C/D]_DQ[ 7: 0]  31   27   23   19   15   11   7    3    */
	/* 0   1   DDR[A/B/C/D]_DQ[15: 8]  30   26   22   18   14   10   6    2    */
	/* 0   2   DDR[A/B/C/D]_DQ[23:16]  29   25   21   17   13   9    5    1    */
	/* 0   3   DDR[A/B/C/D]_DQ[31:24]  28   24   20   16   12   8    4    byte0*/
	/*                                                                         */
	/* 1   0   DDR[A/C]_DQ[ 7: 0]      31   23   15   7                        */
	/* 1   1   DDR[A/C]_DQ[15: 8]      30   22   14   6                        */
	/* 1   2   DDR[A/C]_DQ[23:16]      29   21   13   5                        */
	/* 1   3   DDR[A/C]_DQ[31:24]      28   20   12   4                        */
	/* 1   4   DDR[B/D]_DQ[ 7: 0]      27   19   11   3                        */
	/* 1   5   DDR[B/D]_DQ[15: 8]      26   18   10   2                        */
	/* 1   6   DDR[B/D]_DQ[23:16]      25   17   9    1                        */
	/* 1   7   DDR[B/D]_DQ[31:24]      24   16   8    byte0                    */

	for (i=0;i<stride;i++){

		/* BL/stride i DQS */
		/* ========= = === */
		/* 8/4       0 3   */
		/* 8/4       1 2   */
		/* 8/4       2 1   */
		/* 8/4       3 0   */
		/* 4/8       0 7   */
		/* 4/8       1 6   */
		/* 4/8       2 5   */
		/* 4/8       3 4   */
		/* 4/8       4 3   */
		/* 4/8       5 2   */
		/* 4/8       6 1   */
		/* 4/8       7 0   */

		/* fail_status[7] is set if running test on byte address 7 (not DQS 7) failed. */
		/* fail_status[6] is set if running test on byte address 6 (not DQS 6) failed. */
		/* fail_status[5] is set if running test on byte address 5 (not DQS 5) failed. */
		/* fail_status[4] is set if running test on byte address 4 (not DQS 4) failed. */
		/* fail_status[3] is set if running test on byte address 3 (not DQS 3) failed. */
		/* fail_status[2] is set if running test on byte address 2 (not DQS 2) failed. */
		/* fail_status[1] is set if running test on byte address 1 (not DQS 1) failed. */
		/* fail_status[0] is set if running test on byte address 0 (not DQS 0) failed. */

		if ((fail_status & (0x1<<i)) == 0x0){ /* i.e. if PASS */
			/* Update Tx, Rx results*/
		  dqs_data_table[stride-1-i].rx_sweep_data[rx_strobe]++;
		  dqs_data_table[stride-1-i].tx_sweep_data[tx_strobe/TX_STEP]++;
		  dqs_data_table[stride-1-i].pass_region_size++;
		}
	}

} /* END function update results */


void program_strobe_regs (
	phoenix_reg_t     *dramchbase, 
	int               blength,
	int               c_strobe,
	int               sc_strobe,
	int               rx0_strobe,
	int               rx1_strobe,
	int               rx2_strobe,
	int               rx3_strobe,
	int               rx4_strobe,
	int               rx5_strobe,
	int               rx6_strobe,
	int               rx7_strobe,
	int               rx8_strobe, /* ECC */ 
	int               rx9_strobe, /* ECC */ 
	int               tx0_strobe,
	int               tx1_strobe,
	int               tx2_strobe,
	int               tx3_strobe,
	int               tx4_strobe,
	int               tx5_strobe,
	int               tx6_strobe,
	int               tx7_strobe, 
	int               tx8_strobe, /* ECC */ 
	int               tx9_strobe, /* ECC */ 
	int               ch_id){

	/* Local variables */
	unsigned int data;
	unsigned volatile int readback;

	/* -Deactivate calibration triggering by clearing DDR_CLK_CAL_PARAMS.CLK_TRG_OFF_REF */
	data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_CLK_CALIBRATE_PARAMS);
	data = data & 0xf7ff;
	phoenix_verbose_write_reg(dramchbase, DDR2_CONFIG_REG_CLK_CALIBRATE_PARAMS, data);  

	/* Readback last write to make sure all previous writes have completed */
	readback = 1;
	while (readback != 0){
		readback = 0x800 & phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_CLK_CALIBRATE_PARAMS);
	}

	/* Wait while any in-flight calibrations complete */
	wait_func();

	/* -Deactivate updates of NXT_DLY_CNT */
	data = phoenix_read_reg(dramchbase, ef_info.calibrate_disable_reg_addr);
	data = data & ef_info.calibrate_disable_and_mask;
	phoenix_verbose_write_reg(dramchbase, ef_info.calibrate_disable_reg_addr, data);  

	/* -Program CMD strobe,                    */
	/* -Program SLV_CMD strobe                 */
	/* -Program Tx strobe,                     */
	/* -Program Rx strobe, bit configurations  */
	phoenix_verbose_write_reg (dramchbase, DDR2_CMD_DELAY_STAGE_CONFIG,     c_strobe);
	if (blength == 4){ /* If blength = 8, i.e. 36-bit channel, we should not program SLV_CMD */
		phoenix_verbose_write_reg (dramchbase, DDR2_SLV_CMD_DELAY_STAGE_CONFIG, sc_strobe);
	}

	phoenix_verbose_write_reg (dramchbase, DDR2_DQS0_TX_DELAY_STAGE_CONFIG, tx0_strobe);
	phoenix_verbose_write_reg (dramchbase, DDR2_DQS1_TX_DELAY_STAGE_CONFIG, tx1_strobe);
	phoenix_verbose_write_reg (dramchbase, DDR2_DQS2_TX_DELAY_STAGE_CONFIG, tx2_strobe);
	phoenix_verbose_write_reg (dramchbase, DDR2_DQS3_TX_DELAY_STAGE_CONFIG, tx3_strobe);

	if (blength == 4){ /* 72-bit channel-pair */
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS4_TX_DELAY_STAGE_CONFIG, tx4_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS5_TX_DELAY_STAGE_CONFIG, tx5_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS6_TX_DELAY_STAGE_CONFIG, tx6_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS7_TX_DELAY_STAGE_CONFIG, tx7_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS8_TX_DELAY_STAGE_CONFIG, tx8_strobe); 
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS9_TX_DELAY_STAGE_CONFIG, tx9_strobe);
	}
	else{ /* 36-bit channel */
	  if ((ch_id == 0) || (ch_id == 2)){ /* Channels A or C */
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS8_TX_DELAY_STAGE_CONFIG, tx8_strobe); 
	  }
	  else{ /* Channels B or D */
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS9_TX_DELAY_STAGE_CONFIG_36_WR_ADDR, tx9_strobe); 
	  }
	}

	if (blength == 4){ 
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS4_RX_DELAY_STAGE_CONFIG, rx4_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS5_RX_DELAY_STAGE_CONFIG, rx5_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS6_RX_DELAY_STAGE_CONFIG, rx6_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS7_RX_DELAY_STAGE_CONFIG, rx7_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS8_RX_DELAY_STAGE_CONFIG, rx8_strobe);
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS9_RX_DELAY_STAGE_CONFIG, rx9_strobe);
	}
	else{ /* 36-bit channel */
	  if ((ch_id == 0) || (ch_id == 2)){ /* Channels A or C */
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS8_RX_DELAY_STAGE_CONFIG, rx8_strobe); 
	  }
	  else{ /* Channels B or D */
		phoenix_verbose_write_reg (dramchbase, DDR2_DQS9_RX_DELAY_STAGE_CONFIG_36_WR_ADDR, rx9_strobe); 
	  }
	}

	phoenix_verbose_write_reg (dramchbase, DDR2_DQS0_RX_DELAY_STAGE_CONFIG, rx0_strobe);
	phoenix_verbose_write_reg (dramchbase, DDR2_DQS1_RX_DELAY_STAGE_CONFIG, rx1_strobe);
	phoenix_verbose_write_reg (dramchbase, DDR2_DQS2_RX_DELAY_STAGE_CONFIG, rx2_strobe);
	phoenix_verbose_write_reg (dramchbase, DDR2_DQS3_RX_DELAY_STAGE_CONFIG, rx3_strobe);


	/* Readback last write to make sure all previous writes have completed */
	readback = phoenix_read_reg(dramchbase, DDR2_DQS3_RX_DELAY_STAGE_CONFIG);
	while (readback != rx3_strobe){
		readback = phoenix_read_reg(dramchbase, DDR2_DQS3_RX_DELAY_STAGE_CONFIG);
	}

	/* -Reactivate updates of NXT_DLY_CNT */
	data = phoenix_read_reg(dramchbase, ef_info.calibrate_disable_reg_addr);
	data = data | ef_info.calibrate_enable_or_mask;
	phoenix_verbose_write_reg(dramchbase, ef_info.calibrate_disable_reg_addr, data);  
  
	/* -Manually trigger a calibration */
	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_CLK_CALIBRATE_REQUEST, 0);

	/* Wait while manual calibration completes */
	wait_func();

	/* -Reactivate calibration triggering by setting DDR_CLK_CAL_PARAMS.CLK_TRG_OFF_REF */
	data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_CLK_CALIBRATE_PARAMS);
	data = data | 0x800;
	phoenix_verbose_write_reg(dramchbase, DDR2_CONFIG_REG_CLK_CALIBRATE_PARAMS, data);  

	/* NOTE: CAN NOT do a readback here */

} /* END program_strobe_regs */

int program_P34_activate(
	phoenix_reg_t     *dramchbase,
	spd_entry         *spd_data_reference,
	int               blength,
	int               chosen_AL,
	int               chosen_CL,
	int               chosen_tboard,
	int               treg,
	int               mem_type,
	int               late_settings_used){

	unsigned int data;

	/* -Program RL, subsidiary registers(use spd data)                 */
	program_params_3(dramchbase, spd_data_reference, blength, chosen_AL, chosen_CL, chosen_tboard, treg, mem_type, late_settings_used); 
	program_params_4(dramchbase, spd_data_reference, blength, chosen_AL, chosen_CL, chosen_tboard, treg, mem_type, late_settings_used); 
  
	/* -Activate controller */
	if (mem_type == DDR1){
		data = MEM_CTL_DDR1_RESET_SEQ_LENGTH |
			( 0 << 4 ) |
			( 0 << 7 ) |
			( MEM_CTL_INT_RESET_DURATION_DIV_BY_16 << 8) |
			( 1 << 21 ); 
	}
	else{
		data = MEM_CTL_DDR2_RESET_SEQ_LENGTH |
			( 0 << 4 ) |
			( 0 << 7 ) |
			( MEM_CTL_INT_RESET_DURATION_DIV_BY_16 << 8) |
			( 1 << 21 ); 
	}
	phoenix_verbose_write_reg (dramchbase, DDR2_CONFIG_REG_RESET_TIMERS, data);    
	return(0);
} /* END function program_P34_activate */


void set_calibrate_range( int  blength,
			  int  dimm_presence_vect[], 
			  int  ch_id, 
			  int  *c_lower_ptr, 
			  int  *c_upper_ptr, 
			  int  *tb_lower_ptr, 
			  int  *tb_upper_ptr, 
			  int  *rx_lower_ptr, 
			  int  *rx_upper_ptr, 
			  int  *tx_lower_ptr, 
			  int  *tx_upper_ptr, 
			  int  voltage,  
			  int  treg){

  int sys_config = 0;

  sys_config = ( ( (run_eye_finder == 1)        << 2) |
                 ( (treg == 1)                  << 1) |
		 ( (voltage == 0)               << 0) );

  switch (sys_config) {

	case 0:  /* don't run eye finder, unbuffered DIMMs, high or mid voltage operation */

	    *c_lower_ptr = ef_info.ef_cmd_strb_default_unbuf_hv;
	    *c_upper_ptr = ef_info.ef_cmd_strb_default_unbuf_hv + 1;

	    *tb_lower_ptr = 1;
	    *tb_upper_ptr = 8;

	    *rx_lower_ptr = 0;
	    *rx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;

	    *tx_lower_ptr = 0;
	    *tx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;

	    break;

	case 1:  /* don't run eye finder, unbuffered DIMMs, low voltage operation */

	    *c_lower_ptr = ef_info.ef_cmd_strb_default_unbuf_lv;
	    *c_upper_ptr = ef_info.ef_cmd_strb_default_unbuf_lv + 1;

	    *tb_lower_ptr = 1;
	    *tb_upper_ptr = 8;

	    *rx_lower_ptr = 0;
	    *rx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;

	    *tx_lower_ptr = 0;
	    *tx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;

	    break;

	case 2:  /* don't run eye finder, registered DIMMs, high or mid voltage operation */
	case 3:  /* don't run eye finder, registered DIMMs, low voltage operation */

	    *c_lower_ptr = ef_info.ef_cmd_strb_default_reg;
	    *c_upper_ptr = ef_info.ef_cmd_strb_default_reg + 1;

	    *tb_lower_ptr = 1;
	    *tb_upper_ptr = 8;

	    *rx_lower_ptr = 0;
	    *rx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;

	    *tx_lower_ptr = 0;
	    *tx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;

	    break;

	case 4:
	case 5:
	case 6:
	case 7:  /* run eye finder*/

		*c_lower_ptr = 0;
		*c_upper_ptr = ef_info.ef_cmd_strb_sweep_max;

		*tb_lower_ptr = 0;
		*tb_upper_ptr = 9;

		*rx_lower_ptr = 0;
		*rx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;
		
		*tx_lower_ptr = 0;
		*tx_upper_ptr = ef_info.ef_rxtx_strb_sweep_max;

		break;

	default:
	        break;
  }
	
} /* END set_calibrate_range */

void determine_rx_tx_strobes( 	int               num_banks,			    
				int               bank_loc, 
				int               num_ranks, 
				int               rank_loc, 
				int               num_useful_col_bits, 
				int               col_loc, 
				int               num_row_bits,
				int               row_loc,
				int               chosen_AL,
				int               chosen_CL,
				int               chosen_tboard,
				int               c_strobe, 
				int               sc_strobe, 
				int               tx_lower, 
				int               tx_upper, 
				int               rx_lower, 
				int               rx_upper,
				int               treg,
				int               blength,
				phoenix_reg_t     *dramchbase,
				spd_entry         *spd_data_reference,
				enum mem_protocol mem_type,
				int               stride, 
				int               ch_id){ 
  int i;
  int j;
  int tx_strobe;
  int rx_strobe; /* Strobe portion of the value to be programmed in RX_CFG */
  int rx_value[8];  /* Actual value to be programmed in RX_CFG */

  int fail_status;

  int debug_L2 = 0;

  int late_settings_used = 0;

  /* Initialize arrays */
  for (j=0; j< 8; j++){

    for (i=0; i<TX_MAX/TX_STEP; i++){
      dqs_data_table[j].tx_sweep_data[i] = 0;
    }

    for (i=0; i<RX_MAX; i++){
      dqs_data_table[j].rx_sweep_data[i] = 0;
    }

    dqs_data_table[j].pass_region_size = 0;

  }

  for (j=0; j<stride; j++){
    if (dqs_data_table[j].chosen_rx_en > 0){ /* i.e. if NM/HL or HL/CL */
      late_settings_used = 1;
    }
  }

  for (tx_strobe=tx_lower; tx_strobe<tx_upper; tx_strobe=tx_strobe+TX_STEP){ 
    
    for (rx_strobe=rx_lower; rx_strobe<rx_upper; rx_strobe++){        

	fail_status = 0;
	      
	/* Calculate the Rx strobe value to be programmed */
	for (i = 0; i < 8; i++){
	  rx_value[i] = (0x30 << (dqs_data_table[i].chosen_rx_en + ef_info.rx_en_shift_amount)) | rx_strobe;
	}

	/* Program the strobe registers */
	program_strobe_regs (dramchbase, 
			     blength, 
			     c_strobe, 
			     sc_strobe, 
			     rx_value[0], 
			     rx_value[1], 
			     rx_value[2], 
			     rx_value[3], 
			     rx_value[4], 
			     rx_value[5], 
			     rx_value[6], 
			     rx_value[7], 
			     0,
			     0,
			     tx_strobe,
			     tx_strobe,
			     tx_strobe,
			     tx_strobe,
			     tx_strobe,
			     tx_strobe,
			     tx_strobe,
			     tx_strobe,
			     0,
			     0,
			     ch_id);

	/* Program the parameter registers, activate controller */
	program_P34_activate (dramchbase, 
			      spd_data_reference,
			      blength,
			      chosen_AL, 
			      chosen_CL, 
			      chosen_tboard, 
			      treg,
			      mem_type,
			      late_settings_used);
	
	wait_func();

	/* Test byte lanes */ 
	run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 0, stride, &fail_status, debug_L2);
	run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 1, stride, &fail_status, debug_L2);
	run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 2, stride, &fail_status, debug_L2);
	run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 3, stride, &fail_status, debug_L2);
	
	if (blength == 4){
	  run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 4, stride, &fail_status, debug_L2);
	  run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 5, stride, &fail_status, debug_L2);
	  run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 6, stride, &fail_status, debug_L2);
	  run_test(num_ranks, rank_loc, num_banks, bank_loc, num_useful_col_bits, col_loc, num_row_bits, row_loc, blength, 7, stride, &fail_status, debug_L2);
	}

	/* Update test results */
	update_results( fail_status,
			rx_strobe,
			tx_strobe, 
			stride);
	      
    } /* END for rx_strobe */
	  
  } /* END for tx_strobe */

  /* Locate the Rx, Tx, and Rx_En center values for each byte lane*/
  for (i=0; i<stride; i++){ 
    dqs_data_table[i].tx_center = (find_center(dqs_data_table[i].tx_sweep_data, TX_MAX/TX_STEP)) * TX_STEP;
    dqs_data_table[i].rx_center = find_center(dqs_data_table[i].rx_sweep_data, RX_MAX);
  }

} /* END function determine_rx_tx_strobes */

void decode_tb_delay(int tb_delay_value, int *trial_tboard_ptr, int stride){

  int dqs;

  *trial_tboard_ptr    = tb_delay_value / 3;

  for (dqs = 0; dqs < stride; dqs++){
    dqs_data_table[dqs].chosen_rx_en = tb_delay_value % 3;
  }

} /* END function decode_tb_delay */

void pick_tboard(int *chosen_tboard_ptr, int *min_pass_region_size_ptr, int stride){

  int tboard;
  int dqs;
  int i;
  unsigned long int       byte_lane_score;
  unsigned long int worst_byte_lane_score;
  unsigned long int     best_tboard_score = 0;
  
  /* Algorithm: Choose the tboard setting which has the highest minimum quality.             */
  /* "Minimum quality" is the lowest score among all the byte lanes for that tboard setting. */
  /* A byte lane's score is computed by summing the passing region sizes for that byte-lane  */
  /* corresponding to the three HE/NM, NM/HL, HL/CL settings.                                */ 

  for (tboard = 0; tboard < 3; tboard++){

    worst_byte_lane_score = 100000;

    for (dqs = 0; dqs < stride; dqs++){

      byte_lane_score = 0;

      for (i = 0; i < 3; i++){
	byte_lane_score = byte_lane_score + dqs_data_table[dqs].tb_delay_sweep_data[(tboard * 3) + i];
      }

      if (byte_lane_score < worst_byte_lane_score){
	worst_byte_lane_score = byte_lane_score;
      }

    } /* for (dqs... */

    if (worst_byte_lane_score > best_tboard_score){
      best_tboard_score = worst_byte_lane_score;
      *chosen_tboard_ptr = tboard;
    }

  } /* for (tboard... */

  *min_pass_region_size_ptr = best_tboard_score;  

} /* END function pick_tboard */

void pick_rx_en_values(int tboard){

  int dqs = 0; 
  int rx_en_scores[5];
  int temp = 0;


  /* If tboard == 0                                 */
  /* ---------------------------------------------- */
  /* rx_en_scores array index   [0] [1] [2] [3] [4] */
  /* tb_delay_sweep_data index  Zero 0   1   2   4  */
  /* chosen_rx_en value              0   1   2      */

  /* If tboard == 1                                 */
  /* ---------------------------------------------- */
  /* rx_en_scores array index   [0] [1] [2] [3] [4] */
  /* tb_delay_sweep_data index   1   3   4   5   7  */
  /* chosen_rx_en value              0   1   2      */

  /* If tboard == 2                                 */
  /* ---------------------------------------------- */
  /* rx_en_scores array index   [0] [1] [2] [3] [4] */
  /* tb_delay_sweep_data index   4   6   7   8  Zero*/
  /* chosen_rx_en value              0   1   2      */

  for (dqs = 0; dqs < 8; dqs++){

    /* Copy the relevant data over to the local array */
    if (tboard == 0){
      rx_en_scores[0] = 0;                                          /* NM/HL[tboard-1] */
      rx_en_scores[1] = dqs_data_table[dqs].tb_delay_sweep_data[0]; /* HE/NM[tboard]   */
      rx_en_scores[2] = dqs_data_table[dqs].tb_delay_sweep_data[1]; /* NM/HL[tboard]   */
      rx_en_scores[3] = dqs_data_table[dqs].tb_delay_sweep_data[2]; /* HL/CL[tboard]   */
      rx_en_scores[4] = dqs_data_table[dqs].tb_delay_sweep_data[4]; /* NM/HL[tboard+1] */
    }
    else if (tboard == 1){
      rx_en_scores[0] = dqs_data_table[dqs].tb_delay_sweep_data[1]; /* NM/HL[tboard-1] */
      rx_en_scores[1] = dqs_data_table[dqs].tb_delay_sweep_data[3]; /* HE/NM[tboard]   */
      rx_en_scores[2] = dqs_data_table[dqs].tb_delay_sweep_data[4]; /* NM/HL[tboard]   */
      rx_en_scores[3] = dqs_data_table[dqs].tb_delay_sweep_data[5]; /* HL/CL[tboard]   */
      rx_en_scores[4] = dqs_data_table[dqs].tb_delay_sweep_data[7]; /* NM/HL[tboard+1] */
    }
    else if (tboard == 2){ 
      rx_en_scores[0] = dqs_data_table[dqs].tb_delay_sweep_data[4]; /* NM/HL[tboard-1] */
      rx_en_scores[1] = dqs_data_table[dqs].tb_delay_sweep_data[6]; /* HE/NM[tboard]   */
      rx_en_scores[2] = dqs_data_table[dqs].tb_delay_sweep_data[7]; /* NM/HL[tboard]   */
      rx_en_scores[3] = dqs_data_table[dqs].tb_delay_sweep_data[8]; /* HL/CL[tboard]   */
      rx_en_scores[4] = 0;                                          /* NM/HL[tboard+1] */
    }
    else {
      printf_err("[Error]: Invalid tboard value:%d\n", tboard);
    } 

    /* Although the centering algorithm here will be forced     */
    /* to pick indices 1, 2, or 3 (corresponding to rx_en = 0,  */
    /* 1, or 2), the data stored in indices 0 and 4 is still    */
    /* required to make sure the correct center is found.       */
    temp = find_center(rx_en_scores,5);
    if (temp < 1){
      dqs_data_table[dqs].chosen_rx_en = 0;
    }
    else if (temp > 3){
      dqs_data_table[dqs].chosen_rx_en = 2;
    }
    else{
      dqs_data_table[dqs].chosen_rx_en = temp-1;
    }

  } /* END for (dqs 0 -> 7) */

} /* END function pick_rx_en_values */


void determine_tboard_rx_en_values (int               num_banks,			    
				    int               bank_loc, 
				    int               num_ranks, 
				    int               rank_loc, 
				    int               num_useful_col_bits, 
				    int               col_loc, 
				    int               num_row_bits,
				    int               row_loc,
				    int               chosen_AL,
				    int               chosen_CL,
				    int               *chosen_tboard_ptr,
				    int               c_strobe, 
				    int               sc_strobe, 
				    int               tx_lower, 
				    int               tx_upper, 
				    int               rx_lower, 
				    int               rx_upper,
				    int               tb_lower,
				    int               tb_upper,
				    int               treg,
				    int               blength,
				    phoenix_reg_t     *dramchbase,
				    spd_entry         *spd_data_reference,
				    enum mem_protocol mem_type,
				    int               stride,
				    int               *min_pass_region_size_ptr,
				    int               ch_id){

  int tb_delay_value; 
  int trial_tboard;
  int i;
  int j;

  for (i=0; i<8; i++){
    for (j=0; j< 9; j++){
      dqs_data_table[i].tb_delay_sweep_data[j] = 0;
    }
  }

  for (tb_delay_value = tb_lower; tb_delay_value < tb_upper; tb_delay_value++){

    decode_tb_delay(tb_delay_value, &trial_tboard, stride);

    determine_rx_tx_strobes(num_banks,			    
			    bank_loc, 
			    num_ranks, 
			    rank_loc, 
			    num_useful_col_bits, 
			    col_loc, 
			    num_row_bits,
			    row_loc,
			    chosen_AL,
			    chosen_CL,
			    trial_tboard,
			    c_strobe, 
			    sc_strobe, 
			    tx_lower, 
			    tx_upper, 
			    rx_lower, 
			    rx_upper,
			    treg,
			    blength,
			    dramchbase,
			    spd_data_reference,
			    mem_type,
			    stride,
			    ch_id);

    /* Copy pass_region_size values into the tb_delay_sweep_data array */
    for (i = 0; i< 8; i++){
      dqs_data_table[i].tb_delay_sweep_data[tb_delay_value] = dqs_data_table[i].pass_region_size;
    }

  } /* for tb_delay_value... */

  if (run_eye_finder == 1){
    printf("      <---------- tboard = 0 --------> <--------- tboard = 1 ---------> <--------- tboard = 2 --------->\n"); 
    for (i=0; i < 8; i++){
      printf("DQS:%d HE/NM:%4d NM/HL:%4d HL/CL:%4d HE/NM:%4d NM/HL:%4d HL/CL:%4d HE/NM:%4d NM/HL:%4d HL/CL:%4d\n",  
	     i,
	     dqs_data_table[i].tb_delay_sweep_data[0],
	     dqs_data_table[i].tb_delay_sweep_data[1],
	     dqs_data_table[i].tb_delay_sweep_data[2],
	     dqs_data_table[i].tb_delay_sweep_data[3],
	     dqs_data_table[i].tb_delay_sweep_data[4],
	     dqs_data_table[i].tb_delay_sweep_data[5],
	     dqs_data_table[i].tb_delay_sweep_data[6],
	     dqs_data_table[i].tb_delay_sweep_data[7],
	     dqs_data_table[i].tb_delay_sweep_data[8]);
    }
  }
  pick_tboard(chosen_tboard_ptr, min_pass_region_size_ptr, stride);

  pick_rx_en_values(*chosen_tboard_ptr);

  determine_rx_tx_strobes(num_banks,			    
			  bank_loc, 
			  num_ranks, 
			  rank_loc, 
			  num_useful_col_bits, 
			  col_loc, 
			  num_row_bits,
			  row_loc,
			  chosen_AL,
			  chosen_CL,
			  *chosen_tboard_ptr,
			  c_strobe, 
			  sc_strobe, 
			  tx_lower, 
			  tx_upper, 
			  rx_lower, 
			  rx_upper,
			  treg,
			  blength,
			  dramchbase,
			  spd_data_reference,
			  mem_type,
			  stride,
			  ch_id);

} /* END function determine_tboard_rx_en_values */


void compute_ecc_strobe_values(int *ecc_tx_strobe_value_ptr, int *ecc_rx_strobe_value_ptr, int stride){

  int i;
  int dqs;
  int sum = 0;
  int pop_cnt = 0;
  int max_pop_cnt = 0;
  int chosen_ecc_rx_en = 0;

  /* Program ECC Tx strobe with average of strobes for other lanes */
  for (i =0; i < stride; i++){
    sum = sum + dqs_data_table[i].tx_center;
  }
  *ecc_tx_strobe_value_ptr = sum/stride;

  /* Program ECC Rx strobe with average of strobes for other lanes */
  sum = 0;
  for (i =0; i < stride; i++){
    sum = sum + dqs_data_table[i].rx_center;
  }

  /* Program ECC RxEn value with the most popular value, NOT the average      */
  /* e.g. if four byte lanes have rx_en = 0 (i.e. HE/NM), and four byte-lanes */
  /* have rx_en = 2 (i.e. HL/CL), we should probably pick 0 or 2 for the ECC  */
  /* rx_en value, NOT 1, which is what we would get if we took the mean.      */
  for (i = 0; i < 3; i++){

    pop_cnt = 0;

    for (dqs = 0; dqs < stride; dqs++){
      if (dqs_data_table[dqs].chosen_rx_en == i){
	pop_cnt++;
      }
    }
    if (pop_cnt > max_pop_cnt){
      max_pop_cnt = pop_cnt;
      chosen_ecc_rx_en = i;
    }
  }

  *ecc_rx_strobe_value_ptr = (0x30 << (chosen_ecc_rx_en + ef_info.rx_en_shift_amount)) | (sum/stride);

} /* END function compute_ecc_strobe_values */

void calibrate_channel_strobes(

	int               num_banks,			    
	int               bank_loc, 
	int               num_ranks, 
	int               rank_loc, 
	int               num_useful_col_bits,
	int               num_useful_cols, 
	int               col_loc, 
	int               num_row_bits,
	int               row_loc,
	int               chosen_AL,
	int               chosen_CL,
	int               treg,
	int               blength,
	phoenix_reg_t     *dramchbase,
	phoenix_reg_t     *bridgebase,
	spd_entry         *spd_data_reference,
	enum mem_protocol mem_type, 
	int               ch_id,
	int               dimm_presence_vect[],
	int               voltage,
	int               chosen_period_denom, 
	char              chip_rev[]){ 

	/* Internal variables */
	int   i;

	int   stride;

	int    c_lower;  /* Lower limit of CMD strobe sweep */ 
	int    c_upper;  /* Upper limit of CMD strobe sweep */
	int   tb_lower;  /* Lower limit of tBOARD strobe sweep */ 
	int   tb_upper;  /* Upper limit of tBOARD strobe sweep */
	int   rx_lower;  /* Lower limit of Rx strobe sweep */ 
	int   rx_upper;  /* Upper limit of Rx strobe sweep */
	int   tx_lower;  /* Lower limit of Tx strobe sweep */ 
	int   tx_upper;  /* Upper limit of Tx strobe sweep */

	int   c_strobe;

	int   min_pass_region_size;

	int   late_settings_used;

	int   chosen_tboard = 0;
	int   rx_value[8];

	int   fail_status;

	int   c_center;
	int   ecc_rx_strobe_value = 0;
	int   ecc_tx_strobe_value = 0;


	int   debug = 0;
	int   seed = 0x55;

	set_wait_func_limit(chosen_period_denom, chip_rev);

	/* Initialize CMD and SLV_CMD global results arrays */

	for (i=0; i<C_MAX; i++){
		c_results[i] = 0;
	}

	/* Decide the range of values to be swept */
	set_calibrate_range(blength, dimm_presence_vect, ch_id, &c_lower, &c_upper, &tb_lower, &tb_upper, &rx_lower, &rx_upper, &tx_lower, &tx_upper, voltage, treg);
	stride = 32/blength;

	/* Set up necessary bridge BAR mappings */
	program_eye_finder_bridge_map(bridgebase, ch_id, 0);

	for (c_strobe=c_lower; c_strobe<c_upper; c_strobe++){
  
	    printf("Trying Config: CMD:0x%x\n", c_strobe);
		  
	    determine_tboard_rx_en_values(num_banks,			    
					  bank_loc, 
					  num_ranks, 
					  rank_loc, 
					  num_useful_col_bits,
					  col_loc, 
					  num_row_bits,
					  row_loc,
					  chosen_AL,
					  chosen_CL,
					  &chosen_tboard,
					  c_strobe, 
					  c_strobe, 
					  tx_lower, 
					  tx_upper, 
					  rx_lower, 
					  rx_upper,
					  tb_lower,
					  tb_upper,
					  treg,
					  blength,
					  dramchbase,
					  spd_data_reference,
					  mem_type,
					  stride,
					  &min_pass_region_size,
					  ch_id);

	    SPD_PRINTF("Smallest pass-region aggregate size among all byte-lanes:%d\n", min_pass_region_size);

	    if (min_pass_region_size > ef_info.min_pass_threshold){
	      
	      seed = ~seed;

	      /* Calculate the Rx strobe value to be programmed */
	      for (i = 0; i < 8; i++){
		rx_value[i] = (0x30 << (dqs_data_table[i].chosen_rx_en + ef_info.rx_en_shift_amount)) | dqs_data_table[i].rx_center;
	      }

	      if (run_eye_finder == 1){
		for (i=0; i<8; i++){ 
		  printf("Strobe values being used for DQS 0x%x: Tx:0x%x Rx:0x%x\n", i, 
			  dqs_data_table[i].tx_center, rx_value[i]); 
		}
	      }

	      program_strobe_regs (dramchbase, 
				   blength, 
				   c_strobe, 
				   c_strobe, 
				   rx_value[0],
				   rx_value[1],
				   rx_value[2],
				   rx_value[3],
				   rx_value[4],
				   rx_value[5],
				   rx_value[6],
				   rx_value[7],
				   0,
				   0,
				   dqs_data_table[0].tx_center,
				   dqs_data_table[1].tx_center,
				   dqs_data_table[2].tx_center,
				   dqs_data_table[3].tx_center,
				   dqs_data_table[4].tx_center,
				   dqs_data_table[5].tx_center,
				   dqs_data_table[6].tx_center,
				   dqs_data_table[7].tx_center,
				   0,
				   0,
				   ch_id);


	      late_settings_used = 0;
	      for (i = 0; i < stride; i++){
		if (dqs_data_table[i].chosen_rx_en > 0){
		  late_settings_used = 1;
		}
	      }

	      program_P34_activate (dramchbase, spd_data_reference, blength, chosen_AL, chosen_CL, chosen_tboard, treg, mem_type, late_settings_used);
	      wait_func();
      
	      /* Check stability of this configuration by running memtester */
		  fail_status = dram_page_thrash( num_banks, bank_loc, num_ranks, rank_loc, num_useful_cols, col_loc, 8, row_loc, seed, debug);

          /* -If pass, update known good configs array */
          if (!fail_status){
		      SPD_PRINTF("DRAM Page Thrash Test Passed\n");
		      c_results[c_strobe]++;
		  }
		  else{
		      printf_err("\nDRAM Page Thrash Test Failed.\n");
		  } 
	    } /* END if (min_pass_region_size) */

	} /* END for c_strobe */

	/* Locate CMD, SLV_CMD strobe center values*/
	c_center = find_center(c_results, C_MAX);

	if (c_center == 0){
	  printf_err("\n[Error]: Unable to locate working CMD and/or SLV_CMD strobe config for Channel.\n");
	} else {
        if (run_eye_finder == 1) {
            printf("\n The c-center found is: 0x%x \n", c_center);
        } else {
            SPD_PRINTF("\n The c-center found is: 0x%x \n", c_center);
        }

    }
	/* determine the tboard, rx_en, rx, tx, settings for the chosen CMD and SLV_CMD strobe values */
	determine_tboard_rx_en_values(num_banks,			    
				      bank_loc, 
				      num_ranks, 
				      rank_loc, 
				      num_useful_col_bits,
				      col_loc, 
				      num_row_bits,
				      row_loc,
				      chosen_AL,
				      chosen_CL,
				      &chosen_tboard,
				      c_center, 
				      c_center, 
				      tx_lower, 
				      tx_upper, 
				      rx_lower, 
				      rx_upper,
				      tb_lower,
				      tb_upper,
				      treg,
				      blength,
				      dramchbase,
				      spd_data_reference,
				      mem_type,
				      stride,
				      &min_pass_region_size,
				      ch_id);

	/* Calculate the Rx strobe value to be programmed */
	for (i = 0; i < 8; i++){
	  rx_value[i] = (0x30 << (dqs_data_table[i].chosen_rx_en + ef_info.rx_en_shift_amount)) | dqs_data_table[i].rx_center;
	}

	compute_ecc_strobe_values(&ecc_tx_strobe_value, &ecc_rx_strobe_value, stride);

	program_strobe_regs (dramchbase, 
			     blength, 
			     c_center, 
			     c_center, 
			     rx_value[0],
			     rx_value[1],
			     rx_value[2],
			     rx_value[3],
			     rx_value[4],
			     rx_value[5],
			     rx_value[6],
			     rx_value[7],
			     ecc_rx_strobe_value,
			     ecc_rx_strobe_value,
			     dqs_data_table[0].tx_center,
			     dqs_data_table[1].tx_center,
			     dqs_data_table[2].tx_center,
			     dqs_data_table[3].tx_center,
			     dqs_data_table[4].tx_center,
			     dqs_data_table[5].tx_center,
			     dqs_data_table[6].tx_center,
			     dqs_data_table[7].tx_center,
			     ecc_tx_strobe_value,
			     ecc_tx_strobe_value,
			     ch_id);

	/* Wipe bridge mappings */
	program_eye_finder_bridge_map(bridgebase, ch_id, 1);

	late_settings_used = 0;
	for (i = 0; i < stride; i++){
	  if (dqs_data_table[i].chosen_rx_en > 0){
	    late_settings_used = 1;
	  }
	}

	program_P34_activate (dramchbase, spd_data_reference, blength, chosen_AL, chosen_CL, chosen_tboard, treg, mem_type, late_settings_used);

	wait_func();
#if 0 
    printf("Chosen DRAM Strobe Settings:\n  "); 
    printf("C:0x%x tBOARD:0x%x \n", c_center, chosen_tboard);
    if (stride == 8){ /* 72-bit channel pairs */
        printf("RX0:0x%4x RX1:0x%4x RX2:0x%4x RX3:0x%4x RX4:0x%4x RX5:0x%4x RX6:0x%4x RX7:0x%4x RX8:0x%4x RX9:0x%4x\n", 
                rx_value[0],
                rx_value[1],
                rx_value[2],
                rx_value[3],
                rx_value[4],
                rx_value[5],
                rx_value[6],
                rx_value[7],
                ecc_rx_strobe_value, 
                ecc_rx_strobe_value);

        printf("TX0:0x%4x TX1:0x%4x TX2:0x%4x TX3:0x%4x TX4:0x%4x TX5:0x%4x TX6:0x%4x TX7:0x%4x TX8:0x%4x TX9:0x%4x \n", 
                dqs_data_table[0].tx_center,
                dqs_data_table[1].tx_center,
                dqs_data_table[2].tx_center,
                dqs_data_table[3].tx_center,
                dqs_data_table[4].tx_center,
                dqs_data_table[5].tx_center,
                dqs_data_table[6].tx_center,
                dqs_data_table[7].tx_center,
                ecc_tx_strobe_value,
                ecc_tx_strobe_value);
    } 
    else { /* 36-bit channel */
        printf("RX0:0x%4x RX1:0x%4x RX2:0x%4x RX3:0x%4x ECC:0x%4x\n", 
                rx_value[0],
                rx_value[1],
                rx_value[2],
                rx_value[3], 
                ecc_rx_strobe_value);

        printf("TX0:0x%4x TX1:0x%4x TX2:0x%4x TX3:0x%4x ECC:0x%4x\n", 
                dqs_data_table[0].tx_center,
                dqs_data_table[1].tx_center,
                dqs_data_table[2].tx_center,
                dqs_data_table[3].tx_center,
                ecc_tx_strobe_value);
    }
#endif 
	return;
  
} /* END function calibrate_channel_strobes */

#ifdef SDP_DEBUG
void read_programmed_registers(phoenix_reg_t *dramchbase, int ch_width, enum mem_protocol mem_type, int ch_id) {
	uint32_t read_data;

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_GLB_PARAMS);
	printf("GLB_PARAMS readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CMD_DELAY_STAGE_CONFIG);    
	printf("CMD_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS0_TX_DELAY_STAGE_CONFIG);
	printf("DQS0_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS1_TX_DELAY_STAGE_CONFIG);
	printf("DQS1_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS2_TX_DELAY_STAGE_CONFIG);
	printf("DQS2_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS3_TX_DELAY_STAGE_CONFIG);
	printf("DQS3_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	if ((ch_id == 0) || (ch_id == 2)){
	  read_data = phoenix_read_reg(dramchbase, DDR2_DQS8_TX_DELAY_STAGE_CONFIG);
	  printf("DQS8_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);
	}
	else{ /* 36-bits, Ch 1 or Ch3 */
	  read_data = phoenix_read_reg(dramchbase, DDR2_DQS9_TX_DELAY_STAGE_CONFIG_36_RD_ADDR);
	  printf("DQS9_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);
	}

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS0_RX_DELAY_STAGE_CONFIG);
	printf("DQS0_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS1_RX_DELAY_STAGE_CONFIG);
	printf("DQS1_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS2_RX_DELAY_STAGE_CONFIG);
	printf("DQS2_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_DQS3_RX_DELAY_STAGE_CONFIG);
	printf("DQS3_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	if ((ch_id == 0) || (ch_id == 2)){
	  read_data = phoenix_read_reg(dramchbase, DDR2_DQS8_RX_DELAY_STAGE_CONFIG);
	  printf("DQS8_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);
	}
	else{ /* 36-bits, Ch 1 or Ch3 */
	  read_data = phoenix_read_reg(dramchbase, DDR2_DQS9_RX_DELAY_STAGE_CONFIG_36_RD_ADDR);
	  printf("DQS9_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);
	}

	if (ch_width == 72){
		read_data = phoenix_read_reg(dramchbase, DDR2_SLV_CMD_DELAY_STAGE_CONFIG);
		printf("SLV_CMD_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS4_TX_DELAY_STAGE_CONFIG);
		printf("DQS4_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS5_TX_DELAY_STAGE_CONFIG);
		printf("DQS5_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS6_TX_DELAY_STAGE_CONFIG);
		printf("DQS6_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS7_TX_DELAY_STAGE_CONFIG);
		printf("DQS7_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS9_TX_DELAY_STAGE_CONFIG);
		printf("DQS9_TX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS4_RX_DELAY_STAGE_CONFIG);
		printf("DQS4_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS5_RX_DELAY_STAGE_CONFIG);
		printf("DQS5_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS6_RX_DELAY_STAGE_CONFIG);
		printf("DQS6_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS7_RX_DELAY_STAGE_CONFIG);
		printf("DQS7_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

		read_data = phoenix_read_reg(dramchbase, DDR2_DQS9_RX_DELAY_STAGE_CONFIG);
		printf("DQS9_RX_DELAY_STAGE_CONFIG readback value: 0x%x\n", read_data);

	}

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_WRITE_ODT_MASK);
	printf("WRITE_ODT_MASK readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_READ_ODT_MASK);
	printf("READ_ODT_MASK readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_ADDRESS_PARAMS);
	printf("ADDRESS_PARAMS readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_ADDRESS_PARAMS2);
	printf("ADDRESS_PARAMS2 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_PARAMS_1);
	printf("PARAMS_1 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_PARAMS_2);
	printf("PARAMS_2 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_PARAMS_3);
	printf("PARAMS_3 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_PARAMS_4);
	printf("PARAMS_4 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_PARAMS_5);
	printf("PARAMS_5 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_PARAMS_6);
	printf("PARAMS_6 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_PARAMS_7);
	printf("PARAMS_7 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_MRS);
	printf("MRS readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RDMRS);
	printf("RDMRS readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_EMRS1);
	printf("EMRS1 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_TX0_THERM_CTRL_RD_ADDR);
	printf("TX0_THERM_CTRL readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_RX0_THERM_CTRL_RD_ADDR);
	printf("RX0_THERM_CTRL readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RESET_CMD1);
	printf("RESET_CMD1 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RESET_CMD2);    
	printf("RESET_CMD2 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RESET_CMD3);    
	printf("RESET_CMD3 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RESET_CMD4);
	printf("RESET_CMD4 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RESET_CMD5);
	printf("RESET_CMD5 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RESET_CMD6);
	printf("RESET_CMD6 readback value: 0x%x\n", read_data);

	read_data = phoenix_read_reg(dramchbase, DDR2_CONFIG_REG_RESET_TIMERS);
	printf("RESET_TIMERS readback value: 0x%x\n", read_data);

};
#endif
